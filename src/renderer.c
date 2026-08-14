#include "renderer.h"
#include "camera.h"
#include "debug.h"
#include <SDL3/SDL.h>

void render_gamestate(GameState *state){
    // Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < state->block_count; i++) {
        render_block(renderer, block_sprite, state->blocks[i], state->camera);
    }
    render_player(renderer, player_texture, &state->player, state->camera);

    SDL_FRect srcrect = {288, 32, 200, 180};
    SDL_FRect destrect = {250, 75, 150, 150};
    SDL_RenderTexture(
        renderer,
        bush_sprite_sheet,
        &srcrect,
        &destrect
    );
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &destrect);

    render_fires(renderer, state);
    render_water_particles(renderer, state);

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
        render_fire(renderer, &state->fires[i], state->camera);
        debug_render_fire_health(renderer, &state->fires[i], state->camera);
    }
}
