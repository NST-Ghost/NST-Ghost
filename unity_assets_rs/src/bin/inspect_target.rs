use unity_assets_rs::{SerializedFile, ObjectReader, Endian};
use std::env;

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
            println!("Raw Hex (first 100 bytes):");
            for (i, chunk) in raw.chunks(16).take(10).enumerate() {
                print!("{:04X}: ", i * 16);
                for b in chunk { print!("{:02X} ", b); }
                println!();
            }
            // ลองหาตำแหน่งของ String "What can I do for you?"
            let search = b"What can I do for you?";
            if let Some(pos) = raw.windows(search.len()).position(|w| w == search) {
                println!("\nFound string at offset: {}", pos);
                // ดู 4 byte ก่อนหน้า (Length Prefix)
                if pos >= 4 {
                    let len_bytes = &raw[pos-4..pos];
                    let len = u32::from_le_bytes(len_bytes.try_into().unwrap());
                    println!("Length prefix found: {} (hex: {:02X} {:02X} {:02X} {:02X})", 
                        len, len_bytes[0], len_bytes[1], len_bytes[2], len_bytes[3]);
                }
            }
        }
    }
}
