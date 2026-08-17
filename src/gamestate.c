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
    state->current_mode = MODE_PLAY;

    // BAD: I don't think the editor stuff should be in the gamestate struct.
    // In future we could maybe create a separate place to put this.
    state->selected_fire = NULL;
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(state, dt);
    simulate_water_particles(state, dt);
    check_water_fire_collisions(state);
    update_fires(state, dt);

    // Update camera to follow player (only in play mode)
    // TODO: make this lerp instead of instant movement.
    // Give the camera its own x/y velocity.
    if (state->current_mode == MODE_PLAY) {
        state->camera.x = state->player.x - (state->camera.w / 2);
        state->camera.y = state->player.y - (state->camera.h / 2);
    }
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

// Helper function to find the index of a fire in the fires array
int find_fire_index(GameState *state, Fire *fire) {
    for (int i = 0; i < state->fire_count; i++) {
        if (&state->fires_buf[i] == fire) {
            return i;
        }
    }
    return -1;
}

void save_fire_layout(GameState *state, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        SDL_Log("Failed to open file for writing: %s", filename);
        return;
    }

    // Write header
    fprintf(fp, "FIRE_COUNT %d\n", state->fire_count);

    // Write each fire
    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires_buf[i];
        fprintf(fp, "%d: %f %f %f %f %f %d",
                i, fire->x, fire->y, fire->w, fire->h, fire->health,
                fire->neighbors_size);

        // Write neighbor indices
        for (int j = 0; j < fire->neighbors_size; j++) {
            int neighbor_index = find_fire_index(state, fire->neighbors[j]);
            fprintf(fp, " %d", neighbor_index);
        }

        fprintf(fp, "\n");
    }

    fclose(fp);
    SDL_Log("Successfully saved fire layout to %s", filename);
}

void load_fire_layout(GameState *state, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        SDL_Log("Failed to open file for reading: %s", filename);
        return;
    }

    // Read fire count
    int fire_count;
    if (fscanf(fp, "FIRE_COUNT %d\n", &fire_count) != 1) {
        SDL_Log("Failed to read fire count from file");
        fclose(fp);
        return;
    }
    SDL_Log("Fire count loaded: %i\n", fire_count);

    // Clear our current level before loading.
    pool_free_all(&state->fires_pool);
    state->fire_count = 0;
    //memset(state->fires, 0, state->fire_count);

    // Temporary storage for neighbor indices
    int neighbor_indices[MAX_FIRES][MAX_NEIGHBORS];
    int neighbor_counts[MAX_FIRES];

    // First pass: Read fire data (positions, health)
    for (int i = 0; i < fire_count && i < MAX_FIRES; i++) {
        int idx, neighbor_count;
        float x, y, w, h, health;

        if (fscanf(fp, "%d: %f %f %f %f %f %d",
                   &idx, &x, &y, &w, &h, &health, &neighbor_count) != 7) {
            SDL_Log("Failed to read fire data at index %d", i);
            break;
        }

        Fire* fire = pool_alloc(&state->fires_pool);
        init_fire(fire, x, y, w, h, health);
        neighbor_counts[idx] = neighbor_count;

        // Read neighbor indices
        for (int j = 0; j < neighbor_count && j < MAX_NEIGHBORS; j++) {
            if (fscanf(fp, " %d", &neighbor_indices[idx][j]) != 1) {
                SDL_Log("Failed to read neighbor index");
                break;
            }
        }
    }
    SDL_Log("First read.\n");

    state->fire_count = fire_count;

    // Second pass: Set up neighbor pointers
    for (int i = 0; i < fire_count; i++) {
        Fire *fire = &state->fires_buf[i];
        for (int j = 0; j < neighbor_counts[i]; j++) {
            int neighbor_idx = neighbor_indices[i][j];
            if (neighbor_idx >= 0 && neighbor_idx < fire_count) {
                //add_fire_neighbor(fire, &state->fires[neighbor_idx]);
                add_fire_neighbor_one_way(fire, &state->fires_buf[neighbor_idx]);
            }
        }
    }
    SDL_Log("Second pass read.\n");

    fclose(fp);
    SDL_Log("Successfully loaded fire layout from %s", filename);
}

// Editor helper function to get fire at world position
Fire* get_fire_at_position(GameState *state, float world_x, float world_y) {
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

void editor_handle_left_click(GameState *state, float world_x, float world_y) {
    Fire *clicked_fire = get_fire_at_position(state, world_x, world_y);

    if (clicked_fire != NULL) {
        // Clicked on existing fire
        if (state->selected_fire == NULL) {
            // First click - select this fire
            state->selected_fire = clicked_fire;
            SDL_Log("Fire selected for neighbor connection");
        } else if (state->selected_fire == clicked_fire) {
            // Clicked same fire - deselect
            state->selected_fire = NULL;
            SDL_Log("Fire deselected");
        } else {
            // Second click - create neighbor connection
            // BUG: segfault if we add more than the max allowed
            // neighbors. Add a check here.
            add_fire_neighbor(state->selected_fire, clicked_fire);
            SDL_Log("Connected fires as neighbors");
            state->selected_fire = NULL;
        }
    } else {
        // WARN: THIS WILL CRASH IF YOU GO BEYOND FIRE LIMIT.
        // Clicked on empty space - place new fire
        Fire* fire = pool_alloc(&state->fires_pool);
        init_fire(fire, world_x - 25, world_y - 25, 50, 50, 10);
        state->fire_count++;
        SDL_Log("Placed new fire at (%f, %f)", world_x, world_y);
        state->selected_fire = NULL;
    }
}

void editor_handle_right_click(GameState *state, float world_x, float world_y) {
    Fire *clicked_fire = get_fire_at_position(state, world_x, world_y);

    if (clicked_fire != NULL) {
        // Remove this fire's neighbor references from all other fires
        for (int i = 0; i < state->fire_count; i++) {
            Fire *other_fire = &state->fires_buf[i];

            for (int j = 0; j < other_fire->neighbors_size; j++) {
                if (other_fire->neighbors[j] == clicked_fire) {
                    // Shift neighbors array
                    // TODO: this sucks
                    for (int k = j; k < other_fire->neighbors_size - 1; k++) {
                        other_fire->neighbors[k] = other_fire->neighbors[k + 1];
                    }
                    other_fire->neighbors_size--;
                    break;
                }
            }
        }

        // Clear selected_fire if it was this fire
        if (state->selected_fire == clicked_fire) {
            state->selected_fire = NULL;
        }

        pool_free(&state->fires_pool, clicked_fire);
        state->fire_count -= 1;

        SDL_Log("Deleted fire");
    }
}

