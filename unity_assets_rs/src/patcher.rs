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
    objects:      &[crate::object::ObjectInfo],
    data_offset:  u64,
    file_version: u32,
    meta_endian:  Endian,
    obj_endian:   Endian,
    patches:      &HashMap<i64, Vec<u8>>,
) -> Result<Vec<u8>> {
    // 1. เตรียมข้อมูล Object ทั้งหมด (ทั้งเก่าและใหม่)
    let mut out_meta = file_data[..data_offset as usize].to_vec();
    let mut new_data: Vec<u8> = Vec::new();

    // เรียงลำดับ Object เดิมตาม byte_start
    let mut order: Vec<usize> = (0..objects.len()).collect();
    order.sort_by_key(|&i| objects[i].byte_start);

    let pp_map: HashMap<i64, &ObjectPatchPoint> =
        patch_points.iter().map(|p| (p.path_id, p)).collect();

    for &oi in &order {
        let obj = &objects[oi];
        let pp  = pp_map.get(&obj.path_id).ok_or_else(|| anyhow::anyhow!("no patch point"))?;

        let new_start = new_data.len() as u64;
        let obj_bytes: &[u8] = if let Some(new_bytes) = patches.get(&obj.path_id) {
            new_bytes
        } else {
            let s = (data_offset + obj.byte_start) as usize;
            let e = s + obj.byte_size as usize;
            &file_data[s..e]
        };

        let new_size = obj_bytes.len() as u32;
        new_data.extend_from_slice(obj_bytes);

        // ทำ alignment สำหรับก้อนข้อมูลถัดไป (สำคัญมากสำหรับ Unity)
        let padding = (4 - (new_data.len() % 4)) % 4;
        for _ in 0..padding { new_data.push(0); }

        // Patch ตำแหน่งใน Object Table
        let sp = pp.byte_start_pos as usize;
        if pp.start_is_u64 {
            write_u64_at(&mut out_meta, sp, new_start, meta_endian);
        } else {
            write_u32_at(&mut out_meta, sp, new_start as u32, meta_endian);
        }
        write_u32_at(&mut out_meta, pp.byte_size_pos as usize, new_size, meta_endian);
    }

    // 2. รวม Header/Meta กับ Data ใหม่
    let mut final_file = out_meta;
    final_file.extend_from_slice(&new_data);

    // 3. อัปเดต File Size ใน Header
    let new_file_size = final_file.len() as u64;
    if file_version >= 22 {
        final_file[24..32].copy_from_slice(&new_file_size.to_be_bytes());
    } else {
        final_file[4..8].copy_from_slice(&(new_file_size as u32).to_be_bytes());
    }

    Ok(final_file)
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
    if type_count > 10_000 { anyhow::bail!("too many types"); }
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
    if node_count > 100_000 { anyhow::bail!("too many nodes"); }
    let node_size: u64 = if version >= 19 { 32 } else { 24 };
    r.skip((node_count as i64) * (node_size as i64))?;
    r.skip(str_buf_sz as i64)?;
    Ok(())
}

// ─── inject_object helper ─────────────────────────────────────────────────────

/// Injects a new object into the metadata and data sections.
pub fn inject_object(
    file_data: &[u8],
    new_obj_bytes: &[u8],
    class_id: i32,
) -> Result<Vec<u8>> {
    let sf = crate::SerializedFile::read(file_data)?;
    let _meta_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    
    let next_path_id = sf.objects.iter().map(|o| o.path_id).max().unwrap_or(0) + 1;
    
    let _new_info = crate::object::ObjectInfo {
        path_id: next_path_id,
        byte_start: 0,
        byte_size: new_obj_bytes.len() as u32,
        type_id: class_id,
        class_id: class_id as i16,
        is_stripped: false,
    };

    anyhow::bail!("Injection logic requires metadata rewriting - coming soon")
}
