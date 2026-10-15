#include "far.h"

#include "snake2.h"
#include "title.h"
#include "native_gb.h"
#include "menu_header.h"

#define MBC_ROM_BANK (*(volatile uint8_t *)0x2000)

/* The bank mapped now (bcall.s keeps it too). */
uint8_t far_mapped = BANK_MENU;

void far_bank(uint8_t bank)
{
    far_mapped = bank;
    MBC_ROM_BANK = bank;
}

int far_snake2_handler(int event, struct game_context *ctx)
{
    uint8_t was = far_mapped;
    int result;

    far_bank(BANK_SNAKE);
    result = snake2_handler(event, ctx);
    far_bank(was);
    return result;
}

/* The title and the game-over picture are drawn from Snake II's data, in
   its bank. */
void far_title_start(void)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    title_start();
    far_bank(was);
}

uint8_t far_title_elapse(uint16_t us)
{
    uint8_t was = far_mapped, what;

    far_bank(BANK_SNAKE);
    what = title_elapse(us);
    far_bank(was);
    return what;
}

void far_title_draw(void)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    title_draw();
    far_bank(was);
}

void far_over_start(uint16_t score, uint8_t blink)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    over_start(score, blink);
    far_bank(was);
}

uint8_t far_over_elapse(uint16_t us)
{
    uint8_t was = far_mapped, what;

    far_bank(BANK_SNAKE);
    what = over_elapse(us);
    far_bank(was);
    return what;
}

void far_over_draw(void)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    over_draw();
    far_bank(was);
}

void far_snake2_redraw(void)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    snake2_redraw();
    far_bank(was);
}

void far_scores_start(uint16_t top, uint16_t last, uint8_t show_last, uint8_t kind)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    scores_start(top, last, show_last, kind);
    far_bank(was);
}

uint8_t far_scores_elapse(uint16_t us)
{
    uint8_t was = far_mapped, what;

    far_bank(BANK_SNAKE);
    what = scores_elapse(us);
    far_bank(was);
    return what;
}

void far_scores_draw(void)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    scores_draw();
    far_bank(was);
}

void far_draw_score_box(uint16_t value, uint8_t y, uint8_t medals)
{
    uint8_t was = far_mapped;

    far_bank(BANK_SNAKE);
    draw_score_box(value, y, medals);
    far_bank(was);
}

/* The full-screen menus made at build time: native_tiles.c and the
   screens in one bank, their tiles in the next. */
const uint8_t native_bank = BANK_NATIVE, native_tile_bank = BANK_NATIVE_TILES;

uint8_t far_native_show(uint16_t id)
{
    uint8_t was = far_mapped, result;

    far_bank(BANK_NATIVE);
    result = native_show(id);
    far_bank(was);
    return result;
}

void far_native_cursor(uint8_t row, uint8_t on)
{
    uint8_t was = far_mapped;

    far_bank(BANK_NATIVE);
    native_cursor(row, on);
    far_bank(was);
}

uint8_t far_native_leave(void)
{
    uint8_t was = far_mapped, result;

    far_bank(BANK_NATIVE);
    result = native_leave();
    far_bank(was);
    return result;
}

/* The palette, and on a Game Boy Color or Advance double speed at the
   first call (native_gb.h). */
void far_native_palette(uint8_t shades)
{
    uint8_t was = far_mapped;

    far_bank(BANK_NATIVE);
    native_palette(shades);
    far_bank(was);
}

static void far_text_copy_near(char *to, const char *from, uint8_t size)
{
    for (; size > 1 && *from; size--)
        *to++ = *from++;
    *to = 0;
}

/* The Instructions' texts are with the full-screen menus. */
void far_text_copy(char *to, const char *from, uint8_t size)
{
    uint8_t was = far_mapped;

    far_bank(BANK_NATIVE);
    far_text_copy_near(to, from, size);
    far_bank(was);
}

/* The menu pages' header and its font are there too: the title, in the
   menus' bank, is copied out first. */
void far_menu_header(const char *title, uint8_t number)
{
    char text[MENU_HEADER_MAX + 1];
    uint8_t was = far_mapped;

    far_text_copy_near(text, title, sizeof text);
    far_bank(BANK_NATIVE);
    menu_header_draw(text, number);
    far_bank(was);
}
