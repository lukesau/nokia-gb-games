/* Munkiki's Castles: the screens around the game and the game's flow,
   after the JAR's MIDlet class KKM and its canvas class b: the title,
   the game's own menu and the other lists the phone's MIDP draws for it
   (here drawn in the phone's list style, measured in MAME), the text
   pages before a level, the game with its HUD, the level's end with its
   bonus count, the map of the castles, game over, the High scores page,
   the Instructions with their demos, and the Options. The JAR's names
   are in the comments.

   Text is drawn with the phone's fonts, as the KVM draws it: its SMALL,
   MEDIUM and LARGE are the phone's small plain, medium bold and large
   bold fonts, a glyph a row under the line's top; the Game Boy reaches
   the fonts, which are in other banks, a glyph at a time (mk_glyph). */
#include "munkiki_int.h"

#include <string.h>

#include "font.h"
#include "game_assets.h"
#include "games.h"
#include "sound.h"

/* The screens (KKM.aj). */
enum {
    SCREEN_LIST = -1,
    SCREEN_TITLE = 0,
    SCREEN_GAME = 2,
    SCREEN_SCORES = 3,
    SCREEN_HELP = 4,
    SCREEN_DEMO = 5
};
/* The phone's own pages over the canvas (KKM.ai). */
enum {
    LIST_NONE = 0,
    LIST_PAUSE = 24,
    LIST_MAIN = 25,
    LIST_HELP = 26,
    LIST_OPTIONS = 27,
    LIST_LEVEL = 28
};
/* The game's parts (KKM.ab). */
enum {
    PART_HINT = 0,  /* the "(... press * to restart)" text */
    PART_LEVEL = 1, /* "Level N" and what to collect */
    PART_PLAY = 2,
    PART_DONE = 3,  /* the level's end, Munkiki dancing */
    PART_BONUS = 4,
    PART_MAP = 5,
    PART_OVER = 7
};
/* The texts, by index in the JAR's language file. */
enum {
    TEXT_CONTINUE,
    TEXT_NEW_GAME,
    TEXT_HIGH_SCORES,
    TEXT_INSTRUCTIONS,
    TEXT_LEVEL_N,
    TEXT_COLLECT_N,
    TEXT_COLLECT_N_OR_KEY,
    TEXT_HELP_FIRST,       /* 7 to 16, with the demos between */
    TEXT_HINT = 15,
    TEXT_LIVES_LEFT = 17,
    TEXT_RETURN,
    TEXT_RESTART,
    TEXT_GAME_MENU,
    TEXT_END_GAME,
    TEXT_OPTIONS,
    TEXT_SETTINGS,
    TEXT_GAME_SOUNDS,
    TEXT_LIGHTS,
    TEXT_SOUNDS,
    TEXT_SHAKES,
    TEXT_OFF,
    TEXT_ON
};
/* The Instructions: texts and, as negatives, the demos' levels (KKM.e). */
static const int8_t help_order[16] = { 7, 8, 9, -90, 10, -91, 11, -92, 12, -93, 13, -94, 14, -95, 15, 16 };
/* The key a demo presses on its 15th tick, by page. */
static const int8_t demo_keys[16] = { -1, -1, -1, 2, -1, 5, -1, 0, -1, 2, -1, 5, -1, 3, -1, -1 };

#define TICK_US 100000UL

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

static int8_t screen = SCREEN_LIST;    /* aj */
static uint8_t list = LIST_NONE;       /* ai */
static uint8_t part;                   /* ab */
static int16_t counter;                /* ad */
static uint8_t help_page;              /* ag */
static int16_t bonus_time, bonus_key, bonus_warp, bonus_last; /* r, o, q, p */
static uint16_t top_score, second_score; /* n, m */
static uint8_t level_reached = 1;      /* w */
static uint8_t can_continue;           /* af: a game to go on with */
static uint8_t demo_fresh;             /* ac */
static uint8_t sounds_on = 1, lights_on = 1; /* k, j */
static uint8_t game_exists;            /* ak != null */
static uint8_t game_saved;             /* the JAR's KKM record exists */
static uint8_t selection, list_top;    /* the list's cursor and first row */
static uint8_t list_count;
static uint8_t option_row;             /* the Options page's cursor */
static uint8_t text_scroll;            /* the Instructions page's first row shown */
static uint32_t elapsed_us;
static uint8_t back_held;              /* the C key, which acts when released */
static uint8_t record_changed;         /* for the port to save it */
static int16_t percent;                /* what the game over page says */
/* What the JAR saves at each level's start (d.ax), for Continue after
   the game ends. */
static int8_t saved_lives;
static uint16_t saved_score;
static uint8_t saved_keys, saved_level, saved_deaths;

void mk_lives_reset(void);
void mk_life_take(void);

/* The port's record. */
void munkiki_record_set(const struct munkiki_record *in) MK_FAR
{
    top_score = in->top;
    second_score = in->second;
    level_reached = in->level_reached ? in->level_reached : 1;
    sounds_on = in->sounds;
    lights_on = in->lights;
}

void munkiki_record_get(struct munkiki_record *out) MK_FAR
{
    out->top = top_score;
    out->second = second_score;
    out->level_reached = level_reached;
    out->sounds = sounds_on;
    out->lights = lights_on;
}

/* KKM.i: a sound, when they are on. */
void mk_play(uint8_t sound)
{
    if (sounds_on && games_options.sounds)
        sound_play_notes(munkiki_sounds + munkiki_sound_at[sound], MUNKIKI_SOUND_BANK);
}

/* Text. */
#ifndef MUNKIKI_BANKED
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
    mk_frame_fill(0, 4, (uint8_t)(x > 2 ? x - 2 : 0), 1, 1);
    mk_frame_fill((int8_t)(x + w + 2), 4, (uint8_t)(x + w + 2 < 89 ? 89 - (x + w + 2) : 0), 1, 1);
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
            mk_frame_fill(0, (int8_t)y, 92, 11, 1);
        text_draw_rows(FONT_MEDIUM, 2, (int16_t)(y + 1), name((uint8_t)(top + i)),
                       (uint8_t)strlen(name((uint8_t)(top + i))), on, 10);
    }
    mk_frame_fill(94, 10, 1, 44, 1);
    height = (uint8_t)(42 / count);
    thumb = (uint8_t)(11 + cursor * height);
    mk_frame_fill(93, (int8_t)thumb, 1, height, 1);
    mk_frame_fill(95, (int8_t)thumb, 1, height, 1);
}

/* The lists' entries. */
static const char *main_name(uint8_t i)
{
    if (can_continue) {
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
    return text_with(TEXT_LEVEL_N, i == 0 ? 1 : level_reached);
}

static const char *option_name(uint8_t i)
{
    return munkiki_texts[i == 0 ? TEXT_GAME_SOUNDS : TEXT_LIGHTS];
}

/* d.aw with d.ax: the level loaded, and the game as the JAR saves it
   there. */
static void level_load(uint8_t level)
{
    mk_world_load(level);
    if (screen != SCREEN_DEMO)
        game_saved = 1;
    saved_lives = mk.lives;
    saved_score = mk.score;
    saved_keys = mk.keys;
    saved_level = level;
    saved_deaths = mk.deaths;
}

/* d's construction: a new game, or the one put away. */
static void game_new(uint8_t fresh)
{
    mkv.counter = 0;
    mk.demo = 0;
    mk.demo_key = -1;
    if (fresh) {
        mk_lives_reset();
        mk.lives = 3;
        mk.keys = 0;
        mk.score = 0;
        level_load(level_reached);
    } else if (mk.lives < 0 || part == PART_OVER) {
        /* After the game ended, as the JAR saved it when the level began. */
        mk.lives = saved_lives;
        mk.score = saved_score;
        mk.keys = saved_keys;
        mk.deaths = saved_deaths;
        level_load(saved_level);
    } else {
        /* The game as it was left, with Munkiki sliding in again. The JAR
           reloads it from its record, which snaps moving blocks to their
           cells; here it goes on as it was. */
        mk.phase = -1;
        mkv.dy = -64;
        mk.frame = 0;
        mk.action = 17;
        mk.anim = 15;
        mk.left = munkiki_anim_table[15].count;
        mk.vz = mk.vx = mk.speed = mk.vy = mk.vyaw = 0;
    }
    game_exists = 1;
}

static void list_open(uint8_t which);
static void part_open(uint8_t which);

/* KKM.p: a screen. */
static void screen_open(int8_t which)
{
    list = LIST_NONE;
    screen = which;
    switch (which) {
    case SCREEN_TITLE:
        mk_play(MK_SOUND_TITLE);
        break;
    case SCREEN_GAME:
    case SCREEN_DEMO:
        game_new((uint8_t)(counter == 1));
        if (counter == 1)
            mk.deaths = 0;
        break;
    case SCREEN_SCORES:
        counter = 0;
        break;
    default:
        break;
    }
}

/* The level map's picture (KKM.q case 5): the castles' towers, a mark
   on each level done, and the keys found. */
static void map_draw(void)
{
    uint8_t n3 = (uint8_t)(25 + mk.keys), n4 = n3 > 25 ? 3 : 2, n5;
    int16_t n, n2;

    mk_frame_line(3, 51, 6, 54);
    mk_frame_line(7, 54, 10, 51);
    /* The towers' lower arcs, clipped to the rows from 42; then the
       upper, clipped to the first nine rows, and to 65 columns when the
       third tower is a key's room. The frame is empty but for them. */
    for (n5 = 0; n5 < 2; n5++) {
        mk_frame_arc((int16_t)(16 + 26 * n5), 29, 21, 21, 1);
        mk_frame_arc((int16_t)(23 + 26 * n5), 38, 7, 7, 1);
    }
    memset(mk_frame, 0, 42 * MK_STRIDE);
    for (n5 = 0; n5 < n4; n5++) {
        mk_frame_arc((int16_t)(3 + 26 * n5), 1, 21, 21, 0);
        mk_frame_arc((int16_t)(10 + 26 * n5), 6, 7, 7, 0);
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
        mk_frame_line((int16_t)(3 + n5 * 13), 9, (int16_t)(3 + n5 * 13), n4);
        mk_frame_line((int16_t)(10 + n5 * 13), 9, (int16_t)(10 + n5 * 13), n4);
    }
    if (n3 == 25) {
        mk_frame_line(55, 9, 55, 1);
        mk_frame_line(62, 9, 62, 1);
        mk_frame_line(55, 1, 62, 1);
    } else if (n3 == 26) {
        mk_frame_line(64, 1, 75, 1);
        mk_frame_line(64, 6, 75, 6);
        mk_frame_line(75, 1, 75, 6);
    } else {
        n2 = (int16_t)(n3 * 10 - 251);
        mk_frame_line(68, 9, 68, n2);
        mk_frame_line(75, 9, 75, n2);
        mk_frame_line(68, n2, 75, n2);
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
            mk_frame_blit_part(n2, yy, 11, 9, munkiki_map, 8, 0, 53);
        }
    }
    for (n5 = 0; n5 < mk.keys; n5++) {
        n = (int16_t)(1 + n5 * 10);
        /* drawImage(map, 80 - (level < 26 + n5 ? 0 : 13), n - 13) clipped to (80, n, 14, 10). */
        mk_frame_blit_part(80, n, 14, 10, munkiki_map, 8, (uint8_t)(mk.level < 26 + n5 ? 0 : 13), 13);
    }
}

/* The 16 by 14 at the map's top left: Munkiki's mark. */
static void mark_draw(int16_t x, int16_t y)
{
    mk_frame_blit_part(x, y, 16, 14, munkiki_map, 8, 0, 0);
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

/* KKM.q: a part of the game. */
static void part_open(uint8_t which)
{
    list = LIST_NONE;
    part = which;
    switch (which) {
    case PART_HINT:
        counter = (int16_t)-((65 - wrapped_height(FONT_SMALL, hint_text())) >> 1);
        if (counter > 0)
            counter = 0;
        mk.level = (uint8_t)(level_reached - 1);
        break;
    case PART_LEVEL: {
        const char *body = text_with(mk.level % 5 == 0 && mk.level < 25 ? TEXT_COLLECT_N_OR_KEY : TEXT_COLLECT_N,
                                     mk.notes_left);

        counter = (int16_t)-((65 - wrapped_height(FONT_SMALL, body) - 14) >> 1);
        if (counter > 0)
            counter = 0;
        break;
    }
    case PART_BONUS:
        level_reached = (uint8_t)(mk.level + 1);
        if (mk.level > 30)
            level_reached = 30;
        record_changed = 1;
        counter = 0;
        break;
    case PART_MAP:
        counter = 0;
        break;
    case PART_OVER: {
        uint8_t n = mk.level;

        level_reached = mk.level;
        record_changed = 1;
        mk_play(MK_SOUND_GAME_OVER);
        counter = mk.lives < 0 ? 0 : 20;
        if (mk.lives < 0)
            n--;
        percent = (int16_t)(n * 90 / 30 + mk.score / 1000);
        break;
    }
    default:
        break;
    }
}

/* KKM.r: one of the phone's pages. */
static void list_open(uint8_t which)
{
    list = which;
    selection = list_top = 0;
    switch (which) {
    case LIST_LEVEL:
        list_count = 2;
        break;
    case LIST_PAUSE:
        list_count = 3;
        break;
    case LIST_OPTIONS:
        screen = SCREEN_LIST;
        option_row = 0;
        break;
    case LIST_HELP:
        counter = 0;
        help_page++;
        demo_fresh = 0;
        if (help_page > 16) {
            list_open(LIST_MAIN);
            return;
        }
        if (help_order[help_page - 1] < 0) {
            counter = 1;
            screen_open(SCREEN_DEMO);
            part_open(PART_PLAY);
            level_load((uint8_t)-help_order[help_page - 1]);
            demo_fresh = 1;
            list = LIST_NONE;
            if (help_order[help_page - 1] != -95)
                mk.demo = 1;
            return;
        }
        screen = SCREEN_HELP;
        text_scroll = 0;
        break;
    default:
        list = LIST_MAIN;
        screen = SCREEN_LIST;
        can_continue = game_saved;
        list_count = (uint8_t)(4 + can_continue);
        break;
    }
}

/* KKM.n: a life lost, or the game over. */
static void life_lost(void)
{
    if (mk.lives > 0) {
        mk.lives--;
        mk.deaths++;
        if (mk.life_taken)
            mk_life_take();
        level_load(mk.level);
        part_open(PART_LEVEL);
    } else {
        mk.lives--;
        part_open(PART_OVER);
    }
}

/* KKM.l: a hundred, ten or one of a bonus into the score. */
static int16_t bonus_step(int16_t n)
{
    int16_t step = n >= 100 ? 100 : n >= 10 ? 10 : 1;

    mk.score = (uint16_t)(mk.score + step);
    return (int16_t)(n - step);
}

static void scores_update(void)
{
    if (mk.score > top_score) {
        second_score = top_score;
        top_score = mk.score;
    } else {
        second_score = mk.score;
    }
}

static void scene_render(void);

/* KKM.m: a tick. */
static void game_tick(void)
{
    if (list != LIST_NONE)
        return;
    switch (screen) {
    case SCREEN_TITLE:
        if (mk_any_pressed() || ++counter > 25)
            list_open(LIST_MAIN);
        break;
    case SCREEN_SCORES:
        if (mk_any_pressed() || ++counter == 64)
            list_open(LIST_MAIN);
        break;
    case SCREEN_DEMO:
        if (demo_fresh) {
            counter = 1;
            demo_fresh = 0;
        }
        counter++;
        if ((mk_pressed & (1 << 5)) || counter > 50) {
            mk_pressed &= (uint16_t)~(1 << 5);
            list_open(LIST_HELP);
            break;
        }
        mk_pressed = 0;
        if (counter == 15 && demo_keys[help_page - 1] >= 0)
            mk_pressed = (uint16_t)(1 << demo_keys[help_page - 1]);
        /* then as the game */
        /* fall through */
    case SCREEN_GAME:
        if (part == PART_HINT || part == PART_LEVEL) {
            uint8_t head = part == PART_LEVEL ? 14 : 0;
            uint8_t height = wrapped_height(FONT_SMALL, part == PART_LEVEL
                ? text_with(mk.level % 5 == 0 && mk.level < 25 ? TEXT_COLLECT_N_OR_KEY : TEXT_COLLECT_N, mk.notes_left)
                : hint_text());

            if (height > 65 - head) {
                if (mk_pressed & (1 << 2)) {
                    counter -= 8;
                    if (counter < 0)
                        counter = 0;
                    mk_pressed &= (uint16_t)~(1 << 2);
                }
                if (mk_pressed & (1 << 8)) {
                    int16_t most = (int16_t)(height + head - 65);

                    counter += 8;
                    if (counter > most)
                        counter = most;
                    mk_pressed &= (uint16_t)~(1 << 8);
                }
            }
        }
        switch (part) {
        case PART_HINT:
            if (mk_any_pressed())
                part_open(PART_MAP);
            break;
        case PART_LEVEL:
            if (mk_any_pressed()) {
                part_open(PART_PLAY);
                mk_world_tick();
                scene_render();
            }
            break;
        case PART_PLAY:
            mk_world_tick();
            if (screen == SCREEN_DEMO)
                mk_pressed &= (uint16_t)~(1 << 5);
            if (mk.ended == 1) {
                life_lost();
                break;
            }
            if (mk.ended == 0 || mk.level >= 90) {
                scene_render();
                break;
            }
            counter = 0;
            part_open(PART_DONE);
            bonus_key = mk.ended == MK_THING_KEY ? 500 : 0;
            bonus_warp = mk.ended == MK_THING_WARP ? (int16_t)((5 - mk.level % 5) * 400) : 0;
            bonus_time = (bonus_warp + bonus_key == 0 && mk.time < mk.moves)
                             ? (int16_t)(190L * mk.time / (2L * mk.moves - mk.time) + 10)
                             : 200;
            while (mk.deaths > 0) {
                bonus_time = (int16_t)(bonus_time * 3 >> 2);
                mk.deaths--;
            }
            if (mk.level < 26)
                bonus_time >>= 1;
            bonus_last = mk.level == 30 ? 400 : 0;
            break;
        case PART_DONE: {
            int16_t n = 38;

            if (25 + mk.keys == mk.level) {
                mkv.curtain = (uint8_t)counter;
                mkv.curtain_kind = mk.level == 30 ? 2 : 1;
                n = 58;
            }
            mk.anim = 11;
            mk_world_tick();
            scene_render();
            if (++counter > n)
                part_open(PART_BONUS);
            break;
        }
        case PART_BONUS:
            if (++counter <= 8)
                break;
            if (bonus_time > 0) {
                if ((bonus_time = bonus_step(bonus_time)) == 0)
                    counter = 0;
            } else if (bonus_key > 0) {
                if ((bonus_key = bonus_step(bonus_key)) == 0)
                    counter = 0;
            } else if (bonus_warp > 0) {
                if ((bonus_warp = bonus_step(bonus_warp)) == 0) {
                    counter = 0;
                    bonus_warp = -100;
                }
            } else if (bonus_last > 0) {
                if ((bonus_last = bonus_step(bonus_last)) == 0)
                    counter = 0;
            } else if (counter > 20) {
                if (mk.level < 25 + mk.keys)
                    part_open(PART_MAP);
                else
                    part_open(PART_OVER);
            }
            break;
        case PART_MAP:
            if (mk_any_pressed() || ++counter > 50) {
                mk.level++;
                if (bonus_warp == -100 && mk.level % 5 != 0) {
                    bonus_warp = 100;
                    part_open(PART_BONUS);
                    break;
                }
                level_load(mk.level);
                mk.deaths = 0;
                part_open(PART_LEVEL);
            }
            break;
        case PART_OVER:
            if (++counter > 50) {
                scores_update();
                record_changed = 1;
                list_open(LIST_MAIN);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* The screens' pictures. */
static void hud_draw(void);

/* The game's picture is made as the JAR paints it, after every tick. */
static void scene_render(void)
{
    mk_scene_draw();
    hud_draw();
}

static void hud_draw(void)
{
    if (mk.notes_left != 0 && mk.ended == 0) {
        mk_frame_blit_part(0, 0, 25, 14, munkiki_digits, 8, 0, 0);
        mk_frame_digits(mk.notes_left < 10 ? 11 : 16, 3, mk.notes_left);
    }
    if (mk.demo) {
        mk_frame_blit(73, 2, 21, 23, munkiki_png_kb, 0, 0);
        if (mk.demo_key != -1) {
            uint8_t n = (uint8_t)mk.demo_key;

            if (n == 11)
                n = 12;
            if (n == 0)
                n = 11;
            n--;
            mk_frame_blit((int16_t)(75 + 6 * (n % 3)), (int16_t)(4 + 5 * (n / 3)), 5, 4, munkiki_png_ky, 0, 0);
        }
    }
}

static void game_draw(void)
{
    switch (part) {
    case PART_HINT:
        mk_frame_clear();
        wrapped_draw(FONT_SMALL, (int16_t)-counter, hint_text());
        break;
    case PART_LEVEL: {
        const char *body = text_with(mk.level % 5 == 0 && mk.level < 25 ? TEXT_COLLECT_N_OR_KEY : TEXT_COLLECT_N,
                                     mk.notes_left);

        mk_frame_clear();
        wrapped_draw(FONT_SMALL, (int16_t)(14 - counter), body);
        mk_frame_fill(0, 0, 96, 13, 0);
        wrapped_draw(FONT_MEDIUM, 0, text_with(TEXT_LEVEL_N, mk.level));
        mk_frame_line(0, 12, 95, 12);
        break;
    }
    case PART_PLAY:
    case PART_DONE:
        /* Made at the tick. */
        break;
    case PART_BONUS: {
        int16_t n;

        mk_frame_clear();
        wrapped_draw(FONT_MEDIUM, 0, text_with(TEXT_LEVEL_N, mk.level));
        mk_frame_line(0, 12, 95, 12);
        mk_frame_rect(20, 48, 53, 16);
        number_draw(mk.score, 63, 50, 1);
        if (bonus_time > 0) {
            n = bonus_time;
            mk_frame_blit_part(3, 20, 28, 22, munkiki_map, 8, 26, 18);
        } else if (bonus_key > 0) {
            n = bonus_key;
            mk_frame_blit_part(16, 23, 15, 16, munkiki_map, 8, 50, 49);
        } else if (bonus_warp > 0) {
            n = bonus_warp;
            mk_frame_blit_part(9, 20, 22, 22, munkiki_map, 8, 26, 40);
        } else {
            n = bonus_last;
            if (n != 0)
                mk_frame_blit_part(5, 22, 26, 18, munkiki_map, 8, 26, 0);
        }
        if (n > 0)
            number_draw((uint16_t)n, 63, 24, 0);
        break;
    }
    case PART_MAP: {
        int16_t x, y, px, py;

        mk_frame_clear();
        level_spot(mk.level, &x, &y);
        if (counter < 19) {
            int16_t n5 = counter > 8 ? counter - 8 : 0;

            map_draw();
            level_spot((uint8_t)(mk.level - 1), &px, &py);
            mark_draw((int16_t)((x - px) * n5 / 10 + px), (int16_t)((y - py) * n5 / 10 + py));
        } else if (counter < 39 && mk.level > 24) {
            int16_t n10, nn;

            map_draw();
            if (counter < 35)
                mark_draw(x, y);
            if (counter < 26) {
                /* clip (counter + 51, y + 4, 34 - counter, 7) of the map at (counter + 37, y + 1) */
                mk_frame_blit_part((int16_t)(counter + 51), (int16_t)(y + 4), (uint8_t)(34 - counter), 7, munkiki_map, 8,
                                   14, 3);
                break;
            }
            n10 = counter < 35 ? (counter - 1) % 3 : 0;
            if (counter > 34) {
                nn = (int16_t)(counter - 35);
                /* clip (80, y + 1, 14, 5) of the map drawn at (80 - (nn == 3 ? 13 : 0), y - 12 - (nn + 1 & 3) * 10) */
                mk_frame_blit_part(80, (int16_t)(y + 1), 14, 5, munkiki_map, 8, (uint8_t)(nn == 3 ? 13 : 0),
                                   (uint8_t)(13 + ((nn + 1) & 3) * 10));
            }
            /* clip (78, y + 4, 7, 7) of the map at (65, y - 22 - n10 * 10) */
            mk_frame_blit_part(78, (int16_t)(y + 4), 7, 7, munkiki_map, 8, 13, (uint8_t)(26 + n10 * 10));
        } else {
            map_draw();
            mark_draw(x, y);
        }
        break;
    }
    case PART_OVER:
        mk_frame_clear();
        if (counter < 20) {
            mk_frame_blit(0, (int16_t)(5 - (counter < 10 ? (10 - counter) * 6 : 0)), 96, 54, munkiki_over, 0, 0);
        } else {
            char text[8];
            uint8_t n = 0;
            int16_t v = percent;

            mk_frame_rect(20, 10, 53, 16);
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
    const char *text = munkiki_texts[help_order[help_page - 1]];
    uint8_t len, row = 0;

    mk_frame_clear();
    list_header(munkiki_texts[TEXT_INSTRUCTIONS], help_page);
    while (*text) {
        const char *next = wrap_next(FONT_SMALL, text, &len);

        if (row >= text_scroll && row - text_scroll < 4)
            text_draw(FONT_SMALL, 1, (int16_t)(10 + (row - text_scroll) * 11 + 1), text, len, 0);
        row++;
        text = *next == ' ' ? next + 1 : next;
    }
    softkeys(mk_text_more, mk_text_back);
}

static void options_draw(void)
{
    uint8_t i;

    mk_frame_clear();
    list_header(munkiki_texts[TEXT_OPTIONS], (uint8_t)(option_row + 1));
    for (i = 0; i < 2; i++) {
        uint8_t y = (uint8_t)(10 + i * 22), on = (uint8_t)(i == option_row);
        const char *value = munkiki_texts[(i == 0 ? sounds_on : lights_on) ? TEXT_ON : TEXT_OFF];

        text_draw_z(FONT_SMALL, 2, (int16_t)(y + 1), option_name(i), 0);
        if (on)
            mk_frame_fill(0, (int8_t)(y + 11), 92, 11, 1);
        text_draw_rows(FONT_MEDIUM, 2, (int16_t)(y + 12), value, (uint8_t)strlen(value), on, 10);
    }
    softkeys(mk_text_select, mk_text_back);
}

void munkiki_draw(void) MK_FAR
{
    if (list == LIST_MAIN || list == LIST_PAUSE || list == LIST_LEVEL) {
        mk_frame_clear();
        if (list == LIST_MAIN)
            list_header("munkiki's castles", (uint8_t)(selection + 1));
        else if (list == LIST_PAUSE)
            list_header(text_with(TEXT_LIVES_LEFT, (uint16_t)mk.lives), (uint8_t)(selection + 1));
        else
            list_header(munkiki_texts[TEXT_NEW_GAME], (uint8_t)(selection + 1));
        list_rows(list == LIST_MAIN ? main_name : list == LIST_PAUSE ? pause_name : level_name, list_count, selection,
                  list_top);
        softkeys(mk_text_select, mk_text_back);
    } else if (list == LIST_OPTIONS) {
        options_draw();
    } else if (screen == SCREEN_HELP) {
        help_draw();
    } else if (screen == SCREEN_TITLE) {
        mk_frame_blit(0, 0, 96, 65, munkiki_title, 0, 0);
    } else if (screen == SCREEN_SCORES) {
        int16_t n = (int16_t)((counter << 1) - 24);

        mk_frame_clear();
        mk_frame_rect(20, 0, 53, 16);
        number_draw(top_score, 63, 2, 1);
        if (second_score != 0) {
            mk_frame_rect(20, 48, 53, 16);
            number_draw(second_score, 63, 50, 1);
        }
        mark_draw(n, 25);
    } else if (screen == SCREEN_GAME || screen == SCREEN_DEMO) {
        game_draw();
    } else {
        mk_frame_clear();
    }
    mk_frame_present();
}

void munkiki_open(void) MK_FAR
{
    elapsed_us = 0;
    mk_pressed = mk_released = 0;
    back_held = 0;
    help_page = 0;
    can_continue = 0;
    game_saved = 0;
    game_exists = 0;
    list = LIST_NONE;
    counter = 0;
    screen_open(SCREEN_TITLE);
}

/* The lists' keys (KKM.commandAction and the List's own). */
static uint8_t list_key(uint8_t key)
{
    if (list == LIST_OPTIONS) {
        if (key == MK_KEY_UP || key == MK_KEY_DOWN) {
            option_row ^= 1;
        } else if (key == MK_KEY_SELECT || key == 4 || key == 6) {
            if (option_row == 0) {
                sounds_on ^= 1;
                if (sounds_on)
                    mk_play(MK_SOUND_NOTE);
            } else {
                lights_on ^= 1;
            }
        } else if (key == MK_KEY_BACK) {
            record_changed = 1;
            list_open(LIST_MAIN);
        }
        return MK_DRAW;
    }
    if (screen == SCREEN_HELP) {
        if (key == MK_KEY_SELECT) {
            list_open(LIST_HELP);
        } else if (key == MK_KEY_BACK) {
            list_open(LIST_MAIN);
        } else if (key == MK_KEY_DOWN) {
            if (wrap_count(FONT_SMALL, munkiki_texts[help_order[help_page - 1]]) > text_scroll + 4)
                text_scroll++;
        } else if (key == MK_KEY_UP && text_scroll) {
            text_scroll--;
        }
        return MK_DRAW;
    }
    if (key == MK_KEY_UP || key == MK_KEY_DOWN) {
        if (key == MK_KEY_UP)
            selection = (uint8_t)(selection ? selection - 1 : list_count - 1);
        else
            selection = (uint8_t)(selection + 1 < list_count ? selection + 1 : 0);
        if (selection < list_top)
            list_top = selection;
        if (selection >= list_top + 4)
            list_top = (uint8_t)(selection - 3);
        return MK_DRAW;
    }
    if (key == MK_KEY_BACK) {
        if (list == LIST_MAIN)
            return MK_CLOSED;
        list_open(LIST_MAIN);
        return MK_DRAW;
    }
    if (key != MK_KEY_SELECT)
        return 0;
    switch (list) {
    case LIST_LEVEL:
        if (selection == 0)
            level_reached = 1;
        counter = 1;
        screen_open(SCREEN_GAME);
        part_open(PART_HINT);
        break;
    case LIST_PAUSE:
        if (selection == 0) {
            part_open(PART_PLAY);
        } else if (selection == 1) {
            list = LIST_NONE;
            life_lost();
        } else {
            list_open(LIST_MAIN);
        }
        break;
    default: {
        int8_t n = (int8_t)(selection - can_continue);

        if (n == -1) {
            counter = 0;
            screen_open(SCREEN_GAME);
            part_open(PART_PLAY);
        } else if (n == 0) {
            if (level_reached > 1) {
                list_open(LIST_LEVEL);
            } else {
                counter = 1;
                screen_open(SCREEN_GAME);
                part_open(PART_HINT);
            }
        } else if (n == 1) {
            screen_open(SCREEN_SCORES);
        } else if (n == 2) {
            help_page = 0;
            list_open(LIST_HELP);
        } else {
            list_open(LIST_OPTIONS);
        }
        break;
    }
    }
    return MK_DRAW;
}

uint8_t munkiki_key(uint8_t key, uint8_t down) MK_FAR
{
    if (list != LIST_NONE || screen == SCREEN_HELP) {
        if (!down)
            return 0;
        return list_key(key);
    }
    if (key == MK_KEY_BACK) {
        /* b.c: the C key acts when it comes up. */
        if (down) {
            back_held = 1;
            return 0;
        }
        if (!back_held)
            return 0;
        back_held = 0;
        if (screen == SCREEN_GAME) {
            list_open(LIST_PAUSE);
        } else {
            list_open(LIST_MAIN);
        }
        return MK_DRAW;
    }
    if (key == MK_KEY_SELECT)
        key = 5;
    if (key > MK_KEY_STAR)
        return 0;
    if (down) {
        mk_pressed |= (uint16_t)(1 << key);
        mk_released &= (uint16_t)~(1 << key);
    } else {
        mk_released |= (uint16_t)(1 << key);
    }
    return 0;
}

uint8_t munkiki_tick(void) MK_FAR
{
    uint8_t what = MK_DRAW;

    game_tick();
    /* KKM.j: * held while the game paints opens the pause list. */
    if (screen == SCREEN_GAME && part == PART_PLAY && list == LIST_NONE && (mk_pressed & MK_BIT_STAR)) {
        mk_pressed &= (uint16_t)~MK_BIT_STAR;
        list_open(LIST_PAUSE);
    }
    if (record_changed) {
        record_changed = 0;
        what |= MK_RECORD;
    }
    /* b.e: keys that came up are forgotten after the tick saw them. */
    mk_pressed &= (uint16_t)~mk_released;
    return what;
}

uint8_t munkiki_elapse(uint16_t us) MK_FAR
{
    uint8_t what = 0;

    elapsed_us += us;
    while (elapsed_us >= TICK_US) {
        elapsed_us -= TICK_US;
        what |= munkiki_tick();
    }
    return what;
}

#ifndef __SDCC
#include <stdio.h>

void munkiki_select(uint8_t entry)
{
    selection = entry;
    list_top = entry >= 4 ? (uint8_t)(entry - 3) : 0;
    munkiki_key(MK_KEY_SELECT, 1);
    munkiki_key(MK_KEY_SELECT, 0);
}

int munkiki_state_text(char *out, int size)
{
    int n = 0;
    uint8_t i;

    n += snprintf(out + n, size - n, "K aj=%d ai=%d ab=%d ad=%d ag=%d r=%d o=%d q=%d p=%d n=%d m=%d w=%d af=%d\n", screen,
                  list, part, counter, help_page, bonus_time, bonus_key, bonus_warp, bonus_last, top_score, second_score,
                  level_reached, can_continue);
    n += snprintf(out + n, size - n, "C bt=%d bs=%d br=%d bh=%d bg=%d bf=%d be=%d bc=%d a7=%d a6=%d a5=%d a4=%d\n",
                  mkv.counter, mkv.wobble, mkv.show, (int8_t)mkv.sprite, mkv.dx, mkv.dy, mk.block_count, mk.thing_count,
                  mkv.sparkle, mkv.splash, mkv.curtain, mkv.curtain_kind);
    if (game_exists) {
        n += snprintf(out + n, size - n,
                      "D aj=%d c_=%ld cz=%ld cy=%ld cx=%ld cn=%d bh=%d cp=%d cq=%d cr=%ld ct=%ld cu=%ld cv=%ld cw=%ld "
                      "c2=%d c3=%d c4=%d c5=%d c6=%d c7=%d c8=%d c9=%d b4=%d b5=%d b6=%d b7=%d b8=%d b9=%d by=%d bx=%d\n",
                      mk.phase, (long)mk.px, (long)mk.py, (long)mk.pz, (long)mk.facing, mk.anim, mk.frame, mk.action,
                      mk.left, (long)mk.vz, (long)mk.vx, (long)mk.speed, (long)mk.vy, (long)mk.vyaw, mk.ended, mk.keys,
                      mk.score, mk.lives, mk.level, mk.notes_left, mk.deaths, mk.moves, mk.land, mk.ride, mk.riding,
                      mk.time, mk.note_count, mk.thing_count, mk.mover_count, mk.life_taken);
        n += snprintf(out + n, size - n, "BZ");
        for (i = 0; i < mk.mover_count; i++) {
            const struct mk_mover *m = &mk_movers[i];

            n += snprintf(out + n, size - n, " %d %d %d %d %d %d %d %d", m->ref, m->vx * 4096, m->vy * 4096, m->vz * 4096,
                          m->ticks, m->x, m->y, m->z);
        }
        n += snprintf(out + n, size - n, "\nBD");
        for (i = 0; i < mk.block_count; i++) {
            const struct mk_block *b = &mk_blocks[i];

            n += snprintf(out + n, size - n, " %ld %ld %ld %d %d %d %d 0", (long)b->x * 4096, (long)b->y * 4096,
                          (long)b->z * 4096, b->kind, b->sx, b->sy, b->sz);
        }
        n += snprintf(out + n, size - n, "\nBA");
        for (i = 0; i < mk.thing_count; i++) {
            const struct mk_thing *t = &mk_things[i];

            n += snprintf(out + n, size - n, " %ld %ld %ld %d", (long)t->x * 4096, (long)t->y * 4096, (long)t->z * 4096,
                          t->kind);
        }
        n += snprintf(out + n, size - n, "\nCL ");
        for (i = 0; n < size - 4 && i < 255; i++)
            ;
        {
            uint16_t c;

            for (c = 0; c < MK_CELLS && n < size - 3; c++)
                n += snprintf(out + n, size - n, "%02x", mk_cell[c]);
            n += snprintf(out + n, size - n, "\nCK ");
            for (c = 0; c < MK_CELLS && n < size - 5; c++)
                n += snprintf(out + n, size - n, "%04x", mk_ref[c]);
            n += snprintf(out + n, size - n, "\n");
        }
    } else {
        n += snprintf(out + n, size - n, "D null\n");
    }
    return n;
}
#endif
