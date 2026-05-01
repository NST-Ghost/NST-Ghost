use unity_assets_rs::{SerializedFile, ObjectReader, Endian, rebuild_file, collect_patch_points_v2};
use std::collections::HashMap;

fn align4(len: usize) -> usize {
    (4 - (len % 4)) % 4
}

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets";
    let data = std::fs::read(path).expect("Cannot read source file");
    let sf = SerializedFile::read(&data).expect("Cannot parse assets");
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let target_path_id = 479;
    let old_str = b"What can I do for you?"; // 22 bytes
    let new_str = "มีอะไรให้ฉันช่วยไหม?";      // 54 bytes

    let mut patched_blobs = HashMap::new();

    for info in &sf.objects {
        if info.path_id == target_path_id {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
            let raw = reader.raw_data().unwrap();
            
            if let Some(pos) = raw.windows(old_str.len()).position(|w| w == old_str) {
                let mut new_raw = Vec::new();
                
                // 1. ข้อมูลก่อน String (ไม่รวม length 4 bytes)
                new_raw.extend_from_slice(&raw[..pos-4]);
                
                // 2. ใส่ความยาวใหม่
                let new_len_bytes = new_str.len() as u32;
                new_raw.extend_from_slice(&new_len_bytes.to_le_bytes());
                
                // 3. ใส่ข้อความภาษาไทย
                new_raw.extend_from_slice(new_str.as_bytes());
                
                // 4. ทำ Alignment ให้ใหม่ (สำคัญมาก!)
                let padding = align4(new_str.len());
                for _ in 0..padding { new_raw.push(0); }
                
                // 5. ข้อมูลที่เหลือ (ต้องข้ามข้อมูลเดิม + padding เดิม)
                let old_padding = align4(old_str.len());
                let next_data_pos = pos + old_str.len() + old_padding;
                new_raw.extend_from_slice(&raw[next_data_pos..]);
                
                println!("Patched Object 479 (Surgical): {} -> {} bytes", raw.len(), new_raw.len());
                patched_blobs.insert(target_path_id, new_raw);
            }
        }
    }

    if patched_blobs.is_empty() {
        println!("Target string not found!");
        return;
    }

    let patch_points = collect_patch_points_v2(&data, sf.header.version, sf.header.endian, sf.has_type_tree).unwrap();
    let mut new_file = rebuild_file(
        &data,
        &patch_points,
        &sf.objects,
        sf.header.data_offset,
        sf.header.version,
        obj_endian,
        obj_endian,
        &patched_blobs,
    ).unwrap();

    // แถม: อัปเดต Legacy File Size ใน Header (บางที Unity ตรวจสอบจุดนี้ด้วย)
    let total_size = new_file.len() as u32;
    new_file[4..8].copy_from_slice(&total_size.to_be_bytes());

    std::fs::write("sharedassets1_thai_v2.assets", &new_file).unwrap();
    println!("Successfully wrote sharedassets1_thai_v2.assets");
}
