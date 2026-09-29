#include "level_io.h"
#include "fire.h"
#include "block.h"
#include "pool.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

// Helper function to write blocks for a specific level
static void write_blocks(FILE *fp, GameState *game, int level_index) {
    Block *blocks = game->levels[level_index].blocks_buf;
    int block_count = 0;

    // Count active blocks
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        if (blocks[i].active) block_count++;
    }

    fprintf(fp, "BLOCK_COUNT %d\n", block_count);

    // Write each active block
    int idx = 0;
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        if (blocks[i].active) {
            fprintf(fp, "%d: %f %f %f %f %d\n",
                    idx++, blocks[i].x, blocks[i].y,
                    blocks[i].w, blocks[i].h,
                    blocks[i].sprite_id);
        }
    }
}

// TODO: rename functions here. Rather it be "load level"
static bool read_blocks(FILE *fp, GameState *game, int level_index) {
    int block_count;
    if (fscanf(fp, "BLOCK_COUNT %d\n", &block_count) != 1) {
        return false;
    }

    // Clear existing blocks for this level
    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        Block *block = &game->levels[level_index].blocks_buf[i];
        if (block->active && game->current_level_index == level_index) {
            gamestate_remove_block(game, block);
        } else {
            block->active = false;
        }
    }

    // Load blocks
    // TODO: see if this is uneccessary. We could just switch level when needed
    // instead of always checking.
    for (int i = 0; i < block_count; i++) {
        int idx, sprite_id;
        float x, y, w, h;

        // Read block with sprite_id (always required)
        if (fscanf(fp, "%d: %f %f %f %f %d\n",
                   &idx, &x, &y, &w, &h, &sprite_id) != 6) {
            SDL_Log("ERROR: Failed to read block %d", i);
            return false;
        }

        // Temporarily switch to target level if needed
        int original_level = game->current_level_index;
        if (original_level != level_index) {
            gamestate_switch_level(game, level_index);
        }

        Block *block = gamestate_add_block(game, x, y, w, h);
        if (block) {
            block->sprite_id = sprite_id;
        }

        // Switch back
        if (original_level != level_index) {
            gamestate_switch_level(game, original_level);
        }
    }

    return true;
}

bool level_save_all_levels(GameState *game, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        SDL_Log("ERROR: Failed to open file for writing: %s", filename);
        return false;
    }

    // Write header
    fprintf(fp, "LEVELPACK_VERSION 1\n");
    fprintf(fp, "LEVEL_COUNT %d\n\n", MAX_LEVELS);

    // Save each level
    for (int level_idx = 0; level_idx < MAX_LEVELS; level_idx++) {
        fprintf(fp, "LEVEL %d\n", level_idx);

        // Write spawn position
        fprintf(fp, "PLAYER_SPAWN %f %f\n",
                game->levels[level_idx].player_spawn_x,
                game->levels[level_idx].player_spawn_y);

        // Write blocks
        write_blocks(fp, game, level_idx);

        // Write fires
        Fire *fires = game->levels[level_idx].fires_buf;
        int fire_count = 0;
        for (int i = 0; i < MAX_FIRES; i++) {
            if (fires[i].active) fire_count++;
        }

        fprintf(fp, "FIRE_COUNT %d\n", fire_count);

        int fire_idx = 0;
        for (int i = 0; i < MAX_FIRES; i++) {
            if (fires[i].active) {
                Fire *fire = &fires[i];
                fprintf(fp, "%d: %f %f %f %f %f %d",
                        fire_idx++, fire->x, fire->y, fire->w, fire->h,
                        fire->health, fire->neighbors_size);

                // Write neighbor indices (within this level)
                for (int j = 0; j < fire->neighbors_size; j++) {
                    // Find index of neighbor in this level's buffer
                    int neighbor_idx = -1;
                    for (int k = 0; k < MAX_FIRES; k++) {
                        if (&fires[k] == fire->neighbors[j]) {
                            neighbor_idx = k;
                            break;
                        }
                    }
                    fprintf(fp, " %d", neighbor_idx);
                }
                fprintf(fp, "\n");
            }
        }

        fprintf(fp, "END_LEVEL\n\n");
    }

    fclose(fp);
    SDL_Log("Successfully saved all levels to %s", filename);
    return true;
}

bool level_load_all_levels(GameState *game, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        SDL_Log("ERROR: Failed to open file for reading: %s", filename);
        return false;
    }

    // Read header
    int version, level_count;
    if (fscanf(fp, "LEVELPACK_VERSION %d\n", &version) != 1) {
        SDL_Log("ERROR: Invalid levelpack format");
        fclose(fp);
        return false;
    }

    if (version != 1) {
        SDL_Log("ERROR: Unsupported levelpack version %d", version);
        fclose(fp);
        return false;
    }

    if (fscanf(fp, "LEVEL_COUNT %d\n", &level_count) != 1) {
        SDL_Log("ERROR: Failed to read level count");
        fclose(fp);
        return false;
    }

    if (level_count > MAX_LEVELS) {
        SDL_Log("WARNING: File has %d levels, max is %d", level_count, MAX_LEVELS);
        level_count = MAX_LEVELS;
    }

    SDL_Log("Loading %d levels from %s", level_count, filename);

    // Load each level
    for (int i = 0; i < level_count; i++) {
        int level_idx;

        // Read level marker
        if (fscanf(fp, "LEVEL %d\n", &level_idx) != 1) {
            SDL_Log("ERROR: Failed to read level %d marker", i);
            fclose(fp);
            return false;
        }

        // Read spawn position
        float spawn_x, spawn_y;
        if (fscanf(fp, "PLAYER_SPAWN %f %f\n", &spawn_x, &spawn_y) != 2) {
            SDL_Log("ERROR: Failed to read spawn for level %d", level_idx);
            fclose(fp);
            return false;
        }
        game->levels[level_idx].player_spawn_x = spawn_x;
        game->levels[level_idx].player_spawn_y = spawn_y;

        // Read blocks
        if (!read_blocks(fp, game, level_idx)) {
            SDL_Log("ERROR: Failed to read blocks for level %d", level_idx);
            fclose(fp);
            return false;
        }

        // Read fires
        int fire_count;
        if (fscanf(fp, "FIRE_COUNT %d\n", &fire_count) != 1) {
            SDL_Log("ERROR: Failed to read fire count for level %d", level_idx);
            fclose(fp);
            return false;
        }

        // Clear existing fires
        Fire *fires = game->levels[level_idx].fires_buf;
        for (int j = 0; j < MAX_FIRES; j++) {
            if (fires[j].active && game->current_level_index == level_idx) {
                gamestate_remove_fire(game, &fires[j]);
            } else {
                fires[j].active = false;
            }
        }

        // Load fires with neighbor data
        int neighbor_indices[MAX_FIRES][MAX_NEIGHBORS];
        int neighbor_counts[MAX_FIRES];
        Fire *loaded_fires[MAX_FIRES];

        int original_level = game->current_level_index;
        if (original_level != level_idx) {
            gamestate_switch_level(game, level_idx);
        }

        for (int j = 0; j < fire_count; j++) {
            int idx, neighbor_count;
            float x, y, w, h, health;

            if (fscanf(fp, "%d: %f %f %f %f %f %d",
                       &idx, &x, &y, &w, &h, &health, &neighbor_count) != 7) {
                SDL_Log("ERROR: Failed to read fire %d", j);
                fclose(fp);
                return false;
            }

            Fire *fire = gamestate_add_fire(game, x, y, w, h, health);
            if (fire == NULL) {
                SDL_Log("ERROR: Failed to create fire at index %d", j);
                fclose(fp);
                return false;
            }

            loaded_fires[idx] = fire;
            neighbor_counts[idx] = neighbor_count;

            for (int k = 0; k < neighbor_count; k++) {
                if (fscanf(fp, " %d", &neighbor_indices[idx][k]) != 1) {
                    SDL_Log("ERROR: Failed to read neighbor index");
                    fclose(fp);
                    return false;
                }
            }
            fscanf(fp, "\n");
        }

        // Reconnect neighbors
        for (int j = 0; j < fire_count; j++) {
            for (int k = 0; k < neighbor_counts[j]; k++) {
                int neighbor_idx = neighbor_indices[j][k];
                if (neighbor_idx >= 0 && neighbor_idx < fire_count) {
                    add_fire_neighbor_one_way(loaded_fires[j], loaded_fires[neighbor_idx]);
                }
            }
        }

        if (original_level != level_idx) {
            gamestate_switch_level(game, original_level);
        }

        // Read end marker
        char end_marker[16];
        fscanf(fp, "%15s\n", end_marker);
    }

    fclose(fp);
    SDL_Log("Successfully loaded all levels from %s", filename);
    return true;
}
