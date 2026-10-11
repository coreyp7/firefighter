#ifndef PLAYER_ANIMATION_H
#define PLAYER_ANIMATION_H

#include "gamestate.h"

typedef struct AnimationFrame {
    int sprite_index; // Index into player_sprite_sheet_rects[]
    float duration_ms; // Frame duration in milliseconds (0 = still frame)
} AnimationFrame;

typedef struct AnimationSequence {
    AnimationFrame *frames;
    int frame_count;
    bool loop; // Whether animation loops
} AnimationSequence;

int get_player_sprite_index(const Player *player);
void update_player_animation(Player *player, float dt);

#endif
