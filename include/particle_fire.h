#ifndef PARTICLE_FIRE_H
#define PARTICLE_FIRE_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "camera.h"

#define MAX_FIRE_PARTICLES 2000
#define MAX_FIRE_EMITTERS 50

typedef struct FireParticle {
    float x, y;
    float velocity_x, velocity_y;
    float lifetime;
    float max_lifetime;
    float noise_seed;
    float intensity;
    bool active;
} FireParticle;

typedef struct FireEmitter {
    float x, y;
    int particles_per_frame;
    float spawn_spread;  // Horizontal spread when spawning
} FireEmitter;

typedef struct ParticleFireSystem {
    FireParticle particles[MAX_FIRE_PARTICLES];
    FireEmitter emitters[MAX_FIRE_EMITTERS];
    int emitter_count;
    int active_particle_count;
    float time;  // Running time for animations
} ParticleFireSystem;

// Lifecycle
void particle_fire_init(ParticleFireSystem *system, float emitter_x, float emitter_y);
void particle_fire_cleanup(ParticleFireSystem *system);

// Update & Render
void particle_fire_update(ParticleFireSystem *system, float dt);
void particle_fire_render(ParticleFireSystem *system, SDL_Renderer *renderer, Camera camera);

// Utilities
void particle_fire_set_emitter_position(ParticleFireSystem *system, float x, float y);
void particle_fire_set_spawn_rate(ParticleFireSystem *system, int particles_per_frame);

// Multi-emitter management
void particle_fire_add_emitter(ParticleFireSystem *system, float x, float y);
void particle_fire_clear_emitters(ParticleFireSystem *system);

#endif
