#include "menu.h"
#include "menu_header.h"

#include "bantumi_play.h"
#include "font.h"
#include "native_tiles.h"
#include "game.h"
#include "game_assets.h"
#include "games.h"
#include "lcd.h"
#include "rand.h"
#include "si.h"
#ifdef SI_PLATFORM_ROWS
#include "si_rows.h"
#endif
#include "snake2.h"
#include "sound.h"
#include "sprite.h"
#include "title.h"
#include "version.h"

/* The phone's menu pages: a header line with the page's title in the
   middle and the entry's number on the right, three rows of the large
   font, a scrollbar on the right and two soft keys along the bottom. */
#define LIST_Y 9
#define ROW_HEIGHT 15
#define ROW_TEXT_X 1
#define ROW_TEXT_DY 2
#define VISIBLE_ROWS 3
#define CONTENT_WIDTH 92 /* left of the scrollbar */
#define SCROLLBAR_X 94
#define SCROLLBAR_TOP LIST_Y
#define SCROLLBAR_HEIGHT 45
/* The thumb moves in the track less its first and last rows, and is that
   divided by the number of entries tall, but never less than 6. */
#define THUMB_TRACK (SCROLLBAR_HEIGHT - 2)
#define THUMB_MIN 6
#define SOFTKEY_Y 56

#define GAMES_MENU_NUMBER 7 /* Games is entry 7 of the phone's main menu */
#define GAMES_MENU_ENTRIES 10
#define TITLE_Y 12

enum {
    SCREEN_MAIN,
    SCREEN_GAMES,         /* Games: Select game, Settings */
    SCREEN_SELECT,        /* Select game: the five games */
    SCREEN_ABOUT,         /* the full-screen list's last entry: version and repository */
    SCREEN_SETTINGS,      /* the games' Settings: one page per setting */
    SCREEN_SETTING_VALUE, /* a setting's Off and On */
    SCREEN_TITLE,         /* Snake II's title animation, before its menu */
    SCREEN_GAME,          /* Snake II's own menu */
    SCREEN_OPTIONS,       /* its Game options: Mazes, Level */
    SCREEN_MAZES,
    SCREEN_LEVEL,
    SCREEN_HIGH_SCORES,
    SCREEN_HELP,
    SCREEN_PLAY,
    SCREEN_GAME_OVER,
    /* Space Impact's: its title, its menu, the game (with its continue
       screen and game-over picture), its High scores page, its Chapters
       and its Instructions. */
    SCREEN_SI_TITLE,
    SCREEN_SI_MENU,
    SCREEN_SI_PLAY,
    SCREEN_SI_SCORES,
    SCREEN_SI_CHAPTERS,
    SCREEN_SI_HELP,
    SCREEN_SI_DEMO,
    SCREEN_SI_DONE,
    /* Bantumi's: its title, its menu, the game, its Game options with the
       beans and the level, and its Instructions with their demos. */
    SCREEN_BT_TITLE,
    SCREEN_BT_MENU,
    SCREEN_BT_PLAY,
    SCREEN_BT_OPTIONS,
    SCREEN_BT_BEANS,
    SCREEN_BT_LEVEL,
    SCREEN_BT_HELP,
    SCREEN_BT_DEMO
};

/* Space Impact's menu. Continue is there only while a game is paused. */
enum {
    SI_ITEM_CONTINUE,
    SI_ITEM_NEW_GAME,
    SI_ITEM_HIGH_SCORES,
    SI_ITEM_CHAPTERS,
    SI_ITEM_INSTRUCTIONS
};
#define SI_HELP_TEXTS 4
#define SI_HELP_PAGES 8
#define SI_HELP_LINES 4

/* Bantumi's menu. Continue is there only while a game is paused. */
enum {
    BT_ITEM_CONTINUE,
    BT_ITEM_NEW_GAME,
    BT_ITEM_OPTIONS,
    BT_ITEM_INSTRUCTIONS
};
/* Five texts, each followed by its demo. */
#define BT_HELP_TEXTS 5

/* The games' settings, in the phone's order. The last is the phone's Club
   Nokia score ID, which has no ID and does nothing here. */
enum {
    SETTING_SOUNDS,
    SETTING_LIGHTS,
    SETTING_SHAKES,
    SETTING_CLUB_NOKIA_ID,
    SETTING_COUNT
};

/* The entries of Games. The phone's Download game and More games are
   services the port has not got. */
enum {
    GAMES_SELECT,
    GAMES_SETTINGS,
    GAMES_COUNT
};

/* Snake II's menu. Continue is there only while a game is paused. */
enum {
    ITEM_CONTINUE,
    ITEM_NEW_GAME,
    ITEM_HIGH_SCORES,
    ITEM_OPTIONS,
    ITEM_INSTRUCTIONS
};
static const uint8_t snake_items[] = { ITEM_NEW_GAME, ITEM_HIGH_SCORES, ITEM_OPTIONS, ITEM_INSTRUCTIONS };

/* Its Game options. */
enum {
    OPTION_MAZES,
    OPTION_LEVEL,
    OPTION_COUNT
};

#define HELP_Y 0
#define HELP_LINE_HEIGHT 10
#define HELP_LINES 5
#define HELP_PAGES 4

/* The Level page: nine bars, each three rows taller than the one before,
   a line on its right and a shadow below; the levels up to the one chosen
   are filled. */
#define LEVEL_TEXT_X 5
#define LEVEL_BAR_X 4
#define LEVEL_BAR_PITCH 9
#define LEVEL_BAR_WIDTH 5
#define LEVEL_BAR_BOTTOM 39

#define NO_KEY 0xff

static uint8_t screen;
static uint8_t games_item;  /* selection in Games */
static uint8_t game;        /* selection in Select game */
static uint8_t game_top;    /* first visible row of that list */
static uint8_t item;        /* selection in the game's menu, counting visible items */
static uint8_t item_top;    /* first visible row of the game's menu */
static uint8_t option;      /* selection in Game options */
static uint8_t setting;     /* the setting whose page is shown */
static uint8_t value;       /* selection on its Off and On page */
static uint8_t paused;      /* a game is waiting behind the menu */
static uint8_t level;       /* selection on the Level page, 0 is the first */
static uint8_t maze;        /* selection in the list of mazes */
static uint8_t maze_top;
static uint8_t help_page;
static uint8_t new_top_score;  /* the game just ended beat the top score */
static uint16_t final_score;   /* its score */
static uint8_t held = NO_KEY;  /* the button the game sees as held */
static uint8_t seed_fixed;     /* the games' generator is never reseeded from the time */
static uint16_t uptime;        /* menu_tick calls so far; seeds the games' generator */
static uint8_t board_drawn;    /* the LCD holds the running game's picture */
uint8_t menu_drew_picture;
static uint8_t full_screen;    /* the full-screen variant was chosen */
static uint8_t view_mode;      /* which of the views below the LCD is set up for */
#ifdef NATIVE_PLATFORM_TILES
/* The full-screen menu the platform shows from its tiles (native_tiles.h),
   NATIVE_NONE when it shows none, and the selection the cursor is on. */
static uint16_t tiled_id = NATIVE_NONE;
static uint8_t tiled_selection;
#endif
static uint8_t surround_used;  /* something is drawn around the phone's LCD */
static struct game_settings settings;
static uint16_t top_score;     /* the chosen maze's, which the 3410 keeps apart */
/* The last game's score, the maze it was played on and whether on the
   full-screen board, for the High scores page; none until a game ends. */
static uint8_t last_played, last_maze, last_full;
static uint16_t last_score;
/* Space Impact's menu and game. */
static uint8_t si_item, si_item_top, si_paused, si_help, si_last_played;
static const char *si_help_page; /* the first character of the page shown */
static const char *si_help_pages[SI_HELP_PAGES]; /* where the text's pages start, for Up */
static uint8_t si_help_pageno;
static uint8_t si_demo_waiting; /* a demo started, its first tick not yet come: the page stays up */
static uint8_t si_play_held(uint8_t keys);
static uint16_t si_top, si_last;
/* The "Done" note after a set of chapters is chosen: the phone's shows for
   about 1.5 s, by MAME's clock. */
#define SI_DONE_FRAMES (1500000ul / MENU_FRAME_US)
static uint8_t si_done_frames;
static uint16_t si_units;  /* phone timer units to the game's next tick; 0 none */
static uint16_t si_us;     /* time not yet turned into units */
static uint8_t si_held;    /* the buttons held in the game, MENU_KEY_ bits */
/* Bantumi's menu and game; its Instructions page through Space Impact's
   si_help_ variables. */
static uint8_t bt_item, bt_item_top, bt_paused, bt_option, bt_beans, bt_level, bt_help;
static uint8_t bt_demo_waiting; /* as si_demo_waiting */
static struct game_settings bt_settings;

/* The phone's LCD; the full-screen variant's own menus; its board. */
enum {
    VIEW_PHONE,
    VIEW_NATIVE,
    VIEW_BOARD
};

/* The full-screen board's part of the framebuffer: all of it, or the
   middle 1/LCD_ZOOM of it magnified, on whole 8x8 cells. */
#define BOARD_AREA_W (LCD_FB_WIDTH / LCD_ZOOM)
#define BOARD_AREA_H (LCD_FB_HEIGHT / LCD_ZOOM)
#define BOARD_AREA_X ((LCD_FB_WIDTH - BOARD_AREA_W) / 2 / 8 * 8)
#define BOARD_AREA_Y ((LCD_FB_HEIGHT - BOARD_AREA_H) / 2 / 8 * 8)

/* The full-screen variant's menus: a title bar, a list with every entry
   visible, and a line of button hints. The port's own design and words; the
   entries and their text are the phone's. A screen with room uses the
   phone's large font; a small one, shown magnified by its platform, its
   small fonts. */
#if LCD_FB_HEIGHT >= 120
#define NATIVE_FONT font_large_bold
#define NATIVE_BODY_FONT font_large_bold
#define NATIVE_TITLE_HEIGHT 15
#define NATIVE_LIST_Y 21
#define NATIVE_ROW_HEIGHT 15
#define NATIVE_HINT_Y (LCD_FB_HEIGHT - 14)
#define NATIVE_MARGIN 4
#define NATIVE_CURSOR_HEIGHT 9
#define NATIVE_NOTE_Y 48
#define NATIVE_NOTE_PITCH 16
#else
#define NATIVE_FONT font_small_bold
#define NATIVE_BODY_FONT font_small_plain
#define NATIVE_TITLE_HEIGHT 9
#define NATIVE_LIST_Y 12
#define NATIVE_ROW_HEIGHT 10
#define NATIVE_HINT_Y (LCD_FB_HEIGHT - 9)
#define NATIVE_MARGIN 2
#define NATIVE_CURSOR_HEIGHT 7
#define NATIVE_NOTE_Y 24
#define NATIVE_NOTE_PITCH 11
#endif
#define NATIVE_CURSOR_WIDTH ((NATIVE_CURSOR_HEIGHT + 1) / 2)
#define NATIVE_TEXT_X (NATIVE_MARGIN + NATIVE_CURSOR_WIDTH + 5)

static const char text_hint_select[] = "B back   A select";
static const char text_hint_more[] = "B back   A more";
static const char text_hint_back[] = "B back";
/* The full-screen list's extra entry and its page. The port's own words. */
static const char text_about[] = "About";
static const char text_about_body[] = "Nokia 3410 games " GAME_VERSION "\ngithub.com/lukesau/\nnokia-gb-games";

#if LCD_HAS_SURROUND
/* Shown under the phone's LCD on the first screen. The port's own words. */
static const char text_full_screen_hint[] = "START: full screen";
#endif

/* Where the chosen maze's top score is kept: the full-screen variant's
   board has its own. */
static uint8_t top_score_slot(void)
{
    return full_screen ? GAME_SLOT_SNAKE_FULL_MAZE(settings.option) : GAME_SLOT_SNAKE_MAZE(settings.option);
}

static void top_score_load(void)
{
    struct game_settings maze_record;

    platform_settings_load(top_score_slot(), &maze_record);
    top_score = maze_record.top_score;
}

/* The settings of Snake II, the one game here. */
static void settings_load(void)
{
    platform_settings_load(GAME_SNAKE, &settings);
    if (settings.level >= SNAKE2_LEVELS)
        settings.level = 0;
    if (settings.option >= SNAKE2_MAZE_COUNT)
        settings.option = 0;
    top_score_load();
}

static const char *maze_name(uint8_t which)
{
    switch (which) {
    case 0:
        return text_no_maze;
    case 1:
        return text_maze_box;
    case 2:
        return text_maze_tunnel;
    case 3:
        return text_maze_spiral;
    case 4:
        return text_maze_blockade;
    default:
        return text_maze_twisted;
    }
}

static const char *setting_name(uint8_t which)
{
    switch (which) {
    case SETTING_SOUNDS:
        return text_game_sounds;
    case SETTING_LIGHTS:
        return text_game_lights;
    case SETTING_SHAKES:
        return text_shakes;
    default:
        return text_club_nokia_id;
    }
}

/* The switch a setting is, or null for the score ID. */
static uint8_t *setting_switch(uint8_t which)
{
    switch (which) {
    case SETTING_SOUNDS:
        return &games_options.sounds;
    case SETTING_LIGHTS:
        return &games_options.lights;
    case SETTING_SHAKES:
        return &games_options.shakes;
    default:
        return 0;
    }
}

static const char *setting_value(uint8_t which)
{
    const uint8_t *on = setting_switch(which);

    return !on ? text_no_id : *on ? text_on : text_off;
}

static const char *game_name(uint8_t index)
{
    switch (index) {
    case GAME_SNAKE:
        return text_snake;
    case GAME_SPACE_IMPACT:
        return text_space_impact;
    case GAME_BUMPER:
        return text_bumper;
    case GAME_BANTUMI:
        return text_bantumi;
    default:
        return text_link5;
    }
}

static const char *games_name(uint8_t index)
{
    return index == GAMES_SELECT ? text_select_game : text_settings;
}

static uint8_t item_count(void)
{
    return (uint8_t)(sizeof snake_items + paused);
}

/* The item at visible position `index`. */
static uint8_t item_id(uint8_t index)
{
    if (paused) {
        if (!index)
            return ITEM_CONTINUE;
        index--;
    }
    return snake_items[index];
}

static const char *item_name(uint8_t id)
{
    switch (id) {
    case ITEM_CONTINUE:
        return text_continue;
    case ITEM_NEW_GAME:
        return text_new_game;
    case ITEM_HIGH_SCORES:
        return text_high_scores;
    case ITEM_OPTIONS:
        return text_options;
    default:
        return text_instructions;
    }
}

static const char *option_name(uint8_t index)
{
    return index == OPTION_MAZES ? text_mazes : text_level;
}

/* The Instructions' texts may be kept in another bank (the Game Boy's,
   far.c): the one shown is copied out first. */
#ifdef MENU_TEXT_FAR
void far_text_copy(char *to, const char *from, uint8_t size);
static char help_copy[TEXT_HELP_MAX];

static const char *help_load(const char *text)
{
    far_text_copy(help_copy, text, sizeof help_copy);
    return help_copy;
}
#else
#define help_load(text) (text)
#endif

static const char *help_text(uint8_t page)
{
    switch (page) {
    case 0:
        return text_help_snake_1;
    case 1:
        return text_help_snake_2;
    case 2:
        return text_help_snake_3;
    default:
        return text_help_snake_4;
    }
}

/* A page's header: menu_header.h. */
#define draw_header menu_header_draw

static void draw_scrollbar(uint8_t index, uint8_t count)
{
    uint8_t height = (uint8_t)(THUMB_TRACK / count), top;

    if (height < THUMB_MIN)
        height = THUMB_MIN;
    top = (uint8_t)(SCROLLBAR_TOP + 1 + (count > 1 ? index * (THUMB_TRACK - height) / (count - 1) : 0));
    lcd_fill_rect(SCROLLBAR_X, SCROLLBAR_TOP, 1, SCROLLBAR_HEIGHT, 1);
    lcd_fill_rect(SCROLLBAR_X, top, 1, height, 0);
    lcd_fill_rect(SCROLLBAR_X - 1, top, 1, height, 1);
    lcd_fill_rect(SCROLLBAR_X + 1, top, 1, height, 1);
}

static void draw_softkeys(const char *left, const char *right)
{
    font_draw(&font_small_bold, 0, SOFTKEY_Y, left, 1);
    if (right)
        font_draw(&font_small_bold, LCD_WIDTH - font_text_width(&font_small_bold, right), SOFTKEY_Y, right, 1);
}

static void draw_row(uint8_t row, const char *label, uint8_t selected)
{
    uint8_t y = (uint8_t)(LIST_Y + row * ROW_HEIGHT);

    if (selected)
        lcd_fill_rect(0, y, CONTENT_WIDTH, ROW_HEIGHT, 1);
    font_draw(&font_medium_bold, ROW_TEXT_X, y + ROW_TEXT_DY, label, !selected);
}

/* A list of `count` entries named by `name`, three rows from `top`. */
static void draw_list(const char *title, uint8_t count, uint8_t selection, uint8_t top,
                      const char *(*name)(uint8_t), const char *left)
{
    uint8_t row;

    draw_header(title, (uint8_t)(selection + 1));
    for (row = 0; row < VISIBLE_ROWS && row < count; row++) {
        uint8_t index = (uint8_t)((top + row) % count);

        draw_row(row, name(index), index == selection);
    }
    draw_scrollbar(selection, count);
    draw_softkeys(left, text_back);
}

/* The hint for the full-screen variant, in the space under the phone's LCD,
   and the port's version in the bottom right corner of the screen. */
static void draw_hint(void)
{
#if LCD_HAS_SURROUND
    lcd_view_full();
    font_draw(&font_small_plain, (LCD_FB_WIDTH - font_text_width(&font_small_plain, text_full_screen_hint)) / 2,
              LCD_PHONE_Y + LCD_HEIGHT + 8, text_full_screen_hint, 1);
    font_draw(&font_small_plain, LCD_FB_WIDTH - font_text_width(&font_small_plain, GAME_VERSION) - 2,
              LCD_FB_HEIGHT - font_small_plain.height - 2, GAME_VERSION, 1);
    lcd_view_phone();
    surround_used = 1;
#endif
}

static void draw_main(void)
{
    draw_hint();
    draw_header(text_menu, GAMES_MENU_NUMBER);
    font_draw(&font_large_bold, (CONTENT_WIDTH - font_text_width(&font_large_bold, text_games)) / 2, TITLE_Y, text_games, 1);
    draw_scrollbar(GAMES_MENU_NUMBER - 1, GAMES_MENU_ENTRIES);
    draw_softkeys(text_select, text_exit);
}

/* `count` bars, those up to `chosen` filled. */
static void draw_level(uint8_t count, uint8_t chosen)
{
    uint8_t i, x, top;

    font_draw(&font_medium_bold, LEVEL_TEXT_X, 0, text_level, 1);
    for (i = 0; i < count; i++) {
        x = (uint8_t)(LEVEL_BAR_X + i * LEVEL_BAR_PITCH);
        /* The first bar's line is a row taller than the rule gives. */
        top = i ? (uint8_t)(LEVEL_BAR_BOTTOM - 1 - 3 * i) : LEVEL_BAR_BOTTOM - 2;
        if (i <= chosen)
            lcd_fill_rect(x, top - 1, LEVEL_BAR_WIDTH, LEVEL_BAR_BOTTOM + 1 - top, 1);
        lcd_fill_rect(x + LEVEL_BAR_WIDTH + 1, top, 1, LEVEL_BAR_BOTTOM + 1 - top, 1);
        lcd_fill_rect(x + 1, LEVEL_BAR_BOTTOM + 1, LEVEL_BAR_WIDTH + 1, 1, 1);
    }
    draw_softkeys(text_ok, text_back);
}

static void draw_number(const struct font *font, int x, int y, uint16_t value, uint8_t centred, int width)
{
    char digits[6];
    uint8_t n = sizeof digits - 1;

    digits[n] = 0;
    do {
        digits[--n] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    if (centred)
        x = (width - font_text_width(font, digits + n)) / 2;
    font_draw(font, x, y, digits + n, 1);
}

/* Breaks `text` into lines that fit `limit` pixels of `font`, drawing
   `lines` of them from row y, `pitch` apart. */
static const char *draw_wrapped(const struct font *font, int x, int y, int pitch, uint8_t lines, uint16_t limit,
                                const char *text)
{
    uint8_t row;

    for (row = 0; row < lines && *text; row++) {
        const char *end = text, *p = text;
        uint16_t width = 0;
        int at = x;

        for (;;) {
            while (*p && *p != ' ' && *p != '\n')
                width += font_char_width(font, *p++);
            if (width > limit && end != text)
                break;
            end = p;
            if (!*p || *p == '\n')
                break;
            width += font_char_width(font, *p++);
        }
        for (p = text; p != end; p++) {
            char one[2];

            one[0] = *p;
            one[1] = 0;
            at = font_draw(font, at, y + row * pitch, one, 1);
        }
        text = *end ? end + 1 : end;
    }
    return text;
}

static void native_title(const char *title)
{
    lcd_fill_rect(0, 0, LCD_FB_WIDTH, NATIVE_TITLE_HEIGHT, 1);
    font_draw(&NATIVE_FONT, NATIVE_MARGIN, 1, title, 0);
}

static void native_hint(const char *hint)
{
    lcd_fill_rect(0, NATIVE_HINT_Y - 3, LCD_FB_WIDTH, 1, 1);
    font_draw(&NATIVE_FONT, NATIVE_MARGIN, NATIVE_HINT_Y, hint, 1);
}

static void native_cursor(uint8_t row)
{
    uint8_t y = (uint8_t)(NATIVE_LIST_Y + row * NATIVE_ROW_HEIGHT), i;

    for (i = 0; i < NATIVE_CURSOR_WIDTH; i++)
        lcd_fill_rect(NATIVE_MARGIN + i, y + 2 + i, 1, NATIVE_CURSOR_HEIGHT - 2 * i, 1);
}

static void native_row(uint8_t row, const char *label, uint8_t selected)
{
    font_draw(&NATIVE_FONT, NATIVE_TEXT_X, NATIVE_LIST_Y + row * NATIVE_ROW_HEIGHT + 1, label, 1);
    if (selected)
        native_cursor(row);
}

static void native_list(const char *title, uint8_t count, uint8_t selection, const char *(*name)(uint8_t))
{
    uint8_t i;

    native_title(title);
    for (i = 0; i < count; i++)
        native_row(i, name(i), i == selection);
    native_hint(text_hint_select);
}

/* A note: its lines centred on the screen; %N is the number. */
static void native_note(const char *title, const char *text, uint16_t number)
{
    uint8_t y = NATIVE_NOTE_Y;

    if (title)
        native_title(title);
    for (;;) {
        if (text[0] == '%' && text[1] == 'N')
            draw_number(&NATIVE_FONT, 0, y, number, 1, LCD_FB_WIDTH);
        else
            font_draw(&NATIVE_FONT, (LCD_FB_WIDTH - font_text_width(&NATIVE_FONT, text)) / 2, y, text, 1);
        while (*text && *text != '\n')
            text++;
        if (!*text++)
            break;
        y += NATIVE_NOTE_PITCH;
    }
}

static const char *item_name_at(uint8_t index)
{
    return item_name(item_id(index));
}

static const char *select_name(uint8_t index)
{
    return index < GAME_COUNT ? game_name(index) : text_about;
}

static void native_settings(void)
{
    uint8_t i;

    native_title(text_settings);
    for (i = 0; i < SETTING_COUNT; i++) {
        const char *shown = setting_value(i);

        native_row(i, setting_name(i), i == setting);
        font_draw(&NATIVE_FONT, LCD_FB_WIDTH - NATIVE_MARGIN - font_text_width(&NATIVE_FONT, shown),
                  NATIVE_LIST_Y + i * NATIVE_ROW_HEIGHT + 1, shown, 1);
    }
    native_hint(text_hint_select);
}

void menu_init(void)
{
    screen = SCREEN_MAIN;
    games_item = game = game_top = 0;
    paused = 0;
    full_screen = 0;
    held = NO_KEY;
    games_quiet();
    platform_options_load(&games_options);
}

/* Opens the game's menu on its first entry: Continue when a game is
   paused, else New game. */
static void game_menu_open(void)
{
    screen = SCREEN_GAME;
    item = item_top = 0;
    settings_load();
}

static void play_over(void)
{
    final_score = (uint16_t)games_score;
    new_top_score = final_score > top_score;
    if (new_top_score) {
        struct game_settings maze_record = { 0, 0, 0 };

        top_score = maze_record.top_score = final_score;
        platform_settings_save(top_score_slot(), &maze_record);
    }
    last_played = 1;
    last_score = final_score;
    last_maze = settings.option;
    last_full = full_screen;
    paused = 0;
    held = NO_KEY;
    screen = SCREEN_GAME_OVER;
    board_drawn = 0;
    over_start(final_score, new_top_score);
    /* 0x24b77e */
    if (games_options.sounds)
        sound_play(new_top_score ? SNAKE2_SOUND_TOP_SCORE : SNAKE2_SOUND_GAME_OVER);
}

static void play_start(void)
{
    /* The phone's generator runs on from power-on; here the time of the
       choice seeds it, so that games differ. */
    if (!seed_fixed)
        game_srand((uint32_t)uptime + 1);
    held = NO_KEY;
    snake2_full = full_screen;
    games_start(GAME_SNAKE, (uint8_t)(settings.level + 1), (uint8_t)(settings.option + 1));
    board_drawn = 0;
    screen = SCREEN_PLAY;
}

/* The phone key a button is in the game, or 0: the pad steers, A turns
   clockwise and B anticlockwise, as # and * do on the phone. */
static uint8_t play_phone_key(uint8_t key)
{
    switch (key) {
    case MENU_KEY_UP:
        return GAME_KEY_2;
    case MENU_KEY_DOWN:
        return GAME_KEY_8;
    case MENU_KEY_LEFT:
        return GAME_KEY_4;
    case MENU_KEY_RIGHT:
        return GAME_KEY_6;
    case MENU_KEY_SELECT:
        return GAME_KEY_HASH;
    case MENU_KEY_BACK:
        return GAME_KEY_STAR;
    default:
        return 0;
    }
}

static uint8_t play_key(uint8_t key)
{
    uint8_t phone_key = play_phone_key(key), changed;

    if (!phone_key) {
        /* Start and Select pause into the game's menu, as the phone's Navi
           key does. */
        games_key_up();
        held = NO_KEY;
        paused = 1;
        game_menu_open();
        return 1;
    }
    held = key;
    changed = games_key_down(phone_key);
    if (games_over)
        play_over();
    return changed;
}

uint8_t menu_held(uint8_t keys)
{
    uint8_t key, changed = 0;

    if (screen == SCREEN_SI_PLAY)
        return si_play_held(keys);
    if (screen != SCREEN_PLAY || held == NO_KEY || (keys & (1 << held)))
        return 0;
    /* The held button came up; another one still down takes over. */
    games_key_up();
    held = NO_KEY;
    for (key = MENU_KEY_UP; key <= MENU_KEY_RIGHT && held == NO_KEY; key++) {
        if ((keys & (1 << key)) && play_phone_key(key)) {
            held = key;
            changed = games_key_down(play_phone_key(key));
        }
    }
    if (games_over) {
        play_over();
        return 1;
    }
    return changed;
}

static void game_menu_select(void)
{
    switch (item_id(item)) {
    case ITEM_CONTINUE:
        paused = 0;
        board_drawn = 0;
        screen = SCREEN_PLAY;
        games_continue();
        break;
    case ITEM_NEW_GAME:
        paused = 0;
        play_start();
        break;
    case ITEM_HIGH_SCORES:
        /* The creature the snake eats is one of six, at random. */
        screen = SCREEN_HIGH_SCORES;
        board_drawn = 0;
        scores_start(top_score, last_score,
                     (uint8_t)(last_played && last_maze == settings.option && last_full == full_screen),
                     (uint8_t)((unsigned)game_rand() % 6));
        break;
    case ITEM_OPTIONS:
        screen = SCREEN_OPTIONS;
        option = 0;
        break;
    default:
        screen = SCREEN_HELP;
        help_page = 0;
        break;
    }
}

/* Space Impact, driven as the phone's games application drives it: a tick
   every period the game asks for, counted in the phone's timer units, and
   its keys as they go down and come up, the ones held polled by the game
   (si.h). The pad flies the ship, A fires and B fires the special weapon,
   as 8, 0, * and #, 1 and 4 do on the phone; Start and Select pause. */
#define SI_NO_CODE 0xff

static uint16_t si_period_units(void)
{
    /* The last one worked out, kept: the Game Boy divides slowly. */
    static uint16_t period, units;

    if (si_period != period) {
        period = si_period;
        units = (uint16_t)(period / 255 * 32 + period % 255 * 32 / 255);
    }
    return units;
}

static uint8_t si_item_count(void)
{
    return (uint8_t)(4 + si_paused);
}

static uint8_t si_item_id(uint8_t index)
{
    return si_paused ? index : (uint8_t)(index + 1);
}

static const char *si_item_name(uint8_t index)
{
    switch (si_item_id(index)) {
    case SI_ITEM_CONTINUE:
        return text_continue;
    case SI_ITEM_NEW_GAME:
        return text_new_game;
    case SI_ITEM_HIGH_SCORES:
        return text_high_scores;
    case SI_ITEM_CHAPTERS:
        return text_chapters;
    default:
        return text_instructions;
    }
}

/* The one set of chapters, the phone's own. */
static const char *si_chapter_name(uint8_t index)
{
    (void)index;
    return text_genevas_world;
}

static const char *si_help_text(uint8_t which)
{
    return which == 0 ? text_help_si_1 : which == 1 ? text_help_si_2 : which == 2 ? text_help_si_3 : text_help_si_4;
}

/* The Instructions: four texts, a page at a time, the first three each
   followed by its demo, which leads on to the next text (measured in
   MAME: the framework around the game is not traced). */
static void si_help_open(uint8_t which)
{
    screen = SCREEN_SI_HELP;
    si_help = which;
    si_help_pageno = 0;
    si_help_page = si_help_pages[0] = help_load(si_help_text(which));
}

static void si_menu_open(void)
{
    screen = SCREEN_SI_MENU;
    si_item = si_item_top = 0;
}

/* The game asked to be closed: its title ended, its High scores page was
   left, or a game ended after its game-over picture. */
static void si_closed(void)
{
    si_units = 0;
    board_drawn = 0;
    if (screen == SCREEN_SI_DEMO) {
        si_help_open((uint8_t)(si_help + 1));
        return;
    }
    if (screen == SCREEN_SI_PLAY) {
        struct game_settings record = { 0, 0, 0 };

        si_last = si_score();
        si_last_played = 1;
        if (si_last > si_top) {
            si_top = record.top_score = si_last;
            platform_settings_save(GAME_SPACE_IMPACT, &record);
        }
        si_paused = 0;
        si_menu_open();
        return;
    }
    if (screen == SCREEN_SI_TITLE) {
        si_menu_open();
        return;
    }
    screen = SCREEN_SI_MENU;
}

/* Calls the game, then carries out what it asked of the buzzer and the
   vibrator. The vibrator runs for the three ticks the game counts it
   down over, on games.c's timer, as the phone's own timer would keep it
   going through a pause. */
static uint8_t si_call(uint8_t event, uint8_t a)
{
    uint8_t done = si_event(event, a);

    if (si_sound) {
        if (games_options.sounds)
            sound_play(si_sound);
        si_sound = 0;
    }
    if (si_vibrate == SI_VIBRATE_ON)
        games_vibrate_for((uint8_t)(3 * si_period_units()));
    else if (si_vibrate == SI_VIBRATE_OFF)
        games_quiet();
    si_vibrate = 0;
    return done;
}

static uint8_t si_send(uint8_t event, uint8_t a)
{
    uint8_t done = si_call(event, a);

    if (done & SI_DONE_CLOSE) {
        si_closed();
        return 1;
    }
    return (uint8_t)(done & SI_DONE_REDRAW);
}

/* Starts something the game draws, with its timer. */
static void si_open(uint8_t which, uint8_t event, uint8_t a)
{
    screen = which;
    board_drawn = 0;
    si_call(event, a);
    si_units = si_period_units();
}

static uint8_t si_elapse(void)
{
    uint8_t changed = 0, done;

    for (si_us += MENU_FRAME_US; si_us >= GAMES_UNIT_US; si_us -= GAMES_UNIT_US) {
        if (!si_units || --si_units)
            continue;
        done = si_call(SI_EVENT_TICK, 0);
        si_demo_waiting = 0;
        si_units = si_period_units();
        if (done & SI_DONE_CLOSE) {
            si_closed();
            return 1;
        }
        changed |= done;
    }
    return (uint8_t)(changed & SI_DONE_REDRAW);
}

static uint8_t si_code(uint8_t key)
{
    switch (key) {
    case MENU_KEY_UP:
        return 8;
    case MENU_KEY_DOWN:
        return 0;
    case MENU_KEY_LEFT:
        return SI_KEY_STAR;
    case MENU_KEY_RIGHT:
        return SI_KEY_HASH;
    case MENU_KEY_SELECT:
        return 1;
    case MENU_KEY_BACK:
        return 4;
    default:
        return SI_NO_CODE;
    }
}

static uint8_t si_play_key(uint8_t key)
{
    uint8_t code = si_code(key);

    if (code == SI_NO_CODE) {
        /* Pause: the game saves itself and closes, as on the phone. */
        si_call(SI_EVENT_PAUSE, 0);
        si_keys_held = 0;
        si_held = 0;
        si_units = 0;
        si_paused = 1;
        board_drawn = 0;
        si_menu_open();
        return 1;
    }
    si_held |= (uint8_t)(1 << key);
    si_keys_held |= (uint16_t)(1u << code);
    return si_send(SI_EVENT_KEY_DOWN, code);
}

static uint8_t si_play_held(uint8_t keys)
{
    uint8_t key, changed = 0;

    for (key = 0; key < 8; key++) {
        uint8_t code;

        if (!(si_held >> key & 1) || (keys >> key & 1))
            continue;
        si_held &= (uint8_t)~(1 << key);
        code = si_code(key);
        si_keys_held &= (uint16_t)~(1u << code);
        changed |= si_send(SI_EVENT_KEY_UP, code);
        if (screen != SCREEN_SI_PLAY)
            break;
    }
    return changed;
}

static void si_menu_select(void)
{
    si_top_score = si_top;
    switch (si_item_id(si_item)) {
    case SI_ITEM_CONTINUE:
        /* As the phone does: a new game, then the state it saved. */
        si_paused = 0;
        si_call(SI_EVENT_NEW_GAME, 0);
        si_open(SCREEN_SI_PLAY, SI_EVENT_CONTINUE, 0);
        break;
    case SI_ITEM_NEW_GAME:
        si_paused = 0;
        si_open(SCREEN_SI_PLAY, SI_EVENT_NEW_GAME, 0);
        break;
    case SI_ITEM_HIGH_SCORES:
        si_last_score = si_last;
        si_show_last = si_last_played;
        si_open(SCREEN_SI_SCORES, SI_EVENT_HIGH_SCORES, 0);
        break;
    case SI_ITEM_CHAPTERS:
        screen = SCREEN_SI_CHAPTERS;
        break;
    default:
        si_help_open(0);
        break;
    }
}

/* The page after the one shown: the rest of its text, or "". */
static const char *si_help_next;

/* Up: the page before, within the text. */
static void si_help_back(void)
{
    if (si_help_pageno)
        si_help_page = si_help_pages[--si_help_pageno];
}

/* More or Down: the next page, or after the last one the text's demo
   (event 9 with its number); the last text has none. */
static void si_help_forward(void)
{
    if (*si_help_next) {
        if (si_help_pageno < SI_HELP_PAGES - 1)
            si_help_pages[++si_help_pageno] = si_help_next;
        si_help_page = si_help_next;
        return;
    }
    if (si_help < SI_HELP_TEXTS - 1) {
        si_keys_held = 0;
        si_demo_waiting = 1;
        si_open(SCREEN_SI_DEMO, SI_EVENT_DEMO, (uint8_t)(si_help + 1));
    }
}

/* Moves a selection through a list of `count` that shows three rows; the
   window follows the selection, wrapping with it. */
static void list_move(uint8_t key, uint8_t count, uint8_t *selection, uint8_t *top)
{
    if (key == MENU_KEY_DOWN) {
        *selection = (uint8_t)((*selection + 1) % count);
        if ((uint8_t)((*selection + count - *top) % count) >= VISIBLE_ROWS)
            *top = (uint8_t)((*selection + count - (VISIBLE_ROWS - 1)) % count);
    } else if (key == MENU_KEY_UP) {
        *selection = (uint8_t)((*selection + count - 1) % count);
        if ((uint8_t)((*selection + count - *top) % count) >= VISIBLE_ROWS)
            *top = *selection;
    }
}

/* Lists of no more than three rows: the window never moves. */
static void short_list_move(uint8_t key, uint8_t count, uint8_t *selection)
{
    uint8_t top = 0;

    list_move(key, count, selection, &top);
}

/* Bantumi, driven by bantumi_play.h. Left and right move the hand, A
   sows, B asks for a hint and Up hands the first move to the phone, as 4,
   6, 5, * and # do on the phone; Start and Select pause. */
static uint8_t bt_item_count(void)
{
    return (uint8_t)(3 + bt_paused);
}

static uint8_t bt_item_id(uint8_t index)
{
    return bt_paused ? index : (uint8_t)(index + 1);
}

static const char *bt_item_name(uint8_t index)
{
    switch (bt_item_id(index)) {
    case BT_ITEM_CONTINUE:
        return text_continue;
    case BT_ITEM_NEW_GAME:
        return text_new_game;
    case BT_ITEM_OPTIONS:
        return text_options;
    default:
        return text_instructions;
    }
}

static const char *bt_option_name(uint8_t index)
{
    return index ? text_level : text_no_of_beans;
}

static const char *bt_beans_name(uint8_t index)
{
    return index == 0 ? text_beans_3 : index == 1 ? text_beans_4 : text_beans_5;
}

/* The beans a pit starts with: the record keeps 3 to 5 less 2, 0 for none
   chosen yet. */
static uint8_t bt_beans_count(void)
{
    return bt_settings.option ? (uint8_t)(bt_settings.option + 2) : 4;
}

static const char *bt_help_text(uint8_t which)
{
    static const char *const texts[BT_HELP_TEXTS] = {
        text_help_bantumi_1, text_help_bantumi_2, text_help_bantumi_3, text_help_bantumi_4, text_help_bantumi_5
    };

    return texts[which];
}

static void bt_menu_open(void)
{
    screen = SCREEN_BT_MENU;
    bt_item = bt_item_top = 0;
}

static void bt_help_open(uint8_t which)
{
    screen = SCREEN_BT_HELP;
    bt_help = which;
    si_help_pageno = 0;
    si_help_page = si_help_pages[0] = help_load(bt_help_text(which));
}

/* After a demo: the next text, after the last the first again (seen in
   MAME). */
static void bt_help_next(void)
{
    bt_help_open(bt_help < BT_HELP_TEXTS - 1 ? (uint8_t)(bt_help + 1) : 0);
}

/* The game asked to be closed: its title ended, a game ended, or a demo
   reached the phone's turn. */
static void bt_closed(void)
{
    board_drawn = 0;
    if (screen == SCREEN_BT_DEMO) {
        bt_help_next();
        return;
    }
    if (screen == SCREEN_BT_PLAY)
        bt_paused = 0;
    bt_menu_open();
}

static uint8_t bt_done(uint8_t done)
{
    if (done & BANTUMI_PLAY_CLOSED)
        bt_closed();
    return 1;
}

/* Starts something the game draws, with its timer. */
static void bt_open(uint8_t which, uint8_t event, uint8_t a)
{
    screen = which;
    board_drawn = 0;
    bantumi_level = (uint8_t)(bt_settings.level + 1);
    bantumi_beans = bt_beans_count();
    bt_done(bantumi_play_open(event, a));
}

static uint8_t bt_elapse(void)
{
    uint8_t done = bantumi_play_elapse(MENU_FRAME_US);

    if (done & BANTUMI_PLAY_TICKED)
        bt_demo_waiting = 0;
    return done ? bt_done(done) : 0;
}

static uint8_t bt_play_key(uint8_t key)
{
    static const uint8_t codes[] = { BANTUMI_KEY_HASH, 0xff, 5, BANTUMI_KEY_STAR, 4, 6 };
    uint8_t code = key < sizeof codes ? codes[key] : 0xfe;

    if (code == 0xff)
        return 0;
    if (code == 0xfe) {
        /* Pause: the game puts itself away, as on the phone. */
        bantumi_play_call(BANTUMI_EVENT_PAUSE, 0);
        bantumi_play_stop(0);
        bt_paused = 1;
        board_drawn = 0;
        bt_menu_open();
        return 1;
    }
    return bt_done(bantumi_play_key(code));
}

static void bt_menu_select(void)
{
    switch (bt_item_id(bt_item)) {
    case BT_ITEM_CONTINUE:
        /* As the phone does: a new game, then the game it put away, which
           stands still until a key. */
        bt_paused = 0;
        bantumi_play_call(BANTUMI_EVENT_NEW_GAME, 1);
        bt_open(SCREEN_BT_PLAY, BANTUMI_EVENT_CONTINUE, 0);
        bantumi_play_stop(1);
        break;
    case BT_ITEM_NEW_GAME:
        bt_paused = 0;
        bt_open(SCREEN_BT_PLAY, BANTUMI_EVENT_NEW_GAME, 1);
        bt_done(bantumi_play_open(BANTUMI_EVENT_START, 0));
        break;
    case BT_ITEM_OPTIONS:
        screen = SCREEN_BT_OPTIONS;
        bt_option = 0;
        break;
    default:
        bt_help_open(0);
        break;
    }
}

/* More or Down: the next page, or after the last one the text's demo. */
static void bt_help_forward(void)
{
    if (*si_help_next) {
        if (si_help_pageno < SI_HELP_PAGES - 1)
            si_help_pages[++si_help_pageno] = si_help_next;
        si_help_page = si_help_next;
        return;
    }
    bt_demo_waiting = 1;
    bt_open(SCREEN_BT_DEMO, BANTUMI_EVENT_DEMO, (uint8_t)(bt_help + 1));
}

static uint8_t select_count(void)
{
    return full_screen ? GAME_COUNT + 1 : GAME_COUNT;
}

static uint8_t handle_key(uint8_t key)
{
    /* Start picks the full-screen variant on the first screen, where there
       is one. Outside the game it and the console's Select button are more
       Navi keys. */
    uint8_t start = key == MENU_KEY_START;

    if (screen == SCREEN_PLAY)
        return play_key(key);
    if (screen == SCREEN_SI_PLAY)
        return si_play_key(key);
    if (screen == SCREEN_BT_PLAY)
        return bt_play_key(key);
    if (screen == SCREEN_BT_DEMO) {
        /* As in Space Impact's: any key ends a demo, C back to the menu. */
        bantumi_play_call(BANTUMI_EVENT_PAUSE, 0);
        bantumi_play_stop(0);
        if (key == MENU_KEY_BACK)
            bt_menu_open();
        else
            bt_help_next();
        return 1;
    }
    if (screen == SCREEN_SI_DEMO) {
        /* Any key ends a demo, as a pause; C goes back to the game's menu,
           the others on to the next text. */
        si_call(SI_EVENT_PAUSE, 0);
        si_units = 0;
        if (key == MENU_KEY_BACK)
            screen = SCREEN_SI_MENU;
        else
            si_help_open((uint8_t)(si_help + 1));
        return 1;
    }
    if (start || key == MENU_KEY_ALT)
        key = MENU_KEY_SELECT;
    switch (screen) {
    case SCREEN_SI_TITLE:
        /* A key ends the title: C back to the list, any other into the
           game's menu. */
        si_send(SI_EVENT_KEY_DOWN, si_code(key));
        if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        else
            si_menu_open();
        return 1;
    case SCREEN_SI_SCORES:
        si_send(SI_EVENT_KEY_DOWN, si_code(key));
        screen = SCREEN_SI_MENU;
        return 1;
    case SCREEN_BT_TITLE:
        /* A key ends the title: C back to the list, any other into the
           game's menu. */
        bantumi_play_call(BANTUMI_EVENT_PAUSE, 0);
        bantumi_play_stop(0);
        if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        else
            bt_menu_open();
        return 1;
    case SCREEN_BT_MENU:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP)
            list_move(key, bt_item_count(), &bt_item, &bt_item_top);
        else if (key == MENU_KEY_SELECT)
            bt_menu_select();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        break;
    case SCREEN_BT_OPTIONS:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            short_list_move(key, 2, &bt_option);
        } else if (key == MENU_KEY_SELECT) {
            if (bt_option == 0) {
                screen = SCREEN_BT_BEANS;
                bt_beans = (uint8_t)(bt_beans_count() - 3);
            } else {
                screen = SCREEN_BT_LEVEL;
                bt_level = bt_settings.level;
            }
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_BT_MENU;
        }
        break;
    case SCREEN_BT_BEANS:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            short_list_move(key, 3, &bt_beans);
        } else if (key == MENU_KEY_SELECT) {
            bt_settings.option = (uint8_t)(bt_beans + 1);
            platform_settings_save(GAME_BANTUMI, &bt_settings);
            screen = SCREEN_BT_OPTIONS;
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_BT_OPTIONS;
        }
        break;
    case SCREEN_BT_LEVEL:
        if (key == MENU_KEY_UP && bt_level < BANTUMI_LEVELS - 1) {
            bt_level++;
        } else if (key == MENU_KEY_DOWN && bt_level > 0) {
            bt_level--;
        } else if (key == MENU_KEY_SELECT) {
            bt_settings.level = bt_level;
            platform_settings_save(GAME_BANTUMI, &bt_settings);
            screen = SCREEN_BT_OPTIONS;
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_BT_OPTIONS;
        }
        return 1;
    case SCREEN_BT_HELP:
        if (key == MENU_KEY_SELECT || key == MENU_KEY_DOWN)
            bt_help_forward();
        else if (key == MENU_KEY_UP)
            si_help_back();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_BT_MENU;
        return 1;
    case SCREEN_SI_MENU:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP)
            list_move(key, si_item_count(), &si_item, &si_item_top);
        else if (key == MENU_KEY_SELECT)
            si_menu_select();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        break;
    case SCREEN_SI_CHAPTERS:
        if (key == MENU_KEY_SELECT) {
            screen = SCREEN_SI_DONE;
            si_done_frames = SI_DONE_FRAMES;
        } else if (key == MENU_KEY_BACK)
            screen = SCREEN_SI_MENU;
        break;
    case SCREEN_SI_DONE:
        screen = SCREEN_SI_MENU;
        break;
    case SCREEN_SI_HELP:
        if ((key == MENU_KEY_SELECT && si_help < SI_HELP_TEXTS - 1) || key == MENU_KEY_DOWN)
            si_help_forward();
        else if (key == MENU_KEY_UP)
            si_help_back();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_SI_MENU;
        return 1;
    case SCREEN_MAIN:
        if (key == MENU_KEY_SELECT) {
            full_screen = start && LCD_HAS_SURROUND;
            screen = SCREEN_GAMES;
            games_item = 0;
        }
        break;
    case SCREEN_GAMES:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            short_list_move(key, GAMES_COUNT, &games_item);
        } else if (key == MENU_KEY_SELECT) {
            if (games_item == GAMES_SELECT) {
                screen = SCREEN_SELECT;
                game = game_top = 0;
            } else {
                screen = SCREEN_SETTINGS;
                setting = 0;
            }
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_MAIN;
        }
        break;
    case SCREEN_SELECT:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            list_move(key, select_count(), &game, &game_top);
        } else if (key == MENU_KEY_SELECT) {
            if (game == GAME_SPACE_IMPACT) {
                /* The title seeds the generator from the phone's clock: a
                   scripted run's from the clock MAME gives it. */
                struct game_settings record;

                game_srand(seed_fixed ? 1 : (uint32_t)uptime + 1);
                if (!platform_settings_load(GAME_SPACE_IMPACT, &record))
                    record.top_score = 0;
                si_top = record.top_score;
                si_paused = 0;
                si_open(SCREEN_SI_TITLE, SI_EVENT_TITLE, 0);
            } else if (game == GAME_BANTUMI) {
                if (!platform_settings_load(GAME_BANTUMI, &bt_settings))
                    bt_settings.level = bt_settings.option = 0;
                bt_paused = 0;
                bt_open(SCREEN_BT_TITLE, BANTUMI_EVENT_TITLE, 0);
            } else if (game == GAME_SNAKE) {
                paused = 0;
                screen = SCREEN_TITLE;
                board_drawn = 0;
                title_start();
            } else if (game == GAME_COUNT) {
                screen = SCREEN_ABOUT;
            }
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_GAMES;
        }
        break;
    case SCREEN_ABOUT:
        screen = SCREEN_SELECT;
        break;
    case SCREEN_TITLE:
        /* A key ends the title: C back to the list, any other into the
           game's menu. */
        if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        else
            game_menu_open();
        break;
    case SCREEN_SETTINGS:
        if (key == MENU_KEY_DOWN) {
            setting = (uint8_t)((setting + 1) % SETTING_COUNT);
        } else if (key == MENU_KEY_UP) {
            setting = (uint8_t)((setting + SETTING_COUNT - 1) % SETTING_COUNT);
        } else if (key == MENU_KEY_SELECT && setting_switch(setting)) {
            if (full_screen) {
                *setting_switch(setting) ^= 1;
                platform_options_save(&games_options);
                return 1;
            }
            screen = SCREEN_SETTING_VALUE;
            value = *setting_switch(setting);
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_GAMES;
        }
        break;
    case SCREEN_SETTING_VALUE:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            value ^= 1;
        } else if (key == MENU_KEY_SELECT) {
            *setting_switch(setting) = value;
            platform_options_save(&games_options);
            screen = SCREEN_SETTINGS;
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_SETTINGS;
        }
        break;
    case SCREEN_OPTIONS:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            short_list_move(key, OPTION_COUNT, &option);
        } else if (key == MENU_KEY_SELECT) {
            if (option == OPTION_MAZES) {
                screen = SCREEN_MAZES;
                maze = maze_top = settings.option;
            } else {
                screen = SCREEN_LEVEL;
                level = settings.level;
            }
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_GAME;
        }
        break;
    case SCREEN_LEVEL:
        /* Up and down change the level by one, stopping at the ends; OK
           keeps it. */
        if (key == MENU_KEY_UP && level < SNAKE2_LEVELS - 1) {
            level++;
        } else if (key == MENU_KEY_DOWN && level > 0) {
            level--;
        } else if (key == MENU_KEY_SELECT) {
            settings.level = level;
            platform_settings_save(GAME_SNAKE, &settings);
            screen = SCREEN_OPTIONS;
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_OPTIONS;
        }
        return 1;
    case SCREEN_MAZES:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP) {
            list_move(key, SNAKE2_MAZE_COUNT, &maze, &maze_top);
        } else if (key == MENU_KEY_SELECT) {
            settings.option = maze;
            platform_settings_save(GAME_SNAKE, &settings);
            top_score_load();
            screen = SCREEN_OPTIONS;
        } else if (key == MENU_KEY_BACK) {
            screen = SCREEN_OPTIONS;
        }
        break;
    case SCREEN_HELP:
        if (key == MENU_KEY_SELECT)
            help_page = (uint8_t)((help_page + 1) % HELP_PAGES);
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_GAME;
        break;
    case SCREEN_HIGH_SCORES:
        screen = SCREEN_GAME;
        break;
    case SCREEN_GAME_OVER:
        /* The phone's Navi key cuts the picture short. */
        if (key == MENU_KEY_SELECT)
            game_menu_open();
        break;
    default:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP)
            list_move(key, item_count(), &item, &item_top);
        else if (key == MENU_KEY_SELECT)
            game_menu_select();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_SELECT;
        break;
    }
    return 0;
}

uint8_t menu_key(uint8_t key)
{
    uint8_t was_screen = screen, was_games = games_item, was_game = game, was_item = item, was_top = item_top;
    uint8_t was_setting = setting, was_value = value, was_maze = maze, was_option = option, was_page = help_page;
    uint8_t was_si_item = si_item, was_si_top = si_item_top;
    uint8_t was_bt_item = bt_item, was_bt_top = bt_item_top, was_bt_option = bt_option, was_beans = bt_beans;
    uint8_t changed;

    changed = handle_key(key);
    return changed || screen != was_screen || games_item != was_games || game != was_game || item != was_item
           || item_top != was_top || setting != was_setting || value != was_value || maze != was_maze
           || option != was_option || help_page != was_page || si_item != was_si_item || si_item_top != was_si_top
           || bt_item != was_bt_item || bt_item_top != was_bt_top || bt_option != was_bt_option || bt_beans != was_beans;
}

void menu_seed(uint16_t seed)
{
    game_srand(seed);
    seed_fixed = 1;
}

void menu_redraw_all(void)
{
    board_drawn = 0;
}

uint8_t menu_tick(void)
{
    uint8_t changed = 0;

    uptime++;
    games_rumble_elapse(MENU_FRAME_US);
    switch (screen) {
    case SCREEN_SI_TITLE:
    case SCREEN_SI_PLAY:
    case SCREEN_SI_DEMO:
    case SCREEN_SI_SCORES:
        changed = si_elapse();
        break;
    case SCREEN_BT_TITLE:
    case SCREEN_BT_PLAY:
    case SCREEN_BT_DEMO:
        changed = bt_elapse();
        break;
    case SCREEN_SI_DONE:
        if (!--si_done_frames) {
            screen = SCREEN_SI_MENU;
            changed = 1;
        }
        break;
    case SCREEN_HIGH_SCORES:
        changed = scores_elapse(MENU_FRAME_US);
        break;
    case SCREEN_TITLE: {
        uint8_t what = title_elapse(MENU_FRAME_US);

        if (what & TITLE_OVER) {
            game_menu_open();
            return 1;
        }
        changed = what;
        break;
    }
    case SCREEN_GAME_OVER: {
        uint8_t what = over_elapse(MENU_FRAME_US);

        if (what & TITLE_OVER) {
            game_menu_open();
            return 1;
        }
        changed = what;
        break;
    }
    case SCREEN_PLAY:
        changed = games_elapse(MENU_FRAME_US);
        if (games_over) {
            play_over();
            return 1;
        }
        break;
    default:
        break;
    }
    return changed;
}

static void draw_phone(void)
{
    switch (screen) {
    case SCREEN_SI_MENU:
        draw_list(text_space_impact, si_item_count(), si_item, si_item_top, si_item_name, text_select);
        break;
    case SCREEN_SI_CHAPTERS:
        draw_list(text_chapters, 1, 0, 0, si_chapter_name, text_select);
        break;
    case SCREEN_SI_DONE:
        /* The phone's tick beside it is not drawn yet. */
        font_draw(&font_large_bold, 0, 3, text_done, 1);
        break;
    case SCREEN_SI_HELP:
        si_help_next = draw_wrapped(&font_small_bold, 0, LIST_Y, HELP_LINE_HEIGHT, SI_HELP_LINES, LCD_WIDTH, si_help_page);
        if (si_help < SI_HELP_TEXTS - 1)
            draw_softkeys(text_more, text_back);
        else
            draw_softkeys("", text_exit);
        break;
    case SCREEN_MAIN:
        draw_main();
        break;
    case SCREEN_GAMES:
        draw_list(text_games, GAMES_COUNT, games_item, 0, games_name, text_select);
        break;
    case SCREEN_SELECT:
        draw_list(text_games, GAME_COUNT, game, game_top, game_name, text_select);
        break;
    case SCREEN_SETTINGS:
        draw_header(text_settings, (uint8_t)(setting + 1));
        font_draw(&font_large_bold, ROW_TEXT_X, LIST_Y + ROW_TEXT_DY, setting_name(setting), 1);
        font_draw(&font_large_bold, CONTENT_WIDTH - 2 - font_text_width(&font_large_bold, setting_value(setting)),
                  LIST_Y + ROW_HEIGHT * 2 + ROW_TEXT_DY, setting_value(setting), 1);
        draw_scrollbar(setting, SETTING_COUNT);
        draw_softkeys(text_select, text_back);
        break;
    case SCREEN_SETTING_VALUE:
        draw_header(setting_name(setting), (uint8_t)(value + 1));
        draw_row(0, text_off, value == 0);
        draw_row(1, text_on, value == 1);
        draw_scrollbar(value, 2);
        draw_softkeys(text_ok, text_back);
        break;
    case SCREEN_GAME:
        draw_list(text_snake, item_count(), item, item_top, item_name_at, text_select);
        break;
    case SCREEN_OPTIONS:
        draw_list(text_game_options, OPTION_COUNT, option, 0, option_name, text_select);
        break;
    case SCREEN_MAZES:
        draw_list(text_mazes, SNAKE2_MAZE_COUNT, maze, maze_top, maze_name, text_select);
        break;
    case SCREEN_LEVEL:
        draw_level(SNAKE2_LEVELS, level);
        break;
    case SCREEN_BT_MENU:
        draw_list(text_bantumi, bt_item_count(), bt_item, bt_item_top, bt_item_name, text_select);
        break;
    case SCREEN_BT_OPTIONS:
        draw_list(text_game_options, 2, bt_option, 0, bt_option_name, text_select);
        break;
    case SCREEN_BT_BEANS:
        draw_list(text_no_of_beans, 3, bt_beans, 0, bt_beans_name, text_select);
        break;
    case SCREEN_BT_LEVEL:
        draw_level(BANTUMI_LEVELS, bt_level);
        break;
    case SCREEN_BT_HELP:
        si_help_next = draw_wrapped(&font_small_bold, 0, LIST_Y, HELP_LINE_HEIGHT, SI_HELP_LINES, LCD_WIDTH, si_help_page);
        draw_softkeys(text_more, text_back);
        break;

    case SCREEN_HELP:
        draw_header(text_instructions, (uint8_t)(help_page + 1));
        draw_wrapped(&font_small_plain, 0, LIST_Y, HELP_LINE_HEIGHT, HELP_LINES, LCD_WIDTH, help_load(help_text(help_page)));
        draw_softkeys(text_more, text_back);
        break;
    default:
        break;
    }
}

/* The full-screen menus made at build time (native_tiles.h), numbered:
   Games, Select game, About, Game options, the mazes, the settings with
   each combination of the switches' values, Snake II's menu without and
   with Continue, the instructions' pages, and the first screen. */
#define NATIVE_ID_GAMES 0
#define NATIVE_ID_SELECT 1
#define NATIVE_ID_ABOUT 2
#define NATIVE_ID_OPTIONS 3
#define NATIVE_ID_MAZES 4
#define NATIVE_ID_SETTINGS 5
#define NATIVE_SWITCHES 3
#define NATIVE_ID_GAME (NATIVE_ID_SETTINGS + (1 << NATIVE_SWITCHES))
#define NATIVE_ID_HELP (NATIVE_ID_GAME + 2)
#define NATIVE_ID_MAIN (NATIVE_ID_HELP + HELP_PAGES)
#define NATIVE_ID_END (NATIVE_ID_MAIN + 1)

/* The full-screen menus with a cursor, and where it is. */
static uint8_t native_listed(void)
{
    return (uint8_t)(screen == SCREEN_GAMES || screen == SCREEN_SELECT || screen == SCREEN_SETTINGS
                     || screen == SCREEN_SETTING_VALUE || screen == SCREEN_GAME || screen == SCREEN_OPTIONS
                     || screen == SCREEN_MAZES);
}

static uint8_t native_selection(void)
{
    switch (screen) {
    case SCREEN_GAMES:
        return games_item;
    case SCREEN_SELECT:
        return game;
    case SCREEN_GAME:
        return item;
    case SCREEN_OPTIONS:
        return option;
    case SCREEN_MAZES:
        return maze;
    default:
        return setting;
    }
}

uint16_t menu_native_id(void)
{
    uint8_t i, bits = 0, n = 0;

    switch (screen) {
    case SCREEN_MAIN:
        return NATIVE_ID_MAIN;
    case SCREEN_GAMES:
        return NATIVE_ID_GAMES;
    case SCREEN_SELECT:
        return NATIVE_ID_SELECT;
    case SCREEN_ABOUT:
        return NATIVE_ID_ABOUT;
    case SCREEN_OPTIONS:
        return NATIVE_ID_OPTIONS;
    case SCREEN_MAZES:
        return NATIVE_ID_MAZES;
    case SCREEN_SETTINGS:
    case SCREEN_SETTING_VALUE:
        for (i = 0; i < SETTING_COUNT; i++) {
            const uint8_t *on = setting_switch(i);

            if (on)
                bits |= (uint8_t)((*on ? 1 : 0) << n++);
        }
        return (uint16_t)(NATIVE_ID_SETTINGS + bits);
    case SCREEN_GAME:
        return (uint16_t)(NATIVE_ID_GAME + (paused ? 1 : 0));
    case SCREEN_HELP:
        return (uint16_t)(NATIVE_ID_HELP + help_page);
    default:
        return NATIVE_NONE;
    }
}

static void draw_native(void);
static void draw_main(void);

/* The host draws them for native_gen.c; the platform that shows them has
   no use for these. */
#ifndef NATIVE_PLATFORM_TILES
uint8_t menu_native_draw(uint16_t id)
{
    uint8_t i, n = 0;

    if (id >= NATIVE_ID_END)
        return NATIVE_END;
    full_screen = 1;
    /* No selection: the cursor is drawn over the picture. */
    games_item = game = item = option = maze = setting = 0xff;
    if (id == NATIVE_ID_GAMES) {
        screen = SCREEN_GAMES;
    } else if (id == NATIVE_ID_SELECT) {
        screen = SCREEN_SELECT;
    } else if (id == NATIVE_ID_ABOUT) {
        screen = SCREEN_ABOUT;
    } else if (id == NATIVE_ID_OPTIONS) {
        screen = SCREEN_OPTIONS;
    } else if (id == NATIVE_ID_MAZES) {
        screen = SCREEN_MAZES;
    } else if (id < NATIVE_ID_GAME) {
        screen = SCREEN_SETTINGS;
        for (i = 0; i < SETTING_COUNT; i++) {
            uint8_t *on = setting_switch(i);

            if (on)
                *on = (uint8_t)(((id - NATIVE_ID_SETTINGS) >> n++) & 1);
        }
    } else if (id < NATIVE_ID_HELP) {
        screen = SCREEN_GAME;
        paused = (uint8_t)(id - NATIVE_ID_GAME);
    } else if (id == NATIVE_ID_MAIN) {
        screen = SCREEN_MAIN;
        full_screen = 0;
        lcd_view_phone();
        draw_main();
        return NATIVE_DRAWN;
    } else {
        screen = SCREEN_HELP;
        help_page = (uint8_t)(id - NATIVE_ID_HELP);
    }
    draw_native();
    return NATIVE_DRAWN;
}

void menu_native_cursor(uint8_t row)
{
    native_cursor(row);
}
#endif

static void draw_native(void)
{
    switch (screen) {
    case SCREEN_SI_MENU:
        native_list(text_space_impact, si_item_count(), si_item, si_item_name);
        break;
    case SCREEN_SI_CHAPTERS:
        native_list(text_chapters, 1, 0, si_chapter_name);
        break;
    case SCREEN_SI_DONE:
        native_note(text_chapters, text_done, 0);
        break;
    case SCREEN_SI_HELP:
        native_title(text_instructions);
        si_help_next = draw_wrapped(&NATIVE_BODY_FONT, NATIVE_MARGIN, NATIVE_LIST_Y, NATIVE_ROW_HEIGHT,
                                    (uint8_t)((NATIVE_HINT_Y - 3 - NATIVE_LIST_Y) / NATIVE_ROW_HEIGHT),
                                    LCD_FB_WIDTH - 2 * NATIVE_MARGIN, si_help_page);
        native_hint(si_help < SI_HELP_TEXTS - 1 ? text_hint_more : text_hint_back);
        break;
    case SCREEN_GAMES:
        native_list(text_games, GAMES_COUNT, games_item, games_name);
        break;
    case SCREEN_SELECT:
        native_list(text_select_game, (uint8_t)(GAME_COUNT + 1), game, select_name);
        break;
    case SCREEN_ABOUT:
        native_note(text_about, text_about_body, 0);
        native_hint(text_hint_back);
        break;
    case SCREEN_SETTINGS:
    case SCREEN_SETTING_VALUE:
        native_settings();
        break;
    case SCREEN_GAME:
        native_list(text_snake, item_count(), item, item_name_at);
        break;
    case SCREEN_OPTIONS:
        native_list(text_game_options, OPTION_COUNT, option, option_name);
        break;
    case SCREEN_MAZES:
        native_list(text_mazes, SNAKE2_MAZE_COUNT, maze, maze_name);
        break;
    case SCREEN_LEVEL:
        native_note(text_level, "%N", (uint16_t)(level + 1));
        native_hint(text_hint_select);
        break;
    case SCREEN_BT_MENU:
        native_list(text_bantumi, bt_item_count(), bt_item, bt_item_name);
        break;
    case SCREEN_BT_OPTIONS:
        native_list(text_game_options, 2, bt_option, bt_option_name);
        break;
    case SCREEN_BT_BEANS:
        native_list(text_no_of_beans, 3, bt_beans, bt_beans_name);
        break;
    case SCREEN_BT_LEVEL:
        native_note(text_level, "%N", (uint16_t)(bt_level + 1));
        native_hint(text_hint_select);
        break;
    case SCREEN_BT_HELP:
        native_title(text_instructions);
        si_help_next = draw_wrapped(&NATIVE_BODY_FONT, NATIVE_MARGIN, NATIVE_LIST_Y, NATIVE_ROW_HEIGHT,
                                    (uint8_t)((NATIVE_HINT_Y - 3 - NATIVE_LIST_Y) / NATIVE_ROW_HEIGHT),
                                    LCD_FB_WIDTH - 2 * NATIVE_MARGIN, si_help_page);
        native_hint(text_hint_more);
        break;

    case SCREEN_HELP:
        native_title(text_instructions);
        draw_wrapped(&NATIVE_BODY_FONT, NATIVE_MARGIN, NATIVE_LIST_Y, NATIVE_ROW_HEIGHT,
                     (uint8_t)((NATIVE_HINT_Y - 3 - NATIVE_LIST_Y) / NATIVE_ROW_HEIGHT),
                     LCD_FB_WIDTH - 2 * NATIVE_MARGIN, help_load(help_text(help_page)));
        native_hint(text_hint_more);
        break;
    default:
        break;
    }
}

void menu_draw(void)
{
    /* The full-screen variant has its own menus over the whole framebuffer;
       all else is drawn in the phone's LCD. Changing between them, or
       leaving a screen that drew around the LCD, clears everything. */
    uint8_t picture = screen == SCREEN_PLAY || screen == SCREEN_TITLE || screen == SCREEN_GAME_OVER
                      || screen == SCREEN_HIGH_SCORES || screen == SCREEN_SI_TITLE || screen == SCREEN_SI_PLAY
                      || screen == SCREEN_SI_SCORES || screen == SCREEN_SI_DEMO || screen == SCREEN_BT_TITLE
                      || screen == SCREEN_BT_PLAY || screen == SCREEN_BT_DEMO;
    uint8_t mode = full_screen && screen != SCREEN_MAIN && !picture ? VIEW_NATIVE : VIEW_PHONE;
#ifdef NATIVE_PLATFORM_TILES
    uint16_t was_tiled = tiled_id;
#endif

    if ((screen == SCREEN_SI_DEMO && si_demo_waiting) || (screen == SCREEN_BT_DEMO && bt_demo_waiting))
        return;
#ifdef NATIVE_PLATFORM_TILES
    tiled_id = NATIVE_NONE;
#endif

    if (full_screen && screen == SCREEN_PLAY && LCD_HAS_SURROUND)
        mode = VIEW_BOARD;

    menu_drew_picture = picture;
    if (mode != view_mode || surround_used) {
        lcd_view_full();
        lcd_clear();
        view_mode = mode;
        surround_used = 0;
        board_drawn = 0;
    }
    /* A platform that magnifies shows the phone's LCD bigger, the game too;
       the full-screen menus as they are. */
    if (mode == VIEW_PHONE) {
        lcd_view_phone();
        if (LCD_ZOOM > 1)
            lcd_zoom_set(LCD_PHONE_X, LCD_PHONE_Y, LCD_WIDTH, LCD_HEIGHT, LCD_ZOOM);
        else
            lcd_zoom_set(0, 0, 0, 0, 1);
    } else if (mode == VIEW_BOARD) {
        lcd_view_set(BOARD_AREA_X, BOARD_AREA_Y, BOARD_AREA_W, BOARD_AREA_H);
        if (LCD_ZOOM > 1)
            lcd_zoom_set(BOARD_AREA_X, BOARD_AREA_Y, BOARD_AREA_W, BOARD_AREA_H, LCD_ZOOM);
        else
            lcd_zoom_set(0, 0, 0, 0, 1);
    } else {
        lcd_view_full();
        lcd_zoom_set(0, 0, 0, 0, 1);
    }

#ifdef NATIVE_PLATFORM_TILES
    if (picture)
        platform_native_blank();
#endif
    if (screen == SCREEN_PLAY) {
        games_draw(!board_drawn);
        board_drawn = 1;
        return;
    }
#ifdef SI_PLATFORM_ROWS
    /* A platform that shows the game from si_rows' tiles. */
    if (screen == SCREEN_SI_PLAY || screen == SCREEN_SI_DEMO) {
        si_render_rows();
        si_rows_present(!board_drawn);
        board_drawn = 1;
        return;
    }
#endif
    if (screen == SCREEN_SI_TITLE || screen == SCREEN_SI_PLAY || screen == SCREEN_SI_DEMO
        || screen == SCREEN_SI_SCORES) {
        si_render();
        if (si_scores_shown()) {
            draw_score_box(si_top_score, 1, 1);
            if (si_show_last)
                draw_score_box(si_last_score, LCD_HEIGHT - 19, 0);
        }
        sprite_present(!board_drawn);
        board_drawn = 1;
        return;
    }
    if (screen == SCREEN_BT_TITLE || screen == SCREEN_BT_PLAY || screen == SCREEN_BT_DEMO) {
        bantumi_render(!board_drawn);
        sprite_present(!board_drawn);
        board_drawn = 1;
        return;
    }
    /* The title, the game-over picture and the High scores page are the
       phone's, shown as the game is. */
    if (screen == SCREEN_TITLE || screen == SCREEN_GAME_OVER || screen == SCREEN_HIGH_SCORES) {
        if (screen == SCREEN_TITLE)
            title_draw();
        else if (screen == SCREEN_GAME_OVER)
            over_draw();
        else
            scores_draw();
        sprite_present(!board_drawn);
        board_drawn = 1;
        return;
    }
#ifdef NATIVE_PLATFORM_TILES
    /* A full-screen menu made at build time; when it is up already, only
       the cursor moves. */
    if (mode == VIEW_NATIVE || screen == SCREEN_MAIN) {
        uint16_t id = menu_native_id();
        uint8_t selection = native_selection(), listed = native_listed();

        if (id != NATIVE_NONE && (id == was_tiled || platform_native_show(id))) {
            /* The first screen draws around the LCD. */
            if (screen == SCREEN_MAIN)
                surround_used = LCD_HAS_SURROUND;
            if (id == was_tiled && listed)
                platform_native_cursor(tiled_selection, 0);
            if (listed)
                platform_native_cursor(selection, 1);
            tiled_id = id;
            tiled_selection = selection;
            return;
        }
        platform_native_blank();
    }
#endif
#ifdef NATIVE_PLATFORM_FB
    /* Made at build time: unpacked into the framebuffer, which marks only
       the cells that differ, and the cursor drawn over it. */
    if (mode == VIEW_NATIVE || screen == SCREEN_MAIN) {
        uint16_t id = menu_native_id();

        if (id != NATIVE_NONE && native_fb_show(id)) {
            if (screen == SCREEN_MAIN)
                surround_used = LCD_HAS_SURROUND;
            if (native_listed())
                native_cursor(native_selection());
            board_drawn = 0;
            return;
        }
    }
#endif
    lcd_clear();
    board_drawn = 0;
    if (mode == VIEW_NATIVE)
        draw_native();
    else
        draw_phone();
}

void menu_script(const char *keys)
{
    uint8_t n;

    for (; *keys; keys++) {
        menu_draw();
        switch (*keys) {
        case 'p':
            menu_init();
            break;
        case 'z':
            /* The seed the phone's generator has after power-on. */
            menu_seed(1);
            break;
        case 'w':
        case 't':
            for (n = *keys == 'w' ? MENU_TICKS_PER_SECOND : MENU_TICKS_PER_SECOND / 10; n; n--)
                if (menu_tick())
                    menu_draw();
            break;
        default:
            menu_key(*keys == 'u' ? MENU_KEY_UP : *keys == 'd' ? MENU_KEY_DOWN : *keys == 'l' ? MENU_KEY_LEFT
                     : *keys == 'r' ? MENU_KEY_RIGHT : *keys == 's' ? MENU_KEY_SELECT
                     : *keys == 'a' ? MENU_KEY_START : *keys == 'e' ? MENU_KEY_ALT
                     : MENU_KEY_BACK);
            /* The scripted button is let go at once. */
            menu_held(0);
            break;
        }
    }
}
