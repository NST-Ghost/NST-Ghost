// src/creator.rs
use crate::reader::Endian;
use crate::serialized_file::SerializedFile;
use crate::object::ObjectReader;
use crate::type_tree::UnityValue;
use crate::patcher::{rebuild_file, collect_patch_points_v2};
use crate::writer::write_typetree;
use anyhow::{anyhow, Result};
use indexmap::IndexMap;
use std::collections::HashMap;

pub fn create_legacy_font_from_template(
    template: &[u8],      // ไฟล์ template.font ที่คุณสร้างครั้งเดียว
    ttf_data: &[u8],      // ไฟล์ TTF ใหม่ (bytes)
    font_name: &str,      // ชื่อที่อยากให้แสดงใน Unity
) -> Result<Vec<u8>> {
    let sf = SerializedFile::read(template)?;

    if sf.objects.is_empty() {
        return Err(anyhow!("Template has no objects"));
    }

    // หา Font object (class 128 = Legacy Font)
    let font_obj = sf.objects.iter()
        .find(|o| o.class_id == 128 || o.type_id == 128)
        .ok_or_else(|| anyhow!("Template must contain a Legacy Font (class 128)"))?;

    let path_id = font_obj.path_id;
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    let meta_endian = obj_endian; 

    // TypeTree ของ Font
    let type_id = font_obj.type_id;
    let tree = sf.types.get(&type_id)
        .ok_or_else(|| anyhow!("No TypeTree for this Font object"))?;

    if tree.nodes.is_empty() {
        return Err(anyhow!("Template has has_type_tree = false. Please use a template created in Unity 2020+ with Dynamic font."));
    }

    let reader = ObjectReader::new(font_obj.clone(), template, sf.header.data_offset, obj_endian);
    let mut value = reader.read_typetree(&sf.types)?
        .ok_or_else(|| anyhow!("Cannot read Font object value"))?;

    // ── แก้ไขข้อมูล ─────────────────────────────────────
    if let UnityValue::Map(ref mut map) = value {
        // เปลี่ยนชื่อฟอนต์
        map.insert("m_Name".to_string(), UnityValue::String(font_name.to_string()));

        // เปลี่ยน TTF bytes
        if let Some(UnityValue::Map(fontdata)) = map.get_mut("m_FontData") {
            if let Some(UnityValue::Bytes(_)) = fontdata.get("data") {
                let mut new_fd = IndexMap::new();
                new_fd.insert("m_ByteSize".to_string(), UnityValue::UInt(ttf_data.len() as u64));
                new_fd.insert("data".to_string(), UnityValue::Bytes(ttf_data.to_vec()));
                map.insert("m_FontData".to_string(), UnityValue::Map(new_fd));
            } else {
                map.insert("m_FontData".to_string(), UnityValue::Bytes(ttf_data.to_vec()));
            }
        } else {
            let mut new_fd = IndexMap::new();
            new_fd.insert("m_ByteSize".to_string(), UnityValue::UInt(ttf_data.len() as u64));
            new_fd.insert("data".to_string(), UnityValue::Bytes(ttf_data.to_vec()));
            map.insert("m_FontData".to_string(), UnityValue::Map(new_fd));
        }

        map.insert("m_FontSize".to_string(), UnityValue::Int(16));
        map.insert("m_CharacterSpacing".to_string(), UnityValue::Int(0));
        map.insert("m_LineSpacing".to_string(), UnityValue::Int(1));
    } else {
        return Err(anyhow!("Font value is not a Map"));
    }

    // Serialize กลับเป็น bytes
    let new_obj_bytes = write_typetree(&value, &tree.nodes, obj_endian)?;

    // เก็บ patch points จาก template
    let patch_points = collect_patch_points_v2(
        template,
        sf.header.version,
        sf.header.endian,
        sf.has_type_tree,
    )?;

    let mut patches = HashMap::new();
    patches.insert(path_id, new_obj_bytes);

    // Rebuild ไฟล์ใหม่
    let final_file = rebuild_file(
        template,
        &patch_points,
        &sf.objects,
        sf.header.data_offset,
        sf.header.version,
        meta_endian,
        obj_endian,
        &patches,
    )?;

    Ok(final_file)
}

pub fn dump_font_structure(template: &[u8]) -> Result<()> {
    let sf = SerializedFile::read(template)?;
    let font_obj = sf.objects.iter().find(|o| o.class_id == 128)
        .ok_or_else(|| anyhow!("No Font object found"))?;
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    let reader = ObjectReader::new(font_obj.clone(), template, sf.header.data_offset, endian);
    let value = reader.read_typetree(&sf.types)?.unwrap();
    println!("{:#?}", value);
    Ok(())
}
