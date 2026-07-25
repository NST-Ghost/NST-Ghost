//! Godot Engine Analyzer
//!
//! Extracts and saves translatable strings from Godot games (.pck archives, .tscn scenes, .gd scripts, .csv, .po, .json).

use crate::analyzer::{AnalyzerOutput, GameAnalyzer, TextEntry};
use once_cell::sync::Lazy;
use regex::Regex;
use serde_json::json;
use std::collections::HashSet;
use std::fs;
use std::io::{BufReader, Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};
use walkdir::WalkDir;

// Regex patterns for Godot text extraction
static TSCN_TEXT_PATTERN: Lazy<Regex> = Lazy::new(|| {
    Regex::new(r#"(?m)^\s*(?:text|label|placeholder_text|tooltip_text|dialog_text|title)\s*=\s*"([^"]+)""#).unwrap()
});

static TSCN_MULTILINE_PATTERN: Lazy<Regex> = Lazy::new(|| {
    Regex::new(r#"(?s)^\s*(?:text|label|placeholder_text|tooltip_text|dialog_text|title)\s*=\s*"""(.*?)""""#).unwrap()
});

static TR_CALL_PATTERN: Lazy<Regex> = Lazy::new(|| {
    Regex::new(r#"tr\s*\(\s*"([^"]+)"\s*\)"#).unwrap()
});

static GD_STRING_PATTERN: Lazy<Regex> = Lazy::new(|| {
    Regex::new(r#""([^"\\]*(?:\\.[^"\\]*)*)""#).unwrap()
});

/// Godot PCK File Index Entry
#[derive(Debug, Clone)]
pub struct PckEntry {
    pub path: String,
    pub offset: u64,
    pub size: u64,
    pub md5: [u8; 16],
    pub flags: u32,
}

/// Godot PCK Header Info
#[derive(Debug, Clone)]
pub struct PckHeader {
    pub magic: u32,
    pub format_version: u32,
    pub major_version: u32,
    pub minor_version: u32,
    pub patch_version: u32,
    pub file_count: u32,
}

pub struct GodotAnalyzer;

impl GodotAnalyzer {
    pub fn new() -> Self {
        Self
    }

    /// Read PCK header and file entries from a .pck file or embedded executable
    pub fn parse_pck<R: Read + Seek>(reader: &mut R) -> Result<(PckHeader, Vec<PckEntry>), String> {
        let mut magic_buf = [0u8; 4];
        reader.read_exact(&mut magic_buf).map_err(|e| format!("Failed to read magic: {}", e))?;
        let magic = u32::from_le_bytes(magic_buf);

        // GDPC = 0x43504447 or GDPK = 0x4B504447
        if magic != 0x43504447 && magic != 0x4B504447 {
            return Err(format!("Invalid PCK magic bytes: 0x{:08X}", magic));
        }

        let mut buf4 = [0u8; 4];
        reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read format version: {}", e))?;
        let format_version = u32::from_le_bytes(buf4);

        reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read major version: {}", e))?;
        let major_version = u32::from_le_bytes(buf4);

        reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read minor version: {}", e))?;
        let minor_version = u32::from_le_bytes(buf4);

        reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read patch version: {}", e))?;
        let patch_version = u32::from_le_bytes(buf4);

        let (_flags, _file_base_offset) = if format_version >= 2 {
            reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read flags: {}", e))?;
            let flags = u32::from_le_bytes(buf4);

            let mut buf8 = [0u8; 8];
            reader.read_exact(&mut buf8).map_err(|e| format!("Failed to read file_base_offset: {}", e))?;
            let base_offset = u64::from_le_bytes(buf8);

            if (flags & 1) != 0 {
                return Err("PCK directory index is encrypted (PACK_DIR_ENCRYPTED). Extraction requires project decompiler or decryption key.".to_string());
            }

            (flags, base_offset)
        } else {
            (0, 0)
        };

        // Reserved fields (64 bytes = 16 x u32)
        let mut reserved = [0u8; 64];
        reader.read_exact(&mut reserved).map_err(|e| format!("Failed to read reserved header: {}", e))?;

        reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read file count: {}", e))?;
        let file_count = u32::from_le_bytes(buf4);

        let mut entries = Vec::with_capacity(file_count as usize);

        for i in 0..file_count {
            reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read path length at entry {}: {}", i, e))?;
            let path_len = u32::from_le_bytes(buf4) as usize;

            let mut path_buf = vec![0u8; path_len];
            reader.read_exact(&mut path_buf).map_err(|e| format!("Failed to read path string (len {}) at entry {}: {}", path_len, i, e))?;
            let raw_path = String::from_utf8_lossy(&path_buf);
            let path = raw_path.trim_matches('\0').to_string();

            // Read 4-byte alignment padding if needed
            let padding = (4 - (path_len % 4)) % 4;
            if padding > 0 {
                let mut pad_buf = vec![0u8; padding];
                reader.read_exact(&mut pad_buf).map_err(|e| format!("Failed to read path padding: {}", e))?;
            }

            let mut buf8 = [0u8; 8];
            reader.read_exact(&mut buf8).map_err(|e| format!("Failed to read offset: {}", e))?;
            let offset = u64::from_le_bytes(buf8);

            reader.read_exact(&mut buf8).map_err(|e| format!("Failed to read size: {}", e))?;
            let size = u64::from_le_bytes(buf8);

            let mut md5 = [0u8; 16];
            reader.read_exact(&mut md5).map_err(|e| format!("Failed to read MD5: {}", e))?;

            let flags = if format_version >= 2 {
                reader.read_exact(&mut buf4).map_err(|e| format!("Failed to read flags: {}", e))?;
                u32::from_le_bytes(buf4)
            } else {
                0
            };

            entries.push(PckEntry {
                path,
                offset,
                size,
                md5,
                flags,
            });
        }

        let header = PckHeader {
            magic,
            format_version,
            major_version,
            minor_version,
            patch_version,
            file_count,
        };

        Ok((header, entries))
    }

    /// Extract translatable strings from a CSV file (Godot i18n CSV format)
    fn extract_from_csv(content: &str, file_path: &str) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        let mut lines = content.lines();

        let header_line = match lines.next() {
            Some(h) => h,
            None => return entries,
        };

        let headers: Vec<&str> = header_line.split(',').map(|s| s.trim_matches('"').trim()).collect();
        if headers.is_empty() || !headers[0].eq_ignore_ascii_case("keys") && !headers[0].eq_ignore_ascii_case("key") && !headers[0].eq_ignore_ascii_case("id") {
            // Not a standard Godot translation CSV, treat line by line
            for (row, line) in content.lines().enumerate() {
                for (col, text) in line.split(',').enumerate() {
                    let clean = text.trim_matches('"').trim();
                    if is_translatable(clean) {
                        entries.push(TextEntry {
                            source: clean.to_string(),
                            path: file_path.to_string(),
                            key: format!("csv:{}:{}", row + 1, col + 1),
                            text: None,
                        });
                    }
                }
            }
            return entries;
        }

        for (row_idx, line) in lines.enumerate() {
            let cols: Vec<&str> = line.split(',').map(|s| s.trim_matches('"')).collect();
            if cols.is_empty() {
                continue;
            }
            let key_str = cols[0].trim();
            if key_str.is_empty() {
                continue;
            }

            // Index 1 is usually default/source language
            let source_text = if cols.len() > 1 && !cols[1].trim().is_empty() {
                cols[1].trim()
            } else {
                key_str
            };

            if is_translatable(source_text) {
                entries.push(TextEntry {
                    source: source_text.to_string(),
                    path: file_path.to_string(),
                    key: format!("csv:{}:{}", key_str, row_idx + 2),
                    text: None,
                });
            }
        }

        entries
    }

    /// Extract text entries from a .tscn scene file
    fn extract_from_tscn(content: &str, file_path: &str) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        let mut seen = HashSet::new();

        for cap in TSCN_TEXT_PATTERN.captures_iter(content) {
            if let Some(m) = cap.get(1) {
                let text = m.as_str().replace("\\n", "\n").replace("\\\"", "\"");
                if is_translatable(&text) && seen.insert(text.clone()) {
                    entries.push(TextEntry {
                        source: text.clone(),
                        path: file_path.to_string(),
                        key: format!("tscn:{}", entries.len() + 1),
                        text: None,
                    });
                }
            }
        }

        for cap in TSCN_MULTILINE_PATTERN.captures_iter(content) {
            if let Some(m) = cap.get(1) {
                let text = m.as_str().trim();
                if is_translatable(text) && seen.insert(text.to_string()) {
                    entries.push(TextEntry {
                        source: text.to_string(),
                        path: file_path.to_string(),
                        key: format!("tscn_multi:{}", entries.len() + 1),
                        text: None,
                    });
                }
            }
        }

        entries
    }

    /// Extract text entries from a GDScript (.gd) file
    fn extract_from_gd(content: &str, file_path: &str) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        let mut seen = HashSet::new();

        for (line_num, line) in content.lines().enumerate() {
            let trimmed = line.trim();
            if trimmed.starts_with('#') {
                continue;
            }

            // Check tr("...") calls
            for cap in TR_CALL_PATTERN.captures_iter(line) {
                if let Some(m) = cap.get(1) {
                    let text = m.as_str().to_string();
                    if is_translatable(&text) && seen.insert(text.clone()) {
                        entries.push(TextEntry {
                            source: text,
                            path: file_path.to_string(),
                            key: format!("{}:tr:{}", Path::new(file_path).file_name().unwrap_or_default().to_string_lossy(), line_num + 1),
                            text: None,
                        });
                    }
                }
            }

            // Check string literals containing Japanese/CJK or dialogue characters
            for cap in GD_STRING_PATTERN.captures_iter(line) {
                if let Some(m) = cap.get(1) {
                    let text = m.as_str();
                    if has_cjk_characters(text) && is_translatable(text) && seen.insert(text.to_string()) {
                        entries.push(TextEntry {
                            source: text.to_string(),
                            path: file_path.to_string(),
                            key: format!("{}:{}", Path::new(file_path).file_name().unwrap_or_default().to_string_lossy(), line_num + 1),
                            text: None,
                        });
                    }
                }
            }
        }

        entries
    }

    /// Extract text from a JSON file
    fn extract_from_json(content: &str, file_path: &str) -> Vec<TextEntry> {
        let mut entries = Vec::new();
        if let Ok(val) = serde_json::from_str::<serde_json::Value>(content) {
            let mut key_counter = 0;
            extract_json_strings(&val, file_path, &mut key_counter, &mut entries);
        }
        entries
    }

    /// Scan a directory or PCK archive and extract all translatable text entries
    pub fn extract_all(&self, input_path: &Path) -> Vec<TextEntry> {
        let mut entries = Vec::new();

        // 1. Check if input_path is a .pck file or contains a .pck file
        let pck_paths = find_pck_files(input_path);
        for pck_path in &pck_paths {
            println!("[BGA-Rust] Scanning Godot PCK file: {:?}", pck_path);
            if let Ok(file) = fs::File::open(pck_path) {
                let mut reader = BufReader::new(file);
                match Self::parse_pck(&mut reader) {
                    Ok((header, pck_entries)) => {
                        println!("[BGA-Rust] Parsed PCK header v{}, {} files", header.format_version, pck_entries.len());
                        for entry in &pck_entries {
                            println!("[BGA-Rust] Entry: {} (offset: {}, size: {})", entry.path, entry.offset, entry.size);
                        }
                        for entry in pck_entries {
                            let path_lower = entry.path.to_lowercase();
                            if entry.size == 0 || entry.size > 10_000_000 {
                                continue;
                            }

                            if path_lower.ends_with(".csv")
                                || path_lower.ends_with(".tscn")
                                || path_lower.ends_with(".gd")
                                || path_lower.ends_with(".json")
                                || path_lower.ends_with(".po")
                            {
                                if reader.seek(SeekFrom::Start(entry.offset)).is_ok() {
                                    let mut buffer = vec![0u8; entry.size as usize];
                                    if reader.read_exact(&mut buffer).is_ok() {
                                        let content = String::from_utf8_lossy(&buffer);
                                        if path_lower.ends_with(".csv") {
                                            entries.extend(Self::extract_from_csv(&content, &entry.path));
                                        } else if path_lower.ends_with(".tscn") {
                                            entries.extend(Self::extract_from_tscn(&content, &entry.path));
                                        } else if path_lower.ends_with(".gd") {
                                            entries.extend(Self::extract_from_gd(&content, &entry.path));
                                        } else if path_lower.ends_with(".json") {
                                            entries.extend(Self::extract_from_json(&content, &entry.path));
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Err(e) => {
                        println!("[BGA-Rust] PCK parse error for {:?}: {}", pck_path, e);
                    }
                }
            }
        }

        // 2. Also scan filesystem files if unpacked directory
        if input_path.is_dir() {
            for entry in WalkDir::new(input_path).into_iter().filter_map(|e| e.ok()) {
                if !entry.file_type().is_file() {
                    continue;
                }
                let path = entry.path();
                let path_str = path.to_string_lossy().to_string();

                if let Some(ext) = path.extension().and_then(|e| e.to_str()) {
                    let ext_lower = ext.to_lowercase();
                    if matches!(ext_lower.as_str(), "csv" | "tscn" | "gd" | "json" | "po") {
                        if let Ok(content) = fs::read_to_string(path) {
                            match ext_lower.as_str() {
                                "csv" => entries.extend(Self::extract_from_csv(&content, &path_str)),
                                "tscn" => entries.extend(Self::extract_from_tscn(&content, &path_str)),
                                "gd" => entries.extend(Self::extract_from_gd(&content, &path_str)),
                                "json" => entries.extend(Self::extract_from_json(&content, &path_str)),
                                _ => {}
                            }
                        }
                    }
                }
            }
        }

        entries
    }
}

impl Default for GodotAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

impl GameAnalyzer for GodotAnalyzer {
    fn analyze(&self, input_path: &Path) -> AnalyzerOutput {
        if !input_path.exists() {
            return AnalyzerOutput::error("Path does not exist");
        }

        println!("[BGA-Rust] Analyzing Godot project: {:?}", input_path);
        let entries = self.extract_all(input_path);
        println!("[BGA-Rust] Godot extraction complete. Found {} entries", entries.len());

        if entries.is_empty() {
            return AnalyzerOutput::error("No translatable strings found in Godot project");
        }

        let result = json!({
            "engine": "godot",
            "source": input_path.to_string_lossy(),
            "strings": entries,
        });

        AnalyzerOutput::success(serde_json::to_string_pretty(&result).unwrap_or_default())
    }

    fn save(&self, texts: &[TextEntry]) -> Result<(), String> {
        if texts.is_empty() {
            return Ok(());
        }

        // Save translations as a Godot CSV translation catalog file (`nst_translations.csv`)
        let project_dir = if let Some(first) = texts.first() {
            let path = Path::new(&first.path);
            if path.is_file() {
                path.parent().unwrap_or_else(|| Path::new(".")).to_path_buf()
            } else {
                path.to_path_buf()
            }
        } else {
            PathBuf::from(".")
        };

        let output_path = project_dir.join("nst_translations.csv");
        println!("[BGA-Rust] Writing Godot translations to {:?}", output_path);

        let mut file = fs::File::create(&output_path)
            .map_err(|e| format!("Failed to create translation CSV: {}", e))?;

        writeln!(file, "keys,th")
            .map_err(|e| format!("Failed to write CSV header: {}", e))?;

        let mut csv_buffer = Vec::new();
        csv_buffer.extend_from_slice(b"keys,th\n");

        for entry in texts {
            if let Some(translation) = &entry.text {
                if !translation.trim().is_empty() {
                    let escaped_key = escape_csv(&entry.source);
                    let escaped_val = escape_csv(translation);
                    let line = format!("{},{}\n", escaped_key, escaped_val);
                    file.write_all(line.as_bytes())
                        .map_err(|e| format!("Failed to write CSV row: {}", e))?;
                    csv_buffer.extend_from_slice(line.as_bytes());
                }
            }
        }

        // Also build a Godot Patch PCK file (`<pck_name>_patch.pck` or `patch.pck`)
        let pck_files = find_pck_files(&project_dir);
        let patch_pck_path = if let Some(first_pck) = pck_files.first() {
            let stem = first_pck.file_stem().unwrap_or_default().to_string_lossy();
            if stem.ends_with("_patch") {
                first_pck.clone()
            } else {
                first_pck.with_file_name(format!("{}_patch.pck", stem))
            }
        } else {
            project_dir.join("patch.pck")
        };

        let target_paths = vec![
            "res://assets/translation/tr.csv",
            "res://assets/translation/tr.en.translation",
            "res://assets/translation/tr.jp.translation",
            "res://assets/translation/tr.zh.translation",
            "res://assets/translation/tr.zh_Hant.translation",
            "res://nst_translations.csv",
        ];

        println!("[BGA-Rust] Creating Godot patch PCK at {:?}", patch_pck_path);
        if let Err(e) = create_godot_patch_pck(&patch_pck_path, &target_paths, &csv_buffer) {
            println!("[BGA-Rust] Failed to create patch PCK: {}", e);
        }

        Ok(())
    }
}

/// Helper function to create a Godot 4 Patch PCK archive with multiple target paths
fn create_godot_patch_pck(patch_path: &Path, target_paths: &[&str], content: &[u8]) -> Result<(), String> {
    let mut file = fs::File::create(patch_path)
        .map_err(|e| format!("Failed to create patch PCK file: {}", e))?;

    let mut entries = Vec::new();
    for vpath in target_paths {
        let res_bytes = vpath.as_bytes();
        let path_len = res_bytes.len();
        let padding_len = (4 - (path_len % 4)) % 4;
        let mut padded_path = res_bytes.to_vec();
        padded_path.extend(vec![0u8; padding_len]);
        entries.append(&mut vec![(path_len, padded_path)]);
    }

    let header_size = 100u64;
    let index_size: u64 = entries.iter().map(|(pl, pp)| 4 + pp.len() as u64 + 8 + 8 + 16 + 4).sum();
    let mut current_offset = header_size + index_size;

    // Header: Magic "GDPC" (0x43504447), format_ver 2, Godot 4.3.0
    file.write_all(&0x43504447u32.to_le_bytes()).map_err(|e| e.to_string())?;
    file.write_all(&2u32.to_le_bytes()).map_err(|e| e.to_string())?; // ver 2
    file.write_all(&4u32.to_le_bytes()).map_err(|e| e.to_string())?; // major 4
    file.write_all(&3u32.to_le_bytes()).map_err(|e| e.to_string())?; // minor 3
    file.write_all(&0u32.to_le_bytes()).map_err(|e| e.to_string())?; // patch 0
    file.write_all(&0u32.to_le_bytes()).map_err(|e| e.to_string())?; // flags 0
    file.write_all(&0u64.to_le_bytes()).map_err(|e| e.to_string())?; // base_offset 0
    file.write_all(&[0u8; 64]).map_err(|e| e.to_string())?;           // reserved 64 bytes
    file.write_all(&(entries.len() as u32).to_le_bytes()).map_err(|e| e.to_string())?; // file count

    let md5_bytes = [0u8; 16];

    for (path_len, padded_path) in &entries {
        file.write_all(&(*path_len as u32).to_le_bytes()).map_err(|e| e.to_string())?;
        file.write_all(padded_path).map_err(|e| e.to_string())?;
        file.write_all(&current_offset.to_le_bytes()).map_err(|e| e.to_string())?;
        file.write_all(&(content.len() as u64).to_le_bytes()).map_err(|e| e.to_string())?;
        file.write_all(&md5_bytes).map_err(|e| e.to_string())?;
        file.write_all(&0u32.to_le_bytes()).map_err(|e| e.to_string())?;

        current_offset += content.len() as u64;
    }

    // Write contents
    for _ in 0..entries.len() {
        file.write_all(content).map_err(|e| e.to_string())?;
    }

    Ok(())
}

/// Helper function to check if string has CJK (Japanese/Chinese/Korean) characters
fn has_cjk_characters(s: &str) -> bool {
    s.chars().any(|c| matches!(c,
        '\u{3040}'..='\u{309F}' | // Hiragana
        '\u{30A0}'..='\u{30FF}' | // Katakana
        '\u{4E00}'..='\u{9FFF}' | // CJK Unified Ideographs
        '\u{FF00}'..='\u{FFEF}'   // Fullwidth Forms
    ))
}

/// Helper function to filter out code keywords, URLs, and paths
fn is_translatable(s: &str) -> bool {
    let trimmed = s.trim();
    if trimmed.is_empty() || trimmed.len() < 2 {
        return false;
    }
    if trimmed.starts_with("res://") || trimmed.starts_with("user://") || trimmed.starts_with("uid://") {
        return false;
    }
    if trimmed.ends_with(".tscn") || trimmed.ends_with(".gd") || trimmed.ends_with(".png") || trimmed.ends_with(".ogg") {
        return false;
    }
    true
}

/// Recursively extract strings from JSON values
fn extract_json_strings(val: &serde_json::Value, path: &str, counter: &mut usize, entries: &mut Vec<TextEntry>) {
    match val {
        serde_json::Value::String(s) => {
            if is_translatable(s) {
                *counter += 1;
                entries.push(TextEntry {
                    source: s.clone(),
                    path: path.to_string(),
                    key: format!("json:{}", counter),
                    text: None,
                });
            }
        }
        serde_json::Value::Array(arr) => {
            for item in arr {
                extract_json_strings(item, path, counter, entries);
            }
        }
        serde_json::Value::Object(map) => {
            for (_k, v) in map {
                extract_json_strings(v, path, counter, entries);
            }
        }
        _ => {}
    }
}

/// Helper to search for .pck files in a directory or single file
fn find_pck_files(base_path: &Path) -> Vec<PathBuf> {
    let mut pck_files = Vec::new();
    if base_path.is_file() {
        if base_path.extension().map(|e| e == "pck").unwrap_or(false) {
            pck_files.push(base_path.to_path_buf());
        }
    } else if base_path.is_dir() {
        for entry in WalkDir::new(base_path).into_iter().filter_map(|e| e.ok()) {
            if entry.file_type().is_file() {
                if entry.path().extension().map(|e| e == "pck").unwrap_or(false) {
                    pck_files.push(entry.path().to_path_buf());
                }
            }
        }
    }
    pck_files
}

/// Escape text for CSV format
fn escape_csv(s: &str) -> String {
    if s.contains(',') || s.contains('"') || s.contains('\n') {
        format!("\"{}\"", s.replace('"', "\"\""))
    } else {
        s.to_string()
    }
}
