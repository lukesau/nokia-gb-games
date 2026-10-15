/* Host check that drawing only a move's changes gives the same picture as
   drawing the whole board. Needs the extracted assets. */
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "lcd.h"
#include "rand.h"
#include "snake.h"
#include "sound.h"

void platform_tone(uint16_t hz)
{
    (void)hz;
}

static int play(uint8_t cols, uint8_t rows)
{
    static uint8_t stepped[sizeof lcd_fb];
    static const char keys[] = "2468";
    unsigned step, games = 1, eaten = 0, seed = 1;

    lcd_view_full();
    game_srand(1);
    snake_init(0, 66, cols, rows);
    lcd_clear();
    snake_draw();
    for (step = 0; step < 20000; step++) {
        uint16_t score = snake.score;

        /* Mostly steer towards the food, sometimes at random. */
        seed = seed * 1103515245u + 12345u;
        if ((seed >> 16 & 7) == 0)
            snake_key(keys[seed >> 20 & 3]);
        else if (snake.food_y != snake.head_y && (seed >> 24 & 1 || snake.food_x == snake.head_x))
            snake_key(snake.food_y < snake.head_y ? '2' : '8');
        else if (snake.food_x != snake.head_x)
            snake_key(snake.food_x < snake.head_x ? '4' : '6');

        if (!snake_step() ) {
            games++;
            snake_init(0, 66, cols, rows);
            lcd_clear();
            snake_draw();
            continue;
        }
        eaten += snake.score != score;
        snake_draw_step();
        memcpy(stepped, lcd_fb, sizeof lcd_fb);
        lcd_clear();
        snake_draw();
        if (memcmp(stepped, lcd_fb, sizeof lcd_fb) != 0) {
            printf("FAIL: step %u differs from a full redraw\n", step);
            return 1;
        }
    }
    if (eaten < 100) {
        printf("FAIL: only %u foods eaten; the test did not cover eating\n", eaten);
        return 1;
    }
    printf("ok %ux%u (%u games, %u foods)\n", cols, rows, games, eaten);
    return 0;
}

int main(void)
{
    if (play(SNAKE_COLS, SNAKE_ROWS))
        return 1;
    /* The full-screen board, when built with a bigger framebuffer. */
    if (LCD_HAS_SURROUND && play(SNAKE_FULL_COLS, SNAKE_FULL_ROWS))
        return 1;
    return 0;
}
