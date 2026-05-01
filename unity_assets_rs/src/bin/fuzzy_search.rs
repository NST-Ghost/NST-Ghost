use unity_assets_rs::{SerializedFile, ObjectReader, Endian};
use std::env;

fn main() {
    let path = env::args().nth(1).expect("Need path");
    let data = std::fs::read(&path).expect("Read error");
    let sf = SerializedFile::read(&data).expect("Parse error");
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let search_str = env::args().nth(2).expect("Need search string");
    println!("Scanning {} for exact string: '{}'", path, search_str);

    for info in &sf.objects {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        if let Ok(raw) = reader.raw_data() {
            let raw_str = String::from_utf8_lossy(raw);
            if raw_str.contains(&search_str) {
                println!("--- MATCH FOUND ---");
                println!("PathID: {}, ClassID: {}", info.path_id, info.class_id);
                // Also print some context
                if let Some(pos) = raw_str.find(&search_str) {
                    let start = pos.saturating_sub(20);
                    let end = (pos + search_str.len() + 20).min(raw_str.len());
                    println!("Context: {}", &raw_str[start..end]);
                }
                println!("-------------------\n");
            }
        }
    }
}
