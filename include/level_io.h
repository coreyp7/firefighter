#ifndef LEVEL_IO_H
#define LEVEL_IO_H

#include <stdbool.h>
#include "gamestate.h"

// Levelpack format (multi-level save/load)
bool level_save_all_levels(GameState *game, const char *filename);
bool level_load_all_levels(GameState *game, const char *filename);

#endif
