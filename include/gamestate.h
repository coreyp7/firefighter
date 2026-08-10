#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SDL3/SDL.h>
#include "camera.h"
#include "fire.h"

#define MAX_WATER_PARTICLES 500
#define MAX_BLOCK_AMOUNT 30

#define PLAYER_GRAVITY 700

typedef struct Player {
    float x;
    float y;
    float w;
    float h;
    float xvel;
    float yvel;
    float cursor_x;
    float cursor_y;
    bool is_grounded;
    bool is_facing_left;
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

typedef struct Block {
    float x;
    float y;
    float w;
    float h;
} Block;

typedef struct GameState {
    Player player;
    WaterParticle particles[MAX_WATER_PARTICLES];
    int particle_count;
    Block blocks[MAX_BLOCK_AMOUNT];
    int block_count;
    Fire fires[MAX_FIRES];
    int fire_count;
    Camera camera;
} GameState;

void init_gamestate(GameState *state, int window_width, int window_height);
void simulate_gamestate(GameState *state, float dt);
void cleanup_gamestate(GameState *state);
bool is_colliding(SDL_FRect a, SDL_FRect b);
void update_player(GameState *state, float dt);
void check_water_fire_collisions(GameState *state);

#endif
