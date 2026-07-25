# แผนงาน: ยกเครื่อง NST RPGM Injection Layer เป็น TypeScript Sub-project

## 🎯 เป้าหมาย
เปลี่ยน `Injection/rpgm/NST_TranslationLayer.js` (1058 บรรทัดมือเขียน, hook ติดตาย, รูปแบบ .txt) ให้เป็น **TypeScript sub-project** ที่:
- ใช้ **config-driven hooks** (12 ประเภท จาก translator_scratch) แทน hook ติดตาย
- ใช้ **รูปแบบ JSON** แทน .txt (ทั้งฝั่ง C++ exporter และ JS runtime)
- เป็น **standalone package** ที่นักพัฒนาอื่นต่อยอดได้ (มี package.json, build script, type definitions)
- **เชื่อมกับ NST UI** ผ่านเมนู "Deploy as Injection Layer"

## 🏗️ การตัดสินใจด้านสถาปัตยกรรม (ยืนยันแล้ว)
1. **TS sub-project แบบ standalone** ที่ `Injection/rpgm_ts/` (เลียนแบบรูปแบบ `rpgm_rs/`)
2. **Helper script** `scripts/build-translator.sh` คอมไพล์ TS → คัดลอกผลลัพธ์ไป `Injection/rpgm/`
3. **CMake POST_BUILD** ตรวจว่าไฟล์ที่สร้างมีอยู่ → คัดลอกถ้ามี, เตือนถ้าไม่มี (ไม่บังคับใช้ Node)
4. **JSON format** — เก็บโครงสร้าง `nst_translations/` แต่เปลี่ยนเนื้อในเป็น JSON แยกตามไฟล์ข้อมูล
5. **UI wiring** — เพิ่มตัวเลือก "Deploy as Injection Layer (non-destructive)" ในเมนู

---

## 📐 โครงสร้างไฟล์ใหม่

```
Injection/
├── rpgm/                          ← ย้ายที่เก็บผลลัพธ์สร้าง (generated output)
│   ├── NST_TranslationLayer.js    ← generated จาก TS build (อยู่ใน .gitignore)
│   ├── README.md                  ← อัปเดตเอกสาร
│   ├── mv/js/main.js              ← เดิม (boot shim)
│   └── mz/js/main.js              ← เดิม (boot shim)
└── rpgm_ts/                       ← 🆕 TS sub-project (source)
    ├── package.json
    ├── tsconfig.json
    ├── rollup.config.js
    ├── babel.config.cjs
    ├── README.md
    ├── src/
    │   ├── main.ts                ← plugin entry
    │   ├── core/
    │   │   ├── translator.ts      ← core engine (พอร์ตจาก translator_scratch)
    │   │   └── custom-hooks.ts    ← hook system (พอร์ตจาก translator_scratch)
    │   ├── utils/
    │   │   ├── json-loader.ts     ← NST-adapted (โหลดจาก nst_translations/)
    │   │   ├── font-set.ts        ← ระบบฟอนต์
    │   │   ├── logger.ts
    │   │   ├── error.ts
    │   │   ├── neverthrow.ts
    │   │   └── version-detection.ts
    │   └── defaults/
    │       └── default-config.ts  ← default config (hooks, ignore patterns, control chars)
    └── typings/                   ← RPG Maker type definitions (พอร์ตจาก translator_scratch)
```

## 📦 รูปแบบการปรับใช้ (เกม) — เปลี่ยนจาก .txt เป็น JSON

```
[Game root]/
└── nst_translations/
    ├── config.json                ← config (hooks, patterns, font, sourceLocale)
    ├── Actors.json                ← แยกตามไฟล์ข้อมูล (lazy-load ได้)
    ├── Map001.json
    └── System.json
```

**`config.json`** ตรงกับ schema ของ translator_scratch (`__customHooks__`, `__textFields__`, `__textCommands__`, `__controlCharPatterns__`, `__ignorePatterns__`, `__fontConfig__`, `__sourceLocale__`)

**`Actors.json`** รูปแบบ: `{ "source_text": "translated_text", ... }` + `__regex__` entry พิเศษสำหรับ regex patterns

---

## 🚀 การดำเนินงานเป็น 5 เฟส

### เฟส 1: โครงสร้างพื้นฐาน TS + Runtime (ผลิตภัณฑ์ที่ใช้งานได้ end-to-end)
**เป้าหมาย:** TS ที่สร้างไฟล์ `NST_TranslationLayer.js` ที่อ่าน JSON และใช้ config-driven hooks

1. **สร้าง `Injection/rpgm_ts/`** พร้อม `package.json`, `tsconfig.json`, `rollup.config.js`, `babel.config.cjs` (พอร์ตจาก translator_scratch ปรับชื่อ package เป็น `nst-translator`)
2. **พอร์ต source code** จาก translator_scratch:
   - `translator.ts`, `custom-hooks.ts`, `logger.ts`, `error.ts`, `neverthrow.ts`, `version-detection.ts` — พอร์ตเกือบตรงตัว
   - `font-set.ts` — พอร์ตตรงตัว
   - `json-loader.ts` — **ปรับให้สำคัญ**: โหลดจาก `nst_translations/config.json` + สแกน `nst_translations/*.json` (lazy-load ตามแผนที่, เก็บคุณสมบัติ NST เดิม) แทนที่จะโหลด `data/translations.json` ไฟล์เดียว
3. **สร้าง `src/defaults/default-config.ts`** — default config ที่เทียบเท่า hook ที่ติดตายของ NST ปัจจุบัน (แปลง 10 hook ที่มีเป็น `__customHooks__` entries)
4. **ปรับ rollup banner** ให้เป็น NST plugin header (`@plugindesc NST Translation Layer`)
5. **คัดลอก `typings/`** ทั้งโฟลเดอร์จาก translator_scratch (RPG Maker MV/MZ type defs)
6. **ย้าย `NST_TranslationLayer.js` เดิม** เก็บไว้ที่ `Injection/rpgm/NST_TranslationLayer.legacy.js` (สำรองก่อนลบในเฟสถัดไป)

### เฟส 2: C++ Exporter → JSON Output
**เป้าหมาย:** `RpgmInjectionExporter` เขียน JSON แทน .txt

1. **ปรับ `rpgm_injection_exporter.cpp`**:
   - `writeConfigFile()` → เขียน `config.json` (object พร้อม `__customHooks__`, `__textFields__`, `__textCommands__`, `__controlCharPatterns__`, `__ignorePatterns__`, `__fontConfig__`, `__sourceLocale__`) แทน `config.txt` แบบ INI
   - `writeTranslationFile()` → เขียน `<baseName>.json` (`{ source: translated, ... }`) แทนรูปแบบ `<<<KEY>>>` แบบ marker
   - เพิ่ม `writeDefaultConfig()` helper ที่ embed default config (mirror ของ `src/defaults/default-config.ts` ใน TS)
2. **อัปเดต `rpgm_injection_exporter.h`** doc comment (format description)
3. **เก็บ logic ส่วนที่ไม่เปลี่ยน**: `copyPluginFile()`, `patchPluginsJs()`, `createTranslationFolder()`, `pluginSourcePath()`
4. **ปรับเฟส progress** message ให้สะท้อน JSON

### เฟส 3: Build Integration
**เป้าหมาย:** นักพัฒนาสร้าง TS ได้ และ CMake ใช้ผลลัพธ์อย่างนุ่มนวล

1. **สร้าง `scripts/build-translator.sh`** (และ `.bat` สำหรับ Windows):
   - `cd Injection/rpgm_ts && npm ci && npm run build`
   - คัดลอก `dist/NST_TranslationLayer.js` → `Injection/rpgm/NST_TranslationLayer.js`
   - ตรวจสอบ node/npm มีอยู่ก่อน ไม่งั้นแจ้งวิธีติดตั้ง
2. **ปรับ `CMakeLists.txt` POST_BUILD** (บรรทัด 227–232):
   - เปลี่ยนจาก `copy_directory` ตาบอด เป็นตรวจก่อนว่า `Injection/rpgm/NST_TranslationLayer.js` มีอยู่จริง
   - ถ้ามี → คัดลอกปกติ
   - ถ้าไม่มี → `message(WARNING "...run scripts/build-translator.sh to build the TS layer")` แต่ยังคงสร้างได้ (ไม่พัง CMake build)
3. **อัปเดต `.gitignore`**: เพิ่ม `node_modules/`, `Injection/rpgm_ts/dist/`, `Injection/rpgm/NST_TranslationLayer.js` (generated)
4. **เพิ่มบันทึกการติดตั้ง**: `Injection/rpgm_ts/README.md` อธิบายข้อกำหนด Node, วิธีสร้าง

### เฟส 4: UI Wiring — เชื่อมเข้าเมนู
**เป้าหมาย:** ผู้ใช้เลือก deploy แบบ non-destructive ได้จาก UI

1. **เพิ่ม QAction** ใน `src/ui/menubar.cpp` (~บรรทัด 63): `"Deploy as Injection Layer..."` คู่กับ `deployProjectAction` เดิม
2. **เพิ่ม signal** `deployAsInjection()` ใน `MenuBar`
3. **เชื่อมใน `mainwindow.cpp`** (~บรรทัด 140): `connect(m_menuBar, &MenuBar::deployAsInjection, this, &MainWindow::onDeployAsInjection)`
4. **เพิ่ม slot** `MainWindow::onDeployAsInjection()` → เรียก `m_fileTranslationWidget->onDeployAsInjection()`
5. **เพิ่มเมธอด** `FileTranslationWidget::onDeployAsInjection()`:
   - โต้ตอบถามเส้นทางเกม (QFileDialog, เหมือน `onDeployProject` เดิม)
   - เรียก `m_translationCore->deployAsInjection(languageName)` (ใช้เส้นทางเดิมจาก `m_projectPath`)
   - แสดงความคืบหน้าผ่าน `totalProgressUpdated` signal (แก้ bug lambda ที่ละเว้น `msg` ด้วย — ส่ง msg ไปแสดงด้วย)
6. **เพิ่มเมนู Help** อธิบายความแตกต่าง: destructive (เขียนทับ JSON เกม) vs injection (วางไฟล์ ไม่แต่ต้นฉบับ)

### เฟส 5: การย้ายฟีเจอร์ขั้นสูง (ค่าเพิ่ม, ทำตามลำดับความสำคัญ)
**เป้าหมาย:** ดึงจุดแข็งที่เหลือจาก translator_scratch มาใช้

ทำตามลำดับ (แต่ละข้อเป็นคอมมิตแยก):
1. **รูปแบบ Ignore** — ย้าย ~80 regex จาก `translator_scratch/data/translator_config.json` มาใส่ใน `default-config.ts` (ลดงานแปลล้มเหลว + ประหยัดแคช)
2. **รูปแบบ Control char แบบขยาย** — ย้ายชุด ~90 รูปแบบ (YEP, VisuStella, MOG, GALV, HTML tags, ฯลฯ)
3. **LRU cache** — มาแล้วจากการพอร์ต translator.ts (ใช้ `mnemonist/lru-cache`)
4. **การตรวจสอบ Schema ของ Zod** — มาแล้วจากการพอร์ต json-loader.ts
5. **Third-party plugin hooks** — เพิ่ม hook entries สำหรับ YEP_MessageCore, VisuMZ, YEP_GabWindow, YEP_QuestJournal, GALV_MessageBackLog (จากไฟล์ config ตัวอย่าง) ตั้งค่า `enabled: false` เป็นค่าเริ่มต้น
6. **Debug API** — เพิ่ม `window.NST.TL` API ที่เทียบเท่าเดิม (stats/reload/missed) + API ดีบักใหม่ (testTranslate, extractControls, getDict)

---

## ✅ เกณฑ์การยอมรับ (ต่อเฟส)
- **เฟส 1–2**: เกมทดสอบ RPGM MV + MZ รันแล้วแปลได้ถูกต้อง อ่านจาก `nst_translations/*.json`
- **เฟส 3**: `scripts/build-translator.sh` ทำงานได้บน Linux + Windows; CMake build สำเร็จแม้ไม่มี Node (warning อย่างเดียว)
- **เฟส 4**: เมนู "Deploy as Injection Layer..." ทำงานได้ สร้างโฟลเดอร์ `nst_translations/` ในเกมทดสอบ วางแล้วเล่นได้
- **เฟส 5**: แต่ละฟีเจอร์ทดสอบด้วยเคสเฉพาะ (เช่น ข้อความที่ match ignore pattern ไม่ถูกแปล)

## ⚠️ ความเสี่ยงและการบรรเทา
| ความเสี่ยง | การบรรเทา |
|-----------|-----------|
| ขนาดไฟล์ใหญ่ขึ้นจากการรวม (bundle) | เปิดใช้งาน terser ในโหมด prod, ตรวจสอบขนาดต่อไฟล์ < 100KB |
| เกมเก่าที่ใช้ .txt เสีย | เก็บไฟล์มรดก `NST_TranslationLayer.legacy.js` ไว้ 1 รอบการเผยแพร่; เพิ่มตัวเลือก "ใช้เลเยอร์ดั้งเดิม" |
| Node ไม่มีในเครื่องนักพัฒนา | CMake warning ชัดเจน + เอกสาร; ไฟล์ JS ที่สร้างสามารถ commit ชั่วคราวได้ในตอนเริ่ม |
| ช่องโหว่การผลิกแพ็กเกจ CI | เฟส 3 รวมการอัปเดต `.github/workflows/release-linux.yml` เพื่อคัดลอก `Injection/rpgm/` ไปยัง AppDir (เฟสติดตามผลแยกต่างหาก) |

## 📝 หมายเหตุ
- **เฟส 1+2 ต้องส่งมอบพร้อมกัน** (runtime อ่าน JSON ต้องมี exporter ผลิต JSON)
- ระบบปัจจุบัน `deployAsInjection()` เป็นโค้ดที่ไม่ได้ใช้ (dead code) — เฟส 4 จะเปิดใช้งานจริงเป็นครั้งแรก
- โครงสร้าง TS sub-project ทำให้นักพัฒนาอื่นสร้าง (fork) และเพิ่ม hook ได้โดยไม่ต้องแตะ C++ (แค่แก้ `default-config.ts` หรือไฟล์ `config.json` ในเกม)
