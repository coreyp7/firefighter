#include "fire.h"

void init_fire(Fire *fire, float x, float y, float w, float h, float health) {
    fire->x = x;
    fire->y = y;
    fire->w = w;
    fire->h = h;
    fire->health = health;
    fire->max_health = health;
    fire->active = true;
}

bool is_fire_alive(Fire *fire) {
    return fire->active && fire->health > 0;
}
