use flate2::read::ZlibDecoder;
/// Native RPA-3.0 archive parser
/// Parses Ren'Py's proprietary archive format without any Python dependency.
///
/// Format:
///   Line 1:  "RPA-3.0 {index_offset:016x} {xor_key:08x}\n"
///   At index_offset: zlib( pickle( index_dict ) )
///   index_dict: { filename: [(stored_offset, stored_length, prefix_bytes)] }
///   real_offset = stored_offset ^ xor_key
///   real_length = stored_length ^ xor_key
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Read, Seek, SeekFrom};

#[derive(Debug)]
pub struct RpaEntry {
    pub path: String,
    pub offset: u64,
    pub length: usize,
    pub prefix: Vec<u8>,
}

// ──────────────────────────────────────────────
// Minimal Python Pickle Protocol 2 decoder
// Only handles opcodes that appear in RPA indexes.
// ──────────────────────────────────────────────

#[derive(Debug, Clone)]
enum PVal {
    Int(i64),
    Bytes(Vec<u8>),
    Str(String),
    List(Vec<PVal>),
    Tuple(Vec<PVal>),
    Dict(Vec<(PVal, PVal)>),
}

struct PickleReader<'a> {
    data: &'a [u8],
    pos: usize,
    stack: Vec<PVal>,
    memo: HashMap<u32, PVal>,
}

impl<'a> PickleReader<'a> {
    fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            pos: 0,
            stack: Vec::new(),
            memo: HashMap::new(),
        }
    }

    fn read_byte(&mut self) -> io::Result<u8> {
        if self.pos >= self.data.len() {
            return Err(io::Error::new(io::ErrorKind::UnexpectedEof, "pickle eof"));
        }
        let b = self.data[self.pos];
        self.pos += 1;
        Ok(b)
    }

    fn read_bytes(&mut self, n: usize) -> io::Result<&'a [u8]> {
        if self.pos + n > self.data.len() {
            return Err(io::Error::new(
                io::ErrorKind::UnexpectedEof,
                "pickle eof slice",
            ));
        }
        let s = &self.data[self.pos..self.pos + n];
        self.pos += n;
        Ok(s)
    }

    fn read_u16_le(&mut self) -> io::Result<u16> {
        let b = self.read_bytes(2)?;
        Ok(u16::from_le_bytes([b[0], b[1]]))
    }

    fn read_u32_le(&mut self) -> io::Result<u32> {
        let b = self.read_bytes(4)?;
        Ok(u32::from_le_bytes([b[0], b[1], b[2], b[3]]))
    }

    fn read_i32_le(&mut self) -> io::Result<i32> {
        let b = self.read_bytes(4)?;
        Ok(i32::from_le_bytes([b[0], b[1], b[2], b[3]]))
    }

    /// LONG1: 1-byte count N, then N bytes little-endian signed integer
    fn read_long1(&mut self) -> io::Result<i64> {
        let n = self.read_byte()? as usize;
        if n == 0 {
            return Ok(0);
        }
        let bytes = self.read_bytes(n)?;
        // Sign-extend from n bytes
        let mut val: i64 = 0;
        for (i, &b) in bytes.iter().enumerate() {
            val |= (b as i64) << (i * 8);
        }
        // Sign-extend if highest bit set
        if n < 8 && bytes[n - 1] & 0x80 != 0 {
            val |= !((1i64 << (n * 8)) - 1);
        }
        Ok(val)
    }

    fn find_mark(&self) -> Option<usize> {
        // find the MARK sentinel (we push None as sentinel)
        // We use a special trick: push a sentinel PVal
        // Actually we handle MARK by scanning for a sentinel in the stack
        // Sentinel is encoded as PVal::Bytes(b"__MARK__")
        for (i, v) in self.stack.iter().enumerate().rev() {
            if let PVal::Bytes(b) = v {
                if b == b"__MARK__" {
                    return Some(i);
                }
            }
        }
        None
    }

    pub fn decode(mut self) -> io::Result<PVal> {
        loop {
            let op = self.read_byte()?;
            match op {
                // PROTO: skip next byte
                0x80 => {
                    self.read_byte()?;
                }

                // FRAME: protocol 4 frame length. The frame boundary is only
                // a streaming hint, so the minimal decoder can ignore it.
                0x95 => {
                    self.read_bytes(8)?;
                }

                // EMPTY_DICT
                0x7d => self.stack.push(PVal::Dict(Vec::new())),

                // EMPTY_LIST
                0x5d => self.stack.push(PVal::List(Vec::new())),

                // EMPTY_TUPLE
                0x29 => self.stack.push(PVal::Tuple(Vec::new())),

                // MARK: push sentinel
                0x28 => self.stack.push(PVal::Bytes(b"__MARK__".to_vec())),

                // BINPUT: memo[u8] = top
                0x71 => {
                    let idx = self.read_byte()? as u32;
                    if let Some(v) = self.stack.last() {
                        self.memo.insert(idx, v.clone());
                    }
                }

                // LONG_BINPUT: memo[u32] = top
                0x72 => {
                    let idx = self.read_u32_le()?;
                    if let Some(v) = self.stack.last() {
                        self.memo.insert(idx, v.clone());
                    }
                }

                // MEMOIZE: memo[len(memo)] = top
                0x94 => {
                    let idx = self.memo.len() as u32;
                    if let Some(v) = self.stack.last() {
                        self.memo.insert(idx, v.clone());
                    }
                }

                // BINGET: push memo[u8]
                0x68 => {
                    let idx = self.read_byte()? as u32;
                    if let Some(v) = self.memo.get(&idx) {
                        self.stack.push(v.clone());
                    }
                }

                // LONG_BINGET: push memo[u32]
                0x6a => {
                    let idx = self.read_u32_le()?;
                    if let Some(v) = self.memo.get(&idx) {
                        self.stack.push(v.clone());
                    }
                }

                // BINUNICODE: u32 length + UTF-8 string
                0x58 => {
                    let len = self.read_u32_le()? as usize;
                    let bytes = self.read_bytes(len)?;
                    let s = String::from_utf8_lossy(bytes).into_owned();
                    self.stack.push(PVal::Str(s));
                }

                // SHORT_BINUNICODE: u8 length + UTF-8 string
                0x8c => {
                    let len = self.read_byte()? as usize;
                    let bytes = self.read_bytes(len)?;
                    let s = String::from_utf8_lossy(bytes).into_owned();
                    self.stack.push(PVal::Str(s));
                }

                // SHORT_BINSTRING (Python 2): u8 length + bytes (treat as str)
                0x55 => {
                    let len = self.read_byte()? as usize;
                    let bytes = self.read_bytes(len)?;
                    self.stack.push(PVal::Bytes(bytes.to_vec()));
                }

                // BINSTRING (Python 2): u32 length + bytes
                0x54 => {
                    let len = self.read_u32_le()? as usize;
                    let bytes = self.read_bytes(len)?;
                    self.stack.push(PVal::Bytes(bytes.to_vec()));
                }

                // SHORT_BINBYTES: u8 length + bytes
                0x43 => {
                    let len = self.read_byte()? as usize;
                    let bytes = self.read_bytes(len)?;
                    self.stack.push(PVal::Bytes(bytes.to_vec()));
                }

                // BINBYTES: u32 length + bytes
                0x42 => {
                    let len = self.read_u32_le()? as usize;
                    let bytes = self.read_bytes(len)?;
                    self.stack.push(PVal::Bytes(bytes.to_vec()));
                }

                // BININT1: u8
                0x4b => {
                    let v = self.read_byte()? as i64;
                    self.stack.push(PVal::Int(v));
                }

                // BININT2: u16 LE
                0x4d => {
                    let v = self.read_u16_le()? as i64;
                    self.stack.push(PVal::Int(v));
                }

                // BININT: i32 LE
                0x4a => {
                    let v = self.read_i32_le()? as i64;
                    self.stack.push(PVal::Int(v));
                }

                // LONG1: variable-length integer
                0x8a => {
                    let v = self.read_long1()?;
                    self.stack.push(PVal::Int(v));
                }

                // LONG4: 4-byte count, then count bytes
                0x8b => {
                    let n = self.read_u32_le()? as usize;
                    if n == 0 {
                        self.stack.push(PVal::Int(0));
                        continue;
                    }
                    let bytes = self.read_bytes(n)?;
                    let mut val: i64 = 0;
                    for (i, &b) in bytes.iter().take(8).enumerate() {
                        val |= (b as i64) << (i * 8);
                    }
                    self.stack.push(PVal::Int(val));
                }

                // TUPLE1: (top,)
                0x85 => {
                    let a = self.stack.pop().unwrap_or(PVal::Int(0));
                    self.stack.push(PVal::Tuple(vec![a]));
                }

                // TUPLE2: (second, top)
                0x86 => {
                    let b = self.stack.pop().unwrap_or(PVal::Int(0));
                    let a = self.stack.pop().unwrap_or(PVal::Int(0));
                    self.stack.push(PVal::Tuple(vec![a, b]));
                }

                // TUPLE3: (third, second, top)
                0x87 => {
                    let c = self.stack.pop().unwrap_or(PVal::Int(0));
                    let b = self.stack.pop().unwrap_or(PVal::Int(0));
                    let a = self.stack.pop().unwrap_or(PVal::Int(0));
                    self.stack.push(PVal::Tuple(vec![a, b, c]));
                }

                // TUPLE: all items since MARK -> tuple
                0x74 => {
                    if let Some(mark_idx) = self.find_mark() {
                        let items: Vec<PVal> = self.stack.drain(mark_idx + 1..).collect();
                        self.stack.pop(); // remove MARK
                        self.stack.push(PVal::Tuple(items));
                    }
                }

                // APPENDED: list.append(top)
                0x61 => {
                    let item = self.stack.pop().unwrap_or(PVal::Int(0));
                    if let Some(PVal::List(ref mut lst)) = self.stack.last_mut() {
                        lst.push(item);
                    }
                }

                // APPENDS: list.extend(items since MARK)
                0x65 => {
                    if let Some(mark_idx) = self.find_mark() {
                        let items: Vec<PVal> = self.stack.drain(mark_idx + 1..).collect();
                        self.stack.pop(); // remove MARK
                        if let Some(PVal::List(ref mut lst)) = self.stack.last_mut() {
                            lst.extend(items);
                        }
                    }
                }

                // SETITEM: dict[second] = top
                0x73 => {
                    let val = self.stack.pop().unwrap_or(PVal::Int(0));
                    let key = self.stack.pop().unwrap_or(PVal::Int(0));
                    if let Some(PVal::Dict(ref mut d)) = self.stack.last_mut() {
                        d.push((key, val));
                    }
                }

                // SETITEMS: dict.update( items since MARK )
                0x75 => {
                    if let Some(mark_idx) = self.find_mark() {
                        let items: Vec<PVal> = self.stack.drain(mark_idx + 1..).collect();
                        self.stack.pop(); // remove MARK
                        if let Some(PVal::Dict(ref mut d)) = self.stack.last_mut() {
                            for chunk in items.chunks(2) {
                                if chunk.len() == 2 {
                                    d.push((chunk[0].clone(), chunk[1].clone()));
                                }
                            }
                        }
                    }
                }

                // NONE
                0x4e => self.stack.push(PVal::Bytes(Vec::new())),

                // TRUE / FALSE
                0x88 => self.stack.push(PVal::Int(1)),
                0x89 => self.stack.push(PVal::Int(0)),

                // STOP: return top of stack
                0x2e => {
                    return self.stack.pop().ok_or_else(|| {
                        io::Error::new(io::ErrorKind::InvalidData, "empty stack at STOP")
                    });
                }

                other => {
                    return Err(io::Error::new(
                        io::ErrorKind::InvalidData,
                        format!(
                            "unknown pickle opcode 0x{:02x} at pos {}",
                            other,
                            self.pos - 1
                        ),
                    ));
                }
            }
        }
    }
}

// ──────────────────────────────────────────────
// RPA-3.0 top-level parser
// ──────────────────────────────────────────────

pub fn parse_rpa(path: &str) -> io::Result<Vec<RpaEntry>> {
    let mut f = BufReader::new(File::open(path)?);

    // Read header line: "RPA-3.0 {offset:016x} {key:08x}\n"
    let mut header = String::new();
    f.read_line(&mut header)?;
    let parts: Vec<&str> = header.trim().split_whitespace().collect();

    if parts.len() < 3 || parts[0] != "RPA-3.0" {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "not an RPA-3.0 file",
        ));
    }

    let index_offset = u64::from_str_radix(parts[1], 16)
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "bad offset hex"))?;
    let xor_key = u32::from_str_radix(parts[2], 16)
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "bad key hex"))?;
    let xor_key = xor_key as i64;

    // Read + decompress index
    f.seek(SeekFrom::Start(index_offset))?;
    let mut compressed = Vec::new();
    f.read_to_end(&mut compressed)?;

    let mut decompressed = Vec::new();
    ZlibDecoder::new(&compressed[..]).read_to_end(&mut decompressed)?;

    // Parse pickle
    let root = PickleReader::new(&decompressed).decode()?;

    // Convert PVal::Dict to RpaEntry list
    let dict = match root {
        PVal::Dict(d) => d,
        _ => {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "expected dict root",
            ))
        }
    };

    let mut entries = Vec::with_capacity(dict.len());
    for (k, v) in dict {
        let filename = match k {
            PVal::Str(s) => s,
            PVal::Bytes(b) => String::from_utf8_lossy(&b).into_owned(),
            _ => continue,
        };

        let tuples = match v {
            PVal::List(lst) => lst,
            _ => continue,
        };

        for t in tuples {
            let items = match t {
                PVal::Tuple(items) => items,
                _ => continue,
            };

            if items.len() < 2 {
                continue;
            }

            let stored_offset = match &items[0] {
                PVal::Int(i) => *i,
                _ => continue,
            };
            let stored_length = match &items[1] {
                PVal::Int(i) => *i,
                _ => continue,
            };
            let prefix = match items.get(2) {
                Some(PVal::Bytes(b)) => b.clone(),
                _ => Vec::new(),
            };

            let real_offset = (stored_offset ^ xor_key) as u64;
            let real_length = (stored_length ^ xor_key) as usize;

            entries.push(RpaEntry {
                path: filename.clone(),
                offset: real_offset,
                length: real_length,
                prefix,
            });
            break; // RPA has one segment per file
        }
    }

    Ok(entries)
}

/// Read a single file from the RPA archive
pub fn read_rpa_file(archive_path: &str, entry: &RpaEntry) -> io::Result<Vec<u8>> {
    let mut f = File::open(archive_path)?;
    f.seek(SeekFrom::Start(entry.offset))?;

    let data_len = entry.length.saturating_sub(entry.prefix.len());
    let mut buf = vec![0u8; data_len];
    f.read_exact(&mut buf)?;

    let mut result = entry.prefix.clone();
    result.extend_from_slice(&buf);
    Ok(result)
}
