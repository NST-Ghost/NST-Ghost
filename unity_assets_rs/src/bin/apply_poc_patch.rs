use unity_assets_rs::{SerializedFile, ObjectReader, Endian, rebuild_file, collect_patch_points_v2};
use std::collections::HashMap;

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let target_path_id = 479;
    let old_str = b"What can I do for you?";
    let new_str = "มีอะไรให้ฉันช่วยไหม?";

    let mut patched_blobs = HashMap::new();

    for info in &sf.objects {
        if info.path_id == target_path_id {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
            let raw = reader.raw_data().unwrap();
            
            // หาตำแหน่ง string เดิม
            if let Some(pos) = raw.windows(old_str.len()).position(|w| w == old_str) {
                let mut new_raw = Vec::new();
                // 1. ใส่ข้อมูลก่อนหน้า String (รวมถึง length prefix)
                new_raw.extend_from_slice(&raw[..pos-4]);
                
                // 2. ใส่ length prefix ใหม่ (4 bytes)
                let new_len = new_str.len() as u32;
                new_raw.extend_from_slice(&new_len.to_le_bytes());
                
                // 3. ใส่ string ภาษาไทย
                new_raw.extend_from_slice(new_str.as_bytes());
                
                // 4. ใส่ข้อมูลที่เหลือหลัง string เดิม
                new_raw.extend_from_slice(&raw[pos + old_str.len()..]);
                
                println!("Patched Object 479: {} bytes -> {} bytes", raw.len(), new_raw.len());
                patched_blobs.insert(target_path_id, new_raw);
            }
        }
    }

    // ใช้ระบบ rebuild_file ของเราสร้างไฟล์ใหม่
    let patch_points = collect_patch_points_v2(&data, sf.header.version, sf.header.endian, sf.has_type_tree).unwrap();
    let new_file = rebuild_file(
        &data,
        &patch_points,
        &sf.objects,
        sf.header.data_offset,
        sf.header.version,
        obj_endian, // meta endian
        obj_endian, // obj endian
        &patched_blobs,
    ).unwrap();

    std::fs::write("sharedassets1_thai.assets", &new_file).unwrap();
    println!("Successfully wrote sharedassets1_thai.assets");
}
