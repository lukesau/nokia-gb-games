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

/* The menus as the fork's docs/games_menus_5110.md maps them. Games is
   entry 6 of the main menu's 9, with one still 64x14 icon. */
#define GAMES_MENU_NUMBER 6
#define GAMES_MENU_THUMB 14
#define PHONE_GAMES_ICON_ANIMATED 0
#define GAMES_ICON_HEIGHT 14
#define PHONE_SPARKLE 0       /* a plain Top score note */
#define PHONE_LEVEL_STRIPES 1 /* striped Level bars on a 7-pixel pitch */
/* The Top score and Game over pages close after this many phone ticks. */
#define PHONE_TOP_SCORE_TICKS 376
#define PHONE_GAME_OVER_TICKS 376
/* Logic keeps no score (its score field is 0), so its menu has no Top
   score. */
#define GAME_HAS_TOP_SCORE(g) ((g) != GAME_LOGIC)

#endif
