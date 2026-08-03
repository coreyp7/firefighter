#include "water_particles.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

#define GRAVITY 500.0f
#define WATER_VELOCITY 500.0f
#define WATER_LIFETIME 7.0f

static WaterParticle particles[MAX_WATER_PARTICLES];
static int particle_count = 0;

void init_water_particles(void) {
    particle_count = 0;
    for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
        particles[i].active = false;
    }
}

void shoot_water_particle(float x, float y, float angle) {
    WaterParticle *p = NULL;

    // Try to find an inactive slot first
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) {
            p = &particles[i];
            break;
        }
    }

    // If no inactive slot found, allocate a new one
    if (!p && particle_count < MAX_WATER_PARTICLES) {
        p = &particles[particle_count++];
    }

    // If still no slot available, we're full
    if (!p) {
        return;
    }

    p->x = x;
    p->y = y;
    p->vx = sinf(angle) * WATER_VELOCITY;
    p->vy = cosf(angle) * WATER_VELOCITY - 100.0f;
    p->max_life = WATER_LIFETIME;
    p->life = p->max_life;
    p->active = true;
    p->color = (SDL_FColor){0.2f, 0.8f, 1.0f, 0.8f};
}

void simulate_water_particles(float dt) {
    for (int i = 0; i < particle_count; i++) {
        WaterParticle *p = &particles[i];

        if (!p->active) {
            continue;
        }

        p->vy += GRAVITY * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;

        p->life -= dt;

        if (p->life <= 0) {
            p->active = false;
        }
    }
}

void render_water_particles(SDL_Renderer *renderer) {
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) {
            continue;
        }

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

int get_active_water_particle_count(void) {
    int active_count = 0;
    for (int i = 0; i < particle_count; i++) {
        if (particles[i].active) {
            active_count++;
        }
    }
    return active_count;
}

void cleanup_water_particles(void) {
    particle_count = 0;
}
