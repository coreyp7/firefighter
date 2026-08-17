#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <math.h>

#include "gamestate.h"
#include "editor.h"
#include "water_particles.h"
#include "camera.h"
#include "debug.h"
#include "renderer.h"
#include "input.h"
#include "pool.h"

typedef uint32_t uint32;

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char* path);

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

    EditorState editor;
    editor_init(&editor);

    float dt = 0.0;
    uint32 last_state_update = SDL_GetTicks();

    InputBuffer input_buffer;
    init_input_buffer(&input_buffer);

    SDL_ShowWindow(window);
    init_water_particles();
    init_debug(renderer);

    while(isRunning){
        uint32 start_ticks = SDL_GetTicks();

        gather_input(&input_buffer, &isRunning);

        processInput(&editor, &state, &input_buffer, dt);

        dt = (SDL_GetTicks() - last_state_update) / 1000.f;
        last_state_update = SDL_GetTicks();
        simulate_gamestate(&state, dt);

        // Update camera to follow player in play mode
        // TODO: make this lerp instead of instant movement.
        if (!editor_is_active(&editor)) {
            state.camera.x = state.player.x - (state.camera.w / 2);
            state.camera.y = state.player.y - (state.camera.h / 2);
        }

        render_gamestate(&editor, &state);

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

