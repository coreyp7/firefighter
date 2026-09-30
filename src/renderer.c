#include "renderer.h"
#include "editor.h"
#include "camera.h"
#include "debug.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>

// Global renderer state
SDL_Renderer *renderer = NULL;
SDL_Texture *player_texture = NULL;
SDL_Texture *bush_sprite_sheet = NULL;
SDL_Texture *ground_sprite_sheet = NULL;

// Sprite source rectangles for block rendering
SDL_Rect block_sprites[MAX_BLOCK_SPRITES] = {
    //{0, 0, 969, 552},
    {590, 15, 180, 180}, // Index 0: Middle ground
    {400, 15, 180, 180}, // Index 1: Left Edge
    {770, 15, 180, 180}, // Index 2: Right Edge
    {770, 195, 180, 180} // Index 3: Middle ground filler
    // Add more sprite rectangles here as needed
};

// Helper to load a texture from file
static bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, const char *path) {
    SDL_Surface *img_surface = IMG_Load(path);
    if (!img_surface) {
        SDL_Log("ERROR: Failed to load image %s: %s", path, SDL_GetError());
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, img_surface);
    SDL_DestroySurface(img_surface);

    if (!(*texture)) {
        SDL_Log("ERROR: Failed to create texture from %s: %s", path, SDL_GetError());
        return false;
    }

    return true;
}

bool init_renderer(SDL_Window *window) {
    // Create renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("ERROR: Failed to create renderer: %s", SDL_GetError());
        return false;
    }

    // Load player texture
    if (!loadImage(renderer, &player_texture, "img/player.webp")) {
        SDL_Log("ERROR: Failed to load player texture");
        cleanup_renderer();
        return false;
    }

    // Load bush sprite sheet
    if (!loadImage(renderer, &bush_sprite_sheet, "img/bushes.png")) {
        SDL_Log("ERROR: Failed to load bush sprite sheet");
        cleanup_renderer();
        return false;
    }

    // Load ground sprite sheet
    if (!loadImage(renderer, &ground_sprite_sheet, "img/ground_sprite_sheet.png")) {
        SDL_Log("ERROR: Failed to load ground sprite sheet");
        cleanup_renderer();
        return false;
    }

    SDL_Log("Renderer initialized successfully");
    return true;
}

void cleanup_renderer(void) {
    if (player_texture) {
        SDL_DestroyTexture(player_texture);
        player_texture = NULL;
    }

    if (bush_sprite_sheet) {
        SDL_DestroyTexture(bush_sprite_sheet);
        bush_sprite_sheet = NULL;
    }

    if (ground_sprite_sheet) {
        SDL_DestroyTexture(ground_sprite_sheet);
        ground_sprite_sheet = NULL;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
}

void render_gamestate(EditorState *editor, GameState *state){
    // Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < MAX_BLOCK_AMOUNT; i++) {
        Block *block = &state->blocks_buf[i];
        if (!block->active) continue;


        //render_block(renderer, block_sprites[sprite_id], *block, state->camera);
        render_block(renderer, block->sprite_id, *block, state->camera);
    }
    render_player(renderer, player_texture, &state->player, state->camera);

    // SDL_FRect srcrect = {288, 32, 200, 180};
    // SDL_FRect destrect = {250, 75, 150, 150};
    // SDL_RenderTexture(
    //     renderer,
    //     bush_sprite_sheet,
    //     &srcrect,
    //     &destrect
    // );
    // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    // SDL_RenderRect(renderer, &destrect);

    render_fires(renderer, state);
    render_water_particles(renderer, state);

    // Render editor UI if in editor mode
    if (editor_is_active(editor)) {
        render_editor_ui(renderer, editor, state);
    }
}

void render_player(SDL_Renderer *renderer, SDL_Texture *player_texture, Player *player,
                   Camera camera) {
    SDL_FRect player_rect = {player->x, player->y, 95, 95};
    SDL_FlipMode flipMode = SDL_FLIP_NONE;
    if(!player->is_facing_left){
        flipMode = SDL_FLIP_HORIZONTAL;
    }
    SDL_FPoint newpos = convert_pos_to_camera_pos(camera, player_rect.x, player_rect.y);
    player_rect.x = newpos.x;
    player_rect.y = newpos.y;

    SDL_RenderTextureRotated(renderer, player_texture, NULL, &player_rect, 0.0, NULL, flipMode);
}

void render_water_particles(SDL_Renderer *renderer, GameState *state) {
    SDL_SetRenderDrawColor(renderer, 92, 181, 225, 255);
    //SDL_SetRenderDrawColor(renderer, 150, 250, 0, 255);
    for (int i = 0; i < state->particle_count; i++) {
        if (!state->particles[i].active) {
            continue;
        }

        SDL_FRect frect = {state->particles[i].x, state->particles[i].y, 15.f, 15.f};
        SDL_FPoint newpos = convert_pos_to_camera_pos(state->camera, frect.x, frect.y);
        frect.x = newpos.x;
        frect.y = newpos.y;

        SDL_RenderFillRect(renderer, &frect);
    }
}

void render_block(SDL_Renderer *renderer, int sprite_id, Block block, Camera camera) {
    SDL_FRect rect = {block.x, block.y, block.w, block.h};

    // Get source rectangle and convert to SDL_FRect
    SDL_Rect src_rect = block_sprites[sprite_id];
    SDL_FRect src_frect = {(float)src_rect.x, (float)src_rect.y, (float)src_rect.w, (float)src_rect.h};

    SDL_FPoint newpos = convert_pos_to_camera_pos(camera, rect.x, rect.y);
    rect.x = newpos.x;
    rect.y = newpos.y;

    SDL_RenderTextureRotated(renderer, ground_sprite_sheet, &src_frect, &rect, 0.0, NULL, SDL_FLIP_NONE);
}

void render_fire(SDL_Renderer *renderer, Fire *fire, Camera camera) {
    // if (!is_fire_alive(fire)) {
    //     return;
    // }

    SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};
    SDL_FPoint newpos = convert_pos_to_camera_pos(camera, fire_rect.x, fire_rect.y);
    fire_rect.x = newpos.x;
    fire_rect.y = newpos.y;

    float fire_strength = fire->health / fire->max_health;
    float fire_opacity = 100 * fire_strength;

    SDL_SetRenderDrawColor(renderer, 255, 100, 0, fire_opacity);
    SDL_RenderFillRect(renderer, &fire_rect);

    SDL_SetRenderDrawColor(renderer, 255, 50, 0, fire_opacity);
    SDL_RenderRect(renderer, &fire_rect);
}

void render_fires(SDL_Renderer *renderer, GameState *state) {
    //for (int i = 0; i < state->fire_count; i++) {
    for (int i = 0; i < MAX_FIRES; i++) {
        if(!state->fires_buf[i].active) {
            continue;
        }
        if(state->fires_buf[i].health <= 0) {
            continue;
        }
        render_fire(renderer, &state->fires_buf[i], state->camera);
        if(debug){
            debug_render_fire_health(renderer, &state->fires_buf[i], state->camera);
        }
    }
}

// TODO: this will likely be moved to debug module.
void render_editor_ui(SDL_Renderer *renderer, EditorState *editor, GameState *state) {
    // Render editor controls at top of screen
    float text_x = 10.0f;
    float text_y = 10.0f;
    float line_height = 20.0f;

    EditorMode mode = editor_get_mode(editor);

    if (mode == EDITOR_MODE_FIRE) {
        debug_render_text(renderer, "FIRE EDITOR MODE", text_x, text_y);
        text_y += line_height;
        debug_render_text(renderer, "1: Fire Mode | 2: Block Mode | F1: Exit Editor", text_x, text_y);
        text_y += line_height;
        debug_render_text(renderer, "L-Click: Place/Connect | R-Click: Delete | [/]: Switch Level | L/P: Load/Save", text_x, text_y);
    } else {
        debug_render_text(renderer, "BLOCK EDITOR MODE", text_x, text_y);
        text_y += line_height;
        debug_render_text(renderer, "1: Fire Mode | 2: Block Mode | F1: Exit Editor", text_x, text_y);
        text_y += line_height;

        char sprite_text[128];
        snprintf(sprite_text, sizeof(sprite_text), "L-Click: Place | R-Click: Delete | ,/.: Sprite ID (%d) | [/]: Switch Level | L/P: Load/Save", editor->current_sprite_id);
        debug_render_text(renderer, sprite_text, text_x, text_y);
    }

    float world_x = state->player.cursor_x + state->camera.x;
    float world_y = state->player.cursor_y + state->camera.y;

    if (mode == EDITOR_MODE_FIRE) {
        // Fire editor mode rendering
        Fire *selected_fire = editor_get_selected_fire(editor);
        if (selected_fire != NULL) {
            SDL_FRect fire_rect = {
                selected_fire->x,
                selected_fire->y,
                selected_fire->w,
                selected_fire->h
            };

            SDL_FPoint newpos = convert_pos_to_camera_pos(
                state->camera, fire_rect.x, fire_rect.y
            );
            fire_rect.x = newpos.x;
            fire_rect.y = newpos.y;

            // Draw cyan highlight with thicker border
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            for (int i = -2; i <= 2; i++) {
                SDL_FRect outline = {
                    fire_rect.x + i, fire_rect.y + i,
                    fire_rect.w - 2*i, fire_rect.h - 2*i
                };
                SDL_RenderRect(renderer, &outline);
            }
        }

        // Highlight fire under cursor with white outline
        Fire *fire_at_cursor = editor_get_fire_at_position(state, world_x, world_y);

        if (fire_at_cursor != NULL) {
            SDL_FRect fire_rect = {
                fire_at_cursor->x,
                fire_at_cursor->y,
                fire_at_cursor->w,
                fire_at_cursor->h
            };

            SDL_FPoint newpos = convert_pos_to_camera_pos(
                state->camera, fire_rect.x, fire_rect.y
            );
            fire_rect.x = newpos.x;
            fire_rect.y = newpos.y;

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderRect(renderer, &fire_rect);
        } else {
            // Draw fire placement preview at cursor
            SDL_FRect preview_rect = {
                state->player.cursor_x - 25,
                state->player.cursor_y - 25,
                50, 50
            };

            SDL_SetRenderDrawColor(renderer, 255, 100, 0, 100);
            SDL_RenderFillRect(renderer, &preview_rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
            SDL_RenderRect(renderer, &preview_rect);
        }
    } else {
        // Block editor mode rendering
        Block *selected_block = editor_get_selected_block(editor);
        if (selected_block != NULL) {
            SDL_FRect block_rect = {
                selected_block->x,
                selected_block->y,
                selected_block->w,
                selected_block->h
            };

            SDL_FPoint newpos = convert_pos_to_camera_pos(
                state->camera, block_rect.x, block_rect.y
            );
            block_rect.x = newpos.x;
            block_rect.y = newpos.y;

            // Draw cyan highlight with thicker border
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            for (int i = -2; i <= 2; i++) {
                SDL_FRect outline = {
                    block_rect.x + i, block_rect.y + i,
                    block_rect.w - 2*i, block_rect.h - 2*i
                };
                SDL_RenderRect(renderer, &outline);
            }
        }

        // Highlight block under cursor with white outline
        Block *block_at_cursor = editor_get_block_at_position(state, world_x, world_y);

        if (block_at_cursor != NULL) {
            SDL_FRect block_rect = {
                block_at_cursor->x,
                block_at_cursor->y,
                block_at_cursor->w,
                block_at_cursor->h
            };

            SDL_FPoint newpos = convert_pos_to_camera_pos(
                state->camera, block_rect.x, block_rect.y
            );
            block_rect.x = newpos.x;
            block_rect.y = newpos.y;

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderRect(renderer, &block_rect);
        } else {
            // Draw block placement preview at cursor (250x250 default size)
            // Show the actual sprite that will be placed
            SDL_FRect preview_rect = {
                state->player.cursor_x - 125,
                state->player.cursor_y - 125,
                250, 250
            };

            // Get source rectangle for the selected sprite
            SDL_Rect src_rect = block_sprites[editor->current_sprite_id];
            SDL_FRect src_frect = {(float)src_rect.x, (float)src_rect.y, (float)src_rect.w, (float)src_rect.h};

            // Render the sprite with semi-transparency
            SDL_SetTextureAlphaMod(ground_sprite_sheet, 150);
            SDL_RenderTextureRotated(renderer, ground_sprite_sheet, &src_frect, &preview_rect, 0.0, NULL, SDL_FLIP_NONE);
            SDL_SetTextureAlphaMod(ground_sprite_sheet, 255);

            // Draw white outline
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
            SDL_RenderRect(renderer, &preview_rect);
        }
    }
}
