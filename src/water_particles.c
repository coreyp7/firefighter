#include "water_particles.h"
#include "camera.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define GRAVITY 500.0f
//#define WATER_VELOCITY 500.0f
#define WATER_VELOCITY 700.0f
#define WATER_LIFETIME 7.0f

void init_water_particles(void) {
    // No-op: initialization is now handled by init_gamestate
}

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
    SDL_FPoint converted_cursor_pos = {x, y};
    converted_cursor_pos.x += state->camera.x;
    converted_cursor_pos.y += state->camera.y;


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
    float vx = sinf(angle) * WATER_VELOCITY;
    float vy = cosf(angle) * WATER_VELOCITY - 100.0f;
    p->vx = vx;
    p->vy = vy;

    // float random_dev_x = rand() % 5;
    // float random_dev_y = rand() % 5;
    // if (rand() % 2 == 0){
    //     random_dev_x *= -1;
    // }
    // if (rand() % 2 == 0){
    //     random_dev_y *= -1;
    // }
    //
    // p->vx = vx + random_dev_x;
    // p->vy = vy + random_dev_y;

    p->max_life = WATER_LIFETIME;
    p->life = p->max_life;
    p->active = true;
    p->color = (SDL_FColor){0.2f, 0.8f, 1.0f, 0.8f};
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

void cleanup_water_particles(void) {
    // No-op: cleanup is now handled by cleanup_gamestate
}

