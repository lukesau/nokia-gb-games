#include "memory.h"

#include "game_assets.h"
#include "lcd.h"
#include "rand.h"
#include "sound.h"

#define PICTURES 0x49
#define CARD_SIZE 7
/* The middle of the board, in half cards: the biggest board fills the LCD. */
#define ORIGIN_X 10
#define ORIGIN_Y 6

/* What a card shows: its picture (0 for its back), with this set when it
   is drawn inverted. */
#define SHOWN_INVERTED 0x80
#define SHOWN_NOTHING 0xff

struct memory memory;

static uint8_t shown[MEMORY_MAX_CARDS]; /* what the LCD holds for each card */

static uint8_t *card_at(uint8_t x, uint8_t y)
{
    return &memory.board[x * memory.rows + y];
}

void memory_init(uint8_t level)
{
    uint8_t pictures[PICTURES];
    uint8_t cards, i, n, k, swap;

    memory.cols = memory_board_sizes[level * 4];
    memory.rows = memory_board_sizes[level * 4 + 1];
    cards = (uint8_t)(memory.cols * memory.rows);
    memory.pairs = cards / 2;
    memory.x = memory.y = 0;
    memory.state = MEMORY_NONE_UP;
    memory.tries = 0;
    memory.score = 0;

    /* Shuffle the pictures, deal each of the first ones twice, then shuffle
       the cards; all in the firmware's order, so a seed gives its deal. */
    for (i = 0; i < PICTURES; i++)
        pictures[i] = (uint8_t)(i + 1);
    for (n = PICTURES - 1; n > 1; n--) {
        k = (uint8_t)(game_rand() % n);
        swap = pictures[k];
        pictures[k] = pictures[n];
        pictures[n] = swap;
    }
    for (i = 0; i < cards; i += 2)
        memory.board[i] = memory.board[i + 1] = pictures[i / 2];
    for (n = (uint8_t)(cards - 1); n; n--) {
        k = (uint8_t)(game_rand() % n);
        swap = memory.board[k];
        memory.board[k] = memory.board[n];
        memory.board[n] = swap;
    }
}

/* To the next face-down card (step 1) or the previous one (step -1) in
   reading order, giving up after a whole round of the board. */
static void seek_face_down(int8_t step)
{
    uint8_t tries;

    for (tries = (uint8_t)(memory.cols * memory.rows); tries; tries--) {
        uint8_t x = (uint8_t)((memory.x + memory.cols + step) % memory.cols);

        if (x != memory.x + step) /* wrapped: on to the next row */
            memory.y = (uint8_t)((memory.y + memory.rows + step) % memory.rows);
        memory.x = x;
        if (!(*card_at(memory.x, memory.y) & MEMORY_FACE_UP))
            break;
    }
}

/* Turns up the card under the cursor. Returns nonzero when that ends the
   game. */
static uint8_t turn_up(void)
{
    uint8_t *card = card_at(memory.x, memory.y);
    uint16_t allowed;

    *card |= MEMORY_FACE_UP;
    if (memory.state == MEMORY_NONE_UP) {
        memory.first_x = memory.x;
        memory.first_y = memory.y;
        memory.state = MEMORY_ONE_UP;
        return 0;
    }
    memory.tries++;
    if (*card != *card_at(memory.first_x, memory.first_y)) {
        memory.state = MEMORY_TWO_UP;
        return 0;
    }
    sound_play(SOUND_EAT);
    memory.state = MEMORY_NONE_UP;
    if (--memory.pairs)
        return 0;
    allowed = (uint16_t)(memory.cols * memory.rows * 7 / 4);
    memory.score = memory.tries < allowed ? allowed - memory.tries : 0;
    return 1;
}

uint8_t memory_key(char key)
{
    uint8_t over = 0;

    /* Any key turns back two cards that did not match. */
    if (memory.state == MEMORY_TWO_UP) {
        *card_at(memory.x, memory.y) &= (uint8_t)~MEMORY_FACE_UP;
        *card_at(memory.first_x, memory.first_y) &= (uint8_t)~MEMORY_FACE_UP;
        memory.state = MEMORY_TURNED_BACK;
    }
    switch (key) {
    case '5':
        if (memory.state != MEMORY_TURNED_BACK && !(*card_at(memory.x, memory.y) & MEMORY_FACE_UP))
            over = turn_up();
        else
            seek_face_down(1);
        break;
    case '#':
        seek_face_down(1);
        break;
    case '*':
        seek_face_down(-1);
        break;
    case '2':
        memory.y = (uint8_t)((memory.y + memory.rows - 1) % memory.rows);
        break;
    case '8':
        memory.y = (uint8_t)((memory.y + 1) % memory.rows);
        break;
    case '4':
        memory.x = (uint8_t)((memory.x + memory.cols - 1) % memory.cols);
        break;
    case '6':
        memory.x = (uint8_t)((memory.x + 1) % memory.cols);
        break;
    default:
        break;
    }
    if (memory.state == MEMORY_TURNED_BACK)
        memory.state = MEMORY_NONE_UP;
    return over;
}

static void draw_card(uint8_t x, uint8_t y, uint8_t blink)
{
    uint8_t card = *card_at(x, y);
    uint8_t picture = card & MEMORY_FACE_UP ? card & (uint8_t)~MEMORY_FACE_UP : 0;
    uint8_t *was = &shown[x * memory.rows + y];
    int px = (ORIGIN_X + 2 * x - memory.cols) * 4, py = (ORIGIN_Y + 2 * y - memory.rows) * 4;

    if (blink && x == memory.x && y == memory.y)
        picture |= SHOWN_INVERTED;
    if (*was == picture)
        return;
    *was = picture;
    lcd_blit_bitmap(px, py, CARD_SIZE, CARD_SIZE,
                    picture & (uint8_t)~SHOWN_INVERTED ? game_tile_bitmaps + (picture & (uint8_t)~SHOWN_INVERTED) * CARD_SIZE
                                                        : game_tile_back);
    if (picture & SHOWN_INVERTED)
        lcd_fill_rect(px, py, CARD_SIZE, CARD_SIZE, LCD_INVERT);
}

void memory_draw_changes(uint8_t blink)
{
    uint8_t x, y;

    for (x = 0; x < memory.cols; x++)
        for (y = 0; y < memory.rows; y++)
            draw_card(x, y, blink);
}

void memory_draw(uint8_t blink)
{
    uint8_t i;

    for (i = 0; i < MEMORY_MAX_CARDS; i++)
        shown[i] = SHOWN_NOTHING;
    memory_draw_changes(blink);
}
