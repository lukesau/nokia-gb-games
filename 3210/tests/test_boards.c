/* Host check that Memory and Rotation, drawing only what a key or a tick
   changed, give the same picture as drawing the whole board. Needs the
   extracted assets. */
#include <stdio.h>
#include <string.h>

#include "lcd.h"
#include "memory.h"
#include "rand.h"
#include "rotation.h"
#include "sound.h"

void platform_tone(uint8_t note)
{
    (void)note;
}

static uint8_t stepped[sizeof lcd_fb];
static unsigned seed = 1;

static unsigned pick(unsigned n)
{
    seed = seed * 1103515245u + 12345u;
    return (seed >> 16) % n;
}

static int same_as_full_draw(void (*draw)(void), const char *what, unsigned step)
{
    memcpy(stepped, lcd_fb, sizeof lcd_fb);
    lcd_clear();
    draw();
    if (memcmp(stepped, lcd_fb, sizeof lcd_fb) != 0) {
        printf("FAIL: %s step %u differs from a full redraw\n", what, step);
        return 0;
    }
    return 1;
}

static uint8_t blink;

static void memory_full(void)
{
    memory_draw(blink);
}

static int play_memory(uint8_t level)
{
    static const char keys[] = "24685555#*";
    unsigned step, games = 0;

    memory_init(level);
    lcd_clear();
    memory_draw(blink);
    for (step = 0; step < 20000; step++) {
        if (pick(4) == 0)
            blink ^= 1;
        if (memory_key(keys[pick(sizeof keys - 1)])) {
            games++;
            memory_init(level);
        }
        memory_draw_changes(blink);
        if (!same_as_full_draw(memory_full, "Memory", step))
            return 1;
    }
    if (!games) {
        printf("FAIL: no game of Memory finished at level %u\n", level);
        return 1;
    }
    printf("ok Memory level %u (%u games)\n", level, games);
    return 0;
}

static int play_rotation(uint8_t level)
{
    static const char keys[] = "13246879";
    unsigned step, turns = 0;

    rotation_init(level);
    rotation_resume();
    lcd_clear();
    rotation_draw();
    for (step = 0; step < 20000; step++) {
        if (rotation.over) {
            rotation_init(level);
            rotation_resume();
        } else if (pick(3) == 0) {
            turns += rotation_key(keys[pick(sizeof keys - 1)]) != 0;
        } else {
            rotation_tick();
        }
        rotation_draw_changes();
        if (!same_as_full_draw(rotation_draw, "Rotation", step))
            return 1;
    }
    if (turns < 100) {
        printf("FAIL: only %u turns; the test did not cover turning\n", turns);
        return 1;
    }
    printf("ok Rotation level %u (%u turns)\n", level, turns);
    return 0;
}

int main(void)
{
    uint8_t level;

    lcd_view_phone();
    for (level = 0; level < MEMORY_LEVELS; level++)
        if (play_memory(level))
            return 1;
    for (level = 0; level < ROTATION_LEVELS; level++)
        if (play_rotation(level))
            return 1;
    return 0;
}
