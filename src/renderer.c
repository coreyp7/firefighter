#include "renderer.h"
#include "editor.h"
#include "camera.h"
#include "debug.h"
#include <SDL3/SDL.h>

void render_gamestate(EditorState *editor, GameState *state){
    // Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < state->block_count; i++) {
        render_block(renderer, block_sprite, state->blocks_buf[i], state->camera);
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

void render_block(SDL_Renderer *renderer, SDL_Texture *texture, Block block, Camera camera) {
    SDL_FRect rect = {block.x, block.y, block.w, block.h};
    SDL_FPoint newpos = convert_pos_to_camera_pos(camera, rect.x, rect.y);
    rect.x = newpos.x;
    rect.y = newpos.y;
    SDL_RenderTextureRotated(renderer, texture, NULL, &rect, 0.0, NULL, SDL_FLIP_NONE);
    //SDL_Log("drew block at pos(%f, %f)\n", rect.x, rect.y);
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
    for (int i = 0; i < state->fire_count; i++) {
        if(!state->fires_buf[i].active) {
            continue;
        }
        if(state->fires_buf[i].health <= 0) {
            continue;
        }
        render_fire(renderer, &state->fires_buf[i], state->camera);
        debug_render_fire_health(renderer, &state->fires_buf[i], state->camera);
    }
}

void render_editor_ui(SDL_Renderer *renderer, EditorState *editor, GameState *state) {
    // Mode indicator showing current editor mode
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
    SDL_FRect mode_indicator = {(720/2), 5, 150, 20};
    SDL_RenderFillRect(renderer, &mode_indicator);
    // TODO: Add text rendering to show "FIRE EDITOR" or "BLOCK EDITOR"

    float world_x = state->player.cursor_x + state->camera.x;
    float world_y = state->player.cursor_y + state->camera.y;

    EditorMode mode = editor_get_mode(editor);

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
            SDL_FRect preview_rect = {
                state->player.cursor_x - 125,
                state->player.cursor_y - 125,
                250, 250
            };

            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 100);
            SDL_RenderFillRect(renderer, &preview_rect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
            SDL_RenderRect(renderer, &preview_rect);
        }
    }
}
