/* Bantumi driven as the 3410's games application drives it (bantumi.h):
   a tick every period the game asks for, counted in the phone's timer
   units, its keys as they go down, and its sounds played. Apart from the
   menus, so that a platform can keep it with the game (the Game Boy, in
   Bantumi's bank). */
#ifndef CORE_BANTUMI_PLAY_H
#define CORE_BANTUMI_PLAY_H

#include <stdint.h>

#include "bantumi.h"

/* What the calls report. */
#define BANTUMI_PLAY_CLOSED 1 /* the game asked to be closed */
#define BANTUMI_PLAY_TICKED 2 /* a tick came */

/* An event, and the sound it asked for. */
uint8_t bantumi_play_call(uint8_t event, uint8_t a) BANTUMI_FAR;
/* The same, then the timer started at the period the game asks for. */
uint8_t bantumi_play_open(uint8_t event, uint8_t a) BANTUMI_FAR;
/* The timer stopped; held, it starts again at the next key, as the phone
   does with a game continued. */
void bantumi_play_stop(uint8_t hold) BANTUMI_FAR;
/* Lets `us` microseconds pass, the ticks in them delivered. */
uint8_t bantumi_play_elapse(uint16_t us) BANTUMI_FAR;
/* A key going down, with its code. */
uint8_t bantumi_play_key(uint8_t code) BANTUMI_FAR;

#endif
