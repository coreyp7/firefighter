#ifndef FIRE_H
#define FIRE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define MAX_FIRES 20
#define MAX_NEIGHBORS 8

typedef struct Fire {
    float x;
    float y;
    float w;
    float h;
    float health;
    float max_health;
    bool active;
    struct Fire* neighbors[MAX_NEIGHBORS];
    int neighbors_size;
} Fire;

void init_fire(Fire *fire, float x, float y, float w, float h, float health);
bool is_fire_alive(Fire *fire);
void add_fire_neighbor(Fire *fire, Fire *neighbor);

#endif
