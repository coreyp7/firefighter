#ifndef LEVEL_IO_H
#define LEVEL_IO_H

#include <stdbool.h>
#include "gamestate.h"

bool level_save_fire_layout(GameState *game, const char *filename);
bool level_load_fire_layout(GameState *game, const char *filename);

#endif
