/* What the 3210 port gives the shared code in common/classic. */
#ifndef PHONE_H
#define PHONE_H

#define PHONE_NAME "Nokia 3210"

/* The games in the firmware's table order; the list shows the first three. */
enum {
    GAME_ROTATION,
    GAME_SNAKE,
    GAME_MEMORY,
    GAME_REACTION,
    GAME_LOGIC,
    GAME_TABLE,
    /* Settings slot of the full-screen Snake, after the phone's five. */
    GAME_SNAKE_FULL = GAME_TABLE,
    GAME_SLOTS
};

#define GAME_COUNT 3 /* in the list of games */
#define PHONE_ROTATION 1
#define PHONE_LOGIC 0

#define GAMES_MENU_NUMBER 6 /* Games is entry 6 of the phone's main menu */
#define GAMES_MENU_THUMB 12 /* where that puts the main menu's scrollbar */
#define PHONE_GAMES_ICON_ANIMATED 1 /* the Games icon: 13 pictures */
#define GAMES_ICON_HEIGHT 16
#define PHONE_SPARKLE 1     /* the Top score page's animation */
#define PHONE_LEVEL_STRIPES 0 /* outlined Level bars on an 8-pixel pitch */
/* The Top score and Game over pages close after this many phone ticks. */
#define PHONE_TOP_SCORE_TICKS 768
#define PHONE_GAME_OVER_TICKS 385
#define GAME_HAS_TOP_SCORE(g) 1

#endif
