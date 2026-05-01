use unity_assets_rs::{SerializedFile, ObjectReader, Endian};

fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets0.assets";
    let data = std::fs::read(path).unwrap();
    let sf = SerializedFile::read(&data).unwrap();
    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    // สแกนหา Object ที่มีชื่อว่า "LiberationSans SDF Atlas"
    for info in &sf.objects {
        let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
        if let Some(name) = reader.peek_name() {
            if name.contains("LiberationSans SDF") {
                println!("FOUND! PathID: {}, ClassID: {}, Name: {}", info.path_id, info.class_id, name);
                let raw = reader.raw_data().unwrap();
                let out_name = format!("obj_{}_{}.dat", info.path_id, name.replace(" ", "_"));
                std::fs::write(&out_name, raw).unwrap();
                println!("Saved to {}", out_name);
            }
        }
    }
}
