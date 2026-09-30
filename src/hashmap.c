#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

// This impl is from my old hashmap in c I wrote back in 2024.
// Changed to be generic for whatever data I want.

// Forward declaration for resize
static void _hashmap_resize(HashMap *map, size_t new_capacity);

// 32-bit integer hash function (from original HashTableInt)
static unsigned int _hash_int_32bit(int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return (unsigned int)x;
}

// Hash functions
unsigned int hashmap_hash_int(const void *key, size_t key_size) {
    if (key_size == sizeof(int)) {
        return _hash_int_32bit(*(const int*)key);
    } else if (key_size == sizeof(unsigned int)) {
        return _hash_int_32bit((int)(*(const unsigned int*)key));
    }
    // Fall back to byte-wise hash for other sizes
    return hashmap_hash_bytes(key, key_size);
}

unsigned int hashmap_hash_bytes(const void *key, size_t key_size) {
    // FNV-1a hash algorithm
    unsigned int hash = 2166136261u;
    const unsigned char *data = (const unsigned char*)key;

    for (size_t i = 0; i < key_size; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }

    return hash;
}

unsigned int hashmap_hash_string(const void *key, size_t key_size) {
    (void)key_size;  // Not used for null-terminated strings
    // FNV-1a hash for strings
    unsigned int hash = 2166136261u;
    const unsigned char *str = (const unsigned char*)key;

    while (*str) {
        hash ^= *str++;
        hash *= 16777619u;
    }

    return hash;
}

bool hashmap_key_equals_default(const void *k1, const void *k2, size_t size) {
    return memcmp(k1, k2, size) == 0;
}

// Lifecycle functions
void hashmap_init(
    HashMap *map,
    size_t initial_capacity,
    size_t key_size,
    size_t value_size,
    unsigned int (*hash_func)(const void*, size_t),
    bool (*key_equals)(const void*, const void*, size_t)
) {
    map->capacity = initial_capacity;
    map->count = 0;
    map->key_size = key_size;
    map->value_size = value_size;
    map->hash_func = hash_func;
    map->key_equals = key_equals;

    // Allocate node array
    map->nodes = calloc(initial_capacity, sizeof(HashMapNode));

    // Allocate data buffer for keys and values
    size_t pair_size = key_size + value_size;
    map->data_buf = malloc(pair_size * initial_capacity);

    // Initialize node pointers into data buffer
    for (size_t i = 0; i < initial_capacity; i++) {
        unsigned char *pair_start = map->data_buf + (i * pair_size);
        map->nodes[i].key = pair_start;
        map->nodes[i].value = pair_start + key_size;
        map->nodes[i].alive = false;
    }
}

void hashmap_destroy(HashMap *map) {
    if (map->nodes) {
        free(map->nodes);
        map->nodes = NULL;
    }
    if (map->data_buf) {
        free(map->data_buf);
        map->data_buf = NULL;
    }
    map->capacity = 0;
    map->count = 0;
}

// Operations
bool hashmap_put(HashMap *map, const void *key, const void *value) {
    // Check load factor, resize if needed
    float load = (float)map->count / (float)map->capacity;
    if (load >= 0.7f) {
        _hashmap_resize(map, map->capacity * 2);
    }

    unsigned int hash = map->hash_func(key, map->key_size);
    size_t index = hash % map->capacity;
    size_t start_index = index;

    do {
        HashMapNode *node = &map->nodes[index];

        // Empty slot or matching key - insert/update here
        if (!node->alive || map->key_equals(node->key, key, map->key_size)) {
            memcpy(node->key, key, map->key_size);
            memcpy(node->value, value, map->value_size);

            if (!node->alive) {
                node->alive = true;
                map->count++;
            }
            return true;
        }

        // Linear probe
        index = (index + 1) % map->capacity;
    } while (index != start_index);

    // Table full (shouldn't happen with resize)
    return false;
}

void* hashmap_get(HashMap *map, const void *key) {
    unsigned int hash = map->hash_func(key, map->key_size);
    size_t index = hash % map->capacity;
    size_t start_index = index;

    do {
        HashMapNode *node = &map->nodes[index];

        if (!node->alive) {
            return NULL;  // Empty slot, key not found
        }

        if (map->key_equals(node->key, key, map->key_size)) {
            return node->value;  // Found it
        }

        index = (index + 1) % map->capacity;
    } while (index != start_index);

    return NULL;  // Not found
}

bool hashmap_remove(HashMap *map, const void *key) {
    unsigned int hash = map->hash_func(key, map->key_size);
    size_t index = hash % map->capacity;
    size_t start_index = index;

    do {
        HashMapNode *node = &map->nodes[index];

        if (!node->alive) {
            return false;  // Empty slot, key not found
        }

        if (map->key_equals(node->key, key, map->key_size)) {
            node->alive = false;  // Mark as deleted (tombstone)
            map->count--;
            return true;
        }

        index = (index + 1) % map->capacity;
    } while (index != start_index);

    return false;  // Not found
}

// Utility functions
size_t hashmap_size(HashMap *map) {
    return map->count;
}

float hashmap_load_factor(HashMap *map) {
    if (map->capacity == 0) {
        return 0.0f;
    }
    return (float)map->count / (float)map->capacity;
}

// Internal resize function
static void _hashmap_resize(HashMap *map, size_t new_capacity) {
    // Save old data
    HashMapNode *old_nodes = map->nodes;
    unsigned char *old_buf = map->data_buf;
    size_t old_capacity = map->capacity;

    // Allocate new arrays
    map->capacity = new_capacity;
    map->count = 0;
    map->nodes = calloc(new_capacity, sizeof(HashMapNode));

    size_t pair_size = map->key_size + map->value_size;
    map->data_buf = malloc(pair_size * new_capacity);

    // Initialize new node pointers
    for (size_t i = 0; i < new_capacity; i++) {
        unsigned char *pair_start = map->data_buf + (i * pair_size);
        map->nodes[i].key = pair_start;
        map->nodes[i].value = pair_start + map->key_size;
        map->nodes[i].alive = false;
    }

    // Rehash all alive entries from old table
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_nodes[i].alive) {
            hashmap_put(map, old_nodes[i].key, old_nodes[i].value);
        }
    }

    // Free old memory
    free(old_nodes);
    free(old_buf);
}
