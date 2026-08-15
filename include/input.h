#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#define MAX_INPUT_EVENTS 64

typedef enum InputType {
    INPUT_NONE,
    INPUT_QUIT,

    // Mode switching
    INPUT_TOGGLE_MODE,

    // Player movement
    INPUT_MOVE_LEFT_DOWN,
    INPUT_MOVE_LEFT_UP,
    INPUT_MOVE_RIGHT_DOWN,
    INPUT_MOVE_RIGHT_UP,
    // names are slightly confusing but fine for now
    INPUT_MOVE_UP_DOWN,
    INPUT_MOVE_UP_UP,
    INPUT_MOVE_DOWN_DOWN,
    INPUT_MOVE_DOWN_UP,

    INPUT_JUMP,

    // Actions
    INPUT_SHOOT_WATER,

    // Editor
    INPUT_MOUSE_LEFT_CLICK,
    INPUT_MOUSE_RIGHT_CLICK,
    INPUT_EDITOR_SAVE,
    INPUT_EDITOR_LOAD,

    // Camera movement (editor mode)
    INPUT_CAMERA_LEFT,
    INPUT_CAMERA_RIGHT,
    INPUT_CAMERA_UP,
    INPUT_CAMERA_DOWN,
} InputType;

// Why do we need mouse x/y here when its already in our InputBuffer?
typedef struct InputEvent {
    InputType type;
    float mouse_x;
    float mouse_y;
} InputEvent;

typedef struct InputBuffer {
    InputEvent events[MAX_INPUT_EVENTS];
    int event_count;
    float mouse_x;
    float mouse_y;
    bool space_held;

    bool arrow_left_held;
    bool arrow_right_held;
    bool arrow_up_held;
    bool arrow_down_held;
} InputBuffer;

void init_input_buffer(InputBuffer *buffer);
void gather_input(InputBuffer *buffer, bool *isRunning);

#endif
