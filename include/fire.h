#ifndef FIRE_H
#define FIRE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define MAX_FIRES 100
#define MAX_NEIGHBORS 8

// The emitter is really only in here for convenience for the renderer.
// Allows renderer to easily look at the health of the fire its rendering
typedef struct FireEmitter {
    float x, y;
    int particles_per_frame;
    float spawn_spread;
    float time_since_spawn;
    float spawn_interval;
} FireEmitter;

typedef struct Fire {
    float x;
    float y;
    float w;
    float h;
    float health;
    float max_health;
    struct Fire* neighbors[MAX_NEIGHBORS];
    int neighbors_size;
    bool active; // in pool context, is this a valid fire?
    int last_put_out;
    int last_hit_with_water;
    FireEmitter emitter;
} Fire;

void init_fire(Fire *fire, float x, float y, float w, float h, float health);
bool is_fire_alive(Fire *fire);
void add_fire_neighbor_one_way(Fire *fire, Fire *neighbor);
void add_fire_neighbor(Fire *fire, Fire *neighbor);

#endif
