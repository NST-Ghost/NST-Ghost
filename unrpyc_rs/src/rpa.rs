use anyhow::{Context, Result, bail};
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::Path;
use std::collections::HashMap;
use flate2::read::ZlibDecoder;
use serde_pickle::Value;

pub struct RpaArchive {
    file: File,
    index: HashMap<String, Vec<(u64, u64, Vec<u8>)>>, // filename -> [(offset, len, prefix)]
}

impl RpaArchive {
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        let mut file = File::open(path)?;
        let mut header = [0u8; 8];
        file.read_exact(&mut header)?;

        if &header == b"RPA-3.0 " {
             Self::read_v3(file)
        } else if &header == b"RPA-2.0 " {
             Self::read_v2(file)
        } else {
            bail!("Unsupported or invalid RPA header: {:?}", String::from_utf8_lossy(&header));
        }
    }

    fn read_v3(mut file: File) -> Result<Self> {
        // V3 header: "RPA-3.0 " + offset (16 hex) + " " + key (8 hex) + "\n" (total 40 bytes usually?)
        // loader.py says: l = infile.read(40) (including the "RPA-3.0 " part)
        // We already read 8 bytes.
        let mut rest_of_header = [0u8; 32]; // 40 - 8 = 32
        file.read_exact(&mut rest_of_header)?;
        let header_str = String::from_utf8_lossy(&rest_of_header);
        
        // Format: offset(16) + " " + key(8)
        // Example logic from loader.py: offset = int(l[8:24], 16), key = int(l[25:33], 16)
        // Since we read "RPA-3.0 " (8 bytes) already, indices shift by 8.
        // l[8:24] corresponds to rest_of_header[0..16]
        // l[25:33] corresponds to rest_of_header[17..25]
        
        let offset_str = &header_str[0..16];
        let key_str = &header_str[17..25];
        
        let offset = u64::from_str_radix(offset_str, 16).context("Failed to parse offset")?;
        let key = u64::from_str_radix(key_str, 16).context("Failed to parse key")?;
        
        println!("Debug: Offset={:#x}, Key={:#x}", offset, key);

        file.seek(SeekFrom::Start(offset))?;
        
        // Read zlib compressed index
        println!("Debug: Reading zlib index from offset...");
        let mut decoder = ZlibDecoder::new(&file);
        let mut decompressed = Vec::new();
        decoder.read_to_end(&mut decompressed).context("Failed to decompress index")?;
        println!("Debug: Decompressed {} bytes", decompressed.len());
        
        // Unpickle
        println!("Debug: Unpickling index...");
        let index_val: Value = serde_pickle::from_slice(&decompressed, serde_pickle::DeOptions::new())
            .context("Failed to unpickle RPA index")?;
            
        let mut index = HashMap::new();
        
        // Parse and deobfuscate index
        if let Value::Dict(map) = index_val {
            for (k, v) in map {
                let filename = match k {
                    serde_pickle::HashableValue::String(s) => s,
                    _ => continue,
                };
                
                let mut entries = Vec::new();
                if let Value::List(list) = v {
                    for item in list {
                        // item is tuple (offset, dlen) or (offset, dlen, start)
                        // v3 obfuscation: offset ^ key, dlen ^ key
                        match item {
                            Value::Tuple(t) | Value::List(t) => {
                                if t.len() >= 2 {
                                    let enc_offset = match t[0] { Value::I64(i) => i as u64, _ => 0 };
                                    let enc_dlen = match t[1] { Value::I64(i) => i as u64, _ => 0 };
                                    
                                    let raw_offset = enc_offset ^ key;
                                    let raw_dlen = enc_dlen ^ key;
                                    
                                    let prefix = if t.len() >= 3 {
                                         match &t[2] {
                                             Value::Bytes(b) => b.clone(),
                                             Value::String(s) => s.as_bytes().to_vec(),
                                             _ => Vec::new(),
                                         }
                                    } else {
                                        Vec::new()
                                    };
                                    
                                    entries.push((raw_offset, raw_dlen, prefix));
                                }
                            }
                            _ => {}
                        }
                    }
                }
                index.insert(filename, entries);
            }
        }
        
        Ok(Self { file, index })
    }

    fn read_v2(mut file: File) -> Result<Self> {
        // V2 header: "RPA-2.0 " + offset (16 hex) + " " (total 24 bytes)
        // l = infile.read(24)
        // offset = int(l[8:], 16)
        let mut rest_of_header = [0u8; 16]; // 24 - 8
        file.read_exact(&mut rest_of_header)?;
        let header_str = String::from_utf8_lossy(&rest_of_header);
        
        let offset_str = header_str.trim();
        let offset = u64::from_str_radix(offset_str, 16).context("Failed to parse offset")?;
        
        file.seek(SeekFrom::Start(offset))?;
        
        let mut decoder = ZlibDecoder::new(&file);
        let mut decompressed = Vec::new();
        decoder.read_to_end(&mut decompressed)?;
         
        let index_val: Value = serde_pickle::from_slice(&decompressed, serde_pickle::DeOptions::new())
            .context("Failed to unpickle RPA index")?;
            
        let mut index = HashMap::new();
        
        if let Value::Dict(map) = index_val {
            for (k, v) in map {
                let filename = match k {
                    serde_pickle::HashableValue::String(s) => s,
                    _ => continue,
                };
                
                let mut entries = Vec::new();
                if let Value::List(list) = v {
                    for item in list {
                        match item {
                            Value::Tuple(t) | Value::List(t) => {
                                if t.len() >= 2 {
                                    let offset = match t[0] { Value::I64(i) => i as u64, _ => 0 };
                                    let dlen = match t[1] { Value::I64(i) => i as u64, _ => 0 };
                                    let prefix = if t.len() >= 3 {
                                         match &t[2] {
                                             Value::Bytes(b) => b.clone(),
                                             Value::String(s) => s.as_bytes().to_vec(),
                                             _ => Vec::new(),
                                         }
                                    } else {
                                        Vec::new()
                                    };
                                    entries.push((offset, dlen, prefix));
                                }
                            }
                            _ => {}
                        }
                    }
                }
                index.insert(filename, entries);
            }
        }
        
        Ok(Self { file, index })       
    }

    pub fn list_files(&self) -> Vec<String> {
        self.index.keys().cloned().collect()
    }

    pub fn extract_file(&mut self, filename: &str) -> Result<Option<Vec<u8>>> {
        if let Some(entries) = self.index.get(filename) {
            // Usually takes the first entry if multiple? standardized on the first one or concatenation?
            // RenPy loader: 
            // if len(index[name]) == 1: ...
            // else: ... b"".join(data)
            
            let mut data = Vec::new();
            for (offset, len, prefix) in entries {
                self.file.seek(SeekFrom::Start(*offset))?;
                let mut chunk = vec![0u8; *len as usize];
                self.file.read_exact(&mut chunk)?;
                
                if !prefix.is_empty() {
                    data.extend_from_slice(prefix);
                }
                data.extend_from_slice(&chunk);
            }
            Ok(Some(data))
        } else {
            Ok(None)
        }
    }
}
