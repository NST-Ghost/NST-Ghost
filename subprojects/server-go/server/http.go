package server

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"nst-server-go/cache"
	"nst-server-go/masker"
	"nst-server-go/translator"
)

type HTTPServer struct {
	port       int
	translator *translator.Client
	cache      *cache.MemoryCache
	server     *http.Server
}

func NewHTTPServer(port int, tr *translator.Client, ca *cache.MemoryCache) *HTTPServer {
	return &HTTPServer{
		port:       port,
		translator: tr,
		cache:      ca,
	}
}

type HTTPTranslateReq struct {
	Text       string `json:"text"`
	SourceLang string `json:"source_lang,omitempty"`
	TargetLang string `json:"target_lang,omitempty"`
}

type HTTPTranslateResp struct {
	Status     string `json:"status"`
	Original   string `json:"original"`
	Translated string `json:"translated"`
	Cached     bool   `json:"cached"`
}

func (s *HTTPServer) Start(ctx context.Context) error {
	mux := http.NewServeMux()
	mux.HandleFunc("/health", s.handleHealth)
	mux.HandleFunc("/api/v1/translate", s.handleTranslate)

	s.server = &http.Server{
		Addr:         fmt.Sprintf("0.0.0.0:%d", s.port),
		Handler:      mux,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 35 * time.Second,
	}

	log.Printf("[HTTP Server] Listening on http://0.0.0.0:%d...", s.port)

	go func() {
		<-ctx.Done()
		s.server.Shutdown(context.Background())
	}()

	if err := s.server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		return err
	}
	return nil
}

func (s *HTTPServer) handleHealth(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	w.Write([]byte(`{"status":"ok"}`))
}

func (s *HTTPServer) handleTranslate(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req HTTPTranslateReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil || req.Text == "" {
		http.Error(w, `{"error":"Invalid request payload"}`, http.StatusBadRequest)
		return
	}

	// 1. Check Cache
	if cached, found := s.cache.Get(req.Text); found {
		s.writeJSON(w, http.StatusOK, HTTPTranslateResp{
			Status:     "success",
			Original:   req.Text,
			Translated: cached,
			Cached:     true,
		})
		return
	}

	// 2. Mask Control Codes
	maskRes := masker.Mask(req.Text)

	// 3. Request Translation from Custom Backend
	translatedMasked, err := s.translator.Translate(r.Context(), maskRes.MaskedText)
	if err != nil {
		log.Printf("[HTTP Server] Translation error: %v", err)
		http.Error(w, fmt.Sprintf(`{"error": %q}`, err.Error()), http.StatusInternalServerError)
		return
	}

	// 4. Unmask Control Codes
	finalText := masker.Unmask(translatedMasked, maskRes.TagMap)

	// 5. Store Cache
	s.cache.Set(req.Text, finalText)

	s.writeJSON(w, http.StatusOK, HTTPTranslateResp{
		Status:     "success",
		Original:   req.Text,
		Translated: finalText,
		Cached:     false,
	})
}

func (s *HTTPServer) writeJSON(w http.ResponseWriter, code int, payload interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(code)
	json.NewEncoder(w).Encode(payload)
}
