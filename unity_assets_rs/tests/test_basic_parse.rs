use unity_assets_rs::SerializedFile;
use std::fs;
use std::path::Path;

#[test]
fn test_parse_global_managers() {
    let path = Path::new("demo/KZO v0.2/ZZZTestURP_Data/globalgamemanagers.assets");
    if !path.exists() {
        // Skip if file doesn't exist to avoid CI failure, but print for manual run
        println!("Demo file not found at {:?}", path);
        return;
    }

    let data = fs::read(path).expect("Failed to read asset file");
    let asset = SerializedFile::read(&data).expect("Failed to parse SerializedFile");

    println!("Unity Version: {}", asset.unity_version);
    println!("Target Platform: {}", asset.target_platform);
    println!("Total Objects Found: {}", asset.objects.len());
    println!("Total Types in TypeTree: {}", asset.types.len());

    // Print the first few objects as examples
    for object in asset.objects.iter().take(5) {
        println!("Object PathID: {}, TypeID: {}, Size: {} bytes", 
            object.path_id, object.type_id, object.byte_size);
    }

    assert!(asset.objects.len() > 0);
}
