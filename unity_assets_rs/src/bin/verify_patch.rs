fn main() {
    let data = std::fs::read("sharedassets1_thai.assets").unwrap();
    let search = "มีอะไรให้ฉันช่วยไหม?";
    let search_bytes = search.as_bytes();
    if let Some(pos) = data.windows(search_bytes.len()).position(|w| w == search_bytes) {
        println!("SUCCESS: Found Thai string at file offset {}", pos);
        // Print surrounding bytes
        let start = pos.saturating_sub(10);
        let end = (pos + search_bytes.len() + 10).min(data.len());
        println!("Context (hex): {:02X?}", &data[start..end]);
    } else {
        println!("FAILED: Thai string not found");
    }
}
