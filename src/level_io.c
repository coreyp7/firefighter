#include "level_io.h"
#include "fire.h"
#include "pool.h"
#include <SDL3/SDL.h>
#include <stdio.h>

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

bool level_save_fire_layout(GameState *game, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        SDL_Log("ERROR: Failed to open file for writing: %s", filename);
        return false;
    }

    int fire_count = gamestate_get_fire_count(game);
    Fire *fires_buf = gamestate_get_fires_buffer(game);

    // Write header
    fprintf(fp, "FIRE_COUNT %d\n", fire_count);

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

    // Read fire count
    int fire_count;
    if (fscanf(fp, "FIRE_COUNT %d\n", &fire_count) != 1) {
        SDL_Log("ERROR: Failed to read fire count from file");
        fclose(fp);
        return false;
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
    for (int i = current_fire_count - 1; i >= 0; i--) {
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
