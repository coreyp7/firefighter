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
    fire->active = true;
}

// TODO: this function is okay, but logic should be entirely based off health.
// If the health is greater than 35% of their health vs max_health then it is
// considered alive. Do the math.
// Get rid of the "active" state in the struct, we don't need it.
bool is_fire_alive(Fire *fire) {
    float health_percentage_relative = fire->health / fire->max_health;
    return health_percentage_relative >= 0.3;
}

void add_fire_neighbor_one_way(Fire *fire, Fire *neighbor){
    if(fire->neighbors_size > MAX_NEIGHBORS){
        SDL_Log("Cannot add neighor to fire %p, at max neighbors.\n", &fire);
        return;
    }

    // TODO: we should really check if they're already neighbors from
    // either direction of the relationship.
    fire->neighbors[fire->neighbors_size] = neighbor;
    fire->neighbors_size += 1;

}

void add_fire_neighbor(Fire *fire, Fire *neighbor){
    add_fire_neighbor_one_way(fire, neighbor);
    add_fire_neighbor_one_way(neighbor, fire);
    // if(fire->neighbors_size > MAX_NEIGHBORS){
    //     SDL_Log("Cannot add neighor to fire %p, at max neighbors.\n", &fire);
    //     return;
    // }
    //
    // // TODO: we should really check if they're already neighbors from
    // // either direction of the relationship.
    // fire->neighbors[fire->neighbors_size] = neighbor;
    // fire->neighbors_size += 1;
    //
    //
    // if(neighbor->neighbors_size > MAX_NEIGHBORS){
    //     SDL_Log("Cannot add neighor to fire %p, at max neighbors.\n", &neighbor);
    //     return;
    // }
    //
    // // TODO: we should really check if they're already neighbors from
    // // either direction of the relationship.
    // neighbor->neighbors[neighbor->neighbors_size] = fire;
    // neighbor->neighbors_size += 1;

}
