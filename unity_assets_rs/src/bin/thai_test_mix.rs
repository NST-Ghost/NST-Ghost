fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let mut data = std::fs::read(path).expect("Cannot read original file");
    
    let old_str = b"What can I do for you?"; // 22 bytes
    let test_thai = "T:ช่วย...??   "; // ภาษาไทยผสมอังกฤษ รวม 22 bytes พอดี
    
    if let Some(pos) = data.windows(old_str.len()).position(|w| w == old_str) {
        println!("Patching with: {}", test_thai);
        data[pos..pos+22].copy_from_slice(test_thai.as_bytes());
        std::fs::write("demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets", &data).unwrap();
    }
}
