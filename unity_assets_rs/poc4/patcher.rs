/// patcher.rs — rebuild a Unity serialized file with modified object data.
///
/// Strategy (no full re-serialization of TypeTree needed):
///   1. During metadata parsing, record the *file offset* of each object's
///      `byte_start` and `byte_size` fields.
///   2. Build a fresh data section by concatenating (possibly modified) object blobs.
///   3. Patch the recorded offsets directly in the byte vector, then update
///      `file_size` in the header.
///
/// This means the TypeTree, ExternalRefs, etc. are never touched — only the
/// object table entries and the data block change.

use crate::reader::{BinaryReader, Endian};
use crate::type_tree::TypeTree;
use anyhow::Result;
use std::collections::HashMap;

// ─── ObjectPatchPoint ─────────────────────────────────────────────────────────

/// Remembers exactly where in the raw file bytes each object's `byte_start`
/// and `byte_size` are stored, so we can overwrite them after rebuilding.
#[derive(Debug, Clone)]
pub struct ObjectPatchPoint {
    pub path_id:        i64,
    pub byte_start_pos: u64,   // file offset of the byte_start field
    pub byte_size_pos:  u64,   // file offset of the byte_size field
    pub start_is_u64:   bool,  // true if version >= 22 (byte_start is u64)
}

impl ObjectPatchPoint {
    /// Walk the object table *without* storing objects — just record positions.
    pub fn read_table(reader: &mut BinaryReader, version: u32) -> Result<Vec<ObjectPatchPoint>> {
        let count = reader.read_u32()?;
        if count > 1_000_000 {
            anyhow::bail!("read patch points: too many objects: {}", count);
        }
        let mut points = Vec::with_capacity(count as usize);

        for _ in 0..count {
            if version >= 14 { reader.align(4)?; }

            let path_id = if version >= 14 {
                reader.read_i64()?
            } else {
                reader.read_i32()? as i64
            };

            let byte_start_pos = reader.tell();
            let start_is_u64 = version >= 22;
            if start_is_u64 { reader.read_u64()?; } else { reader.read_u32()?; }

            let byte_size_pos = reader.tell();
            reader.read_u32()?; // byte_size

            reader.read_i32()?; // type_id
            if version < 16 { reader.read_i16()?; }           // class_id
            if version >= 11 && version <= 15 { reader.read_u16()?; } // is_destroyed

            points.push(ObjectPatchPoint { path_id, byte_start_pos, byte_size_pos, start_is_u64 });
        }
        Ok(points)
    }
}

// ─── patch_write helpers ──────────────────────────────────────────────────────

fn write_u32_at(buf: &mut Vec<u8>, pos: usize, v: u32, endian: Endian) {
    let bytes = match endian {
        Endian::Little => v.to_le_bytes(),
        Endian::Big    => v.to_be_bytes(),
    };
    buf[pos..pos + 4].copy_from_slice(&bytes);
}

fn write_u64_at(buf: &mut Vec<u8>, pos: usize, v: u64, endian: Endian) {
    let bytes = match endian {
        Endian::Little => v.to_le_bytes(),
        Endian::Big    => v.to_be_bytes(),
    };
    buf[pos..pos + 8].copy_from_slice(&bytes);
}

// ─── main rebuild function ────────────────────────────────────────────────────

/// Rebuild `file_data` with `patches` applied.
///
/// `patches` maps `path_id → new object bytes`.
/// Objects not in `patches` are copied verbatim from the original.
///
/// Returns the fully patched file as a `Vec<u8>`.
pub fn rebuild_file(
    file_data:    &[u8],
    patch_points: &[ObjectPatchPoint],
    objects:      &[crate::object::ObjectInfo],   // original layout info
    data_offset:  u64,
    file_version: u32,
    meta_endian:  Endian,
    obj_endian:   Endian,
    patches:      &HashMap<i64, Vec<u8>>,
) -> Result<Vec<u8>> {
    // Start with a copy of the original file (we'll overwrite only changed parts).
    let mut out = file_data.to_vec();

    // Sort objects by original byte_start to process in order.
    let mut order: Vec<usize> = (0..objects.len()).collect();
    order.sort_by_key(|&i| objects[i].byte_start);

    // Build a map from path_id → patch_point for quick lookup.
    let pp_map: HashMap<i64, &ObjectPatchPoint> =
        patch_points.iter().map(|p| (p.path_id, p)).collect();

    // ── build new data section ────────────────────────────────────────────────
    let mut new_data: Vec<u8> = Vec::new();

    for &oi in &order {
        let obj = &objects[oi];
        let pp  = match pp_map.get(&obj.path_id) {
            Some(p) => p,
            None    => anyhow::bail!("no patch point for path_id={}", obj.path_id),
        };

        let new_start = new_data.len() as u64;

        // Choose object bytes: patched or original.
        let obj_bytes: &[u8] = if let Some(new_bytes) = patches.get(&obj.path_id) {
            new_bytes
        } else {
            let s = (data_offset + obj.byte_start) as usize;
            let e = s + obj.byte_size as usize;
            if e > file_data.len() {
                anyhow::bail!("original object OOB path_id={}", obj.path_id);
            }
            &file_data[s..e]
        };

        let new_size = obj_bytes.len() as u32;
        new_data.extend_from_slice(obj_bytes);

        // ── patch byte_start in the object table ───────────────────────────
        let sp = pp.byte_start_pos as usize;
        if pp.start_is_u64 {
            write_u64_at(&mut out, sp, new_start, meta_endian);
        } else {
            write_u32_at(&mut out, sp, new_start as u32, meta_endian);
        }

        // ── patch byte_size ────────────────────────────────────────────────
        write_u32_at(&mut out, pp.byte_size_pos as usize, new_size, meta_endian);
    }

    // ── replace data section ──────────────────────────────────────────────────
    let do_ = data_offset as usize;
    out.truncate(do_);
    out.extend_from_slice(&new_data);

    // ── patch file_size in the header (always Big-endian) ─────────────────────
    let new_file_size = out.len() as u64;
    if file_version >= 22 {
        // file_size is a u64 at offset 24
        let bytes = new_file_size.to_be_bytes();
        out[24..32].copy_from_slice(&bytes);
    } else {
        // file_size is a u32 at offset 4
        let bytes = (new_file_size as u32).to_be_bytes();
        out[4..8].copy_from_slice(&bytes);
    }

    let _ = (obj_endian, file_version); // used above, suppress lint
    Ok(out)
}

// ─── convenience: parse patch points from raw metadata ───────────────────────

/// Re-read the metadata section and collect ObjectPatchPoints.
/// Call this once after `SerializedFile::read`.
pub fn collect_patch_points(data: &[u8], version: u32, endian_byte: u8) -> Result<Vec<ObjectPatchPoint>> {
    let meta_endian = if endian_byte == 0 { Endian::Little } else { Endian::Big };
    let meta_start: u64 = if version >= 22 { 48 } else { 16 };

    let mut reader = BinaryReader::new(data, meta_endian);
    reader.seek(meta_start)?;

    reader.read_fixed_string(12)?; // unity_version
    reader.read_u32()?;            // target_platform
    reader.read_u8()?;             // has_type_tree

    // Skip TypeTree (we just need to reach the object table)
    skip_type_tree(&mut reader, version)?;

    ObjectPatchPoint::read_table(&mut reader, version)
}

/// Skip over the TypeTree block without parsing it fully.
/// Must match TypeTree::read exactly.
fn skip_type_tree(reader: &mut BinaryReader, version: u32) -> Result<()> {
    let count = reader.read_u32()?;
    let has_type_tree_byte = {
        // We already consumed has_type_tree above — need to know it.
        // It was stored before this call. We'll re-read from context.
        // Actually we can't go back. Let me restructure.
        // For now, peek: this function is only called after reading has_type_tree.
        // The caller must pass it in.
        true // placeholder — see note below
    };
    let _ = has_type_tree_byte; // see collect_patch_points_v2 below

    // This placeholder approach won't work cleanly — see the improved version below.
    let _ = count;
    anyhow::bail!("use collect_patch_points_v2 instead")
}

/// Improved version that takes `has_type_tree` explicitly.
pub fn collect_patch_points_v2(
    data:          &[u8],
    version:       u32,
    endian_byte:   u8,
    has_type_tree: bool,
) -> Result<Vec<ObjectPatchPoint>> {
    let meta_endian = if endian_byte == 0 { Endian::Little } else { Endian::Big };
    let meta_start: u64 = if version >= 22 { 48 } else { 16 };

    let mut r = BinaryReader::new(data, meta_endian);
    r.seek(meta_start)?;

    r.read_fixed_string(12)?; // unity_version
    r.read_u32()?;            // target_platform
    r.read_u8()?;             // has_type_tree flag (already known)

    // ── Skip TypeTree ──────────────────────────────────────────────────────
    let type_count = r.read_u32()?;
    for _ in 0..type_count {
        let class_id = r.read_i32()?;
        if version >= 16 { r.read_u8()?; }                             // is_stripped
        if version >= 17 { r.read_i16()?; }                            // script_index
        if version >= 13 {
            let has_guid = if version >= 16 { class_id == 114 } else { class_id < 0 };
            if has_guid { r.skip(16)?; }
            r.skip(16)?; // type_hash
        }
        if has_type_tree {
            skip_nodes(&mut r, version)?;
        }
    }

    // ref-types block (v21+, only when has_type_tree)
    if version >= 21 && has_type_tree {
        let ref_count = r.read_u32()?;
        for _ in 0..ref_count {
            let class_id = r.read_i32()?;
            if version >= 16 { r.read_u8()?; }
            if version >= 17 { r.read_i16()?; }
            if version >= 13 {
                if class_id == 114 { r.skip(16)?; }
                r.skip(16)?;
            }
            if has_type_tree { skip_nodes(&mut r, version)?; }
        }
    }

    // ── Now we're at the object table ──────────────────────────────────────
    ObjectPatchPoint::read_table(&mut r, version)
}

fn skip_nodes(r: &mut BinaryReader, version: u32) -> Result<()> {
    let node_count = r.read_u32()?;
    let str_buf_sz = r.read_u32()?;
    // Each node: version(2) level(1) is_array(1) type_idx(4) name_idx(4)
    //            byte_size(4) index(4) meta_flag(4) [ref_hash(8) if v>=19]
    let node_size: u64 = if version >= 19 { 32 } else { 24 };
    r.skip((node_count as i64) * (node_size as i64))?;
    r.skip(str_buf_sz as i64)?;
    Ok(())
}
