#include "water_particles.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

#define GRAVITY 500.0f
#define WATER_VELOCITY 500.0f
#define WATER_LIFETIME 7.0f

void simulate_water_particles(GameState *state, float dt) {
    for (int i = 0; i < state->particle_count; i++) {
        WaterParticle *p = &state->particles[i];

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

void shoot_water_particle(GameState *state, float x, float y, float angle) {
    WaterParticle *p = NULL;

    for (int i = 0; i < state->particle_count; i++) {
        if (!state->particles[i].active) {
            p = &state->particles[i];
            break;
        }
    }

    // If no inactive slot found, allocate a new one
    if (!p && state->particle_count < MAX_WATER_PARTICLES) {
        p = &state->particles[state->particle_count++];
    }

    // If still not available, we're full
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

void render_water_particles(SDL_Renderer *renderer, GameState *state) {
    for (int i = 0; i < state->particle_count; i++) {
        if (!state->particles[i].active) {
            continue;
        }

        // float alpha = state->particles[i].life / state->particles[i].max_life;
        // SDL_SetRenderDrawColorFloat(renderer,
        //                             state->particles[i].color.r * alpha,
        //                             state->particles[i].color.g * alpha,
        //                             state->particles[i].color.b * alpha,
        //                             state->particles[i].color.a * alpha);
        // SDL_RenderPoint(renderer, state->particles[i].x, state->particles[i].y);
        SDL_FRect frect = {state->particles[i].x, state->particles[i].y, 15.f, 15.f};
        SDL_SetRenderDrawColor(renderer, 92, 181, 225, 255);
        SDL_RenderFillRect(renderer, &frect);
    }
}

int get_active_water_particle_count(GameState *state) {
    int active_count = 0;
    for (int i = 0; i < state->particle_count; i++) {
        if (state->particles[i].active) {
            active_count++;
        }
    }
    return active_count;
}

