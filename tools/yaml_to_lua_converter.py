#!/usr/bin/env python3
"""
YAML to Lua Translator Plugin Converter for NST (Novelty Translation Tool)
Converts simple, human-readable .yaml provider definitions into full-featured NST Lua plugin scripts (.lua).
"""

import sys
import os
import argparse
import yaml
import json

TEMPLATE_OPENAI_COMPATIBLE = '''-- Name: {name}
-- Version: {version}
-- Author: {author}
-- Description: {description}

local DEFAULT_MODEL = "{default_model}"
local DEFAULT_BASE_URL = "{base_url}"

function on_define_settings()
    return {{
        {{
            key = "api_key",
            label = "{name} API Key",
            type = "password",
            default = ""
        }},
        {{
            key = "base_url",
            label = "Base URL",
            type = "string",
            default = DEFAULT_BASE_URL
        }},
        {{
            key = "model",
            label = "Model Name",
            type = "dropdown",
            default = DEFAULT_MODEL,
            options = {models_lua}
        }}
    }}
end

function on_get_models()
    return {models_lua}
end

local function request_with_retry(url, method, headers, body)
    local max_retries = 3
    local backoff_ms = 2000

    for attempt = 1, max_retries + 1 do
        nst_log("[{name}] Request attempt " .. attempt .. " to " .. url)
        local response_body, status, response_headers = nst_http_request(url, method, headers, body)
        
        if status == 200 then
            return response_body, status, nil
        elseif status == 429 then
            nst_log("[{name}] Rate limit (429) received. Waiting before retry...")
            local wait_ms = backoff_ms
            if response_headers and type(response_headers) == "table" then
                for k, v in pairs(response_headers) do
                    if string.lower(k) == "retry-after" then
                        local sec = tonumber(v)
                        if sec then wait_ms = sec * 1000 end
                        break
                    end
                end
            end
            if attempt <= max_retries then
                if nst_sleep then nst_sleep(wait_ms) end
                backoff_ms = backoff_ms * 2
            end
        else
            return response_body, status, "{name} HTTP Error Status: " .. tostring(status) .. " - " .. (response_body or "nil")
        end
    end
    return nil, 0, "{name} max retries exceeded"
end

function on_text_extract(text)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("{env_key_name}") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: {name} API Key is not configured."
    end

    if not model or model == "" then model = DEFAULT_MODEL end
    base_url = base_url:gsub("/+$", "")

    local url = base_url .. "/chat/completions"
    local headers = {{
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }}
    
    local payload = {{
        model = model,
        messages = {{
            {{
                role = "system",
                content = "{system_prompt}"
            }},
            {{ role = "user", content = text }}
        }},
        temperature = {temperature}
    }}
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end
    
    local response = nst_json_decode(response_body)
    if response and response.choices and response.choices[1] and response.choices[1].message then
        local translated_text = response.choices[1].message.content:gsub("^%s*(.-)%s*$", "%1")
        return translated_text, nil
    end
    
    return nil, "Failed to parse {name} API response"
end

function on_batch_text_extract(text_array)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("{env_key_name}") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: {name} API Key is not configured."
    end

    if not model or model == "" then model = DEFAULT_MODEL end
    base_url = base_url:gsub("/+$", "")

    local url = base_url .. "/chat/completions"
    local headers = {{
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }}
    
    local payload = {{
        model = model,
        messages = {{
            {{
                role = "system",
                content = "{batch_system_prompt}"
            }},
            {{ role = "user", content = nst_json_encode(text_array) }}
        }},
        temperature = {temperature}
    }}
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local response = nst_json_decode(response_body)
    if response and response.choices and response.choices[1] and response.choices[1].message then
        local content = response.choices[1].message.content
        
        local first_bracket = content:find("%[")
        local last_bracket = content:reverse():find("%]")
        if first_bracket and last_bracket then
            last_bracket = #content - last_bracket + 1
            content = content:sub(first_bracket, last_bracket)
        else
            content = content:gsub("```json", ""):gsub("```", ""):gsub("^%s*", ""):gsub("%s*$", "")
        end
        
        local translated_array = nst_json_decode(content)
        if type(translated_array) == "table" then
            if #translated_array == #text_array then
                return translated_array, nil
            else
                nst_log("[{name}] Batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array)
                return nil, "{name} batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array
            end
        else
            nst_log("[{name}] JSON decode failed for batch content: " .. tostring(content))
        end
    end
    
    return nil, "Failed to parse {name} batch response"
end
'''

def py_list_to_lua(py_list):
    items = [f'"{item}"' for item in py_list]
    return "{\n        " + ",\n        ".join(items) + "\n    }"

def convert_yaml_to_lua(yaml_file_path, output_lua_path=None):
    with open(yaml_file_path, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)

    name = data.get('name', 'Custom Translator')
    version = data.get('version', '1.0.0')
    author = data.get('author', 'Community')
    description = data.get('description', f'{name} LLM Plugin')
    
    settings = data.get('settings', {})
    base_url = settings.get('base_url', 'https://api.openai.com/v1')
    models = settings.get('models', ['default-model'])
    default_model = settings.get('default_model', models[0] if models else 'default-model')
    
    system_prompt = data.get('system_prompt', 
        "You are a professional game translator. Translate text from Japanese/English to Thai. Keep control codes, tags, and formatting intact. Return ONLY the translated text without explanations or quotes.")
    # Escape quotes and backslashes in prompts for Lua string literals
    system_prompt = system_prompt.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    
    batch_system_prompt = data.get('batch_system_prompt', 
        "Translate the given JSON array of strings from Japanese/English to Thai.\\nIMPORTANT:\\n- Output MUST be a valid JSON array of strings.\\n- Length MUST match the input exactly.\\n- Keep tags, control codes, and variables intact.\\n- Return ONLY the JSON array.")
    batch_system_prompt = batch_system_prompt.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

    temperature = data.get('temperature', 0.3)
    env_key_name = name.upper().replace(' ', '_').replace('-', '_') + "_API_KEY"

    models_lua = py_list_to_lua(models)

    lua_code = TEMPLATE_OPENAI_COMPATIBLE.format(
        name=name,
        version=version,
        author=author,
        description=description,
        default_model=default_model,
        base_url=base_url,
        models_lua=models_lua,
        system_prompt=system_prompt,
        batch_system_prompt=batch_system_prompt,
        temperature=temperature,
        env_key_name=env_key_name
    )

    if not output_lua_path:
        filename = os.path.basename(yaml_file_path).replace('.yaml', '.lua').replace('.yml', '.lua')
        output_lua_path = os.path.join(os.path.dirname(yaml_file_path) or '.', filename)

    os.makedirs(os.path.dirname(os.path.abspath(output_lua_path)), exist_ok=True)
    with open(output_lua_path, 'w', encoding='utf-8') as f:
        f.write(lua_code)

    print(f"Successfully converted '{yaml_file_path}' -> '{output_lua_path}'")
    return output_lua_path

def main():
    parser = argparse.ArgumentParser(description="Convert YAML plugin definition to NST Lua plugin script")
    parser.add_argument("input_yaml", help="Path to input .yaml plugin file or directory containing .yaml files")
    parser.add_argument("-o", "--output", help="Path to output .lua file or directory")
    args = parser.parse_args()

    input_path = args.input_yaml
    if os.path.isdir(input_path):
        for root, dirs, files in os.walk(input_path):
            for file in files:
                if file.endswith('.yaml') or file.endswith('.yml'):
                    y_path = os.path.join(root, file)
                    convert_yaml_to_lua(y_path)
    elif os.path.isfile(input_path):
        convert_yaml_to_lua(input_path, args.output)
    else:
        print(f"Error: Path '{input_path}' not found.", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
