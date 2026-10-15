/* The layer between a platform and the games: it turns keys and elapsed
   time into the game's events and acts on what the game's handler returns,
   as the phone's games application does. */
#ifndef CORE_GAMES_H
#define CORE_GAMES_H

#include <stdint.h>

#include "game.h"

/* The phone's timers count in units of 249/32 ms. */
#define GAMES_UNIT_US 7781
/* The games' options (Sounds, Lights, Shakes), which a platform saves. */
#define GAMES_OPTIONS 1

/* The settings the games follow: a sound is only played and the vibrator
   only run when its switch is on. menu.c loads and keeps them. */
extern struct game_options games_options;

/* The game being played or last played, a GAME_ code. */
extern uint8_t games_playing;

/* Whether the picture last put on the screen has Space Impact's strip of
   terrain, which a platform may show by other means (platform/gb/strip.c). */
extern uint8_t games_strip;

/* Nonzero once the game has ended; games_score is then its score. */
extern uint8_t games_over;
extern uint32_t games_score;

/* Starts a new game: a GAME_ code, the phone's level setting plus one
   and its option setting plus one (Snake II's maze). */
void games_start(uint8_t game, uint8_t level, uint8_t option);

/* The game goes on after a pause. */
void games_continue(void);

/* A key went down (an GAME_KEY_ code) or the held key came up. One key is
   held at a time, as on the phone's keypad. Returns nonzero when the
   screen needs drawing. */
uint8_t games_key_down(uint8_t key);
void games_key_up(void);

/* Lets this many timer units pass. Returns nonzero when the screen needs
   drawing. */
uint8_t games_advance(uint16_t units);

/* Lets this much time pass, in microseconds, at most 50000; a platform
   calls it once per screen frame. Returns nonzero when the screen needs
   drawing. */
uint8_t games_elapse(uint16_t us);

/* Picks the game, level and option games_event delivers to, as
   games_start does, without starting it. */
void games_setup(uint8_t game, uint8_t level, uint8_t option);

/* Delivers one event straight to the game, for replaying a recorded
   sequence. Returns nonzero when the screen needs drawing. */
uint8_t games_event(int event);

/* Brings sprite_screen up to date with the game. */
void games_render(void);

/* Draws the game into the LCD view; with `all`, not just what changed. */
void games_draw(uint8_t all);

/* The phone's vibrator, as a game asks for it: a pulse of GAMES_VIBRATE_UNITS
   timer units, started afresh by every call, when Shakes is on. */
#define GAMES_VIBRATE_UNITS 62
void games_vibrate(void);

/* Lets this much time pass for the vibrator, in microseconds; a platform
   or the menus call it once per screen frame whether or not a game is
   being played, since the phone's vibration is a system timer that runs
   on through a pause. */
void games_rumble_elapse(uint16_t us);

/* Stops the vibrator at once: for starting over. */
void games_quiet(void);

/* The platform's vibrator or rumble motor, on or off. */
void platform_rumble(uint8_t on);

/* Called as a game starts, a GAME_ code: a platform that keeps one game's
   data where another's state goes, as the Game Boy keeps Space Impact's
   where Snake II's goes, makes it whole again here. */
void platform_game_starts(uint8_t game);

#endif
