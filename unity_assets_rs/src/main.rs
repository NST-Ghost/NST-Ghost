/// main.rs — Translation mod POC for Unity serialized assets.
use anyhow::Result;
use indexmap::IndexMap;
use std::collections::HashMap;
use unity_assets_rs::{
    SerializedFile, ObjectReader, UnityValue, Endian,
    write_typetree, rebuild_file, collect_patch_points_v2
};

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        print_usage();
        std::process::exit(1);
    }
    match args[1].as_str() {
        "dump"  => {
            if args.len() < 4 { print_usage(); std::process::exit(1); }
            cmd_dump (&args[2], &args[3])
        },
        "patch" => {
            if args.len() < 5 { print_usage(); std::process::exit(1); }
            cmd_patch(&args[2], &args[3], &args[4])
        },
        "info" => {
            if args.len() < 3 { print_usage(); std::process::exit(1); }
            cmd_info(&args[2])
        }
        other   => { eprintln!("Unknown command: {}", other); print_usage(); std::process::exit(1); }
    }
}

fn print_usage() {
    eprintln!("Usage:");
    eprintln!("  info  <assets>");
    eprintln!("  dump  <assets>  <out.json>");
    eprintln!("  patch <assets>  <strings.json>  <out.assets>");
}

// ─── info ─────────────────────────────────────────────────────────────────────

fn cmd_info(path: &str) -> Result<()> {
    let data = std::fs::read(path)?;
    let sf = SerializedFile::read(&data)?;
    println!("--- Serialized File Header ---");
    println!("Unity Version:   {}", sf.unity_version);
    println!("Objects Count:   {}", sf.objects.len());
    
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    println!("\nSearching for Font/SDF objects...");
    for info in &sf.objects {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        if let Some(name) = reader.peek_name() {
            if name.to_lowercase().contains("font") || name.to_lowercase().contains("sdf") {
                println!("  [ID: {}] Class: {} -> {}", info.path_id, info.class_id, name);
            }
        }
    }
    Ok(())
}

// ─── dump ─────────────────────────────────────────────────────────────────────

fn cmd_dump(assets_path: &str, out_json: &str) -> Result<()> {
    let data = std::fs::read(assets_path)?;
    let sf   = SerializedFile::read(&data)?;

    println!("Scanning {} objects...", sf.objects.len());

    let mut strings: IndexMap<String, StringEntry> = IndexMap::new();
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    for info in &sf.objects {
        let obj_reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
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

fn collect_strings(prefix: &str, value: &UnityValue, out: &mut IndexMap<String, StringEntry>) {
    match value {
        UnityValue::String(s) => {
            if !s.is_empty() {
                out.insert(prefix.to_string(), StringEntry {
                    original:   s.clone(),
                    translated: s.clone(),
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

    let mut diff: HashMap<i64, Vec<(String, String)>> = HashMap::new();
    for (key, entry) in &entries {
        if entry.original == entry.translated { continue; }
        if let Some(slash) = key.find('/') {
            let id_part    = &key[..slash];
            let field_path = &key[slash + 1..];
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
        println!("No translations found. Nothing to do.");
        return Ok(());
    }
    println!("Patching {} objects...", diff.len());

    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    let meta_endian = obj_endian;
    let mut patched_blobs: HashMap<i64, Vec<u8>> = HashMap::new();

    for info in &sf.objects {
        let changes = match diff.get(&info.path_id) {
            Some(c) => c,
            None    => continue,
        };
        let obj_reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
        let tree = match sf.types.get(&info.type_id) {
            Some(t) => t,
            None    => continue,
        };
        if tree.nodes.is_empty() { continue; }
        
        let mut value = match obj_reader.read_typetree(&sf.types)? {
            Some(v) => v,
            None    => continue,
        };

        let mut applied = 0;
        for (field_path, new_str) in changes {
            if apply_string_patch(&mut value, field_path, new_str) {
                applied += 1;
            }
        }
        if applied == 0 { continue; }

        match write_typetree(&value, &tree.nodes, obj_endian) {
            Ok(blob) => {
                patched_blobs.insert(info.path_id, blob);
            }
            Err(e) => eprintln!("  ERROR path_id={}: write failed: {}", info.path_id, e),
        }
    }

    let patch_points = collect_patch_points_v2(&data, sf.header.version, sf.header.endian, sf.has_type_tree)?;
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

fn apply_string_patch(value: &mut UnityValue, path: &str, new_str: &str) -> bool {
    if path.is_empty() {
        if let UnityValue::String(s) = value {
            *s = new_str.to_string();
            return true;
        }
        return false;
    }

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
