# LLM Translation Plugin Specification & Guidelines (Lua)

This document defines the strict specification and guidelines for developing **LLM (Large Language Model) Translation Plugins** in NST using Lua.

---

## 🔴 Section 1: Interface & Metadata Rules

* **Rule 1.1 (MUST)**: Every plugin file MUST include a Metadata Header as comments at the top of the file:
  ```lua
  -- Name: <Provider/Model Name>
  -- Version: <Plugin Version, e.g., 1.0.0>
  -- Author: <Author Name>
  -- Description: <Short Description>
  ```
* **Rule 1.2 (MUST)**: The plugin MUST implement `on_define_settings()` returning a table of configurable UI settings.
* **Rule 1.3 (MUST)**: API Key fields in `on_define_settings()` MUST use `type = "password"` to prevent plaintext exposure on GUI displays.
* **Rule 1.4 (MUST)**: Plugins MUST specify their default `base_url` and supported `model` choices inside `on_define_settings()` or export `on_get_models()`.
* **Rule 1.5 (SHOULD)**: Plugins MAY implement `on_get_models()` returning a table of supported model names to allow dynamic model selection in the NST UI.

---

## 🔴 Section 2: Translation Contract & Return Values

* **Rule 2.1 (MUST)**: `on_text_extract(text)` MUST return exactly **2 values**: `(result_text, error_message)`.
  - Success: `return translated_string, nil`
  - Failure: `return nil, error_message_string`
  - ❌ MUST NOT return a single value or `nil` without an error string on failure.
* **Rule 2.2 (MUST)**: `on_batch_text_extract(text_array)` MUST return exactly **2 values**: `(result_array, error_message)`.
  - Success: `return translated_table, nil`
  - Failure: `return nil, error_message_string`
* **Rule 2.3 (MUST - CRITICAL)**: `on_batch_text_extract(text_array)` MUST support ALL API formats and models selectable in the plugin's settings. A plugin MUST NOT break or partially support batch translation for specific selectable API formats.

---

## 🔴 Section 3: Batch Processing & Data Integrity Rules

* **Rule 3.1 (MUST - CRITICAL)**: `on_batch_text_extract` MUST validate that the returned array length matches the input array length (`#translated_array == #text_array`).
  - If array length does not match, return `nil, "Batch size mismatch: expected X, got Y"`.
* **Rule 3.2 (MUST)**: Markdown sanitization MUST be performed on LLM responses prior to JSON decoding:
  - Strip markdown code blocks (` ```json ... ``` `).
  - Extract inner JSON array bounds using pattern matching (`%[.*%]`).
* **Rule 3.3 (MUST)**: Translated string outputs MUST be whitespace-trimmed (`translated_text:gsub("^%s*(.-)%s*$", "%1")`).

---

## 🔴 Section 4: Network & Rate Limiting Rules

* **Rule 4.1 (MUST)**: All HTTP communication MUST use the standard injected `nst_http_request(url, method, headers, body)` function.
* **Rule 4.2 (MUST)**: Retry logic with Exponential Backoff MUST be implemented to handle HTTP Status Codes `429` (Rate Limit) and `5xx` (Server Error).
* **Rule 4.3 (MUST)**: If `Retry-After` is present in response headers, parse the value and delay execution using `nst_sleep(ms)`.
* **Rule 4.4 (MUST)**: A `max_retries` guard (recommended 3–5) MUST be enforced to prevent infinite loops.

---

## 🔴 Section 5: Prompt Engineering Rules

* **Rule 5.1 (MUST)**: System prompts MUST strictly instruct the LLM to preserve control codes, formatting tags, and variables:
  - Control characters: `\n`, `\r`, `\t`
  - Variables: `{0}`, `{name}`, `%s`, `%d`
  - Game Engine codes: `\p[1]`, `\c[2]`, `\v[10]`, `<color=...>`
* **Rule 5.2 (MUST)**: Single translation prompts MUST instruct the LLM to return ONLY the translation result without introductory text, quotes, or markdown wrappers.
* **Rule 5.3 (MUST)**: Batch translation prompts MUST instruct the LLM to return ONLY a valid JSON Array of strings matching the input length.

---

## 🔴 Section 6: Security & Logging Rules

* **Rule 6.1 (MUST - CRITICAL)**: Plugins MUST NEVER log API keys in plaintext via `nst_log`. Always mask keys (e.g., `api_key: set` or `api_key: missing`).
* **Rule 6.2 (SHOULD)**: Log messages SHOULD be prefixed with severity tags: `[DEBUG]`, `[INFO]`, `[WARNING]`, `[ERROR]`.
