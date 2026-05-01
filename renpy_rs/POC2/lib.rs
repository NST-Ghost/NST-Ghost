//! renpy_rs – Ren'Py translation toolkit
//!
//! Module layout
//! ─────────────
//! rpa         → RPA-3.0 archive parser
//! extractor   → .rpy source text extractor  (produces TextBlock)
//! translator  → Anthropic API batch translator
//! patcher     → tl/<lang>/*.rpy file generator
//! ffi         → C-ABI exports (for embedding in other tools)

pub mod extractor;
pub mod ffi;
pub mod patcher;
pub mod rpa;
pub mod translator;

// Convenient re-exports so callers can write `renpy_rs::TextBlock` etc.
pub use extractor::TextBlock;
pub use patcher::{build_translation_map, font_override_block, patch_tl_file};
pub use rpa::{parse_rpa, read_rpa_file, RpaEntry};
pub use translator::translate_blocks;
