#include "debug.h"
#include "camera.h"
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

    snprintf(frame_text, sizeof(frame_text), "Frame: %.2f ms", frame_time_ms);
    snprintf(particle_text, sizeof(particle_text), "Water: %d", active_particles);
    snprintf(player_text, sizeof(player_text), "Player: (%.1f, %.1f)", player_x, player_y);

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

    // Render fire neighbor connections
    debug_render_fire_neighbors(renderer, state);
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

void debug_render_fire_neighbors(SDL_Renderer *renderer, GameState *state) {
    // Use yellow color for neighbor connection lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);

    // Iterate through all fires
    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires[i];

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
