-- Name: MaxPlus AI Translator
-- Version: 1.0.0
-- Author: NST Team
-- Description: MaxPlus AI LLM Translation Plugin (OpenAI-compatible API at https://api.maxplus-ai.cc)

local DEFAULT_MODEL = "gpt-4o-mini"
local DEFAULT_BASE_URL = "https://api.maxplus-ai.cc/v1"

function on_define_settings()
    return {
        {
            key = "api_key",
            label = "MaxPlus API Key",
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
            key = "model",
            label = "Model Name",
            type = "dropdown",
            default = DEFAULT_MODEL,
            options = {
                "gpt-4o",
                "gpt-4o-mini",
                "claude-3-5-sonnet",
                "deepseek-v3",
                "deepseek-r1"
            }
        }
    }
end

function on_get_models()
    return {
        "gpt-4o",
        "gpt-4o-mini",
        "claude-3-5-sonnet",
        "deepseek-v3",
        "deepseek-r1"
    }
end

-- Helper for API requests with retry logic and rate-limit handling
local function request_with_retry(url, method, headers, body)
    local max_retries = 3
    local backoff_ms = 2000

    for attempt = 1, max_retries + 1 do
        nst_log("[MaxPlus AI] Request attempt " .. attempt .. " to " .. url)
        local response_body, status, response_headers = nst_http_request(url, method, headers, body)
        
        if status == 200 then
            return response_body, status, nil
        elseif status == 429 then
            nst_log("[MaxPlus AI] Rate limit (429) received. Waiting before retry...")
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
            return response_body, status, "MaxPlus AI HTTP Error Status: " .. tostring(status) .. " - " .. (response_body or "nil")
        end
    end
    return nil, 0, "MaxPlus AI max retries exceeded"
end

function on_text_extract(text)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("MAXPLUS_API_KEY") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: MaxPlus API Key is not configured."
    end

    if not model or model == "" then model = DEFAULT_MODEL end
    base_url = base_url:gsub("/+$", "")

    local url = base_url .. "/chat/completions"
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    
    local payload = {
        model = model,
        messages = {
            {
                role = "system",
                content = "You are a professional game translator. Translate text from Japanese/English to Thai. Keep control codes, tags, and formatting intact. Return ONLY the translated text without explanations or quotes."
            },
            { role = "user", content = text }
        },
        temperature = 0.3
    }
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end
    
    local response = nst_json_decode(response_body)
    if response and response.choices and response.choices[1] and response.choices[1].message then
        local translated_text = response.choices[1].message.content:gsub("^%s*(.-)%s*$", "%1")
        return translated_text, nil
    end
    
    return nil, "Failed to parse MaxPlus AI API response"
end

function on_batch_text_extract(text_array)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("MAXPLUS_API_KEY") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: MaxPlus API Key is not configured."
    end

    if not model or model == "" then model = DEFAULT_MODEL end
    base_url = base_url:gsub("/+$", "")

    local url = base_url .. "/chat/completions"
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. api_key
    }
    
    local payload = {
        model = model,
        messages = {
            {
                role = "system",
                content = "Translate the given JSON array of strings from Japanese/English to Thai.\nIMPORTANT:\n- Output MUST be a valid JSON array of strings.\n- Length MUST match the input exactly.\n- Keep tags, control codes, and variables intact.\n- Return ONLY the JSON array."
            },
            { role = "user", content = nst_json_encode(text_array) }
        },
        temperature = 0.3
    }
    
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
                nst_log("[MaxPlus AI] Batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array)
                return nil, "MaxPlus AI batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array
            end
        else
            nst_log("[MaxPlus AI] JSON decode failed for batch content: " .. tostring(content))
        end
    end
    
    return nil, "Failed to parse MaxPlus AI batch response"
end
