#include "input.h"
#include "gamestate.h"
#include "input_config.h"
#include <SDL3/SDL.h>
#include <string.h>

void init_input_buffer(InputBuffer *buffer) {
    memset(buffer, 0, sizeof(InputBuffer));
}

void gather_input(InputBuffer *buffer, bool *isRunning) {
    buffer->event_count = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (buffer->event_count >= MAX_INPUT_EVENTS) {
            SDL_Log("Warning: Input buffer full, dropping events");
            break;
        }

        InputEvent *input_event = &buffer->events[buffer->event_count];
        input_event->type = INPUT_NONE;

        switch (event.type) {
            case SDL_EVENT_QUIT:
                *isRunning = false;
                input_event->type = INPUT_QUIT;
                buffer->event_count++;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_F1 || event.key.key == SDLK_0) {
                    input_event->type = INPUT_TOGGLE_MODE;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_A) {
                    input_event->type = INPUT_MOVE_LEFT_DOWN;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_D) {
                    input_event->type = INPUT_MOVE_RIGHT_DOWN;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_W) {
                    input_event->type = INPUT_JUMP;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_S) {
                    input_event->type = INPUT_MOVE_DOWN_DOWN;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_W) {
                    input_event->type = INPUT_MOVE_UP_DOWN;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_L) {
                    input_event->type = INPUT_EDITOR_LOAD;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_P) {
                    input_event->type = INPUT_EDITOR_SAVE;
                    buffer->event_count++;
                }
                break;

            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_A) {
                    input_event->type = INPUT_MOVE_LEFT_UP;
                    buffer->event_count++;
                } else if (event.key.key == SDLK_D) {
                    input_event->type = INPUT_MOVE_RIGHT_UP;
                    buffer->event_count++;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    input_event->type = INPUT_MOUSE_LEFT_CLICK;
                    input_event->mouse_x = event.button.x;
                    input_event->mouse_y = event.button.y;
                    buffer->event_count++;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    input_event->type = INPUT_MOUSE_RIGHT_CLICK;
                    input_event->mouse_x = event.button.x;
                    input_event->mouse_y = event.button.y;
                    buffer->event_count++;
                }
                break;
        }
    }

    SDL_GetMouseState(&buffer->mouse_x, &buffer->mouse_y);

    // BAD: The way we're checking inputs is pretty inconsistent and all over
    // the place. This works for now but can be organized later.
    const bool *key_states = SDL_GetKeyboardState(NULL);
    buffer->space_held = key_states[SDL_SCANCODE_SPACE];
    buffer->arrow_left_held = key_states[SDL_SCANCODE_LEFT];
    buffer->arrow_right_held = key_states[SDL_SCANCODE_RIGHT];
    buffer->arrow_up_held = key_states[SDL_SCANCODE_UP];
    buffer->arrow_down_held = key_states[SDL_SCANCODE_DOWN];
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

    // Update player shooting state based on spacebar
    player->is_shooting_water = input->space_held;
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

                    // Delegate to gamestate to handle the editor logic
                    editor_handle_left_click(state, world_x, world_y);
                }
                break;

            case INPUT_MOUSE_RIGHT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    // Delegate to gamestate to handle the editor logic
                    editor_handle_right_click(state, world_x, world_y);
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
