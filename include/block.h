#ifndef BLOCK_H
#define BLOCK_H

#include <stdbool.h>

typedef struct Block {
    float x;
    float y;
    float w;
    float h;
    bool active;
    int sprite_id;  // Index into block sprite texture array (0-based)
} Block;

void init_block(Block *block, float x, float y, float w, float h);
void init_block_with_sprite(Block *block, float x, float y, float w, float h, int sprite_id);

#endif
