use std::ffi::{c_char, CStr, CString};
use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;
use std::process::Command;
use walkdir::WalkDir;
use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};
use serde_json::json;

// --- Models ---

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TextEntry {
    pub source: String,
    pub path: String,
    pub key: String,
    pub text: Option<String>,
}

// Regex patterns for Ren'Py dialogue
static DIALOG_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s*(?:[a-zA-Z_]\w*\s+)?"([^"]+)""#).unwrap());

static MENU_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s*"([^"]+)"\s*:"#).unwrap());

pub struct RenpyAnalyzer;

impl RenpyAnalyzer {
    fn find_unrpyc() -> Option<Vec<String>> {
        let output = Command::new("python3")
            .args(["-m", "unrpyc", "--help"])
            .output();

        if let Ok(out) = output {
            if out.status.success() {
                return Some(vec!["python3".to_string(), "-m".to_string(), "unrpyc".to_string()]);
            }
        }

        let output = Command::new("unrpyc").arg("--help").output();
        if let Ok(out) = output {
            if out.status.success() {
                return Some(vec!["unrpyc".to_string()]);
            }
        }
        None
    }

    fn decompile_rpyc(dir: &Path, unrpyc_cmd: &[String]) {
        for entry in WalkDir::new(dir).into_iter().filter_map(|e| e.ok()) {
            if entry.file_type().is_file() && entry.path().extension().map_or(false, |ext| ext == "rpyc") {
                let rpy_path = entry.path().with_extension("rpy");
                if rpy_path.exists() { continue; }

                let mut cmd = Command::new(&unrpyc_cmd[0]);
                for arg in &unrpyc_cmd[1..] { cmd.arg(arg); }
                cmd.arg(entry.path());
                let _ = cmd.output();
            }
        }
    }

    fn extract_from_rpy(path: &Path) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        let file = match fs::File::open(path) { Ok(f) => f, Err(_) => return entries };
        let reader = BufReader::new(file);
        let file_path = path.to_string_lossy().to_string();

        for (line_num, line_result) in reader.lines().enumerate() {
            let line = match line_result { Ok(l) => l, Err(_) => continue };
            let text = DIALOG_PATTERN.captures(&line)
                .or_else(|| MENU_PATTERN.captures(&line))
                .and_then(|caps| caps.get(1))
                .map(|m| m.as_str().to_string());

            if let Some(text) = text {
                if !text.trim().is_empty() {
                    entries.push(TextEntry {
                        source: text,
                        path: file_path.clone(),
                        key: format!("{}:{}", path.file_name().unwrap().to_string_lossy(), line_num + 1),
                        text: None,
                    });
                }
            }
        }
        entries
    }
}

// --- FFI ---

#[no_mangle]
pub unsafe extern "C" fn renpy_analyze(path: *const c_char) -> *mut c_char {
    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let input_path = Path::new(path_str);
    let game_path = if input_path.join("game").is_dir() { input_path.join("game") } else { input_path.to_path_buf() };

    if let Some(unrpyc_cmd) = RenpyAnalyzer::find_unrpyc() {
        RenpyAnalyzer::decompile_rpyc(&game_path, &unrpyc_cmd);
    }

    let mut all_entries = Vec::new();
    for entry in WalkDir::new(&game_path).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file() && entry.path().extension().map_or(false, |ext| ext == "rpy") {
            all_entries.extend(RenpyAnalyzer::extract_from_rpy(entry.path()));
        }
    }

    let result = json!({ "strings": all_entries, "engine": "renpy" });
    let payload = serde_json::to_string(&result).unwrap_or_default();
    CString::new(payload).unwrap().into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn renpy_save(path: *const c_char, texts_json: *const c_char) -> i32 {
    let json_str = match CStr::from_ptr(texts_json).to_str() { Ok(s) => s, Err(_) => return -1 };
    let texts: Vec<TextEntry> = match serde_json::from_str(json_str) { Ok(t) => t, Err(_) => return -2 };
    
    let game_dir = if let Some(first) = texts.first() {
        let path = Path::new(&first.path);
        path.parent().and_then(|p| {
             let mut curr = Some(p);
             while let Some(c) = curr {
                 if c.file_name().map_or(false, |n| n == "game") { return Some(c); }
                 curr = c.parent();
             }
             None
        }).unwrap_or(Path::new("."))
    } else { Path::new(".") };

    let output_path = game_dir.join("nst_translations.rpy");
    if let Ok(mut file) = fs::File::create(output_path) {
        for entry in texts {
            if let Some(translation) = entry.text {
                if !translation.is_empty() {
                    let _ = writeln!(file, "translate None:\n    old \"{}\"\n    new \"{}\"\n", entry.source, translation);
                }
            }
        }
        0
    } else { -3 }
}

#[no_mangle]
pub unsafe extern "C" fn renpy_free_string(ptr: *mut c_char) {
    if !ptr.is_null() { let _ = CString::from_raw(ptr); }
}
