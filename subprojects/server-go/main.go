package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"strconv"
	"syscall"

	"nst-server-go/cache"
	"nst-server-go/server"
	"nst-server-go/translator"
)

func getEnv(key, defaultVal string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultVal
}

func getEnvInt(key string, defaultVal int) int {
	if valStr := os.Getenv(key); valStr != "" {
		if val, err := strconv.Atoi(valStr); err == nil {
			return val
		}
	}
	return defaultVal
}

func main() {
	log.Println("==========================================")
	log.Println("🚀 Starting NST Go Translation Server")
	log.Println("==========================================")

	tcpPort := getEnvInt("TCP_PORT", 14478)
	httpPort := getEnvInt("HTTP_PORT", 8080)

	backendURL := getEnv("BACKEND_URL", "http://my-llm-backend:8000/v1/chat/completions")
	apiKey := getEnv("API_KEY", "")
	modelName := getEnv("LLM_MODEL", "qwen2.5-7b")
	sourceLang := getEnv("SOURCE_LANG", "Japanese")
	targetLang := getEnv("TARGET_LANG", "Thai")

	log.Printf("Backend Config -> URL: %s | Model: %s | %s -> %s", backendURL, modelName, sourceLang, targetLang)

	trClient := translator.NewClient(translator.Config{
		BackendURL: backendURL,
		APIKey:     apiKey,
		Model:      modelName,
		SourceLang: sourceLang,
		TargetLang: targetLang,
	})

	memCache := cache.NewMemoryCache()

	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer cancel()

	tcpServer := server.NewTCPServer(tcpPort, trClient, memCache)
	httpServer := server.NewHTTPServer(httpPort, trClient, memCache)

	go func() {
		if err := tcpServer.Start(ctx); err != nil {
			log.Printf("[TCP Error] %v", err)
		}
	}()

	go func() {
		if err := httpServer.Start(ctx); err != nil {
			log.Printf("[HTTP Error] %v", err)
		}
	}()

	<-ctx.Done()
	log.Println("Shutting down NST Go Translation Server gracefully...")
}
