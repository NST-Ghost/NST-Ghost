use unity_assets_rs::{SerializedFile, ObjectReader, Endian, UnityValue};
use std::fs;

fn main() -> anyhow::Result<()> {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets0.assets";
    let data = fs::read(path)?;
    let sf = SerializedFile::read(&data)?;

    let endian = if sf.header.endian == 0 { Endian::Little } else { Endian::Big };

    for info in &sf.objects {
        // TMP_FontAsset มักจะเป็น MonoBehaviour (114)
        if info.class_id == 114 || info.type_id == 114 {
            let reader = ObjectReader::new(info.clone(), &data, sf.header.data_offset, endian);
            if let Ok(Some(value)) = reader.read_typetree(&sf.types) {
                if let UnityValue::Map(map) = &value {
                    if let Some(UnityValue::String(name)) = map.get("m_Name") {
                        if name.contains("LiberationSans SDF") {
                            println!("Found TMP_FontAsset: {} (PathID={})", name, info.path_id);
                            
                            // ดัมพ์โครงสร้างออกมาให้ดู
                            let json = serde_json::to_string_pretty(&value).unwrap();
                            fs::write("tmp_font_dump.json", &json)?;
                            println!("✅ Dumped to tmp_font_dump.json");
                            
                            // โชว์เฉพาะคีย์ระดับบนสุด เพื่อดูว่ามันมีอะไรบ้าง
                            println!("\nTop-level keys in this FontAsset:");
                            for key in map.keys() {
                                println!(" - {}", key);
                            }
                            return Ok(());
                        }
                    }
                }
            }
        }
    }
    println!("Not found.");
    Ok(())
}
