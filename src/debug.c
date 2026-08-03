#include "debug.h"
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

static void render_frame_time_text(SDL_Renderer *renderer, float frame_time_ms, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Frame: %.2f ms", frame_time_ms);

    // Render at top right with 10px padding from right edge
    render_text(renderer, text, 1080.0f - 200.0f, y_offset);
}

static void render_particle_count_text(SDL_Renderer *renderer, int active_particles, float y_offset) {
    char text[64];
    snprintf(text, sizeof(text), "Water: %d", active_particles);

    // Render at top right with 10px padding from right edge
    render_text(renderer, text, 1080.0f - 200.0f, y_offset);
}

void debug_render(SDL_Renderer *renderer, float frame_time_ms, int active_particles) {
    if (!debug_font) {
        return;
    }

    float y_offset = 10.0f;
    float line_height = 20.0f;

    render_frame_time_text(renderer, frame_time_ms, y_offset);
    y_offset += line_height;

    render_particle_count_text(renderer, active_particles, y_offset);
}

void cleanup_debug(void) {
    if (debug_font) {
        TTF_CloseFont(debug_font);
        debug_font = NULL;
    }
    TTF_Quit();
}
