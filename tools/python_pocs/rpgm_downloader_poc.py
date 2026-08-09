#!/usr/bin/env python3
"""
Chanomhub RPGM Compressed File Downloader (PoC)
"""

import os
import json
import urllib.request
import urllib.parse
import re

# Configuration
API_GRAPHQL_URL = "https://api.chanomhub.com/api/graphql"
API_BASE_URL = "https://api.chanomhub.com"
OUTPUT_DIR = os.path.abspath("./rpgm_downloads_poc")

# Compressed file extensions priority
COMPRESSED_EXTENSIONS = ('.zip', '.7z', '.rar', '.xz', '.tar.gz', '.gz', '.tar', '.bz2')

# Known direct download host domains (can be expanded)
DIRECT_HOST_KEYWORDS = ('catbox.moe', 'pixeldrain.com', 'gofile.io', 'send.now', 'krakenfiles.com', 'buzzheavier.com')

def fetch_graphql(query: str):
    req = urllib.request.Request(
        API_GRAPHQL_URL,
        data=json.dumps({"query": query}).encode('utf-8'),
        headers={
            "Content-Type": "application/json",
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
        }
    )
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def is_compressed_link(url: str) -> bool:
    url_lower = url.lower()
    return any(url_lower.endswith(ext) or ext + "?" in url_lower for ext in COMPRESSED_EXTENSIONS)

def sanitize_folder_name(name: str) -> str:
    name = re.sub(r'[\\/*?:"<>|]', '', name)
    return name.strip().replace(' ', '_').lower()

def resolve_download_url(raw_url: str) -> str:
    """
    Resolve relative paths or internal storage URLs into full direct HTTP URLs.
    """
    if raw_url.startswith("public/"):
        return f"{API_BASE_URL}/{raw_url}"
    elif raw_url.startswith("/"):
        return f"{API_BASE_URL}{raw_url}"
    return raw_url

def main():
    print("=== Chanomhub RPGM Compressed File Downloader (PoC) ===")
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # 1. Fetch articles
    query_articles = """
    {
      articles(limit: 100, offset: 0) {
        id
        title
        slug
        viewsCount
        engine {
          name
        }
      }
    }
    """
    res = fetch_graphql(query_articles)
    articles = res.get('data', {}).get('articles', [])
    
    # Filter RPGM engine
    rpgm_articles = [a for a in articles if a.get('engine') and 'rpg' in str(a['engine']).lower()]
    rpgm_articles.sort(key=lambda x: x.get('viewsCount', 0), reverse=True)
    
    print(f"Found {len(rpgm_articles)} RPGM articles.")
    
    for article in rpgm_articles:
        aid = article['id']
        title = article['title']
        slug = article['slug']
        
        # 2. Fetch downloads for each article via GraphQL
        query_downloads = f"{{ downloads(articleId: {aid}) {{ id name type url isActive }} }}"
        dl_res = fetch_graphql(query_downloads)
        downloads = dl_res.get('data', {}).get('downloads', [])
        
        # Filter for compressed file links
        compressed_links = [d for d in downloads if is_compressed_link(d.get('url', ''))]
        
        folder_name = sanitize_folder_name(title)
        target_dir = os.path.join(OUTPUT_DIR, folder_name)
        os.makedirs(target_dir, exist_ok=True)
        
        # Save metadata
        meta_path = os.path.join(target_dir, "metadata.json")
        with open(meta_path, "w", encoding="utf-8") as f:
            json.dump({
                "article": article,
                "all_downloads": downloads,
                "compressed_downloads": compressed_links
            }, f, indent=2, ensure_ascii=False)
            
        print(f"\n📁 Processing: {title} (ID: {aid}) -> {target_dir}")
        
        if not compressed_links:
            print("   ⚠️  No direct compressed file links found in API.")
            continue
            
        # Download compressed files
        for item in compressed_links:
            raw_url = item.get('url', '')
            final_url = resolve_download_url(raw_url)
            filename = os.path.basename(urllib.parse.urlparse(final_url).path) or f"download_{item['id']}.zip"
            dest_file = os.path.join(target_dir, filename)
            
            print(f"   ⬇️  Downloading: {filename} from {final_url}")
            try:
                req = urllib.request.Request(final_url, headers={"User-Agent": "Mozilla/5.0"})
                with urllib.request.urlopen(req) as resp, open(dest_file, "wb") as out_f:
                    out_f.write(resp.read())
                print(f"   ✅ Downloaded: {dest_file} ({os.path.getsize(dest_file)} bytes)")
            except urllib.error.HTTPError as e:
                print(f"   ❌ HTTP Error {e.code}: Failed to download {final_url} (Internal storage 404 or restricted)")
            except Exception as e:
                print(f"   ❌ Error: {e}")

if __name__ == "__main__":
    main()
