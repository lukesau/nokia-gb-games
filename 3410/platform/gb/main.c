/* Game Boy layer. The core's framebuffer is the whole 160x144 screen, shown
   as a 20x18 block of background tiles, one tile per 8x8 cell; the phone's
   96x65 LCD is a window in the middle of it, on whole tiles. In the menus
   A, Select or Start is the phone's Navi key, B is its C key and the D-pad
   scrolls; Start on the first screen picks the full-screen variant, whose
   menus fill the screen. In the game the D-pad steers, A turns clockwise,
   B anticlockwise, and Start or Select pauses.

   This file is in the ROM's first 16 KiB with the rest of what every part
   of the program needs; far.c says what is in the other banks. */
#include <stdint.h>

#include <string.h>

#include "far.h"
#include "game.h"
#include "game_assets.h"
#include "games.h"
#include "lcd.h"
#include "menu.h"
#include "native_gb.h"
#include "native_tiles.h"
#include "screen.h"
#include "sound.h"

#define REG(addr) (*(volatile uint8_t *)(addr))
#define P1 REG(0xff00)
#define TMA REG(0xff06)
#define TAC REG(0xff07)
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

/* lcd_fb's tiles have both bit planes alike and the made menus' only the
   first (native_tiles.c): the palette takes the colour from the first
   alone. */
#define PALETTE NATIVE_PALETTE

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

/* Keys pressed at power-on, for scripted screenshots; see menu_script. */
#ifndef START_KEYS
#define START_KEYS ""
#endif

/* Cartridge RAM (MBC5, 8 KiB, battery-backed), which stays enabled. Its
   layout is in save.c. */
#define MBC_RAM_ENABLE REG(0x0000)

/* The MBC5's RAM bank register: bit 3 is the motor of a rumble cartridge.
   The RAM bank stays 0. */
#define MBC_RAM_BANK REG(0x4000)
#define MBC_RUMBLE 0x08

/* Tiles converted at a time before being copied to video RAM. */
#define STAGED_TILES 4
/* Most frames of game time made up at once after a slow draw. */
#define MAX_CATCH_UP 18

/* Frames since power-on, counted by the vertical-blank handler in crt0.s. */
volatile uint8_t frame_count;

/* Tile data on its way to video RAM, and where it goes; flush_tiles in
   crt0.s copies it between the lines the LCD is drawing. */
uint8_t staged[STAGED_TILES * 16];
uint8_t *staged_at[STAGED_TILES];
uint8_t staged_count;
const uint8_t *staged_from; /* flush_tiles' place in staged */

void flush_tiles(void);
/* Whether any cell of lcd_dirty is set (blocks.s). */
uint8_t gb_any_dirty(void);
typedef char lcd_dirty_cells[sizeof lcd_dirty == 360 ? 1 : -1];

/* The buzzer is pulse channel 2: a 50% square wave at full volume, of
   131072 / (2048 - its frequency register) hertz. These are eight times
   131072 over the hertz of the twelve semitones from 440 Hz up; an octave
   higher is half. */
static const uint16_t tone_divider[12] = { 2383, 2249, 2123, 2004, 1891, 1785, 1685, 1591, 1501, 1417, 1337, 1262 };

/* Called by sound_tick, from the timer interrupt. */
void platform_tone(uint8_t note)
{
    uint8_t octave = 0;
    uint16_t period;

    if (note == SOUND_SILENCE) {
        NR22 = 0x00; /* volume 0 switches the channel off */
        NR24 = 0x80;
        return;
    }
    if (note >= SOUND_LOW) {
        /* Two octaves under 440 Hz: the dividers doubled and doubled. */
        note -= SOUND_LOW;
        for (; note >= 12; note -= 12)
            octave++;
        period = (uint16_t)(2048 - (((tone_divider[note] << (2 - octave)) + 4) >> 3));
    } else {
        for (; note >= 12; note -= 12)
            octave++;
        period = (uint16_t)(2048 - (((tone_divider[note] >> octave) + 4) >> 3));
    }
    NR21 = 0x80;
    NR22 = 0xf0;
    NR23 = (uint8_t)period;
    NR24 = (uint8_t)(0x80 | (period >> 8));
}

void platform_rumble(uint8_t on)
{
    MBC_RAM_BANK = on ? MBC_RUMBLE : 0;
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

/* Where each row of tiles starts in video RAM. */
static uint8_t *tile_row[TILES_Y];

static uint8_t *tile_address(uint8_t tx, uint8_t ty)
{
    return tile_row[ty] + (uint16_t)(tx << 4);
}

uint8_t *gb_tile_address(uint8_t tx, uint8_t ty)
{
    return tile_address(tx, ty);
}

/* Brings video RAM up to date with the cells of lcd_fb drawn to since the
   last call. The LCD stays on: the tiles go in a few at a time between the
   lines being drawn, so a changed screen fills in over a few frames instead
   of blinking. */
static void present(void)
{
    uint8_t tx, ty, n = 0;
    uint8_t *dirty = lcd_dirty;

    if (!gb_any_dirty())
        return;
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
}

/* menu_draw showed a full-screen menu made at build time, or moved the
   cursor on one: lcd_fb does not hold what is on the screen. */
static uint8_t native_drew;

uint8_t platform_native_show(uint16_t id)
{
    if (!far_native_show(id))
        return 0;
    native_drew = 1;
    return 1;
}

void platform_native_cursor(uint8_t row, uint8_t on)
{
    far_native_cursor(row, on);
    native_drew = 1;
}

void platform_native_blank(void)
{
    if (native_up)
        far_native_palette(0);
}

/* Something was drawn into lcd_fb since it was last shown. */
static uint8_t lcd_drawn(void)
{
    return gb_any_dirty();
}

/* Puts what menu_draw drew on the screen: the menus through lcd_fb, the
   game through it too (screen.c), or a made screen, which is up already. */
static void show(void)
{
    if (native_drew) {
        native_drew = 0;
        memset(lcd_dirty, 0, sizeof lcd_dirty);
        return;
    }
    if (native_up && lcd_drawn())
        far_native_leave();
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

/* The buttons down, as menu_held wants them. */
static uint8_t held_keys(void)
{
    uint8_t pad = pad_last, keys = 0;

    if (pad & PAD_UP)
        keys |= 1 << MENU_KEY_UP;
    if (pad & PAD_DOWN)
        keys |= 1 << MENU_KEY_DOWN;
    if (pad & PAD_LEFT)
        keys |= 1 << MENU_KEY_LEFT;
    if (pad & PAD_RIGHT)
        keys |= 1 << MENU_KEY_RIGHT;
    if (pad & PAD_A)
        keys |= 1 << MENU_KEY_SELECT;
    if (pad & PAD_B)
        keys |= 1 << MENU_KEY_BACK;
    return keys;
}

#ifdef SI_BENCH
/* The benchmark ROM (make bench-gb): a recorded game played as fast as
   the Game Boy can, as platform/host/si_replay_main.c plays it, drawing
   whenever the game asks. The empty functions mark each part of each
   step for tools/bench_report.py, which times them in SameBoy. */
#include "rand.h"
#include "si.h"
#include "si_bench.h"
#include "si_rows.h"

#define BENCH_BANK 19

void bench_step(void) { __asm__("nop"); }
void bench_render(void) { __asm__("nop"); __asm__("nop"); }
void bench_present(void) { __asm__("nop"); __asm__("nop"); __asm__("nop"); }
void bench_drawn(void) { __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); }
void bench_end(void) { __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); }

static const uint8_t *const bench_chunks[SI_BENCH_CHUNKS] = { SI_BENCH_LIST };
static uint8_t bench_chunk, bench_run;
static uint16_t bench_at, bench_keys;

static uint8_t bench_byte(void)
{
    uint8_t b, was = far_mapped;

    far_bank((uint8_t)(BENCH_BANK + bench_chunk));
    b = bench_chunks[bench_chunk][bench_at];
    far_bank(was);
    if (++bench_at == SI_BENCH_CHUNK) {
        bench_at = 0;
        bench_chunk++;
    }
    return b;
}

/* The next event, 0xff for the lives, and the keys held for it; 0 at the
   end. */
static uint8_t bench_decode(uint8_t *e, uint8_t *a, uint16_t *keys)
{
    uint8_t op;

    for (;;) {
        if (bench_run) {
            bench_run--;
            *e = 0;
            *a = 0;
            *keys = bench_keys;
            return 1;
        }
        op = bench_byte();
        if (op < 0x40) {
            bench_run = (uint8_t)(op + 1);
            continue;
        }
        if (op == 0x80) {
            bench_keys = bench_byte();
            bench_keys |= (uint16_t)bench_byte() << 8;
            continue;
        }
        if (op == 0xfe)
            return 0;
        *keys = bench_keys;
        if (op == 0xff) {
            *e = 0xff;
            *a = bench_byte();
        } else if (op == 0xc0) {
            *e = bench_byte();
            *a = bench_byte();
        } else {
            *e = op & 0x20 ? SI_EVENT_KEY_UP : SI_EVENT_KEY_DOWN;
            *a = op & 0x0f;
        }
        return 1;
    }
}

static void show(void);

static void si_bench(void)
{
    uint8_t e, a, ne = 0, na = 0, more, next, first = 1, done;
    uint16_t keys, nk = 0;

    game_srand(SI_BENCH_SEED);
    more = bench_decode(&e, &a, &keys);
    while (more) {
        next = bench_decode(&ne, &na, &nk);
        if (e == 0xff) {
            si_debug_lives((int8_t)a);
        } else {
            si_keys_held = keys;
            bench_step();
            done = si_event(e, a);
            if ((done & SI_DONE_REDRAW) && !(e == SI_EVENT_NEW_GAME && next && ne == SI_EVENT_CONTINUE)) {
                bench_render();
                si_render_rows();
                bench_present();
                si_rows_present(first);
                show();
                first = 0;
                bench_drawn();
            }
        }
        e = ne;
        a = na;
        keys = nk;
        more = next;
    }
    bench_end();
}
#endif

void main(void)
{
    uint8_t tx, ty, pressed, changed, seen = 0;
    uint16_t i;

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
    /* On a Game Boy Color or Advance also Color mode, at double speed. */
    far_native_palette(PALETTE);
    screen_cuts();
    STAT = 0x40; /* interrupt when LY reaches LYC: the cuts in crt0.s */

    /* Video RAM holds whatever the boot ROM left: write every tile once. */
    for (i = 0; i < sizeof lcd_dirty; i++)
        lcd_dirty[i] = 1;

    NR52 = 0x80; /* sound on, full volume, channel 2 to both sides */
    NR50 = 0x77;
    NR51 = 0x22;
    /* The timer interrupts once per unit of the phone's timers, for the
       sounds: 4096 Hz over 32 is every 7.8 ms (at double speed the timer
       counts twice as fast, so over 64). */
    TMA = gb_cgb == GB_CGB ? 0xc0 : 0xe0;
    TAC = 0x04;

    MBC_RAM_ENABLE = 0x0a;
    MBC_RAM_BANK = 0;
    far_bank(BANK_MENU);

    menu_init();
    menu_script(START_KEYS);
    menu_draw();
    show();
    LCDC = LCDC_ON;

    IF = 0;
    IE = 0x07; /* vertical blank, LCD status and timer */
    __asm__("ei");
#ifdef SI_BENCH
    si_bench();
    for (;;)
        __asm__("halt");
#endif

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
        changed |= menu_held(held_keys());
        /* Draw only when a key changed something, not on every press. The
           time that takes is not game time: a new game's clock starts once
           its board is on the screen. But a key in a game under way, a
           shot fired, draws the game again, and that time is the game's:
           it is made up like any other draw's, or firing would slow the
           game down. */
        if (changed) {
            uint8_t under_way = menu_drew_picture;

            menu_draw();
            under_way = under_way && menu_drew_picture;
            show();
            if (!under_way)
                seen = frame_count;
        }

        /* One menu tick per frame that has passed, then one picture. The
           game takes several frames to put on the screen when much of it
           changes; its ticks are then all made up before the next picture,
           so that it keeps the phone's pace and shows fewer pictures. Never
           more than MAX_CATCH_UP frames' worth, or a long draw would be
           followed by a visible jump. */
        frames = (uint8_t)(frame_count - seen);
        seen += frames;
        if (frames > MAX_CATCH_UP)
            frames = MAX_CATCH_UP;
        changed = 0;
        while (frames--)
            changed |= menu_tick();
        if (changed) {
            menu_draw();
            show();
        }
    }
}
