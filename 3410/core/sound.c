#include "sound.h"

#include "game_assets.h"

volatile uint8_t sound_active;

static const uint8_t *volatile requested; /* a sound to start: its notes, or null */
static volatile uint8_t requested_bank;   /* the Game Boy bank they are in, 0 for none */
static const uint8_t *next;               /* the note after the one sounding, or null */
static uint8_t next_bank;                 /* its bank */
static uint8_t left;                      /* units the note sounding still has */

#ifdef __SDCC
/* A bank mapped from the timer interrupt is put back from far_mapped,
   which every bank switch keeps. */
extern uint8_t far_mapped;
#define BANK_MAP(b) (*(volatile uint8_t *)0x2000 = (b))
#else
#define far_mapped 0
#define BANK_MAP(b) ((void)(b))
#endif

/* Only leaves a request: sound_tick runs from an interrupt, and it alone
   touches the notes. */
void sound_play(uint8_t sound)
{
    uint8_t place;

    if (sound < GAME_SOUND_FIRST || sound >= GAME_SOUND_FIRST + sizeof game_sound_places)
        return;
    place = game_sound_places[sound - GAME_SOUND_FIRST];
    if (place == 0xff)
        return;
    requested_bank = 0;
    requested = game_sounds + place;
    sound_active = 1;
}

/* A run of notes in the same form kept anywhere else, and the Game Boy
   bank it is in (0 when none need be mapped to read them). */
void sound_play_notes(const uint8_t *notes, uint8_t bank)
{
    requested_bank = bank;
    requested = notes;
    sound_active = 1;
}

void sound_tick(void)
{
    const uint8_t *request = requested;
    uint8_t note, length;

    if (request) {
        requested = 0;
        next = request;
        next_bank = requested_bank;
    } else if (!next || --left) {
        return;
    }
    if (next_bank)
        BANK_MAP(next_bank);
    note = next[0];
    length = next[1];
    if (next_bank)
        BANK_MAP(far_mapped);
    if (note == SOUND_SILENCE) {
        next = 0;
        sound_active = 0;
        platform_tone(SOUND_SILENCE);
        return;
    }
    platform_tone(note == SOUND_REST ? SOUND_SILENCE : note);
    left = length;
    next += 2;
}
