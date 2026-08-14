#include "fire.h"

void init_fire(Fire *fire, float x, float y, float w, float h, float health) {
    fire->x = x;
    fire->y = y;
    fire->w = w;
    fire->h = h;
    fire->health = health;
    fire->max_health = health;
    memset(fire->neighbors, 0, sizeof(fire->neighbors));
    fire->neighbors_size = 0;
}

// TODO: this function is okay, but logic should be entirely based off health.
// If the health is greater than 35% of their health vs max_health then it is
// considered alive. Do the math.
// Get rid of the "active" state in the struct, we don't need it.
bool is_fire_alive(Fire *fire) {
    float health_percentage_relative = fire->health / fire->max_health;
    return health_percentage_relative >= 0.3;
}

void add_fire_neighbor(Fire *fire, Fire *neighbor){
    if(fire->neighbors_size > MAX_NEIGHBORS){
        SDL_Log("Cannot add neighor to fire %p, at max neighbors.\n", &fire);
        return;
    }

    fire->neighbors[fire->neighbors_size] = neighbor;
    fire->neighbors_size += 1;
}
