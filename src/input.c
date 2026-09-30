#include "input.h"
#include "gamestate.h"
#include "editor.h"
#include "level_io.h"
#include "input_config.h"
#include "debug.h"
#include "hashmap.h"
#include <SDL3/SDL.h>
#include <string.h>

// Handler function type - called after input type is set
typedef void (*InputHandler)(InputEvent *event, SDL_Event *sdl_event);

// Default handler - no special processing needed
static void default_handler(InputEvent *event, SDL_Event *sdl_event) {
    (void)event;
    (void)sdl_event;
}

// Mouse handler - captures click coordinates
static void mouse_handler(InputEvent *event, SDL_Event *sdl_event) {
    event->mouse_x = sdl_event->button.x;
    event->mouse_y = sdl_event->button.y;
}

// Mapping structure for input bindings
typedef struct {
    InputType input_type;
    bool requires_debug;
    InputHandler handler;
} InputMapping;

// Hash maps for O(1) input lookups
static HashMap key_down_map;
static HashMap key_up_map;
static HashMap mouse_map;

void init_input_buffer(InputBuffer *buffer) {
    memset(buffer, 0, sizeof(InputBuffer));
}

void init_input_maps(void) {
    // Initialize hash maps
    hashmap_init(&key_down_map, 32, sizeof(SDL_Keycode), sizeof(InputMapping),
                 hashmap_hash_int, hashmap_key_equals_default);
    hashmap_init(&key_up_map, 32, sizeof(SDL_Keycode), sizeof(InputMapping),
                 hashmap_hash_int, hashmap_key_equals_default);
    hashmap_init(&mouse_map, 8, sizeof(Uint8), sizeof(InputMapping),
                 hashmap_hash_int, hashmap_key_equals_default);

    // Populate key down mappings
    SDL_Keycode key;
    InputMapping mapping;

    // Editor mode toggle (debug only)
    key = SDLK_F1;
    mapping = (InputMapping){ INPUT_TOGGLE_MODE, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_0;
    mapping = (InputMapping){ INPUT_TOGGLE_MODE, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    // Player movement (always available)
    key = SDLK_A;
    mapping = (InputMapping){ INPUT_MOVE_LEFT_DOWN, false, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_D;
    mapping = (InputMapping){ INPUT_MOVE_RIGHT_DOWN, false, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_W;
    mapping = (InputMapping){ INPUT_JUMP, false, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_S;
    mapping = (InputMapping){ INPUT_MOVE_DOWN_DOWN, false, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    // Editor controls (debug only)
    key = SDLK_L;
    mapping = (InputMapping){ INPUT_EDITOR_LOAD, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_P;
    mapping = (InputMapping){ INPUT_EDITOR_SAVE, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_1;
    mapping = (InputMapping){ INPUT_EDITOR_MODE_FIRE, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_2;
    mapping = (InputMapping){ INPUT_EDITOR_MODE_BLOCK, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_LEFTBRACKET;
    mapping = (InputMapping){ INPUT_LEVEL_PREV, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    key = SDLK_RIGHTBRACKET;
    mapping = (InputMapping){ INPUT_LEVEL_NEXT, true, default_handler };
    hashmap_put(&key_down_map, &key, &mapping);

    // Populate key up mappings
    key = SDLK_A;
    mapping = (InputMapping){ INPUT_MOVE_LEFT_UP, false, default_handler };
    hashmap_put(&key_up_map, &key, &mapping);

    key = SDLK_D;
    mapping = (InputMapping){ INPUT_MOVE_RIGHT_UP, false, default_handler };
    hashmap_put(&key_up_map, &key, &mapping);

    // Populate mouse mappings
    Uint8 button;

    button = SDL_BUTTON_LEFT;
    mapping = (InputMapping){ INPUT_MOUSE_LEFT_CLICK, true, mouse_handler };
    hashmap_put(&mouse_map, &button, &mapping);

    button = SDL_BUTTON_RIGHT;
    mapping = (InputMapping){ INPUT_MOUSE_RIGHT_CLICK, true, mouse_handler };
    hashmap_put(&mouse_map, &button, &mapping);
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

            case SDL_EVENT_KEY_DOWN: {
                SDL_Keycode pressed_key = event.key.key;
                InputMapping *mapping = (InputMapping*)hashmap_get(&key_down_map, &pressed_key);

                if (mapping && (!mapping->requires_debug || debug)) {
                    input_event->type = mapping->input_type;
                    mapping->handler(input_event, &event);
                    buffer->event_count++;
                }
                break;
            }

            case SDL_EVENT_KEY_UP: {
                SDL_Keycode released_key = event.key.key;
                InputMapping *mapping = (InputMapping*)hashmap_get(&key_up_map, &released_key);

                if (mapping && (!mapping->requires_debug || debug)) {
                    input_event->type = mapping->input_type;
                    mapping->handler(input_event, &event);
                    buffer->event_count++;
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                Uint8 pressed_button = event.button.button;
                InputMapping *mapping = (InputMapping*)hashmap_get(&mouse_map, &pressed_button);

                if (mapping && (!mapping->requires_debug || debug)) {
                    input_event->type = mapping->input_type;
                    mapping->handler(input_event, &event);
                    buffer->event_count++;
                }
                break;
            }
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

                    if (editor_get_mode(editor) == EDITOR_MODE_FIRE) {
                        editor_handle_left_click(editor, state, world_x, world_y);
                    } else {
                        editor_handle_block_left_click(editor, state, world_x, world_y);
                    }
                }
                break;

            case INPUT_MOUSE_RIGHT_CLICK:
                {
                    // Convert screen coordinates to world coordinates
                    float world_x = event->mouse_x + state->camera.x;
                    float world_y = event->mouse_y + state->camera.y;

                    if (editor_get_mode(editor) == EDITOR_MODE_FIRE) {
                        editor_handle_right_click(editor, state, world_x, world_y);
                    } else {
                        editor_handle_block_right_click(editor, state, world_x, world_y);
                    }
                }
                break;

            case INPUT_EDITOR_SAVE:
                //level_save_fire_layout(state, "fire_layouts/default.txt");
                level_save_all_levels(state, "fire_layouts/levelpack.txt");
                break;

            case INPUT_EDITOR_LOAD:
                //level_load_fire_layout(state, "fire_layouts/default.txt");
                level_load_all_levels(state, "fire_layouts/levelpack.txt");
                break;

            case INPUT_EDITOR_MODE_FIRE:
                editor_set_mode_fire(editor);
                break;

            case INPUT_EDITOR_MODE_BLOCK:
                editor_set_mode_block(editor);
                break;

            case INPUT_LEVEL_PREV:
                if (state->current_level_index > 0) {
                    gamestate_switch_level(state, state->current_level_index - 1);
                }
                break;

            case INPUT_LEVEL_NEXT:
                if (state->current_level_index < MAX_LEVELS - 1) {
                    gamestate_switch_level(state, state->current_level_index + 1);
                }
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
