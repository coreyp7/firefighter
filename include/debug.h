#ifndef DEBUG_H
#define DEBUG_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "fire.h"
#include "camera.h"

void init_debug(SDL_Renderer *renderer);
void debug_render(SDL_Renderer *renderer, float frame_time_ms, int active_particles, float player_x, float player_y);
void debug_render_fire_health(SDL_Renderer *renderer, Fire *fire, Camera camera);
void cleanup_debug(void);

#endif
