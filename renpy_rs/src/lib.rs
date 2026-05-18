use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};
use serde_json::json;
use std::ffi::{c_char, CStr, CString};
use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;
use std::process::Command;
use walkdir::WalkDir;

mod rpa;

// --- Models ---

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TextEntry {
    pub source: String,
    pub path: String,
    pub key: String,
    pub text: Option<String>,
    #[serde(rename = "targetLanguage", skip_serializing_if = "Option::is_none")]
    pub target_language: Option<String>,
}

// Regex patterns for Ren'Py dialogue
static DIALOG_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s*(?:[a-zA-Z_]\w*\s+)?"([^"]+)""#).unwrap());

static MENU_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r#"^\s*"([^"]+)"\s*:"#).unwrap());

pub struct RenpyAnalyzer;

impl RenpyAnalyzer {
    fn find_unrpyc() -> Option<Vec<String>> {
        let output = Command::new("python3")
            .args(["-m", "unrpyc", "--help"])
            .output();

        if let Ok(out) = output {
            if out.status.success() {
                return Some(vec![
                    "python3".to_string(),
                    "-m".to_string(),
                    "unrpyc".to_string(),
                ]);
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
            if entry.file_type().is_file()
                && entry.path().extension().map_or(false, |ext| ext == "rpyc")
            {
                let rpy_path = entry.path().with_extension("rpy");
                if rpy_path.exists() {
                    continue;
                }

                let mut cmd = Command::new(&unrpyc_cmd[0]);
                for arg in &unrpyc_cmd[1..] {
                    cmd.arg(arg);
                }
                cmd.arg(entry.path());
                let _ = cmd.output();
            }
        }
    }

    fn extract_from_rpy(path: &Path) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        let file = match fs::File::open(path) {
            Ok(f) => f,
            Err(_) => return entries,
        };
        let reader = BufReader::new(file);
        let file_path = path.to_string_lossy().to_string();

        for (line_num, line_result) in reader.lines().enumerate() {
            let line = match line_result {
                Ok(l) => l,
                Err(_) => continue,
            };
            let text = DIALOG_PATTERN
                .captures(&line)
                .or_else(|| MENU_PATTERN.captures(&line))
                .and_then(|caps| caps.get(1))
                .map(|m| m.as_str().to_string());

            if let Some(text) = text {
                if should_extract_renpy_text(&text) {
                    entries.push(TextEntry {
                        source: text,
                        path: file_path.clone(),
                        key: format!(
                            "{}:{}",
                            path.file_name().unwrap().to_string_lossy(),
                            line_num + 1
                        ),
                        text: None,
                        target_language: None,
                    });
                }
            }
        }
        entries
    }

    fn is_translation_file(path: &Path) -> bool {
        path.components()
            .any(|component| component.as_os_str() == "tl")
    }

    fn extract_from_rpa(archive_path: &Path) -> Vec<TextEntry> {
        let archive = archive_path.to_string_lossy().to_string();
        let entries = match rpa::parse_rpa(&archive) {
            Ok(entries) => entries,
            Err(_) => return Vec::new(),
        };

        let mut all_entries = Vec::new();
        for entry in entries {
            if !entry.path.ends_with(".rpy") || entry.path.starts_with("tl/") {
                continue;
            }

            let bytes = match rpa::read_rpa_file(&archive, &entry) {
                Ok(bytes) => bytes,
                Err(_) => continue,
            };

            let bytes = if bytes.starts_with(b"\xef\xbb\xbf") {
                &bytes[3..]
            } else {
                &bytes[..]
            };
            let text = String::from_utf8_lossy(bytes).replace("\r\n", "\n");
            let reader = BufReader::new(text.as_bytes());
            let file_path = format!("{}!{}", archive_path.to_string_lossy(), entry.path);

            for (line_num, line_result) in reader.lines().enumerate() {
                let line = match line_result {
                    Ok(l) => l,
                    Err(_) => continue,
                };
                let text = DIALOG_PATTERN
                    .captures(&line)
                    .or_else(|| MENU_PATTERN.captures(&line))
                    .and_then(|caps| caps.get(1))
                    .map(|m| m.as_str().to_string());

                if let Some(text) = text {
                    if should_extract_renpy_text(&text) {
                        entries_push(
                            &mut all_entries,
                            text,
                            &file_path,
                            &entry.path,
                            line_num + 1,
                        );
                    }
                }
            }
        }

        all_entries
    }
}

fn entries_push(
    entries: &mut Vec<TextEntry>,
    source: String,
    path: &str,
    file_name: &str,
    line_num: usize,
) {
    entries.push(TextEntry {
        source,
        path: path.to_string(),
        key: format!("{}:{}", file_name, line_num),
        text: None,
        target_language: None,
    });
}

fn should_extract_renpy_text(text: &str) -> bool {
    let trimmed = text.trim();
    if trimmed.is_empty() {
        return false;
    }

    let lower = trimmed.to_lowercase();
    const ASSET_EXTENSIONS: &[&str] = &[
        ".png", ".jpg", ".jpeg", ".webp", ".gif", ".bmp", ".svg", ".ogg", ".oga", ".mp3", ".wav",
        ".opus", ".webm", ".mp4", ".avi", ".ttf", ".otf",
    ];
    if ASSET_EXTENSIONS.iter().any(|ext| lower.ends_with(ext)) {
        return false;
    }
    if (trimmed.contains('/') || trimmed.contains('\\')) && !trimmed.contains(' ') {
        return false;
    }
    if trimmed.contains('_')
        && trimmed
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-')
    {
        return false;
    }
    if trimmed.starts_with('#') || trimmed.starts_with('$') {
        return false;
    }

    trimmed.chars().any(|c| c.is_alphabetic())
}

// --- FFI ---

#[no_mangle]
pub unsafe extern "C" fn renpy_analyze(path: *const c_char) -> *mut c_char {
    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let input_path = Path::new(path_str);
    let game_path = if input_path.join("game").is_dir() {
        input_path.join("game")
    } else {
        input_path.to_path_buf()
    };

    if let Some(unrpyc_cmd) = RenpyAnalyzer::find_unrpyc() {
        RenpyAnalyzer::decompile_rpyc(&game_path, &unrpyc_cmd);
    }

    let mut all_entries = Vec::new();
    for entry in WalkDir::new(&game_path).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file()
            && entry.path().extension().map_or(false, |ext| ext == "rpy")
            && !RenpyAnalyzer::is_translation_file(entry.path())
        {
            all_entries.extend(RenpyAnalyzer::extract_from_rpy(entry.path()));
        }
    }

    if all_entries.is_empty() {
        for entry in WalkDir::new(&game_path).into_iter().filter_map(|e| e.ok()) {
            if entry.file_type().is_file()
                && entry.path().extension().map_or(false, |ext| ext == "rpa")
            {
                all_entries.extend(RenpyAnalyzer::extract_from_rpa(entry.path()));
            }
        }
    }

    let result = json!({ "strings": all_entries, "engine": "renpy" });
    let payload = serde_json::to_string(&result).unwrap_or_default();
    CString::new(payload).unwrap().into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn renpy_save(path: *const c_char, texts_json: *const c_char) -> i32 {
    let output_path = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };
    let json_str = match CStr::from_ptr(texts_json).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };
    let texts: Vec<TextEntry> = match serde_json::from_str(json_str) {
        Ok(t) => t,
        Err(_) => return -2,
    };

    let base_path = Path::new(output_path);
    let game_dir = if base_path.join("game").is_dir() {
        base_path.join("game")
    } else if base_path.file_name().map_or(false, |name| name == "game") {
        base_path.to_path_buf()
    } else if let Some(first) = texts.first() {
        find_game_dir_from_entry(&first.path).unwrap_or_else(|| base_path.to_path_buf())
    } else {
        base_path.to_path_buf()
    };

    let target_language = texts
        .iter()
        .find_map(|entry| entry.target_language.as_deref())
        .map(renpy_language_name)
        .unwrap_or_else(|| "Thai".to_string());

    let tl_dir = game_dir.join("tl").join(&target_language);
    if fs::create_dir_all(&tl_dir).is_err() {
        return -3;
    }

    if write_language_bootstrap(&game_dir, &target_language).is_err() {
        return -3;
    }

    let output_file = tl_dir.join("nst_translations.rpy");
    if let Ok(mut file) = fs::File::create(output_file) {
        if writeln!(
            file,
            "# Generated by NST\ntranslate {} strings:\n",
            target_language
        )
        .is_err()
        {
            return -4;
        }
        for entry in texts {
            if let Some(translation) = entry.text {
                if !translation.is_empty() {
                    let _ = writeln!(
                        file,
                        "    old \"{}\"\n    new \"{}\"\n",
                        escape_rpy(&entry.source),
                        escape_rpy(&translation)
                    );
                }
            }
        }
        0
    } else {
        -3
    }
}

fn find_game_dir_from_entry(entry_path: &str) -> Option<std::path::PathBuf> {
    let real_path = entry_path.split('!').next().unwrap_or(entry_path);
    let path = Path::new(real_path);
    let mut current = if path.is_dir() {
        Some(path)
    } else {
        path.parent()
    };
    while let Some(dir) = current {
        if dir.file_name().map_or(false, |name| name == "game") {
            return Some(dir.to_path_buf());
        }
        current = dir.parent();
    }
    None
}

fn write_language_bootstrap(game_dir: &Path, target_language: &str) -> std::io::Result<()> {
    let escaped_language = escape_rpy(target_language);
    fs::write(
        game_dir.join("nst_language.rpy"),
        format!(
            "# Generated by NST\ninit -1700 python:\n    config.language = \"{}\"\n    config.default_language = \"{}\"\n",
            escaped_language,
            escaped_language
        ),
    )
}

fn escape_rpy(s: &str) -> String {
    s.replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('\n', "\\n")
}

fn renpy_language_name(language: &str) -> String {
    let normalized = language.trim();
    match normalized {
        "ar" => "Arabic".to_string(),
        "de" => "German".to_string(),
        "en" => "English".to_string(),
        "es" => "Spanish".to_string(),
        "fr" => "French".to_string(),
        "hi" => "Hindi".to_string(),
        "it" => "Italian".to_string(),
        "ja" => "Japanese".to_string(),
        "ko" => "Korean".to_string(),
        "pt" => "Portuguese".to_string(),
        "ru" => "Russian".to_string(),
        "th" => "Thai".to_string(),
        "zh-CN" | "zh_cn" | "zh" => "Chinese".to_string(),
        _ if normalized.is_empty() => "Thai".to_string(),
        _ => {
            let sanitized = normalized
                .chars()
                .filter(|c| c.is_ascii_alphanumeric() || *c == '_')
                .collect::<String>();
            if sanitized.is_empty() {
                "Thai".to_string()
            } else {
                sanitized
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn renpy_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        let _ = CString::from_raw(ptr);
    }
}
