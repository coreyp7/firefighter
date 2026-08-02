#include "water_particles.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

#define GRAVITY 150.0f
#define WATER_VELOCITY 300.0f
#define WATER_LIFETIME 1.0f

static WaterParticle particles[MAX_WATER_PARTICLES];
static int particle_count = 0;

void init_water_particles(void) {
    particle_count = 0;
    SDL_Log("Water particle system initialized");
}

void emit_water_particle(float x, float y, float angle) {
    if (particle_count >= MAX_WATER_PARTICLES) {
        SDL_Log("WARNING: Particle limit reached!");
        return;
    }

    WaterParticle *p = &particles[particle_count++];
    p->x = x;
    p->y = y;
    p->vx = sinf(angle) * WATER_VELOCITY;
    p->vy = cosf(angle) * WATER_VELOCITY - 100.0f;
    p->max_life = WATER_LIFETIME;
    p->life = p->max_life;
    p->color = (SDL_FColor){0.2f, 0.8f, 1.0f, 0.8f};
    SDL_Log("Emitted water particle at (%.1f, %.1f) with angle %.2f", x, y, angle);
}

void update_water_particles(float dt) {
    for (int i = 0; i < particle_count; i++) {
        WaterParticle *p = &particles[i];

        p->vy += GRAVITY * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;

        p->life -= dt;

        if (p->life <= 0) {
            SDL_Log("Removing dead particle %d", i);
            particles[i] = particles[--particle_count];
            i--;
        }
    }
}

void render_water_particles(SDL_Renderer *renderer) {
    for (int i = 0; i < particle_count; i++) {
        // float alpha = particles[i].life / particles[i].max_life;
        // SDL_SetRenderDrawColorFloat(renderer,
        //                             particles[i].color.r * alpha,
        //                             particles[i].color.g * alpha,
        //                             particles[i].color.b * alpha,
        //                             particles[i].color.a * alpha);
        // SDL_RenderPoint(renderer, particles[i].x, particles[i].y);
        SDL_FRect frect = {particles[i].x, particles[i].y, 15.f, 15.f};
        SDL_SetRenderDrawColor(renderer, 92, 181, 225, 255);
        SDL_RenderFillRect(renderer, &frect);
    }
}

void cleanup_water_particles(void) {
    particle_count = 0;
    SDL_Log("Water particle system cleaned up");
}
