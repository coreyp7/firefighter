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
#include "input.h"

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
void processInput(GameState *state, InputBuffer *input);

//const float PLAYER_WALK_SPEED = 500.f;
const float PLAYER_WALK_SPEED = 250.f;
const float PLAYER_JUMP_FORCE = 350.f;
const float EDITOR_CAMERA_SPEED = 500.f;
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

    InputBuffer input_buffer;
    init_input_buffer(&input_buffer);

    SDL_ShowWindow(window);
    init_water_particles();
    init_debug(renderer);

    while(isRunning){
        uint32 start_ticks = SDL_GetTicks();

        gather_input(&input_buffer, &isRunning);

        processInput(&state, &input_buffer);

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

void processPlayInput(GameState *state, InputBuffer *input);
void processEditorInput(GameState *state, InputBuffer *input);

// TODO: these should be moved into input.c.
void processInput(GameState *state, InputBuffer *input) {
    Player *player = &state->player;

    // Process each input event
    for (int i = 0; i < input->event_count; i++) {
        InputEvent *event = &input->events[i];

        switch (event->type) {
            case INPUT_TOGGLE_MODE:
                if (state->current_mode == MODE_PLAY) {
                    state->current_mode = MODE_FIRE_EDITOR;
                    state->selected_fire = NULL;
                    SDL_Log("Switched to FIRE EDITOR mode");
                } else {
                    state->current_mode = MODE_PLAY;
                    state->selected_fire = NULL;
                    // Reset camera to follow player when switching back to play mode
                    state->camera.x = state->player.x - (state->camera.w / 2);
                    state->camera.y = state->player.y - (state->camera.h / 2);
                    SDL_Log("Switched to PLAY mode");
                }
                break;

            default:
                // Other events handled by mode-specific functions
                break;
        }
    }

    // Update player cursor with mouse (both modes need this)
    player->cursor_x = input->mouse_x;
    player->cursor_y = input->mouse_y;

    // Branch based on current mode
    if (state->current_mode == MODE_PLAY) {
        processPlayInput(state, input);
    } else if (state->current_mode == MODE_FIRE_EDITOR) {
        processEditorInput(state, input);
    }
}

void processPlayInput(GameState *state, InputBuffer *input) {
    Player *player = &state->player;

    // Process input events
    for (int i = 0; i < input->event_count; i++) {
        InputEvent *event = &input->events[i];

        switch (event->type) {
            case INPUT_MOVE_LEFT_DOWN:
                player->xvel = -PLAYER_WALK_SPEED;
                break;

            case INPUT_MOVE_RIGHT_DOWN:
                player->xvel = PLAYER_WALK_SPEED;
                break;

            case INPUT_MOVE_LEFT_UP:
            case INPUT_MOVE_RIGHT_UP:
                player->xvel = 0;
                break;

            case INPUT_JUMP:
                if (state->player.is_grounded) {
                    player->yvel -= PLAYER_JUMP_FORCE;
                }
                break;

            default:
                break;
        }
    }

    // Continuous water shooting (spacebar held)
    if (input->space_held) {
        SDL_FPoint player_relative_pos = convert_pos_to_camera_pos(state->camera, player->x, player->y);
        float dx = player->cursor_x - player_relative_pos.x;
        float dy = player->cursor_y - player_relative_pos.y;
        float angle = atan2f(dx, dy);
        shoot_water_particle(state, player->x, player->y, angle);
    }
}

// Helper function to get fire at world position
Fire* get_fire_at_position(GameState *state, float world_x, float world_y) {
    for (int i = 0; i < state->fire_count; i++) {
        Fire *fire = &state->fires[i];

        SDL_FRect fire_rect = {fire->x, fire->y, fire->w, fire->h};

        // Check if point is inside fire rectangle
        if (world_x >= fire_rect.x && world_x <= fire_rect.x + fire_rect.w &&
            world_y >= fire_rect.y && world_y <= fire_rect.y + fire_rect.h) {
            return fire;
        }
    }
    return NULL;
}

// BAD: This thing needs to be split up.
void processEditorInput(GameState *state, InputBuffer *input) {
    // Process input events

    // NOTE: this is fps based
    // TODO: move this into some constant
    const float camera_move_speed = 10.0f;
    for (int i = 0; i < input->event_count; i++) {
        InputEvent *event = &input->events[i];

        switch (event->type) {
            case INPUT_MOUSE_LEFT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    Fire *clicked_fire = get_fire_at_position(state, world_x, world_y);

                    if (clicked_fire != NULL) {
                        // Clicked on existing fire
                        if (state->selected_fire == NULL) {
                            // First click - select this fire
                            state->selected_fire = clicked_fire;
                            SDL_Log("Fire selected for neighbor connection");
                        } else if (state->selected_fire == clicked_fire) {
                            // Clicked same fire - deselect
                            state->selected_fire = NULL;
                            SDL_Log("Fire deselected");
                        } else {
                            // Second click - create neighbor connection
                            // BUG: segfault if we add more than the max allowed
                            // neighbors. Add a check here.
                            add_fire_neighbor(state->selected_fire, clicked_fire);
                            add_fire_neighbor(clicked_fire, state->selected_fire);
                            SDL_Log("Connected fires as neighbors");
                            state->selected_fire = NULL;
                        }
                    } else {
                        // Clicked on empty space - place new fire
                        if (state->fire_count < MAX_FIRES) {
                            init_fire(&state->fires[state->fire_count],
                                     world_x - 25, world_y - 25, 50, 50, 50);
                            state->fire_count++;
                            SDL_Log("Placed new fire at (%f, %f)", world_x, world_y);
                            state->selected_fire = NULL;
                        } else {
                            SDL_Log("Max fires reached!");
                        }
                    }
                }
                break;

            case INPUT_MOUSE_RIGHT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    Fire *clicked_fire = get_fire_at_position(state, world_x, world_y);
                    // THIS IS BROKEN RIGHT NOW, impl pool allocator.
                    //
                    // if (clicked_fire != NULL) {
                    //     // Remove this fire's neighbor references from all other fires
                    //     for (int i = 0; i < state->fire_count; i++) {
                    //         Fire *other_fire = &state->fires[i];
                    //         for (int j = 0; j < other_fire->neighbors_size; j++) {
                    //             if (other_fire->neighbors[j] == clicked_fire) {
                    //                 // Shift neighbors array
                    //                 for (int k = j; k < other_fire->neighbors_size - 1; k++) {
                    //                     other_fire->neighbors[k] = other_fire->neighbors[k + 1];
                    //                 }
                    //                 other_fire->neighbors_size--;
                    //                 break;
                    //             }
                    //         }
                    //     }
                    //
                    //     // Clear selected_fire if it was this fire
                    //     if (state->selected_fire == clicked_fire) {
                    //         state->selected_fire = NULL;
                    //     }
                    //
                    //     SDL_Log("Deleted fire");
                    // }
                }
                break;

            // These are fine for now.
            case INPUT_EDITOR_SAVE:
                save_fire_layout(state, "fire_layouts/default.txt");
                SDL_Log("Saved fire layout to fire_layouts/default.txt");
                break;

            case INPUT_EDITOR_LOAD:
                load_fire_layout(state, "fire_layouts/default.txt");
                SDL_Log("Loaded fire layout from fire_layouts/default.txt");
                break;

            default:
                break;
        }
    }

    // Camera movement in editor mode (continuous, based on arrow keys)
    if (input->arrow_left_held) {
        state->camera.x -= camera_move_speed;
    }
    if (input->arrow_right_held) {
        state->camera.x += camera_move_speed;
    }
    if (input->arrow_up_held) {
        state->camera.y -= camera_move_speed;
    }
    if (input->arrow_down_held) {
        state->camera.y += camera_move_speed;
    }
}

