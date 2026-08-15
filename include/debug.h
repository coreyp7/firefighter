#ifndef DEBUG_H
#define DEBUG_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "fire.h"
#include "camera.h"
#include "gamestate.h"

void init_debug(SDL_Renderer *renderer);
void debug_render(SDL_Renderer *renderer, GameState *state, float frame_time_ms);
void debug_render_fire_health(SDL_Renderer *renderer, Fire *fire, Camera camera);
void debug_render_fire_neighbors(SDL_Renderer *renderer, GameState *state);
void debug_render_fire_hover_info(SDL_Renderer *renderer, GameState *state);
void cleanup_debug(void);

#endif
