/* Writes a reference frame for the platform builds and the MAME comparison.
   Usage: frame NAME OUT.pgm
   NAME: testcard, outline, snake-start, or menu-KEYS where KEYS is the menu
   keys pressed from the first screen (u up, d down, l left, r right,
   s select, e the console's Select button, b back, a the full-screen key, t one move of the running game,
   F forget what is drawn so the next draw is a full one, k flip the phase
   of Memory's blinking cursor, w wait for a timed page to close), and
   menu-KEYS+N the same N screen frames later. A name seedXXXXXXXX-menu-KEYS
   starts the games from that seed of rand, in hex. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "game_assets.h"
#include "lcd.h"
#include "menu.h"
#include "native_tiles.h"
#include "pgm.h"
#include "snake.h"
#include "sound.h"
#include "testcard.h"

/* The host keeps no settings: every frame starts from a fresh cartridge. */
void platform_settings_load(uint8_t game, struct game_settings *out)
{
    (void)game;
    out->top_score = 0;
    out->level = 0;
}

void platform_settings_save(uint8_t game, const struct game_settings *in)
{
    (void)game;
    (void)in;
}

void platform_tone(uint16_t hz)
{
    (void)hz;
}

int main(int argc, char **argv)
{
    unsigned long seed = 0;
    int seeded = 0;

    if (argc == 3 && strcmp(argv[1], "native") == 0)
        return native_gen(argv[2]);
    if (argc != 3) {
        fprintf(stderr, "usage: frame NAME OUT.pgm\n");
        return 2;
    }
    if (strncmp(argv[1], "seed", 4) == 0 && strlen(argv[1]) > 13 && argv[1][12] == '-') {
        seed = strtoul(argv[1] + 4, 0, 16);
        seeded = 1;
        argv[1] += 13;
    }
    if (strcmp(argv[1], "testcard") == 0) {
        testcard_draw();
    } else if (strcmp(argv[1], "outline") == 0) {
        testcard_frame();
    } else if (strcmp(argv[1], "snake-start") == 0) {
        lcd_clear();
        snake_init(0, game_speed_table[0], SNAKE_COLS, SNAKE_ROWS);
        snake_draw();
    } else if (strncmp(argv[1], "menu-", 5) == 0) {
        const char *key;
        char *plus = strchr(argv[1], '+');
        long frames = plus ? strtol(plus + 1, 0, 10) : 0;
        int wait;

        if (plus)
            *plus = 0;
        menu_init();
        if (seeded)
            menu_seed((uint32_t)seed);
        for (key = argv[1] + 5; *key; key++) {
            /* Draw after every key, as a platform does, so that drawing only
               a move's changes is exercised. */
            menu_draw();
            if (*key == 'F')
                menu_redraw_all();
            else if (*key == 'k')
                menu_blink();
            else if (*key == 'w')
                for (wait = 0; wait < 10 * MENU_TICKS_PER_SECOND && !menu_tick(); wait++)
                    ;
            else if (*key == 't')
                menu_game_step();
            else
                menu_key(*key == 'u' ? MENU_KEY_UP : *key == 'd' ? MENU_KEY_DOWN : *key == 'l' ? MENU_KEY_LEFT
                         : *key == 'r' ? MENU_KEY_RIGHT : *key == 's' ? MENU_KEY_SELECT
                         : *key == 'a' ? MENU_KEY_START : *key == 'e' ? MENU_KEY_ALT
                         : MENU_KEY_BACK);
        }
        menu_draw();
        while (frames-- > 0)
            if (menu_tick())
                menu_draw();
    } else {
        fprintf(stderr, "unknown frame %s\n", argv[1]);
        return 2;
    }
    if (pgm_write_lcd(argv[2]) != 0) {
        perror(argv[2]);
        return 1;
    }
    printf("wrote %s\n", argv[2]);
    return 0;
}
