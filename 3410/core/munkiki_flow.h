/* Munkiki's Castles: the game's flow (munkiki_game.c, the JAR's class
   KKM) and its pages (munkiki_pages.c, the drawing of its screens), which
   the Game Boy keeps in banks of their own: what they share. The JAR's
   names are in the comments. */
#ifndef CORE_MUNKIKI_FLOW_H
#define CORE_MUNKIKI_FLOW_H

#include "munkiki_int.h"

/* The screens (KKM.aj). */
enum {
    SCREEN_LIST = -1,
    SCREEN_TITLE = 0,
    SCREEN_GAME = 2,
    SCREEN_SCORES = 3,
    SCREEN_HELP = 4,
    SCREEN_DEMO = 5
};
/* The phone's own pages over the canvas (KKM.ai). */
enum {
    LIST_NONE = 0,
    LIST_PAUSE = 24,
    LIST_MAIN = 25,
    LIST_HELP = 26,
    LIST_OPTIONS = 27,
    LIST_LEVEL = 28
};
/* The game's parts (KKM.ab). */
enum {
    PART_HINT = 0,  /* the "(... press * to restart)" text */
    PART_LEVEL = 1, /* "Level N" and what to collect */
    PART_PLAY = 2,
    PART_DONE = 3,  /* the level's end, Munkiki dancing */
    PART_BONUS = 4,
    PART_MAP = 5,
    PART_OVER = 7
};
/* The texts, by index in the JAR's language file. */
enum {
    TEXT_CONTINUE,
    TEXT_NEW_GAME,
    TEXT_HIGH_SCORES,
    TEXT_INSTRUCTIONS,
    TEXT_LEVEL_N,
    TEXT_COLLECT_N,
    TEXT_COLLECT_N_OR_KEY,
    TEXT_HELP_FIRST,       /* 7 to 16, with the demos between */
    TEXT_HINT = 15,
    TEXT_LIVES_LEFT = 17,
    TEXT_RETURN,
    TEXT_RESTART,
    TEXT_GAME_MENU,
    TEXT_END_GAME,
    TEXT_OPTIONS,
    TEXT_SETTINGS,
    TEXT_GAME_SOUNDS,
    TEXT_LIGHTS,
    TEXT_SOUNDS,
    TEXT_SHAKES,
    TEXT_OFF,
    TEXT_ON
};
/* The Instructions: texts and, as negatives, the demos' levels (KKM.e). */
extern const int8_t mk_help_order[16];

/* The flow's state. */
struct mk_flow {
    int8_t screen;    /* aj */
    uint8_t list;     /* ai */
    uint8_t part;     /* ab */
    int16_t counter;  /* ad */
    uint8_t help_page; /* ag */
    int16_t bonus_time, bonus_key, bonus_warp, bonus_last; /* r, o, q, p */
    uint16_t top_score, second_score; /* n, m */
    uint8_t level_reached; /* w */
    uint8_t can_continue;  /* af: a game to go on with */
    uint8_t demo_fresh;    /* ac */
    uint8_t sounds_on, lights_on; /* k, j */
    uint8_t game_exists;   /* ak != null */
    uint8_t game_saved;    /* the JAR's KKM record exists */
    uint8_t selection, list_top, list_count; /* the list shown */
    uint8_t option_row;    /* the Options page's cursor */
    uint8_t text_scroll;   /* the Instructions page's first row shown */
    int16_t percent;       /* what the game over page says */
};
extern struct mk_flow mkf;

/* munkiki_pages.c: how tall a part's text is (the hint's or the level's,
   for its scrolling), how many lines the Instructions page has, the HUD
   over the game's picture. */
uint8_t mk_page_height(uint8_t part) MK_FAR;
uint8_t mk_help_lines(void) MK_FAR;
void mk_hud_draw(void) MK_FAR;

#endif
