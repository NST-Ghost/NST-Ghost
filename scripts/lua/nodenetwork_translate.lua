-- Name: NodeNetwork PAYG Translator
-- Version: 1.2.0
-- Author: NST Team
-- Description: Multi-format LLM Translation Plugin via NodeNetwork PAYG API (Supports OpenAI, Anthropic, and Google Formats)

local DEFAULT_BASE_URL = "https://payg.nodenetwork.ovh"
local DEFAULT_MODEL = "gemini-3.6-flash"

local NODENETWORK_MODELS = {
    "gemini-3.6-flash",
    "gemini-3.5-flash",
    "gemini-3.1-flash",
    "gemini-3.1-pro",
    "gpt-5.6-luna",
    "gpt-5.6-sol",
    "gpt-5.6-terra",
    "gpt-5.5",
    "gpt-5.5-instant",
    "gpt-5.4",
    "gpt-5.4-mini",
    "gpt-5.4-nano",
    "gpt-5.3-codex",
    "gpt-5.3-codex-spark",
    "claude-sonnet-5",
    "claude-opus-5",
    "claude-opus-4.8",
    "claude-haiku-4.5",
    "claude-fable-5",
    "deepseek-v4-flash",
    "deepseek-v4-pro",
    "glm-5.2",
    "grok-4.5",
    "mimo-v2.5",
    "mimo-v2.5-pro",
    "minimax-m3",
    "qwen-3.7-max",
    "qwen-3.7-plus"
}

function on_define_settings()
    return {
        {
            key = "api_key",
            label = "NodeNetwork API Key",
            type = "password",
            default = ""
        },
        {
            key = "base_url",
            label = "Base URL",
            type = "string",
            default = DEFAULT_BASE_URL
        },
        {
            key = "api_format",
            label = "API Format",
            type = "dropdown",
            default = "OpenAI v1",
            options = {
                "OpenAI v1",
                "Anthropic v1",
                "Google v1beta"
            }
        },
        {
            key = "model",
            label = "Model Name",
            type = "dropdown",
            default = DEFAULT_MODEL,
            options = NODENETWORK_MODELS
        }
    }
end

function on_get_models()
    return NODENETWORK_MODELS
end

function on_fetch_models(api_key, base_url)
    if not base_url or base_url == "" then base_url = DEFAULT_BASE_URL end
    base_url = base_url:gsub("/+$", "")
    
    local url = base_url .. "/v1/models"
    local headers = {}
    if api_key and api_key ~= "" then
        headers["Authorization"] = "Bearer " .. api_key
    end

    local resp_body, status = nst_http_request(url, "GET", headers, "")
    if status == 200 then
        local data = nst_json_decode(resp_body)
        if data and data.data and type(data.data) == "table" then
            local models = {}
            for _, item in ipairs(data.data) do
                if item and item.id then
                    table.insert(models, item.id)
                end
            end
            if #models > 0 then
                return models, nil
            end
        end
    end
    
    -- Fallback to static model list if request fails or key is missing
    return NODENETWORK_MODELS, nil
end

-- Helper: Network request with exponential backoff & Retry-After handling
local function request_with_retry(url, method, headers, body)
    local max_retries = 3
    local backoff_ms = 1000

    for attempt = 1, max_retries + 1 do
        nst_log("[NodeNetwork] Request attempt " .. attempt .. " to " .. url)
        local response_body, status, response_headers = nst_http_request(url, method, headers, body)
        
        if status == 200 then
            return response_body, status, nil
        elseif status == 429 then
            nst_log("[NodeNetwork] Rate limit (429) hit. Waiting before retry...")
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
            return response_body, status, "HTTP Error Status: " .. tostring(status) .. " - " .. (response_body or "nil")
        end
    end
    return nil, 0, "Max retries exceeded"
end

-- Helper: Extract and validate JSON array from LLM response
local function parse_and_validate_json_array(content, expected_length)
    if not content or content == "" then return nil end
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
        if #translated_array == expected_length then
            return translated_array
        else
            nst_log("[NodeNetwork] Batch length mismatch: expected " .. expected_length .. ", got " .. #translated_array)
        end
    else
        nst_log("[NodeNetwork] JSON decode failed for batch content: " .. tostring(content))
    end
    return nil
end

local function sanitize_base_url(base_url)
    if not base_url or base_url == "" then base_url = DEFAULT_BASE_URL end
    base_url = base_url:gsub("/+$", "")
    return base_url
end

local function get_openai_url(base_url)
    base_url = sanitize_base_url(base_url)
    if base_url:sub(-3) == "/v1" then
        return base_url .. "/chat/completions"
    else
        return base_url .. "/v1/chat/completions"
    end
end

local function get_anthropic_url(base_url)
    base_url = sanitize_base_url(base_url)
    if base_url:sub(-3) == "/v1" then
        return base_url .. "/messages"
    else
        return base_url .. "/v1/messages"
    end
end

local function get_google_url(base_url, model, api_key)
    base_url = sanitize_base_url(base_url)
    if base_url:sub(-7) == "/v1beta" then
        return base_url .. "/models/" .. model .. ":generateContent?key=" .. api_key
    else
        return base_url .. "/v1beta/models/" .. model .. ":generateContent?key=" .. api_key
    end
end

-- ==================== SINGLE TRANSLATION ====================

local function translate_openai(base_url, api_key, model, text)
    local url = get_openai_url(base_url)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    local payload = {
        model = model,
        messages = {
            {
                role = "system",
                content = "You are a professional game translator. Translate text to Thai. Keep control codes, tags, and formatting intact. Return ONLY translated text."
            },
            { role = "user", content = text }
        },
        temperature = 0.3
    }
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.choices and resp.choices[1] and resp.choices[1].message then
        local result = resp.choices[1].message.content:gsub("^%s*(.-)%s*$", "%1")
        return result, nil
    end
    return nil, "Failed to parse OpenAI format response"
end

local function translate_anthropic(base_url, api_key, model, text)
    local url = get_anthropic_url(base_url)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key,
        ["x-api-key"] = api_key,
        ["anthropic-version"] = "2023-06-01"
    }
    local payload = {
        model = model,
        max_tokens = 2048,
        system = "You are a professional game translator. Translate text to Thai. Keep control codes, tags, and formatting intact. Return ONLY translated text.",
        messages = {
            { role = "user", content = text }
        }
    }

    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.content and resp.content[1] and resp.content[1].text then
        local result = resp.content[1].text:gsub("^%s*(.-)%s*$", "%1")
        return result, nil
    end
    return nil, "Failed to parse Anthropic format response"
end

local function translate_google(base_url, api_key, model, text)
    local url = get_google_url(base_url, model, api_key)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    local payload = {
        system_instruction = {
            parts = { { text = "You are a professional game translator. Translate text to Thai. Keep control codes, tags, and formatting intact. Return ONLY translated text." } }
        },
        contents = {
            { parts = { { text = text } } }
        }
    }

    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.candidates and resp.candidates[1] and resp.candidates[1].content and resp.candidates[1].content.parts and resp.candidates[1].content.parts[1] then
        local result = resp.candidates[1].content.parts[1].text:gsub("^%s*(.-)%s*$", "%1")
        return result, nil
    end
    return nil, "Failed to parse Google format response"
end

function on_text_extract(text)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local format = nst_get_setting("api_format") or "OpenAI v1"
    local model = nst_get_setting("model") or DEFAULT_MODEL
    
    if not api_key or api_key == "" then
        return nil, "NodeNetwork API Key is not configured."
    end

    local res, err
    if format == "Anthropic v1" then
        res, err = translate_anthropic(base_url, api_key, model, text)
    elseif format == "Google v1beta" then
        res, err = translate_google(base_url, api_key, model, text)
    else
        res, err = translate_openai(base_url, api_key, model, text)
    end

    -- Automatic Fallback to OpenAI format if Anthropic/Google format fails
    if not res and format ~= "OpenAI v1" then
        nst_log("[NodeNetwork] Primary format '" .. format .. "' failed (" .. tostring(err) .. "). Retrying with OpenAI v1 format...")
        res, err = translate_openai(base_url, api_key, model, text)
    end

    return res, err
end

-- ==================== BATCH TRANSLATION ====================

local function batch_translate_openai(base_url, api_key, model, text_array)
    local url = get_openai_url(base_url)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    local payload = {
        model = model,
        messages = {
            {
                role = "system",
                content = "Translate the given JSON array of strings to Thai.\nIMPORTANT:\n- Output MUST be a valid JSON array of strings.\n- Length MUST match the input exactly.\n- Keep all tags, control codes, and variables intact.\n- Return ONLY the JSON array."
            },
            { role = "user", content = nst_json_encode(text_array) }
        },
        temperature = 0.3
    }
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.choices and resp.choices[1] and resp.choices[1].message then
        local result_array = parse_and_validate_json_array(resp.choices[1].message.content, #text_array)
        if result_array then return result_array, nil end
        return nil, "OpenAI batch response length mismatch or invalid JSON"
    end
    return nil, "Failed to parse OpenAI batch response"
end

local function batch_translate_anthropic(base_url, api_key, model, text_array)
    local url = get_anthropic_url(base_url)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key,
        ["x-api-key"] = api_key,
        ["anthropic-version"] = "2023-06-01"
    }
    local payload = {
        model = model,
        max_tokens = 4096,
        system = "Translate the given JSON array of strings to Thai.\nIMPORTANT:\n- Output MUST be a valid JSON array of strings.\n- Length MUST match the input exactly.\n- Keep all tags, control codes, and variables intact.\n- Return ONLY the JSON array.",
        messages = {
            { role = "user", content = nst_json_encode(text_array) }
        }
    }

    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.content and resp.content[1] and resp.content[1].text then
        local result_array = parse_and_validate_json_array(resp.content[1].text, #text_array)
        if result_array then return result_array, nil end
        return nil, "Anthropic batch response length mismatch or invalid JSON"
    end
    return nil, "Failed to parse Anthropic batch response"
end

local function batch_translate_google(base_url, api_key, model, text_array)
    local url = get_google_url(base_url, model, api_key)
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    local payload = {
        system_instruction = {
            parts = { { text = "Translate the given JSON array of strings to Thai.\nIMPORTANT:\n- Output MUST be a valid JSON array of strings.\n- Length MUST match the input exactly.\n- Keep all tags, control codes, and variables intact.\n- Return ONLY the JSON array." } }
        },
        contents = {
            { parts = { { text = nst_json_encode(text_array) } } }
        }
    }

    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end

    local resp = nst_json_decode(response_body)
    if resp and resp.candidates and resp.candidates[1] and resp.candidates[1].content and resp.candidates[1].content.parts and resp.candidates[1].content.parts[1] then
        local result_array = parse_and_validate_json_array(resp.candidates[1].content.parts[1].text, #text_array)
        if result_array then return result_array, nil end
        return nil, "Google batch response length mismatch or invalid JSON"
    end
    return nil, "Failed to parse Google batch response"
end

function on_batch_text_extract(text_array)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local format = nst_get_setting("api_format") or "OpenAI v1"
    local model = nst_get_setting("model") or DEFAULT_MODEL
    
    if not api_key or api_key == "" then
        return nil, "NodeNetwork API Key is not configured."
    end

    local res, err
    if format == "Anthropic v1" then
        res, err = batch_translate_anthropic(base_url, api_key, model, text_array)
    elseif format == "Google v1beta" then
        res, err = batch_translate_google(base_url, api_key, model, text_array)
    else
        res, err = batch_translate_openai(base_url, api_key, model, text_array)
    end

    -- Automatic Fallback to OpenAI format if Anthropic/Google batch format fails
    if not res and format ~= "OpenAI v1" then
        nst_log("[NodeNetwork] Primary batch format '" .. format .. "' failed (" .. tostring(err) .. "). Retrying with OpenAI v1 batch format...")
        res, err = batch_translate_openai(base_url, api_key, model, text_array)
    end

    return res, err
end
