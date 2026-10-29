/* Munkiki's Castles, the Nokia 3410's Java game (MIDP 1.0 with the Nokia
   UI and M3D APIs), re-implemented in C from a decompile of its JAR,
   whose pictures, levels, text and sounds are read from the JAR at build
   time (tools/extract_munkiki.py).

   On the phone the game runs in the KVM, reached from Games, More games,
   as "Munkiki", with its own menu (New game, High scores, Instructions,
   Options) drawn by the phone's MIDP lists; its timer ticks every 100 ms.
   The port keeps the whole of it behind this interface: the screens are
   drawn here, the menus in the phone's list style; the port's menus only
   open it, hand it keys and ticks and see when it closes.

   The world, its rules and Munkiki's moves follow the JAR's code (class d)
   and are checked tick for tick against the JAR run in a harness; the
   picture is the port's own rasterizer, following the JAR's geometry and
   camera (class c; the phone's M3D rasterizer was not read); the flow of
   screens follows the JAR's class KKM. See docs/munkiki.md. */
#ifndef CORE_MUNKIKI_H
#define CORE_MUNKIKI_H

#include <stdint.h>

/* The Game Boy keeps the game in banks of its own: what is called from
   outside them is banked (MUNKIKI_BANKED), and Munkiki's frames are in a
   bank apart from the rest, reached through MK_SHEET_FAR. */
#ifdef MUNKIKI_BANKED
#define MK_FAR __banked
#define MK_SHEET_FAR __banked
#else
#define MK_FAR
#define MK_SHEET_FAR
#endif

/* The keys, as the phone's keypad: the digits 0 to 9, then #, *, the C key
   (the right soft key: Back) and the Navi key (the left soft key: Select),
   and the scroll keys. */
enum {
    MK_KEY_0 = 0,
    MK_KEY_HASH = 10,
    MK_KEY_STAR = 11,
    MK_KEY_BACK = 12,
    MK_KEY_SELECT = 13,
    MK_KEY_UP = 14,
    MK_KEY_DOWN = 15
};

/* What the calls report. */
#define MK_DRAW 1   /* the screen changed: call munkiki_draw */
#define MK_CLOSED 2 /* the game ended itself (Back on its menu) */
#define MK_RECORD 4 /* its record changed: save it */

/* Starts the game, as the phone does from More games: its title first. */
void munkiki_open(void) MK_FAR;
/* A key went down or came up. */
uint8_t munkiki_key(uint8_t key, uint8_t down) MK_FAR;
/* A console button went down or came up, as menu.h numbers them (up,
   down, A, B, left, right, Start, Select): on the game's lists and pages
   A selects and B goes back; in the game the D-pad is 2, 8, 4 and 6, A
   is 5 (push), B is 0 (hit), Start is the C key (the pause list) and
   Select is # (look around). */
uint8_t munkiki_button(uint8_t button, uint8_t down) MK_FAR;
/* Lets this much time pass, in microseconds; a tick is every 100 ms. */
uint8_t munkiki_elapse(uint16_t us) MK_FAR;
/* One tick, for scripted runs. */
uint8_t munkiki_tick(void) MK_FAR;
/* Draws the screen into the LCD view. */
void munkiki_draw(void) MK_FAR;

/* What the port keeps for the game between power cycles, in its settings
   record: the level reached (1 to 30), and the two top scores. */
struct munkiki_record {
    uint16_t top, second;
    uint8_t level_reached;
    uint8_t sounds, lights; /* the game's own Options */
};
void munkiki_record_set(const struct munkiki_record *in) MK_FAR;
void munkiki_record_get(struct munkiki_record *out) MK_FAR;

/* For the host's tools: the state the harness logs, written as text, and
   a list's entry chosen by index, as the harness does. */
#ifndef __SDCC
int munkiki_state_text(char *out, int size);
void munkiki_select(uint8_t entry);
#endif

#endif
