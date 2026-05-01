use unity_assets_rs::{SerializedFile, ObjectReader, Endian};

fn main() {
    let path = "demo/game 2/AboveImmortalRealms_Data/resources.assets";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    for info in &sf.objects {
        if info.class_id == 114 {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
            // TMP_FontAsset มักจะมีชื่อฟอนต์อยู่ที่ offset 28-32 (หลังพ้น Header MonoBehaviour)
            let raw = reader.raw_data().unwrap();
            if raw.len() > 100 {
                let text = String::from_utf8_lossy(raw);
                if text.contains("SDF") || text.contains("Liberation") {
                    println!("POTENTIAL FONT ASSET! PathID: {}, Size: {}", info.path_id, raw.len());
                    // ดู 100 byte แรกแบบ hex
                    println!("{:02X?}", &raw[..100]);
                }
            }
        }
    }
}
