#include "gamestate.h"
#include "water_particles.h"
#include "camera.h"
#include "fire.h"
#include <math.h>
#include <stdio.h>
#include <string.h>


void init_gamestate(GameState *state, int window_width, int window_height) {
    state->player.x = 50.0;
    state->player.y = 400.0;
    state->player.xvel = 0.0;
    state->player.yvel = 0.0;
    state->player.cursor_x = 0.0;
    state->player.cursor_y = 0.0;
    state->player.is_facing_left = false;
    state->player.is_shooting_water = false;

    state->particle_count = 0;
    // for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
    //     state->particles[i].active = false;
    // }

    state->block_count = 5;
    state->blocks[0] = (Block){0, 500, 250, 250};
    state->blocks[1] = (Block){250, 500, 250, 250};
    state->blocks[2] = (Block){500, 500, 250, 250};
    state->blocks[3] = (Block){750, 500, 250, 250};
    state->blocks[4] = (Block){1000, 500, 250, 250};

    pool_init(
        &state->fires_pool,
        &state->fires_buf,
        MAX_FIRES * sizeof(Fire),
        sizeof(Fire)
    );

    // For now, we won't create any fires until loading.

    state->camera = (Camera){0, 0, window_width, window_height};
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(state, dt);
    simulate_water_particles(state, dt);
    check_water_fire_collisions(state);
    update_fires(state, dt);
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
    // TODO: improve logic around player x_vel on the ground with
    // forces applied from water spray.

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

    // Handle water shooting
    if (player->is_shooting_water) {
        float dx = player->cursor_x - player_pos_relative.x;
        float dy = player->cursor_y - player_pos_relative.y;
        float angle = atan2f(dx, dy);
        shoot_water_particle(state, player->x, player->y, angle);

        // TODO: move normalize code into function in appropriate module.
        float xsq = dx * dx;
        float ysq = dy * dy;
        float sum = xsq + ysq;
        float magnitude = sqrt(sum);

        float x_normal = dx / magnitude;
        float y_normal = dy / magnitude;

        // TODO: put these into constants
        if(y_normal > 0){
            player->yvel += (-y_normal) * 10;
        } else if (y_normal < 0){
            //player->yvel += (-y_normal) * 3;
        }

        if(!player->is_grounded){
            player->xvel += (-x_normal) * 2;
        }
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
    // Currently n^2, can fix in future if I switch to spacial data structure.
    // There's a max of around 370~ water particles.
    for (int i = 0; i < state->particle_count; i++) {
        WaterParticle *particle = &state->particles[i];

        // TODO: rename to isActive
        if (!particle->active) {
            continue;
        }

        SDL_FRect water_rect = {particle->x, particle->y, 15.0f, 15.0f};

        for (int j = 0; j < state->fire_count; j++) {
            Fire *fire = &state->fires_buf[j];

            //if (!is_fire_alive(fire)) {
            if(fire->health == 0){
                continue;
            }

            SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};

            // Leaving commented for experimenting with later.
            //int seconds = (SDL_GetTicks() - fire->last_put_out) / 1000;
            //if (is_colliding(water_rect, fire_rect) && seconds > 4) {

            if (is_colliding(water_rect, fire_rect)){
                fire->health -= 1.0f;
                if (fire->health <= 0) {
                    // TODO: get rid of this active logic.
                    // Just have it be a function that looks at the health.
                    fire->health = 0;
                    fire->last_put_out = SDL_GetTicks();
                }

                // Kill the water particle
                particle->life = 0;
                particle->active = false;

                break; // Water particle can only hit one fire
            }
        }
    }
}

// SLOW: there's a better way to do this but worry about that later.
void update_fires(GameState *state, float dt){
    // Check all the neighbors of an inactive fire, and if they are alive,
    // then begin lighting the fire.
    for(int i=0; i<state->fire_count; i++){
        Fire *fire = &state->fires_buf[i];
        if(fire->health >= fire->max_health){
            fire->health = fire->max_health;
            continue;
        }

        for(int j=0; j<fire->neighbors_size; j++){
            Fire *neighbor = fire->neighbors[j];
            float seconds = (SDL_GetTicks() - fire->last_put_out) / 1000;

            if(is_fire_alive(neighbor) && seconds > 0.5){
            // TODO: update this to be dynamic based on how active
            // the neighbor fire is.
                fire->health += 0.1;
            }
        }
    }
}

// ============================================================================
// GameState editor api (here for now)
// ============================================================================

Fire* gamestate_add_fire(GameState *state, float x, float y, float w, float h, float health) {
    if (state->fire_count >= MAX_FIRES) {
        SDL_Log("ERROR: Cannot add fire - max fires (%d) reached", MAX_FIRES);
        return NULL;
    }

    Fire *fire = pool_alloc(&state->fires_pool);
    if (fire == NULL) {
        SDL_Log("ERROR: Cannot add fire - pool allocation failed");
        return NULL;
    }

    init_fire(fire, x, y, w, h, health);
    state->fire_count++;

    return fire;
}

void gamestate_remove_fire(GameState *state, Fire *fire) {
    if (fire == NULL) {
        return;
    }

    // Remove this fire's references from all other fires' neighbor lists
    // BAD: fine for now but pretty lazy.
    for (int i = 0; i < state->fire_count; i++) {
        Fire *other_fire = &state->fires_buf[i];

        // Skip if this is the fire being deleted
        if (other_fire == fire) {
            continue;
        }

        for (int j = 0; j < other_fire->neighbors_size; j++) {
            if (other_fire->neighbors[j] == fire) {
                // Shift neighbors array to remove this reference
                // TODO: look into this, maybe broken
                for (int k = j; k < other_fire->neighbors_size - 1; k++) {
                    other_fire->neighbors[k] = other_fire->neighbors[k + 1];
                }
                other_fire->neighbors_size--;
                break;
            }
        }
    }

    // Free the fire from the pool
    pool_free(&state->fires_pool, fire);
    state->fire_count--;
}

Fire* gamestate_find_fire_at_position(GameState *state, float world_x, float world_y) {
    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires_buf[i];

        SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};

        // Check if point is inside fire rectangle
        if (world_x >= fire_rect.x && world_x <= fire_rect.x + fire_rect.w &&
            world_y >= fire_rect.y && world_y <= fire_rect.y + fire_rect.h) {
            return fire;
        }
    }
    return NULL;
}

bool gamestate_connect_fires(GameState *state, Fire *fire1, Fire *fire2) {
    if (fire1 == NULL || fire2 == NULL) {
        SDL_Log("ERROR: Cannot connect NULL fires");
        return false;
    }

    if (fire1 == fire2) {
        SDL_Log("ERROR: Cannot connect fire to itself");
        return false;
    }

    if (fire1->neighbors_size >= MAX_NEIGHBORS) {
        SDL_Log("ERROR: Fire at (%f, %f) already has max neighbors (%d)",
                fire1->x, fire1->y, MAX_NEIGHBORS);
        return false;
    }

    if (fire2->neighbors_size >= MAX_NEIGHBORS) {
        SDL_Log("ERROR: Fire at (%f, %f) already has max neighbors (%d)",
                fire2->x, fire2->y, MAX_NEIGHBORS);
        return false;
    }

    for (int i = 0; i < fire1->neighbors_size; i++) {
        if (fire1->neighbors[i] == fire2) {
            SDL_Log("WARNING: Fires are already neighbors");
            return false;
        }
    }

    add_fire_neighbor(fire1, fire2);

    return true;
}

void gamestate_disconnect_fire(GameState *state, Fire *fire) {
    if (fire == NULL) {
        return;
    }

    gamestate_remove_fire(state, fire);
}

int gamestate_get_fire_count(GameState *state) {
    return state->fire_count;
}

Fire* gamestate_get_fires_buffer(GameState *state) {
    return state->fires_buf;
}

