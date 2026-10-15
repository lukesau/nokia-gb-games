/* What the shared GBA layer (common/platform/gba) asks of the 3210's and
   5110's core, as of the other ports' core/games.h. */
#ifndef CORE_GAMES_H
#define CORE_GAMES_H

#include "game.h"

/* One unit of the phone's timers, for the sounds. */
#define GAMES_UNIT_US GAME_TICK_US
/* No game options (Sounds, Lights, Shakes) to save: the 3310's and
   3410's. */
#define GAMES_OPTIONS 0

#endif
