#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include "gamestate.h"
#include "camera.h"
#include "fire.h"

// Forward declarations
typedef struct EditorState EditorState;


SDL_Renderer *renderer;
SDL_Texture *player_texture;
SDL_Texture *block_sprite;
SDL_Texture *bush_sprite_sheet;

void render_gamestate(EditorState *editor, GameState *state);
void render_player(SDL_Renderer *renderer, SDL_Texture *player_texture, Player *player, Camera camera);
void render_water_stream(SDL_Renderer *renderer, GameState *state);
void render_water_particles(SDL_Renderer *renderer, GameState *state);
void render_block(SDL_Renderer *renderer, SDL_Texture *texture, Block block, Camera camera);
void render_fire(SDL_Renderer *renderer, Fire *fire, Camera camera);
void render_fires(SDL_Renderer *renderer, GameState *state);
void render_editor_ui(SDL_Renderer *renderer, EditorState *editor, GameState *state);

#endif
