use unity_assets_rs::{SerializedFile, ObjectReader, Endian, rebuild_file, collect_patch_points_v2};
use std::collections::HashMap;

fn align4(len: usize) -> usize {
    (4 - (len % 4)) % 4
}

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let target_path_id = 479;
    let old_str = b"What can I do for you?";
    let new_str = "มีอะไรให้ช่วยไหม?"; // ภาษาไทย

    let mut patched_blobs = HashMap::new();

    for info in &sf.objects {
        if info.path_id == target_path_id {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, obj_endian);
            let raw = reader.raw_data().unwrap();
            
            if let Some(pos) = raw.windows(old_str.len()).position(|w| w == old_str) {
                let mut new_raw = Vec::new();
                new_raw.extend_from_slice(&raw[..pos-4]);
                
                let new_len_bytes = new_str.len() as u32;
                new_raw.extend_from_slice(&new_len_bytes.to_le_bytes());
                new_raw.extend_from_slice(new_str.as_bytes());
                
                let padding = align4(new_str.len());
                for _ in 0..padding { new_raw.push(0); }
                
                let old_padding = align4(old_str.len());
                let next_data_pos = pos + old_str.len() + old_padding;
                new_raw.extend_from_slice(&raw[next_data_pos..]);
                
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

    // บังคับเขียน Header ใหม่ให้ถูกต้อง
    let original_header = &data[..48];
    new_file[..48].copy_from_slice(original_header);
    let total_size = new_file.len() as u64;
    new_file[24..32].copy_from_slice(&total_size.to_be_bytes());

    std::fs::write("demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets", &new_file).unwrap();
    println!("Applied Thai Patch (V3-Fixed) to sharedassets1.assets");
}
