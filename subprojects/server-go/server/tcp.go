package server

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"strings"
	"sync"

	"nst-server-go/cache"
	"nst-server-go/masker"
	"nst-server-go/translator"
)

type TCPServer struct {
	port       int
	translator *translator.Client
	cache      *cache.MemoryCache
	listener   net.Listener
	mu         sync.Mutex
	running    bool
}

func NewTCPServer(port int, tr *translator.Client, ca *cache.MemoryCache) *TCPServer {
	return &TCPServer{
		port:       port,
		translator: tr,
		cache:      ca,
	}
}

// Request/Response protocol struct for JSON payloads
type JSONRequest struct {
	Text string `json:"text"`
}

type JSONResponse struct {
	Original   string `json:"original"`
	Translated string `json:"translated"`
	Cached     bool   `json:"cached"`
}

func (s *TCPServer) Start(ctx context.Context) error {
	addr := fmt.Sprintf("0.0.0.0:%d", s.port)
	l, err := net.Listen("tcp", addr)
	if err != nil {
		return fmt.Errorf("failed to listen on %s: %w", addr, err)
	}

	s.listener = l
	s.running = true
	log.Printf("[TCP Server] Listening on %s...", addr)

	go func() {
		<-ctx.Done()
		s.Stop()
	}()

	for s.running {
		conn, err := l.Accept()
		if err != nil {
			if !s.running {
				break
			}
			log.Printf("[TCP Server] Accept error: %v", err)
			continue
		}

		go s.handleConnection(ctx, conn)
	}

	return nil
}

func (s *TCPServer) Stop() {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.running {
		s.running = false
		if s.listener != nil {
			s.listener.Close()
		}
		log.Println("[TCP Server] Stopped.")
	}
}

func (s *TCPServer) handleConnection(ctx context.Context, conn net.Conn) {
	defer conn.Close()
	scanner := bufio.NewScanner(conn)

	for scanner.Scan() {
		rawInput := strings.TrimSpace(scanner.Text())
		if rawInput == "" {
			continue
		}

		sourceText := rawInput
		isJSON := false

		// Check if payload is JSON
		var req JSONRequest
		if err := json.Unmarshal([]byte(rawInput), &req); err == nil && req.Text != "" {
			sourceText = req.Text
			isJSON = true
		}

		// 1. Check Cache
		if cachedTranslation, found := s.cache.Get(sourceText); found {
			s.sendReply(conn, sourceText, cachedTranslation, isJSON, true)
			continue
		}

		// 2. Control Code Masking
		maskRes := masker.Mask(sourceText)

		// 3. Request Translation from Custom Backend
		translatedMasked, err := s.translator.Translate(ctx, maskRes.MaskedText)
		if err != nil {
			log.Printf("[TCP Server] Translation error for '%s': %v", sourceText, err)
			s.sendReply(conn, sourceText, sourceText, isJSON, false) // fallback to original
			continue
		}

		// 4. Unmask Control Codes
		finalText := masker.Unmask(translatedMasked, maskRes.TagMap)

		// 5. Store in Cache
		s.cache.Set(sourceText, finalText)

		// 6. Send Response
		s.sendReply(conn, sourceText, finalText, isJSON, false)
	}
}

func (s *TCPServer) sendReply(conn net.Conn, original, translated string, isJSON bool, fromCache bool) {
	if isJSON {
		resp := JSONResponse{
			Original:   original,
			Translated: translated,
			Cached:     fromCache,
		}
		data, _ := json.Marshal(resp)
		conn.Write(append(data, '\n'))
	} else {
		// Plain text protocol (as in standard NST TCP server)
		conn.Write([]byte(translated + "\n"))
	}
}
