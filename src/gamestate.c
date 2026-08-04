#include "gamestate.h"
#include "water_particles.h"
#include <math.h>

static void update_player(Player *player, float dt);

void init_gamestate(GameState *state) {
    state->player.x = 50.0;
    state->player.y = 400.0;
    state->player.xvel = 0.0;
    state->player.yvel = 0.0;
    state->player.cursor_x = 0.0;
    state->player.cursor_y = 0.0;

    state->particle_count = 0;
    for (int i = 0; i < MAX_WATER_PARTICLES; i++) {
        state->particles[i].active = false;
    }
}

void simulate_gamestate(GameState *state, float dt) {
    update_player(&state->player, dt);
    simulate_water_particles(state, dt);
}

void cleanup_gamestate(GameState *state) {
    state->particle_count = 0;
}

static void update_player(Player *player, float dt) {
    player->yvel += 150 * dt;

    player->x += player->xvel * dt;
    player->y += player->yvel * dt;
}
