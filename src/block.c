#include "block.h"

void init_block(Block *block, float x, float y, float w, float h) {
    block->x = x;
    block->y = y;
    block->w = w;
    block->h = h;
    block->active = true;
}
