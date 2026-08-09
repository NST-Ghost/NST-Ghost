import os
import subprocess
from PIL import Image, ImageChops

BADGE_PATH = "/home/jop/.gemini/antigravity-cli/brain/9ac74bb7-3cbc-49bc-aa5a-9d932b699fb9/thai_translated_badge_1786233905939.jpg"
GAME_TITLE_DIR = "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー/img/titles1"
TARGET_COVER = os.path.join(GAME_TITLE_DIR, "Mesumon.png")
BACKUP_COVER = os.path.join(GAME_TITLE_DIR, "Mesumon_original_backup.png")
OUTPUT_ARTIFACT_COVER = "/home/jop/.gemini/antigravity-cli/brain/9ac74bb7-3cbc-49bc-aa5a-9d932b699fb9/translated_game_cover.png"

def make_transparent_badge(badge_path):
    badge = Image.open(badge_path).convert("RGBA")
    datas = badge.getdata()

    new_data = []
    # Make outer white background pixels transparent
    for item in datas:
        # Check if color is close to white (R, G, B > 235)
        if item[0] > 235 and item[1] > 235 and item[2] > 235:
            new_data.append((255, 255, 255, 0)) # transparent
        else:
            new_data.append(item)

    badge.putdata(new_data)
    return badge

def main():
    print("=== Game Cover Icon Overlay Process ===")
    
    # 1. Backup original cover
    if not os.path.exists(BACKUP_COVER) and os.path.exists(TARGET_COVER):
        print(f"Backing up original cover -> {BACKUP_COVER}")
        Image.open(TARGET_COVER).save(BACKUP_COVER)

    # 2. Process Badge Icon (Remove background & resize)
    print("Processing badge icon (removing white background)...")
    badge_rgba = make_transparent_badge(BADGE_PATH)
    
    # Resize badge
    badge_size = (240, 240)
    badge_resized = badge_rgba.resize(badge_size, Image.Resampling.LANCZOS)

    # 3. Load Target Cover Image
    if not os.path.exists(TARGET_COVER):
        print(f"Target cover not found: {TARGET_COVER}")
        return

    cover = Image.open(TARGET_COVER).convert("RGBA")
    cover_w, cover_h = cover.size
    print(f"Original Game Cover Dimensions: {cover_w}x{cover_h}")

    # Position badge on Top-Right corner (Margin 30px)
    margin = 30
    position = (cover_w - badge_size[0] - margin, margin)
    
    # Paste transparent badge onto game cover
    cover.paste(badge_resized, position, badge_resized)

    # 4. Save modified cover back to game directory & artifacts
    cover_rgb = cover.convert("RGB")
    cover.save(TARGET_COVER)
    cover.save(OUTPUT_ARTIFACT_COVER)
    print(f"✅ Modified game cover saved to {TARGET_COVER}")

    # 5. Push to Android Emulator Storage
    emulator_path = "/sdcard/NSTGames/AkudochiClicker/img/titles1/"
    print("Pushing updated cover to Android Emulator storage via ADB...")
    subprocess.run(["adb", "-s", "emulator-5554", "shell", f"mkdir -p {emulator_path}"])
    subprocess.run(["adb", "-s", "emulator-5554", "push", TARGET_COVER, emulator_path])
    print("🎉 Updated game cover pushed to Android Emulator successfully!")

if __name__ == "__main__":
    main()
