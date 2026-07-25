# NST Lua Plugin API & Framework Documentation

## Overview
NST (New Translation System) supports Lua scripts to extend translation services, text processors, and tool integrations.

For detailed guidelines on writing LLM (Large Language Model) translation plugins, see [LLM Plugin Specification](file:///home/jop/work/NST/docs/LLM_PLUGIN_SPEC.md).

---

## Plugin Life Cycle & Hooks

### `on_define_settings()`
Defines configuration options exposed in the NST GUI.

```lua
function on_define_settings()
    return {
        {
            key = "api_key",
            label = "API Key",
            type = "password", -- "password", "string", "dropdown"
            default = ""
        },
        {
            key = "model",
            label = "Model Name",
            type = "dropdown",
            default = "gpt-4o-mini",
            options = { "gpt-4o-mini", "gpt-4o" }
        }
    }
end
```

### `on_text_extract(text)`
Translates a single string.
- **Parameters**: `text` (string)
- **Returns**: `(translated_text, error_message)`
  - On Success: `return result, nil`
  - On Error: `return nil, "Error description"`

### `on_batch_text_extract(text_array)`
Translates a table of strings in batch mode.
- **Parameters**: `text_array` (table/list of strings)
- **Returns**: `(translated_array, error_message)`
  - On Success: `return table_result, nil`
  - On Error: `return nil, "Error description"`

### `on_install()`
Called when the user installs the plugin via Plugin Manager.

### `get_menu_items()` & `on_menu_click()`
Used for custom menu entries in NST.

---

## Global Runtime Injected API Functions

### `nst_log(message)`
Logs a message to the NST application log.
```lua
nst_log("[INFO] Processing request...")
```

### `nst_get_setting(key)`
Retrieves a setting value configured by the user.
```lua
local api_key = nst_get_setting("api_key")
```

### `nst_http_request(url, method, headers_table, body_string)`
Performs a synchronous HTTP network request.
- **Returns**: `response_body` (string), `status_code` (number), `response_headers` (table)

```lua
local body, status, headers = nst_http_request(
    "https://api.openai.com/v1/chat/completions",
    "POST",
    { ["Authorization"] = "Bearer " .. api_key, ["Content-Type"] = "application/json" },
    payload_json
)
```

### `nst_sleep(milliseconds)`
Pauses worker execution for the specified milliseconds without blocking the UI main thread.
```lua
nst_sleep(2000)
```

### `nst_json_encode(lua_table)`
Encodes a Lua table into a JSON string.
```lua
local json_str = nst_json_encode({ model = "gpt-4o-mini", temperature = 0.3 })
```

### `nst_json_decode(json_string)`
Decodes a JSON string into a Lua table.
```lua
local data = nst_json_decode(json_str)
```
