#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <math.h>

#include "gamestate.h"
#include "water_particles.h"
#include "camera.h"
#include "debug.h"
#include "renderer.h"

typedef uint32_t uint32;

/**
Immediate todo:
- draw relative to the camera
- add gameobject for a bush
    - can be on fire
    - show animation if on fire
- allow player to put out fires
    - puts out animation
    - damage numbers when hitting it lol
    - maybe a small splash effect
*/

/**
Future state:
- shooting water should have a tangible physics feedback for gamefeel and fun.
Allow player to gain speed by shooting water behind them.
- idea: allow player to walk on water shot? Maybe a different material could
allow the player to walk on it, powerup?
*/

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char* path);
void processInput(GameState *state, bool *isRunning);

//const float PLAYER_WALK_SPEED = 500.f;
const float PLAYER_WALK_SPEED = 250.f;
const float PLAYER_JUMP_FORCE = 350.f;
const int WINDOW_HEIGHT = 720;
const int WINDOW_WIDTH = 1080;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    initSDL();

    SDL_Window *window = SDL_CreateWindow("ff", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        cleanupSDL();
        return 1;
    }
    bool isRunning = true;

    // TODO: move renderer init into renderer module
    renderer = SDL_CreateRenderer(window, NULL);
    player_texture = NULL;

    if (!loadImage(renderer, &player_texture, "img/player.webp")) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 1;
    }

    bush_sprite_sheet = NULL;
    if (!loadImage(renderer, &bush_sprite_sheet, "img/bushes.png")) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 2;
    }

    block_sprite = NULL;
    if (!loadImage(renderer, &block_sprite, "img/block.png")) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 2;
    }

    GameState state;
    init_gamestate(&state, WINDOW_WIDTH, WINDOW_HEIGHT);
    float dt = 0.0;
    uint32 last_state_update = SDL_GetTicks();

    SDL_ShowWindow(window);
    init_water_particles();
    init_debug(renderer);

    while(isRunning){
        uint32 start_ticks = SDL_GetTicks();

        processInput(&state, &isRunning);

        dt = (SDL_GetTicks() - last_state_update) / 1000.f;
        last_state_update = SDL_GetTicks();
        simulate_gamestate(&state, dt);

        render_gamestate(&state);

        // vsync
        uint32 time_of_frame = SDL_GetTicks() - start_ticks;
        debug_render(renderer, &state, (float)time_of_frame);

        SDL_RenderPresent(renderer);
        uint32 required_length_of_frame = 1000.0 / 60.0; // 60 fps
        if(time_of_frame < required_length_of_frame){
            uint32 time_to_wait = required_length_of_frame - time_of_frame;
            SDL_Delay(time_to_wait);
        }
    }

    SDL_DestroyTexture(player_texture);
    SDL_DestroyWindow(window);
    cleanupSDL();
    cleanup_gamestate(&state);
    cleanup_water_particles();
    cleanup_debug();

    return 0;
}

bool initSDL(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }

    // sdl3 doesn't require init anymore? look into this.
    // int imgFlags = IMG_INIT_PNG;
    // if (!(IMG_Init(imgFlags) & imgFlags)) {
    //     SDL_Quit();
    //     return false;
    // }

    return true;
}

void cleanupSDL(void) {
    //IMG_Quit();
    SDL_Quit();
}

bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char *path) {
    SDL_Surface *img_surface = IMG_Load(path);
    if (!img_surface) {
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, img_surface);
    if (!(*texture)) {
        SDL_DestroySurface(img_surface);
        return false;
    }

    SDL_DestroySurface(img_surface);
    return true;
}

void processInput(GameState *state, bool *isRunning) {
    Player *player = &state->player;
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EVENT_QUIT:
                *isRunning = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if(event.key.key == SDLK_D) {
                    player->xvel = PLAYER_WALK_SPEED;
                } else if(event.key.key == SDLK_A) {
                    player->xvel = -PLAYER_WALK_SPEED;
                }
                if(event.key.key == SDLK_W && state->player.is_grounded){
                    player->yvel -= PLAYER_JUMP_FORCE;
                }
                break;
            case SDL_EVENT_KEY_UP:
                if(event.key.key == SDLK_D || event.key.key == SDLK_A) {
                    player->xvel = 0;
                }
                break;
        }
    }


    // Update player cursor with mouse
    float x, y;
    SDL_GetMouseState(&x, &y);
    player->cursor_x = x;
    player->cursor_y = y;

    // spacebar/water shoot
    const bool *key_states = SDL_GetKeyboardState(NULL);
    if (key_states[SDL_SCANCODE_SPACE]) {
        SDL_FPoint player_relative_pos = convert_pos_to_camera_pos(state->camera, player->x, player->y);
        // float dx = player->cursor_x - player->x;
        // float dy = player->cursor_y - player->y;
        float dx = player->cursor_x - player_relative_pos.x;
        float dy = player->cursor_y - player_relative_pos.y;
        float angle = atan2f(dx, dy);
        shoot_water_particle(state, player->x, player->y, angle);
    }
}

