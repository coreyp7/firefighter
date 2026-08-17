#include "editor.h"
#include "gamestate.h"
#include "camera.h"
#include <SDL3/SDL.h>

void editor_init(EditorState *editor) {
    editor->selected_fire = NULL;
    editor->is_active = false;
    editor->camera_move_speed = 10.0f;
}

void editor_activate(EditorState *editor, GameState *game) {
    editor->is_active = true;
    editor->selected_fire = NULL;
    SDL_Log("Switched to FIRE EDITOR mode");
}

void editor_deactivate(EditorState *editor, GameState *game) {
    editor->is_active = false;
    editor->selected_fire = NULL;

    // Reset camera to follow player when switching back to play mode
    game->camera.x = game->player.x - (game->camera.w / 2);
    game->camera.y = game->player.y - (game->camera.h / 2);

    SDL_Log("Switched to PLAY mode");
}

void editor_handle_left_click(EditorState *editor, GameState *game, float world_x, float world_y) {
    Fire *clicked_fire = gamestate_find_fire_at_position(game, world_x, world_y);

    if (clicked_fire != NULL) {
        if (editor->selected_fire == NULL) {
            // Clicked new fire, set it as selected
            editor->selected_fire = clicked_fire;
            SDL_Log("Fire selected for neighbor connection");
        } else if (editor->selected_fire == clicked_fire) {
            // Clicked same fire, deselect it
            editor->selected_fire = NULL;
            SDL_Log("Fire deselected");
        } else {
            // Create neighbor connection between fires
            // TODO: prevent ability to prevent duplicates, either here or in
            // the gamestate functions (prolly the latter).
            if (gamestate_connect_fires(game, editor->selected_fire, clicked_fire)) {
                SDL_Log("Connected fires as neighbors");
            }
            editor->selected_fire = NULL;
        }
    } else {
        // Clicked on nothing, place new fire
        Fire *fire = gamestate_add_fire(game, world_x - 25, world_y - 25, 50, 50, 10);

        if (fire != NULL) {
            SDL_Log("Placed new fire at (%f, %f)", world_x, world_y);
        }

        editor->selected_fire = NULL;
    }
}

void editor_handle_right_click(EditorState *editor, GameState *game, float world_x, float world_y) {
    Fire *clicked_fire = gamestate_find_fire_at_position(game, world_x, world_y);

    if (clicked_fire != NULL) {
        if (editor->selected_fire == clicked_fire) {
            editor->selected_fire = NULL;
        }

        // Remove fire (gamestate handles neighbor cleanup automatically)
        gamestate_remove_fire(game, clicked_fire);

        SDL_Log("Deleted fire");
    }
}

void editor_update_camera(EditorState *editor, GameState *game, InputBuffer *input, float dt) {
    (void)dt;  // Unused right now

    // Camera movement in editor mode
    if (input->arrow_left_held) {
        game->camera.x -= editor->camera_move_speed;
    }
    if (input->arrow_right_held) {
        game->camera.x += editor->camera_move_speed;
    }
    if (input->arrow_up_held) {
        game->camera.y -= editor->camera_move_speed;
    }
    if (input->arrow_down_held) {
        game->camera.y += editor->camera_move_speed;
    }
}

Fire* editor_get_fire_at_position(GameState *game, float world_x, float world_y) {
    return gamestate_find_fire_at_position(game, world_x, world_y);
}

Fire* editor_get_selected_fire(EditorState *editor) {
    return editor->selected_fire;
}

bool editor_is_active(EditorState *editor) {
    return editor->is_active;
}
