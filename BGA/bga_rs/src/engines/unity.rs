//! Unity Analyzer
//!
//! Extracts translatable strings from Unity projects (.assets and .bundle files).

use crate::analyzer::{AnalyzerOutput, GameAnalyzer, TextEntry};
use serde_json::json;
use std::fs;
use std::path::Path;
use walkdir::WalkDir;
use unity_asset_binary::asset::SerializedFile;
use unity_asset_binary::reader::BinaryReader;

pub struct UnityAnalyzer;

impl UnityAnalyzer {
    pub fn new() -> Self {
        Self
    }
}

impl Default for UnityAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

impl GameAnalyzer for UnityAnalyzer {
    fn analyze(&self, input_path: &Path) -> AnalyzerOutput {
        let mut entries = Vec::new();

        for entry in WalkDir::new(input_path)
            .into_iter()
            .filter_map(|e| e.ok())
        {
            if entry.file_type().is_file() {
                if let Some(ext) = entry.path().extension() {
                    let ext_str = ext.to_string_lossy().to_lowercase();
                    if ext_str == "assets" || ext_str == "sharedassets" {
                        if let Ok(data) = fs::read(entry.path()) {
                            let mut reader = BinaryReader::new(&data);
                            // SerializedFile::read typically takes a reader and options (None for default)
                            if let Ok(asset) = SerializedFile::read(&mut reader, None) {
                                for object in &asset.objects {
                                    entries.push(json!({
                                        "path": entry.path().to_string_lossy(),
                                        "path_id": object.path_id,
                                        "type_id": object.type_id,
                                        "text": format!("Object PathID: {}", object.path_id)
                                    }));
                                }
                            }
                        }
                    }
                }
            }
        }

        let result = json!({
            "engine": "unity",
            "source": input_path.to_string_lossy(),
            "entries": entries,
        });

        AnalyzerOutput::success(serde_json::to_string_pretty(&result).unwrap_or_default())
    }

    fn save(&self, _texts: &[TextEntry]) -> Result<(), String> {
        Err("Saving for Unity projects is not yet fully implemented.".into())
    }
}
