package cache

import (
	"sync"
)

type MemoryCache struct {
	mu    sync.RWMutex
	store map[string]string
}

func NewMemoryCache() *MemoryCache {
	return &MemoryCache{
		store: make(map[string]string),
	}
}

func (c *MemoryCache) Get(key string) (string, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	val, ok := c.store[key]
	return val, ok
}

func (c *MemoryCache) Set(key, value string) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.store[key] = value
}
