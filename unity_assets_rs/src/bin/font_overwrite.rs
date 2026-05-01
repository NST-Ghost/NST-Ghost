use unity_assets_rs::{SerializedFile, ObjectReader, Endian, rebuild_file, collect_patch_points_v2};
use std::collections::HashMap;
use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 5 {
        println!("Usage: font_overwrite <src_assets> <src_path_id> <target_assets> <target_path_id>");
        return;
    }

    let src_path = &args[1];
    let src_id: i64 = args[2].parse().expect("Invalid src_path_id");
    let target_path = &args[3];
    let target_id: i64 = args[4].parse().expect("Invalid target_path_id");

    // 1. ดึงก้อนข้อมูลฟอนต์ไทยจากไฟล์ต้นฉบับ
    println!("Extracting font from {} (PathID: {})...", src_path, src_id);
    let src_data = std::fs::read(src_path).expect("Cannot read src_assets");
    let src_sf = SerializedFile::read(&src_data).expect("Cannot parse src_assets");
    let src_endian = if src_sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    
    let mut font_blob = Vec::new();
    for info in &src_sf.objects {
        if info.path_id == src_id {
            let reader = ObjectReader::new(info.clone(), &src_data, src_sf.header.data_offset, src_endian);
            font_blob = reader.raw_data().expect("Cannot read font raw data").to_vec();
            break;
        }
    }
    if font_blob.is_empty() { panic!("Source font PathID not found!"); }

    // 2. ยัดเข้าไฟล์เป้าหมาย (สวมรอย)
    println!("Overwriting font in {} (PathID: {})...", target_path, target_id);
    let target_data = std::fs::read(target_path).expect("Cannot read target_assets");
    let target_sf = SerializedFile::read(&target_data).expect("Cannot parse target_assets");
    let target_endian = if target_sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let mut patched_blobs = HashMap::new();
    patched_blobs.insert(target_id, font_blob);

    let patch_points = collect_patch_points_v2(&target_data, target_sf.header.version, target_sf.header.endian, target_sf.has_type_tree).unwrap();
    let mut new_file = rebuild_file(
        &target_data,
        &patch_points,
        &target_sf.objects,
        target_sf.header.data_offset,
        target_sf.header.version,
        target_endian,
        target_endian,
        &patched_blobs,
    ).unwrap();

    // รักษาความสมบูรณ์ของ Header
    let original_header = &target_data[..48];
    new_file[..48].copy_from_slice(original_header);
    let total_size = new_file.len() as u64;
    new_file[24..32].copy_from_slice(&total_size.to_be_bytes());

    let out_name = format!("{}_mod.assets", target_path.split('.').next().unwrap());
    std::fs::write(&out_name, &new_file).unwrap();
    println!("SUCCESS! Modded file saved as: {}", out_name);
}
