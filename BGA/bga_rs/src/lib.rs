//! BGA (Background Game Analyzer) - Rust Core Library
//!
//! This library provides game string extraction and injection for various game engines.

pub mod analyzer;
pub mod engines;
mod ffi;

pub use analyzer::{AnalyzerOutput, GameAnalyzer, TextEntry};
pub use engines::{godot::GodotAnalyzer, renpy::RenpyAnalyzer, rpgm::RpgmAnalyzer, unity::UnityAnalyzer};

// Re-export FFI functions
pub use ffi::*;
