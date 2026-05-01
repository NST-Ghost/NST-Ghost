pub mod reader;
pub mod serialized_file;
pub mod object;
pub mod type_tree;
pub mod writer;
pub mod patcher;
pub mod creator;

pub use reader::{BinaryReader, Endian};
pub use serialized_file::SerializedFile;
pub use object::{ObjectInfo, ObjectReader, ExternalRef};
pub use type_tree::{TypeTree, TypeTreeNode, UnityValue, read_typetree, FLAG_ALIGN_BYTES};
pub use writer::write_typetree;
pub use patcher::{rebuild_file, collect_patch_points_v2, ObjectPatchPoint};
pub use creator::create_legacy_font_from_template;
