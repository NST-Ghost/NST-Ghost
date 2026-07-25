use std::collections::{HashMap, HashSet};
use std::ffi::{c_char, CStr, CString};
use std::fs;
use std::path::Path;
use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};
use serde_json::{json, Map, Value};
use walkdir::WalkDir;

// --- Models (Simplified versions of those in BGA) ---

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TextEntry {
    pub source: String,
    pub path: String,
    pub key: String,
    pub text: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AnalyzerOutput {
    pub format: String,
    pub payload: String,
    pub error_message: Option<String>,
}

impl AnalyzerOutput {
    pub fn success(payload: String) -> Self {
        Self {
            format: "application/json".to_string(),
            payload,
            error_message: None,
        }
    }

    pub fn error(message: String) -> Self {
        Self {
            format: "application/json".to_string(),
            payload: String::new(),
            error_message: Some(message),
        }
    }
}

// --- RPG Maker Implementation (Copied/Adapted from BGA engines/rpgm.rs) ---

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u16)]
pub enum EventCode {
    ShowTextSetup = 101,
    ShowTextLine = 401,
    ShowChoices = 102,
    InputNumber = 103,
    SelectItem = 104,
    ShowScrollingText = 105,
    ShowScrollingTextLine = 405,
    Comment = 108,
    CommentContinuation = 408,
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
    ChangeGold = 125,
    ChangeItems = 126,
    ChangeWeapons = 127,
    ChangeArmors = 128,
    ChangePartyMember = 129,
    TransferPlayer = 201,
    SetVehicleLocation = 202,
    SetEventLocation = 203,
    ScrollMap = 204,
    SetMovementRoute = 205,
    ShowPicture = 231,
    PlayBGM = 241,
    FadeoutBGM = 242,
    PlayBGS = 245,
    FadeoutBGS = 246,
    PlayME = 249,
    PlaySE = 250,
    BattleProcessing = 301,
    ChangeHP = 311,
    ChangeMP = 312,
    ChangeState = 313,
    RecoverAll = 314,
    ForceAction = 339,
    ChangeName = 320,
    ChangeNickname = 324,
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

static URL_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"(?i)^(https?|ftp|file)://").unwrap());
static FILE_EXT_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"(?i)\.(png|jpg|jpeg|gif|bmp|wav|ogg|m4a|mp3|json|js)$").unwrap());
static CONTROL_CODE_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"(?i)^\\[a-z]\[\d+\]$").unwrap());
static EVENT_NAME_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"(?i)^EV\d{3,}$").unwrap());
static PLUGIN_CMD_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"(?i)^[A-Z][a-zA-Z]+ (open|close|add|remove|set|get|show|hide|enable|disable)").unwrap());
static SYMBOL_ONLY_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"^[^a-zA-Z0-9\p{Thai}\p{Han}\p{Hiragana}\p{Katakana}\p{Hangul}\p{Cyrillic}\p{Arabic}]+$").unwrap());
static ARRAY_INDEX_PATTERN: Lazy<Regex> = Lazy::new(|| Regex::new(r"\[(\d+)\]").unwrap());

static WHITELISTED_KEYS: Lazy<HashSet<&'static str>> = Lazy::new(|| {
    ["name", "description", "message1", "message2", "message3", "message4", "note", "nickname", "profile", "gameTitle", "currencyUnit", "terms", "basic", "commands", "params", "messages", "actionFailure", "actorDamage", "actorDrain", "actorGain", "actorLoss", "actorNoDamage", "actorNoHit", "actorRecovery", "alwaysDash", "bgmVolume", "bgsVolume", "buffAdd", "buffRemove", "commandRemember", "counterAttack", "criticalToActor", "criticalToEnemy", "debuffAdd", "defeat", "emerge", "enemyDamage", "enemyDrain", "enemyGain", "enemyLoss", "enemyNoDamage", "enemyNoHit", "enemyRecovery", "escapeFailure", "escapeStart", "evasion", "expNext", "expTotal", "file", "levelUp", "loadMessage", "magicEvasion", "magicReflection", "meVolume", "obtainExp", "obtainGold", "obtainItem", "obtainSkill", "partyName", "possession", "preemptive", "saveMessage", "seVolume", "substitute", "surprise", "useItem", "victory", "title", "memo", "text", "caption", "label", "comment"].into_iter().collect()
});

static BLACKLISTED_KEYS: Lazy<HashSet<&'static str>> = Lazy::new(|| {
    ["se", "bgm", "bgs", "me", "animation1Name", "animation2Name", "battlerName", "characterName", "faceName", "motion", "overlay1Name", "overlay2Name", "tileset", "parallaxName", "battleback1Name", "battleback2Name", "script", "url"].into_iter().collect()
});

static SYSTEM_PREFIXES: &[&str] = &["img/", "audio/", "data/", "js/", "fonts/", "Actor", "Class", "Skill", "Item", "Weapon", "Armor", "Enemy", "Troop", "State", "Animation", "Tileset", "CommonEvent", "System", "MapInfo"];

pub struct RpgmAnalyzer;

impl RpgmAnalyzer {
    fn is_system_string(text: &str) -> bool {
        let trimmed = text.trim();
        if trimmed.is_empty() || trimmed.parse::<f64>().is_ok() || text.contains('/') || URL_PATTERN.is_match(text) || FILE_EXT_PATTERN.is_match(text) || CONTROL_CODE_PATTERN.is_match(trimmed) || EVENT_NAME_PATTERN.is_match(text) || PLUGIN_CMD_PATTERN.is_match(text) || text.to_lowercase().contains("$game") || SYMBOL_ONLY_PATTERN.is_match(text) {
            return true;
        }
        for prefix in SYSTEM_PREFIXES {
            if text.to_lowercase().starts_with(&prefix.to_lowercase()) {
                return true;
            }
        }
        false
    }

    fn is_audio_object(obj: &Map<String, Value>) -> bool {
        obj.contains_key("name") && obj.contains_key("volume") && obj.contains_key("pitch") && obj.contains_key("pan")
    }

    fn extract_strings(value: &Value, entries: &mut Vec<TextEntry>, file_path: &str, key_path: &str) {
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
                if Self::is_audio_object(obj) { return; }
                if let (Some(Value::Number(code)), Some(Value::Array(params))) = (obj.get("code"), obj.get("parameters")) {
                    if let Some(code_val) = code.as_i64() {
                        let extracted = Self::extract_from_event_command(code_val, params, entries, file_path, key_path);
                        for (key, val) in obj {
                            if (key == "parameters" && extracted) || key == "code" || key == "indent" { continue; }
                            let new_path = if key_path.is_empty() { key.clone() } else { format!("{}.{}", key_path, key) };
                            if val.is_array() || val.is_object() { Self::extract_strings(val, entries, file_path, &new_path); }
                        }
                        return;
                    }
                }
                for (key, val) in obj {
                    if BLACKLISTED_KEYS.contains(key.as_str()) { continue; }
                    let new_path = if key_path.is_empty() { key.clone() } else { format!("{}.{}", key_path, key) };
                    match val {
                        Value::String(text) => {
                            if WHITELISTED_KEYS.contains(key.as_str()) && !text.is_empty() && !Self::is_system_string(text) {
                                entries.push(TextEntry {
                                    source: text.clone(),
                                    path: file_path.to_string(),
                                    key: new_path,
                                    text: None,
                                });
                            }
                        }
                        Value::Array(_) | Value::Object(_) => { Self::extract_strings(val, entries, file_path, &new_path); }
                        _ => {}
                    }
                }
            }
            Value::Array(arr) => {
                for (i, item) in arr.iter().enumerate() {
                    let new_path = if key_path.is_empty() { i.to_string() } else { format!("{}[{}]", key_path, i) };
                    Self::extract_strings(item, entries, file_path, &new_path);
                }
            }
            _ => {}
        }
    }

    fn extract_from_event_command(code: i64, params: &[Value], entries: &mut Vec<TextEntry>, file_path: &str, key_path: &str) -> bool {
        let event_code = match EventCode::from_i64(code) { Some(c) => c, None => return false };
        let make_param_path = |idx: usize| -> String { if key_path.is_empty() { format!("parameters[{}]", idx) } else { format!("{}.parameters[{}]", key_path, idx) } };
        match event_code {
            EventCode::ShowTextSetup => {
                if let Some(Value::String(name)) = params.get(4) {
                    if !name.is_empty() && !Self::is_system_string(name) {
                        entries.push(TextEntry { source: name.clone(), path: file_path.to_string(), key: make_param_path(4), text: None });
                    }
                }
                true
            }
            EventCode::ShowTextLine | EventCode::ShowScrollingText | EventCode::ShowScrollingTextLine => {
                if let Some(Value::String(text)) = params.get(0) {
                    if !text.is_empty() && !Self::is_system_string(text) {
                        entries.push(TextEntry { source: text.clone(), path: file_path.to_string(), key: make_param_path(0), text: None });
                    }
                }
                true
            }
            EventCode::ShowChoices => {
                if let Some(Value::Array(choices)) = params.get(0) {
                    for (i, choice) in choices.iter().enumerate() {
                        if let Value::String(text) = choice {
                            if !text.is_empty() && !Self::is_system_string(text) {
                                let path = if key_path.is_empty() { format!("parameters[0][{}]", i) } else { format!("{}.parameters[0][{}]", key_path, i) };
                                entries.push(TextEntry { source: text.clone(), path: file_path.to_string(), key: path, text: None });
                            }
                        }
                    }
                }
                true
            }
            EventCode::ChangeName | EventCode::ChangeNickname => {
                if let Some(Value::String(name)) = params.get(1) {
                    if !name.is_empty() && !Self::is_system_string(name) {
                        entries.push(TextEntry { source: name.clone(), path: file_path.to_string(), key: make_param_path(1), text: None });
                    }
                }
                true
            }
            _ => true,
        }
    }

    fn update_json_value(doc: &mut Value, key_path: &str, new_value: &str) -> bool {
        let parts: Vec<&str> = key_path.split('.').collect();
        Self::update_value_recursive(doc, &parts, 0, new_value)
    }

    fn update_value_recursive(current: &mut Value, parts: &[&str], index: usize, new_value: &str) -> bool {
        if index >= parts.len() { return false; }
        let part = parts[index];
        if part.contains('[') {
            let bracket_pos = part.find('[').unwrap();
            let base_key = &part[..bracket_pos];
            let indices_str = &part[bracket_pos..];
            let indices: Vec<usize> = ARRAY_INDEX_PATTERN.captures_iter(indices_str).filter_map(|cap| cap.get(1).and_then(|m| m.as_str().parse().ok())).collect();
            if indices.is_empty() { return false; }
            let target = if base_key.is_empty() { current } else {
                match current {
                    Value::Object(obj) => match obj.get_mut(base_key) { Some(v) => v, None => return false },
                    _ => return false,
                }
            };
            let mut nav = target;
            for (i, &idx) in indices.iter().enumerate() {
                let is_last_index = i == indices.len() - 1 && index == parts.len() - 1;
                match nav {
                    Value::Array(arr) => {
                        if idx >= arr.len() { return false; }
                        if is_last_index { arr[idx] = Value::String(new_value.to_string()); return true; }
                        nav = &mut arr[idx];
                    }
                    _ => return false,
                }
            }
            if index < parts.len() - 1 { return Self::update_value_recursive(nav, parts, index + 1, new_value); }
            false
        } else {
            match current {
                Value::Object(obj) => {
                    if index == parts.len() - 1 {
                        obj.insert(part.to_string(), Value::String(new_value.to_string()));
                        return true;
                    } else {
                        match obj.get_mut(part) {
                            Some(v) => Self::update_value_recursive(v, parts, index + 1, new_value),
                            None => {
                                obj.insert(part.to_string(), Value::Object(Map::new()));
                                Self::update_value_recursive(obj.get_mut(part).unwrap(), parts, index + 1, new_value)
                            },
                        }
                    }
                }
                Value::Array(arr) => {
                    if let Ok(idx) = part.parse::<usize>() {
                        if idx < arr.len() {
                            if index == parts.len() - 1 { arr[idx] = Value::String(new_value.to_string()); return true; }
                            return Self::update_value_recursive(&mut arr[idx], parts, index + 1, new_value);
                        }
                    }
                    false
                }
                _ => false,
            }
        }
    }

    fn find_json_files(project_path: &Path) -> Vec<std::path::PathBuf> {
        let mut files = Vec::new();
        let data_dirs = [
            project_path.join("data"),
            project_path.join("Data"),
            project_path.join("www/data"),
            project_path.join("www/Data"),
            project_path.join("Resources/data"),
            project_path.join("Resources/Data"),
            project_path.join("Resources"),
            project_path.join("js/plugins"),
            project_path.join("www/js/plugins"),
            project_path.join("Resources/js/plugins"),
        ];
        for dir in &data_dirs {
            if dir.exists() {
                for entry in WalkDir::new(dir).max_depth(1).into_iter().filter_map(|e| e.ok()) {
                    if entry.file_type().is_file() && entry.path().extension().map(|e| e == "json").unwrap_or(false) {
                        let name = entry.file_name().to_string_lossy().to_lowercase();
                        if name != "package.json" {
                            files.push(entry.into_path());
                        }
                    }
                }
            }
        }
        if files.is_empty() {
            for entry in WalkDir::new(project_path).into_iter().filter_map(|e| e.ok()) {
                if entry.file_type().is_file() && entry.path().extension().map(|e| e == "json").unwrap_or(false) {
                    let name = entry.file_name().to_string_lossy().to_lowercase();
                    if name != "package.json" { files.push(entry.into_path()); }
                }
            }
        }
        files
    }
}

// --- FFI ---

#[no_mangle]
pub unsafe extern "C" fn rpgm_analyze(path: *const c_char) -> *mut c_char {
    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let input_path = Path::new(path_str);
    let json_files = RpgmAnalyzer::find_json_files(input_path);
    let mut entries = Vec::new();
    for file_path in &json_files {
        if let Ok(content) = fs::read_to_string(file_path) {
            if let Ok(doc) = serde_json::from_str::<Value>(&content) {
                RpgmAnalyzer::extract_strings(&doc, &mut entries, &file_path.to_string_lossy(), "");
            }
        }
    }
    let result = json!({ "strings": entries, "engine": "rpgm" });
    let payload = serde_json::to_string(&result).unwrap_or_default();
    CString::new(payload).unwrap().into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn rpgm_save(path: *const c_char, texts_json: *const c_char) -> i32 {
    let json_str = match CStr::from_ptr(texts_json).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };
    let texts: Vec<TextEntry> = match serde_json::from_str(json_str) {
        Ok(t) => t,
        Err(_) => return -2,
    };
    let mut updates_by_file: HashMap<String, Vec<(&str, &str)>> = HashMap::new();
    for entry in &texts {
        if let Some(text) = &entry.text {
            updates_by_file.entry(entry.path.clone()).or_default().push((&entry.key, text));
        }
    }
    for (file_path, updates) in updates_by_file {
        if let Ok(content) = fs::read_to_string(&file_path) {
            if let Ok(mut doc) = serde_json::from_str::<Value>(&content) {
                for (key_path, new_value) in updates {
                    RpgmAnalyzer::update_json_value(&mut doc, key_path, new_value);
                }
                // Automatically disable imageFontFlag for PGMMV games so TTF font rendering is used for translations
                if let Some(font_list) = doc.get_mut("fontList").and_then(|v| v.as_array_mut()) {
                    for font in font_list {
                        if let Some(obj) = font.as_object_mut() {
                            obj.insert("imageFontFlag".to_string(), Value::Bool(false));
                            if let Some(locs) = obj.get_mut("localeSettings").and_then(|l| l.as_object_mut()) {
                                for loc in locs.values_mut() {
                                    if let Some(loc_obj) = loc.as_object_mut() {
                                        loc_obj.insert("imageFontFlag".to_string(), Value::Bool(false));
                                    }
                                }
                            }
                        }
                    }
                }
                if let Ok(output) = serde_json::to_string_pretty(&doc) {
                    let _ = fs::write(&file_path, output);
                }
            }
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn rpgm_free_json(ptr: *mut c_char) {
    if !ptr.is_null() {
       let _ = CString::from_raw(ptr);
    }
}

#[no_mangle]
pub unsafe extern "C" fn rpgm_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        let _ = CString::from_raw(ptr);
    }
}
