package translator

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"
)

type Config struct {
	BackendURL string // e.g. "http://my-llm-backend:8000/v1/chat/completions" or custom API
	APIKey     string
	Model      string
	SourceLang string
	TargetLang string
	SystemPrompt string
}

type Client struct {
	cfg        Config
	httpClient *http.Client
}

func NewClient(cfg Config) *Client {
	if cfg.SystemPrompt == "" {
		cfg.SystemPrompt = fmt.Sprintf(
			"You are a professional video game translator. Translate the given text from %s to %s. "+
				"Preserve all special tags like [VAR_0], [VAR_1] exactly as they are without modifying or translating them. "+
				"Return ONLY the translated text without extra explanation.",
			cfg.SourceLang, cfg.TargetLang,
		)
	}

	return &Client{
		cfg: cfg,
		httpClient: &http.Client{
			Timeout: 30 * time.Second,
		},
	}
}

// OpenAI API Request / Response schemas
type openAIRequest struct {
	Model    string          `json:"model"`
	Messages []openAIMessage `json:"messages"`
}

type openAIMessage struct {
	Role    string `json:"role"`
	Content string `json:"content"`
}

type openAIResponse struct {
	Choices []struct {
		Message openAIMessage `json:"message"`
	} `json:"choices"`
	Error *struct {
		Message string `json:"message"`
	} `json:"error,omitempty"`
}

func (c *Client) Translate(ctx context.Context, text string) (string, error) {
	if c.cfg.BackendURL == "google" || strings.HasPrefix(c.cfg.BackendURL, "https://translate.googleapis.com") {
		return c.translateGoogle(ctx, text)
	}

	reqBody := openAIRequest{
		Model: c.cfg.Model,
		Messages: []openAIMessage{
			{Role: "system", Content: c.cfg.SystemPrompt},
			{Role: "user", Content: text},
		},
	}

	jsonData, err := json.Marshal(reqBody)
	if err != nil {
		return "", fmt.Errorf("failed to marshal request: %w", err)
	}

	req, err := http.NewRequestWithContext(ctx, http.MethodPost, c.cfg.BackendURL, bytes.NewBuffer(jsonData))
	if err != nil {
		return "", fmt.Errorf("failed to create http request: %w", err)
	}

	req.Header.Set("Content-Type", "application/json")
	if c.cfg.APIKey != "" {
		req.Header.Set("Authorization", "Bearer "+c.cfg.APIKey)
	}

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return "", fmt.Errorf("http request failed: %w", err)
	}
	defer resp.Body.Close()

	bodyBytes, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", fmt.Errorf("failed to read response body: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("backend returned non-200 status %d: %s", resp.StatusCode, string(bodyBytes))
	}

	var res openAIResponse
	if err := json.Unmarshal(bodyBytes, &res); err != nil {
		return "", fmt.Errorf("failed to unmarshal response: %w", err)
	}

	if res.Error != nil && res.Error.Message != "" {
		return "", fmt.Errorf("backend api error: %s", res.Error.Message)
	}

	if len(res.Choices) == 0 {
		return "", fmt.Errorf("empty choices returned from backend")
	}

	return res.Choices[0].Message.Content, nil
}

func (c *Client) translateGoogle(ctx context.Context, text string) (string, error) {
	sl := "ja"
	tl := "th"
	if c.cfg.SourceLang != "" && c.cfg.SourceLang != "Japanese" {
		sl = c.cfg.SourceLang
	}
	if c.cfg.TargetLang != "" && c.cfg.TargetLang != "Thai" {
		tl = c.cfg.TargetLang
	}

	apiURL := fmt.Sprintf("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%s&tl=%s&dt=t&q=%s",
		sl, tl, url.QueryEscape(text))

	req, err := http.NewRequestWithContext(ctx, http.MethodGet, apiURL, nil)
	if err != nil {
		return "", err
	}
	req.Header.Set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	bodyBytes, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	var rawData []interface{}
	if err := json.Unmarshal(bodyBytes, &rawData); err != nil {
		return "", fmt.Errorf("failed to parse google response: %w", err)
	}

	if len(rawData) > 0 {
		if sentences, ok := rawData[0].([]interface{}); ok {
			var sb strings.Builder
			for _, item := range sentences {
				if tuple, ok := item.([]interface{}); ok && len(tuple) > 0 {
					if translatedChunk, ok := tuple[0].(string); ok {
						sb.WriteString(translatedChunk)
					}
				}
			}
			return sb.String(), nil
		}
	}

	return text, nil
}
