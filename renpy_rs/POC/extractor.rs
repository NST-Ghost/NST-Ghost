/// Ren'Py .rpy text extractor
/// Produces structured entries that preserve enough context to reconstruct tl files.

use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};
use std::io::{BufRead, BufReader};

static RE_LABEL: Lazy<Regex> = Lazy::new(|| Regex::new(r"^label\s+(\w+)\s*:").unwrap());
static RE_VOICE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"^\s+voice\s+"([^"]+)""#).unwrap());
static RE_DIALOG: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s+(\w+)\s+"((?:[^"\\]|\\.)*)""#).unwrap());
static RE_NARR: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s+"((?:[^"\\]|\\.)*)""#).unwrap());
static RE_MENU: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"^\s+"((?:[^"\\]|\\.)*)"(\s*:)"#).unwrap());

const SKIP_KEYWORDS: &[&str] = &[
    "label", "define", "default", "show", "hide", "play", "stop", "image",
    "scene", "with", "translate", "if", "elif", "else", "return", "python",
    "jump", "call", "pass", "menu", "window", "nvl", "voice", "pause",
    "queue", "layeredimage", "transform", "style", "init",
];

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TextBlock {
    /// Source file name (relative to archive root)
    pub file: String,
    /// Line number in source file (1-based)
    pub line: usize,
    /// Ren'Py label the line is inside (e.g. "start")
    pub label: String,
    /// Type: "dialogue" | "narration" | "choice"
    #[serde(rename = "type")]
    pub kind: String,
    /// Character variable name (dialogue only)
    pub speaker: Option<String>,
    /// Original English text
    pub source: String,
    /// Translated text (filled in by translate stage)
    pub translation: Option<String>,
    /// voice line that precedes this line (if any)
    pub voice: Option<String>,
}

pub fn extract_from_bytes(filename: &str, bytes: &[u8]) -> Vec<TextBlock> {
    let mut entries = Vec::new();
    let reader = BufReader::new(bytes);
    let mut current_label = String::from("none");
    let mut pending_voice: Option<String> = None;

    for (idx, line_result) in reader.lines().enumerate() {
        let line = match line_result {
            Ok(l) => l,
            Err(_) => continue,
        };
        let line_no = idx + 1;

        // Track current label
        if let Some(caps) = RE_LABEL.captures(&line) {
            current_label = caps.get(1).unwrap().as_str().to_string();
            pending_voice = None;
            continue;
        }

        // Track voice line
        if let Some(caps) = RE_VOICE.captures(&line) {
            pending_voice = Some(caps.get(1).unwrap().as_str().to_string());
            continue;
        }

        // Menu choice (line ends with colon)
        if let Some(caps) = RE_MENU.captures(&line) {
            let text = caps.get(1).unwrap().as_str().to_string();
            if !text.trim().is_empty() {
                entries.push(TextBlock {
                    file: filename.to_string(),
                    line: line_no,
                    label: current_label.clone(),
                    kind: "choice".to_string(),
                    speaker: None,
                    source: text,
                    translation: None,
                    voice: pending_voice.take(),
                });
            }
            pending_voice = None;
            continue;
        }

        // Character dialogue: speaker "text"
        if let Some(caps) = RE_DIALOG.captures(&line) {
            let spk = caps.get(1).unwrap().as_str();
            let text = caps.get(2).unwrap().as_str().to_string();
            if !SKIP_KEYWORDS.contains(&spk) && !text.trim().is_empty() {
                entries.push(TextBlock {
                    file: filename.to_string(),
                    line: line_no,
                    label: current_label.clone(),
                    kind: "dialogue".to_string(),
                    speaker: Some(spk.to_string()),
                    source: text,
                    translation: None,
                    voice: pending_voice.take(),
                });
                pending_voice = None;
                continue;
            }
        }

        // Narration: bare "text"
        if let Some(caps) = RE_NARR.captures(&line) {
            let text = caps.get(1).unwrap().as_str().to_string();
            if !text.trim().is_empty() {
                entries.push(TextBlock {
                    file: filename.to_string(),
                    line: line_no,
                    label: current_label.clone(),
                    kind: "narration".to_string(),
                    speaker: None,
                    source: text,
                    translation: None,
                    voice: pending_voice.take(),
                });
            }
        }

        pending_voice = None;
    }

    entries
}
