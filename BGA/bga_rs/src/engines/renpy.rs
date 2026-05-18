//! Ren'Py Analyzer
//!
//! Extracts and saves translatable strings from Ren'Py games.

use crate::analyzer::{AnalyzerOutput, GameAnalyzer, TextEntry};
use once_cell::sync::Lazy;
use regex::Regex;
use serde_json::json;
use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;
use std::process::Command;
use walkdir::WalkDir;

// Regex patterns for Ren'Py dialogue
static DIALOG_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s*(?:[a-zA-Z_]\w*\s+)?"([^"]+)""#).unwrap());

static MENU_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r#"^\s*"([^"]+)"\s*:"#).unwrap());

pub struct RenpyAnalyzer;

impl RenpyAnalyzer {
    pub fn new() -> Self {
        Self
    }

    /// Check if unrpyc is available and return the command to use
    fn find_unrpyc() -> Option<Vec<String>> {
        // Try python3 -m unrpyc
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

        // Try unrpyc directly
        let output = Command::new("unrpyc").arg("--help").output();

        if let Ok(out) = output {
            if out.status.success() {
                return Some(vec!["unrpyc".to_string()]);
            }
        }

        None
    }

    /// Decompile .rpyc files to .rpy
    fn decompile_rpyc(dir: &Path, unrpyc_cmd: &[String]) -> Result<(), String> {
        for entry in WalkDir::new(dir).into_iter().filter_map(|e| e.ok()) {
            if entry.file_type().is_file() {
                if let Some(ext) = entry.path().extension() {
                    if ext == "rpyc" {
                        let rpyc_path = entry.path();
                        let rpy_path = rpyc_path.with_extension("rpy");

                        // Skip if .rpy already exists
                        if rpy_path.exists() {
                            continue;
                        }

                        // Run decompiler
                        let mut cmd = Command::new(&unrpyc_cmd[0]);
                        for arg in &unrpyc_cmd[1..] {
                            cmd.arg(arg);
                        }
                        cmd.arg(rpyc_path);

                        let output = cmd
                            .output()
                            .map_err(|e| format!("Failed to run unrpyc: {}", e))?;

                        if !output.status.success() {
                            println!(
                                "Decompile failed for {:?}: {}",
                                rpyc_path,
                                String::from_utf8_lossy(&output.stderr)
                            );
                        }
                    }
                }
            }
        }

        Ok(())
    }

    /// Extract strings from .rpy files
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

            // Try dialog pattern first, then menu pattern
            let text = DIALOG_PATTERN
                .captures(&line)
                .or_else(|| MENU_PATTERN.captures(&line))
                .and_then(|caps| caps.get(1))
                .map(|m| m.as_str().to_string());

            if let Some(text) = text {
                if !text.trim().is_empty() {
                    entries.push(TextEntry {
                        source: text.clone(),
                        path: file_path.clone(),
                        key: format!(
                            "{}:{}",
                            path.file_name().unwrap().to_string_lossy(),
                            line_num + 1
                        ),
                        text: None,
                    });
                }
            }
        }

        entries
    }
}

impl Default for RenpyAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

impl GameAnalyzer for RenpyAnalyzer {
    fn analyze(&self, input_path: &Path) -> AnalyzerOutput {
        if !input_path.exists() {
            return AnalyzerOutput::error("Path does not exist");
        }

        // Ren'Py files are usually in 'game' subdirectory
        let game_path = if input_path.join("game").is_dir() {
            input_path.join("game")
        } else {
            input_path.to_path_buf()
        };

        println!("[BGA-Rust] Base path: {:?}", input_path);
        println!("[BGA-Rust] Searching in: {:?}", game_path);

        // Check for .rpyc files and decompile if necessary (Recursive)
        let has_rpyc = WalkDir::new(&game_path)
            .into_iter()
            .filter_map(|e| e.ok())
            .any(|e| {
                e.file_type().is_file()
                    && e.path()
                        .extension()
                        .map(|ext| ext == "rpyc")
                        .unwrap_or(false)
            });

        println!("[BGA-Rust] Found .rpyc files: {}", has_rpyc);

        if has_rpyc {
            if let Some(unrpyc_cmd) = Self::find_unrpyc() {
                println!("[BGA-Rust] Using unrpyc command: {:?}", unrpyc_cmd);
                if let Err(e) = Self::decompile_rpyc(&game_path, &unrpyc_cmd) {
                    // Log error but continue if some .rpy exist
                    println!("[BGA-Rust] Decompile error: {}", e);
                }
            } else {
                println!("[BGA-Rust] unrpyc not found");
            }
        }

        // Extract from .rpy files (Recursive)
        let mut all_entries = Vec::new();
        let mut rpy_count = 0;

        for entry in WalkDir::new(&game_path).into_iter().filter_map(|e| e.ok()) {
            if entry.file_type().is_file() {
                if let Some(ext) = entry.path().extension() {
                    if ext == "rpy" {
                        rpy_count += 1;
                        let entries = Self::extract_from_rpy(entry.path());
                        all_entries.extend(entries);
                    }
                }
            }
        }

        println!("[BGA-Rust] Found {} .rpy files", rpy_count);
        println!("[BGA-Rust] Extracted {} strings", all_entries.len());

        if all_entries.is_empty() {
            return AnalyzerOutput::error("No translatable strings found in .rpy files");
        }

        let result = json!({
            "engine": "renpy",
            "source": input_path.to_string_lossy(),
            "strings": all_entries,
        });

        AnalyzerOutput::success(serde_json::to_string_pretty(&result).unwrap_or_default())
    }

    fn save(&self, texts: &[TextEntry]) -> Result<(), String> {
        // For Ren'Py, we typically generate a translation file in 'game/tl/...'
        // or just a single translations.rpy for now.
        // We'll use the path in the first entry to find the game directory
        let game_dir = if let Some(first) = texts.first() {
            let real_path = first.path.split('!').next().unwrap_or(&first.path);
            let path = Path::new(real_path);
            let mut current = path.parent();
            let mut found_game = None;
            while let Some(p) = current {
                if p.file_name().map(|n| n == "game").unwrap_or(false) {
                    found_game = Some(p);
                    break;
                }
                current = p.parent();
            }
            found_game.unwrap_or_else(|| Path::new(".")).to_path_buf()
        } else {
            Path::new(".").to_path_buf()
        };

        let output_dir = game_dir.join("tl").join("Thai");
        fs::create_dir_all(&output_dir)
            .map_err(|e| format!("Failed to create translation directory: {}", e))?;

        write_language_bootstrap(&game_dir, "Thai")
            .map_err(|e| format!("Failed to create language bootstrap: {}", e))?;

        let output_path = output_dir.join("nst_translations.rpy");

        let mut file = fs::File::create(&output_path)
            .map_err(|e| format!("Failed to create output file: {}", e))?;

        writeln!(file, "# Generated by NST\ntranslate Thai strings:\n")
            .map_err(|e| format!("Write error: {}", e))?;

        for entry in texts {
            if let Some(translation) = &entry.text {
                if !translation.is_empty() {
                    writeln!(file, "    old \"{}\"", escape_rpy(&entry.source))
                        .map_err(|e| format!("Write error: {}", e))?;
                    writeln!(file, "    new \"{}\"", escape_rpy(translation))
                        .map_err(|e| format!("Write error: {}", e))?;
                    writeln!(file).map_err(|e| format!("Write error: {}", e))?;
                }
            }
        }

        Ok(())
    }
}

fn escape_rpy(s: &str) -> String {
    s.replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('\n', "\\n")
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
