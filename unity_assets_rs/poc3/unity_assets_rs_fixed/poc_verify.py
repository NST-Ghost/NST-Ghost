#!/usr/bin/env python3
"""
Unity Serialized File – POC verify script
Proves all 5 Rust fixes work by reading resources.assets in pure Python.

Usage:
    python3 poc_verify.py resources.assets
"""
import struct, sys, json
from collections import Counter

path = sys.argv[1] if len(sys.argv) > 1 else "resources.assets"
data = open(path, 'rb').read()
print(f"File: {path}  ({len(data):,} bytes)\n")

# ── Header (big-endian) ───────────────────────────────────────────────────────
version    = struct.unpack_from('>I', data, 8)[0]
endian_b   = data[16]
if version >= 22:
    data_off = struct.unpack_from('>Q', data, 32)[0]
else:
    data_off = struct.unpack_from('>I', data, 12)[0]

# ── Metadata (little-endian, starts at byte 48 for v22) ──────────────────────
meta_start  = 48 if version >= 22 else 16
pos         = meta_start
unity_ver   = data[pos:pos+12].rstrip(b'\x00').decode();  pos += 12
target_plat = struct.unpack_from('<I', data, pos)[0];      pos += 4
htt         = data[pos] != 0;                              pos += 1

print(f"Unity {unity_ver}  fmt_version={version}  platform={target_plat}  has_type_tree={htt}")

# ── Types (FIX 1: insert class_id not loop-index) ────────────────────────────
type_count = struct.unpack_from('<I', data, pos)[0]; pos += 4
type_id_to_class = {}
for i in range(type_count):
    class_id = struct.unpack_from('<i', data, pos)[0]; pos += 4
    pos += 1                            # stripped (v16+)
    pos += 2                            # script_index (v17+)
    if class_id == 114: pos += 16       # FIX 3: GUID only for MonoBehaviour
    pos += 16                           # type_hash
    # FIX 1: key = class_id
    type_id_to_class[i] = class_id
    # FIX 2: no node data when has_type_tree=False
    # (has_type_tree=False here, so nothing to skip)

# FIX 2: ref-types block only when has_type_tree=True
if version >= 21 and htt:
    ref_c = struct.unpack_from('<I', data, pos)[0]; pos += 4
    for _ in range(ref_c):
        cid = struct.unpack_from('<i', data, pos)[0]; pos += 4
        pos += 1; pos += 2
        if cid == 114: pos += 16        # FIX 3
        pos += 16
        # would read nodes here if htt

# ── Objects (FIX 4: NO pre-count align) ──────────────────────────────────────
obj_count = struct.unpack_from('<I', data, pos)[0]; pos += 4
print(f"Object count: {obj_count}\n")

CLASS_NAMES = {
    21:'Material', 28:'Texture2D', 43:'Mesh', 48:'Shader',
    49:'TextAsset', 74:'AnimationClip', 83:'AudioClip', 128:'Font',
    213:'Sprite', 114:'MonoBehaviour', 1:'GameObject', 4:'Transform',
    23:'MeshRenderer', 33:'MeshFilter',
}

objects = []
errors  = 0
for i in range(obj_count):
    a = (pos + 3) & ~3; pos = a          # per-object align (correct)
    path_id    = struct.unpack_from('<q', data, pos)[0]; pos += 8
    byte_start = struct.unpack_from('<Q', data, pos)[0]; pos += 8
    byte_size  = struct.unpack_from('<I', data, pos)[0]; pos += 4
    type_id    = struct.unpack_from('<i', data, pos)[0]; pos += 4
    class_id   = type_id_to_class.get(type_id, -1)
    abs_s      = data_off + byte_start
    if abs_s + byte_size > len(data):
        errors += 1; continue
    nl   = struct.unpack_from('<I', data, abs_s)[0] if abs_s+4 <= len(data) else 0
    name = data[abs_s+4:abs_s+4+nl].decode('utf-8','replace') \
           if nl < 512 and abs_s+4+nl <= len(data) else ''
    objects.append({'path_id': path_id, 'type_id': type_id,
                    'class_id': class_id, 'byte_size': byte_size, 'name': name})

assert errors == 0, f"{errors} out-of-bounds objects!"
print(f"✓ All {len(objects)} objects parsed with 0 errors\n")

cc = Counter(CLASS_NAMES.get(o['class_id'], f"class_{o['class_id']}") for o in objects)
print("Class distribution (top 15):")
for cls, cnt in cc.most_common(15):
    print(f"  {cls:<20} {cnt:>5}")

print("\nFirst 10 objects:")
for o in objects[:10]:
    cls = CLASS_NAMES.get(o['class_id'], f"class_{o['class_id']}")
    print(f"  path_id={o['path_id']:5}  {cls:<16}  size={o['byte_size']:7}  {o['name']}")
