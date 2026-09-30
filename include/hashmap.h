#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct HashMapNode HashMapNode;
struct HashMapNode {
    unsigned char *key;    // Pointer to key data in buffer
    unsigned char *value;  // Pointer to value data in buffer
    bool alive;            // Tombstone flag
};

typedef struct HashMap HashMap;
struct HashMap {
    HashMapNode *nodes;      // Array of node metadata
    unsigned char *data_buf; // Backing buffer for key-value data
    size_t capacity;         // Number of slots
    size_t count;            // Number of alive entries
    size_t key_size;         // Size of each key in bytes
    size_t value_size;       // Size of each value in bytes

    // Function pointers for operations
    unsigned int (*hash_func)(const void *key, size_t key_size);
    bool (*key_equals)(const void *key1, const void *key2, size_t key_size);
};

// Lifecycle
void hashmap_init(
    HashMap *map,
    size_t initial_capacity,
    size_t key_size,
    size_t value_size,
    unsigned int (*hash_func)(const void*, size_t),
    bool (*key_equals)(const void*, const void*, size_t)
);
void hashmap_destroy(HashMap *map);

// Operations
bool hashmap_put(HashMap *map, const void *key, const void *value);
void* hashmap_get(HashMap *map, const void *key);
bool hashmap_remove(HashMap *map, const void *key);

// Utility
size_t hashmap_size(HashMap *map);
float hashmap_load_factor(HashMap *map);

// Default hash functions
unsigned int hashmap_hash_int(const void *key, size_t key_size);
unsigned int hashmap_hash_string(const void *key, size_t key_size);
unsigned int hashmap_hash_bytes(const void *key, size_t key_size);

// Default key comparison (memcmp)
bool hashmap_key_equals_default(const void *k1, const void *k2, size_t size);

#endif
