use unity_assets_rs::{SerializedFile, Endian, rebuild_file, collect_patch_points_v2};
use std::collections::HashMap;

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let data = std::fs::read(path).expect("Cannot read original file");
    let sf = SerializedFile::read(&data).expect("Cannot parse assets");
    let obj_endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let patched_blobs: HashMap<i64, Vec<u8>> = HashMap::new();

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

    // --- FIX HEADER: คัดลอก 48 byte แรกจากต้นฉบับมาเลยเพื่อความชัวร์ ---
    let original_header = &data[..48];
    new_file[..48].copy_from_slice(original_header);

    // อัปเดตเฉพาะ File Size ในตำแหน่งที่ถูกต้อง (เวอร์ชัน 22 อยู่ที่ offset 24 เป็น u64)
    let total_size = new_file.len() as u64;
    let size_bytes = total_size.to_be_bytes();
    new_file[24..32].copy_from_slice(&size_bytes);

    std::fs::write("sharedassets1_zero_change_v2.assets", &new_file).unwrap();
    println!("Successfully wrote sharedassets1_zero_change_v2.assets");
}
