/* What the 5110 port gives the shared code in common/classic. */
#ifndef PHONE_H
#define PHONE_H

#define PHONE_NAME "Nokia 5110"

/* The games in the firmware's table order (0x2a79cc in v5.28), which is
   also the list's. */
enum {
    GAME_MEMORY,
    GAME_SNAKE,
    GAME_LOGIC,
    GAME_TABLE,
    /* Settings slot of the full-screen Snake, after the phone's three. */
    GAME_SNAKE_FULL = GAME_TABLE,
    GAME_SLOTS
};

#define GAME_COUNT 3 /* in the list of games */
#define PHONE_ROTATION 0
#define PHONE_LOGIC 1

/* Taken from the 3210 until the 5110's menus are mapped: the main menu's
   Games entry and its scrollbar. Neither the 3210's Games icon nor its Top
   score animation is in the 5110's firmware. */
#define GAMES_MENU_NUMBER 6
#define GAMES_MENU_THUMB 12
#define PHONE_GAMES_ICON 0
#define PHONE_SPARKLE 0

#endif
