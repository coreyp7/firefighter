#ifndef DEBUG_H
#define DEBUG_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void init_debug(SDL_Renderer *renderer);
void debug_render(SDL_Renderer *renderer, float frame_time_ms, int active_particles);
void cleanup_debug(void);

#endif
