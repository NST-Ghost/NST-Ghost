use std::fs;

#[test]
fn probe_header() {
    let data = fs::read("demo/KZO v0.2/ZZZTestURP_Data/globalgamemanagers.assets").unwrap();
    
    // Scan for "2022.3.6"
    let pattern = b"2022.3.6";
    if let Some(pos) = data.windows(pattern.len()).position(|window| window == pattern) {
        println!("Found Unity Version string at offset: {}", pos);
        // Look around for other metadata
        println!("Bytes around offset: {:02X?}", &data[pos-32..pos+32]);
    }
}
