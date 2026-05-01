use unity_assets_rs::Endian;
use std::fs;

fn main() -> anyhow::Result<()> {
    // 1. สร้างก้อน Binary Header เล็กๆ (จำลอง SerializedFile)
    let mut data = Vec::new();
    // Unity Header (Big Endian)
    data.extend_from_slice(&0u32.to_be_bytes()); // metadata_size (placeholder)
    data.extend_from_slice(&0u32.to_be_bytes()); // file_size (placeholder)
    data.extend_from_slice(&22u32.to_be_bytes()); // version
    data.extend_from_slice(&0u32.to_be_bytes()); // data_offset (placeholder)
    data.push(0); // endian (0 = Little)
    data.extend_from_slice(&[0, 0, 0]); // padding
    
    // Metadata ส่วนที่ 2 (Little Endian เพราะ endian_byte = 0)
    let meta_start = data.len();
    data.extend_from_slice(b"2022.3.0f1\0\0"); // unity_version (12 bytes)
    data.extend_from_slice(&19u32.to_le_bytes()); // target_platform
    data.push(1); // has_type_tree = true
    
    // --- TypeTree Simulation (Class 128 - Font) ---
    data.extend_from_slice(&1u32.to_le_bytes()); // type count = 1
    data.extend_from_slice(&128i32.to_le_bytes()); // class_id = 128
    data.push(0); // is_stripped
    data.extend_from_slice(&(-1i16).to_le_bytes()); // script_index
    data.extend_from_slice(&[0u8; 16]); // type_hash
    
    // TypeTree Nodes (ย่อส่วน เฉพาะ m_Name และ m_FontData)
    // สำหรับการทดสอบนี้ เราจะใช้ TypeTree แบบที่ตัวอ่านของเราเข้าใจได้
    data.extend_from_slice(&2u32.to_le_bytes()); // node count = 2
    data.extend_from_slice(&20u32.to_le_bytes()); // string buffer size
    // Node 0: Base
    data.extend_from_slice(&0u16.to_le_bytes()); // version
    data.push(0); // level
    data.push(0); // is_array
    data.extend_from_slice(&0u32.to_le_bytes()); // type_idx
    data.extend_from_slice(&5u32.to_le_bytes()); // name_idx
    data.extend_from_slice(&(-1i32).to_le_bytes()); // byte_size
    data.extend_from_slice(&0u32.to_le_bytes()); // index
    data.extend_from_slice(&0u32.to_le_bytes()); // meta_flag
    data.extend_from_slice(&0u64.to_le_bytes()); // ref_hash
    // Node 1: m_Name
    data.extend_from_slice(&0u16.to_le_bytes()); // version
    data.push(1); // level
    data.push(0); // is_array
    data.extend_from_slice(&10u32.to_le_bytes()); // type_idx (string)
    data.extend_from_slice(&15u32.to_le_bytes()); // name_idx (m_Name)
    data.extend_from_slice(&(-1i32).to_le_bytes()); // byte_size
    data.extend_from_slice(&1u32.to_le_bytes()); // index
    data.extend_from_slice(&0u32.to_le_bytes()); // meta_flag
    data.extend_from_slice(&0u64.to_le_bytes()); // ref_hash
    
    data.extend_from_slice(b"Font\0string\0m_Name\0"); // string buffer
    
    // --- Object Table Simulation ---
    data.extend_from_slice(&1u32.to_le_bytes()); // object count = 1
    data.extend_from_slice(&[0u8; 3]); // align(4)
    data.extend_from_slice(&1i64.to_le_bytes()); // path_id = 1
    data.extend_from_slice(&0u64.to_le_bytes()); // byte_start
    data.extend_from_slice(&12u32.to_le_bytes()); // byte_size
    data.extend_from_slice(&128i32.to_le_bytes()); // type_id
    
    // Data section start offset
    let data_offset = data.len() as u64;
    
    // Object Data: m_Name = "Test"
    data.extend_from_slice(&4u32.to_le_bytes()); // string len
    data.extend_from_slice(b"Test");
    data.extend_from_slice(&[0u8; 0]); // padding
    
    // อัปเดต Header
    let file_size = data.len() as u64;
    data[24..32].copy_from_slice(&file_size.to_be_bytes());
    data[16..20].copy_from_slice(&(data_offset as u32).to_be_bytes()); // metadata_size (approx)
    data[32..40].copy_from_slice(&data_offset.to_be_bytes()); // data_offset
    
    fs::write("mock_template.font", &data)?;
    println!("Generated mock_template.font (Size: {} bytes)", file_size);
    Ok(())
}
