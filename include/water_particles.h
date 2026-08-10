#ifndef WATER_PARTICLES_H
#define WATER_PARTICLES_H

#include <SDL3/SDL.h>
#include "gamestate.h"

void init_water_particles(void);
void simulate_water_particles(GameState *state, float dt);
void shoot_water_particle(GameState *state, float x, float y, float angle);
void cleanup_water_particles(void);
int get_active_water_particle_count(GameState *state);

#endif
