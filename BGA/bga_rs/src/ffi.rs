//! C FFI (Foreign Function Interface) for BGA
//!
//! These functions allow C/C++ code to call into the Rust library.

use crate::analyzer::TextEntry;
use crate::engines::{renpy::RenpyAnalyzer, rpgm::RpgmAnalyzer, unity::UnityAnalyzer};
use crate::GameAnalyzer;
use std::ffi::{c_char, CStr, CString};
use std::path::Path;

/// Analyze a game project
///
/// # Arguments
/// * `engine` - Engine name ("rpgm", "unity", "renpy")
/// * `path` - Path to the game project
///
/// # Returns
/// A JSON string containing the analysis result (caller must free with `bga_free_string`)
///
/// # Safety
/// Both `engine` and `path` must be valid null-terminated C strings.
#[no_mangle]
pub unsafe extern "C" fn bga_analyze(engine: *const c_char, path: *const c_char) -> *mut c_char {
    let engine_str = match unsafe { CStr::from_ptr(engine) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    let path_str = match unsafe { CStr::from_ptr(path) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    println!("[BGA-Rust-FFI] bga_analyze(engine='{}', path='{}')", engine_str, path_str);

    let result = match engine_str.to_lowercase().as_str() {
        "rpgm" => {
            let analyzer = RpgmAnalyzer::new();
            analyzer.analyze(Path::new(path_str))
        }
        "unity" => {
            let analyzer = UnityAnalyzer::new();
            analyzer.analyze(Path::new(path_str))
        }
        "renpy" => {
            let analyzer = RenpyAnalyzer::new();
            analyzer.analyze(Path::new(path_str))
        }
        _ => {
            println!("[BGA-Rust-FFI] Unknown engine: '{}'", engine_str);
            return std::ptr::null_mut();
        }
    };

    // Convert the payload to a C string
    match CString::new(result.payload) {
        Ok(cstr) => cstr.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Save translated texts back to game files
///
/// # Arguments
/// * `engine` - Engine name ("rpgm", "unity", "renpy")
/// * `texts_json` - JSON array of text entries with translations
///
/// # Returns
/// 0 on success, non-zero on error
///
/// # Safety
/// Both `engine` and `texts_json` must be valid null-terminated C strings.
#[no_mangle]
pub unsafe extern "C" fn bga_save(engine: *const c_char, texts_json: *const c_char) -> i32 {
    let engine_str = match unsafe { CStr::from_ptr(engine) }.to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    let json_str = match unsafe { CStr::from_ptr(texts_json) }.to_str() {
        Ok(s) => s,
        Err(_) => return -2,
    };

    let texts: Vec<TextEntry> = match serde_json::from_str(json_str) {
        Ok(t) => t,
        Err(_) => return -3,
    };

    let result = match engine_str.to_lowercase().as_str() {
        "rpgm" => {
            let analyzer = RpgmAnalyzer::new();
            analyzer.save(&texts)
        }
        "unity" => {
            let analyzer = UnityAnalyzer::new();
            analyzer.save(&texts)
        }
        "renpy" => {
            let analyzer = RenpyAnalyzer::new();
            analyzer.save(&texts)
        }
        _ => return -4,
    };

    match result {
        Ok(()) => 0,
        Err(_) => -5,
    }
}

/// Get available analyzer names
///
/// # Returns
/// A JSON array of analyzer names (caller must free with `bga_free_string`)
#[no_mangle]
pub extern "C" fn bga_available_analyzers() -> *mut c_char {
    let analyzers = vec!["rpgm", "unity", "renpy"];
    let json = serde_json::to_string(&analyzers).unwrap_or_else(|_| "[]".to_string());

    match CString::new(json) {
        Ok(cstr) => cstr.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Free a string allocated by the Rust library
///
/// # Safety
/// `ptr` must be a pointer returned by `bga_analyze`, `bga_available_analyzers`, or similar functions.
#[no_mangle]
pub unsafe extern "C" fn bga_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        let _ = unsafe { CString::from_raw(ptr) };
    }
}

/// Get the library version
#[no_mangle]
pub extern "C" fn bga_version() -> *const c_char {
    static VERSION: &[u8] = b"0.1.0\0";
    VERSION.as_ptr() as *const c_char
}
