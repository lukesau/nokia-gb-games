/* Snake. The phone's board is 20x11 cells on a 4 px pitch; the snake is a
   ring of 2-bit directions walked from the tail to the head. A bigger board,
   up to what the framebuffer holds, gives the full-screen variant. */
#ifndef CORE_SNAKE_H
#define CORE_SNAKE_H

#include <stdint.h>

#include "lcd.h"

#define SNAKE_COLS 20 /* the phone's board */
#define SNAKE_ROWS 11
/* The biggest board the framebuffer holds: 4 px a cell plus the border. */
#define SNAKE_MAX_COLS ((LCD_FB_WIDTH - 3) / 4)
#define SNAKE_MAX_ROWS ((LCD_FB_HEIGHT - 3) / 4)
/* The full-screen board fills the part of the framebuffer the platform
   shows for it: all of it, or the middle 1/LCD_ZOOM of it magnified (cut to
   whole 8x8 cells). It is one cell smaller each way than would fit, which
   leaves room to put the same margin on every side (to within the odd
   pixel: the board's outline is always an odd number of pixels across). */
#define SNAKE_AREA_WIDTH (LCD_FB_WIDTH / LCD_ZOOM)
#define SNAKE_AREA_HEIGHT (LCD_FB_HEIGHT / LCD_ZOOM)
#define SNAKE_AREA_X ((LCD_FB_WIDTH - SNAKE_AREA_WIDTH) / 2 / 8 * 8)
#define SNAKE_AREA_Y ((LCD_FB_HEIGHT - SNAKE_AREA_HEIGHT) / 2 / 8 * 8)
#define SNAKE_FULL_COLS ((SNAKE_AREA_WIDTH - 3) / 4 - 1)
#define SNAKE_FULL_ROWS ((SNAKE_AREA_HEIGHT - 3) / 4 - 1)
#define SNAKE_FULL_WIDTH (SNAKE_FULL_COLS * 4 + 3)
#define SNAKE_FULL_HEIGHT (SNAKE_FULL_ROWS * 4 + 3)
#define SNAKE_FULL_X (SNAKE_AREA_X + (SNAKE_AREA_WIDTH - SNAKE_FULL_WIDTH) / 2)
#define SNAKE_FULL_Y (SNAKE_AREA_Y + (SNAKE_AREA_HEIGHT - SNAKE_FULL_HEIGHT) / 2)

enum {
    SNAKE_UP,
    SNAKE_RIGHT,
    SNAKE_DOWN,
    SNAKE_LEFT
};

struct snake {
    uint8_t cols, rows;
    uint16_t ring_size; /* cols * rows, rounded down to a multiple of 4 */
    uint16_t tail_index;
    uint16_t head_index;
    int8_t head_x, head_y;
    int8_t tail_x, tail_y;
    uint8_t direction; /* of the last move */
    uint8_t pending;   /* direction the next move will take */
    uint8_t grow;      /* the last move ate, so the tail stays for one move */
    uint8_t hit;       /* the last move was blocked; one more ends the game */
    uint8_t level;     /* 0..8 */
    uint8_t speed;     /* time between moves, in units of 10 ms */
    uint16_t score;
    uint8_t ring[SNAKE_MAX_COLS * SNAKE_MAX_ROWS / 4];
    /* One bit per cell: byte x + cols * (y / 8), bit y & 7. */
    uint8_t occupied[SNAKE_MAX_COLS * ((SNAKE_MAX_ROWS - 1) / 8 + 1)];
    int8_t food_x, food_y;
    /* What the last step changed, for snake_draw_step. */
    int8_t old_tail_x, old_tail_y;
    uint8_t moved, tail_moved;
};

extern struct snake snake;

/* New one-player game on a board of cols x rows cells: nine cells along the
   bottom row heading right, and the first food in the middle of the board.
   `speed` is the time between moves in units of 10 ms; the phone takes it
   from its speed table by level. */
void snake_init(uint8_t level, uint8_t speed, uint8_t cols, uint8_t rows);

/* The time between moves in scheduler ticks of 7.78125 ms. */
uint8_t snake_move_ticks(void);

/* A phone key, '1' to '9': 2/4/6/8 steer, and the corner keys turn towards
   whichever of their two directions is a turn. A reversal is ignored. */
void snake_key(char key);

/* One move. Returns the delay until the next one in scheduler ticks of
   7.78125 ms, or 0 when the game is over. */
uint8_t snake_step(void);

void snake_move_head(void);
void snake_advance_tail(void);
void snake_draw(void);

/* Draws only what the last snake_step changed, on top of an earlier
   snake_draw. The result is the same as clearing and drawing again. */
void snake_draw_step(void);

#endif
