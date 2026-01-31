use anyhow::{bail, Context, Result};
use std::io::Read;
use std::fs::File;
use std::path::Path;
use flate2::read::ZlibDecoder;

pub fn read_rpyc_file<P: AsRef<Path>>(path: P) -> Result<Vec<u8>> {
    let mut file = File::open(path)?;
    let mut buffer = Vec::new();
    file.read_to_end(&mut buffer)?;

    if buffer.starts_with(b"RENPY RPC2") {
        // V2 format
        parse_v2_container(&buffer)
    } else {
        // V1 format (just the zlib blob potentially)
        // Note: The python script says "if the header isn't present, it should be a RPYC V1 file, which is just the blob"
        // But then it tries zlib.decompress(contents).
        Ok(buffer)
    }
}

fn parse_v2_container(data: &[u8]) -> Result<Vec<u8>> {
    let header_len = 10; // "RENPY RPC2"
    let mut position = header_len;
    
    // Each slot is 12 bytes: slot(u32), start(u32), length(u32)
    while position + 12 <= data.len() {
        let slot_bytes = &data[position..position+4];
        let start_bytes = &data[position+4..position+8];
        let len_bytes = &data[position+8..position+12];

        let slot = u32::from_le_bytes(slot_bytes.try_into()?);
        let start = u32::from_le_bytes(start_bytes.try_into()?);
        let length = u32::from_le_bytes(len_bytes.try_into()?);

        if slot == 0 {
            break;
        }

        if slot == 1 {
            // This is the data slot
            let start = start as usize;
            let length = length as usize;
            if start + length > data.len() {
                bail!("Invalid slot definition: data out of bounds");
            }
            return Ok(data[start..start+length].to_vec());
        }

        position += 12;
    }

    bail!("Could not find slot 1 in RPYC v2 container");
}

pub fn decompress_data(data: &[u8]) -> Result<Vec<u8>> {
    let mut decoder = ZlibDecoder::new(data);
    let mut decompressed = Vec::new();
    decoder.read_to_end(&mut decompressed).context("Failed to decompress zlib data")?;
    Ok(decompressed)
}
