#ifndef BLOCK_H
#define BLOCK_H

#include <stdbool.h>

typedef struct Block {
    float x;
    float y;
    float w;
    float h;
    // TODO: add a field here that indicates if the Block is active.
    // Blocks are stored in a pool allocator so we won't know if a
    // block is alive or not unless this exists.
    // Add code to update this when a Block is allocated and when
    // it is freed from the pool.
} Block;

void init_block(Block *block, float x, float y, float w, float h);

#endif
