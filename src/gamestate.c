#include "gamestate.h"
#include "water_particles.h"
#include "camera.h"
#include "fire.h"
#include <math.h>


void init_gamestate(GameState *state, int window_width, int window_height) {
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

    state->block_count = 5;
    state->blocks[0] = (Block){0, 500, 250, 250};
    state->blocks[1] = (Block){250, 500, 250, 250};
    state->blocks[2] = (Block){500, 500, 250, 250};
    state->blocks[3] = (Block){750, 500, 250, 250};
    state->blocks[4] = (Block){1000, 500, 250, 250};

    // Initialize fires
    state->fire_count = 3;
    init_fire(&state->fires[0], 300, 450, 50, 50, 10);
    init_fire(&state->fires[1], 600, 450, 50, 50, 25);
    init_fire(&state->fires[2], 900, 450, 50, 50, 50);
    init_fire(&state->fires[3], 1200, 450, 50, 50, 75);

    state->camera = (Camera){0, 0, window_width, window_height};
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(state, dt);
    simulate_water_particles(state, dt);
    check_water_fire_collisions(state);

    // Update camera to follow player
    // TODO: make this lerp instead of instant movement.
    // Give the camera its own x/y velocity.
    state->camera.x = state->player.x - (state->camera.w / 2);
    state->camera.y = state->player.y - (state->camera.h / 2);
}

void cleanup_gamestate(GameState *state) {
    state->particle_count = 0;
}

// TODO: maybe split this up into some functions.
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

    // Update prince facing direction
    SDL_FPoint player_pos_relative = convert_pos_to_camera_pos(
        state->camera, player->x, player->y
    );
    if(player_pos_relative.x < player->cursor_x){
        player->is_facing_left = false;
    } else {
        player->is_facing_left = true;
    }
}

// AABB
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

void check_water_fire_collisions(GameState *state) {
    // Iterate through all water particles
    for (int i = 0; i < state->particle_count; i++) {
        WaterParticle *particle = &state->particles[i];

        if (!particle->active) {
            continue;
        }

        // Create rect for water particle
        SDL_FRect water_rect = {particle->x, particle->y, 15.0f, 15.0f};

        // Check collision with each fire
        for (int j = 0; j < state->fire_count; j++) {
            Fire *fire = &state->fires[j];

            if (!is_fire_alive(fire)) {
                continue;
            }

            // Create rect for fire
            SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};

            // Check if they collide
            if (is_colliding(water_rect, fire_rect)) {
                // Damage the fire
                fire->health -= 1.0f;

                // Put out the water particle (set life to max so it disappears)
                particle->life = 0;
                particle->active = false;

                // Check if fire is extinguished
                if (fire->health <= 0) {
                    fire->active = false;
                }

                break; // Water particle can only hit one fire
            }
        }
    }
}

