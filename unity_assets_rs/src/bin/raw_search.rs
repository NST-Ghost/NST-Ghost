use unity_assets_rs::{SerializedFile, ObjectReader, Endian};
use std::env;

fn main() {
    let path = env::args().nth(1).expect("Need path");
    let data = match std::fs::read(&path) {
        Ok(d) => d,
        Err(_) => return, // ignore read errors
    };
    let sf = match SerializedFile::read(&data) {
        Ok(s) => s,
        Err(_) => return, // ignore parse errors
    };
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let search = b"Restraint";

    for info in &sf.objects {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        if let Ok(raw) = reader.raw_data() {
            let mut i = 0;
            while i + search.len() <= raw.len() {
                if &raw[i..i+search.len()] == search {
                    let start = i.saturating_sub(20);
                    let end = (i + search.len() + 20).min(raw.len());
                    
                    let context: String = raw[start..end].iter()
                        .map(|&c| if c.is_ascii_graphic() || c == b' ' { c as char } else { '.' })
                        .collect();
                        
                    println!("{}: found in path_id={}, class_id={} -> {}", path, info.path_id, info.class_id, context);
                }
                i += 1;
            }
        }
    }
}
