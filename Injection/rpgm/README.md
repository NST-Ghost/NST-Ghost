# NST Translation Layer — RPG Maker

ระบบแปลภาษาแบบ drop-in สำหรับเกม RPG Maker MV/MZ  
**ไม่แก้ไขไฟล์ต้นฉบับเกม** — วางไฟล์แล้วเล่นได้เลย

## รองรับ
- ✅ NW.js (PC — Windows/Linux/Mac)
- ✅ JoiPlay (Android)
- ✅ Browser
- ✅ Electron

---

## วิธีติดตั้ง (สำหรับผู้ใช้)

### 1. คัดลอกไฟล์ลงเกม

วาง 2 อย่างนี้ลงใน root ของเกม:

```
[เกม]/
├── js/plugins/
│   └── NST_TranslationLayer.js     ← คัดลอกไฟล์นี้มาวาง
├── nst_translations/                ← คัดลอกโฟลเดอร์นี้มาวาง
│   ├── config.txt
│   ├── Map001.txt
│   ├── Map002.txt
│   ├── Actors.txt
│   ├── Items.txt
│   └── System.txt
├── data/                            ← ไฟล์เกมเดิม (ไม่ต้องแตะ)
└── index.html
```

### 2. เปิดใช้ Plugin

เปิดไฟล์ `js/plugins.js` แล้วเพิ่มบรรทัดนี้ในอาร์เรย์:

```js
{"name":"NST_TranslationLayer","status":true,"description":"NST Translation","parameters":{}}
```

**ตัวอย่าง plugins.js:**
```js
var $plugins = [
  // ... plugin อื่นๆ ของเกม ...
  {"name":"NST_TranslationLayer","status":true,"description":"NST Translation","parameters":{}}
];
```

### 3. เล่นเกม

เปิดเกมตามปกติ — ข้อความจะถูกแปลอัตโนมัติ

---

## Format ไฟล์คำแปล (.txt)

```
# ไฟล์ Map001.txt

<<<ORIGINAL>>>
こんにちは、旅人よ。
<<<TRANSLATED>>>
สวัสดี, นักเดินทาง
<<<END>>>

<<<ORIGINAL>>>
この先は危険だ。
気をつけて。
<<<TRANSLATED>>>
ข้างหน้านี้อันตราย
ระวังตัวด้วย
<<<END>>>
```

### Key-based matching (optional — แม่นยำกว่า)

```
<<<KEY>>>
events[5].list[3].parameters[0]
<<<ORIGINAL>>>
こんにちは、旅人よ。
<<<TRANSLATED>>>
สวัสดี, นักเดินทาง
<<<END>>>
```

---

## config.txt

```ini
# เปิด/ปิดการแปล
enabled=true

# แสดง debug ใน console (F12)
debug=false

# โหลดเฉพาะ map ที่เล่น (ประหยัด memory)
lazy_load=true

# ถ้าไม่มีคำแปล: original / empty / [untranslated]
fallback=original
```

---

## Debug (F12 Console)

```js
NST.TL.stats()       // ดูจำนวน entries ที่โหลด
NST.TL.reload()      // โหลดคำแปลใหม่
NST.TL.translate("テスト")  // ทดสอบแปลคำ
```
