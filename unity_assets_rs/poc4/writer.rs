/// writer.rs — serialize a UnityValue back to binary bytes using the TypeTree.
/// Mirrors read_node() in type_tree.rs exactly (same alignment rules).
use crate::reader::Endian;
use crate::type_tree::{TypeTreeNode, UnityValue, FLAG_ALIGN_BYTES};
use anyhow::{bail, Result};

// ─── public entry point ───────────────────────────────────────────────────────

pub fn write_typetree(value: &UnityValue, nodes: &[TypeTreeNode], endian: Endian) -> Result<Vec<u8>> {
    if nodes.is_empty() { bail!("TypeTree has no nodes"); }
    let mut buf = Vec::new();
    write_node(&mut buf, value, nodes, 0, endian)?;
    Ok(buf)
}

// ─── helpers ──────────────────────────────────────────────────────────────────

#[inline]
fn align4(buf: &mut Vec<u8>) {
    let r = buf.len() % 4;
    if r != 0 { buf.resize(buf.len() + (4 - r), 0); }
}

macro_rules! write_scalar {
    ($buf:expr, $v:expr, $endian:expr, $ty:ty) => {{
        let bytes: [u8; std::mem::size_of::<$ty>()] = match $endian {
            Endian::Little => (<$ty>::to_le_bytes($v as $ty)),
            Endian::Big    => (<$ty>::to_be_bytes($v as $ty)),
        };
        $buf.extend_from_slice(&bytes);
    }};
}

// ─── recursive node writer ───────────────────────────────────────────────────

fn write_node(
    buf:    &mut Vec<u8>,
    value:  &UnityValue,
    nodes:  &[TypeTreeNode],
    idx:    usize,
    endian: Endian,
) -> Result<usize> {
    let node      = &nodes[idx];
    let type_name = node.type_name.as_str();
    let align     = node.meta_flag & FLAG_ALIGN_BYTES != 0;

    // ── scalars ───────────────────────────────────────────────────────────────
    let wrote = match (type_name, value) {
        ("bool", UnityValue::Bool(v)) => {
            buf.push(*v as u8); true
        }
        ("SInt8" | "char", UnityValue::Int(v)) => {
            buf.push(*v as i8 as u8); true
        }
        ("UInt8" | "unsigned char", UnityValue::UInt(v)) => {
            buf.push(*v as u8); true
        }
        ("SInt16" | "short", UnityValue::Int(v)) => {
            write_scalar!(buf, *v, endian, i16); true
        }
        ("UInt16" | "unsigned short", UnityValue::UInt(v)) => {
            write_scalar!(buf, *v, endian, u16); true
        }
        ("SInt32" | "int", UnityValue::Int(v)) => {
            write_scalar!(buf, *v, endian, i32); true
        }
        ("UInt32" | "unsigned int", UnityValue::UInt(v)) => {
            write_scalar!(buf, *v, endian, u32); true
        }
        ("SInt64" | "long long", UnityValue::Int(v)) => {
            write_scalar!(buf, *v, endian, i64); true
        }
        ("UInt64" | "unsigned long long" | "FileSize", UnityValue::UInt(v)) => {
            write_scalar!(buf, *v, endian, u64); true
        }
        ("float", UnityValue::Float(v)) => {
            write_scalar!(buf, v.to_bits(), endian, u32); true
        }
        ("double", UnityValue::Double(v)) => {
            write_scalar!(buf, v.to_bits(), endian, u64); true
        }
        _ => false,
    };

    if wrote {
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── string ────────────────────────────────────────────────────────────────
    if type_name == "string" {
        let s = match value {
            UnityValue::String(s) => s.as_bytes(),
            // Allow Bytes too, for raw binary strings
            _ => bail!("Expected String for node '{}', got {:?}", node.name, value),
        };
        write_scalar!(buf, s.len() as u32, endian, u32);
        buf.extend_from_slice(s);
        align4(buf); // string always aligns (matches reader)
        return Ok(skip_children(nodes, idx));
    }

    // ── TypelessData ──────────────────────────────────────────────────────────
    if type_name == "TypelessData" {
        let bytes = match value {
            UnityValue::Bytes(b) => b,
            _ => bail!("Expected Bytes for TypelessData"),
        };
        write_scalar!(buf, bytes.len() as u32, endian, u32);
        buf.extend_from_slice(bytes);
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── Array / is_array ──────────────────────────────────────────────────────
    if type_name == "Array" || node.is_array {
        let children = child_indices(nodes, idx);
        if children.len() < 2 { bail!("Array node '{}' has <2 children", node.name); }

        match value {
            // Byte arrays written as bulk
            UnityValue::Bytes(bytes) => {
                write_scalar!(buf, bytes.len() as u32, endian, u32);
                buf.extend_from_slice(bytes);
            }
            UnityValue::Array(arr) => {
                write_scalar!(buf, arr.len() as u32, endian, u32);
                for elem in arr {
                    write_node(buf, elem, nodes, children[1], endian)?;
                }
            }
            _ => bail!("Expected Array/Bytes for Array node '{}'", node.name),
        }
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── vector (wraps an Array child) ─────────────────────────────────────────
    if type_name == "vector" {
        let children = child_indices(nodes, idx);
        if children.is_empty() { bail!("vector has no children"); }
        write_node(buf, value, nodes, children[0], endian)?;
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── map ───────────────────────────────────────────────────────────────────
    if type_name == "map" {
        let map = match value {
            UnityValue::Map(m) => m,
            _ => bail!("Expected Map for map node"),
        };
        let children     = child_indices(nodes, idx);
        if children.is_empty() { bail!("map has no children"); }
        let arr_idx      = children[0];
        let arr_children = child_indices(nodes, arr_idx);
        if arr_children.len() < 2 { bail!("map Array has <2 children"); }
        let pair_idx      = arr_children[1];
        let pair_children = child_indices(nodes, pair_idx);
        if pair_children.len() < 2 { bail!("map pair has <2 children"); }

        write_scalar!(buf, map.len() as u32, endian, u32);
        for (key, val) in map {
            write_node(buf, &UnityValue::String(key.clone()), nodes, pair_children[0], endian)?;
            write_node(buf, val, nodes, pair_children[1], endian)?;
        }
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── pair ──────────────────────────────────────────────────────────────────
    if type_name == "pair" {
        let map = match value {
            UnityValue::Map(m) => m,
            _ => bail!("Expected Map for pair node"),
        };
        let children = child_indices(nodes, idx);
        if children.len() < 2 { bail!("pair has <2 children"); }
        let first  = map.get("first") .ok_or_else(|| anyhow::anyhow!("pair missing 'first'"))?;
        let second = map.get("second").ok_or_else(|| anyhow::anyhow!("pair missing 'second'"))?;
        write_node(buf, first,  nodes, children[0], endian)?;
        write_node(buf, second, nodes, children[1], endian)?;
        if align { align4(buf); }
        return Ok(skip_children(nodes, idx));
    }

    // ── struct / object (default) ─────────────────────────────────────────────
    let map = match value {
        UnityValue::Map(m) => m,
        _ => bail!("Expected Map for struct node '{}' (type={})", node.name, type_name),
    };
    for &child_idx in &child_indices(nodes, idx) {
        let child_name = &nodes[child_idx].name;
        let child_val  = map.get(child_name.as_str())
            .ok_or_else(|| anyhow::anyhow!("struct '{}': missing field '{}'", node.name, child_name))?;
        write_node(buf, child_val, nodes, child_idx, endian)?;
    }
    if align { align4(buf); }
    Ok(skip_children(nodes, idx))
}

// ─── tree traversal helpers (mirrors type_tree.rs) ───────────────────────────

pub fn child_indices(nodes: &[TypeTreeNode], idx: usize) -> Vec<usize> {
    let target = nodes[idx].level + 1;
    let mut result = vec![];
    let mut i = idx + 1;
    while i < nodes.len() {
        match nodes[i].level.cmp(&target) {
            std::cmp::Ordering::Less    => break,
            std::cmp::Ordering::Equal   => result.push(i),
            std::cmp::Ordering::Greater => {}
        }
        i += 1;
    }
    result
}

pub fn skip_children(nodes: &[TypeTreeNode], idx: usize) -> usize {
    let parent_level = nodes[idx].level;
    let mut i = idx + 1;
    while i < nodes.len() && nodes[i].level > parent_level { i += 1; }
    i
}
