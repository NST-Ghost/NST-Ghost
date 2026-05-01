/// main.rs — Translation mod POC for Unity serialized assets.
///
/// Workflow:
///   1. `dump`  — parse assets file, write all strings to strings.json
///   2. `patch` — read strings.json (translated), rebuild assets file
///
/// Usage:
///   cargo run -- dump  resources.assets strings.json
///   # edit strings.json (translate values)
///   cargo run -- patch resources.assets strings.json resources_patched.assets

mod reader;
mod serialized_file;
mod object;
mod type_tree;
mod writer;
mod patcher;

use anyhow::Result;
use indexmap::IndexMap;
use object::ObjectReader;
use serialized_file::SerializedFile;
use type_tree::UnityValue;
use writer::write_typetree;
use patcher::{collect_patch_points_v2, rebuild_file};
use std::collections::HashMap;

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 4 {
        eprintln!("Usage:");
        eprintln!("  dump  <assets>  <out.json>");
        eprintln!("  patch <assets>  <strings.json>  <out.assets>");
        std::process::exit(1);
    }
    match args[1].as_str() {
        "dump"  => cmd_dump (&args[2], &args[3]),
        "patch" => cmd_patch(&args[2], &args[3], &args[4]),
        other   => { eprintln!("Unknown command: {}", other); std::process::exit(1); }
    }
}

// ─── dump ─────────────────────────────────────────────────────────────────────

fn cmd_dump(assets_path: &str, out_json: &str) -> Result<()> {
    let data = std::fs::read(assets_path)?;
    let sf   = SerializedFile::read(&data)?;

    println!("Unity version : {}", sf.unity_version);
    println!("File version  : {}", sf.header.version);
    println!("Objects       : {}", sf.objects.len());
    println!("Types in tree : {}", sf.types.len());

    // Collect all strings keyed by "path_id/field_path"
    // e.g.  "1234/m_Name" or "1234/m_Script/m_Name"
    let mut strings: IndexMap<String, StringEntry> = IndexMap::new();

    let obj_endian = if sf.header.endian == 0 {
        reader::Endian::Little
    } else {
        reader::Endian::Big
    };

    for info in &sf.objects {
        let obj_reader = ObjectReader::new(
            info.clone(),
            &data,
            sf.header.data_offset,
            obj_endian,
        );
        match obj_reader.read_typetree(&sf.types) {
            Ok(Some(value)) => {
                let prefix = format!("{}#{}", info.path_id, info.class_id);
                collect_strings(&prefix, &value, &mut strings);
            }
            Ok(None) => {}
            Err(e) => eprintln!("  WARN path_id={} parse error: {}", info.path_id, e),
        }
    }

    let json = serde_json::to_string_pretty(&strings)?;
    std::fs::write(out_json, &json)?;
    println!("Wrote {} string entries to {}", strings.len(), out_json);
    Ok(())
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
struct StringEntry {
    pub original: String,
    pub translated: String,
}

/// Walk a UnityValue tree, collecting all String leaves into a flat map.
/// Key format: "<prefix>/<field_name>" — unique, stable across runs.
fn collect_strings(prefix: &str, value: &UnityValue, out: &mut IndexMap<String, StringEntry>) {
    match value {
        UnityValue::String(s) => {
            if !s.is_empty() {
                out.insert(prefix.to_string(), StringEntry {
                    original:   s.clone(),
                    translated: s.clone(), // user will fill this in
                });
            }
        }
        UnityValue::Map(map) => {
            for (k, v) in map {
                collect_strings(&format!("{}/{}", prefix, k), v, out);
            }
        }
        UnityValue::Array(arr) => {
            for (i, v) in arr.iter().enumerate() {
                collect_strings(&format!("{}[{}]", prefix, i), v, out);
            }
        }
        _ => {}
    }
}

// ─── patch ────────────────────────────────────────────────────────────────────

fn cmd_patch(assets_path: &str, strings_json: &str, out_path: &str) -> Result<()> {
    let data     = std::fs::read(assets_path)?;
    let sf       = SerializedFile::read(&data)?;
    let json_str = std::fs::read_to_string(strings_json)?;
    let entries: IndexMap<String, StringEntry> = serde_json::from_str(&json_str)?;

    // Build a nested diff map:  path_id → list of (field_path, new_string)
    // Key format: "<path_id>#<class_id>/<fields...>"
    let mut diff: HashMap<i64, Vec<(String, String)>> = HashMap::new();
    for (key, entry) in &entries {
        if entry.original == entry.translated { continue; } // unchanged
        // split on first '/'
        if let Some(slash) = key.find('/') {
            let id_part    = &key[..slash];
            let field_path = &key[slash + 1..];
            // id_part is "<path_id>#<class_id>"
            if let Some(hash) = id_part.find('#') {
                if let Ok(path_id) = id_part[..hash].parse::<i64>() {
                    diff.entry(path_id)
                        .or_default()
                        .push((field_path.to_string(), entry.translated.clone()));
                }
            }
        }
    }

    if diff.is_empty() {
        println!("No translations found (all 'translated' fields match 'original'). Nothing to do.");
        return Ok(());
    }
    println!("Patching {} objects...", diff.len());

    let obj_endian = if sf.header.endian == 0 {
        reader::Endian::Little
    } else {
        reader::Endian::Big
    };
    let meta_endian = obj_endian; // same endian for both in v14+

    // Re-serialize objects that have changes.
    let mut patched_blobs: HashMap<i64, Vec<u8>> = HashMap::new();

    for info in &sf.objects {
        let changes = match diff.get(&info.path_id) {
            Some(c) => c,
            None    => continue,
        };
        let obj_reader = ObjectReader::new(
            info.clone(), &data, sf.header.data_offset, obj_endian,
        );
        let tree = match sf.types.get(&info.type_id) {
            Some(t) => t,
            None    => {
                eprintln!("  WARN path_id={}: no TypeTree, skipping", info.path_id);
                continue;
            }
        };
        if tree.nodes.is_empty() {
            eprintln!("  WARN path_id={}: empty TypeTree, skipping", info.path_id);
            continue;
        }
        let mut value = match obj_reader.read_typetree(&sf.types)? {
            Some(v) => v,
            None    => continue,
        };

        // Apply all changes for this object.
        let mut applied = 0usize;
        for (field_path, new_str) in changes {
            if apply_string_patch(&mut value, field_path, new_str) {
                applied += 1;
            } else {
                eprintln!("  WARN path_id={}: field '{}' not found", info.path_id, field_path);
            }
        }
        if applied == 0 { continue; }

        // Re-serialize.
        match write_typetree(&value, &tree.nodes, obj_endian) {
            Ok(blob) => {
                println!("  patched path_id={} {} fields ({} → {} bytes)",
                    info.path_id, applied, info.byte_size, blob.len());
                patched_blobs.insert(info.path_id, blob);
            }
            Err(e) => eprintln!("  ERROR path_id={}: write failed: {}", info.path_id, e),
        }
    }

    if patched_blobs.is_empty() {
        println!("No objects were successfully patched.");
        return Ok(());
    }

    // Collect patch points (object table positions).
    let patch_points = collect_patch_points_v2(
        &data,
        sf.header.version,
        sf.header.endian,
        sf.has_type_tree,
    )?;

    // Rebuild the file.
    let new_file = rebuild_file(
        &data,
        &patch_points,
        &sf.objects,
        sf.header.data_offset,
        sf.header.version,
        meta_endian,
        obj_endian,
        &patched_blobs,
    )?;

    std::fs::write(out_path, &new_file)?;
    println!("Wrote patched file to {} ({} bytes)", out_path, new_file.len());
    Ok(())
}

/// Navigate a UnityValue tree by slash-separated field path and replace the
/// leaf String.  Array indices are written as `[N]`.
/// Returns true if the field was found and replaced.
fn apply_string_patch(value: &mut UnityValue, path: &str, new_str: &str) -> bool {
    if path.is_empty() {
        if let UnityValue::String(s) = value {
            *s = new_str.to_string();
            return true;
        }
        return false;
    }

    // Split first segment
    let (head, tail) = match path.find('/') {
        Some(i) => (&path[..i], &path[i + 1..]),
        None    => (path, ""),
    };

    match value {
        UnityValue::Map(map) => {
            if let Some(child) = map.get_mut(head) {
                return apply_string_patch(child, tail, new_str);
            }
            false
        }
        UnityValue::Array(arr) => {
            // head should be "[N]"
            if head.starts_with('[') && head.ends_with(']') {
                if let Ok(idx) = head[1..head.len()-1].parse::<usize>() {
                    if idx < arr.len() {
                        return apply_string_patch(&mut arr[idx], tail, new_str);
                    }
                }
            }
            false
        }
        _ => false,
    }
}
