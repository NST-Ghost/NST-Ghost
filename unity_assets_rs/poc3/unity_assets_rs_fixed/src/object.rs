use crate::reader::{BinaryReader, Endian};
use crate::type_tree::{TypeTree, UnityValue, read_typetree};
use anyhow::Result;
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct ObjectInfo {
    pub path_id:     i64,
    pub byte_start:  u64,
    pub byte_size:   u32,
    pub type_id:     i32,
    pub class_id:    i16,
    pub is_stripped: bool,
}

impl ObjectInfo {
    pub fn read_table(reader: &mut BinaryReader, version: u32) -> Result<Vec<ObjectInfo>> {
        // FIX: removed the pre-count align(4).
        // In Unity's own writer, after the types block the cursor is NOT necessarily
        // 4-byte aligned (especially when has_type_tree=false).  The alignment
        // in the original code caused the reader to skip 1–3 bytes and land on
        // a padding zero, reading an object count of 0.
        //
        // The per-entry align(4) inside the loop is still correct and required.
        let count = reader.read_u32()?;
        let mut objects = Vec::with_capacity(count as usize);

        for i in 0..count {
            // Per-object alignment (v14+).
            if version >= 14 { reader.align(4)?; }

            let path_id = if version >= 14 { reader.read_i64()? } else { reader.read_i32()? as i64 };

            let byte_start = if version >= 22 {
                reader.read_u64()?
            } else {
                reader.read_u32()? as u64
            };

            let byte_size = reader.read_u32()?;
            let type_id   = reader.read_i32()?;

            let class_id = if version < 16 { reader.read_i16()? } else { type_id as i16 };

            if version >= 11 && version <= 15 {
                let _is_destroyed = reader.read_u16()?;
            }

            let is_stripped = false;

            if i < 3 || i % 1000 == 0 {
                println!("  obj[{}] path_id={} byte_start={} byte_size={} type_id={}",
                    i, path_id, byte_start, byte_size, type_id);
            }

            objects.push(ObjectInfo { path_id, byte_start, byte_size, type_id, class_id, is_stripped });
        }
        Ok(objects)
    }
}

#[derive(Debug, Clone)]
pub struct ExternalRef {
    pub guid:       [u8; 16],
    pub r#type:     i32,
    pub path:       String,
    pub temp_empty: String,
}

impl ExternalRef {
    pub fn read_table(reader: &mut BinaryReader, _version: u32) -> Result<Vec<ExternalRef>> {
        let count = reader.read_u32()?;
        let mut refs = Vec::with_capacity(count as usize);
        for _ in 0..count {
            let temp_empty = reader.read_cstring()?;
            let mut guid = [0u8; 16];
            for b in &mut guid { *b = reader.read_u8()?; }
            let r#type = reader.read_i32()?;
            let path   = reader.read_cstring()?;
            refs.push(ExternalRef { guid, r#type, path, temp_empty });
        }
        Ok(refs)
    }
}

pub struct ObjectReader<'a> {
    pub info:        ObjectInfo,
    file_data:       &'a [u8],
    data_offset:     u64,
    pub endian:      Endian,
}

impl<'a> ObjectReader<'a> {
    pub fn new(info: ObjectInfo, file_data: &'a [u8], data_offset: u64, endian: Endian) -> Self {
        Self { info, file_data, data_offset, endian }
    }

    pub fn abs_offset(&self) -> u64 { self.data_offset + self.info.byte_start }

    pub fn raw_data(&self) -> Result<&'a [u8]> {
        let start = self.abs_offset() as usize;
        let end   = start + self.info.byte_size as usize;
        if end > self.file_data.len() {
            anyhow::bail!("object path_id={} OOB (start={} end={} file={})",
                self.info.path_id, start, end, self.file_data.len());
        }
        Ok(&self.file_data[start..end])
    }

    pub fn read_typetree(&self, types: &HashMap<i32, TypeTree>) -> Result<Option<UnityValue>> {
        let tree = match types.get(&self.info.type_id) {
            Some(t) => t,
            None    => return Ok(None),
        };
        if tree.nodes.is_empty() { return Ok(None); }
        let data  = self.raw_data()?;
        let value = read_typetree(data, &tree.nodes, self.endian)?;
        Ok(Some(value))
    }

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
