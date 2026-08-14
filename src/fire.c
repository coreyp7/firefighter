#include "fire.h"

void init_fire(Fire *fire, float x, float y, float w, float h, float health) {
    fire->x = x;
    fire->y = y;
    fire->w = w;
    fire->h = h;
    fire->health = health;
    fire->max_health = health;
    fire->active = true;
    memset(fire->neighbors, 0, sizeof(fire->neighbors));
    fire->neighbors_size = 0;
}

bool is_fire_alive(Fire *fire) {
    return fire->active && fire->health > 0;
}

void add_fire_neighbor(Fire *fire, Fire *neighbor){
    if(fire->neighbors_size > MAX_NEIGHBORS){
        SDL_Log("Cannot add neighor to fire %p, at max neighbors.\n", &fire);
        return;
    }

    fire->neighbors[fire->neighbors_size] = neighbor;
    fire->neighbors_size += 1;
}
