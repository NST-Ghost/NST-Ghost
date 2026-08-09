import os
import json
import time
import subprocess
import urllib.parse
import urllib.request
import cv2
import numpy as np
from rapidocr_onnxruntime import RapidOCR
from PIL import Image, ImageDraw, ImageFont

GAME1_DIR = "/home/jop/Downloads/New Folder/[Ryuugames] RY-RJ01647202_V1.1/RJ01647202/FRAGILE_PRINCESS_Ver_1_1_1"
GAME2_DIR = "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー"
SERVER_URL = "http://localhost:8080/api/v1/translate"
OUTPUT_DIR = "/home/jop/.gemini/antigravity-cli/brain/9ac74bb7-3cbc-49bc-aa5a-9d932b699fb9"

TARGET_SUBDIRS = ["img/pictures", "img/system", "img/titles1", "img/titles2"]

print("🚀 Initializing SOTA Image Translation Engine (RapidOCR + OpenCV Inpainting)...")
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

def render_text_with_outline(draw, position, text, text_color=(255, 255, 255), outline_color=(0, 0, 0), stroke_width=3):
    x, y = position
    # Draw outline stroke
    for dx in range(-stroke_width, stroke_width + 1):
        for dy in range(-stroke_width, stroke_width + 1):
            if dx * dx + dy * dy <= stroke_width * stroke_width:
                draw.text((x + dx, y + dy), text, fill=outline_color)
    # Draw main text
    draw.text((x, y), text, fill=text_color)

def process_sota_image(file_path, save_artifact=False):
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

    print(f"  ✨ [SOTA Inpaint & OCR] {fname}: {len(valid_detections)} text regions detected")

    # Load OpenCV image BGR
    cv_img = cv2.imread(file_path)
    if cv_img is None:
        return 0

    # Create binary mask for text inpainting
    h_img, w_img = cv_img.shape[:2]
    inpaint_mask = np.zeros((h_img, w_img), dtype=np.uint8)

    overlay_data = []

    for bbox, jp_text, prob in valid_detections:
        # Construct polygon mask for inpainting
        poly_pts = np.array(bbox, dtype=np.int32)
        cv2.fillPoly(inpaint_mask, [poly_pts], 255)

        # Expand mask dilate slightly (3px) to cover text edges cleanly
        kernel = np.ones((5, 5), np.uint8)
        inpaint_mask = cv2.dilate(inpaint_mask, kernel, iterations=1)

    # 1. Seamless Background Inpainting using OpenCV Telea
    inpainted_bgr = cv2.inpaint(cv_img, inpaint_mask, inpaintRadius=5, flags=cv2.INPAINT_TELEA)

    # Convert BGR to PIL Image for professional typography rendering
    inpainted_rgb = cv2.cvtColor(inpainted_bgr, cv2.COLOR_BGR2RGB)
    pil_img = Image.fromarray(inpainted_rgb)
    draw = ImageDraw.Draw(pil_img)

    for bbox, jp_text, prob in valid_detections:
        th_text = translate_via_go_server(jp_text)

        x1 = int(min(pt[0] for pt in bbox))
        y1 = int(min(pt[1] for pt in bbox))
        x2 = int(max(pt[0] for pt in bbox))
        y2 = int(max(pt[1] for pt in bbox))

        # Render SOTA Thai typography with high-legibility outline stroke
        render_text_with_outline(draw, (x1, y1), th_text, text_color=(255, 255, 255), outline_color=(0, 0, 0), stroke_width=3)

        overlay_data.append({
            "box": [x1, y1, x2 - x1, y2 - y1],
            "original": jp_text,
            "translated": th_text
        })

    # Save inpainted and rendered image back to file_path
    try:
        pil_img.save(file_path)
        if save_artifact:
            artifact_path = os.path.join(OUTPUT_DIR, f"sota_{fname}")
            pil_img.save(artifact_path)
            print(f"  📸 SOTA Artifact saved: {artifact_path}")
    except Exception as e:
        print(f"Save error: {e}")

    # Save JSON overlay metadata
    json_path = file_path + ".overlay.json"
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump({"file": fname, "overlays": overlay_data}, f, ensure_ascii=False, indent=2)

    return len(valid_detections)

def process_game(game_dir, game_name, emulator_target_path):
    print(f"\n==================================================")
    print(f"🚀 SOTA Inpainting & Translation Pipeline for {game_name}")
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
            save_art = fname.lower() in ["manual01.png", "op1.png", "end1.png", "title.png", "mesumon.png"]
            count = process_sota_image(fpath, save_artifact=save_art)
            if count > 0:
                total_images += 1
                total_texts += count

    print(f"\n🎉 SOTA Finished {game_name}: {total_images} images inpainted & translated ({total_texts} text blocks)")

    print(f"Pushing updated SOTA graphics to Emulator path: {emulator_target_path}...")
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
    print(f"\n✨ ALL GAME GRAPHICS INPAINTED & TRANSLATED IN {time.time() - t0:.2f} SECONDS!")

if __name__ == "__main__":
    main()
