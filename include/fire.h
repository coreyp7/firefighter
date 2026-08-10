#ifndef FIRE_H
#define FIRE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define MAX_FIRES 20

typedef struct Fire {
    float x;
    float y;
    float w;
    float h;
    float health;
    float max_health;
    bool active;
} Fire;

void init_fire(Fire *fire, float x, float y, float w, float h, float health);
bool is_fire_alive(Fire *fire);

#endif
