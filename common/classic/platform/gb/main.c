/* Game Boy layer. The core's framebuffer is the whole 160x144 screen, shown
   as a 20x18 block of background tiles, one tile per 8x8 cell; the phone's
   84x48 LCD is a window in the middle of it. A, Select or Start is the
   phone's Navi key, B is its C key, the D-pad scrolls and steers, and Start
   on the first screen picks the full-screen variant.

   This file is in the ROM's first 16 KiB with the rest of the code; bank 1
   holds the menus and the firmware's pictures and text, and is the bank
   mapped but while a full-screen menu made at build time is put up from
   banks 2 and 3 (native_tiles.h). */
#include <stdint.h>

#include <string.h>

#include "game.h"
#include "lcd.h"
#include "menu.h"
#include "native_gb.h"
#include "native_tiles.h"
#include "sound.h"

#define REG(addr) (*(volatile uint8_t *)(addr))
#define P1 REG(0xff00)
#define IF REG(0xff0f)
#define IE REG(0xffff)
#define NR21 REG(0xff16)
#define NR22 REG(0xff17)
#define NR23 REG(0xff18)
#define NR24 REG(0xff19)
#define NR50 REG(0xff24)
#define NR51 REG(0xff25)
#define NR52 REG(0xff26)
#define LCDC REG(0xff40)
#define STAT REG(0xff41)
#define SCY REG(0xff42)
#define SCX REG(0xff43)
#define LY REG(0xff44)
#define LYC REG(0xff45)
#define BGP REG(0xff47)

#define VRAM_MAP ((uint8_t *)0x9800)

#define TILES_X LCD_CELLS_X /* 20 */
#define TILES_Y LCD_CELLS_Y /* 18 */

/* 360 tiles are more than the 256 a tile number can name. The first 240
   (12 rows) are at 0x8000, the rest at 0x9000, and the handlers in crt0.s
   switch the LCD between the two tile areas at this line of every frame. */
#define SPLIT_ROW 12
#define SPLIT_TILE (SPLIT_ROW * TILES_X)
#define SPLIT_LINE (SPLIT_ROW * 8)

#define LCDC_ON 0x91 /* LCD on, tile data at 0x8000, background on */

#define PAD_A 0x01
#define PAD_B 0x02
#define PAD_SELECT 0x04
#define PAD_START 0x08
#define PAD_RIGHT 0x10
#define PAD_LEFT 0x20
#define PAD_UP 0x40
#define PAD_DOWN 0x80

/* Keys pressed at power-on, for scripted screenshots: u, d, l, r, s select,
   e the Select button, b back, a the full-screen key, t one move of the
   running game, and p to start over as after a power cycle (settings are
   read back). */
#ifndef START_KEYS
#define START_KEYS ""
#endif

/* Battery-backed cartridge RAM (MBC1): a two-byte signature, then one
   four-byte record per game laid out as the phone stores them: top score
   high byte, low byte, level, and a check byte. */
#define MBC_RAM_ENABLE REG(0x0000)
#define SAVE ((uint8_t *)0xa000)
#define SAVE_SIGNATURE_0 'N'
#define SAVE_SIGNATURE_1 '3'
#define SAVE_CHECK(r) ((uint8_t)((r)[0] + (r)[1] + (r)[2] + 0x5a))

/* Tiles converted at a time before being copied to video RAM. */
#define STAGED_TILES 4
/* Most frames of game time made up at once after a slow draw. */
#define MAX_CATCH_UP 6

/* Frames since power-on, counted by the vertical-blank handler in crt0.s. */
volatile uint8_t frame_count;

/* Tile data on its way to video RAM, and where it goes; flush_tiles in
   crt0.s copies it between the lines the LCD is drawing. */
uint8_t staged[STAGED_TILES * 16];
uint8_t *staged_at[STAGED_TILES];
uint8_t staged_count;
const uint8_t *staged_from; /* flush_tiles' place in staged */

void flush_tiles(void);

/* Called once a frame by the vertical-blank handler in crt0.s. */
void sound_frame(void)
{
    sound_tick(16743); /* a frame is 1/59.73 s */
}

/* The buzzer is pulse channel 2: a 50% square wave at full volume. Its
   frequency register is 2048 - 131072 / hz. */
void platform_tone(uint16_t hz)
{
    uint16_t period;

    if (!hz) {
        NR22 = 0x00; /* volume 0 switches the channel off */
        NR24 = 0x80;
        return;
    }
    period = (uint16_t)(2048 - 131072ul / hz);
    NR21 = 0x80;
    NR22 = 0xf0;
    NR23 = (uint8_t)period;
    NR24 = (uint8_t)(0x80 | (period >> 8));
}

void platform_settings_load(uint8_t game, struct game_settings *out)
{
    const uint8_t *record = SAVE + 2 + game * 4;

    out->top_score = 0;
    out->level = 0;
    MBC_RAM_ENABLE = 0x0a;
    if (SAVE[0] == SAVE_SIGNATURE_0 && SAVE[1] == SAVE_SIGNATURE_1 && record[3] == SAVE_CHECK(record)) {
        out->top_score = (uint16_t)(record[0] << 8 | record[1]);
        out->level = record[2];
    }
    MBC_RAM_ENABLE = 0x00;
}

void platform_settings_save(uint8_t game, const struct game_settings *in)
{
    uint8_t *record = SAVE + 2 + game * 4;

    MBC_RAM_ENABLE = 0x0a;
    SAVE[0] = SAVE_SIGNATURE_0;
    SAVE[1] = SAVE_SIGNATURE_1;
    record[0] = (uint8_t)(in->top_score >> 8);
    record[1] = (uint8_t)in->top_score;
    record[2] = in->level;
    record[3] = SAVE_CHECK(record);
    MBC_RAM_ENABLE = 0x00;
}

/* Used once, at power-on, before interrupts are enabled. */
static void wait_vblank(void)
{
    while (LY != 144)
        ;
}

/* Converts one 8x8 cell of lcd_fb to tile data: a set pixel is colour 3, a
   clear one 0. A framebuffer row is 20 bytes, one per tile across, so each
   tile row is one framebuffer byte written to both bit planes. */
static void render_tile(uint8_t tx, uint8_t ty, uint8_t *tile)
{
    const uint8_t *src = lcd_fb + ty * (8 * LCD_STRIDE) + tx;
    uint8_t row;

    for (row = 8; row; row--, src += LCD_STRIDE) {
        *tile++ = *src;
        *tile++ = *src;
    }
}

/* For the magnified rectangle, in crt0.s. zoom_tile is render_tile for a
   quarter of a cell, 4x4 pixels each drawn 2x2: `src` is the cell's byte on
   the quarter's first row and zoom_right picks the right-hand half of the
   bytes. zoom_take compares four rows of a cell in lcd_fb with the copy of
   what is shown, brings the copy up to date, and returns the bits that
   differed in any row. */
uint8_t zoom_right;
void zoom_tile(const uint8_t *src, uint8_t *tile);
uint8_t zoom_take(const uint8_t *src, uint8_t *was);

/* Where each row of tiles starts in video RAM. */
static uint8_t *tile_row[TILES_Y];

static uint8_t *tile_address(uint8_t tx, uint8_t ty)
{
    return tile_row[ty] + (uint16_t)(tx << 4);
}

static void stage_tile(uint8_t tx, uint8_t ty, uint8_t *n)
{
    staged_at[*n] = tile_address(tx, ty);
    if (++*n == STAGED_TILES) {
        staged_count = *n;
        flush_tiles();
        *n = 0;
    }
}

static uint8_t zoomed; /* the screen holds the magnified rectangle */

/* The magnified rectangle is as wide as the screen at 2x and as tall as the
   phone's LCD. What the screen holds of it is kept here, a byte for every
   eight pixels as in lcd_fb, so that only tiles whose pixels changed are
   converted and copied. */
#define ZOOM_BYTES (LCD_FB_WIDTH / 2 / 8)
#define ZOOM_ROWS LCD_HEIGHT
static uint8_t zoom_shown[ZOOM_BYTES * ZOOM_ROWS];

/* One tile to video RAM: straight there with the LCD off, which it only is
   at power-on, otherwise by way of the staging buffer. */
static uint8_t *tile_target(uint8_t tx, uint8_t ty, uint8_t n)
{
    return LCDC & 0x80 ? staged + n * 16 : tile_address(tx, ty);
}

/* The core has named a rectangle to show at 2x in the middle of the screen
   (whole cells, ZOOM_BYTES cells wide and ZOOM_ROWS pixels tall). Every
   cell of it is four tiles, one for each quarter; the tiles above and below
   it are blanked when it first comes up, and cells outside it are not
   shown. */
static void present_zoomed(void)
{
    uint8_t first_row = (uint8_t)((TILES_Y - ZOOM_ROWS / 4) / 2);
    uint8_t cx, cy, half, n = 0, i;
    uint8_t *dirty = lcd_dirty + lcd_zoom_y / 8 * LCD_CELLS_X + lcd_zoom_x / 8;
    const uint8_t *fb = lcd_fb + lcd_zoom_y * LCD_STRIDE + lcd_zoom_x / 8;
    uint8_t *shown = zoom_shown;
    const uint8_t *src;
    uint8_t *was, *tile;

    if (!zoomed) {
        uint8_t tx, ty;

        for (ty = 0; ty < TILES_Y; ty++) {
            if ((uint8_t)(ty - first_row) < ZOOM_ROWS / 4)
                continue;
            for (tx = 0; tx < TILES_X; tx++) {
                tile = tile_target(tx, ty, n);
                for (i = 0; i < 16; i++)
                    tile[i] = 0;
                if (LCDC & 0x80)
                    stage_tile(tx, ty, &n);
            }
        }
    }
    for (cy = 0; cy < ZOOM_ROWS / 8; cy++, dirty += LCD_CELLS_X, fb += 8 * LCD_STRIDE, shown += 8 * ZOOM_BYTES) {
        for (cx = 0; cx < ZOOM_BYTES; cx++) {
            if (!dirty[cx])
                continue;
            dirty[cx] = 0;
            /* The cell's first byte in lcd_fb and in the copy of what is
               shown; each half of it is four rows and two tiles. */
            src = fb + cx;
            was = shown + cx;
            for (half = 0; half < 2; half++) {
                uint8_t changed = zoom_take(src, was);

                if (!zoomed)
                    changed = 0xff;
                for (i = 0; i < 2; i++) {
                    uint8_t tx = (uint8_t)(cx * 2 + i), ty = (uint8_t)(first_row + cy * 2 + half);

                    if (!(changed & (i ? 0x0f : 0xf0)))
                        continue;
                    zoom_right = i;
                    zoom_tile(src, tile_target(tx, ty, n));
                    if (LCDC & 0x80)
                        stage_tile(tx, ty, &n);
                }
                src += 4 * LCD_STRIDE;
                was += 4 * ZOOM_BYTES;
            }
        }
    }
    if (n) {
        staged_count = n;
        flush_tiles();
    }
    if (!(LCDC & 0x80))
        LCDC = LCDC_ON;
    zoomed = 1;
}

/* Brings video RAM up to date with the cells of lcd_fb drawn to since the
   last call. The LCD stays on: the tiles go in a few at a time between the
   lines being drawn, so a changed screen fills in over a few frames instead
   of blinking. */
static void present(void)
{
    uint8_t tx, ty, n = 0;
    uint8_t *dirty = lcd_dirty;

    if (lcd_zoom_w) {
        present_zoomed();
        return;
    }
    zoomed = 0;
    for (ty = 0; ty < TILES_Y; ty++) {
        for (tx = 0; tx < TILES_X; tx++, dirty++) {
            if (!*dirty)
                continue;
            *dirty = 0;
            if (!(LCDC & 0x80)) {
                /* Only at power-on: nothing is being drawn yet. */
                render_tile(tx, ty, tile_address(tx, ty));
                continue;
            }
            render_tile(tx, ty, staged + n * 16);
            staged_at[n] = tile_address(tx, ty);
            if (++n == STAGED_TILES) {
                staged_count = n;
                flush_tiles();
                n = 0;
            }
        }
    }
    if (n) {
        staged_count = n;
        flush_tiles();
    }
    if (!(LCDC & 0x80))
        LCDC = LCDC_ON;
}

/* The banks of the cartridge (MBC1) mapped at 0x4000. */
#define MBC_ROM_BANK REG(0x2000)
enum {
    BANK_MENU = 1,
    BANK_NATIVE,
    BANK_NATIVE_TILES
};
const uint8_t native_bank = BANK_NATIVE, native_tile_bank = BANK_NATIVE_TILES;

/* Only the first bit plane decides (native_gb.h). */
#define PALETTE NATIVE_PALETTE

/* menu_draw showed a full-screen menu made at build time, or moved the
   cursor on one: lcd_fb does not hold what is on the screen. */
static uint8_t native_drew;

uint8_t platform_native_show(uint16_t id)
{
    uint8_t shown;

    MBC_ROM_BANK = BANK_NATIVE;
    shown = native_show(id);
    MBC_ROM_BANK = BANK_MENU;
    if (!shown)
        return 0;
    /* The magnified rectangle's tiles are gone. */
    zoomed = 0;
    native_drew = 1;
    return 1;
}

void platform_native_cursor(uint8_t row, uint8_t on)
{
    MBC_ROM_BANK = BANK_NATIVE;
    native_cursor(row, on);
    MBC_ROM_BANK = BANK_MENU;
    native_drew = 1;
}

void platform_native_blank(void)
{
    /* Something else is drawn now: a made screen drawn before it, by a
       key script with no show in between, is not what show puts up. */
    native_drew = 0;
    if (native_up)
        BGP = 0;
}

/* Something was drawn into lcd_fb since it was last shown. */
static uint8_t lcd_drawn(void)
{
    const uint8_t *dirty = lcd_dirty;
    uint16_t n;

    for (n = sizeof lcd_dirty; n; n--)
        if (*dirty++)
            return 1;
    return 0;
}

/* Puts what menu_draw drew on the screen, or nothing when that was a made
   screen, which is up already. */
static void show(void)
{
    if (native_drew) {
        native_drew = 0;
        memset(lcd_dirty, 0, sizeof lcd_dirty);
        /* At power-on, present would have switched the LCD on. */
        if (!(LCDC & 0x80))
            LCDC = LCDC_ON;
        return;
    }
    if (native_up && lcd_drawn()) {
        MBC_ROM_BANK = BANK_NATIVE;
        native_leave();
        MBC_ROM_BANK = BANK_MENU;
    }
    present();
}

/* The pad is read once a frame by the vertical-blank handler in crt0.s:
   pad_last is what was held then (A, B, Select, Start in bits 0-3, Right,
   Left, Up, Down in bits 4-7) and pad_latch collects every new press until
   the main loop takes it. */
volatile uint8_t pad_last, pad_latch;

static uint8_t take_presses(void)
{
    uint8_t pressed;

    __asm__("di");
    pressed = pad_latch;
    pad_latch = 0;
    __asm__("ei");
    return pressed;
}

static uint8_t press(uint8_t key)
{
    return menu_key(key);
}

void main(void)
{
    uint8_t tx, ty, pressed, changed, seen = 0;
    uint16_t i;
    const char *key;

    wait_vblank();
    LCDC = 0;
    for (ty = 0; ty < TILES_Y; ty++)
        tile_row[ty] = ty < SPLIT_ROW ? (uint8_t *)0x8000 + ty * (TILES_X * 16)
                                      : (uint8_t *)0x9000 + (ty - SPLIT_ROW) * (TILES_X * 16);
    /* Every cell of the screen gets its own tile. */
    for (ty = 0; ty < TILES_Y; ty++)
        for (tx = 0; tx < TILES_X; tx++)
            VRAM_MAP[ty * 32 + tx] = (uint8_t)((ty * TILES_X + tx) % SPLIT_TILE);
    SCX = 0;
    SCY = 0;
    BGP = PALETTE;
    LYC = SPLIT_LINE - 1; /* the handler switches at the end of this line */
    STAT = 0x40; /* interrupt when LY reaches LYC */

    /* Video RAM holds whatever the boot ROM left: write every tile once. */
    for (i = 0; i < sizeof lcd_dirty; i++)
        lcd_dirty[i] = 1;

    NR52 = 0x80; /* sound on, full volume, channel 2 to both sides */
    NR50 = 0x77;
    NR51 = 0x22;

    menu_init();
    for (key = START_KEYS; *key; key++) {
        if (*key == 'p')
            menu_init();
        else if (*key == 't')
            menu_game_step();
        else
            press(*key == 'u' ? MENU_KEY_UP : *key == 'd' ? MENU_KEY_DOWN : *key == 'l' ? MENU_KEY_LEFT
                  : *key == 'r' ? MENU_KEY_RIGHT : *key == 's' ? MENU_KEY_SELECT
                  : *key == 'a' ? MENU_KEY_START : *key == 'e' ? MENU_KEY_ALT
                  : MENU_KEY_BACK);
    }
    menu_draw();
    show();

    IF = 0;
    IE = 0x03; /* vertical blank and LCD status */
    __asm__("ei");

    for (;;) {
        uint8_t frames;

        while (frame_count == seen)
            ;

        /* Keys first, so a press takes effect before the game's next move. */
        pressed = take_presses();
        changed = 0;
        if (pressed & PAD_A)
            changed |= press(MENU_KEY_SELECT);
        if (pressed & PAD_SELECT)
            changed |= press(MENU_KEY_ALT);
        if (pressed & PAD_START)
            changed |= press(MENU_KEY_START);
        if (pressed & PAD_B)
            changed |= press(MENU_KEY_BACK);
        if (pressed & PAD_UP)
            changed |= press(MENU_KEY_UP);
        if (pressed & PAD_DOWN)
            changed |= press(MENU_KEY_DOWN);
        if (pressed & PAD_LEFT)
            changed |= press(MENU_KEY_LEFT);
        if (pressed & PAD_RIGHT)
            changed |= press(MENU_KEY_RIGHT);
        /* Draw only when a key changed something, not on every press. The
           time that takes is not game time: a new game's clock starts once
           its board is on the screen. */
        if (changed) {
            menu_draw();
            show();
            seen = frame_count;
        }

        /* One menu tick per frame, catching up on frames spent drawing a
           move, but never by so much that the game visibly jumps ahead. */
        frames = (uint8_t)(frame_count - seen);
        seen += frames;
        if (frames > MAX_CATCH_UP)
            frames = MAX_CATCH_UP;
        while (frames--) {
            if (menu_tick()) {
                menu_draw();
                show();
            }
        }
    }
}
