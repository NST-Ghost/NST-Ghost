# NST Translation Layer — RPG Maker

ระบบแปลภาษาแบบ **drop-in** สำหรับเกม RPG Maker MV/MZ  
**ไม่ต้องเปิดโปรแกรมอะไร — ไม่ต้องแก้ไขอะไร — วางไฟล์แล้วเล่นได้เลย**

ใช้ได้ทุกที่: NW.js (PC) / JoiPlay (Android) / Browser / Linux / Mac

---

## วิธีใช้ — วางแล้วเล่น

### สำหรับเกม RPG Maker MV

คัดลอกทั้ง 3 อย่างนี้วางทับลงในโฟลเดอร์เกม:

```
คัดลอกจาก mv/ :
├── js/
│   ├── main.js                      ← วางทับ (auto-boot)
│   └── plugins/
│       └── NST_TranslationLayer.js  ← วาง
└── nst_translations/                ← วาง
    ├── config.txt
    ├── Map001.txt
    └── ...
```

### สำหรับเกม RPG Maker MZ

คัดลอกจาก `mz/` แทน (โครงสร้างเหมือนกัน)

### จบ — เปิดเกมเล่นได้เลย

---

## สร้างไฟล์คำแปล

### Format (.txt)

สร้างไฟล์ `.txt` ใน `nst_translations/` ตั้งชื่อตาม data file ของเกม:

| ไฟล์เกม (data/) | ไฟล์แปล (nst_translations/) |
|-----------------|---------------------------|
| Map001.json | Map001.txt |
| Actors.json | Actors.txt |
| Items.json | Items.txt |
| System.json | System.txt |
| CommonEvents.json | CommonEvents.txt |

### เขียนคำแปล

```
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

### config.txt

```ini
enabled=true
debug=false
lazy_load=true
fallback=original
```

---

## Debug (F12)

```js
NST.TL.stats()       // ดูจำนวน entries
NST.TL.reload()      // โหลดคำแปลใหม่
```
