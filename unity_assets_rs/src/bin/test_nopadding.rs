fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let mut data = std::fs::read(path).expect("Cannot read original file");
    
    let old_str = b"What can I do for you?";
    let new_str = "มีอะไรให้ฉันช่วยไหม?";
    let new_bytes = new_str.as_bytes();

    // ค้นหาตำแหน่ง String ในทั้งไฟล์เลย (แบบถึก)
    let mut found = false;
    let mut i = 0;
    while i <= data.len() - old_str.len() {
        if &data[i..i+old_str.len()] == old_str {
            println!("FOUND target string at file offset {}", i);
            
            // ตรวจสอบว่าถ้าเราใส่ภาษาไทยลงไป มันจะทับข้อมูลอื่นไหม?
            // เนื่องจากเราไม่ต้องการขยับไฟล์ เราจะใช้วิธี "เขียนทับ" 
            // แต่ภาษาไทยยาวกว่าอังกฤษ (54 vs 22 bytes) เราทำไม่ได้หากไม่ขยับข้อมูล...
            
            // งั้นเราจะลองเปลี่ยนเป็นอังกฤษคำอื่นที่สั้นกว่าหรือเท่าเดิมเพื่อทดสอบก่อน!
            let test_str = b"Can I help you, sir?? "; // 22 bytes เป๊ะๆ
            data[i..i+22].copy_from_slice(test_str);
            found = true;
            break;
        }
        i += 1;
    }

    if found {
        std::fs::write("sharedassets1_test_nopadding.assets", &data).unwrap();
        println!("Successfully wrote sharedassets1_test_nopadding.assets (replaced with same-length English)");
    } else {
        println!("Target string not found!");
    }
}
