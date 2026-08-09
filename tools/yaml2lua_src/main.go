package main

import (
	"bytes"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"text/template"

	"gopkg.in/yaml.v3"
)

type SettingsConfig struct {
	BaseURL      string   `yaml:"base_url"`
	DefaultModel string   `yaml:"default_model"`
	Models       []string `yaml:"models"`
}

type YAMLPluginConfig struct {
	Name              string         `yaml:"name"`
	Version           string         `yaml:"version"`
	Author            string         `yaml:"author"`
	Description       string         `yaml:"description"`
	Settings          SettingsConfig `yaml:"settings"`
	SystemPrompt      string         `yaml:"system_prompt"`
	BatchSystemPrompt string         `yaml:"batch_system_prompt"`
	Temperature       float64        `yaml:"temperature"`
}

type TemplateData struct {
	Name              string
	Version           string
	Author            string
	Description       string
	DefaultModel      string
	BaseURL           string
	ModelsLua         string
	SystemPrompt      string
	BatchSystemPrompt string
	Temperature       float64
	EnvKey            string
}

const rawLuaTemplate = `-- Name: {{.Name}}
-- Version: {{.Version}}
-- Author: {{.Author}}
-- Description: {{.Description}}

local DEFAULT_MODEL = "{{.DefaultModel}}"
local DEFAULT_BASE_URL = "{{.BaseURL}}"

function on_define_settings()
    return {
        {
            key = "api_key",
            label = "{{.Name}} API Key",
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
            options = {{.ModelsLua}}
        }
    }
end

function on_get_models()
    return {{.ModelsLua}}
end

local function request_with_retry(url, method, headers, body)
    local max_retries = 3
    local backoff_ms = 2000

    for attempt = 1, max_retries + 1 do
        nst_log("[{{.Name}}] Request attempt " .. attempt .. " to " .. url)
        local response_body, status, response_headers = nst_http_request(url, method, headers, body)
        
        if status == 200 then
            return response_body, status, nil
        elseif status == 429 then
            nst_log("[{{.Name}}] Rate limit (429) received. Waiting before retry...")
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
            return response_body, status, "{{.Name}} HTTP Error Status: " .. tostring(status) .. " - " .. (response_body or "nil")
        end
    end
    return nil, 0, "{{.Name}} max retries exceeded"
end

function on_text_extract(text)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("{{.EnvKey}}") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: {{.Name}} API Key is not configured."
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
                content = "{{.SystemPrompt}}"
            },
            { role = "user", content = text }
        },
        temperature = {{.Temperature}}
    }
    
    local response_body, status, err = request_with_retry(url, "POST", headers, nst_json_encode(payload))
    if err then return nil, err end
    
    local response = nst_json_decode(response_body)
    if response and response.choices and response.choices[1] and response.choices[1].message then
        local translated_text = response.choices[1].message.content:gsub("^%s*(.-)%s*$", "%1")
        return translated_text, nil
    end
    
    return nil, "Failed to parse {{.Name}} API response"
end

function on_batch_text_extract(text_array)
    local api_key = nst_get_setting("api_key")
    local base_url = nst_get_setting("base_url") or DEFAULT_BASE_URL
    local model = nst_get_setting("model")
    
    if not api_key or api_key == "" then
        api_key = os.getenv("{{.EnvKey}}") or os.getenv("OPENAI_API_KEY")
    end

    if not api_key or api_key == "" then
        return nil, "Error: {{.Name}} API Key is not configured."
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
                content = "{{.BatchSystemPrompt}}"
            },
            { role = "user", content = nst_json_encode(text_array) }
        },
        temperature = {{.Temperature}}
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
            content = content:gsub("\x60\x60\x60json", ""):gsub("\x60\x60\x60", ""):gsub("^%s*", ""):gsub("%s*$", "")
        end
        
        local translated_array = nst_json_decode(content)
        if type(translated_array) == "table" then
            if #translated_array == #text_array then
                return translated_array, nil
            else
                nst_log("[{{.Name}}] Batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array)
                return nil, "{{.Name}} batch length mismatch: expected " .. #text_array .. ", got " .. #translated_array
            end
        else
            nst_log("[{{.Name}}] JSON decode failed for batch content: " .. tostring(content))
        end
    end
    
    return nil, "Failed to parse {{.Name}} batch response"
end
`

func escapeLuaString(s string) string {
	s = strings.ReplaceAll(s, "\\", "\\\\")
	s = strings.ReplaceAll(s, "\"", "\\\"")
	s = strings.ReplaceAll(s, "\n", "\\n")
	return s
}

func sliceToLuaArray(items []string) string {
	var quoted []string
	for _, item := range items {
		quoted = append(quoted, fmt.Sprintf("\"%s\"", item))
	}
	return "{\n        " + strings.Join(quoted, ",\n        ") + "\n    }"
}

func convertFile(inputPath, outputPath string) error {
	data, err := os.ReadFile(inputPath)
	if err != nil {
		return err
	}

	var cfg YAMLPluginConfig
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return err
	}

	if cfg.Name == "" {
		cfg.Name = "Custom Translator"
	}
	if cfg.Version == "" {
		cfg.Version = "1.0.0"
	}
	if cfg.Author == "" {
		cfg.Author = "Community"
	}
	if cfg.Description == "" {
		cfg.Description = fmt.Sprintf("%s LLM Plugin", cfg.Name)
	}
	if cfg.Settings.BaseURL == "" {
		cfg.Settings.BaseURL = "https://api.openai.com/v1"
	}
	if len(cfg.Settings.Models) == 0 {
		cfg.Settings.Models = []string{"default-model"}
	}
	if cfg.Settings.DefaultModel == "" {
		cfg.Settings.DefaultModel = cfg.Settings.Models[0]
	}
	if cfg.Temperature == 0 {
		cfg.Temperature = 0.3
	}

	if cfg.SystemPrompt == "" {
		cfg.SystemPrompt = "You are a professional game translator. Translate text from Japanese/English to Thai. Keep control codes, tags, and formatting intact. Return ONLY the translated text without explanations or quotes."
	}
	if cfg.BatchSystemPrompt == "" {
		cfg.BatchSystemPrompt = "Translate the given JSON array of strings from Japanese/English to Thai.\\nIMPORTANT:\\n- Output MUST be a valid JSON array of strings.\\n- Length MUST match the input exactly.\\n- Keep tags, control codes, and variables intact.\\n- Maintain story continuity, character names, and pronouns consistently across the dialogue sequence.\\n- Return ONLY the JSON array."
	}

	sysPrompt := escapeLuaString(cfg.SystemPrompt)
	batchPrompt := escapeLuaString(cfg.BatchSystemPrompt)

	envKey := strings.ToUpper(cfg.Name)
	envKey = strings.ReplaceAll(envKey, " ", "_")
	envKey = strings.ReplaceAll(envKey, "-", "_") + "_API_KEY"

	tmpl, err := template.New("lua").Parse(rawLuaTemplate)
	if err != nil {
		return fmt.Errorf("failed to parse template: %w", err)
	}

	tData := TemplateData{
		Name:              cfg.Name,
		Version:           cfg.Version,
		Author:            cfg.Author,
		Description:       cfg.Description,
		DefaultModel:      cfg.Settings.DefaultModel,
		BaseURL:           cfg.Settings.BaseURL,
		ModelsLua:         sliceToLuaArray(cfg.Settings.Models),
		SystemPrompt:      sysPrompt,
		BatchSystemPrompt: batchPrompt,
		Temperature:       cfg.Temperature,
		EnvKey:            envKey,
	}

	var buf bytes.Buffer
	if err := tmpl.Execute(&buf, tData); err != nil {
		return fmt.Errorf("failed to execute template: %w", err)
	}

	if outputPath == "" {
		ext := filepath.Ext(inputPath)
		baseName := filepath.Base(inputPath[:len(inputPath)-len(ext)])
		dirName := filepath.Base(filepath.Dir(inputPath))
		if dirName == baseName {
			outputPath = filepath.Join(filepath.Dir(inputPath), baseName+".lua")
		} else {
			outputPath = filepath.Join(filepath.Dir(inputPath), baseName, baseName+".lua")
		}
	}

	if err := os.MkdirAll(filepath.Dir(outputPath), 0755); err != nil {
		return err
	}

	if err := os.WriteFile(outputPath, buf.Bytes(), 0644); err != nil {
		return err
	}

	fmt.Printf("[yaml2lua-go] Converted: %s -> %s\n", inputPath, outputPath)
	return nil
}

func main() {
	outputFlag := flag.String("o", "", "Output path for generated .lua file")
	flag.Parse()

	if flag.NArg() < 1 {
		fmt.Println("Usage: yaml2lua [-o output.lua] <input.yaml | directory>")
		os.Exit(1)
	}

	inputPath := flag.Arg(0)
	info, err := os.Stat(inputPath)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}

	if info.IsDir() {
		err := filepath.Walk(inputPath, func(path string, info os.FileInfo, err error) error {
			if err != nil {
				return err
			}
			if !info.IsDir() && (strings.HasSuffix(path, ".yaml") || strings.HasSuffix(path, ".yml")) {
				return convertFile(path, "")
			}
			return nil
		})
		if err != nil {
			fmt.Printf("Error walking directory: %v\n", err)
			os.Exit(1)
		}
	} else {
		if err := convertFile(inputPath, *outputFlag); err != nil {
			fmt.Printf("Error converting file: %v\n", err)
			os.Exit(1)
		}
	}
}
