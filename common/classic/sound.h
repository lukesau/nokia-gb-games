/* The buzzer sounds the games make, as traced from the firmware in MAME: the
   pitches are exact, the lengths are the traced ones corrected for MAME's
   slightly fast clock. */
#ifndef CORE_SOUND_H
#define CORE_SOUND_H

#include <stdint.h>

enum {
    SOUND_EAT,       /* one short high blip; also a pair found in Memory */
    SOUND_GAME_OVER, /* three quick low pulses */
    SOUND_TOP_SCORE, /* a six-note tune */
    SOUND_SOLVED     /* Rotation's board is in order: two rising notes */
};

/* Starts a sound at the next sound_tick. */
void sound_play(uint8_t sound);

/* Advances the sound in progress by `us` microseconds. A platform calls it
   once a frame, from its frame interrupt, so that a sound keeps time while
   the main loop is busy drawing. */
void sound_tick(uint16_t us);

/* The platform's buzzer: a square wave of this many hertz, 0 for silence. */
void platform_tone(uint16_t hz);

#endif
