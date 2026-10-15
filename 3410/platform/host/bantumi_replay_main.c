/* Replays a recorded Bantumi game and writes the frames it produces, for
   comparison with frames captured from the firmware in MAME.

   Usage: bantumi_replay EVENTS OUT_DIR LEVEL [BEANS]

   EVENTS holds the handler's calls, one "EVENT A B" in hex per line
   (tools/bantumi_events.py). A frame is written each time the picture
   changes, as OUT_DIR/NNNNNN.pgm,
   numbered by the event that produced it. LEVEL is the level the game was
   played at, BEANS the beans a pit starts with (4). The sounds each event
   asks for go to OUT_DIR/sounds.txt, as bantumi_events.py writes the
   phone's. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bantumi.h"
#include "lcd.h"
#include "pgm.h"
#include "sprite.h"

void platform_tone(uint8_t note)
{
    (void)note;
}

void platform_rumble(uint8_t on)
{
    (void)on;
}

int main(int argc, char **argv)
{
    static uint8_t last[sizeof sprite_screen];
    char path[1024];
    unsigned event, a, b, n = 0, written = 0;
    FILE *events, *sounds;

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "usage: bantumi_replay EVENTS OUT_DIR LEVEL [BEANS]\n");
        return 2;
    }
    events = fopen(argv[1], "r");
    if (!events) {
        perror(argv[1]);
        return 1;
    }
    snprintf(path, sizeof path, "%s/sounds.txt", argv[2]);
    sounds = fopen(path, "w");
    if (!sounds) {
        perror(path);
        return 1;
    }
    bantumi_level = (uint8_t)atoi(argv[3]);
    bantumi_beans = (uint8_t)(argc == 5 ? atoi(argv[4]) : 4);
    lcd_view_phone();
    memset(last, 0xaa, sizeof last);
    for (; fscanf(events, "%x %x %x", &event, &a, &b) == 3; n++) {
        long at = ftell(events);
        unsigned next = 0xff;
        uint8_t done;

        /* Menu leaves the game for the phone's menu; Continue is New game
           then the game put away at once: the phone shows nothing of
           either. */
        if (fscanf(events, "%x", &next) != 1)
            next = 0xff;
        fseek(events, at, SEEK_SET);
        done = bantumi_event((uint8_t)event, (uint8_t)a);
        if (bantumi_sound)
            fprintf(sounds, "%u snd %x\n", n, bantumi_sound);
        bantumi_sound = 0;
        /* Nor does it draw the game once the game has asked to close, or a
           demo before its first tick: the text before it stays up. */
        if (event == BANTUMI_EVENT_PAUSE || (event == BANTUMI_EVENT_NEW_GAME && next == BANTUMI_EVENT_CONTINUE)
            || (done & BANTUMI_DONE_CLOSE) || event == BANTUMI_EVENT_DEMO)
            continue;
        bantumi_render(1);
        if (memcmp(last, sprite_screen, sizeof last) == 0)
            continue;
        memcpy(last, sprite_screen, sizeof last);
        sprite_present(1);
        snprintf(path, sizeof path, "%s/%06u.pgm", argv[2], n);
        if (pgm_write_lcd(path) != 0) {
            perror(path);
            return 1;
        }
        written++;
    }
    fclose(events);
    fclose(sounds);
    printf("%u events, %u frames in %s\n", n, written, argv[2]);
    return 0;
}
