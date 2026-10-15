#include "bantumi_play.h"

#include "games.h"
#include "sound.h"

static uint16_t units;   /* phone timer units to the game's next tick; 0 none */
static uint16_t left_us; /* time not yet turned into units */
static uint8_t held;     /* stopped until a key */

/* The period in timer units: the phone divides milliseconds by 255/32.
   The last one worked out is kept: the Game Boy divides slowly. */
static uint16_t period_units(void)
{
    static uint16_t period, n;

    if (bantumi_period != period) {
        period = bantumi_period;
        n = (uint16_t)(period / 255 * 32 + period % 255 * 32 / 255);
    }
    return n;
}

uint8_t bantumi_play_call(uint8_t event, uint8_t a) BANTUMI_FAR
{
    uint8_t done = bantumi_event(event, a);

    if (bantumi_sound) {
        if (games_options.sounds)
            sound_play(bantumi_sound);
        bantumi_sound = 0;
    }
    if (done & BANTUMI_DONE_CLOSE) {
        units = 0;
        return BANTUMI_PLAY_CLOSED;
    }
    return 0;
}

uint8_t bantumi_play_open(uint8_t event, uint8_t a) BANTUMI_FAR
{
    uint8_t done = bantumi_play_call(event, a);

    held = 0;
    if (!done)
        units = period_units();
    return done;
}

void bantumi_play_stop(uint8_t hold) BANTUMI_FAR
{
    units = 0;
    held = hold;
}

uint8_t bantumi_play_elapse(uint16_t us) BANTUMI_FAR
{
    uint8_t done = 0;

    for (left_us += us; left_us >= GAMES_UNIT_US; left_us -= GAMES_UNIT_US) {
        if (!units || --units)
            continue;
        units = period_units();
        done = (uint8_t)(BANTUMI_PLAY_TICKED | bantumi_play_call(BANTUMI_EVENT_TICK, 0));
        if (done & BANTUMI_PLAY_CLOSED)
            break;
    }
    return done;
}

uint8_t bantumi_play_key(uint8_t code) BANTUMI_FAR
{
    if (held) {
        held = 0;
        units = period_units();
    }
    return bantumi_play_call(BANTUMI_EVENT_KEY_DOWN, code);
}
