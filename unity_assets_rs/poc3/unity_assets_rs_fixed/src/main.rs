use unity_assets_rs::SerializedFile;
use std::collections::HashMap;

fn main() {
    let path = std::env::args().nth(1).unwrap_or_else(|| "resources.assets".to_string());
    println!("Reading: {}", path);

    let data = std::fs::read(&path).expect("Cannot read file");
    let sf = SerializedFile::read(&data).expect("Parse failed");

    println!("\n=== SUMMARY ===");
    println!("Unity version  : {}", sf.unity_version);
    println!("Platform       : {}", sf.target_platform);
    println!("has_type_tree  : {}", sf.has_type_tree);
    println!("data_offset    : 0x{:x}", sf.header.data_offset);
    println!("Total objects  : {}", sf.objects.len());

    // class_id distribution
    let mut by_type: HashMap<i32, usize> = HashMap::new();
    for obj in &sf.objects {
        *by_type.entry(obj.type_id).or_insert(0) += 1;
    }
    let mut dist: Vec<_> = by_type.iter().collect();
    dist.sort_by_key(|(_, c)| std::cmp::Reverse(**c));
    println!("\nTop type_ids:");
    for (tid, cnt) in dist.iter().take(10) {
        println!("  type_id {:3} : {} objects", tid, cnt);
    }

    // First 20 object names via peek_name
    use unity_assets_rs::reader::Endian;
    use unity_assets_rs::object::ObjectReader;
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };
    println!("\nFirst 20 object names:");
    for info in sf.objects.iter().take(20) {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        let name = reader.peek_name().unwrap_or_else(|| "<no name>".to_string());
        println!("  path_id={:5}  type={:3}  size={:8}  name={}", info.path_id, info.type_id, info.byte_size, name);
    }
}
