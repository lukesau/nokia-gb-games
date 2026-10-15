/* Memory. Pairs of pictures face down on a board of 2x2 up to 10x6 cards on
   an 8 px pitch; a try turns two of them up, and a pair that matches stays
   up. The fewer tries, the higher the score. */
#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stdint.h>

#define MEMORY_LEVELS 5
#define MEMORY_MAX_CARDS 60 /* the biggest board, 10x6 */
#define MEMORY_FACE_UP 0x80

enum {
    MEMORY_NONE_UP,    /* no unmatched card is face up */
    MEMORY_ONE_UP,     /* the first card of a try is face up */
    MEMORY_TWO_UP,     /* two cards that differ are face up until the next key */
    MEMORY_TURNED_BACK /* that key turned them back; lasts until it is handled */
};

struct memory {
    uint8_t cols, rows;
    uint8_t x, y;             /* the cursor */
    uint8_t first_x, first_y; /* the first card of the try */
    uint8_t state;
    uint8_t pairs;            /* pairs still to find */
    uint16_t tries;
    uint16_t score;           /* set when the game ends */
    /* Card (x, y) is byte x * rows + y: its picture, 1 to 0x49, with
       MEMORY_FACE_UP set while it shows. */
    uint8_t board[MEMORY_MAX_CARDS];
};

extern struct memory memory;

/* Deals a new game at level 0..4. */
void memory_init(uint8_t level);

/* A phone key: 2/4/6/8 move the cursor, wrapping at the edges; 5 turns the
   card under it up, or on a card already up jumps like #; # and * jump to
   the next and the previous card still face down. Returns nonzero when the
   last pair has been found: memory.score is then the tries left out of the
   seven for every four cards allowed, or 0 when none were left. */
uint8_t memory_key(char key);

/* Draws the board; the cursor's card is drawn inverted when `blink` is
   set, which the phone alternates about twice a second. */
void memory_draw(uint8_t blink);

/* The same picture, drawing only the cards that changed since the last
   memory_draw or memory_draw_changes. */
void memory_draw_changes(uint8_t blink);

#endif
