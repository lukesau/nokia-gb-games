#include "snake.h"

#include "game.h"
#include "game_assets.h"
#include "lcd.h"
#include "rand.h"
#include "sound.h"

/* Delay after a blocked move, giving one last chance to turn away. */
#define HIT_GRACE_TICKS 20

struct snake snake;

static void set_occupied(int8_t x, int8_t y, uint8_t on)
{
    uint8_t *cell = &snake.occupied[x + snake.cols * (y / 8)];
    uint8_t bit = (uint8_t)(1 << (y & 7));

    if (on)
        *cell |= bit;
    else
        *cell &= (uint8_t)~bit;
}

static uint8_t ring_get(uint16_t index)
{
    return (snake.ring[index >> 2] >> ((index & 3) << 1)) & 3;
}

static int8_t step_x(uint8_t direction)
{
    return direction == SNAKE_LEFT ? -1 : direction == SNAKE_RIGHT;
}

static int8_t step_y(uint8_t direction)
{
    return direction == SNAKE_UP ? -1 : direction == SNAKE_DOWN;
}

static uint8_t is_occupied(int8_t x, int8_t y)
{
    return (snake.occupied[x + snake.cols * (y / 8)] >> (y & 7)) & 1;
}

void snake_init(uint8_t level, uint8_t speed, uint8_t cols, uint8_t rows)
{
    unsigned i;

    snake.speed = speed;
    snake.cols = cols;
    snake.rows = rows;
    snake.ring_size = (uint16_t)(cols * rows / 4 * 4);
    snake.level = level;
    snake.score = 0;
    snake.grow = 0;
    snake.hit = 0;
    snake.direction = snake.pending = SNAKE_RIGHT;
    snake.tail_index = 0;
    snake.head_index = 0;
    snake.head_x = snake.tail_x = 0;
    snake.head_y = snake.tail_y = snake.rows - 1;
    for (i = 0; i < sizeof snake.occupied; i++)
        snake.occupied[i] = 0;
    for (i = 0; i < sizeof snake.ring; i++)
        snake.ring[i] = 0;
    set_occupied(snake.head_x, snake.head_y, 1);

    for (i = 0; i < 8; i++)
        snake_move_head();

    snake.food_x = snake.cols / 2;
    snake.food_y = snake.rows / 2;
}

void snake_key(char key)
{
    uint8_t vertical = !(snake.direction & 1);

    switch (key) {
    case '2':
        if (snake.direction != SNAKE_DOWN)
            snake.pending = SNAKE_UP;
        break;
    case '8':
        if (snake.direction != SNAKE_UP)
            snake.pending = SNAKE_DOWN;
        break;
    case '4':
        if (snake.direction != SNAKE_RIGHT)
            snake.pending = SNAKE_LEFT;
        break;
    case '6':
        if (snake.direction != SNAKE_LEFT)
            snake.pending = SNAKE_RIGHT;
        break;
    case '1':
        snake.pending = vertical ? SNAKE_LEFT : SNAKE_UP;
        break;
    case '3':
        snake.pending = vertical ? SNAKE_RIGHT : SNAKE_UP;
        break;
    case '7':
        snake.pending = vertical ? SNAKE_LEFT : SNAKE_DOWN;
        break;
    case '9':
        snake.pending = vertical ? SNAKE_RIGHT : SNAKE_DOWN;
        break;
    default:
        break;
    }
}

/* Whether the next head cell is blocked: a wall or the snake itself. The
   tail's own cell is free when the tail is about to move out of it. */
static uint8_t move_blocked(void)
{
    int8_t x = (int8_t)(snake.head_x + step_x(snake.direction));
    int8_t y = (int8_t)(snake.head_y + step_y(snake.direction));

    if (x < 0 || y < 0 || x >= snake.cols || y >= snake.rows)
        return 1;
    if (x == snake.tail_x && y == snake.tail_y)
        return snake.grow;
    return is_occupied(x, y);
}

/* Up to 255 tries for a free cell, as the firmware does. */
static void place_food(void)
{
    uint8_t tries = 255;
    int8_t x, y;

    do {
        x = (int8_t)(game_rand() % snake.cols);
        y = (int8_t)(game_rand() % snake.rows);
    } while (is_occupied(x, y) && --tries);
    snake.food_x = x;
    snake.food_y = y;
}

uint8_t snake_step(void)
{
    snake.direction = snake.pending;
    snake.moved = snake.tail_moved = 0;
    if (move_blocked()) {
        if (snake.hit)
            return 0;
        snake.hit = 1;
        return HIT_GRACE_TICKS;
    }
    snake.hit = 0;
    snake.moved = 1;
    if (!snake.grow) {
        snake.old_tail_x = snake.tail_x;
        snake.old_tail_y = snake.tail_y;
        snake.tail_moved = 1;
        snake_advance_tail();
    }
    snake_move_head();
    snake.grow = snake.head_x == snake.food_x && snake.head_y == snake.food_y;
    if (snake.grow) {
        sound_play(SOUND_EAT);
        snake.score += snake.level + 1;
        place_food();
    }
    return snake_move_ticks();
}

/* speed is in units of 10 ms; a tick is 7.78125 ms (249/32). */
uint8_t snake_move_ticks(void)
{
    return (uint8_t)((uint16_t)snake.speed * 320 / 249);
}

/* The firmware also scores a filled board here; that is not done yet. */
void snake_move_head(void)
{
    uint8_t shift = (uint8_t)((snake.head_index & 3) << 1);
    uint8_t *slot = &snake.ring[snake.head_index >> 2];

    if ((snake.head_index + 1) % snake.ring_size == snake.tail_index)
        snake_advance_tail();

    *slot = (uint8_t)((*slot & ~(3 << shift)) | (snake.direction << shift));
    snake.head_index = (uint16_t)((snake.head_index + 1) % snake.ring_size);
    snake.head_x += step_x(snake.direction);
    snake.head_y += step_y(snake.direction);
    set_occupied(snake.head_x, snake.head_y, 1);
}

void snake_advance_tail(void)
{
    uint8_t direction = ring_get(snake.tail_index);

    set_occupied(snake.tail_x, snake.tail_y, 0);
    snake.tail_x += step_x(direction);
    snake.tail_y += step_y(direction);
    snake.tail_index = (uint16_t)((snake.tail_index + 1) % snake.ring_size);
}

static void draw_food(void)
{
    lcd_blit_bitmap(snake.food_x * 4 + 2, snake.food_y * 4 + 2, 4, 4, snake_food_bitmap);
}

void snake_draw(void)
{
    uint16_t index;
    int8_t x = snake.tail_x, y = snake.tail_y;

    /* Border: on the phone 83x47, leaving the last column and row of the
       LCD clear. */
    lcd_fill_rect(0, 0, snake.cols * 4 + 2, 1, 1);
    lcd_fill_rect(snake.cols * 4 + 2, 0, 1, snake.rows * 4 + 2, 1);
    lcd_fill_rect(0, 0, 1, snake.rows * 4 + 2, 1);
    lcd_fill_rect(0, snake.rows * 4 + 2, snake.cols * 4 + 3, 1, 1);

    draw_food();

    /* The tail is a 3x3 block; every later segment is widened by one pixel
       towards the cell it came from, so neighbours join. */
    lcd_fill_rect(x * 4 + 2, y * 4 + 2, 3, 3, 1);
    for (index = snake.tail_index; index != snake.head_index; index = (uint16_t)((index + 1) % snake.ring_size)) {
        uint8_t direction = ring_get(index);

        x += step_x(direction);
        y += step_y(direction);
        lcd_fill_rect(x * 4 + 2 - (direction == SNAKE_RIGHT), y * 4 + 2 - (direction == SNAKE_DOWN),
             direction & 1 ? 4 : 3, direction & 1 ? 3 : 4, 1);
    }
}

void snake_draw_step(void)
{
    if (!snake.moved)
        return;
    if (snake.tail_moved) {
        /* Clear the old tail block and the pixel that joined it to the next
           segment; that leaves the new tail as a plain 3x3 block. */
        int8_t x = snake.old_tail_x < snake.tail_x ? snake.old_tail_x : snake.tail_x;
        int8_t y = snake.old_tail_y < snake.tail_y ? snake.old_tail_y : snake.tail_y;
        uint8_t along_x = snake.old_tail_x != snake.tail_x;
        int px = x * 4 + 2, py = y * 4 + 2;

        if (along_x && snake.old_tail_x > snake.tail_x)
            px += 3; /* the new tail is to the left: keep its three columns */
        if (!along_x && snake.old_tail_y > snake.tail_y)
            py += 3;
        lcd_fill_rect(px, py, along_x ? 4 : 3, along_x ? 3 : 4, 0);
    }
    lcd_fill_rect(snake.head_x * 4 + 2 - (snake.direction == SNAKE_RIGHT), snake.head_y * 4 + 2 - (snake.direction == SNAKE_DOWN),
         snake.direction & 1 ? 4 : 3, snake.direction & 1 ? 3 : 4, 1);
    if (snake.grow)
        draw_food();
}
