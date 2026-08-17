#include "input.h"
#include "gamestate.h"
#include "editor.h"
#include "level_io.h"
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

void processEditorInput(EditorState *editor, GameState *state, InputBuffer *input, float dt) {
    // Process input events
    for (int i = 0; i < input->event_count; i++) {
        InputEvent *event = &input->events[i];

        switch (event->type) {
            case INPUT_MOUSE_LEFT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    editor_handle_left_click(editor, state, world_x, world_y);
                }
                break;

            case INPUT_MOUSE_RIGHT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    editor_handle_right_click(editor, state, world_x, world_y);
                }
                break;

            case INPUT_EDITOR_SAVE:
                level_save_fire_layout(state, "fire_layouts/default.txt");
                break;

            case INPUT_EDITOR_LOAD:
                level_load_fire_layout(state, "fire_layouts/default.txt");
                break;

            default:
                break;
        }
    }

    // Camera movement in editor mode
    editor_update_camera(editor, state, input, dt);
}

void processInput(EditorState *editor, GameState *state, InputBuffer *input, float dt) {
    Player *player = &state->player;

    // Process each input event
    for (int i = 0; i < input->event_count; i++) {
        InputEvent *event = &input->events[i];

        switch (event->type) {
            case INPUT_TOGGLE_MODE:
                if (editor_is_active(editor)) {
                    editor_deactivate(editor, state);
                } else {
                    editor_activate(editor, state);
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
    if (editor_is_active(editor)) {
        processEditorInput(editor, state, input, dt);
    } else {
        processPlayInput(state, input);
    }
}
