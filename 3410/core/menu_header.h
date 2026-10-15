/* The menu pages' header (menu.c), apart from the menus so that a
   platform can keep it, and its tiny font, with something else (the Game
   Boy, in the bank of its full-screen menus, through far.c). */
#ifndef CORE_MENU_HEADER_H
#define CORE_MENU_HEADER_H

#include <stdint.h>

#define MENU_HEADER_LINE_Y 3

/* The title, at most MENU_HEADER_MAX characters, and the number. */
#define MENU_HEADER_MAX 23
void menu_header_draw(const char *title, uint8_t number);

#endif
