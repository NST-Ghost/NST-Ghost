use unity_assets_rs::{SerializedFile, ObjectReader, Endian};

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    let target_path_id = 479;
    for info in &sf.objects {
        if info.path_id == target_path_id {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
            let raw = reader.raw_data().unwrap();
            let search = b"What can I do for you?";
            
            if let Some(pos) = raw.windows(search.len()).position(|w| w == search) {
                println!("--- String found at offset {} ---", pos);
                // ขอดูข้อมูลก่อนหน้า 64 byte และหลัง 64 byte
                let start = pos.saturating_sub(64);
                let end = (pos + search.len() + 64).min(raw.len());
                
                println!("Context Hex View:");
                for (i, chunk) in raw[start..end].chunks(16).enumerate() {
                    let current_offset = start + (i * 16);
                    print!("{:04X} | ", current_offset);
                    for b in chunk { print!("{:02X} ", b); }
                    print!(" | ");
                    for b in chunk {
                        if b.is_ascii_graphic() || *b == b' ' { print!("{}", *b as char); } else { print!("."); }
                    }
                    println!();
                }
            }
        }
    }
}
