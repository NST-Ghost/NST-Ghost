use unity_assets_rs::{SerializedFile, UnityValue};
use std::fs;
use std::path::Path;

#[test]
fn test_poc_extraction() {
    let path = Path::new("demo/KZO v0.2/ZZZTestURP_Data/resources.assets");
    if !path.exists() {
        println!("Demo file not found at {:?}", path);
        return;
    }

    let data = fs::read(path).expect("Failed to read asset file");
    let asset = SerializedFile::read(&data).expect("Failed to parse SerializedFile");

    println!("Unity Version: {}", asset.unity_version);
    println!("Total Objects Found: {}", asset.objects.len());

    let mut found_count = 0;
    for (reader, value) in asset.iter_objects(&data).flatten() {
        found_count += 1;
        println!("Object #{} | PathID: {}, ClassID: {}, TypeID: {}", 
            found_count, reader.info.path_id, reader.info.class_id, reader.info.type_id);
        
        if let Some(val) = value {
            // Check if it's a map and print its keys
            if let unity_assets_rs::UnityValue::Map(map) = val {
                println!("  Keys: {:?}", map.keys().collect::<Vec<_>>());
            }
        }

        if found_count >= 20 { break; }
    }

    assert!(found_count > 0);
}
