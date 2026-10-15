/* Logic. Mastermind: a secret row of four or five figures to guess in up to
   ten tries, drawn as columns across the screen, the newest at the right.
   A try is scored above the divider: a rectangle for a right figure in the
   right place, a line for a right figure in the wrong place. The figures
   are drawn with Memory's tiles, ten of them picked at random for the game.
   The firmware carries the game complete but dispatches to it from
   nowhere; the phone's text block has its name and instructions. */
#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdint.h>

/* Levels 0..3 have four figures in the code, 4..7 five; within each the
   figures come from 4, 6, 8 or 10 kinds. */
#define LOGIC_LEVELS 8
#define LOGIC_ROWS 10 /* tries on the screen; older ones scroll off */
#define LOGIC_CODE 5  /* cells per try in the board array */
#define LOGIC_PALETTE 10
#define LOGIC_WON 0xff /* the score of a right guess */

struct logic {
    uint8_t level;
    uint8_t row, col;           /* the cursor: the try being made, the cell in it */
    uint8_t last_row, last_col; /* the cell being copied from, in an earlier try;
                                   the cursor itself when not copying */
    uint8_t held;               /* the cursor cell's own figure while copying */
    /* Try r, cell c at r * LOGIC_CODE + c: 0 for blank, else the figure,
       1 to the number of kinds. */
    uint8_t board[LOGIC_ROWS * LOGIC_CODE];
    /* Per try: right places * 16 + right figures elsewhere; LOGIC_WON for
       the winning try. One more than the tries: the firmware's scroll
       clears the entry after the last. */
    uint8_t scores[LOGIC_ROWS + 1];
    uint8_t secret[LOGIC_CODE];
    uint8_t palette[LOGIC_PALETTE]; /* the tile each figure is drawn as */
    uint8_t won;
};

extern struct logic logic;

/* New game at level 0..7. */
void logic_init(uint8_t level);

/* Figures in the code, and the kinds they come from, at the level. */
uint8_t logic_code_length(void);
uint8_t logic_kinds(void);

/* A phone key. 2 and 8 move the cursor up and down the try; 5 changes the
   figure under it to the next kind, blank after the last. 4 copies: the
   first press takes the figure from the cell above the cursor in the try
   before, each further press from one try earlier, and past the first try
   it gives the cursor's own figure back; 2 and 8 meanwhile move the cell
   copied from, and 5 keeps what is shown. * checks the try and moves on to
   the next, scrolling when the ten are used. Returns nonzero when the try
   was right: the game is won and logic.won is set. */
uint8_t logic_key(char key);

/* The score of a guess against the secret, as kept in logic.scores. */
uint8_t logic_score(const uint8_t *guess);

/* The whole board; the cursor's cell is drawn inverted when `blink` is
   set, which the phone alternates about twice a second. */
void logic_draw(uint8_t blink);

#endif
