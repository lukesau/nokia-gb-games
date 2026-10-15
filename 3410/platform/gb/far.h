/* The ROM is banks of 16 KiB, of which the cartridge (MBC5) maps the
   first always and one of the others at a time:

   0  startup, this layer, the framebuffer, the sprite layer, the game's
      timers and the sounds, which play from an interrupt;
   1  the menus, the fonts and the text (what the cartridge RAM keeps is
      in bank 0);
   2  Snake II and its data, the score's digits among it, its title, its
      game-over picture and its High scores page;
   3  the full-screen menus made at build time and what shows them, and
      the Instructions' texts;
   4  their tiles;
   5-8  Space Impact (core/si_int.h), each bank starting with the same copy
      of its data so that it reads the same at the same address whichever
      of them is mapped, and 11 its demos; 9 and 10 its pictures' rows;
   12  Bantumi (common/bantumi/), its data, its search and the graphics
      library's drawing again.

   The main loop runs with bank 1 mapped. A call into bank 2 or 3 goes
   through one of the far_ functions here, which map that bank for the call
   and then the one that was mapped before; Space Impact's functions are
   banked (bcall.s). */
#ifndef GB_FAR_H
#define GB_FAR_H

#include <stdint.h>

enum {
    BANK_MENU = 1,
    BANK_SNAKE,
    BANK_NATIVE,
    BANK_NATIVE_TILES
};

extern uint8_t far_mapped;

void far_bank(uint8_t bank);

/* native_gb.h's, from the bank they are in. */
uint8_t far_native_show(uint16_t id);
void far_native_cursor(uint8_t row, uint8_t on);
uint8_t far_native_leave(void);
void far_native_palette(uint8_t shades);

#endif
