/* The full-screen menus made at build time (native_tiles.h), on the Game
   Boy.

   The menus' screen gives every 8x8 cell its own tile in video RAM, the
   first 12 rows' at 0x8000 and the rest at 0x9000 (the cut in crt0.s), and
   the tile map names them in order. A made screen keeps that, except that
   empty cells and full ones name two tiles kept for them; every other cell
   gets its tile copied into its own. So nothing is drawn: a screen is
   about a hundred and fifty tiles copied and its tile map written, a few
   frames.

   Only the first bit plane is written. The ports' palette (NATIVE_PALETTE)
   takes the colour from that plane alone, so what the second holds from
   before does not matter.

   This file and the screens are in one bank, NATIVE_BANK; the tiles fill
   the next (native_put.s, in the first bank, reads them). The port says
   what both are and calls these through its far_ functions. */
#include <stdint.h>

#include <string.h>

#include "lcd.h"
#include "native_gb.h"
#include "native_tiles.h"

#define REG(addr) (*(volatile uint8_t *)(addr))
#ifdef NATIVE_PLATFORM_PALETTE
/* What the boot ROM left in A, which the port's crt0.s keeps: 0x11 on a
   Game Boy Color or an Advance. */
uint8_t gb_cgb;

#define LCDC REG(0xff40)
#define LY REG(0xff44)
#define BGP REG(0xff47)
#define BCPS REG(0xff68)
#define BCPD REG(0xff69)
#define KEY1 REG(0xff4d)
#define VBK REG(0xff4f)
#define P1 REG(0xff00)

/* In work RAM, which starts clear. */
static uint8_t color_started;

void native_palette(uint8_t shades)
{
    /* White, light grey, dark grey, black, as RGB555. */
    static const uint16_t colours[4] = { 0x7fff, 0x56b5, 0x294a, 0x0000 };
    uint8_t i;

    BGP = shades;
    if (gb_cgb != GB_CGB)
        return;
    /* The first call, at power-on with the LCD off: Color mode at double
       speed, the tiles' attributes cleared. */
    if (!color_started) {
        color_started = 1;
        VBK = 1;
        memset((uint8_t *)0x9800, 0, 0x800);
        VBK = 0;
        P1 = 0x30;
        KEY1 = 0x01;
        __asm__("stop");
    }
    /* The Color palette is not there to write while a line is drawn: in
       the vertical blank, with a line of it left (or at once, the LCD
       off). */
    if (LCDC & 0x80)
        while (LY < 144 || LY >= 152)
            ;
    BCPS = 0x80;
    for (i = 0; i < 4; i++, shades >>= 2) {
        BCPD = (uint8_t)colours[shades & 3];
        BCPD = (uint8_t)(colours[shades & 3] >> 8);
    }
}

#define set_palette(shades) native_palette(shades)
#else
#define BGP REG(0xff47)
#define set_palette(shades) (BGP = (shades))
#endif

#define CELLS_X 20
#define CELLS_Y 18
#define SPLIT_ROW 12

/* The two kept tiles, by the number both halves of the screen name them
   with (0x8fd0 and 0x8fe0: tiles 253 and 254 at 0x8000, -3 and -2 at
   0x9000). Clear of the tiles the game at 2x keeps up there. */
#define FULL_ID 0xfd
#define EMPTY_ID 0xfe
#define FULL_TILE ((uint8_t *)0x8fd0)
#define EMPTY_TILE ((uint8_t *)0x8fe0)

#define MAP ((uint8_t *)0x9800)

extern const uint16_t native_screen_count;
extern const uint16_t native_row_data[];
extern const uint16_t native_screen_rows[];
extern const uint16_t native_screen_start[];
extern const uint8_t native_cursor_data[];
extern const uint16_t native_cursor_start[];
extern const uint8_t native_tiles[][8];

static const uint8_t empty[8];
static const uint8_t full[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* The screen up, as where its rows are in native_row_data; null when none
   is. */
static const uint16_t *shown;
uint8_t native_up;
/* The list row the cursor is drawn on, NO_ROW for none. */
#define NO_ROW 0xff
static uint8_t cursor_row = NO_ROW;

/* The first cell of a row: its tile, and the number the map names it by. */
static uint8_t row_first(uint8_t ty)
{
    return (uint8_t)((ty < SPLIT_ROW ? ty : ty - SPLIT_ROW) * CELLS_X);
}

static uint8_t *row_tiles(uint8_t ty)
{
    return (ty < SPLIT_ROW ? (uint8_t *)0x8000 : (uint8_t *)0x9000) + ((uint16_t)row_first(ty) << 4);
}

uint8_t native_show(uint16_t id)
{
    const uint16_t *rows, *was, *old;
    uint8_t ty, n;

    if (id >= native_screen_count || native_screen_start[id] == 0xffff)
        return 0;
    rows = native_screen_rows + native_screen_start[id];
    /* Over a made screen, only the cells whose tiles differ are copied:
       the cursor's cells are made plain first. */
    was = shown;
    if (was && cursor_row != NO_ROW)
        native_cursor(cursor_row, 0);
    if (!was) {
        native_put_tile(EMPTY_TILE, empty);
        native_put_tile(FULL_TILE, full);
    }
    shown = rows;
    native_up = 1;
    for (ty = 0; ty < CELLS_Y; ty++) {
        memset(native_old, 0, sizeof native_old);
        if (was) {
            /* The same row: nothing to do. */
            if (was[ty] == rows[ty])
                continue;
            old = native_row_data + was[ty];
            for (n = (uint8_t)*old++; n; n--, old++)
                native_old[(uint8_t)(*old >> 11)] = *old & 0x7ff;
        }
        native_first = row_first(ty);
        native_row(native_row_data + rows[ty], row_tiles(ty));
        native_put_row(MAP + ty * 32, native_map_row);
    }
    /* In case the screen was blanked (platform_native_blank). */
    set_palette(NATIVE_PALETTE);
    return 1;
}

/* The tile of the screen up at a cell: 0 empty, 1 full, or a made one. */
static uint16_t shown_tile(uint8_t tx, uint8_t ty)
{
    const uint16_t *p = native_row_data + shown[ty];
    uint8_t n;

    for (n = (uint8_t)*p++; n; n--, p++)
        if ((uint8_t)(*p >> 11) == tx)
            return *p & 0x7ff;
    return 0;
}

void native_cursor(uint8_t row, uint8_t on)
{
    const uint8_t *c, *end;

    if (!shown || row >= NATIVE_CURSOR_ROWS)
        return;
    if (on)
        cursor_row = row;
    else if (row == cursor_row)
        cursor_row = NO_ROW;
    c = native_cursor_data + native_cursor_start[row];
    end = native_cursor_data + native_cursor_start[row + 1];
    for (; c != end; c += 10) {
        uint8_t tx = c[0], ty = c[1], i;
        uint16_t tile = shown_tile(tx, ty);
        uint8_t bytes[8];

        if (tile >= 2)
            native_get_far(bytes, native_tiles[tile - 2]);
        for (i = 0; i < 8; i++)
            bytes[i] = (uint8_t)((tile == 0 ? 0 : tile == 1 ? 0xff : bytes[i]) | (on ? c[2 + i] : 0));
        native_put_tile(row_tiles(ty) + ((uint16_t)tx << 4), bytes);
        native_put_byte(MAP + ty * 32 + tx, (uint8_t)(row_first(ty) + tx));
    }
}

/* Every cell its own tile again in the map, the screen blanked by the
   palette; with `fb`, the tiles made from lcd_fb as well. */
static uint8_t leave(const uint8_t *fb)
{
    uint8_t tx, ty;

    if (!shown)
        return 0;
    shown = 0;
    native_up = 0;
    cursor_row = NO_ROW;
    set_palette(0);
    for (ty = 0; ty < CELLS_Y; ty++) {
        uint8_t first = row_first(ty);

        if (fb) {
            native_fb_row(row_tiles(ty), fb);
            fb += 8 * LCD_STRIDE;
        }
        for (tx = 0; tx < CELLS_X; tx++)
            native_map_row[tx] = (uint8_t)(first + tx);
        native_put_row(MAP + ty * 32, native_map_row);
    }
    return 1;
}

uint8_t native_leave(void)
{
    /* The screen blanked at once by the palette, every colour light, while
       the cells' own tiles are made from lcd_fb and the tile map is put
       back; then it all comes up together. */
    if (!leave(lcd_fb))
        return 0;
    set_palette(NATIVE_PALETTE);
    memset(lcd_dirty, 0, sizeof lcd_dirty);
    return 1;
}

uint8_t native_leave_blank(void)
{
    return leave(0);
}
