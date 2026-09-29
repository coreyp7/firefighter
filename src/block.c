#include "block.h"

void init_block(Block *block, float x, float y, float w, float h) {
    block->x = x;
    block->y = y;
    block->w = w;
    block->h = h;
    block->active = true;
    block->sprite_id = 0;  // Default to sprite 0
}

void init_block_with_sprite(Block *block, float x, float y, float w, float h, int sprite_id) {
    init_block(block, x, y, w, h);
    block->sprite_id = sprite_id;
}
