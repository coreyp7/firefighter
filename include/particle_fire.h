#ifndef PARTICLE_FIRE_H
#define PARTICLE_FIRE_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "camera.h"
#include "fire.h"

#define MAX_FIRE_PARTICLES 4000

typedef struct FireParticle {
    float x, y;
    float velocity_x, velocity_y;
    float lifetime;
    float max_lifetime;
    float noise_seed;
    float intensity;
    bool active;
} FireParticle;

typedef struct ParticleFireSystem {
    FireParticle particles[MAX_FIRE_PARTICLES];
    int active_particle_count;
    float time;  // Running time for animations
} ParticleFireSystem;

// Lifecycle
void particle_fire_init(ParticleFireSystem *system);
void particle_fire_cleanup(ParticleFireSystem *system);

// Update & Render
void particle_fire_update(ParticleFireSystem *system, Fire *fires_buf, int max_fires, float dt);
void particle_fire_render(ParticleFireSystem *system, SDL_Renderer *renderer, Camera camera);

#endif
