#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include "gamestate.h"
#include "input.h"

typedef struct EditorState {
    Fire *selected_fire;
    // TODO: rename this I don't love the verbage here
    bool is_active; // is editor mode active?
    float camera_move_speed; // this is lazily fps based for now
} EditorState;

// Lifecycle
void editor_init(EditorState *editor);
void editor_activate(EditorState *editor, GameState *game);
void editor_deactivate(EditorState *editor, GameState *game);

// Operations
void editor_handle_left_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_handle_right_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_update_camera(EditorState *editor, GameState *game, InputBuffer *input, float dt);

// Queries
Fire* editor_get_fire_at_position(GameState *game, float world_x, float world_y);
Fire* editor_get_selected_fire(EditorState *editor);
bool editor_is_active(EditorState *editor);

#endif
