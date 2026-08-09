import os
import json
import time
import subprocess
import urllib.parse
import urllib.request
from rapidocr_onnxruntime import RapidOCR
from PIL import Image, ImageDraw

GAME1_DIR = "/home/jop/Downloads/New Folder/[Ryuugames] RY-RJ01647202_V1.1/RJ01647202/FRAGILE_PRINCESS_Ver_1_1_1"
GAME2_DIR = "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー"
SERVER_URL = "http://localhost:8080/api/v1/translate"

TARGET_SUBDIRS = ["img/pictures", "img/system", "img/titles1", "img/titles2"]

print("🚀 Initializing RapidOCR (ONNXRuntime Ultra-Fast Engine)...")
engine = RapidOCR()

def translate_via_go_server(text):
    if not text or not text.strip():
        return text
    try:
        payload = json.dumps({"text": text}).encode('utf-8')
        req = urllib.request.Request(SERVER_URL, data=payload, headers={'Content-Type': 'application/json'})
        res = urllib.request.urlopen(req, timeout=5).read().decode('utf-8')
        data = json.loads(res)
        if data.get("status") == "success":
            return data.get("translated", text)
    except Exception:
        pass
    return text

def is_text_graphic_candidate(filepath):
    fname = os.path.basename(filepath).lower()
    if any(fname.startswith(prefix) for prefix in ["icon", "item", "gauge", "circle", "hand", "heart", "chibi", "darksoul"]):
        return False
    
    priority_keywords = ["manual", "op", "end", "title", "splash", "gameover", "stage", "result", "warning", "infowindow", "_h", "_back", "logo", "mesumon"]
    if any(kw in fname for kw in priority_keywords):
        return True
        
    try:
        with Image.open(filepath) as img:
            w, h = img.size
            return w >= 300 and h >= 200
    except Exception:
        return False

def process_image(file_path):
    if not is_text_graphic_candidate(file_path):
        return 0

    fname = os.path.basename(file_path)

    try:
        results, elapse = engine(file_path)
    except Exception:
        return 0

    if not results:
        return 0

    valid_detections = []
    for item in results:
        bbox, text_str, prob = item[0], str(item[1]).strip(), float(item[2])
        if prob >= 0.35 and len(text_str) >= 2:
            valid_detections.append((bbox, text_str, prob))

    if not valid_detections:
        return 0

    print(f"  ⚡ [RapidOCR] {fname}: {len(valid_detections)} text regions detected")

    try:
        img = Image.open(file_path).convert("RGBA")
    except Exception:
        return 0

    draw = ImageDraw.Draw(img)
    overlay_data = []

    for bbox, jp_text, prob in valid_detections:
        th_text = translate_via_go_server(jp_text)
        
        x1 = int(min(pt[0] for pt in bbox))
        y1 = int(min(pt[1] for pt in bbox))
        x2 = int(max(pt[0] for pt in bbox))
        y2 = int(max(pt[1] for pt in bbox))
        
        w = max(x2 - x1, 140)
        h = max(y2 - y1, 32)

        draw.rectangle([x1, y1, x1 + w, y1 + h], fill=(15, 23, 42, 230), outline=(139, 92, 246, 255), width=2)
        draw.text((x1 + 6, y1 + 4), th_text, fill=(255, 255, 255, 255))

        overlay_data.append({
            "box": [x1, y1, w, h],
            "original": jp_text,
            "translated": th_text
        })

    try:
        img.convert("RGB").save(file_path)
    except Exception:
        pass

    json_path = file_path + ".overlay.json"
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump({"file": fname, "overlays": overlay_data}, f, ensure_ascii=False, indent=2)

    return len(valid_detections)

def process_game(game_dir, game_name, emulator_target_path):
    print(f"\n==================================================")
    print(f"🚀 RapidOCR Batch Translating ALL Images in {game_name}")
    print(f"==================================================")

    total_images = 0
    total_texts = 0

    for subdir in TARGET_SUBDIRS:
        full_subdir = os.path.join(game_dir, subdir)
        if not os.path.exists(full_subdir):
            continue

        print(f"\nScanning directory: {subdir}...")
        files = sorted([f for f in os.listdir(full_subdir) if f.lower().endswith('.png') or f.lower().endswith('.jpg')])
        
        for fname in files:
            fpath = os.path.join(full_subdir, fname)
            count = process_image(fpath)
            if count > 0:
                total_images += 1
                total_texts += count

    print(f"\n🎉 Finished {game_name}: {total_images} story images translated ({total_texts} text blocks)")

    print(f"Pushing updated graphics to Emulator path: {emulator_target_path}...")
    for subdir in TARGET_SUBDIRS:
        src = os.path.join(game_dir, subdir)
        dst = os.path.join(emulator_target_path, subdir)
        if os.path.exists(src):
            subprocess.run(["adb", "-s", "emulator-5554", "shell", f"mkdir -p {dst}"])
            subprocess.run(["adb", "-s", "emulator-5554", "push", src, os.path.dirname(dst)])

def main():
    t0 = time.time()
    process_game(GAME2_DIR, "悪堕ち魔法少女クリッカー", "/sdcard/NSTGames/AkudochiClicker")
    process_game(GAME1_DIR, "FRAGILE PRINCESS Ver 1.1.1", "/sdcard/NSTGames/FragilePrincess")
    print(f"\n✨ ALL GAME STORY GRAPHICS TRANSLATED WITH RAPIDOCR IN {time.time() - t0:.2f} SECONDS!")

if __name__ == "__main__":
    main()
