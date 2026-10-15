/* Bantumi, after the Nokia 3310's (NHM-5 v6.39): six-pit mancala against
   the phone.

   As the other games, one handler taking an event and a context
   (core/game.h); core/games.c drives it. It draws with the sprite layer
   (core/sprite.h) and makes and frees its sprites in the firmware's own
   order, which decides what is drawn over what. The computer's search is
   the firmware's, its two mistakes kept, and runs a hundred steps a tick
   as on the phone. The pictures are read from the user's dump; the map it
   follows is docs/games_bantumi_3310.md in the MAME fork. */
#ifndef CORE_BANTUMI_H
#define CORE_BANTUMI_H

#include <stdint.h>

#include "game.h"

/* The levels the menus offer: ctx->level is 1 to BANTUMI_LEVELS. Only the
   computer's search depth changes with it, and the hint (*) is there on
   the first. */
#define BANTUMI_LEVELS 5

/* The one sound: a bean dropped. */
#define BANTUMI_SOUND_BEAN 0x1b

/* The game ends with GAME_RESULT_END and ctx->score the player's beans
   less the phone's, as a signed number. */

int bantumi_handler(int event, struct game_context *ctx);

/* Draws the sprites into sprite_screen; a platform whose pictures are not
   always in reach (the Game Boy's banks) calls it with them mapped. */
void bantumi_render(void);

#endif
