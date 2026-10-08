#include "fire.h"

void init_fire(Fire *fire, float x, float y, float w, float h, float health) {
    fire->x = x;
    fire->y = y;
    fire->w = w;
    fire->h = h;
    fire->health = health;
    fire->max_health = health;
    //memset(fire->neighbors, 0, sizeof(fire)*MAX_NEIGHBORS);
    // for(int i=0; i<fire->neighbors_size; i++){
    //     fire->neighbors[i].active = false;
    // }
    fire->neighbors_size = 0;
    fire->active = true;
    fire->last_put_out = 0;
    fire->last_hit_with_water = 0;

    fire->emitter.x = x + w / 2.0f;
    fire->emitter.y = y + h / 2.0f;
    fire->emitter.particles_per_frame = 3;
    fire->emitter.spawn_spread = 15.0f;
    fire->emitter.time_since_spawn = 0.0f;
    fire->emitter.spawn_interval = 1.0f / 60.0f / 1.0f;  // 3 particles per frame (we're always 60fps)
}

// TODO: this function is okay, but logic should be entirely based off health.
// If the health is greater than 35% of their health vs max_health then it is
// considered alive. Do the math.
// Get rid of the "active" state in the struct, we don't need it.
bool is_fire_alive(Fire *fire) {
    float health_percentage_relative = fire->health / fire->max_health;
    //return health_percentage_relative >= 0.3;
    //return health_percentage_relative >= 0.75;
    return health_percentage_relative >= 0.35;
}

// Used when loading levels. Makes it easier to setup
// neighbors without duplicates.
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
}
