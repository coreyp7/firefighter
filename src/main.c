#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "gamestate.h"
#include "editor.h"
#include "water_particles.h"
#include "camera.h"
#include "debug.h"
#include "renderer.h"
#include "input.h"
#include "pool.h"
#include "random_utils.h"
#include "level_io.h"

typedef uint32_t uint32;

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char* path);

const int WINDOW_HEIGHT = 720;
const int WINDOW_WIDTH = 1080;

// extern from debug.c
bool debug = false;

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug = true;
            SDL_Log("Debug mode enabled");
        }
    }

    initSDL();
    random_init();

    SDL_Window *window = SDL_CreateWindow("ff", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        cleanupSDL();
        return 1;
    }

    // Initialize renderer and load all textures
    if (!init_renderer(window)) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 2;
    }

    bool isRunning = true;
    GameState state;
    init_gamestate(&state, WINDOW_WIDTH, WINDOW_HEIGHT);

    EditorState editor;
    editor_init(&editor);

    float dt = 0.0;
    uint32 last_state_update = SDL_GetTicks();

    InputBuffer input_buffer;
    init_input_buffer(&input_buffer);
    init_input_maps();

    SDL_ShowWindow(window);
    init_water_particles();
    if (debug) {
        init_debug(renderer);
    }

    level_load_all_levels(&state, "fire_layouts/levelpack.txt");

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
        // TODO: add this var and extern so renderer can access
        if(debug){
            debug_render(renderer, &state, (float)time_of_frame);
        }

        SDL_RenderPresent(renderer);
        uint32 required_length_of_frame = 1000.0 / 60.0; // 60 fps
        if(time_of_frame < required_length_of_frame){
            uint32 time_to_wait = required_length_of_frame - time_of_frame;
            SDL_Delay(time_to_wait);
        }
    }

    cleanup_renderer();
    SDL_DestroyWindow(window);
    cleanupSDL();
    cleanup_gamestate(&state);
    cleanup_water_particles();
    if (debug) {
        cleanup_debug();
    }

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

