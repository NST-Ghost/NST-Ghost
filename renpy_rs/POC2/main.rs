//! renpy_extract – command-line tool
//!
//! Usage:
//!   renpy_extract extract <game_dir> [--out translations.json]
//!   renpy_extract translate <translations.json> --lang Thai --key <api_key>
//!   renpy_extract patch <translations.json> --ref tl/Vietnamese --lang Thai --out tl/Thai

use renpy_rs::{
    build_translation_map, extract_from_bytes_via_walkdir,
    font_override_block, patch_tl_file, translate_blocks,
};
use std::{collections::HashMap, fs, path::Path, process};

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 2 {
        print_usage(&args[0]);
        process::exit(1);
    }

    match args[1].as_str() {
        "extract"   => cmd_extract(&args[2..]),
        "translate" => cmd_translate(&args[2..]),
        "patch"     => cmd_patch(&args[2..]),
        _           => { print_usage(&args[0]); process::exit(1); }
    }
}

// ── extract ──────────────────────────────────────────────────────────────────

fn cmd_extract(args: &[String]) {
    let game_dir = args.first().unwrap_or_else(|| {
        eprintln!("Usage: extract <game_dir> [--out <file.json>]");
        process::exit(1);
    });

    let out_path = flag(args, "--out").unwrap_or("translations.json".to_string());

    let base = Path::new(game_dir);
    let game = if base.join("game").is_dir() { base.join("game") } else { base.to_path_buf() };

    let mut blocks = Vec::new();
    for entry in walkdir::WalkDir::new(&game).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file()
            && entry.path().extension().map_or(false, |e| e == "rpy")
        {
            let rel = entry.path()
                .strip_prefix(&game).unwrap_or(entry.path())
                .to_string_lossy().to_string();
            if let Ok(bytes) = fs::read(entry.path()) {
                blocks.extend(renpy_rs::extractor::extract_from_bytes(&rel, &bytes));
            }
        }
    }

    let json = serde_json::to_string_pretty(&blocks).unwrap();
    fs::write(&out_path, json).unwrap();
    eprintln!("[extract] {} strings → {}", blocks.len(), out_path);
}

// ── translate ─────────────────────────────────────────────────────────────────

fn cmd_translate(args: &[String]) {
    let json_path = args.first().unwrap_or_else(|| {
        eprintln!("Usage: translate <translations.json> --lang <Lang> --key <api_key>");
        process::exit(1);
    });

    let lang    = flag(args, "--lang").unwrap_or_else(|| { eprintln!("--lang required"); process::exit(1); });
    let api_key = flag(args, "--key").unwrap_or_else(|| { eprintln!("--key required");  process::exit(1); });

    let raw = fs::read_to_string(json_path).expect("cannot read json");
    let mut blocks: Vec<renpy_rs::TextBlock> = serde_json::from_str(&raw).expect("bad json");

    let map = translate_blocks(&blocks, &lang, &api_key).expect("translation failed");

    for b in &mut blocks {
        if let Some(t) = map.get(&b.source) {
            b.translation = Some(t.clone());
        }
    }

    let out = serde_json::to_string_pretty(&blocks).unwrap();
    fs::write(json_path, out).unwrap();
    eprintln!("[translate] done → {}", json_path);
}

// ── patch ─────────────────────────────────────────────────────────────────────

fn cmd_patch(args: &[String]) {
    let json_path = args.first().unwrap_or_else(|| {
        eprintln!("Usage: patch <translations.json> --ref <tl/Lang> --lang <Lang> [--font fonts/X.ttf] --out <tl/Lang>");
        process::exit(1);
    });

    let ref_dir  = flag(args, "--ref").unwrap_or_else(|| { eprintln!("--ref required");  process::exit(1); });
    let lang     = flag(args, "--lang").unwrap_or_else(|| { eprintln!("--lang required"); process::exit(1); });
    let font     = flag(args, "--font");
    let out_dir  = flag(args, "--out").unwrap_or_else(|| format!("tl/{}", lang));

    let raw = fs::read_to_string(json_path).expect("cannot read json");
    let blocks: Vec<renpy_rs::TextBlock> = serde_json::from_str(&raw).expect("bad json");
    let tl_map = build_translation_map(&blocks);

    // Detect reference language name from directory name
    let ref_lang = Path::new(&ref_dir)
        .file_name().unwrap().to_string_lossy().to_string();

    fs::create_dir_all(&out_dir).unwrap();

    for entry in walkdir::WalkDir::new(&ref_dir).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file()
            && entry.path().extension().map_or(false, |e| e == "rpy")
        {
            let ref_text = fs::read_to_string(entry.path()).unwrap_or_default();
            let rel = entry.path().strip_prefix(&ref_dir).unwrap().to_string_lossy();

            let mut patched = String::new();

            // Prepend font override block to first file only
            if let Some(ref f) = font {
                if rel.contains("script") || rel == "common.rpy" {
                    patched.push_str(&font_override_block(&lang, f));
                }
            }

            patched.push_str(&patch_tl_file(
                &ref_text, &ref_lang, &lang,
                &tl_map, &HashMap::new(),
            ));

            let out_path = Path::new(&out_dir).join(rel.as_ref());
            if let Some(parent) = out_path.parent() { fs::create_dir_all(parent).unwrap(); }
            fs::write(&out_path, patched).unwrap();
            eprintln!("[patch] → {}", out_path.display());
        }
    }
}

// ── helpers ──────────────────────────────────────────────────────────────────

fn flag(args: &[String], name: &str) -> Option<String> {
    args.windows(2).find(|w| w[0] == name).map(|w| w[1].clone())
}

fn print_usage(bin: &str) {
    eprintln!("Usage:");
    eprintln!("  {bin} extract   <game_dir>           [--out translations.json]");
    eprintln!("  {bin} translate <translations.json>  --lang Thai --key <api_key>");
    eprintln!("  {bin} patch     <translations.json>  --ref tl/Vietnamese --lang Thai [--font fonts/Sarabun-Regular.ttf] [--out tl/Thai]");
}
