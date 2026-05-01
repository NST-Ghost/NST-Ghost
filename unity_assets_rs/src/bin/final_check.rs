fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let mut data = std::fs::read(path).unwrap();
    
    // "มีไรไห้ช่วยมะ?" -> UTF-8 คือ:
    // E0 B8 AD E0 B8 B0 E0 B9 84 E0 B8 A3 E0 B9 83 E0 B8 AB E0 B9 89 E0 B8 8A E0 B9 88 E0 B8 A7 E0 B8 A2 E0 B9 84 E0 B8 AB E0 B8 A1 E0 B9 88
    // โอ้... ภาษาไทยสั้นๆ ก็เกิน 22 bytes แล้วครับ
    
    // งั้นลองแก้เป็นอังกฤษประโยคสั้นๆ แทนเพื่อเช็คว่า "จุดนี้ใช่จุดที่เกมใช้โชว์ไหม"
    let old_str = b"What can I do for you?";
    let test_str = b"I AM MOOOODDED!!!!!!  "; // 22 bytes
    
    if let Some(pos) = data.windows(old_str.len()).position(|w| w == old_str) {
        data[pos..pos+22].copy_from_slice(test_str);
        std::fs::write("demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets", &data).unwrap();
        println!("SUCCESS: Overwrote at file offset {}", pos);
    } else {
        println!("FAILED: Target string not found in original file.");
    }
}
