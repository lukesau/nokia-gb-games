/* Munkiki's Castles: its screens drawn, for the flow in munkiki_game.c:
   the title, the lists the phone's MIDP draws for the game (here in the
   phone's list style, measured in MAME), the text pages before a level,
   the HUD over the game, the bonus count, the map of the castles, game
   over, the High scores page, the Instructions and the Options.

   Text is drawn with the phone's fonts, as the KVM draws it: its SMALL,
   MEDIUM and LARGE are the phone's small plain, medium bold and large
   bold fonts, a glyph a row under the line's top; the Game Boy reaches
   the fonts, which are in other banks, a glyph at a time (mk_glyph). */
#define MK_BLIT_PAGES
#include "munkiki_blit.h"

#include "font.h"
#include "game_assets.h"
#include "lcd.h"
#include "munkiki_flow.h"

/* The fonts, and the heights the KVM reports for them. */
enum {
    FONT_SMALL,
    FONT_MEDIUM,
    FONT_LARGE,
    FONT_TINY,
    FONT_SMALL_BOLD
};
static const uint8_t font_height[5] = { 11, 13, 15, 8, 9 };
/* The rows the glyphs take. */
static const uint8_t font_rows[5] = { 9, 11, 13, 8, 9 };
/* What the phone's soft keys say for the game's lists and pages. */
static const char mk_text_select[] = "Select", mk_text_back[] = "Back", mk_text_more[] = "More";


/* Text. */
#ifdef MUNKIKI_BANKED
uint8_t mk_glyph(uint8_t which, char c, uint16_t *columns)
{
    return which == FONT_TINY ? mk_glyph_tiny(c, columns) : mk_glyph_menu(which, c, columns);
}
#else
uint8_t mk_glyph(uint8_t which, char c, uint16_t *columns)
{
    const struct font *f = which == FONT_SMALL ? &font_small_plain : which == FONT_MEDIUM ? &font_medium_bold
                           : which == FONT_LARGE ? &font_large_bold : which == FONT_TINY ? &font_tiny_plain
                           : &font_small_bold;
    uint8_t code = (uint8_t)c, i, n;

    if (code < FONT_FIRST_CHAR || code > FONT_LAST_CHAR)
        code = '?';
    i = (uint8_t)(code - FONT_FIRST_CHAR);
    n = (uint8_t)(f->offsets[i + 1] - f->offsets[i]);
    if (columns)
        memcpy(columns, f->columns + f->offsets[i], n * sizeof *columns);
    return n;
}
#endif

static uint8_t text_width(uint8_t font, const char *text, uint8_t len)
{
    uint8_t w = 0;

    while (len--)
        w += mk_glyph(font, *text++, 0);
    return w;
}

/* Draws `len` characters with their top-left corner at (x, y), black on
   white or, inverted, white on black over `rows` rows, clipped to the
   frame. */
static void text_draw_rows(uint8_t font, int16_t x, int16_t y, const char *text, uint8_t len, uint8_t inverted,
                           uint8_t rows)
{
    uint16_t columns[16];
    uint8_t n, i, row, bit;

    while (len--) {
        n = mk_glyph(font, *text++, columns);
        for (i = 0; i < n; i++, x++) {
            if (x < 0 || x >= MK_W)
                continue;
            bit = (uint8_t)(0x80 >> (x & 7));
            for (row = 0; row < rows; row++) {
                int16_t py = (int16_t)(y + row);
                uint8_t *p;

                if (py < 0 || py >= MK_H)
                    continue;
                p = mk_frame + py * MK_STRIDE + (x >> 3);
                if (((columns[i] >> row) & 1) != inverted)
                    *p |= bit;
                else
                    *p &= (uint8_t)~bit;
            }
        }
    }
}

static void text_draw(uint8_t font, int16_t x, int16_t y, const char *text, uint8_t len, uint8_t inverted)
{
    text_draw_rows(font, x, y, text, len, inverted, font_rows[font]);
}

static void text_draw_z(uint8_t font, int16_t x, int16_t y, const char *text, uint8_t inverted)
{
    text_draw(font, x, y, text, (uint8_t)strlen(text), inverted);
}

/* b.a: the text with %N replaced by a number. */
static char made[MUNKIKI_TEXT_MAX + 8];

static const char *text_with(uint8_t which, uint16_t n)
{
    const char *t = munkiki_texts[which];
    char *o = made;

    for (; *t; t++) {
        if (t[0] == '%' && t[1] == 'N') {
            char digits[6];
            uint8_t i = sizeof digits;

            digits[--i] = 0;
            do {
                digits[--i] = (char)('0' + n % 10);
                n /= 10;
            } while (n);
            strcpy(o, digits + i);
            o += strlen(o);
            t++;
        } else {
            *o++ = *t;
        }
    }
    *o = 0;
    return made;
}

/* The hint, in parentheses, as the JAR shows it. */
static const char *hint_text(void)
{
    size_t n;

    text_with(TEXT_HINT, 0);
    n = strlen(made);
    memmove(made + 1, made, n + 1);
    made[0] = '(';
    made[n + 1] = ')';
    made[n + 2] = 0;
    return made;
}

/* b.b: the text in lines of at most 94 pixels, words joined while the
   line with a space and the word is no wider: the next line's start and
   length. */
static const char *wrap_next(uint8_t font, const char *text, uint8_t *len)
{
    const char *p = text, *end = text;
    uint8_t width = 0, space = mk_glyph(font, ' ', 0);

    for (;;) {
        const char *word = p;
        uint8_t w = 0;

        while (*p && *p != ' ')
            w += mk_glyph(font, *p++, 0);
        if (end != text && width + w + space > 94)
            break;
        if (end != text)
            width += space;
        width += w;
        end = p;
        if (!*p)
            break;
        p++;
        (void)word;
    }
    *len = (uint8_t)(end - text);
    return end;
}

static uint8_t wrap_count(uint8_t font, const char *text)
{
    uint8_t n = 0, len;

    while (*text) {
        text = wrap_next(font, text, &len);
        n++;
        if (*text == ' ')
            text++;
    }
    return n;
}

/* The block of lines, as b.b makes it, with its top at y. */
static void wrapped_draw(uint8_t font, int16_t y, const char *text)
{
    uint8_t len, pitch = (uint8_t)(font_height[font] + 1);

    while (*text) {
        const char *next = wrap_next(font, text, &len);

        text_draw(font, (96 - text_width(font, text, len)) >> 1, (int16_t)(y + 1), text, len, 0);
        y += pitch;
        text = *next == ' ' ? next + 1 : next;
    }
}

static uint8_t wrapped_height(uint8_t font, const char *text)
{
    return (uint8_t)((font_height[font] + 1) * wrap_count(font, text) + 1);
}

/* KKM.k: a number in the large font, its digits ten apart ending at x,
   all five with `all`. */
static void number_draw(uint16_t n, int16_t x, int16_t y, uint8_t all)
{
    uint8_t i;
    char digit[2];

    if (n == 0 && !all)
        return;
    digit[1] = 0;
    for (i = 0; i < 5; i++) {
        digit[0] = (char)('0' + n % 10);
        text_draw(FONT_LARGE, (int16_t)(x - i * 10), (int16_t)(y + 1), digit, 1, 0);
        n /= 10;
        if (n == 0 && !all)
            return;
    }
}

/* The phone's list page: the title in the tiny font between lines, the
   number of the entry on the right, rows of eleven from y 10 in the
   medium font, the chosen one on a black bar, a scrollbar, and the soft
   keys. */
static void list_header(const char *title, uint8_t number)
{
    uint8_t w = text_width(FONT_TINY, title, (uint8_t)strlen(title)), x = (uint8_t)((89 - w) / 2);
    char digit[3];

    text_draw_z(FONT_TINY, x, 0, title, 0);
    frame_fill(0, 4, (uint8_t)(x > 2 ? x - 2 : 0), 1, 1);
    frame_fill((int8_t)(x + w + 2), 4, (uint8_t)(x + w + 2 < 89 ? 89 - (x + w + 2) : 0), 1, 1);
    if (number >= 10) {
        digit[0] = (char)('0' + number / 10);
        digit[1] = (char)('0' + number % 10);
        digit[2] = 0;
    } else {
        digit[0] = (char)('0' + number);
        digit[1] = 0;
    }
    text_draw_z(FONT_TINY, (int16_t)(95 - text_width(FONT_TINY, digit, (uint8_t)strlen(digit))), 1, digit, 0);
}

static void softkeys(const char *left, const char *right)
{
    if (left)
        text_draw_z(FONT_SMALL_BOLD, 1, 55, left, 0);
    if (right)
        text_draw_z(FONT_SMALL_BOLD, (int16_t)(95 - text_width(FONT_SMALL_BOLD, right, (uint8_t)strlen(right))), 55,
                    right, 0);
}

static void list_rows(const char *(*name)(uint8_t), uint8_t count, uint8_t cursor, uint8_t top)
{
    uint8_t i, height, thumb;

    for (i = 0; i < 4 && top + i < count; i++) {
        uint8_t y = (uint8_t)(10 + i * 11), on = (uint8_t)(top + i == cursor);

        if (on)
            frame_fill(0, (int8_t)y, 92, 11, 1);
        text_draw_rows(FONT_MEDIUM, 2, (int16_t)(y + 1), name((uint8_t)(top + i)),
                       (uint8_t)strlen(name((uint8_t)(top + i))), on, 10);
    }
    frame_fill(94, 10, 1, 44, 1);
    height = (uint8_t)(42 / count);
    thumb = (uint8_t)(11 + cursor * height);
    frame_fill(93, (int8_t)thumb, 1, height, 1);
    frame_fill(95, (int8_t)thumb, 1, height, 1);
}

/* The lists' entries. */
static const char *main_name(uint8_t i)
{
    if (mkf.can_continue) {
        if (i == 0)
            return munkiki_texts[TEXT_CONTINUE];
        i--;
    }
    return munkiki_texts[i == 0 ? TEXT_NEW_GAME : i == 1 ? TEXT_HIGH_SCORES : i == 2 ? TEXT_INSTRUCTIONS : TEXT_OPTIONS];
}

static const char *pause_name(uint8_t i)
{
    return munkiki_texts[i == 0 ? TEXT_RETURN : i == 1 ? (mk.lives == 0 ? TEXT_END_GAME : TEXT_RESTART) : TEXT_GAME_MENU];
}

static const char *level_name(uint8_t i)
{
    return text_with(TEXT_LEVEL_N, i == 0 ? 1 : mkf.level_reached);
}

static const char *option_name(uint8_t i)
{
    return munkiki_texts[i == 0 ? TEXT_GAME_SOUNDS : TEXT_LIGHTS];
}

/* The level map's picture (KKM.q case 5): the castles' towers, a mark
   on each level done, and the keys found. */
static void map_draw(void)
{
    uint8_t n3 = (uint8_t)(25 + mk.keys), n4 = n3 > 25 ? 3 : 2, n5;
    int16_t n, n2;

    frame_line(3, 51, 6, 54);
    frame_line(7, 54, 10, 51);
    /* The towers' lower arcs, clipped to the rows from 42; then the
       upper, clipped to the first nine rows, and to 65 columns when the
       third tower is a key's room. The frame is empty but for them. */
    for (n5 = 0; n5 < 2; n5++) {
        frame_arc((int16_t)(16 + 26 * n5), 29, 21, 21, 1);
        frame_arc((int16_t)(23 + 26 * n5), 38, 7, 7, 1);
    }
    memset(mk_frame, 0, 42 * MK_STRIDE);
    for (n5 = 0; n5 < n4; n5++) {
        frame_arc((int16_t)(3 + 26 * n5), 1, 21, 21, 0);
        frame_arc((int16_t)(10 + 26 * n5), 6, 7, 7, 0);
    }
    for (n = 9; n < 42; n++)
        memset(mk_frame + n * MK_STRIDE, 0, MK_STRIDE);
    if (n3 == 26) {
        uint8_t y;

        for (y = 0; y < 9; y++)
            for (n = 65; n < 96; n++)
                mk_frame[y * MK_STRIDE + (n >> 3)] &= (uint8_t)~(0x80 >> (n & 7));
    }
    for (n5 = 0; n5 < 5; n5++) {
        n4 = n5 == 0 ? 51 : 41;
        frame_line((int16_t)(3 + n5 * 13), 9, (int16_t)(3 + n5 * 13), n4);
        frame_line((int16_t)(10 + n5 * 13), 9, (int16_t)(10 + n5 * 13), n4);
    }
    if (n3 == 25) {
        frame_line(55, 9, 55, 1);
        frame_line(62, 9, 62, 1);
        frame_line(55, 1, 62, 1);
    } else if (n3 == 26) {
        frame_line(64, 1, 75, 1);
        frame_line(64, 6, 75, 6);
        frame_line(75, 1, 75, 6);
    } else {
        n2 = (int16_t)(n3 * 10 - 251);
        frame_line(68, 9, 68, n2);
        frame_line(75, 9, 75, n2);
        frame_line(68, n2, 75, n2);
    }
    for (n = mk.level - 1; n >= 0; n--) {
        uint8_t n6 = (uint8_t)(n % 5);

        n2 = (int16_t)(n / 5 * 13 + 2);
        {
            int16_t yy = (int16_t)(42 - n6 * 10);

            if (n % 10 > 4)
                yy = (int16_t)(44 - yy);
            if (n6 == 4)
                n2++;
            else if (n6 == 0 && n != 0)
                n2--;
            /* drawImage(map, n2, yy - 53) clipped to (n2, yy, 11, 9): the
               map's first 11 columns of its rows from 53. */
            frame_blit_part(n2, yy, 11, 9, munkiki_map, 8, 0, 53);
        }
    }
    for (n5 = 0; n5 < mk.keys; n5++) {
        n = (int16_t)(1 + n5 * 10);
        /* drawImage(map, 80 - (level < 26 + n5 ? 0 : 13), n - 13) clipped to (80, n, 14, 10). */
        frame_blit_part(80, n, 14, 10, munkiki_map, 8, (uint8_t)(mk.level < 26 + n5 ? 0 : 13), 13);
    }
}

/* The 16 by 14 at the map's top left: Munkiki's mark. */
static void mark_draw(int16_t x, int16_t y)
{
    frame_blit_part(x, y, 16, 14, munkiki_map, 8, 0, 0);
}

/* Where level n's mark is on the map. */
static void level_spot(uint8_t n, int16_t *x, int16_t *y)
{
    uint8_t n2 = (uint8_t)(n % 5);

    *x = (int16_t)(n / 5 * 13);
    *y = (int16_t)(39 - n2 * 10);
    if (n % 10 > 4) {
        *y = (int16_t)(39 - *y);
        (*x)--;
    }
}

/* d.av: a count in the HUD's small digits, 5 wide and 4 apart in the
   digits picture from column 24, the last ending at x. */
static void hud_digits(int16_t x, int16_t y, uint16_t value)
{
    uint8_t n = 0;

    do {
        int16_t at = (int16_t)(x - n++ * 5);

        frame_blit_part(at, y, 5, 7, munkiki_digits, 8, (uint8_t)(24 + value % 10 * 4), 0);
    } while ((value /= 10) > 0);
}

/* The frame onto the LCD view, marking what changed. */
static void frame_present(void)
{
    uint8_t y, x;
    uint8_t *dst;
    const uint8_t *src = mk_frame;

    for (y = 0; y < MK_H; y++) {
        dst = lcd_fb + (lcd_view_y + y) * LCD_STRIDE + (lcd_view_x >> 3);
        for (x = 0; x < MK_STRIDE; x++) {
            if (dst[x] != src[x]) {
                dst[x] = src[x];
                lcd_mark_dirty((uint8_t)(lcd_view_x + x * 8), (uint8_t)(lcd_view_y + y), 8, 1);
            }
        }
        src += MK_STRIDE;
    }
}

static void hud_draw(void)
{
    if (mk.notes_left != 0 && mk.ended == 0) {
        frame_blit_part(0, 0, 25, 14, munkiki_digits, 8, 0, 0);
        hud_digits(mk.notes_left < 10 ? 11 : 16, 3, mk.notes_left);
    }
    if (mk.demo) {
        frame_blit(73, 2, 21, 23, munkiki_png_kb, 0, 0);
        if (mk.demo_key != -1) {
            uint8_t n = (uint8_t)mk.demo_key;

            if (n == 11)
                n = 12;
            if (n == 0)
                n = 11;
            n--;
            frame_blit((int16_t)(75 + 6 * (n % 3)), (int16_t)(4 + 5 * (n / 3)), 5, 4, munkiki_png_ky, 0, 0);
        }
    }
}

static void game_draw(void)
{
    switch (mkf.part) {
    case PART_HINT:
        frame_clear();
        wrapped_draw(FONT_SMALL, (int16_t)-mkf.counter, hint_text());
        break;
    case PART_LEVEL: {
        const char *body = text_with(mk.level % 5 == 0 && mk.level < 25 ? TEXT_COLLECT_N_OR_KEY : TEXT_COLLECT_N,
                                     mk.notes_left);

        frame_clear();
        wrapped_draw(FONT_SMALL, (int16_t)(14 - mkf.counter), body);
        frame_fill(0, 0, 96, 13, 0);
        wrapped_draw(FONT_MEDIUM, 0, text_with(TEXT_LEVEL_N, mk.level));
        frame_line(0, 12, 95, 12);
        break;
    }
    case PART_PLAY:
    case PART_DONE:
        /* Made at the tick. */
        break;
    case PART_BONUS: {
        int16_t n;

        frame_clear();
        wrapped_draw(FONT_MEDIUM, 0, text_with(TEXT_LEVEL_N, mk.level));
        frame_line(0, 12, 95, 12);
        frame_rect(20, 48, 53, 16);
        number_draw(mk.score, 63, 50, 1);
        if (mkf.bonus_time > 0) {
            n = mkf.bonus_time;
            frame_blit_part(3, 20, 28, 22, munkiki_map, 8, 26, 18);
        } else if (mkf.bonus_key > 0) {
            n = mkf.bonus_key;
            frame_blit_part(16, 23, 15, 16, munkiki_map, 8, 50, 49);
        } else if (mkf.bonus_warp > 0) {
            n = mkf.bonus_warp;
            frame_blit_part(9, 20, 22, 22, munkiki_map, 8, 26, 40);
        } else {
            n = mkf.bonus_last;
            if (n != 0)
                frame_blit_part(5, 22, 26, 18, munkiki_map, 8, 26, 0);
        }
        if (n > 0)
            number_draw((uint16_t)n, 63, 24, 0);
        break;
    }
    case PART_MAP: {
        int16_t x, y, px, py;

        frame_clear();
        level_spot(mk.level, &x, &y);
        if (mkf.counter < 19) {
            int16_t n5 = mkf.counter > 8 ? mkf.counter - 8 : 0;

            map_draw();
            level_spot((uint8_t)(mk.level - 1), &px, &py);
            mark_draw((int16_t)((x - px) * n5 / 10 + px), (int16_t)((y - py) * n5 / 10 + py));
        } else if (mkf.counter < 39 && mk.level > 24) {
            int16_t n10, nn;

            map_draw();
            if (mkf.counter < 35)
                mark_draw(x, y);
            if (mkf.counter < 26) {
                /* clip (counter + 51, y + 4, 34 - counter, 7) of the map at (counter + 37, y + 1) */
                frame_blit_part((int16_t)(mkf.counter + 51), (int16_t)(y + 4), (uint8_t)(34 - mkf.counter), 7, munkiki_map, 8,
                                   14, 3);
                break;
            }
            n10 = mkf.counter < 35 ? (mkf.counter - 1) % 3 : 0;
            if (mkf.counter > 34) {
                nn = (int16_t)(mkf.counter - 35);
                /* clip (80, y + 1, 14, 5) of the map drawn at (80 - (nn == 3 ? 13 : 0), y - 12 - (nn + 1 & 3) * 10) */
                frame_blit_part(80, (int16_t)(y + 1), 14, 5, munkiki_map, 8, (uint8_t)(nn == 3 ? 13 : 0),
                                   (uint8_t)(13 + ((nn + 1) & 3) * 10));
            }
            /* clip (78, y + 4, 7, 7) of the map at (65, y - 22 - n10 * 10) */
            frame_blit_part(78, (int16_t)(y + 4), 7, 7, munkiki_map, 8, 13, (uint8_t)(26 + n10 * 10));
        } else {
            map_draw();
            mark_draw(x, y);
        }
        break;
    }
    case PART_OVER:
        frame_clear();
        if (mkf.counter < 20) {
            frame_blit(0, (int16_t)(5 - (mkf.counter < 10 ? (10 - mkf.counter) * 6 : 0)), 96, 54, munkiki_over, 0, 0);
        } else {
            char text[8];
            uint8_t n = 0;
            int16_t v = mkf.percent;

            frame_rect(20, 10, 53, 16);
            number_draw(mk.score, 63, 12, 1);
            if (v >= 100)
                text[n++] = (char)('0' + v / 100);
            if (v >= 10)
                text[n++] = (char)('0' + v / 10 % 10);
            text[n++] = (char)('0' + v % 10);
            text[n++] = '%';
            text[n] = 0;
            text_draw(FONT_LARGE, (96 - text_width(FONT_LARGE, text, n)) >> 1, 41, text, n, 0);
        }
        break;
    default:
        break;
    }
}

static void help_draw(void)
{
    const char *text = munkiki_texts[mk_help_order[mkf.help_page - 1]];
    uint8_t len, row = 0;

    frame_clear();
    list_header(munkiki_texts[TEXT_INSTRUCTIONS], mkf.help_page);
    while (*text) {
        const char *next = wrap_next(FONT_SMALL, text, &len);

        if (row >= mkf.text_scroll && row - mkf.text_scroll < 4)
            text_draw(FONT_SMALL, 1, (int16_t)(10 + (row - mkf.text_scroll) * 11 + 1), text, len, 0);
        row++;
        text = *next == ' ' ? next + 1 : next;
    }
    softkeys(mk_text_more, mk_text_back);
}

static void options_draw(void)
{
    uint8_t i;

    frame_clear();
    list_header(munkiki_texts[TEXT_OPTIONS], (uint8_t)(mkf.option_row + 1));
    for (i = 0; i < 2; i++) {
        uint8_t y = (uint8_t)(10 + i * 22), on = (uint8_t)(i == mkf.option_row);
        const char *value = munkiki_texts[(i == 0 ? mkf.sounds_on : mkf.lights_on) ? TEXT_ON : TEXT_OFF];

        text_draw_z(FONT_SMALL, 2, (int16_t)(y + 1), option_name(i), 0);
        if (on)
            frame_fill(0, (int8_t)(y + 11), 92, 11, 1);
        text_draw_rows(FONT_MEDIUM, 2, (int16_t)(y + 12), value, (uint8_t)strlen(value), on, 10);
    }
    softkeys(mk_text_select, mk_text_back);
}

void munkiki_draw(void) MK_FAR
{
    if (mkf.list == LIST_MAIN || mkf.list == LIST_PAUSE || mkf.list == LIST_LEVEL) {
        frame_clear();
        if (mkf.list == LIST_MAIN)
            list_header("munkiki's castles", (uint8_t)(mkf.selection + 1));
        else if (mkf.list == LIST_PAUSE)
            list_header(text_with(TEXT_LIVES_LEFT, (uint16_t)mk.lives), (uint8_t)(mkf.selection + 1));
        else
            list_header(munkiki_texts[TEXT_NEW_GAME], (uint8_t)(mkf.selection + 1));
        list_rows(mkf.list == LIST_MAIN ? main_name : mkf.list == LIST_PAUSE ? pause_name : level_name, mkf.list_count, mkf.selection,
                  mkf.list_top);
        softkeys(mk_text_select, mk_text_back);
    } else if (mkf.list == LIST_OPTIONS) {
        options_draw();
    } else if (mkf.screen == SCREEN_HELP) {
        help_draw();
    } else if (mkf.screen == SCREEN_TITLE) {
        frame_blit(0, 0, 96, 65, munkiki_title, 0, 0);
    } else if (mkf.screen == SCREEN_SCORES) {
        int16_t n = (int16_t)((mkf.counter << 1) - 24);

        frame_clear();
        frame_rect(20, 0, 53, 16);
        number_draw(mkf.top_score, 63, 2, 1);
        if (mkf.second_score != 0) {
            frame_rect(20, 48, 53, 16);
            number_draw(mkf.second_score, 63, 50, 1);
        }
        mark_draw(n, 25);
    } else if (mkf.screen == SCREEN_GAME || mkf.screen == SCREEN_DEMO) {
        game_draw();
    } else {
        frame_clear();
    }
    frame_present();
}


uint8_t mk_page_height(uint8_t which) MK_FAR
{
    if (which == PART_LEVEL)
        return wrapped_height(FONT_SMALL, text_with(mk.level % 5 == 0 && mk.level < 25 ? TEXT_COLLECT_N_OR_KEY
                                                                                           : TEXT_COLLECT_N,
                                                    mk.notes_left));
    return wrapped_height(FONT_SMALL, hint_text());
}

uint8_t mk_help_lines(void) MK_FAR
{
    return wrap_count(FONT_SMALL, munkiki_texts[mk_help_order[mkf.help_page - 1]]);
}

void mk_hud_draw(void) MK_FAR
{
    hud_draw();
}
