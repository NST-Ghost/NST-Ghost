//! C-ABI exports
//!
//! Exposes three symbols for embedding in non-Rust hosts:
//!   renpy_analyze(path)           → JSON string  (caller must free)
//!   renpy_save(path, texts_json)  → i32 status
//!   renpy_free_string(ptr)        → void

use crate::extractor;
use std::ffi::{c_char, CStr, CString};
use std::fs;
use std::io::Write;
use std::path::Path;
use std::process::Command;
use walkdir::WalkDir;
use serde_json::json;

// ── unrpyc helper ────────────────────────────────────────────────────────────

fn find_unrpyc() -> Option<Vec<String>> {
    // Try `python3 -m unrpyc` first, then bare `unrpyc` binary
    for args in [
        vec!["python3", "-m", "unrpyc"],
        vec!["unrpyc"],
    ] {
        let ok = Command::new(args[0])
            .args(&args[1..])
            .arg("--help")
            .output()
            .map(|o| o.status.success())
            .unwrap_or(false);

        if ok {
            return Some(args.iter().map(|s| s.to_string()).collect());
        }
    }
    None
}

fn decompile_rpyc(dir: &Path, cmd: &[String]) {
    for entry in WalkDir::new(dir).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file()
            && entry.path().extension().map_or(false, |e| e == "rpyc")
            && !entry.path().with_extension("rpy").exists()
        {
            let mut c = Command::new(&cmd[0]);
            for arg in &cmd[1..] { c.arg(arg); }
            let _ = c.arg(entry.path()).output();
        }
    }
}

// ── FFI surface ──────────────────────────────────────────────────────────────

/// Analyse a Ren'Py game directory and return all dialogue as JSON.
///
/// # Safety
/// `path` must be a valid, NUL-terminated UTF-8 C string.
/// The returned pointer must be freed with `renpy_free_string`.
#[no_mangle]
pub unsafe extern "C" fn renpy_analyze(path: *const c_char) -> *mut c_char {
    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    let input  = Path::new(path_str);
    let game   = if input.join("game").is_dir() { input.join("game") } else { input.to_path_buf() };

    // Decompile any .rpyc files that lack a .rpy counterpart
    if let Some(cmd) = find_unrpyc() {
        decompile_rpyc(&game, &cmd);
    }

    // Use extractor module (single source of truth for parsing)
    let mut blocks = Vec::new();
    for entry in WalkDir::new(&game).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file()
            && entry.path().extension().map_or(false, |e| e == "rpy")
        {
            let rel = entry.path()
                .strip_prefix(&game)
                .unwrap_or(entry.path())
                .to_string_lossy()
                .to_string();

            if let Ok(bytes) = fs::read(entry.path()) {
                blocks.extend(extractor::extract_from_bytes(&rel, &bytes));
            }
        }
    }

    let payload = serde_json::to_string(&json!({ "strings": blocks, "engine": "renpy" }))
        .unwrap_or_default();

    CString::new(payload).unwrap_or_default().into_raw()
}

/// Write translated strings back to a `nst_translations.rpy` file.
///
/// `texts_json` must be a JSON array of `TextBlock` objects (with `translation` filled).
///
/// Returns 0 on success, negative on error:
///   -1  invalid UTF-8 in arguments
///   -2  JSON parse error
///   -3  file write error
///
/// # Safety
/// Both pointers must be valid, NUL-terminated UTF-8 C strings.
#[no_mangle]
pub unsafe extern "C" fn renpy_save(
    _path: *const c_char,
    texts_json: *const c_char,
) -> i32 {
    let json_str = match CStr::from_ptr(texts_json).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    let blocks: Vec<extractor::TextBlock> = match serde_json::from_str(json_str) {
        Ok(b) => b,
        Err(_) => return -2,
    };

    // Resolve the output directory from the path stored in the first block
    let out_dir = blocks.first().map(|b| {
        let p = Path::new(&b.file);
        // Walk up to find the `game/` folder
        let mut cur = p.parent();
        while let Some(c) = cur {
            if c.file_name().map_or(false, |n| n == "game") { return c.to_path_buf(); }
            cur = c.parent();
        }
        Path::new(".").to_path_buf()
    }).unwrap_or_else(|| Path::new(".").to_path_buf());

    let out_path = out_dir.join("nst_translations.rpy");

    let mut file = match fs::File::create(&out_path) {
        Ok(f) => f,
        Err(_) => return -3,
    };

    for block in &blocks {
        if let Some(tl) = &block.translation {
            if !tl.is_empty() {
                let _ = writeln!(
                    file,
                    "translate None:\n    old \"{}\"\n    new \"{}\"\n",
                    block.source, tl
                );
            }
        }
    }

    0
}

/// Free a string returned by `renpy_analyze`.
///
/// # Safety
/// `ptr` must have been returned by `renpy_analyze` and not yet freed.
#[no_mangle]
pub unsafe extern "C" fn renpy_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        drop(CString::from_raw(ptr));
    }
}
