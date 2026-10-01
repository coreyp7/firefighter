#include "gamestate.h"
#include "random_utils.h"
#include "math_utils.h"
#include "water_particles.h"
#include "camera.h"
#include "fire.h"
#include "block.h"
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

    // Initialize all levels with their pools
    for (int i = 0; i < MAX_LEVELS; i++) {
        memset(&state->levels[i].blocks_buf, 0, MAX_BLOCK_AMOUNT * sizeof(Block));
        memset(&state->levels[i].fires_buf, 0, MAX_FIRES * sizeof(Fire));

        // Initialize blocks pool for this level
        pool_init(
            &state->levels[i].blocks_pool,
            state->levels[i].blocks_buf,
            MAX_BLOCK_AMOUNT * sizeof(Block),
            sizeof(Block)
        );

        // Initialize fires pool for this level
        pool_init(
            &state->levels[i].fires_pool,
            state->levels[i].fires_buf,
            MAX_FIRES * sizeof(Fire),
            sizeof(Fire)
        );

        // Set default spawn position for each level
        state->levels[i].player_spawn_x = 50.0f;
        state->levels[i].player_spawn_y = 400.0f;
    }

    // Set current level to first level
    state->current_level_index = 0;
    state->blocks_buf = state->levels[0].blocks_buf;
    state->fires_buf = state->levels[0].fires_buf;

    // Point to first level's pools
    state->blocks_pool_ptr = &state->levels[0].blocks_pool;
    state->fires_pool_ptr = &state->levels[0].fires_pool;
    state->block_count = 0;

    // Add default blocks
    gamestate_add_block(state, 0, 500, 250, 250);
    gamestate_add_block(state, 250, 500, 250, 250);
    gamestate_add_block(state, 500, 500, 250, 250);
    gamestate_add_block(state, 750, 500, 250, 250);
    gamestate_add_block(state, 1000, 500, 250, 250);

    state->fire_count = 0;

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
    // TODO: LOL this has been hardcoded this whole time. Fix this.
    SDL_FRect player_rect = {player->x, player->y, 95, 95};

    float oldx = player->x;
    float oldy = player->y;

    player->yvel += PLAYER_GRAVITY * dt;
    // TODO: improve logic around player x_vel on the ground with
    // forces applied from water spray.

    // Check horizontal collisions
    player->x += player->xvel * dt;
    player_rect.x = player->x;
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        Block *block = &state->blocks_buf[i];
        if (!block->active) continue;
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
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        Block *block = &state->blocks_buf[i];
        if (!block->active) continue;
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

    // Push player up if above fire "wind"
    // loop through fires_buf (where active=true).
    // A: Naive solution: just check if the player.x intercepts with a fire.x and if
    // the player.y > fire.y (will actually be < because of y being flipped).
    // If true, then add y velocity to the player: play with it.
    // So, in player simulate: loop through all active fires with health > 0 and
    // do this check. Add y force if true.
    int max_boosts = 2; // TODO: move into constant
    int boosts = 0;
    for(int i=0; i<MAX_FIRES; i++){
        Fire* fire = &state->fires_buf[i];
        if(!fire->active || fire->health == 0){
            continue;
        }

        bool above_fire = is_player_above_fire(player, fire);
        bool in_air = !player->is_grounded;
        if(above_fire && in_air){
            // player->yvel -= 2.5;
            player->yvel -= 2.0;
            boosts += 1;
        }

        if(boosts >= max_boosts){
            break;
        }
    }

    // Handle water shooting
    if (player->is_shooting_water) {
        float dx = player->cursor_x - player_pos_relative.x;
        float dy = player->cursor_y - player_pos_relative.y;

        //float noise = random_float_range(-0.5f, 0.5f);
        float noise = random_float_range(-0.25f, 0.25f);
        float angle = atan2f(dx, dy) + noise;
        //float angle = atan2f(dx, dy);
        shoot_water_particle(state, player->x, player->y, angle);

        float x_normal = dx;
        float y_normal = dy;
        vec2_normalize(&x_normal, &y_normal);

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

        //for (int j = 0; j < state->fire_count; j++) {
        for (int j = 0; j < MAX_FIRES; j++) {
            Fire *fire = &state->fires_buf[j];
            if(!fire->active) continue;

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
                fire->last_hit_with_water = SDL_GetTicks();

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
// TODO: make this feel better since right now they regenerate immediately.
// TODO: Rename this to 'regrowFires' or something
void update_fires(GameState *state, float dt){
    // Check all the neighbors of an inactive fire, and if they are alive,
    // then begin lighting the fire.
    //for(int i=0; i<state->fire_count; i++){
    for (int i = 0; i < MAX_FIRES; i++) {
        Fire *fire = &state->fires_buf[i];
        if(!fire->active) continue;

        if(fire->health > fire->max_health){
            fire->health = fire->max_health;
            continue;
        }

        if(fire->neighbors_size == 0){
            continue;
        }

        for(int j=0; j<fire->neighbors_size; j++){
            Fire *neighbor = fire->neighbors[j];
            float seconds_since_put_out = (SDL_GetTicks() - fire->last_put_out) / 1000;
            float seconds_since_hit = (SDL_GetTicks() - fire->last_hit_with_water) / 1000;

            bool can_be_hit = seconds_since_put_out > 0.5 && seconds_since_hit > 0.5;
            // if(is_fire_alive(neighbor) && seconds > 0.5){
            if(is_fire_alive(neighbor) && can_be_hit){
            // TODO: update this to be dynamic based on how active
            // the neighbor fire is. (is this already done elsewhere)
                //fire->health += 0.1;
                fire->health += 0.2;
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

    Fire *fire = pool_alloc(state->fires_pool_ptr);
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

    //for (int i = 0; i < state->fire_count; i++) {
    for (int i = 0; i < MAX_FIRES; i++) {
        Fire *other_fire = &state->fires_buf[i];
        if(!other_fire->active) continue;

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
    fire->active = false;
    pool_free(state->fires_pool_ptr, fire);
    state->fire_count--;
}

Fire* gamestate_find_fire_at_position(GameState *state, float world_x, float world_y) {
    //for (int i = 0; i < state->fire_count; i++) {
    for (int i = 0; i < MAX_FIRES; i++) {
        Fire *fire = &state->fires_buf[i];
        if(!fire->active) continue;

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

// ============================================================================
// GameState Block Management API
// ============================================================================

Block* gamestate_add_block(GameState *state, float x, float y, float w, float h) {
    // Check if we've reached max blocks
    if (state->block_count >= MAX_BLOCK_AMOUNT) {
        SDL_Log("ERROR: Cannot add block - max blocks (%d) reached", MAX_BLOCK_AMOUNT);
        return NULL;
    }

    // Allocate block from pool
    Block *block = pool_alloc(state->blocks_pool_ptr);
    if (block == NULL) {
        SDL_Log("ERROR: Cannot add block - pool allocation failed");
        return NULL;
    }

    // Initialize the block
    init_block(block, x, y, w, h);
    state->block_count++;

    return block;
}

void gamestate_remove_block(GameState *state, Block *block) {
    if (block == NULL) {
        return;
    }

    block->active = false;
    pool_free(state->blocks_pool_ptr, block);
    state->block_count--;
}

Block* gamestate_find_block_at_position(GameState *state, float world_x, float world_y) {
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        Block *block = &state->blocks_buf[i];

        if (!block->active) continue;

        SDL_FRect block_rect = {block->x, block->y, block->w, block->h};

        // Check if point is inside block rectangle
        if (world_x >= block_rect.x && world_x <= block_rect.x + block_rect.w &&
            world_y >= block_rect.y && world_y <= block_rect.y + block_rect.h) {
            return block;
        }
    }
    return NULL;
}

int gamestate_get_block_count(GameState *state) {
    return state->block_count;
}

Block* gamestate_get_blocks_buffer(GameState *state) {
    return state->blocks_buf;
}

void gamestate_switch_level(GameState *state, int new_level_index) {
    // Validation
    if (new_level_index < 0 || new_level_index >= MAX_LEVELS) {
        SDL_Log("ERROR: Invalid level index %d (must be 0-%d)",
                new_level_index, MAX_LEVELS - 1);
        return;
    }

    if (new_level_index == state->current_level_index) {
        SDL_Log("Already on level %d", new_level_index);
        return;
    }

    SDL_Log("Switching from level %d to level %d",
            state->current_level_index, new_level_index);

    // 1. Clear water particles (global state)
    state->particle_count = 0;
    for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
        state->particles[i].active = false;
    }

    // 2. Update buffer pointers
    state->blocks_buf = state->levels[new_level_index].blocks_buf;
    state->fires_buf = state->levels[new_level_index].fires_buf;

    // 3. Update pool pointers (preserves allocation state)
    state->blocks_pool_ptr = &state->levels[new_level_index].blocks_pool;
    state->fires_pool_ptr = &state->levels[new_level_index].fires_pool;

    // 4. Recalculate counts
    state->block_count = 0;
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        if (state->blocks_buf[i].active) {
            state->block_count++;
        }
    }

    state->fire_count = 0;
    for (int i = 0; i < MAX_FIRES; i++) {
        if (state->fires_buf[i].active) {
            state->fire_count++;
            state->fires_buf[i].health = state->fires_buf[i].max_health;
        }
    }

    // 5. Update level index
    state->current_level_index = new_level_index;

    // 6. Reset player position
    state->player.x = state->levels[new_level_index].player_spawn_x;
    state->player.y = state->levels[new_level_index].player_spawn_y;

    // 7. Reset player physics state
    state->player.xvel = 0.0f;
    state->player.yvel = 0.0f;
    state->player.is_grounded = false;
    state->player.is_shooting_water = false;
    state->player.is_facing_left = false;

    // 8. Reset camera (center on player)
    state->camera.x = state->player.x - state->camera.w / 2;
    state->camera.y = state->player.y - state->camera.h / 2;

    SDL_Log("Successfully switched to level %d", new_level_index);
    SDL_Log("  Blocks: %d, Fires: %d", state->block_count, state->fire_count);
}

bool is_player_above_fire(Player *player, Fire *fire){
    //if(player->x + player->w < fire->x || player->x > fire->x + fire->w){
    if(player->x + 95 < fire->x || player->x > fire->x + fire->w){
        return false;
    }

    // A little rough but should would.
    //if(player->y + player->h < fire->y){
    if(player->y + 95 > fire->y){
        return false;
    }

    return true;
}

