#include "level_io.h"
#include "fire.h"
#include "block.h"
#include "pool.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

// Helper function to find the index of a fire in the fires array
static int find_fire_index(GameState *state, Fire *fire) {
    Fire *fires_buf = gamestate_get_fires_buffer(state);
    int fire_count = gamestate_get_fire_count(state);

    for (int i = 0; i < fire_count; i++) {
        if (&fires_buf[i] == fire) {
            return i;
        }
    }
    return -1;
}

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
            fprintf(fp, "%d: %f %f %f %f\n",
                    idx++, blocks[i].x, blocks[i].y,
                    blocks[i].w, blocks[i].h);
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
        int idx;
        float x, y, w, h;
        if (fscanf(fp, "%d: %f %f %f %f\n", &idx, &x, &y, &w, &h) != 5) {
            return false;
        }

        // Temporarily switch to target level if needed
        int original_level = game->current_level_index;
        if (original_level != level_index) {
            gamestate_switch_level(game, level_index);
        }

        gamestate_add_block(game, x, y, w, h);

        // Switch back
        if (original_level != level_index) {
            gamestate_switch_level(game, original_level);
        }
    }

    return true;
}

//  TODO: remove this, we're not gonna need this much logic.
static bool is_levelpack_format(FILE *fp) {
    char buffer[32];
    long pos = ftell(fp);
    bool is_pack = false;

    if (fgets(buffer, sizeof(buffer), fp)) {
        is_pack = (strncmp(buffer, "LEVELPACK_VERSION", 17) == 0);
    }

    fseek(fp, pos, SEEK_SET); // Reset to start
    return is_pack;
}

bool level_save_fire_layout(GameState *game, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        SDL_Log("ERROR: Failed to open file for writing: %s", filename);
        return false;
    }

    int fire_count = gamestate_get_fire_count(game);
    Fire *fires_buf = gamestate_get_fires_buffer(game);

    // Write fire count header
    fprintf(fp, "FIRE_COUNT %d\n", fire_count);

    // Write player spawn position
    fprintf(fp, "PLAYER_SPAWN %f %f\n",
            game->levels[game->current_level_index].player_spawn_x,
            game->levels[game->current_level_index].player_spawn_y);

    // Write blocks
    write_blocks(fp, game, game->current_level_index);

    // Write each fire
    for (int i = 0; i < fire_count; i++) {
        Fire *fire = &fires_buf[i];
        fprintf(fp, "%d: %f %f %f %f %f %d",
                i, fire->x, fire->y, fire->w, fire->h, fire->health,
                fire->neighbors_size);

        // Write neighbor indices
        for (int j = 0; j < fire->neighbors_size; j++) {
            int neighbor_index = find_fire_index(game, fire->neighbors[j]);
            fprintf(fp, " %d", neighbor_index);
        }

        fprintf(fp, "\n");
    }

    fclose(fp);
    SDL_Log("Successfully saved fire layout to %s", filename);
    return true;
}

bool level_load_fire_layout(GameState *game, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        SDL_Log("ERROR: Failed to open file for reading: %s", filename);
        return false;
    }

    // Check if it's a levelpack file
    if (is_levelpack_format(fp)) {
        fclose(fp);
        SDL_Log("Detected levelpack format, use level_load_all_levels() instead");
        return false;
    }

    // Read fire count
    int fire_count;
    if (fscanf(fp, "FIRE_COUNT %d\n", &fire_count) != 1) {
        SDL_Log("ERROR: Failed to read fire count from file");
        fclose(fp);
        return false;
    }

    // Read player spawn position (with backward compatibility)
    float spawn_x, spawn_y;
    if (fscanf(fp, "PLAYER_SPAWN %f %f\n", &spawn_x, &spawn_y) == 2) {
        game->levels[game->current_level_index].player_spawn_x = spawn_x;
        game->levels[game->current_level_index].player_spawn_y = spawn_y;
        SDL_Log("Loaded spawn position: (%f, %f)", spawn_x, spawn_y);
    } else {
        // Backward compatibility: use default if not in file
        SDL_Log("WARNING: No spawn position in file, using default");
        game->levels[game->current_level_index].player_spawn_x = 50.0f;
        game->levels[game->current_level_index].player_spawn_y = 400.0f;
    }

    // Try to read blocks (backward compatibility - might not exist in old files)
    char peek_buffer[32];
    long pos = ftell(fp);
    if (fgets(peek_buffer, sizeof(peek_buffer), fp) &&
        strncmp(peek_buffer, "BLOCK_COUNT", 11) == 0) {
        // Blocks present, reset position and read them
        fseek(fp, pos, SEEK_SET);
        if (!read_blocks(fp, game, game->current_level_index)) {
            SDL_Log("ERROR: Failed to read blocks");
            fclose(fp);
            return false;
        }
    } else {
        // No blocks, reset position to continue with fires
        fseek(fp, pos, SEEK_SET);
    }

    if (fire_count > MAX_FIRES) {
        SDL_Log("WARNING: File contains %d fires, but max is %d. Will truncate.",
                fire_count, MAX_FIRES);
        fire_count = MAX_FIRES;
    }

    SDL_Log("Loading %d fires from %s", fire_count, filename);

    // Clear current fires (this will handle neighbor cleanup via gamestate API)
    Fire *fires_buf = gamestate_get_fires_buffer(game);
    int current_fire_count = gamestate_get_fire_count(game);

    // Remove all existing fires
    //for (int i = current_fire_count - 1; i >= 0; i--) {
    for (int i = MAX_FIRES - 1; i >= 0; i--) {
        if(!fires_buf[i].active) continue;
        gamestate_remove_fire(game, &fires_buf[i]);
    }

    // Temporary storage for neighbor indices
    int neighbor_indices[MAX_FIRES][MAX_NEIGHBORS];
    int neighbor_counts[MAX_FIRES];
    Fire *loaded_fires[MAX_FIRES];

    // First pass: Read fire data and create fires
    for (int i = 0; i < fire_count; i++) {
        int idx, neighbor_count;
        float x, y, w, h, health;

        if (fscanf(fp, "%d: %f %f %f %f %f %d",
                   &idx, &x, &y, &w, &h, &health, &neighbor_count) != 7) {
            SDL_Log("ERROR: Failed to read fire data at index %d", i);
            fclose(fp);
            return false;
        }

        // Use gamestate API to add fire
        Fire *fire = gamestate_add_fire(game, x, y, w, h, health);
        if (fire == NULL) {
            SDL_Log("ERROR: Failed to create fire at index %d", i);
            fclose(fp);
            return false;
        }

        loaded_fires[idx] = fire;
        neighbor_counts[idx] = neighbor_count;

        // Read neighbor indices
        for (int j = 0; j < neighbor_count && j < MAX_NEIGHBORS; j++) {
            if (fscanf(fp, " %d", &neighbor_indices[idx][j]) != 1) {
                SDL_Log("ERROR: Failed to read neighbor index");
                fclose(fp);
                return false;
            }
        }
    }

    // Second pass: Set up neighbor connections using gamestate API
    for (int i = 0; i < fire_count; i++) {
        Fire *fire = loaded_fires[i];

        for (int j = 0; j < neighbor_counts[i]; j++) {
            int neighbor_idx = neighbor_indices[i][j];

            if (neighbor_idx >= 0 && neighbor_idx < fire_count) {
                Fire *neighbor = loaded_fires[neighbor_idx];

                // Use add_fire_neighbor_one_way since we're loading bidirectional
                // relationships that are already stored in the file
                add_fire_neighbor_one_way(fire, neighbor);
            }
        }
    }

    fclose(fp);
    SDL_Log("Successfully loaded fire layout from %s", filename);
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
