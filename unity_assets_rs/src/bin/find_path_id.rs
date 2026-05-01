use unity_assets_rs::{SerializedFile, ObjectReader, Endian};

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    for info in &sf.objects {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        if let Ok(raw) = reader.raw_data() {
            if raw.windows(10).any(|w| w == b"What can I") {
                println!("FOUND! PathID: {}, ClassID: {}", info.path_id, info.class_id);
            }
        }
    }
}
