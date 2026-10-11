#include "player_animation.h"
#include <math.h>


// This file is largely just configuration of each animation (see
// animation_sequences).

static AnimationFrame standing_frames[] = {
    { .sprite_index = 0, .duration_ms = 0 }  // Still frame
};

const int duration = 100;

static AnimationFrame running_frames[] = {
    { .sprite_index = 0, .duration_ms = duration },
    { .sprite_index = 1, .duration_ms = duration },
    { .sprite_index = 2, .duration_ms = duration },
    { .sprite_index = 3, .duration_ms = duration }
};

static AnimationFrame jumping_frames[] = {
    { .sprite_index = 1, .duration_ms = 0 }
};

static AnimationFrame falling_frames[] = {
    { .sprite_index = 2, .duration_ms = 0 }
};

// State -> Animation mapping
static AnimationSequence animation_sequences[5] = {
    [PLAYER_STATE_STANDING] = {
        .frames = standing_frames,
        .frame_count = 1,
        .loop = false
    },
    [PLAYER_STATE_RUNNING_LEFT] = {
        .frames = running_frames,
        .frame_count = 4,
        .loop = true
    },
    [PLAYER_STATE_RUNNING_RIGHT] = {
        .frames = running_frames,
        .frame_count = 4,
        .loop = true
    },
    [PLAYER_STATE_JUMPING] = {
        .frames = jumping_frames,
        .frame_count = 1,
        .loop = false
    },
    [PLAYER_STATE_FALLING] = {
        .frames = falling_frames,
        .frame_count = 1,
        .loop = false
    }
};

int get_player_sprite_index(const Player *player) {
    AnimationSequence *anim = &animation_sequences[player->state];

    // Bounds check
    if (player->current_frame >= anim->frame_count) {
        return anim->frames[0].sprite_index;
    }

    return anim->frames[player->current_frame].sprite_index;
}

void update_player_animation(Player *player, float dt) {
    AnimationSequence *anim = &animation_sequences[player->state];

    if (anim->frame_count == 1 && anim->frames[0].duration_ms == 0) {
        return;
    }

    float dt_ms = dt * 1000.0f;
    player->frame_timer_ms += dt_ms;

    // Check if we should advance to next frame
    AnimationFrame *current = &anim->frames[player->current_frame];
    if (player->frame_timer_ms >= current->duration_ms) {
        player->frame_timer_ms = 0.0f;
        player->current_frame++;

        // Handle looping
        if (player->current_frame >= anim->frame_count) {
            if (anim->loop) {
                player->current_frame = 0;
            } else {
                player->current_frame = anim->frame_count - 1;
            }
        }
    }
}
