/* The interface between the game core and a platform layer.

   Every game is one handler taking one event code, as in the firmware.
   The platform layer delivers events and supplies the services below. */
#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <stdint.h>

/* The phone's games: GAME_* in its firmware's order, and what it has. */
#include "phone.h"

enum {
    GAME_EVENT_INIT = 0x49,
    GAME_EVENT_RESUME = 0x53,
    GAME_EVENT_TICK = 0x54,
    GAME_EVENT_DRAW = 0x57
    /* Key events are the key's ASCII code: '1'..'9', '*', '#'. */
};


typedef void (*game_handler)(int event);

/* Per-game settings, kept in save storage by the platform. */
struct game_settings {
    uint16_t top_score;
    uint8_t level;  /* index 0..8 */
    uint8_t option; /* the other ports' second setting; 0 here */
};

/* One scheduler tick is 7.78125 ms (249/32 ms). */
#define GAME_TICK_US 7781

/* Services a platform layer provides. */
void platform_request_tick(unsigned ticks); /* deliver GAME_EVENT_TICK after this many ticks */
/* Load gives zeros, and returns 0, when nothing has been saved. */
uint8_t platform_settings_load(uint8_t game, struct game_settings *out);
void platform_settings_save(uint8_t game, const struct game_settings *in);

#endif
