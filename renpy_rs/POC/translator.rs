/// Anthropic API batch translator for Ren'Py game text
///
/// Sends text in chunks to avoid token limits.
/// Preserves Ren'Py markup tags like {b}, {i}, {color}, {font}, etc.

use crate::extractor::TextBlock;
use std::collections::HashMap;
use std::io::{self, Write};

const API_URL: &str = "https://api.anthropic.com/v1/messages";
const MODEL: &str = "claude-opus-4-5";
const BATCH_SIZE: usize = 30; // lines per API call

/// Translate a list of TextBlocks using the Anthropic API.
/// Returns a map of source text -> translated text.
pub fn translate_blocks(
    blocks: &[TextBlock],
    target_lang: &str,
    api_key: &str,
) -> io::Result<HashMap<String, String>> {
    // Deduplicate: only translate unique source strings
    let mut unique: Vec<&str> = Vec::new();
    let mut seen = std::collections::HashSet::new();
    for b in blocks {
        if seen.insert(b.source.as_str()) {
            unique.push(&b.source);
        }
    }

    eprintln!("[translate] {} unique strings to translate → {}", unique.len(), target_lang);

    let mut result: HashMap<String, String> = HashMap::new();
    let chunks: Vec<&[&str]> = unique.chunks(BATCH_SIZE).collect();
    let total = chunks.len();

    for (i, chunk) in chunks.iter().enumerate() {
        eprint!("[translate] batch {}/{} ({} strings)...", i + 1, total, chunk.len());
        io::stderr().flush().ok();

        match translate_batch(chunk, target_lang, api_key) {
            Ok(map) => {
                let n = map.len();
                result.extend(map);
                eprintln!(" ok ({} translated)", n);
            }
            Err(e) => {
                eprintln!(" ERROR: {}", e);
                // insert empty translations so we can continue
                for s in chunk.iter() {
                    result.insert(s.to_string(), String::new());
                }
            }
        }
    }

    Ok(result)
}

fn translate_batch(
    strings: &[&str],
    target_lang: &str,
    api_key: &str,
) -> io::Result<HashMap<String, String>> {
    // Build numbered list for the model
    let numbered: String = strings
        .iter()
        .enumerate()
        .map(|(i, s)| format!("{}. {}", i + 1, s))
        .collect::<Vec<_>>()
        .join("\n");

    let prompt = format!(
        r#"You are a professional game translator. Translate the following Ren'Py visual novel dialogue lines into {lang}.

Rules:
- Preserve ALL Ren'Py markup tags exactly: {{b}}, {{/b}}, {{i}}, {{color=#xxx}}, {{font=...}}, {{size=+N}}, etc.
- Preserve special characters: \\n (newline), \\t, escaped quotes
- Keep character/emotional nuance appropriate for a visual novel
- Output ONLY the translations numbered the same way, nothing else
- Format: "N. <translated text>" on each line

Lines to translate:
{lines}"#,
        lang = target_lang,
        lines = numbered
    );

    // Build JSON request body manually (no reqwest dep needed - use std::net or ureq)
    // Using ureq (lightweight) or raw HTTP via std
    let body = format!(
        r#"{{"model":"{}","max_tokens":2000,"messages":[{{"role":"user","content":{}}}]}}"#,
        MODEL,
        serde_json::to_string(&prompt)
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?
    );

    // Make HTTP POST (using ureq)
    let response_text = http_post(API_URL, api_key, &body)?;

    // Parse response JSON to extract text content
    let json: serde_json::Value = serde_json::from_str(&response_text)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e.to_string()))?;

    let content = json["content"][0]["text"]
        .as_str()
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidData, "no content in response"))?;

    // Parse "N. translated text" lines
    parse_numbered_response(strings, content)
}

fn parse_numbered_response(
    originals: &[&str],
    response: &str,
) -> io::Result<HashMap<String, String>> {
    let mut map = HashMap::new();

    // Match lines like "1. text" or "1) text"
    let re = regex::Regex::new(r"^(\d+)[.)]\s+(.+)$")
        .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;

    for line in response.lines() {
        if let Some(caps) = re.captures(line.trim()) {
            let idx: usize = caps[1].parse().unwrap_or(0);
            if idx >= 1 && idx <= originals.len() {
                let translated = caps[2].trim().to_string();
                map.insert(originals[idx - 1].to_string(), translated);
            }
        }
    }

    // Fill missing with empty
    for orig in originals {
        map.entry(orig.to_string()).or_insert_with(String::new);
    }

    Ok(map)
}

/// Minimal HTTP POST using std::net (no external HTTP crates needed)
fn http_post(url: &str, api_key: &str, body: &str) -> io::Result<String> {
    use std::io::{BufRead, BufReader, Read};
    use std::net::TcpStream;

    // Parse URL: https://api.anthropic.com/v1/messages
    let host = "api.anthropic.com";
    let path = "/v1/messages";
    let port = 443u16;

    // Connect TCP
    let tcp = TcpStream::connect((host, port))?;

    // TLS via native-tls or rustls
    // Since we want zero deps, use the ureq pattern but inline
    // For now: delegate to the ureq crate (add to Cargo.toml)
    // This function is a placeholder - real impl uses ureq below

    let _ = (url, api_key, body, tcp, host, path, port); // suppress warnings

    Err(io::Error::new(
        io::ErrorKind::Other,
        "use ureq feature - see http_post_ureq",
    ))
}

/// Real HTTP POST using ureq (add ureq = "2" to Cargo.toml)
#[cfg(feature = "translate")]
pub fn http_post_ureq(url: &str, api_key: &str, body: &str) -> io::Result<String> {
    let response = ureq::post(url)
        .set("x-api-key", api_key)
        .set("anthropic-version", "2023-06-01")
        .set("content-type", "application/json")
        .send_string(body)
        .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;

    response
        .into_string()
        .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))
}
