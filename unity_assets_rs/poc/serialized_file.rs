use crate::reader::{BinaryReader, Endian};
use crate::type_tree::TypeTree;
use crate::object::{ExternalRef, ObjectInfo, ObjectReader};
use anyhow::{bail, Result};
use std::collections::HashMap;

// ─────────────────────────────────────────────────────────────────────────────
// Header
// ─────────────────────────────────────────────────────────────────────────────

/// Parsed header of a Unity serialized file.
pub struct SerializedFileHeader {
    pub metadata_size: u32,
    pub file_size:     u64,
    /// Serialized-file format version (not the Unity engine version string).
    pub version:       u32,
    /// Absolute byte offset where the data section starts.
    pub data_offset:   u64,
    /// 0 = big-endian, 1 = little-endian.
    pub endian:        u8,
}

impl SerializedFileHeader {
    /// Size of the binary header in bytes (depends on `version`).
    pub fn byte_size(version: u32) -> u64 {
        if version >= 22 { 48 } else if version >= 9 { 16 } else { 12 }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SerializedFile
// ─────────────────────────────────────────────────────────────────────────────

/// A fully-parsed Unity serialized file (`.assets`, CAB-*, etc.).
pub struct SerializedFile {
    pub header:          SerializedFileHeader,
    /// Engine version string, e.g. `"2022.3.14f1"`.
    pub unity_version:   String,
    pub target_platform: u32,
    pub has_type_tree:   bool,
    pub types:           HashMap<i32, TypeTree>,
    pub objects:         Vec<ObjectInfo>,
    pub externals:       Vec<ExternalRef>,
}

impl SerializedFile {
    /// Parse a serialized file from its raw bytes.
    pub fn read(data: &[u8]) -> Result<Self> {
        // ── 1. Header (always big-endian for the first 16 bytes) ─────────────
        let mut header_reader = BinaryReader::new(data, Endian::Big);

        let metadata_size_raw = header_reader.read_u32()?;
        let file_size_raw     = header_reader.read_u32()?;
        let version           = header_reader.read_u32()?;
        let data_offset_raw   = header_reader.read_u32()?;

        // endian flag and reserved bytes are present from v9 onward.
        let endian_byte = if version >= 9 {
            let e = header_reader.read_u8()?;
            header_reader.skip(3)?; // 3 reserved bytes
            e
        } else {
            0u8
        };

        let (metadata_size, file_size, data_offset) = if version >= 22 {
            // Extended 64-bit header starts at offset 16, little-endian.
            let mut ext = BinaryReader::new(data, Endian::Little);
            ext.seek(16)?;
            let ms  = ext.read_u32()?;
            let fs  = ext.read_u64()?;
            let dof = ext.read_u64()?;
            let _unknown = ext.read_i64()?;
            (ms, fs, dof)
        } else {
            (metadata_size_raw, file_size_raw as u64, data_offset_raw as u64)
        };

        if version < 5 {
            bail!("Serialized file format version {} is not supported (min 5)", version);
        }

        let header = SerializedFileHeader {
            metadata_size,
            file_size,
            version,
            data_offset,
            endian: endian_byte,
        };

        // ── 2. Metadata section ───────────────────────────────────────────────
        // Metadata immediately follows the header.
        let meta_start = SerializedFileHeader::byte_size(version);

        // Metadata uses the same endianness flag as the rest of the file.
        let meta_endian = if endian_byte == 0 { Endian::Big } else { Endian::Little };
        let mut meta = BinaryReader::new(data, meta_endian);
        meta.seek(meta_start)?;

        // unity_version string
        let unity_version = if version >= 7 {
            meta.read_cstring()?
        } else {
            String::new()
        };

        // target_platform (v8+)
        let target_platform = if version >= 8 {
            meta.read_u32()?
        } else {
            0
        };

        // has_type_tree flag (v13+)
        let has_type_tree = if version >= 13 {
            meta.read_bool()?
        } else {
            true // older format always embeds type info
        };

        // ── 3. Types ──────────────────────────────────────────────────────────
        let types = TypeTree::read(&mut meta, version, has_type_tree)?;

        // ── 4. Objects ────────────────────────────────────────────────────────
        let objects = ObjectInfo::read_table(&mut meta, version)?;

        // ── 5. Script types (v11+) ────────────────────────────────────────────
        if version >= 11 {
            let script_count = meta.read_u32()?;
            for _ in 0..script_count {
                if version >= 14 { meta.align(4)?; }
                // local_serialized_file_index (i32) + local_identifier (i32/i64)
                meta.skip(4)?;
                if version >= 14 { meta.align(4)?; meta.skip(8)?; }
                else             { meta.skip(4)?; }
            }
        }

        // ── 6. Externals ──────────────────────────────────────────────────────
        let externals = ExternalRef::read_table(&mut meta, version)?;

        // v5+: user information string (skip)
        if version >= 5 {
            let _user_info = meta.read_cstring()?;
        }

        Ok(Self {
            header,
            unity_version,
            target_platform,
            has_type_tree,
            types,
            objects,
            externals,
        })
    }

    // ── Convenience accessors ─────────────────────────────────────────────────

    pub fn endian(&self) -> Endian {
        if self.header.endian == 0 { Endian::Big } else { Endian::Little }
    }

    /// Build an [`ObjectReader`] for the given object using the provided full
    /// file buffer (the same slice passed to [`SerializedFile::read`]).
    pub fn object_reader<'a>(
        &self,
        info:      ObjectInfo,
        file_data: &'a [u8],
    ) -> ObjectReader<'a> {
        ObjectReader::new(info, file_data, self.header.data_offset, self.endian())
    }

    /// Find an object by `path_id` and return its [`ObjectReader`].
    pub fn get_object<'a>(
        &self,
        path_id:   i64,
        file_data: &'a [u8],
    ) -> Option<ObjectReader<'a>> {
        self.objects
            .iter()
            .find(|o| o.path_id == path_id)
            .map(|o| self.object_reader(o.clone(), file_data))
    }

    /// Iterate over all objects, yielding `(ObjectReader, Option<UnityValue>)`.
    ///
    /// Objects without a TypeTree entry yield `None` for the value.
    pub fn iter_objects<'a>(
        &'a self,
        file_data: &'a [u8],
    ) -> impl Iterator<Item = Result<(ObjectReader<'a>, Option<crate::type_tree::UnityValue>)>> + 'a {
        self.objects.iter().map(move |info| {
            let reader = self.object_reader(info.clone(), file_data);
            let value  = reader.read_typetree(&self.types)?;
            Ok((reader, value))
        })
    }
}
