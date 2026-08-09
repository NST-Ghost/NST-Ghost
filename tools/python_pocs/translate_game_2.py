import os
import sys
import json
import re
import shutil
import time
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor

GAME_DIR = "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー"
DATA_DIR = os.path.join(GAME_DIR, "data")
BACKUP_DIR = os.path.join(GAME_DIR, "data_backup")
SERVER_URL = "http://localhost:8080/api/v1/translate"

JP_REGEX = re.compile(r'[\u3040-\u309F\u30A0-\u30FF\u4E00-\u9FAF]')

def contains_japanese(text):
    if not isinstance(text, str):
        return False
    return bool(JP_REGEX.search(text))

def fetch_from_go_server(text, retries=3):
    if not text or not text.strip():
        return text
    
    payload = json.dumps({"text": text}).encode('utf-8')
    req = urllib.request.Request(SERVER_URL, data=payload, headers={'Content-Type': 'application/json'})
    
    for attempt in range(retries):
        try:
            res = urllib.request.urlopen(req, timeout=10).read().decode('utf-8')
            data = json.loads(res)
            if data.get("status") == "success":
                return data.get("translated", text)
        except Exception:
            time.sleep(0.3 * (attempt + 1))
            
    return text

def collect_jp_strings(data, results):
    if isinstance(data, str):
        if contains_japanese(data) and data not in results:
            results.append(data)
    elif isinstance(data, list):
        for item in data:
            collect_jp_strings(item, results)
    elif isinstance(data, dict):
        for k, v in data.items():
            if k in ['battlerName', 'characterName', 'faceName', 'bgmName', 'bgsName', 'meName', 'seName', 'note']:
                continue
            collect_jp_strings(v, results)

def replace_jp_strings(data, trans_map):
    if isinstance(data, str):
        if data in trans_map:
            return trans_map[data]
        return data
    elif isinstance(data, list):
        return [replace_jp_strings(item, trans_map) for item in data]
    elif isinstance(data, dict):
        new_dict = {}
        for k, v in data.items():
            if k in ['battlerName', 'characterName', 'faceName', 'bgmName', 'bgsName', 'meName', 'seName', 'note']:
                new_dict[k] = v
            else:
                new_dict[k] = replace_jp_strings(v, trans_map)
        return new_dict
    return data

def main():
    print("==================================================")
    print("🚀 Translating Game 2 via NST Go Translation Server")
    print("==================================================")
    print(f"Target Game Directory: {GAME_DIR}")
    print(f"Go Server Endpoint: {SERVER_URL}")
    
    if not os.path.exists(BACKUP_DIR):
        print(f"Creating backup of data directory -> {BACKUP_DIR}...")
        shutil.copytree(DATA_DIR, BACKUP_DIR)
        print("Backup complete.")

    json_files = [f for f in os.listdir(DATA_DIR) if f.endswith('.json')]
    print(f"Found {len(json_files)} JSON files in data directory.")

    all_jp_strings = []
    file_data_map = {}

    for fname in sorted(json_files):
        fpath = os.path.join(DATA_DIR, fname)
        with open(fpath, 'r', encoding='utf-8') as f:
            content = json.load(f)
            file_data_map[fname] = content
            collect_jp_strings(content, all_jp_strings)

    unique_jp = list(set(all_jp_strings))
    print(f"Total Japanese strings found: {len(all_jp_strings)}")
    print(f"Unique Japanese strings: {len(unique_jp)}")

    trans_map = {}
    print(f"Translating {len(unique_jp)} strings through Go Server (20 workers)...")
    done_count = 0
    total_needed = len(unique_jp)
    start_time = time.time()

    with ThreadPoolExecutor(max_workers=20) as executor:
        future_to_text = {executor.submit(fetch_from_go_server, text): text for text in unique_jp}
        for future in future_to_text:
            orig_text = future_to_text[future]
            try:
                res = future.result()
            except Exception:
                res = orig_text
            trans_map[orig_text] = res
            done_count += 1
            if done_count % 250 == 0 or done_count == total_needed:
                elapsed = time.time() - start_time
                speed = done_count / elapsed if elapsed > 0 else 0
                print(f"Go Server Progress: [{done_count}/{total_needed}] ({done_count*100/total_needed:.1f}%) - {speed:.1f} str/s")

    print(f"Translation finished in {time.time() - start_time:.2f} seconds.")

    print("Writing translated JSON files...")
    for fname, data in file_data_map.items():
        translated_data = replace_jp_strings(data, trans_map)
        fpath = os.path.join(DATA_DIR, fname)
        with open(fpath, 'w', encoding='utf-8') as f:
            json.dump(translated_data, f, ensure_ascii=False, indent=2)

    print("🎉 ALL GAME DATA FILES TRANSLATED SUCCESSFULLY!")

if __name__ == "__main__":
    main()
