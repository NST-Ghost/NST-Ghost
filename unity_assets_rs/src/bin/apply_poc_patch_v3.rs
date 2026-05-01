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
    let old_str = b"What can I do for you?";
    let new_str = "มีอะไรให้ฉันช่วยไหม?"; 

    let mut patched_blobs = HashMap::new();

    for info in &sf.objects {
        if info.path_id == target_path_id {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
            let raw = reader.raw_data().unwrap();
            
            if let Some(pos) = raw.windows(old_str.len()).position(|w| w == old_str) {
                let mut new_raw = Vec::new();
                
                // คัดลอกทุกอย่างจนถึงจุดเริ่มความยาวของ string เดิม (4 bytes ก่อน string)
                new_raw.extend_from_slice(&raw[..pos-4]);
                
                // ใส่ความยาวใหม่
                let new_len_bytes = new_str.len() as u32;
                new_raw.extend_from_slice(&new_len_bytes.to_le_bytes());
                
                // ใส่ข้อความไทย
                new_raw.extend_from_slice(new_str.as_bytes());
                
                // ทำ Alignment (4-byte)
                let padding = align4(new_str.len());
                for _ in 0..padding { new_raw.push(0); }
                
                // ข้ามข้อมูลเดิม + padding เดิม
                let old_padding = align4(old_str.len());
                let next_data_pos = pos + old_str.len() + old_padding;
                
                // ใส่ข้อมูลที่เหลือทั้งหมดต่อท้าย
                new_raw.extend_from_slice(&raw[next_data_pos..]);
                
                println!("Patched Object 479 V3: Original size {}, New size {}", raw.len(), new_raw.len());
                patched_blobs.insert(target_path_id, new_raw);
            }
        }
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

    // บังคับอัปเดตไฟล์ไซส์ทั้งแบบเก่าและแบบใหม่ใน Header
    let total_size = new_file.len() as u64;
    // Legacy Header (offset 4)
    new_file[4..8].copy_from_slice(&(total_size as u32).to_be_bytes());
    // Modern Header (offset 24)
    if sf.header.version >= 22 {
        new_file[24..32].copy_from_slice(&total_size.to_be_bytes());
    }

    std::fs::write("sharedassets1_thai_v3.assets", &new_file).unwrap();
    println!("Successfully wrote sharedassets1_thai_v3.assets");
}
