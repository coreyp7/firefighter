#ifndef BLOCK_H
#define BLOCK_H

#include <stdbool.h>

typedef struct Block {
    float x;
    float y;
    float w;
    float h;
    bool active;
} Block;

void init_block(Block *block, float x, float y, float w, float h);

#endif
