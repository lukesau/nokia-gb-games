/* Bantumi, after the Nokia 3310's (NHM-5 v6.39) and the 3410's (NHM-2
   v5.46): six-pit mancala against the phone.

   The two phones' games are one game: the same rules, the same hand
   going from pit to pit, the same search for the phone's move with the
   same two mistakes, a hundred steps a tick. They draw differently: the
   3310 with its sprite list (core/sprite.h), the hands as a mask and a
   picture; the 3410 with its graphics library (core/si_pic.h), on a board
   12 pixels wider and 17 taller, with a title, a board that opens and
   closes, and a pass on the first turn. Each phone's core/bantumi_phone.h
   says which it is and where its data is, read from the user's dump. The
   maps this follows are docs/games_bantumi_3310.md and
   docs/games_bantumi_3410.md in the MAME fork. */
#ifndef BANTUMI_H
#define BANTUMI_H

#include <stdint.h>

#include "bantumi_phone.h"

/* The levels the menus offer: 1 to BANTUMI_LEVELS. Only the computer's
   search depth changes with it, and the hint (*) is there on the first. */
#define BANTUMI_LEVELS 5

#ifdef BANTUMI_TREE

/* The 3410: one handler for the title, the game and the Instructions'
   demos, called with the phone's events. */
#ifdef BANTUMI_BANKED
#define BANTUMI_FAR __banked
#else
#define BANTUMI_FAR
#endif

enum {
    BANTUMI_EVENT_TICK = 0x00,
    BANTUMI_EVENT_KEY_DOWN = 0x01, /* a: the key's code, 0..9 or these */
    BANTUMI_EVENT_KEY_UP = 0x02,
    BANTUMI_EVENT_PAUSE = 0x03,    /* Menu: the game is put away for Continue */
    BANTUMI_EVENT_DEMO = 0x09,     /* a: BANTUMI_DEMO_ */
    BANTUMI_EVENT_NEW_GAME = 0x0a, /* from bantumi_level and bantumi_beans */
    BANTUMI_EVENT_START = 0x0c,    /* after New game: the board comes in */
    BANTUMI_EVENT_CONTINUE = 0x0d,
    BANTUMI_EVENT_TITLE = 0x0e     /* chosen in Select game */
};

/* The Instructions' demos, each after its text: one move sown; the keys
   shown on a keypad; finishing in the big pot; the opponent's big pot
   skipped; a capture. A demo closes the game when the phone's turn comes;
   a pause ends it early. */
enum {
    BANTUMI_DEMO_SOW = 1,
    BANTUMI_DEMO_KEYS = 2,
    BANTUMI_DEMO_BIG_POT = 3,
    BANTUMI_DEMO_SKIP = 4,
    BANTUMI_DEMO_CAPTURE = 5
};

#define BANTUMI_KEY_STAR 10 /* a hint, on the first level */
#define BANTUMI_KEY_HASH 11 /* on the first turn: the phone moves first */

/* What the handler asks of the framework, which draws the picture again
   after every event. */
enum {
    BANTUMI_DONE_CLOSE = 1 /* back to the game's menu */
};

/* The sounds, as codes in the phone's sound table (0x3f7d0e, less 3): a
   bean dropped; each tick of the pause at the game's end, won or not. */
#define BANTUMI_SOUND_BEAN 0x17     /* 0xfa9 */
#define BANTUMI_SOUND_LOST 0x21     /* 0xfa2 */
#define BANTUMI_SOUND_WON 0x23      /* 0xfa4 */
/* The last sound the last event asked for, 0 for none; the caller plays
   it and clears it. */
extern uint8_t bantumi_sound;

/* The phone's two settings, read at New game: the level, and the beans in
   each pit at the start (4 unless the phone's record says otherwise). */
extern uint8_t bantumi_level, bantumi_beans;

/* The tick period the game has asked for, in ms: the title's steps vary,
   the game's are 100. */
extern uint16_t bantumi_period;

uint8_t bantumi_event(uint8_t event, uint8_t a) BANTUMI_FAR;

/* Draws the picture into sprite_screen: all of it, or only if it has
   changed since it was last drawn. */
void bantumi_render(uint8_t all) BANTUMI_FAR;

#else

#include "game.h"

/* The 3310: driven by core/games.c, as the other games are; ctx->level is
   1 to BANTUMI_LEVELS. */

/* The one sound: a bean dropped. */
#define BANTUMI_SOUND_BEAN 0x1b

/* The game ends with GAME_RESULT_END and ctx->score the player's beans
   less the phone's, as a signed number. */

int bantumi_handler(int event, struct game_context *ctx);

/* Draws the sprites into sprite_screen; a platform whose pictures are not
   always in reach (the Game Boy's banks) calls it with them mapped. */
void bantumi_render(void);

#endif

#endif
