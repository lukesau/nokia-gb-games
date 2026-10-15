#include "logic.h"

#include "game_assets.h"
#include "lcd.h"
#include "rand.h"
#include "sound.h"

#define TILE_SIZE 7
#define TILES 0x49       /* pictures in game_tile_bitmaps, after the blank one */
#define TILE_FRAME 0xfe  /* the frame tile instead of a picture */
#define BOARD_Y 9        /* first row of the tries, under the divider at 7 */
#define CELL_PITCH 8

struct logic logic;

uint8_t logic_code_length(void)
{
    return (uint8_t)(LOGIC_CODE - (logic.level < 4));
}

uint8_t logic_kinds(void)
{
    return (uint8_t)(((logic.level & 3) + 2) * 2);
}

static uint8_t *cell(uint8_t row, uint8_t col)
{
    return &logic.board[row * LOGIC_CODE + col];
}

/* Ten different tiles for the figures, as the firmware draws them: one
   more at a time, kept only when none before it is the same. */
static void pick_palette(void)
{
    uint8_t i = 0, j, k;

    do {
        logic.palette[i] = (uint8_t)(game_rand() % TILES + 1);
        k = i;
        for (j = i; j; j--)
            if (logic.palette[j - 1] == logic.palette[k])
                k--;
        i = (uint8_t)(k + 1);
    } while (i < LOGIC_PALETTE);
}

void logic_init(uint8_t level)
{
    uint8_t i, length, kinds;

    logic.level = level;
    logic.row = logic.col = 0;
    logic.last_row = logic.last_col = 0;
    logic.held = 0;
    for (i = 0; i < LOGIC_ROWS * LOGIC_CODE; i++)
        logic.board[i] = 0;
    for (i = 0; i < LOGIC_ROWS; i++)
        logic.scores[i] = 0;
    logic.won = 0;
    length = logic_code_length();
    kinds = logic_kinds();
    for (i = 0; i < length; i++)
        logic.secret[i] = (uint8_t)(game_rand() % kinds + 1);
    pick_palette();
}

uint8_t logic_score(const uint8_t *guess)
{
    uint8_t length = logic_code_length(), i, j;
    uint8_t exact = 0, elsewhere = 0;
    uint8_t right = 0; /* bit i: guess cell i is in the right place */
    uint8_t taken;     /* bit j: secret cell j has been matched */

    for (i = 0; i < length; i++)
        if (guess[i] == logic.secret[i]) {
            exact++;
            right |= (uint8_t)(1 << i);
        }
    if (exact == length)
        return LOGIC_WON;
    taken = right;
    for (i = 0; i < length; i++) {
        if (right & (1 << i))
            continue;
        for (j = 0; j < length; j++)
            if (!(taken & (1 << j)) && guess[i] == logic.secret[j]) {
                elsewhere++;
                taken |= (uint8_t)(1 << j);
                break;
            }
    }
    return (uint8_t)(exact * 16 + elsewhere);
}

/* The tries move up one row; the last is cleared. The firmware clears the
   score after the last row's instead of the last row's own, which the next
   check overwrites before anything draws it. */
static void scroll(void)
{
    uint8_t i;

    for (i = 0; i < (LOGIC_ROWS - 1) * LOGIC_CODE; i++)
        logic.board[i] = logic.board[i + LOGIC_CODE];
    for (i = 0; i < LOGIC_ROWS - 1; i++)
        logic.scores[i] = logic.scores[i + 1];
    for (i = 0; i < LOGIC_CODE; i++)
        logic.board[(LOGIC_ROWS - 1) * LOGIC_CODE + i] = 0;
    logic.scores[LOGIC_ROWS] = 0;
}

static uint8_t copying(void)
{
    return logic.row != logic.last_row || logic.col != logic.last_col;
}

static void stop_copying(void)
{
    logic.last_row = logic.row;
    logic.last_col = logic.col;
}

uint8_t logic_key(char key)
{
    uint8_t length = logic_code_length(), score;

    switch (key) {
    case '2':
    case '8':
        if (copying()) {
            logic.last_col = (uint8_t)((logic.last_col + (key == '2' ? length - 1 : 1)) % length);
            *cell(logic.row, logic.col) = *cell(logic.last_row, logic.last_col);
        } else {
            logic.col = (uint8_t)((logic.col + (key == '2' ? length - 1 : 1)) % length);
            stop_copying();
        }
        break;
    case '4':
        if (!copying())
            logic.held = *cell(logic.row, logic.col);
        if (logic.last_row < 1) {
            stop_copying();
            *cell(logic.row, logic.col) = logic.held;
        } else {
            logic.last_row--;
            *cell(logic.row, logic.col) = *cell(logic.last_row, logic.last_col);
        }
        break;
    case '5':
        if (copying()) {
            *cell(logic.row, logic.col) = *cell(logic.last_row, logic.last_col);
            stop_copying();
        } else {
            *cell(logic.row, logic.col) = (uint8_t)((*cell(logic.row, logic.col) + 1) % (logic_kinds() + 1));
        }
        break;
    case '*':
        logic.col = 0;
        score = logic_score(cell(logic.row, 0));
        logic.scores[logic.row] = score;
        if (score == LOGIC_WON) {
            logic.won = 1;
            return 1;
        }
        sound_play(SOUND_EAT);
        if (logic.row + 1 < LOGIC_ROWS)
            logic.row++;
        else
            scroll();
        stop_copying();
        break;
    default:
        break;
    }
    return 0;
}

/* A figure's tile, or the frame, at a cell position. */
static void draw_tile(uint8_t tile, int x, int y)
{
    lcd_blit_bitmap(x, y, TILE_SIZE, TILE_SIZE,
                    tile == TILE_FRAME ? game_tile_cursor : game_tile_bitmaps + tile * TILE_SIZE);
}

/* The tile a cell's figure is drawn as; blank for an empty cell. */
static uint8_t figure_tile(uint8_t figure)
{
    return figure ? logic.palette[figure - 1] : 0;
}

/* The marks for a try's score in the 8x7 space above it: rectangles for
   the right places, lines for the right figures elsewhere, in one column
   when they fit, else in two. */
static void draw_marks(uint8_t row, uint8_t score)
{
    uint8_t exact = score >> 4, elsewhere = score & 15;
    uint8_t x = (uint8_t)(row * CELL_PITCH), offset = elsewhere * 2 + exact * 3 < 8 ? 2 : 0;
    uint8_t i, first;

    for (i = 0; i < exact && i < 2; i++)
        lcd_fill_rect(x + offset, i * 3, 3, 2, 1);
    for (i = 2; i < exact; i++)
        lcd_fill_rect(x + 4, i * 3 - 6, 3, 2, 1);
    for (i = 0; i < elsewhere && i + (exact & 1) < 3 && exact < 2; i++)
        lcd_fill_rect(x + offset, i * 2 + (exact & 1) * 3, 3, 1, 1);
    for (first = i; i < elsewhere; i++)
        lcd_fill_rect(x + 4, (i - first) * 2 + exact / 3 * 3, 3, 1, 1);
}

void logic_draw(uint8_t blink)
{
    /* SDCC mistakes a local named like a member of logic for the member:
       the try and cell are r and c here. */
    uint8_t length = logic_code_length(), kinds = logic_kinds(), r, c;
    uint8_t figure;

    /* The kinds to choose from, while the first try is being made. */
    if (logic.row == 0)
        for (c = 0; c < kinds && c < LOGIC_PALETTE; c++)
            draw_tile(logic.palette[c], c * CELL_PITCH, 0);
    for (r = 0; r <= logic.row; r++) {
        if (r != logic.row)
            draw_marks(r, logic.scores[r]);
        for (c = 0; c < length; c++)
            draw_tile(figure_tile(*cell(r, c)), r * CELL_PITCH, BOARD_Y + c * CELL_PITCH);
    }
    lcd_fill_rect(0, 7, LOGIC_ROWS * CELL_PITCH, 1, 1);
    if (logic.scores[logic.row] != LOGIC_WON) {
        figure = *cell(logic.row, logic.col);
        draw_tile(figure ? figure_tile(figure) : TILE_FRAME, logic.row * CELL_PITCH, BOARD_Y + logic.col * CELL_PITCH);
        if (blink)
            lcd_fill_rect(logic.row * CELL_PITCH, BOARD_Y + logic.col * CELL_PITCH, TILE_SIZE, TILE_SIZE, LCD_INVERT);
    }
    if (logic.last_row < logic.row)
        draw_tile(TILE_FRAME, logic.last_row * CELL_PITCH, BOARD_Y + logic.last_col * CELL_PITCH);
}
