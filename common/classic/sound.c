#include "sound.h"

struct note {
    uint16_t hz; /* 0 for a rest */
    uint16_t ms;
};

/* Each sound ends with a zero-length rest. */
static const struct note eat[] = { { 2637, 7 }, { 0, 0 } };
static const struct note game_over[] = { { 440, 14 }, { 0, 16 }, { 440, 16 }, { 0, 16 }, { 440, 16 }, { 0, 0 } };
static const struct note top_score[] = {
    { 587, 145 }, { 0, 148 }, { 587, 117 }, { 0, 23 }, { 587, 117 }, { 0, 23 },
    { 880, 412 }, { 0, 23 },  { 587, 117 }, { 0, 23 }, { 880, 444 }, { 0, 0 },
};

static const struct note solved[] = { { 523, 117 }, { 0, 23 }, { 784, 444 }, { 0, 0 } };

static const struct note *const sounds[] = { eat, game_over, top_score, solved };

static volatile uint8_t requested; /* a sound to start, plus one; 0 for none */
static const struct note *playing; /* the note sounding now, or null */
static int32_t left_us;            /* time it still has to run */

/* Only leaves a request: sound_tick may run from an interrupt, and it alone
   touches the sequencer's state. */
void sound_play(uint8_t sound)
{
    requested = (uint8_t)(sound + 1);
}

void sound_tick(uint16_t us)
{
    uint8_t request = requested;

    if (request) {
        requested = 0;
        playing = sounds[request - 1];
        left_us = (int32_t)playing->ms * 1000;
        platform_tone(playing->hz);
        return;
    }
    if (!playing)
        return;
    left_us -= us;
    while (left_us <= 0) {
        playing++;
        if (!playing->ms) {
            playing = 0;
            platform_tone(0);
            return;
        }
        left_us += (int32_t)playing->ms * 1000;
        platform_tone(playing->hz);
    }
}
