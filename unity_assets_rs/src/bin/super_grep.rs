use std::io::Read;

fn main() {
    let keywords = b"Do you have any useful tips?";
    let dir = "demo/KZO v0.2/ZZZTestURP_Data/";
    
    for entry in std::fs::read_dir(dir).unwrap() {
        let path = entry.unwrap().path();
        if path.is_file() {
            let mut file = std::fs::File::open(&path).unwrap();
            let mut buffer = Vec::new();
            file.read_to_end(&mut buffer).unwrap();
            
            if buffer.windows(keywords.len()).any(|w| w == keywords) {
                println!("FOUND IN: {:?}", path);
            }
        }
    }
}
