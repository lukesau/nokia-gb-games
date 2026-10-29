/* What a platform layer keeps for the games between power cycles. */
#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <stdint.h>

/* The games in the order of the phone's Select game list. */
enum {
    GAME_SNAKE,
    GAME_SPACE_IMPACT,
    GAME_BUMPER,
    GAME_BANTUMI,
    GAME_LINK5,
    GAME_COUNT
};

/* Per-game settings, kept in save storage by the platform. */
struct game_settings {
    uint16_t top_score;
    uint8_t level;  /* 0 is the phone's first */
    uint8_t option; /* Snake II's maze, 0 for none; 0 to 15 */
};

/* What a game's handler is handed, as the phone's games application hands
   it: events, a context, and a result that says what to do next. */

/* Events. Keys are the phone's key codes; a held key repeats as its code
   plus GAME_KEY_REPEAT. */
enum {
    GAME_EVENT_TIMER = 0x00, /* the one-shot timer ran out */
    GAME_EVENT_TICK = 0x01,
    GAME_KEY_SCROLL_DOWN = 0x04,
    GAME_KEY_SCROLL_UP = 0x05,
    GAME_KEY_0 = 0x09,
    GAME_KEY_1 = 0x0a,
    GAME_KEY_2 = 0x0b,
    GAME_KEY_3 = 0x0c,
    GAME_KEY_4 = 0x0d,
    GAME_KEY_5 = 0x0e,
    GAME_KEY_6 = 0x0f,
    GAME_KEY_7 = 0x10,
    GAME_KEY_8 = 0x11,
    GAME_KEY_9 = 0x12,
    GAME_KEY_HASH = 0x14,
    GAME_KEY_STAR = 0x16,
    GAME_EVENT_START = 0x2b,
    GAME_EVENT_SUSPEND = 0x34,
    GAME_EVENT_RESUME = 0x35,
    GAME_KEY_REPEAT = 0x80
};

/* What a handler returns. */
enum {
    GAME_RESULT_NONE = 0x01,          /* nothing changed */
    GAME_RESULT_FAILED = 0x02,
    GAME_RESULT_UNUSED = 0x05,
    GAME_RESULT_RESTART_TIMERS = 0x13, /* start the tick and the one-shot timer afresh */
    GAME_RESULT_RESTART_TICK = 0x16,   /* start the tick afresh with the period now set */
    GAME_RESULT_GAME_OVER = 0x18,      /* the score is in the context */
    GAME_RESULT_SOUND = 0x1b,          /* play the context's sound */
    GAME_RESULT_ONE_SHOT_SOUND = 0x1f, /* start the one-shot afresh and play the sound */
    GAME_RESULT_ONE_SHOT = 0x20,       /* start the one-shot afresh */
    GAME_RESULT_REDRAW = 0x21
};

struct game_context {
    uint16_t period;   /* tick period, ms */
    uint16_t one_shot; /* one-shot timer, ms; 0 runs out at the next unit */
    uint32_t score;
    uint16_t sound;
    uint8_t level;     /* the phone's level setting plus one */
    uint8_t option;    /* Snake II's maze plus one */
};

/* The slots of the settings: one per game, GAME_SNAKE and so on, then one
   per Snake II maze for its top score, which the 3410 keeps per maze, then
   the same for the full-screen variant's board; then two for Munkiki's
   Castles (the Java game, which the port lists sixth): its top score with
   the level reached in the level and option fields, and its second score
   with its own Sounds and Lights switches. */
#define GAME_SLOT_SNAKE_MAZE(maze) (GAME_COUNT + (maze))
#define GAME_SLOT_SNAKE_FULL_MAZE(maze) (GAME_COUNT + 6 + (maze))
#define GAME_SLOT_MUNKIKI (GAME_COUNT + 12)
#define GAME_SLOT_MUNKIKI_2 (GAME_COUNT + 13)
#define GAME_SLOT_COUNT (GAME_COUNT + 14)

/* Load reports whether anything had been saved, and gives zeros if not. */
uint8_t platform_settings_load(uint8_t game, struct game_settings *out);
void platform_settings_save(uint8_t game, const struct game_settings *in);

/* The games' own settings, the phone's Games, Settings pages: each 0 or 1.
   Game lights is kept but does nothing here. The port starts with all
   three on. */
struct game_options {
    uint8_t sounds;
    uint8_t lights;
    uint8_t shakes;
};

/* Load reports whether anything had been saved, and leaves `out` as it
   was if not. */
uint8_t platform_options_load(struct game_options *out);
void platform_options_save(const struct game_options *in);

#endif
