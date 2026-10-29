/* The games' sounds on the phone's buzzer: each a run of notes read from
   the firmware (game_sounds in the extracted assets). */
#ifndef CORE_SOUND_H
#define CORE_SOUND_H

#include <stdint.h>

/* Starts one of the games' sounds (a SNAKE2_SOUND_ or SI_SOUND_ code) at
   the next sound_tick, in place of the one in progress, as on the phone. */
void sound_play(uint8_t sound);

/* Starts a run of notes kept elsewhere, in the same form: Munkiki's
   Castles' (munkiki_data.h), which the Game Boy keeps in bank `bank` (0
   when none need be mapped to read them). */
void sound_play_notes(const uint8_t *notes, uint8_t bank);

/* Lets one of the phone's timer units (GAMES_UNIT_US) pass for the sound in
   progress. A platform calls it from a timer interrupt, since the notes
   are shorter than a screen frame, and need not while sound_active is 0. */
void sound_tick(void);
extern volatile uint8_t sound_active;

/* The platform's buzzer: a square wave this many semitones above 440 Hz,
   0 to 47, or SOUND_LOW plus semitones above 110 Hz, 0 to 23, for the
   two octaves under (Munkiki's Castles' tunes go there), or
   SOUND_SILENCE. In a sound, SOUND_REST in place of a note is silence
   for its length, and SOUND_SILENCE ends the sound. */
#define SOUND_SILENCE 0xff
#define SOUND_REST 0xfe
#define SOUND_LOW 0x40
void platform_tone(uint8_t note);

#endif
