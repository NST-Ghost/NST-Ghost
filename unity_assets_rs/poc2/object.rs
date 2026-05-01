use crate::reader::{BinaryReader, Endian};
use crate::type_tree::{TypeTree, UnityValue, read_typetree};
use anyhow::Result;
use std::collections::HashMap;

// ─────────────────────────────────────────────────────────────────────────────
// ObjectInfo  —  entry from the serialized file's objects table
// ─────────────────────────────────────────────────────────────────────────────

/// Metadata for one object entry inside a serialized file.
#[derive(Debug, Clone)]
pub struct ObjectInfo {
    /// Stable 64-bit identifier for this object within the file.
    pub path_id:   i64,
    /// Byte offset from `data_offset` to the first byte of this object's data.
    pub byte_start: u64,
    /// Byte length of the object's data.
    pub byte_size:  u32,
    /// Index into the types table.
    pub type_id:    i32,
    /// Class identifier (redundant from v16+, but kept for compatibility).
    pub class_id:   i16,
    /// True when the object was stripped during a build (v16+).
    pub is_stripped: bool,
}

impl ObjectInfo {
    /// Read the objects table.  The cursor must be positioned at the count field.
    pub fn read_table(reader: &mut BinaryReader, version: u32) -> Result<Vec<ObjectInfo>> {
        let count = reader.read_u32()?;
        let mut objects = Vec::with_capacity(count as usize);

        for _ in 0..count {
            // v14+: path_id is 8 bytes and must be aligned to 4.
            if version >= 14 {
                reader.align(4)?;
            }

            let path_id = if version >= 14 {
                reader.read_i64()?
            } else {
                reader.read_i32()? as i64
            };

            let byte_start = if version >= 22 {
                reader.read_u64()?
            } else {
                reader.read_u32()? as u64
            };

            let byte_size = reader.read_u32()?;
            let type_id   = reader.read_i32()?;

            // class_id is separate only in v15 and below.
            let class_id = if version < 16 {
                reader.read_i16()?
            } else {
                type_id as i16 // same field in modern versions
            };

            // v11–v15: extra is_destroyed u16
            if version >= 11 && version <= 15 {
                let _is_destroyed = reader.read_u16()?;
            }

            // v16+: is_stripped moved to the TYPES block; do NOT read it here.
            let is_stripped = false;

            objects.push(ObjectInfo {
                path_id,
                byte_start,
                byte_size,
                type_id,
                class_id,
                is_stripped,
            });
        }
        Ok(objects)
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ExternalRef  —  entry from the external files table
// ─────────────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ExternalRef {
    pub guid:     [u8; 16],
    pub r#type:   i32,
    pub path:     String,
    pub temp_empty: String,
}

impl ExternalRef {
    pub fn read_table(reader: &mut BinaryReader, version: u32) -> Result<Vec<ExternalRef>> {
        let count = reader.read_u32()?;
        let mut refs = Vec::with_capacity(count as usize);
        for _ in 0..count {
            let temp_empty = reader.read_cstring()?;
            let mut guid = [0u8; 16];
            for b in &mut guid { *b = reader.read_u8()?; }
            let r#type = reader.read_i32()?;
            let path = reader.read_cstring()?;
            refs.push(ExternalRef { guid, r#type, path, temp_empty });
        }
        Ok(refs)
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ObjectReader  —  lazy accessor that can parse an object's data on demand
// ─────────────────────────────────────────────────────────────────────────────

/// Combines object metadata with a reference to the file's data section so
/// that the raw bytes (or a fully-parsed `UnityValue`) can be retrieved without
/// copying the entire data block upfront.
pub struct ObjectReader<'a> {
    pub info:        ObjectInfo,
    /// Slice of the entire serialized-file buffer starting at byte 0.
    file_data:       &'a [u8],
    /// Absolute byte offset where the data section begins inside `file_data`.
    data_offset:     u64,
    pub endian:      Endian,
}

impl<'a> ObjectReader<'a> {
    pub fn new(
        info:        ObjectInfo,
        file_data:   &'a [u8],
        data_offset: u64,
        endian:      Endian,
    ) -> Self {
        Self { info, file_data, data_offset, endian }
    }

    /// Absolute byte position of this object's data inside the file buffer.
    pub fn abs_offset(&self) -> u64 {
        self.data_offset + self.info.byte_start
    }

    /// Return the raw bytes of this object.
    pub fn raw_data(&self) -> Result<&'a [u8]> {
        let start = self.abs_offset() as usize;
        let end   = start + self.info.byte_size as usize;
        if end > self.file_data.len() {
            anyhow::bail!(
                "ObjectReader: object path_id={} extends beyond file \
                 (start={}, end={}, file_len={})",
                self.info.path_id, start, end, self.file_data.len()
            );
        }
        Ok(&self.file_data[start..end])
    }

    /// Parse the object's data using its TypeTree.
    ///
    /// Returns `None` when no TypeTree is available for this object's `type_id`.
    pub fn read_typetree(
        &self,
        types: &HashMap<i32, TypeTree>,
    ) -> Result<Option<UnityValue>> {
        let type_tree = match types.get(&self.info.type_id) {
            Some(t) => t,
            None    => return Ok(None),
        };
        let data  = self.raw_data()?;
        let value = read_typetree(data, &type_tree.nodes, self.endian)?;
        Ok(Some(value))
    }

    /// Try to read the object's name (`m_Name`) without parsing the full tree.
    ///
    /// Works for any object whose first field is a `string m_Name`.
    pub fn peek_name(&self) -> Option<String> {
        let start = self.abs_offset() as usize;
        if start + 4 > self.file_data.len() { return None; }
        let raw = &self.file_data[start..];
        let len = match self.endian {
            Endian::Little => u32::from_le_bytes(raw[..4].try_into().ok()?) as usize,
            Endian::Big    => u32::from_be_bytes(raw[..4].try_into().ok()?) as usize,
        };
        if 4 + len > raw.len() { return None; }
        Some(String::from_utf8_lossy(&raw[4..4 + len]).into_owned())
    }
}
