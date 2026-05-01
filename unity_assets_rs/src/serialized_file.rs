use crate::reader::{BinaryReader, Endian};
use crate::type_tree::TypeTree;
use crate::object::{ExternalRef, ObjectInfo, ObjectReader};
use anyhow::Result;
use std::collections::HashMap;

pub struct SerializedFileHeader {
    pub metadata_size: u32,
    pub file_size:     u64,
    pub version:       u32,
    pub data_offset:   u64,
    pub endian:        u8,
}

pub struct SerializedFile {
    pub header:          SerializedFileHeader,
    pub unity_version:   String,
    pub target_platform: u32,
    pub has_type_tree:   bool,
    pub types:           HashMap<i32, TypeTree>,
    pub objects:         Vec<ObjectInfo>,
    pub externals:       Vec<ExternalRef>,
}

impl SerializedFile {
    pub fn read(data: &[u8]) -> Result<Self> {
        let mut hr = BinaryReader::new(data, Endian::Big);
        let ms_legacy   = hr.read_u32()?;
        let fs_legacy   = hr.read_u32()?;
        let version     = hr.read_u32()?;
        let do_legacy   = hr.read_u32()?;
        let endian_byte = hr.read_u8()?;
        hr.skip(3)?;

        let (metadata_size, file_size, data_offset) = if version >= 22 {
            let ms  = hr.read_u32()?;
            let fs  = hr.read_u64()?;
            let dof = hr.read_u64()?;
            let _   = hr.read_u64()?;
            (ms, fs, dof)
        } else {
            (ms_legacy, fs_legacy as u64, do_legacy as u64)
        };

        let header = SerializedFileHeader { metadata_size, file_size, version, data_offset, endian: endian_byte };

        let meta_start: u64 = if version >= 22 { 48 } else { 16 };
        // FIX: endian_byte == 0 -> Little
        let meta_endian = if endian_byte == 0 { Endian::Little } else { Endian::Big };
        let mut meta = BinaryReader::new(data, meta_endian);
        meta.seek(meta_start)?;

        let unity_version   = meta.read_fixed_string(12)?;
        let target_platform = meta.read_u32()?;
        let has_type_tree   = meta.read_u8()? != 0;

        let types = TypeTree::read(&mut meta, version, has_type_tree)?;
        let objects = ObjectInfo::read_table(&mut meta, version)?;
        let externals = ExternalRef::read_table(&mut meta, version).unwrap_or_default();

        Ok(Self { header, unity_version, target_platform, has_type_tree, types, objects, externals })
    }

    pub fn iter_objects<'a>(
        &'a self,
        file_data: &'a [u8],
    ) -> impl Iterator<Item = Result<(ObjectReader<'a>, Option<crate::type_tree::UnityValue>)>> + 'a {
        let obj_endian = if self.header.endian == 0 { Endian::Little } else { Endian::Big };
        self.objects.iter().map(move |info| {
            let reader = ObjectReader::new(info.clone(), file_data, self.header.data_offset, obj_endian);
            let value  = reader.read_typetree(&self.types)?;
            Ok((reader, value))
        })
    }
}
