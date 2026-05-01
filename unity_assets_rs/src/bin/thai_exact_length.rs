fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let mut data = std::fs::read(path).expect("Cannot read original file");
    
    let old_str = b"What can I do for you?"; // 22 bytes
    let new_thai = "ช่วยไหม?"; // 22 bytes ใน UTF-8 (ช-ว-ย-่-ไ-ห-ม-?)
    
    if let Some(pos) = data.windows(old_str.len()).position(|w| w == old_str) {
        println!("FOUND target at offset {}. Overwriting with Thai (22 bytes)...", pos);
        data[pos..pos+22].copy_from_slice(new_thai.as_bytes());
        
        std::fs::write("demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets", &data).unwrap();
        println!("Successfully applied fixed-length Thai patch.");
    } else {
        println!("Target string not found!");
    }
}
