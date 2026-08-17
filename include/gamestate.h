#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SDL3/SDL.h>
#include "camera.h"
#include "fire.h"
#include "pool.h"

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
    bool is_shooting_water;
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
    Fire fires_buf[MAX_FIRES];
    Pool fires_pool;
    int fire_count;
    Camera camera;
} GameState;

void init_gamestate(GameState *state, int window_width, int window_height);
void simulate_gamestate(GameState *state, float dt);
void cleanup_gamestate(GameState *state);
bool is_colliding(SDL_FRect a, SDL_FRect b);
void update_player(GameState *state, float dt);
void check_water_fire_collisions(GameState *state);
void update_fires(GameState *state, float dt);

// GameState Fire Management API
// NOTE: In future, maybe we can split this up across files in a dir.
// For now, all of these are in this single header.
Fire* gamestate_add_fire(GameState *state, float x, float y, float w, float h, float health);
void gamestate_remove_fire(GameState *state, Fire *fire);
Fire* gamestate_find_fire_at_position(GameState *state, float world_x, float world_y);
bool gamestate_connect_fires(GameState *state, Fire *fire1, Fire *fire2);
void gamestate_disconnect_fire(GameState *state, Fire *fire);
int gamestate_get_fire_count(GameState *state);
Fire* gamestate_get_fires_buffer(GameState *state);


#endif
