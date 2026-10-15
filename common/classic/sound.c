#include "sound.h"

#include "games.h"

struct note {
    uint8_t note; /* semitones above 440 Hz, or REST */
    uint16_t ms;
};

#define REST SOUND_SILENCE

/* Each sound ends with a zero-length rest. The pitches are 2637 Hz (E7,
   31), 440 (A4, 0), 587 (D5, 5), 880 (A5, 12), 523 (C5, 3) and 784 (G5,
   10). */
static const struct note eat[] = { { 31, 7 }, { REST, 0 } };
static const struct note game_over[] = { { 0, 14 }, { REST, 16 }, { 0, 16 }, { REST, 16 }, { 0, 16 }, { REST, 0 } };
static const struct note top_score[] = {
    { 5, 145 },  { REST, 148 }, { 5, 117 },  { REST, 23 }, { 5, 117 },  { REST, 23 },
    { 12, 412 }, { REST, 23 },  { 5, 117 },  { REST, 23 }, { 12, 444 }, { REST, 0 },
};

static const struct note solved[] = { { 3, 117 }, { REST, 23 }, { 10, 444 }, { REST, 0 } };

static const struct note *const sounds[] = { eat, game_over, top_score, solved };

static volatile uint8_t requested; /* a sound to start, plus one; 0 for none */
static const struct note *playing; /* the note sounding now, or null */
static int32_t left_us;            /* time it still has to run */

/* Only leaves a request: sound_advance may run from an interrupt, and it alone
   touches the sequencer's state. */
void sound_play(uint8_t sound)
{
    requested = (uint8_t)(sound + 1);
}

void sound_tick(void)
{
    sound_advance(GAMES_UNIT_US);
}

void sound_advance(uint16_t us)
{
    uint8_t request = requested;

    if (request) {
        requested = 0;
        playing = sounds[request - 1];
        left_us = (int32_t)playing->ms * 1000;
        platform_tone(playing->note);
        return;
    }
    if (!playing)
        return;
    left_us -= us;
    while (left_us <= 0) {
        playing++;
        if (!playing->ms) {
            playing = 0;
            platform_tone(SOUND_SILENCE);
            return;
        }
        left_us += (int32_t)playing->ms * 1000;
        platform_tone(playing->note);
    }
}
