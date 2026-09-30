#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include "gamestate.h"
#include "input.h"

typedef enum EditorMode {
    EDITOR_MODE_FIRE,
    EDITOR_MODE_BLOCK
} EditorMode;

typedef struct EditorState {
    Fire *selected_fire;
    Block *selected_block;
    // TODO: rename this I don't love the verbage here
    bool is_active; // is editor mode active?
    float camera_move_speed; // this is lazily fps based for now
    EditorMode current_mode;
    int current_sprite_id; // sprite_id for next placed block (0-6)
} EditorState;

// Lifecycle
void editor_init(EditorState *editor);
void editor_activate(EditorState *editor, GameState *game);
void editor_deactivate(EditorState *editor, GameState *game);

// Operations
void editor_handle_left_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_handle_right_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_handle_block_left_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_handle_block_right_click(EditorState *editor, GameState *game, float world_x, float world_y);
void editor_update_camera(EditorState *editor, GameState *game, InputBuffer *input, float dt);

// Mode management
void editor_set_mode_fire(EditorState *editor);
void editor_set_mode_block(EditorState *editor);
EditorMode editor_get_mode(EditorState *editor);

// Queries
Fire* editor_get_fire_at_position(GameState *game, float world_x, float world_y);
Fire* editor_get_selected_fire(EditorState *editor);
Block* editor_get_block_at_position(GameState *game, float world_x, float world_y);
Block* editor_get_selected_block(EditorState *editor);
bool editor_is_active(EditorState *editor);

#endif
