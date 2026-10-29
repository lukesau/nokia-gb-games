#include "menu_header.h"

#include "font.h"
#include "game_assets.h"
#include "lcd.h"

/* A page's header: the title in the tiny font in the middle of a line
   across, and the entry's number on the right. The phone centres the
   title in the 93 columns left of the number, its width counting the
   space after its last letter. */
#define HEADER_CENTRE_WIDTH 93

void menu_header_draw(const char *title, uint8_t number)
{
    char digits[3];
    uint8_t n = 0, width = (uint8_t)font_text_width(&font_tiny_plain, title);
    uint8_t x = (uint8_t)((HEADER_CENTRE_WIDTH - width) / 2), number_x;

    if (number >= 10)
        digits[n++] = (char)('0' + number / 10);
    digits[n++] = (char)('0' + number % 10);
    digits[n] = 0;
    number_x = (uint8_t)(LCD_WIDTH - font_text_width(&font_tiny_plain, digits));
    lcd_fill_rect(0, MENU_HEADER_LINE_Y, x - 2, 1, 1);
    font_draw(&font_tiny_plain, x, 0, title, 1);
    lcd_fill_rect(x + width, MENU_HEADER_LINE_Y, number_x - 2 - x - width, 1, 1);
    font_draw(&font_tiny_plain, number_x, 0, digits, 1);
}

#ifdef MUNKIKI_BANKED
/* Munkiki's Castles' page titles are in the tiny font here
   (munkiki_int.h). */
#include <string.h>

#include "munkiki_int.h"

uint8_t mk_glyph_tiny(char c, uint16_t *columns) MK_FAR
{
    uint8_t code = (uint8_t)c, i, n;

    if (code < FONT_FIRST_CHAR || code > FONT_LAST_CHAR)
        code = '?';
    i = (uint8_t)(code - FONT_FIRST_CHAR);
    n = (uint8_t)(font_tiny_plain.offsets[i + 1] - font_tiny_plain.offsets[i]);
    if (columns)
        memcpy(columns, font_tiny_plain.columns + font_tiny_plain.offsets[i], n * sizeof *columns);
    return n;
}
#endif
