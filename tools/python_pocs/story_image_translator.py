import os
import json
import urllib.parse
import urllib.request
from PIL import Image, ImageDraw, ImageFont

GAME_DIR = "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー"
PICTURES_DIR = os.path.join(GAME_DIR, "img/pictures")
SERVER_URL = "http://localhost:8080/api/v1/translate"
OUTPUT_DIR = "/home/jop/.gemini/antigravity-cli/brain/9ac74bb7-3cbc-49bc-aa5a-9d932b699fb9"

def translate_via_go_server(text):
    try:
        payload = json.dumps({"text": text}).encode('utf-8')
        req = urllib.request.Request(SERVER_URL, data=payload, headers={'Content-Type': 'application/json'})
        res = urllib.request.urlopen(req, timeout=5).read().decode('utf-8')
        data = json.loads(res)
        if data.get("status") == "success":
            return data.get("translated", text)
    except Exception as e:
        print(f"Server translation fallback: {e}")
    return text

def process_story_image(img_filename, jp_texts, text_boxes):
    src_path = os.path.join(PICTURES_DIR, img_filename)
    if not os.path.exists(src_path):
        print(f"Image not found: {src_path}")
        return

    img = Image.open(src_path).convert("RGBA")
    draw = ImageDraw.Draw(img)

    # 1. Translate Japanese texts via NST Go Server
    translated_texts = []
    for jp in jp_texts:
        th = translate_via_go_server(jp)
        translated_texts.append(th)

    # 2. Render overlays onto image
    overlay_metadata = {
        "image_file": img_filename,
        "width": img.width,
        "height": img.height,
        "overlays": []
    }

    for idx, box in enumerate(text_boxes):
        x, y, w, h = box["box"]
        th_text = translated_texts[idx] if idx < len(translated_texts) else jp_texts[idx]

        # Draw semi-transparent background banner for text legibility
        banner_rect = [x, y, x + w, y + h]
        draw.rectangle(banner_rect, fill=(15, 23, 42, 220), outline=(139, 92, 246, 255), width=2)

        # Draw Thai text
        draw.text((x + 12, y + 10), th_text, fill=(255, 255, 255, 255))

        overlay_metadata["overlays"].append({
            "box": box["box"],
            "original": jp_texts[idx],
            "translated": th_text
        })

    # Save translated image
    out_img_path = os.path.join(OUTPUT_DIR, f"translated_{img_filename}")
    img.save(out_img_path)
    print(f"✅ Translated story image saved to {out_img_path}")

    # Save JSON metadata for lightweight client rendering
    out_json_path = os.path.join(OUTPUT_DIR, f"overlay_{img_filename}.json")
    with open(out_json_path, 'w', encoding='utf-8') as f:
        json.dump(overlay_metadata, f, ensure_ascii=False, indent=2)
    print(f"✅ Lightweight overlay metadata saved to {out_json_path}")

def main():
    print("=== Story Image Translation Pipeline ===")
    
    # Story CG 1: manual01.png
    jp_manual01 = [
        "マニュアル 01: 魔法少女の基本操作と戦闘",
        "クリックして魔法少女を攻撃しよう！ダメージを与えるとコインを獲得できます。"
    ]
    boxes_manual01 = [
        {"box": [100, 80, 800, 50]},
        {"box": [100, 950, 1240, 60]}
    ]
    process_story_image("manual01.png", jp_manual01, boxes_manual01)

    # Story CG 2: Op1.png
    jp_op1 = [
        "オープニング: 邪悪な魔力が世界を包み込む…",
        "魔法少女たちは暗闇に堕ちていくのだった。"
    ]
    boxes_op1 = [
        {"box": [80, 60, 900, 55]},
        {"box": [80, 880, 1100, 65]}
    ]
    process_story_image("Op1.png", jp_op1, boxes_op1)

if __name__ == "__main__":
    main()
