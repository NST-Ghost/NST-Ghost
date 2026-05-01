fn main() {
    let path = "demo/KZO v0.2/ZZZTestURP_Data/sharedassets1.assets.original";
    let mut data = std::fs::read(path).expect("Cannot read original file");
    
    let old_str = b"Do you have any useful tips?";
    let test_str = b"Can you give me some tips???"; // 28 bytes เป๊ะๆ

    let mut found_count = 0;
    let mut i = 0;
    while i <= data.len() - old_str.len() {
        if &data[i..i+old_str.len()] == old_str {
            println!("FOUND match at file offset {}", i);
            data[i..i+old_str.len()].copy_from_slice(test_str);
            found_count += 1;
        }
        i += 1;
    }

    if found_count > 0 {
        std::fs::write("sharedassets1_test_tips.assets", &data).unwrap();
        println!("Successfully patched {} occurrences with same-length string.", found_count);
    } else {
        println!("Target string not found!");
    }
}
