mod extractor;
mod patcher;
mod rpa;
mod translator;

use extractor::TextBlock;
use serde_json::json;
use std::collections::HashMap;
use std::fs;
use std::path::Path;

const REFERENCE_LANG: &str = "Vietnamese";
const DEFAULT_TARGET: &str = "Thai";

fn rpa_native_index(archive_path: &str) -> Vec<rpa::RpaEntry> {
    match rpa::parse_rpa(archive_path) {
        Ok(e) => e,
        Err(err) => {
            eprintln!("[error] {}", err);
            std::process::exit(1);
        }
    }
}

fn read_entry(archive_path: &str, entry: &rpa::RpaEntry) -> Vec<u8> {
    rpa::read_rpa_file(archive_path, entry).unwrap_or_default()
}

fn to_str(bytes: &[u8]) -> String {
    let b = if bytes.starts_with(b"\xef\xbb\xbf") {
        &bytes[3..]
    } else {
        bytes
    };
    String::from_utf8_lossy(b).replace("\r\n", "\n")
}

fn flag_value(args: &[String], flag: &str) -> Option<String> {
    args.windows(2).find(|w| w[0] == flag).map(|w| w[1].clone())
}

// ── Stage 1: extract ──────────────────────────────────────────────────────

fn cmd_extract(archive: &str, output: &str, base_only: bool) {
    eprintln!("[extract] Parsing: {}", archive);
    let entries = rpa_native_index(archive);
    eprintln!("[extract] {} files in archive", entries.len());

    let scripts: Vec<&rpa::RpaEntry> = entries
        .iter()
        .filter(|e| e.path.ends_with(".rpy") && (!base_only || !e.path.contains("tl/")))
        .collect();

    eprintln!("[extract] {} .rpy files", scripts.len());

    let mut all_blocks: Vec<TextBlock> = Vec::new();
    for entry in scripts {
        let bytes = read_entry(archive, entry);
        if bytes.is_empty() {
            continue;
        }
        let blocks = extractor::extract_from_bytes(&entry.path, to_str(&bytes).as_bytes());
        eprintln!("  {} → {} blocks", entry.path, blocks.len());
        all_blocks.extend(blocks);
    }

    let payload = json!({
        "archive": archive,
        "total": all_blocks.len(),
        "dialogue": all_blocks.iter().filter(|b| b.kind=="dialogue").count(),
        "narration": all_blocks.iter().filter(|b| b.kind=="narration").count(),
        "choices":   all_blocks.iter().filter(|b| b.kind=="choice").count(),
        "blocks": all_blocks,
    });

    fs::write(output, serde_json::to_string_pretty(&payload).unwrap()).unwrap();
    eprintln!("[extract] {} blocks → {}", all_blocks.len(), output);
}

// ── Stage 2: translate ────────────────────────────────────────────────────

fn cmd_translate(strings_json: &str, target_lang: &str, api_key: &str, output: &str) {
    let raw = fs::read_to_string(strings_json).expect("cannot read strings JSON");
    let mut parsed: serde_json::Value = serde_json::from_str(&raw).expect("invalid JSON");
    let blocks: Vec<TextBlock> = serde_json::from_value(parsed["blocks"].take()).unwrap();

    let trans_map = translator::translate_blocks(&blocks, target_lang, api_key).unwrap();

    let translated: Vec<TextBlock> = blocks
        .into_iter()
        .map(|mut b| {
            if let Some(t) = trans_map.get(&b.source) {
                b.translation = Some(t.clone());
            }
            b
        })
        .collect();

    let n_done = translated
        .iter()
        .filter(|b| b.translation.is_some())
        .count();
    let payload = json!({
        "archive": parsed["archive"],
        "target_lang": target_lang,
        "total": translated.len(),
        "translated": n_done,
        "blocks": translated,
    });

    fs::write(output, serde_json::to_string_pretty(&payload).unwrap()).unwrap();
    eprintln!("[translate] {}/{} translated → {}", n_done, n_done, output);
}

// ── Stage 3: patch ────────────────────────────────────────────────────────

fn cmd_patch(translated_json: &str, archive: &str, out_dir: &str) {
    let raw = fs::read_to_string(translated_json).expect("cannot read JSON");
    let parsed: serde_json::Value = serde_json::from_str(&raw).unwrap();
    let target_lang = parsed["target_lang"].as_str().unwrap_or(DEFAULT_TARGET);
    let blocks: Vec<TextBlock> = serde_json::from_value(parsed["blocks"].clone()).unwrap();

    let trans_map = patcher::build_translation_map(&blocks);
    eprintln!("[patch] {} translations", trans_map.len());

    let entries = rpa_native_index(archive);
    let tl_dir = Path::new(out_dir).join("game").join("tl").join(target_lang);
    fs::create_dir_all(&tl_dir).unwrap();
    eprintln!("[patch] Output: {}", tl_dir.display());

    let ref_files: Vec<&rpa::RpaEntry> = entries
        .iter()
        .filter(|e| {
            e.path.starts_with(&format!("tl/{}/", REFERENCE_LANG)) && e.path.ends_with(".rpy")
        })
        .collect();

    eprintln!(
        "[patch] {} reference tl files ({})",
        ref_files.len(),
        REFERENCE_LANG
    );

    for ref_entry in ref_files {
        let ref_bytes = read_entry(archive, ref_entry);
        if ref_bytes.is_empty() {
            continue;
        }
        let ref_text = to_str(&ref_bytes);

        let char_defines: HashMap<String, String> = HashMap::new();
        let patched = patcher::patch_tl_file(
            &ref_text,
            REFERENCE_LANG,
            target_lang,
            &trans_map,
            &char_defines,
        );

        let filename = ref_entry
            .path
            .strip_prefix(&format!("tl/{}/", REFERENCE_LANG))
            .unwrap_or(&ref_entry.path);

        let out_path = tl_dir.join(filename);
        fs::write(&out_path, patched.as_bytes()).unwrap();
        eprintln!("  {}", out_path.display());
    }

    eprintln!(
        "[patch] Done! Copy {} into your game/tl/ folder",
        tl_dir.display()
    );
    eprintln!("[patch] In-game: Preferences → Language → {}", target_lang);
}

// ── Info ──────────────────────────────────────────────────────────────────

fn cmd_info(archive: &str) {
    let entries = rpa_native_index(archive);
    println!("Archive: {}", archive);
    println!("Files: {}", entries.len());

    let mut langs: HashMap<&str, usize> = HashMap::new();
    let mut ext_count: HashMap<&str, usize> = HashMap::new();
    for e in &entries {
        let ext = e.path.rsplit('.').next().unwrap_or("?");
        *ext_count.entry(ext).or_insert(0) += 1;
        if e.path.starts_with("tl/") {
            let lang = e.path.split('/').nth(1).unwrap_or("?");
            *langs.entry(lang).or_insert(0) += 1;
        }
    }

    println!("\nFonts found:");
    for e in &entries {
        if e.path.to_lowercase().ends_with(".ttf") {
            println!("  {}", e.path);
        }
    }

    println!("\nBy extension:");
    let mut exts: Vec<_> = ext_count.iter().collect();
    exts.sort_by(|a, b| b.1.cmp(a.1));
    for (ext, n) in exts {
        println!("  .{:<10} {}", ext, n);
    }

    println!("\nExisting translations:");
    let mut ls: Vec<_> = langs.iter().collect();
    ls.sort_by(|a, b| b.1.cmp(a.1));
    for (lang, n) in ls {
        println!("  {:<25} {} files", lang, n);
    }
}

fn cmd_search(archive: &str, pattern: &str) {
    let entries = rpa_native_index(archive);
    for entry in entries.iter().filter(|e| e.path.ends_with(".rpy")) {
        let bytes = read_entry(archive, entry);
        let text = to_str(&bytes);
        if text.contains(pattern) {
            println!("Found in {}:", entry.path);
            for (i, line) in text.lines().enumerate() {
                if line.contains(pattern) {
                    println!("  {:>4}: {}", i + 1, line.trim());
                }
            }
        }
    }
}

// ── Main ──────────────────────────────────────────────────────────────────

fn main() {
    let args: Vec<String> = std::env::args().collect();
    match args.get(1).map(String::as_str) {
        Some("info") => cmd_info(args.get(2).map(String::as_str).unwrap_or("archive.rpa")),
        Some("search") => {
            let archive = args.get(2).map(String::as_str).unwrap_or("archive.rpa");
            let pattern = args.get(3).map(String::as_str).unwrap_or("");
            if pattern.is_empty() {
                eprintln!("Usage: search <archive> <pattern>");
                return;
            }
            cmd_search(archive, pattern);
        }
        Some("dump") => {
            let archive = args.get(2).map(String::as_str).unwrap_or("archive.rpa");
            let path = args.get(3).map(String::as_str).unwrap_or("");
            let out = args.get(4).map(String::as_str).unwrap_or("out.bin");
            let entries = rpa_native_index(archive);
            if let Some(e) = entries.iter().find(|e| e.path == path) {
                let bytes = read_entry(archive, e);
                std::fs::write(out, bytes).unwrap();
                println!("Extracted {} to {}", path, out);
            } else {
                eprintln!("File not found in archive: {}", path);
            }
        }
        Some("extract") => {
            let archive = args.get(2).map(String::as_str).unwrap_or("archive.rpa");
            let output = flag_value(&args, "-o").unwrap_or("strings.json".into());
            cmd_extract(archive, &output, args.contains(&"--base-only".into()));
        }
        Some("translate") => {
            let input = args.get(2).map(String::as_str).unwrap_or("strings.json");
            let lang = flag_value(&args, "--lang").unwrap_or(DEFAULT_TARGET.into());
            let key = flag_value(&args, "--api-key")
                .or_else(|| std::env::var("ANTHROPIC_API_KEY").ok())
                .unwrap_or_default();
            if key.is_empty() {
                eprintln!("Set ANTHROPIC_API_KEY or use --api-key");
                std::process::exit(1);
            }
            let output = flag_value(&args, "-o").unwrap_or("translated.json".into());
            cmd_translate(input, &lang, &key, &output);
        }
        Some("patch") => {
            let json = args.get(2).map(String::as_str).unwrap_or("translated.json");
            let archive = args.get(3).map(String::as_str).unwrap_or("archive.rpa");
            let out_dir = flag_value(&args, "--out-dir").unwrap_or("./output".into());
            cmd_patch(json, archive, &out_dir);
        }
        _ => eprintln!(
            "renpy_extract — Ren'Py translation pipeline

USAGE:
  renpy_extract info      <archive.rpa>
  renpy_extract extract   <archive.rpa> [-o strings.json] [--base-only]
  renpy_extract translate <strings.json> --lang Thai [--api-key KEY] [-o translated.json]
  renpy_extract patch     <translated.json> <archive.rpa> [--out-dir ./output]

WORKFLOW:
  1. extract   → strings.json       (ดึงบทพูดทั้งหมด)
  2. translate → translated.json    (แปลผ่าน Anthropic API)
  3. patch     → game/tl/Thai/      (สร้างไฟล์แปล)
  4. วาง game/tl/Thai/ ลง game folder แล้วเลือก Language ในเกม
"
        ),
    }
}
