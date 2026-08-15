#include "debug.h"
#include "camera.h"
#include "pool.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

static TTF_Font *debug_font = NULL;

void init_debug(SDL_Renderer *renderer) {
    (void)renderer;

    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return;
    }

    debug_font = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", 16);
    if (!debug_font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
    }
}

static int get_text_width(const char *text) {
    if (!debug_font) {
        return 0;
    }

    int width = 0;
    TTF_MeasureString(debug_font, text, strlen(text), 0, &width, NULL);
    return width;
}

static void render_text(SDL_Renderer *renderer, const char *text, float x, float y) {
    if (!debug_font) {
        return;
    }

    SDL_Color color = {255, 255, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(debug_font, text, strlen(text), color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }

    int text_width = surface->w;
    int text_height = surface->h;
    SDL_DestroySurface(surface);

    SDL_FRect dest_rect = {
        x,
        y,
        (float)text_width,
        (float)text_height
    };

    SDL_RenderTexture(renderer, texture, NULL, &dest_rect);
    SDL_DestroyTexture(texture);
}

static void render_frame_time_text(SDL_Renderer *renderer, float frame_time_ms, float x_offset, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Frame: %.2f ms", frame_time_ms);
    render_text(renderer, text, x_offset, y_offset);
}

static void render_particle_count_text(SDL_Renderer *renderer, int active_particles, float x_offset, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Water: %d", active_particles);
    render_text(renderer, text, x_offset, y_offset);
}

static void render_player_position_text(SDL_Renderer *renderer, float player_x, float player_y, float x_offset, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Player: (%.1f, %.1f)", player_x, player_y);
    render_text(renderer, text, x_offset, y_offset);
}

static void render_fire_count_text(SDL_Renderer *renderer, int fire_count, int max_fires, float x_offset, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Fires: %d/%d", fire_count, max_fires);
    render_text(renderer, text, x_offset, y_offset);
}

static int count_free_pool_blocks(Pool *pool) {
    int free_count = 0;
    Pool_Node *current = pool->head;
    while (current != NULL) {
        free_count++;
        current = current->next;
    }
    return free_count;
}

static void render_pool_capacity_text(SDL_Renderer *renderer, Pool *pool, float x_offset, float y_offset) {
    int free_blocks = count_free_pool_blocks(pool);
    char text[64];
    snprintf(text, sizeof(text), "Pool Free: %d", free_blocks);
    render_text(renderer, text, x_offset, y_offset);
}

void debug_render(SDL_Renderer *renderer, GameState *state, float frame_time_ms) {
    if (!debug_font) {
        return;
    }

    // Extract values from GameState
    float player_x = state->player.x;
    float player_y = state->player.y;

    // Count active particles
    int active_particles = 0;
    for (int i = 0; i < state->particle_count; i++) {
        if (state->particles[i].active) {
            active_particles++;
        }
    }

    // Build all text strings
    char frame_text[64];
    char particle_text[64];
    char player_text[64];
    char fire_text[64];
    char pool_text[64];

    snprintf(frame_text, sizeof(frame_text), "Frame: %.2f ms", frame_time_ms);
    snprintf(particle_text, sizeof(particle_text), "Water: %d", active_particles);
    snprintf(player_text, sizeof(player_text), "Player: (%.1f, %.1f)", player_x, player_y);
    snprintf(fire_text, sizeof(fire_text), "Fires: %d/%d", state->fire_count, MAX_FIRES);

    int free_blocks = count_free_pool_blocks(&state->fires_pool);
    snprintf(pool_text, sizeof(pool_text), "Pool Free: %d", free_blocks);

    // Calculate max width
    int max_width = 0;
    int width;

    // TODO: change these to loop through a list instead.
    width = get_text_width(frame_text);
    if (width > max_width) max_width = width;

    width = get_text_width(particle_text);
    if (width > max_width) max_width = width;

    width = get_text_width(player_text);
    if (width > max_width) max_width = width;

    width = get_text_width(fire_text);
    if (width > max_width) max_width = width;

    width = get_text_width(pool_text);
    if (width > max_width) max_width = width;

    // Calculate x offset to align to right edge with 10px padding
    float x_offset = 1080.0f - max_width - 10.0f;
    float y_offset = 10.0f;
    float line_height = 20.0f;

    // Render all text
    // TODO: write a generic function for drawing the text.
    // Then loop through and draw them all generically.
    render_frame_time_text(renderer, frame_time_ms, x_offset, y_offset);
    y_offset += line_height;

    render_particle_count_text(renderer, active_particles, x_offset, y_offset);
    y_offset += line_height;

    render_player_position_text(renderer, player_x, player_y, x_offset, y_offset);
    y_offset += line_height;

    render_fire_count_text(renderer, state->fire_count, MAX_FIRES, x_offset, y_offset);
    y_offset += line_height;

    render_pool_capacity_text(renderer, &state->fires_pool, x_offset, y_offset);

    // Render fire neighbor connections
    debug_render_fire_neighbors(renderer, state);

    // Render fire hover info (memory address and neighbors)
    debug_render_fire_hover_info(renderer, state);
}

void debug_render_fire_health(SDL_Renderer *renderer, Fire *fire, Camera camera) {
    // if (!is_fire_alive(fire)) {
    //     return;
    // }

    // Convert fire position to camera space
    SDL_FPoint fire_screen_pos = convert_pos_to_camera_pos(camera, fire->x, fire->y);

    // Health bar dimensions
    float bar_width = fire->w;
    float bar_height = 5.0f;
    float bar_x = fire_screen_pos.x;
    float bar_y = fire_screen_pos.y - 10.0f; // 10px above fire

    // Background bar (red/dark)
    SDL_FRect bg_rect = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bg_rect);

    // Health bar (green)
    float health_percent = fire->health / fire->max_health;
    SDL_FRect health_rect = {bar_x, bar_y, bar_width * health_percent, bar_height};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &health_rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &bg_rect);
}

static Fire* get_fire_at_screen_position(GameState *state, float mouse_x, float mouse_y) {
    // Convert screen coordinates to world coordinates
    float world_x = mouse_x + state->camera.x;
    float world_y = mouse_y + state->camera.y;

    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires_buf[i];
        SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};

        if (world_x >= fire_rect.x && world_x <= fire_rect.x + fire_rect.w &&
            world_y >= fire_rect.y && world_y <= fire_rect.y + fire_rect.h) {
            return fire;
        }
    }
    return NULL;
}

void debug_render_fire_hover_info(SDL_Renderer *renderer, GameState *state) {
    if (!debug_font) {
        return;
    }

    Fire *hovered_fire = get_fire_at_screen_position(state, state->player.cursor_x, state->player.cursor_y);

    if (hovered_fire == NULL) {
        return;
    }

    // Render fire memory address and neighbor info in top-left corner
    float x = 10.0f;
    float y = 10.0f;
    float line_height = 20.0f;

    // Render memory address
    char addr_text[128];
    snprintf(addr_text, sizeof(addr_text), "Fire Addr: %p", (void*)hovered_fire);
    render_text(renderer, addr_text, x, y);
    y += line_height;

    // Render neighbors header
    char neighbors_header[64];
    snprintf(neighbors_header, sizeof(neighbors_header), "Neighbors (%d):", hovered_fire->neighbors_size);
    render_text(renderer, neighbors_header, x, y);
    y += line_height;

    // Render each neighbor address
    for (int i = 0; i < hovered_fire->neighbors_size; i++) {
        char neighbor_text[128];
        snprintf(neighbor_text, sizeof(neighbor_text), "  [%d] %p", i, (void*)hovered_fire->neighbors[i]);
        render_text(renderer, neighbor_text, x, y);
        y += line_height;
    }

    // Also render the memory address directly on the fire in the world
    SDL_FPoint fire_screen_pos = convert_pos_to_camera_pos(state->camera, hovered_fire->x, hovered_fire->y);
    char fire_addr_text[32];
    snprintf(fire_addr_text, sizeof(fire_addr_text), "%p", (void*)hovered_fire);
    render_text(renderer, fire_addr_text, fire_screen_pos.x, fire_screen_pos.y - 30.0f);
}

void debug_render_fire_neighbors(SDL_Renderer *renderer, GameState *state) {
    // Use yellow color for neighbor connection lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);

    // Iterate through all fires
    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires_buf[i];

        // Calculate fire center in world space
        float fire_center_x = fire->x + fire->w / 2.0f;
        float fire_center_y = fire->y + fire->h / 2.0f;

        // Convert to camera space
        SDL_FPoint fire_screen_pos = convert_pos_to_camera_pos(
            state->camera, fire_center_x, fire_center_y
        );

        // Draw lines to each neighbor
        for (int j = 0; j < fire->neighbors_size; j++) {
            Fire *neighbor = fire->neighbors[j];

            // Avoid drawing duplicate lines for bidirectional neighbors
            // Only draw if current fire pointer < neighbor pointer
            if (fire >= neighbor) {
                continue;
            }

            // Calculate neighbor center in world space
            float neighbor_center_x = neighbor->x + neighbor->w / 2.0f;
            float neighbor_center_y = neighbor->y + neighbor->h / 2.0f;

            // Convert to camera space
            SDL_FPoint neighbor_screen_pos = convert_pos_to_camera_pos(
                state->camera, neighbor_center_x, neighbor_center_y
            );

            // Draw line between fire centers
            SDL_RenderLine(
                renderer,
                fire_screen_pos.x, fire_screen_pos.y,
                neighbor_screen_pos.x, neighbor_screen_pos.y
            );
        }
    }
}

void cleanup_debug(void) {
    if (debug_font) {
        TTF_CloseFont(debug_font);
        debug_font = NULL;
    }
    TTF_Quit();
}
