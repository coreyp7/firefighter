#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SDL3/SDL.h>

#define MAX_WATER_PARTICLES 500

typedef struct Player {
    float x;
    float y;
    float xvel;
    float yvel;
    float cursor_x;
    float cursor_y;
} Player;

typedef struct WaterParticle {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    bool active;
    SDL_FColor color;
} WaterParticle;

typedef struct GameState {
    Player player;
    WaterParticle particles[MAX_WATER_PARTICLES];
    int particle_count;
} GameState;

void init_gamestate(GameState *state);
void simulate_gamestate(GameState *state, float dt);
void cleanup_gamestate(GameState *state);

#endif
