/* The Game Boy side of the full-screen menus made at build time: see
   native_tiles.c, and native_put.s for the copying, which is in the first
   bank because it maps the bank of the tiles. */
#ifndef NATIVE_GB_H
#define NATIVE_GB_H

#include <stdint.h>

/* The bank of native_tiles.c and the screens, and the bank of the tiles.
   The port defines them. */
extern const uint8_t native_bank, native_tile_bank;

/* A made screen is up. */
extern uint8_t native_up;

/* native_tiles.c, called with native_bank mapped. */
uint8_t native_show(uint16_t id);
void native_cursor(uint8_t row, uint8_t on);
/* Puts lcd_fb on the screen, every cell its own tile again, as the menus'
   framebuffer has it; returns 0 if no made screen was up. */
uint8_t native_leave(void);
/* The same with the tiles left as they are and the screen left blank (the
   palette all light), for a platform that draws them its own way; it puts
   the palette back. */
uint8_t native_leave_blank(void);

/* The palette every port shows its tiles with: colours 1 and 3 dark, 0 and
   2 light, so that only the first bit plane decides, which is all
   native_tiles.c writes. */
#define NATIVE_PALETTE 0xcc

/* A port that runs in Color mode on a Game Boy Color or Advance, at double
   speed (the 3310's and the 3410's: its header says it can, makebin -yc),
   defines NATIVE_PLATFORM_PALETTE, keeps in gb_cgb what the boot ROM left
   in A (its crt0.s), and sets the palette with native_palette, which also
   sets the Color palette that stands for BGP's shades. Its first call, at
   power-on with the LCD off, switches a Color or an Advance (gb_cgb is
   GB_CGB) to double speed, the tiles' attributes cleared; the timer then
   counts twice as fast. In native_bank, called through the port's far_
   functions from elsewhere. */
#ifdef NATIVE_PLATFORM_PALETTE
extern uint8_t gb_cgb;
#define GB_CGB 0x11
void native_palette(uint8_t shades);
#endif

/* native_put.s. Video RAM is written between the lines being drawn, four
   bytes at a time, as flush_tiles does. */
/* 8 bytes to the first bit plane of the tile at `tile`. */
void native_put_tile(uint8_t *tile, const uint8_t *bytes);
/* 8 bytes from the tiles' bank to `out`. */
void native_get_far(uint8_t *out, const uint8_t *bytes);
/* 20 bytes, a row of the tile map. */
void native_put_row(uint8_t *map, const uint8_t *bytes);
void native_put_byte(uint8_t *at, uint8_t value);
/* A row of a screen: the tiles of its made cells copied to theirs, the
   row's first at `tiles`, and native_map_row filled in, each cell's own
   tile being native_first + its column. A cell whose tile in native_old,
   the screen up's, is the same is not copied again. Returns the next
   row. */
extern uint8_t native_first, native_map_row[20];
extern uint16_t native_old[20];
const uint16_t *native_row(const uint16_t *row, uint8_t *tiles);
/* A row of lcd_fb's 20 cells, from its first byte, to their tiles. */
void native_fb_row(uint8_t *tiles, const uint8_t *fb);

#endif
