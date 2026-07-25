// Type definitions for mnemonist
// Project: mnemonist
// Definitions by: Assistant

declare module 'mnemonist/lru-cache' {
  export default class LRUCache<K, V> {
    constructor(capacity: number);
    has(key: K): boolean;
    get(key: K): V | undefined;
    set(key: K, value: V): this;
    delete(key: K): boolean;
    clear(): void;
    forEach(callback: (value: V, key: K, cache: this) => void, thisArg?: any): void;
    keys(): IterableIterator<K>;
    values(): IterableIterator<V>;
    entries(): IterableIterator<[K, V]>;
    [Symbol.iterator](): IterableIterator<[K, V]>;
  }
}