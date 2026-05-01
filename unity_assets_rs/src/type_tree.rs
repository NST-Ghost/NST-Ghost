use crate::reader::{BinaryReader, Endian};
use anyhow::{bail, Result};
use indexmap::IndexMap;
use std::collections::HashMap;

static COMMON_STRINGS: &[(u32, &str)] = &[
    (0,   "AABB"),
    (5,   "AnimationClip"),
    (19,  "AnimationCurve"),
    (34,  "AnimationState"),
    (49,  "Array"),
    (55,  "Base"),
    (60,  "BitField"),
    (69,  "bitset"),
    (76,  "bool"),
    (81,  "char"),
    (86,  "ColorRGBA"),
    (96,  "Component"),
    (106, "data"),
    (111, "deque"),
    (117, "double"),
    (124, "dynamic_array"),
    (138, "FastPropertyName"),
    (155, "first"),
    (161, "float"),
    (167, "Font"),
    (172, "GameObject"),
    (183, "Generic Mono"),
    (196, "GradientNEW"),
    (208, "GUID"),
    (213, "GUIStyle"),
    (222, "int"),
    (226, "list"),
    (231, "long long"),
    (241, "map"),
    (245, "Matrix4x4f"),
    (256, "MdFour"),
    (263, "MonoBehaviour"),
    (277, "MonoScript"),
    (288, "m_ByteSize"),
    (299, "m_Curve"),
    (307, "m_EditorClassIdentifier"),
    (331, "m_EditorHideFlags"),
    (349, "m_Enabled"),
    (359, "m_ExtensionPtr"),
    (374, "m_GameObject"),
    (387, "m_Index"),
    (395, "m_IsArray"),
    (405, "m_IsStatic"),
    (416, "m_MetaFlag"),
    (427, "m_Name"),
    (434, "m_ObjectHideFlags"),
    (452, "m_PrefabInternal"),
    (469, "m_PrefabParentObject"),
    (490, "m_Script"),
    (499, "m_StaticEditorFlags"),
    (519, "m_Type"),
    (526, "m_Version"),
    (536, "Object"),
    (543, "pair"),
    (548, "PPtr<Component>"),
    (564, "PPtr<GameObject>"),
    (581, "PPtr<Material>"),
    (596, "PPtr<MonoBehaviour>"),
    (616, "PPtr<MonoScript>"),
    (633, "PPtr<Object>"),
    (646, "PPtr<Prefab>"),
    (659, "PPtr<Sprite>"),
    (672, "PPtr<TextAsset>"),
    (688, "PPtr<Texture>"),
    (702, "PPtr<Texture2D>"),
    (718, "PPtr<Transform>"),
    (734, "prefabs"),
    (742, "Prefab"),
    (749, "Quaternionf"),
    (761, "Rectf"),
    (767, "RectInt"),
    (775, "RectOffset"),
    (786, "second"),
    (793, "set"),
    (797, "short"),
    (803, "size"),
    (808, "SInt16"),
    (815, "SInt32"),
    (822, "SInt64"),
    (829, "SInt8"),
    (835, "staticvector"),
    (848, "string"),
    (855, "Texture"),
    (863, "Texture2D"),
    (873, "Transform"),
    (883, "TypelessData"),
    (896, "UInt16"),
    (903, "UInt32"),
    (910, "UInt64"),
    (917, "UInt8"),
    (923, "unsigned int"),
    (936, "unsigned long long"),
    (955, "unsigned short"),
    (970, "vector"),
    (977, "Vector2f"),
    (986, "Vector3f"),
    (995, "Vector4f"),
    (1004,"m_ScriptingClassIdentifier"),
    (1030,"Gradient"),
    (1039,"Type*"),
    (1045,"int2_storage"),
    (1058,"int3_storage"),
    (1071,"BoundsInt"),
    (1081,"m_CorrespondingSourceObject"),
    (1109,"m_PrefabInstance"),
    (1126,"m_PrefabAsset"),
    (1140,"FileSize"),
    (1149,"Hash128"),
];

fn common_string(index: u32) -> Option<&'static str> {
    COMMON_STRINGS.iter().find(|&&(k, _)| k == index).map(|&(_, v)| v)
}

pub const FLAG_ALIGN_BYTES: u32 = 0x4000;

#[derive(Debug, Clone)]
pub struct TypeTreeNode {
    pub version:       u16,
    pub level:         u8,
    pub is_array:      bool,
    pub type_name:     String,
    pub name:          String,
    pub byte_size:     i32,
    pub index:         u32,
    pub meta_flag:     u32,
    pub ref_type_hash: Option<u64>,
}

impl TypeTreeNode {
    #[inline]
    pub fn align_bytes(&self) -> bool { self.meta_flag & FLAG_ALIGN_BYTES != 0 }
}

pub struct TypeTree {
    pub nodes:         Vec<TypeTreeNode>,
    pub string_buffer: Vec<u8>,
}

impl TypeTree {
    pub fn read(
        reader:       &mut BinaryReader,
        version:      u32,
        has_type_tree: bool,
    ) -> Result<HashMap<i32, TypeTree>> {
        let mut types = HashMap::new();
        let count = reader.read_u32()?;
        if count > 10_000 {
            anyhow::bail!("TypeTree::read: too many types: {}", count);
        }

        for i in 0..count {
            let class_id = reader.read_i32()?;

            if version >= 16 { reader.read_u8()?; } 

            let script_index: i16 = if version >= 17 { reader.read_i16()? } else { -1 };

            if version >= 13 {
                let has_guid = if version >= 16 { class_id == 114 } else { class_id < 0 };
                if has_guid { reader.skip(16)?; }
                reader.skip(16)?; // type_hash
            }

            if has_type_tree {
                // FIX #1: Key = class_id, not index i
                let tree = Self::read_nodes(reader, version)?;
                types.insert(class_id, tree);
            } else {
                types.insert(class_id, TypeTree { nodes: vec![], string_buffer: vec![] });
            }
            let _ = (i, script_index);
        }

        // FIX #2: ref-types block only when has_type_tree is true
        if version >= 21 && has_type_tree {
            let ref_count = reader.read_u32()?;
            for _ in 0..ref_count {
                let class_id = reader.read_i32()?;
                if version >= 16 { reader.read_u8()?; }
                let scr_idx: i16 = if version >= 17 { reader.read_i16()? } else { -1 };
                if version >= 13 {
                    // FIX #3: class_id == 114
                    if class_id == 114 { reader.skip(16)?; }
                    reader.skip(16)?;
                }
                if has_type_tree {
                    Self::read_nodes(reader, version)?;
                }
                let _ = scr_idx;
            }
        }

        Ok(types)
    }

    fn read_nodes(reader: &mut BinaryReader, version: u32) -> Result<TypeTree> {
        let node_count      = reader.read_u32()?;
        let string_buf_size = reader.read_u32()?;

        if node_count > 100_000 {
            anyhow::bail!("read_nodes: too many nodes: {}", node_count);
        }

        let mut raw = Vec::with_capacity(node_count as usize);
        for _ in 0..node_count {
            let node_ver  = reader.read_u16()?;
            let level     = reader.read_u8()?;
            let is_array  = reader.read_u8()? != 0;
            let type_idx  = reader.read_u32()?;
            let name_idx  = reader.read_u32()?;
            let byte_size = reader.read_i32()?;
            let index     = reader.read_u32()?;
            let meta_flag = reader.read_u32()?;
            let ref_hash  = if version >= 19 { Some(reader.read_u64()?) } else { None };
            raw.push((node_ver, level, is_array, type_idx, name_idx, byte_size, index, meta_flag, ref_hash));
        }

        let string_buffer = reader.read_bytes(string_buf_size as usize)?;

        let nodes = raw.into_iter().map(|(node_ver, level, is_array, type_idx, name_idx, byte_size, index, meta_flag, ref_hash)| {
            TypeTreeNode {
                version:       node_ver,
                level,
                is_array,
                type_name:     Self::resolve_string(&string_buffer, type_idx),
                name:          Self::resolve_string(&string_buffer, name_idx),
                byte_size,
                index,
                meta_flag,
                ref_type_hash: ref_hash,
            }
        }).collect();

        Ok(TypeTree { nodes, string_buffer })
    }

    pub fn resolve_string(local_buf: &[u8], index: u32) -> String {
        if index & 0x8000_0000 != 0 {
            let common_idx = index & 0x7FFF_FFFF;
            common_string(common_idx)
                .map(|s| s.to_string())
                .unwrap_or_else(|| format!("<common:{}>", common_idx))
        } else {
            let start = index as usize;
            if start >= local_buf.len() { return "<oor>".to_string(); }
            let end = local_buf[start..].iter().position(|&b| b == 0).unwrap_or(local_buf.len() - start);
            String::from_utf8_lossy(&local_buf[start..start + end]).into_owned()
        }
    }
}

// ─── UnityValue ──────────────────────────────────────────────────────────────

#[derive(Debug, Clone, serde::Serialize)]
#[serde(untagged)]
pub enum UnityValue {
    Bool(bool),
    Int(i64),
    UInt(u64),
    Float(f32),
    Double(f64),
    String(String),
    Bytes(Vec<u8>),
    Array(Vec<UnityValue>),
    Map(IndexMap<String, UnityValue>),
}

pub fn read_typetree(data: &[u8], nodes: &[TypeTreeNode], endian: Endian) -> Result<UnityValue> {
    if nodes.is_empty() { bail!("TypeTree has no nodes"); }
    let mut reader = BinaryReader::new(data, endian);
    let (value, _) = read_node(&mut reader, nodes, 0)?;
    Ok(value)
}

fn read_node(reader: &mut BinaryReader, nodes: &[TypeTreeNode], idx: usize) -> Result<(UnityValue, usize)> {
    let node = &nodes[idx];
    let type_name = node.type_name.as_str();

    let scalar = match type_name {
        "bool"                                       => Some(UnityValue::Bool(reader.read_bool()?)),
        "SInt8"  | "char"                            => Some(UnityValue::Int(reader.read_i8()? as i64)),
        "UInt8"  | "unsigned char"                   => Some(UnityValue::UInt(reader.read_u8()? as u64)),
        "SInt16" | "short"                           => Some(UnityValue::Int(reader.read_i16()? as i64)),
        "UInt16" | "unsigned short"                  => Some(UnityValue::UInt(reader.read_u16()? as u64)),
        "SInt32" | "int"                             => Some(UnityValue::Int(reader.read_i32()? as i64)),
        "UInt32" | "unsigned int"                    => Some(UnityValue::UInt(reader.read_u32()? as u64)),
        "SInt64" | "long long"                       => Some(UnityValue::Int(reader.read_i64()?)),
        "UInt64" | "unsigned long long" | "FileSize" => Some(UnityValue::UInt(reader.read_u64()?)),
        "float"                                      => Some(UnityValue::Float(reader.read_f32()?)),
        "double"                                     => Some(UnityValue::Double(reader.read_f64()?)),
        _ => None,
    };

    if let Some(v) = scalar {
        if node.align_bytes() { reader.align(4)?; }
        return Ok((v, skip_children(nodes, idx)));
    }

    if type_name == "string" {
        let len   = reader.read_u32()? as usize;
        let bytes = reader.read_bytes(len)?;
        reader.align(4)?;
        return Ok((UnityValue::String(String::from_utf8_lossy(&bytes).into_owned()), skip_children(nodes, idx)));
    }

    if type_name == "TypelessData" {
        let len   = reader.read_u32()? as usize;
        let bytes = reader.read_bytes(len)?;
        if node.align_bytes() { reader.align(4)?; }
        return Ok((UnityValue::Bytes(bytes), skip_children(nodes, idx)));
    }

    if type_name == "Array" || node.is_array {
        let children = child_indices(nodes, idx);
        if children.len() < 2 { bail!("Array node has <2 children at idx={}", idx); }
        let (size_val, _) = read_node(reader, nodes, children[0])?;
        let count = match &size_val {
            UnityValue::Int(n)  => *n as usize,
            UnityValue::UInt(n) => *n as usize,
            _ => bail!("Array size not integer"),
        };
        let elem_type = nodes[children[1]].type_name.as_str();
        if matches!(elem_type, "UInt8" | "unsigned char" | "char" | "SInt8") {
            let bytes = reader.read_bytes(count)?;
            if node.align_bytes() { reader.align(4)?; }
            return Ok((UnityValue::Bytes(bytes), skip_children(nodes, idx)));
        }
        let mut arr = Vec::with_capacity(count);
        for _ in 0..count {
            let (elem, _) = read_node(reader, nodes, children[1])?;
            arr.push(elem);
        }
        if node.align_bytes() { reader.align(4)?; }
        return Ok((UnityValue::Array(arr), skip_children(nodes, idx)));
    }

    if type_name == "vector" {
        let children = child_indices(nodes, idx);
        if children.is_empty() { bail!("vector has no children"); }
        let (v, _) = read_node(reader, nodes, children[0])?;
        if node.align_bytes() { reader.align(4)?; }
        return Ok((v, skip_children(nodes, idx)));
    }

    if type_name == "map" {
        let children = child_indices(nodes, idx);
        if children.is_empty() { bail!("map has no children"); }
        let arr_idx      = children[0];
        let arr_children = child_indices(nodes, arr_idx);
        if arr_children.len() < 2 { bail!("map Array has <2 children"); }
        let (size_val, _) = read_node(reader, nodes, arr_children[0])?;
        let count = match &size_val {
            UnityValue::Int(n)  => *n as usize,
            UnityValue::UInt(n) => *n as usize,
            _ => bail!("map size not integer"),
        };
        let pair_idx      = arr_children[1];
        let pair_children = child_indices(nodes, pair_idx);
        if pair_children.len() < 2 { bail!("map pair has <2 children"); }
        let mut map = IndexMap::with_capacity(count);
        for _ in 0..count {
            let (key_val, _) = read_node(reader, nodes, pair_children[0])?;
            let key_str = match key_val { UnityValue::String(s) => s, other => format!("{:?}", other) };
            let (val, _) = read_node(reader, nodes, pair_children[1])?;
            map.insert(key_str, val);
        }
        if node.align_bytes() { reader.align(4)?; }
        return Ok((UnityValue::Map(map), skip_children(nodes, idx)));
    }

    if type_name == "pair" {
        let children = child_indices(nodes, idx);
        if children.len() < 2 { bail!("pair has <2 children"); }
        let (first, _)  = read_node(reader, nodes, children[0])?;
        let (second, _) = read_node(reader, nodes, children[1])?;
        let mut m = IndexMap::new();
        m.insert("first".to_string(), first);
        m.insert("second".to_string(), second);
        if node.align_bytes() { reader.align(4)?; }
        return Ok((UnityValue::Map(m), skip_children(nodes, idx)));
    }

    // Struct / object
    let children = child_indices(nodes, idx);
    let mut map  = IndexMap::new();
    for &child_idx in &children {
        let child_name = nodes[child_idx].name.clone();
        let (child_val, _) = read_node(reader, nodes, child_idx)?;
        map.insert(child_name, child_val);
    }
    if node.align_bytes() { reader.align(4)?; }
    Ok((UnityValue::Map(map), skip_children(nodes, idx)))
}

fn child_indices(nodes: &[TypeTreeNode], idx: usize) -> Vec<usize> {
    let target = nodes[idx].level + 1;
    let mut result = vec![];
    let mut i = idx + 1;
    while i < nodes.len() {
        let lvl = nodes[i].level;
        if lvl < target { break; }
        if lvl == target { result.push(i); }
        i += 1;
    }
    result
}

fn skip_children(nodes: &[TypeTreeNode], idx: usize) -> usize {
    let parent_level = nodes[idx].level;
    let mut i = idx + 1;
    while i < nodes.len() && nodes[i].level > parent_level { i += 1; }
    i
}
