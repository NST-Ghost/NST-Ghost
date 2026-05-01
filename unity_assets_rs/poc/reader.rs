use byteorder::{BigEndian, LittleEndian, ReadBytesExt};
use std::io::{Cursor, Read, Seek, SeekFrom};

/// Byte-order used by the reader.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Endian {
    Big,
    Little,
}

/// Cursor-backed binary reader that respects a runtime-switchable endianness.
pub struct BinaryReader<'a> {
    pub cursor: Cursor<&'a [u8]>,
    pub endian: Endian,
}

impl<'a> BinaryReader<'a> {
    pub fn new(data: &'a [u8], endian: Endian) -> Self {
        Self { cursor: Cursor::new(data), endian }
    }

    pub fn set_endian(&mut self, endian: Endian) {
        self.endian = endian;
    }

    pub fn len(&self) -> u64 {
        self.cursor.get_ref().len() as u64
    }

    // ── scalar reads ────────────────────────────────────────────────────────

    pub fn read_u8(&mut self) -> anyhow::Result<u8> {
        Ok(self.cursor.read_u8()?)
    }

    pub fn read_i8(&mut self) -> anyhow::Result<i8> {
        Ok(self.cursor.read_i8()?)
    }

    pub fn read_bool(&mut self) -> anyhow::Result<bool> {
        Ok(self.read_u8()? != 0)
    }

    pub fn read_i16(&mut self) -> anyhow::Result<i16> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_i16::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_i16::<LittleEndian>()?),
        }
    }

    pub fn read_u16(&mut self) -> anyhow::Result<u16> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_u16::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_u16::<LittleEndian>()?),
        }
    }

    pub fn read_i32(&mut self) -> anyhow::Result<i32> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_i32::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_i32::<LittleEndian>()?),
        }
    }

    pub fn read_u32(&mut self) -> anyhow::Result<u32> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_u32::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_u32::<LittleEndian>()?),
        }
    }

    pub fn read_i64(&mut self) -> anyhow::Result<i64> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_i64::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_i64::<LittleEndian>()?),
        }
    }

    pub fn read_u64(&mut self) -> anyhow::Result<u64> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_u64::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_u64::<LittleEndian>()?),
        }
    }

    pub fn read_f32(&mut self) -> anyhow::Result<f32> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_f32::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_f32::<LittleEndian>()?),
        }
    }

    pub fn read_f64(&mut self) -> anyhow::Result<f64> {
        match self.endian {
            Endian::Big    => Ok(self.cursor.read_f64::<BigEndian>()?),
            Endian::Little => Ok(self.cursor.read_f64::<LittleEndian>()?),
        }
    }

    // ── string / bytes ──────────────────────────────────────────────────────

    /// Read a Unity-style length-prefixed string and align to 4 bytes.
    pub fn read_string(&mut self) -> anyhow::Result<String> {
        let length = self.read_u32()? as usize;
        let mut buf = vec![0u8; length];
        self.cursor.read_exact(&mut buf)?;
        let padding = (4 - (length % 4)) % 4;
        self.cursor.seek(SeekFrom::Current(padding as i64))?;
        Ok(String::from_utf8_lossy(&buf).into_owned())
    }

    /// Read a null-terminated or fixed-width string (trim trailing `\0`).
    pub fn read_fixed_string(&mut self, length: usize) -> anyhow::Result<String> {
        let mut buf = vec![0u8; length];
        self.cursor.read_exact(&mut buf)?;
        let s = String::from_utf8_lossy(&buf)
            .trim_end_matches('\0')
            .to_string();
        Ok(s)
    }

    /// Read a null-terminated C-string from the current position.
    pub fn read_cstring(&mut self) -> anyhow::Result<String> {
        let mut buf = Vec::new();
        loop {
            let b = self.read_u8()?;
            if b == 0 { break; }
            buf.push(b);
        }
        Ok(String::from_utf8_lossy(&buf).into_owned())
    }

    /// Read exactly `n` bytes.
    pub fn read_bytes(&mut self, n: usize) -> anyhow::Result<Vec<u8>> {
        let mut buf = vec![0u8; n];
        self.cursor.read_exact(&mut buf)?;
        Ok(buf)
    }

    // ── position helpers ────────────────────────────────────────────────────

    pub fn seek(&mut self, pos: u64) -> anyhow::Result<()> {
        self.cursor.seek(SeekFrom::Start(pos))?;
        Ok(())
    }

    pub fn skip(&mut self, n: i64) -> anyhow::Result<()> {
        self.cursor.seek(SeekFrom::Current(n))?;
        Ok(())
    }

    pub fn tell(&self) -> u64 {
        self.cursor.position()
    }

    /// Advance cursor so `tell() % alignment == 0`.
    pub fn align(&mut self, alignment: u64) -> anyhow::Result<()> {
        if alignment == 0 { return Ok(()); }
        let pos    = self.tell();
        let offset = (alignment - (pos % alignment)) % alignment;
        if offset > 0 {
            self.cursor.seek(SeekFrom::Current(offset as i64))?;
        }
        Ok(())
    }
}
