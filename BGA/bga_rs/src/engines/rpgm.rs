//! RPG Maker MV/MZ Analyzer
//!
//! Extracts and saves translatable strings from RPG Maker games.

use crate::analyzer::{AnalyzerOutput, GameAnalyzer, TextEntry};
use once_cell::sync::Lazy;
use regex::Regex;
use serde_json::{json, Map, Value};
use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::Path;
use walkdir::WalkDir;

/// RPG Maker MV/MZ event command codes
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u16)]
pub enum EventCode {
    // Text Display Commands
    ShowTextSetup = 101,
    ShowTextLine = 401,
    ShowChoices = 102,
    InputNumber = 103,
    SelectItem = 104,
    ShowScrollingText = 105,
    ShowScrollingTextLine = 405,

    // Comment Commands
    Comment = 108,
    CommentContinuation = 408,

    // Control Flow
    ConditionalBranch = 111,
    Loop = 112,
    BreakLoop = 113,
    ExitEventProcessing = 115,
    CommonEvent = 117,
    Label = 118,
    JumpToLabel = 119,
    ControlSwitches = 121,
    ControlVariables = 122,
    ControlSelfSwitch = 123,
    ControlTimer = 124,

    // Game Progression
    ChangeGold = 125,
    ChangeItems = 126,
    ChangeWeapons = 127,
    ChangeArmors = 128,
    ChangePartyMember = 129,

    // Screen Commands
    TransferPlayer = 201,
    SetVehicleLocation = 202,
    SetEventLocation = 203,
    ScrollMap = 204,
    SetMovementRoute = 205,
    ShowPicture = 231,

    // Audio Commands
    PlayBGM = 241,
    FadeoutBGM = 242,
    PlayBGS = 245,
    FadeoutBGS = 246,
    PlayME = 249,
    PlaySE = 250,

    // Battle Commands
    BattleProcessing = 301,
    ChangeHP = 311,
    ChangeMP = 312,
    ChangeState = 313,
    RecoverAll = 314,
    ForceAction = 339,

    // Actor Commands
    ChangeName = 320,
    ChangeNickname = 324,

    // Plugin/Script Commands
    Script = 355,
    PluginCommandMV = 356,
    PluginCommandMZ = 357,
    ScriptContinuation = 655,
}

impl EventCode {
    fn from_i64(code: i64) -> Option<Self> {
        match code {
            101 => Some(Self::ShowTextSetup),
            401 => Some(Self::ShowTextLine),
            102 => Some(Self::ShowChoices),
            103 => Some(Self::InputNumber),
            104 => Some(Self::SelectItem),
            105 => Some(Self::ShowScrollingText),
            405 => Some(Self::ShowScrollingTextLine),
            108 => Some(Self::Comment),
            408 => Some(Self::CommentContinuation),
            111 => Some(Self::ConditionalBranch),
            112 => Some(Self::Loop),
            113 => Some(Self::BreakLoop),
            115 => Some(Self::ExitEventProcessing),
            117 => Some(Self::CommonEvent),
            118 => Some(Self::Label),
            119 => Some(Self::JumpToLabel),
            121 => Some(Self::ControlSwitches),
            122 => Some(Self::ControlVariables),
            123 => Some(Self::ControlSelfSwitch),
            124 => Some(Self::ControlTimer),
            125 => Some(Self::ChangeGold),
            126 => Some(Self::ChangeItems),
            127 => Some(Self::ChangeWeapons),
            128 => Some(Self::ChangeArmors),
            129 => Some(Self::ChangePartyMember),
            201 => Some(Self::TransferPlayer),
            202 => Some(Self::SetVehicleLocation),
            203 => Some(Self::SetEventLocation),
            204 => Some(Self::ScrollMap),
            205 => Some(Self::SetMovementRoute),
            231 => Some(Self::ShowPicture),
            241 => Some(Self::PlayBGM),
            242 => Some(Self::FadeoutBGM),
            245 => Some(Self::PlayBGS),
            246 => Some(Self::FadeoutBGS),
            249 => Some(Self::PlayME),
            250 => Some(Self::PlaySE),
            301 => Some(Self::BattleProcessing),
            311 => Some(Self::ChangeHP),
            312 => Some(Self::ChangeMP),
            313 => Some(Self::ChangeState),
            314 => Some(Self::RecoverAll),
            339 => Some(Self::ForceAction),
            320 => Some(Self::ChangeName),
            324 => Some(Self::ChangeNickname),
            355 => Some(Self::Script),
            356 => Some(Self::PluginCommandMV),
            357 => Some(Self::PluginCommandMZ),
            655 => Some(Self::ScriptContinuation),
            _ => None,
        }
    }
}

// Lazy-initialized regex patterns
static URL_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"(?i)^(https?|ftp|file)://").unwrap());

static FILE_EXT_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"(?i)\.(png|jpg|jpeg|gif|bmp|wav|ogg|m4a|mp3|json|js)$").unwrap());

static CONTROL_CODE_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"(?i)^\\[a-z]\[\d+\]$").unwrap());

static EVENT_NAME_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"(?i)^EV\d{3,}$").unwrap());

static PLUGIN_CMD_PATTERN: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"(?i)^[A-Z][a-zA-Z]+ (open|close|add|remove|set|get|show|hide|enable|disable)").unwrap());

static SYMBOL_ONLY_PATTERN: Lazy<Regex> = Lazy::new(|| {
    Regex::new(r"^[^a-zA-Z0-9\p{Thai}\p{Han}\p{Hiragana}\p{Katakana}\p{Hangul}\p{Cyrillic}\p{Arabic}]+$").unwrap()
});

static ARRAY_INDEX_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"\[(\d+)\]").unwrap());

/// Whitelisted keys for database objects
static WHITELISTED_KEYS: Lazy<HashSet<&'static str>> = Lazy::new(|| {
    [
        "name", "description", "message1", "message2", "message3", "message4",
        "note", "nickname", "profile", "gameTitle", "currencyUnit",
        "terms", "basic", "commands", "params", "messages",
        "actionFailure", "actorDamage", "actorDrain", "actorGain", "actorLoss",
        "actorNoDamage", "actorNoHit", "actorRecovery", "alwaysDash", "bgmVolume",
        "bgsVolume", "buffAdd", "buffRemove", "commandRemember", "counterAttack",
        "criticalToActor", "criticalToEnemy", "debuffAdd", "defeat", "emerge",
        "enemyDamage", "enemyDrain", "enemyGain", "enemyLoss", "enemyNoDamage",
        "enemyNoHit", "enemyRecovery", "escapeFailure", "escapeStart", "evasion",
        "expNext", "expTotal", "file", "levelUp", "loadMessage", "magicEvasion",
        "magicReflection", "meVolume", "obtainExp", "obtainGold", "obtainItem",
        "obtainSkill", "partyName", "possession", "preemptive", "saveMessage",
        "seVolume", "substitute", "surprise", "useItem", "victory",
    ]
    .into_iter()
    .collect()
});

/// Blacklisted keys (contain filenames/technical data)
static BLACKLISTED_KEYS: Lazy<HashSet<&'static str>> = Lazy::new(|| {
    [
        "se", "bgm", "bgs", "me",
        "animation1Name", "animation2Name",
        "battlerName", "characterName", "faceName",
        "motion", "overlay1Name", "overlay2Name",
        "tileset", "parallaxName", "battleback1Name", "battleback2Name",
        "script", "url",
    ]
    .into_iter()
    .collect()
});

/// System prefixes that indicate non-translatable strings
static SYSTEM_PREFIXES: &[&str] = &[
    "img/", "audio/", "data/", "js/", "fonts/",
    "Actor", "Class", "Skill", "Item", "Weapon", "Armor",
    "Enemy", "Troop", "State", "Animation", "Tileset",
    "CommonEvent", "System", "MapInfo",
];

pub struct RpgmAnalyzer;

impl RpgmAnalyzer {
    pub fn new() -> Self {
        Self
    }

    /// Check if a string is a system/technical string that shouldn't be translated
    fn is_system_string(text: &str) -> bool {
        let trimmed = text.trim();

        // Empty or whitespace only
        if trimmed.is_empty() {
            return true;
        }

        // Pure numbers
        if trimmed.parse::<f64>().is_ok() {
            return true;
        }

        // Paths (contains /)
        if text.contains('/') {
            return true;
        }

        // URLs
        if URL_PATTERN.is_match(text) {
            return true;
        }

        // File extensions
        if FILE_EXT_PATTERN.is_match(text) {
            return true;
        }

        // System prefixes
        for prefix in SYSTEM_PREFIXES {
            if text.to_lowercase().starts_with(&prefix.to_lowercase()) {
                return true;
            }
        }

        // Control codes (\\c[1], \\n[1], etc.)
        if CONTROL_CODE_PATTERN.is_match(trimmed) {
            return true;
        }

        // Generic event names (EV001, EV030, etc.)
        if EVENT_NAME_PATTERN.is_match(text) {
            return true;
        }

        // Plugin command patterns
        if PLUGIN_CMD_PATTERN.is_match(text) {
            return true;
        }

        // Variable references
        if text.to_lowercase().contains("$game") {
            return true;
        }

        // Only symbols/punctuation
        if SYMBOL_ONLY_PATTERN.is_match(text) {
            return true;
        }

        false
    }

    /// Check if an object is an audio configuration object
    fn is_audio_object(obj: &Map<String, Value>) -> bool {
        obj.contains_key("name")
            && obj.contains_key("volume")
            && obj.contains_key("pitch")
            && obj.contains_key("pan")
    }

    /// Extract strings from a JSON value recursively
    fn extract_strings(
        value: &Value,
        entries: &mut Vec<TextEntry>,
        file_path: &str,
        key_path: &str,
    ) {
        match value {
            Value::String(text) => {
                if !text.is_empty() && !Self::is_system_string(text) {
                    entries.push(TextEntry {
                        source: text.clone(),
                        path: file_path.to_string(),
                        key: key_path.to_string(),
                        text: None,
                    });
                }
            }
            Value::Object(obj) => {
                // Skip audio objects
                if Self::is_audio_object(obj) {
                    return;
                }

                // Check for event command structure
                if let (Some(Value::Number(code)), Some(Value::Array(params))) =
                    (obj.get("code"), obj.get("parameters"))
                {
                    if let Some(code_val) = code.as_i64() {
                        let extracted = Self::extract_from_event_command(
                            code_val, params, entries, file_path, key_path,
                        );

                        // Recurse into non-parameter fields
                        for (key, val) in obj {
                            if key == "parameters" && extracted {
                                continue;
                            }
                            if key == "code" || key == "indent" {
                                continue;
                            }

                            let new_path = if key_path.is_empty() {
                                key.clone()
                            } else {
                                format!("{}.{}", key_path, key)
                            };

                            if val.is_array() || val.is_object() {
                                Self::extract_strings(val, entries, file_path, &new_path);
                            }
                        }
                        return;
                    }
                }

                // Regular object processing with whitelist
                for (key, val) in obj {
                    if BLACKLISTED_KEYS.contains(key.as_str()) {
                        continue;
                    }

                    let new_path = if key_path.is_empty() {
                        key.clone()
                    } else {
                        format!("{}.{}", key_path, key)
                    };

                    match val {
                        Value::String(text) => {
                            if WHITELISTED_KEYS.contains(key.as_str())
                                && !text.is_empty()
                                && !Self::is_system_string(text)
                            {
                                entries.push(TextEntry {
                                    source: text.clone(),
                                    path: file_path.to_string(),
                                    key: new_path,
                                    text: None,
                                });
                            }
                        }
                        Value::Array(_) | Value::Object(_) => {
                            Self::extract_strings(val, entries, file_path, &new_path);
                        }
                        _ => {}
                    }
                }
            }
            Value::Array(arr) => {
                for (i, item) in arr.iter().enumerate() {
                    let new_path = if key_path.is_empty() {
                        i.to_string()
                    } else {
                        format!("{}[{}]", key_path, i)
                    };
                    Self::extract_strings(item, entries, file_path, &new_path);
                }
            }
            _ => {}
        }
    }

    /// Extract translatable strings from event commands
    fn extract_from_event_command(
        code: i64,
        params: &[Value],
        entries: &mut Vec<TextEntry>,
        file_path: &str,
        key_path: &str,
    ) -> bool {
        let event_code = match EventCode::from_i64(code) {
            Some(c) => c,
            None => return false,
        };

        let make_param_path = |idx: usize| -> String {
            if key_path.is_empty() {
                format!("parameters[{}]", idx)
            } else {
                format!("{}.parameters[{}]", key_path, idx)
            }
        };

        match event_code {
            // Show Text Setup - parameters[4] = actor name
            EventCode::ShowTextSetup => {
                if let Some(Value::String(name)) = params.get(4) {
                    if !name.is_empty() && !Self::is_system_string(name) {
                        entries.push(TextEntry {
                            source: name.clone(),
                            path: file_path.to_string(),
                            key: make_param_path(4),
                            text: None,
                        });
                    }
                }
                true
            }

            // Show Text Line - parameters[0] = dialogue text
            EventCode::ShowTextLine
            | EventCode::ShowScrollingText
            | EventCode::ShowScrollingTextLine => {
                if let Some(Value::String(text)) = params.get(0) {
                    if !text.is_empty() && !Self::is_system_string(text) {
                        entries.push(TextEntry {
                            source: text.clone(),
                            path: file_path.to_string(),
                            key: make_param_path(0),
                            text: None,
                        });
                    }
                }
                true
            }

            // Show Choices - parameters[0] = array of choice texts
            EventCode::ShowChoices => {
                if let Some(Value::Array(choices)) = params.get(0) {
                    for (i, choice) in choices.iter().enumerate() {
                        if let Value::String(text) = choice {
                            if !text.is_empty() && !Self::is_system_string(text) {
                                let path = if key_path.is_empty() {
                                    format!("parameters[0][{}]", i)
                                } else {
                                    format!("{}.parameters[0][{}]", key_path, i)
                                };
                                entries.push(TextEntry {
                                    source: text.clone(),
                                    path: file_path.to_string(),
                                    key: path,
                                    text: None,
                                });
                            }
                        }
                    }
                }
                true
            }

            // Change Name - parameters[1] = new name
            EventCode::ChangeName | EventCode::ChangeNickname => {
                if let Some(Value::String(name)) = params.get(1) {
                    if !name.is_empty() && !Self::is_system_string(name) {
                        entries.push(TextEntry {
                            source: name.clone(),
                            path: file_path.to_string(),
                            key: make_param_path(1),
                            text: None,
                        });
                    }
                }
                true
            }

            // Commands that don't have translatable content
            EventCode::InputNumber
            | EventCode::SelectItem
            | EventCode::Comment
            | EventCode::CommentContinuation
            | EventCode::ConditionalBranch
            | EventCode::Loop
            | EventCode::BreakLoop
            | EventCode::ExitEventProcessing
            | EventCode::CommonEvent
            | EventCode::Label
            | EventCode::JumpToLabel
            | EventCode::ControlSwitches
            | EventCode::ControlVariables
            | EventCode::ControlSelfSwitch
            | EventCode::ControlTimer
            | EventCode::ChangeGold
            | EventCode::ChangeItems
            | EventCode::ChangeWeapons
            | EventCode::ChangeArmors
            | EventCode::ChangePartyMember
            | EventCode::TransferPlayer
            | EventCode::SetVehicleLocation
            | EventCode::SetEventLocation
            | EventCode::ScrollMap
            | EventCode::SetMovementRoute
            | EventCode::ShowPicture
            | EventCode::PlayBGM
            | EventCode::FadeoutBGM
            | EventCode::PlayBGS
            | EventCode::FadeoutBGS
            | EventCode::PlayME
            | EventCode::PlaySE
            | EventCode::BattleProcessing
            | EventCode::ChangeHP
            | EventCode::ChangeMP
            | EventCode::ChangeState
            | EventCode::RecoverAll
            | EventCode::ForceAction
            | EventCode::Script
            | EventCode::PluginCommandMV
            | EventCode::PluginCommandMZ
            | EventCode::ScriptContinuation => true,
        }
    }

    /// Update a JSON value at the given key path
    fn update_json_value(doc: &mut Value, key_path: &str, new_value: &str) -> bool {
        let parts: Vec<&str> = key_path.split('.').collect();
        Self::update_value_recursive(doc, &parts, 0, new_value)
    }

    fn update_value_recursive(
        current: &mut Value,
        parts: &[&str],
        index: usize,
        new_value: &str,
    ) -> bool {
        if index >= parts.len() {
            return false;
        }

        let part = parts[index];

        // Check if this part contains array indices
        if part.contains('[') {
            let bracket_pos = part.find('[').unwrap();
            let base_key = &part[..bracket_pos];
            let indices_str = &part[bracket_pos..];

            // Extract all array indices
            let indices: Vec<usize> = ARRAY_INDEX_PATTERN
                .captures_iter(indices_str)
                .filter_map(|cap| cap.get(1).and_then(|m| m.as_str().parse().ok()))
                .collect();

            if indices.is_empty() {
                return false;
            }

            // Navigate to the base key first (if not empty)
            let target = if base_key.is_empty() {
                current
            } else {
                match current {
                    Value::Object(obj) => match obj.get_mut(base_key) {
                        Some(v) => v,
                        None => return false,
                    },
                    _ => return false,
                }
            };

            // Navigate through the array indices
            let mut nav = target;
            for (i, &idx) in indices.iter().enumerate() {
                let is_last_index = i == indices.len() - 1 && index == parts.len() - 1;

                match nav {
                    Value::Array(arr) => {
                        if idx >= arr.len() {
                            return false;
                        }
                        if is_last_index {
                            arr[idx] = Value::String(new_value.to_string());
                            return true;
                        }
                        nav = &mut arr[idx];
                    }
                    _ => return false,
                }
            }

            // Continue recursion if not at the end
            if index < parts.len() - 1 {
                return Self::update_value_recursive(nav, parts, index + 1, new_value);
            }

            false
        } else {
            // Simple key navigation
            match current {
                Value::Object(obj) => {
                    if index == parts.len() - 1 {
                        // Last part - update the value
                        if obj.contains_key(part) {
                            obj.insert(part.to_string(), Value::String(new_value.to_string()));
                            return true;
                        }
                        false
                    } else {
                        // Navigate deeper
                        match obj.get_mut(part) {
                            Some(v) => Self::update_value_recursive(v, parts, index + 1, new_value),
                            None => false,
                        }
                    }
                }
                Value::Array(arr) => {
                    if let Ok(idx) = part.parse::<usize>() {
                        if idx < arr.len() {
                            if index == parts.len() - 1 {
                                arr[idx] = Value::String(new_value.to_string());
                                return true;
                            }
                            return Self::update_value_recursive(
                                &mut arr[idx],
                                parts,
                                index + 1,
                                new_value,
                            );
                        }
                    }
                    false
                }
                _ => false,
            }
        }
    }

    /// Find JSON files in the project directory
    fn find_json_files(project_path: &Path) -> Vec<std::path::PathBuf> {
        let mut files = Vec::new();

        // Check if the path is a data folder directly
        let is_data_folder = project_path
            .file_name()
            .map(|n| n.to_string_lossy().to_lowercase() == "data")
            .unwrap_or(false);

        if is_data_folder {
            // Scan the data folder directly
            for entry in WalkDir::new(project_path)
                .max_depth(1)
                .into_iter()
                .filter_map(|e| e.ok())
            {
                if entry.file_type().is_file() {
                    if let Some(ext) = entry.path().extension() {
                        if ext == "json" {
                            files.push(entry.into_path());
                        }
                    }
                }
            }
        } else {
            // Search for data folder (including www/ subdirectory for NW.js packaged games)
            let data_dirs = [
                project_path.join("data"),
                project_path.join("Data"),
                project_path.join("www").join("data"),
                project_path.join("www").join("Data"),
            ];

            for dir in &data_dirs {
                if dir.exists() {
                    for entry in WalkDir::new(dir)
                        .max_depth(1)
                        .into_iter()
                        .filter_map(|e| e.ok())
                    {
                        if entry.file_type().is_file() {
                            if let Some(ext) = entry.path().extension() {
                                if ext == "json" {
                                    files.push(entry.into_path());
                                }
                            }
                        }
                    }
                }
            }

            // Search js/plugins folder (including www/ subdirectory)
            let plugins_dirs = [
                project_path.join("js").join("plugins"),
                project_path.join("www").join("js").join("plugins"),
            ];
            
            for plugins_dir in &plugins_dirs {
                if plugins_dir.exists() {
                    for entry in WalkDir::new(plugins_dir)
                        .max_depth(1)
                        .into_iter()
                        .filter_map(|e| e.ok())
                    {
                        if entry.file_type().is_file() {
                            if let Some(ext) = entry.path().extension() {
                                if ext == "json" {
                                    files.push(entry.into_path());
                                }
                            }
                        }
                    }
                }
            }
        }

        files
    }

    /// Find font files in the project
    fn find_font_files(project_path: &Path) -> Vec<Value> {
        let mut fonts = Vec::new();

        let parent = if project_path
            .file_name()
            .map(|n| n.to_string_lossy().to_lowercase() == "data")
            .unwrap_or(false)
        {
            project_path.parent().unwrap_or(project_path)
        } else {
            project_path
        };

        // Check both root and www/ subdirectory for fonts
        let font_parents = [parent.to_path_buf(), parent.join("www")];
        
        for font_parent in &font_parents {
            for font_dir_name in ["fonts", "Fonts"] {
                let font_dir = font_parent.join(font_dir_name);
                if font_dir.exists() {
                    for entry in WalkDir::new(&font_dir)
                        .max_depth(1)
                        .into_iter()
                        .filter_map(|e| e.ok())
                    {
                        if entry.file_type().is_file() {
                            if let Some(ext) = entry.path().extension() {
                                let ext_lower = ext.to_string_lossy().to_lowercase();
                                if ["ttf", "otf", "woff", "woff2"].contains(&ext_lower.as_str()) {
                                    fonts.push(json!({
                                        "name": entry.path().file_stem()
                                            .map(|s| s.to_string_lossy().to_string())
                                            .unwrap_or_default(),
                                        "path": entry.path().to_string_lossy().to_string()
                                    }));
                                }
                            }
                        }
                    }
                }
            }
        }

        fonts
    }
}

impl Default for RpgmAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

impl GameAnalyzer for RpgmAnalyzer {
    fn analyze(&self, input_path: &Path) -> AnalyzerOutput {
        let json_files = Self::find_json_files(input_path);
        let mut entries = Vec::new();
        let mut processed_count = 0;
        let mut failed_count = 0;

        for file_path in &json_files {
            // Skip package.json
            if let Some(name) = file_path.file_name() {
                if name.to_string_lossy().to_lowercase() == "package.json" {
                    continue;
                }
            }

            match fs::read_to_string(file_path) {
                Ok(content) => match serde_json::from_str::<Value>(&content) {
                    Ok(doc) => {
                        Self::extract_strings(
                            &doc,
                            &mut entries,
                            &file_path.to_string_lossy(),
                            "",
                        );
                        processed_count += 1;
                    }
                    Err(_) => {
                        failed_count += 1;
                    }
                },
                Err(_) => {
                    failed_count += 1;
                }
            }
        }

        let fonts = Self::find_font_files(input_path);

        let result = json!({
            "strings": entries,
            "fonts": fonts,
            "engine": "rpgm",
            "source": input_path.to_string_lossy(),
            "filesProcessed": processed_count,
            "filesFailed": failed_count,
        });

        AnalyzerOutput::success(serde_json::to_string_pretty(&result).unwrap_or_default())
    }

    fn save(&self, texts: &[TextEntry]) -> Result<(), String> {
        // Group updates by file path
        let mut updates_by_file: HashMap<String, Vec<(&str, &str)>> = HashMap::new();

        for entry in texts {
            if let Some(text) = &entry.text {
                updates_by_file
                    .entry(entry.path.clone())
                    .or_default()
                    .push((&entry.key, text));
            }
        }

        // Process each file
        for (file_path, updates) in updates_by_file {
            // Read the file
            let content = fs::read_to_string(&file_path)
                .map_err(|e| format!("Failed to read {}: {}", file_path, e))?;

            let mut doc: Value = serde_json::from_str(&content)
                .map_err(|e| format!("Failed to parse {}: {}", file_path, e))?;

            // Apply all updates
            for (key_path, new_value) in &updates {
                if !Self::update_json_value(&mut doc, key_path, new_value) {
                    eprintln!(
                        "Warning: Failed to update value at {} in {}",
                        key_path, file_path
                    );
                }
            }

            // Create backup
            let backup_path = format!("{}.backup", file_path);
            let _ = fs::remove_file(&backup_path);
            fs::copy(&file_path, &backup_path)
                .map_err(|e| format!("Failed to create backup: {}", e))?;

            // Write updated content
            let output = serde_json::to_string_pretty(&doc)
                .map_err(|e| format!("Failed to serialize: {}", e))?;

            fs::write(&file_path, output).map_err(|e| format!("Failed to write {}: {}", file_path, e))?;

            // Remove backup on success
            let _ = fs::remove_file(&backup_path);
        }

        Ok(())
    }

    fn can_edit_script(&self) -> bool {
        true
    }

    fn get_script_path(&self, project_path: &Path) -> Option<String> {
        let paths = [
            project_path.join("www/js/rpg_windows.js"),
            project_path.join("js/rpg_windows.js"),
        ];

        for path in &paths {
            if path.exists() {
                return Some(path.to_string_lossy().to_string());
            }
        }
        None
    }

    fn get_script_target(&self) -> Option<String> {
        Some("Window_Base.prototype.convertEscapeCharacters".to_string())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_is_system_string() {
        // Should be system strings
        assert!(RpgmAnalyzer::is_system_string(""));
        assert!(RpgmAnalyzer::is_system_string("   "));
        assert!(RpgmAnalyzer::is_system_string("123"));
        assert!(RpgmAnalyzer::is_system_string("42.5"));
        assert!(RpgmAnalyzer::is_system_string("img/pictures/bg01"));
        assert!(RpgmAnalyzer::is_system_string("https://example.com"));
        assert!(RpgmAnalyzer::is_system_string("background.png"));
        assert!(RpgmAnalyzer::is_system_string("EV001"));
        assert!(RpgmAnalyzer::is_system_string("$gameVariables"));

        // Should NOT be system strings
        assert!(!RpgmAnalyzer::is_system_string("Hello, world!"));
        assert!(!RpgmAnalyzer::is_system_string("Yes"));
        assert!(!RpgmAnalyzer::is_system_string("No"));
        assert!(!RpgmAnalyzer::is_system_string("สวัสดี"));
        assert!(!RpgmAnalyzer::is_system_string("こんにちは"));
    }

    #[test]
    fn test_update_json_value() {
        let mut doc = json!({
            "events": [{
                "list": [{
                    "parameters": ["Original text"]
                }]
            }]
        });

        let success =
            RpgmAnalyzer::update_json_value(&mut doc, "events[0].list[0].parameters[0]", "Updated");

        assert!(success);
        assert_eq!(
            doc["events"][0]["list"][0]["parameters"][0],
            "Updated"
        );
    }
}
