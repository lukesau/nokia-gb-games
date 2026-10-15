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

/* Starts a sound at the next sound_advance. */
void sound_play(uint8_t sound);

/* Advances the sound in progress by `us` microseconds. A platform calls it
   from an interrupt, so that a sound keeps time while the main loop is
   busy drawing: once a frame, or through sound_tick from a timer. */
void sound_advance(uint16_t us);

/* sound_advance by one of the phone's timer units (GAMES_UNIT_US), as the
   shared GBA layer calls it from its timer. */
void sound_tick(void);

/* The platform's buzzer: a square wave this many semitones above 440 Hz,
   or SOUND_SILENCE, as the other ports' (every pitch these games play is
   one). */
#define SOUND_SILENCE 0xff
void platform_tone(uint8_t note);

#endif
