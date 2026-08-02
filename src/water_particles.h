#ifndef WATER_PARTICLES_H
#define WATER_PARTICLES_H

#include <SDL3/SDL.h>

typedef struct WaterParticle {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    SDL_FColor color;
} WaterParticle;

#define MAX_WATER_PARTICLES 500

void init_water_particles(void);
void update_water_particles(float dt);
void render_water_particles(SDL_Renderer *renderer);
void emit_water_particle(float x, float y, float angle);
void cleanup_water_particles(void);

#endif
