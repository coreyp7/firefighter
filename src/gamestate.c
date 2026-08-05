#include "gamestate.h"
#include "water_particles.h"
#include <math.h>


void init_gamestate(GameState *state) {
    state->player.x = 50.0;
    state->player.y = 400.0;
    state->player.xvel = 0.0;
    state->player.yvel = 0.0;
    state->player.cursor_x = 0.0;
    state->player.cursor_y = 0.0;

    state->particle_count = 0;
    for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
        state->particles[i].active = false;
    }

    // Test block
    Block *block = state->blocks;
    block->x = 0;
    block->y = 500;
    block->w = 250;
    block->h = 250;
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(state, dt);
    simulate_water_particles(state, dt);
}

void cleanup_gamestate(GameState *state) {
    state->particle_count = 0;
}

void update_player(GameState *state, float dt) {
    Player *player = &state->player;
    SDL_FRect player_rect = {player->x, player->y, 95, 95};
    Block *block = &state->blocks[0];
    SDL_FRect block_rect = {block->x, block->y, block->w, block->h};

    float oldx = player->x;
    float oldy = player->y;

    player->yvel += 150 * dt;

    player->x += player->xvel * dt;
    player_rect.x = player->x;
    if(is_colliding(player_rect, block_rect)){
        player->x = oldx;
        player->xvel = 0;
        player_rect.x = oldx;
    }

    player->y += player->yvel * dt;
    player_rect.y = player->y;
    if(is_colliding(player_rect, block_rect)){
        player->y = oldy;
        player->yvel = 0;
        player_rect.y = oldy;

    }

    // Check collisions
    // SDL_FRect player_rect = {player->x, player->y, 95, 95};
    // Block *block = blocks[0];
    // SDL_FRect block_rect = {block->x, block->y, block->w, block->h};
}

// bool check_collision_2d(AABB2D a, AABB2D b) {
//     // Check if there is separation on any axis
//     if (a.maxX < b.minX || a.minX > b.maxX) return false;
//     if (a.maxY < b.minY || a.minY > b.maxY) return false;
//
//     // Overlapping on both axes means a collision is occurring
//     return true;
// }

bool is_colliding(SDL_FRect a, SDL_FRect b){
    // If separation on either axis return false
    if(a.x + a.w < b.x || a.x > b.x + b.w){
        return false;
    }

    if(a.y + a.h < b.y || a.y > b.y + b.h){
        return false;
    }
    return true;
}
