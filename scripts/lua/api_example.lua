-- API Example Script for NST Translation Plugin
-- This script demonstrates how to use nst_http_request to call an external API.

-- Metadata
-- Name: API Example Translator
-- Version: 1.0

function on_text_extract(text)
    nst_log("Translating via API: " .. text)

    -- Example: Call a mock API (e.g., httpbin.org) to simulate a translation service
    -- In a real scenario, you would call DeepL, OpenAI, or Google Translate API.
    
    local url = "https://httpbin.org/post"
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer my-fake-token"
    }
    
    -- Construct a JSON body (simple string concatenation for this example)
    -- For complex JSON, consider adding a JSON library to the Lua environment.
    local body = '{"text": "' .. text .. '", "target_lang": "TH"}'
    
    -- Call the API
    -- nst_http_request(url, method, headers, body)
    -- Returns: response_body, status_code
    local response, status = nst_http_request(url, "POST", headers, body)
    
    nst_log("API Status: " .. status)
    
    if status == 200 then
        -- Parse the response to extract the "translation"
        -- Since httpbin returns the data we sent, we'll just mock the result based on input
        -- In a real script, you would parse 'response' (which is a JSON string).
        
        -- Mocking a successful translation for demonstration
        return "[API] " .. text
    else
        nst_log("API Error: " .. response)
        return text -- Fallback to original
    end
end
