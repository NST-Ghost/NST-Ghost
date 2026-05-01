pub mod reader;
pub mod serialized_file;
pub mod object;
pub mod type_tree;

pub use reader::{BinaryReader, Endian};
pub use serialized_file::SerializedFile;
pub use object::{ObjectInfo, ObjectReader, ExternalRef};
pub use type_tree::{TypeTree, TypeTreeNode, UnityValue, read_typetree, FLAG_ALIGN_BYTES};
