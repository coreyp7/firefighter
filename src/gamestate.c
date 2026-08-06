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
    state->player.is_facing_left = false;

    state->particle_count = 0;
    for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
        state->particles[i].active = false;
    }

    // Initialize blocks at ground level
    state->block_count = 5;

    state->blocks[0] = (Block){0, 500, 250, 250};
    state->blocks[1] = (Block){250, 500, 250, 250};
    state->blocks[2] = (Block){500, 500, 250, 250};
    state->blocks[3] = (Block){750, 500, 250, 250};
    state->blocks[4] = (Block){1000, 500, 250, 250};
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(state, dt);
    simulate_water_particles(state, dt);
}

void cleanup_gamestate(GameState *state) {
    state->particle_count = 0;
}

// TODO: split this up into some functions.
void update_player(GameState *state, float dt) {
    Player *player = &state->player;
    SDL_FRect player_rect = {player->x, player->y, 95, 95};

    float oldx = player->x;
    float oldy = player->y;

    player->yvel += PLAYER_GRAVITY * dt;

    // Check horizontal collisions
    player->x += player->xvel * dt;
    player_rect.x = player->x;
    for (int i = 0; i < state->block_count; i++) {
        Block *block = &state->blocks[i];
        SDL_FRect block_rect = {block->x, block->y, block->w, block->h};
        if(is_colliding(player_rect, block_rect)){
            player->x = oldx;
            player->xvel = 0;
            player_rect.x = oldx;
            break;
        }
    }

    // Check vertical collisions
    player->y += player->yvel * dt;
    player_rect.y = player->y;
    bool collided_vertically = false;
    for (int i = 0; i < state->block_count; i++) {
        Block *block = &state->blocks[i];
        SDL_FRect block_rect = {block->x, block->y, block->w, block->h};
        if(is_colliding(player_rect, block_rect)){
            player->is_grounded = true;
            player->y = oldy;
            player->yvel = 0;
            player_rect.y = oldy;
            collided_vertically = true;
            break;
        }
    }

    if (!collided_vertically) {
        player->is_grounded = false;
    }

    // Update facing direction
    if(player->cursor_x > player->x){
        player->is_facing_left = false;
    } else {
        player->is_facing_left = true;
    }
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
