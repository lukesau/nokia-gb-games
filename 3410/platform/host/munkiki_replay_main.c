/* Runs Munkiki's Castles from a script, as the JAR's harness does, and
   writes the same state lines and, when asked, frames.

   Usage: munkiki_replay SCRIPT [OUT_DIR]

   SCRIPT is a text file of commands, one per line, in the harness's
   words: "tick [N]" lets N ticks of 100 ms pass, writing the state after
   each; "press K", "release K" and "key K" (both) send a key, K one of
   0-9, star, hash, up, down, left, right, fire (the Navi key) and soft2
   (the C key); "select N" chooses entry N of the list shown; "back" and
   "more" are the soft keys; "frame FILE" writes the screen as a PGM
   (into OUT_DIR when given); "# ..." is a comment. The state lines are
   written to stdout: "T n", then "K ...", "D ...", "C ...", "BZ ...",
   "BD ...", "BA ...", "CL ...", "CK ...", the JAR's field names, and
   "S ..." the sounds the tick played. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "games.h"
#include "lcd.h"
#include "munkiki.h"
#include "pgm.h"
#include "sound.h"

static char sounds_line[64];
struct game_options games_options = { 1, 1, 1 };

void platform_tone(uint8_t note)
{
    (void)note;
}

void platform_rumble(uint8_t on)
{
    (void)on;
}

/* The sound asked for, by index in the JAR, from where it starts. */
#include "munkiki_data.h"
void sound_play_notes(const uint8_t *notes, uint8_t bank)
{
    uint8_t i;

    (void)bank;
    for (i = 0; i < MUNKIKI_SOUND_COUNT; i++)
        if (notes == munkiki_sounds + munkiki_sound_at[i])
            snprintf(sounds_line + strlen(sounds_line), sizeof sounds_line - strlen(sounds_line), " %u", i);
}

void sound_play(uint8_t sound)
{
    (void)sound;
}

static uint8_t key_code(const char *name)
{
    if (strlen(name) == 1 && name[0] >= '0' && name[0] <= '9')
        return (uint8_t)(name[0] - '0');
    if (!strcmp(name, "star"))
        return MK_KEY_STAR;
    if (!strcmp(name, "hash"))
        return MK_KEY_HASH;
    if (!strcmp(name, "up"))
        return 2;
    if (!strcmp(name, "down"))
        return 8;
    if (!strcmp(name, "left"))
        return 4;
    if (!strcmp(name, "right"))
        return 6;
    if (!strcmp(name, "fire") || !strcmp(name, "soft1"))
        return MK_KEY_SELECT;
    if (!strcmp(name, "soft2"))
        return MK_KEY_BACK;
    fprintf(stderr, "unknown key %s\n", name);
    exit(2);
}

int main(int argc, char **argv)
{
    FILE *script;
    char line[256];
    unsigned tick = 0;
    static char state[20000];
    const char *out_dir = argc > 2 ? argv[2] : ".";

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: munkiki_replay SCRIPT [OUT_DIR]\n");
        return 2;
    }
    script = fopen(argv[1], "r");
    if (!script) {
        perror(argv[1]);
        return 1;
    }
    lcd_view_phone();
    munkiki_open();
    while (fgets(line, sizeof line, script)) {
        char word[32], arg[64];
        int n = 1;

        if (line[0] == '#' || sscanf(line, "%31s", word) != 1)
            continue;
        if (!strcmp(word, "tick")) {
            sscanf(line, "%*s %d", &n);
            while (n-- > 0) {
                munkiki_tick();
                munkiki_state_text(state, sizeof state);
                printf("T %u\n%sS%s\n", ++tick, state, sounds_line);
                sounds_line[0] = 0;
            }
        } else if (!strcmp(word, "press") || !strcmp(word, "release") || !strcmp(word, "key")) {
            uint8_t code;

            if (sscanf(line, "%*s %63s", arg) != 1)
                continue;
            code = key_code(arg);
            if (strcmp(word, "release"))
                munkiki_key(code, 1);
            if (strcmp(word, "press"))
                munkiki_key(code, 0);
        } else if (!strcmp(word, "select")) {
            sscanf(line, "%*s %d", &n);
            munkiki_select((uint8_t)n);
        } else if (!strcmp(word, "back")) {
            munkiki_key(MK_KEY_BACK, 1);
            munkiki_key(MK_KEY_BACK, 0);
        } else if (!strcmp(word, "more")) {
            munkiki_key(MK_KEY_SELECT, 1);
            munkiki_key(MK_KEY_SELECT, 0);
        } else if (!strcmp(word, "frame")) {
            char path[1024];

            if (sscanf(line, "%*s %63s", arg) != 1)
                continue;
            snprintf(path, sizeof path, "%s/%s", out_dir, arg);
            munkiki_draw();
            if (pgm_write_lcd(path) != 0) {
                perror(path);
                return 1;
            }
        }
    }
    fclose(script);
    return 0;
}
