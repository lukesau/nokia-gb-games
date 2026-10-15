#include "menu.h"

#include "font.h"
#include "game.h"
#include "game_assets.h"
#include "lcd.h"
#if PHONE_LOGIC
#include "logic.h"
#endif
#include "memory.h"
#include "native_tiles.h"
#include "rand.h"
#if PHONE_ROTATION
#include "rotation.h"
#endif
#include "snake.h"
#include "sound.h"
#include "version.h"

/* Screen geometry shared by the phone's menu pages. */
#define CONTENT_WIDTH 78 /* left of the scrollbar */
#define HEADER_Y 0
#define LIST_Y 7
#define ROW_HEIGHT 10
#define VISIBLE_ROWS 3
#define SCROLLBAR_X 81
#define SCROLLBAR_HEIGHT 30
#define THUMB_HEIGHT 7
#define SOFTKEY_Y 40

/* GAMES_MENU_NUMBER, the main menu's Games entry, and GAMES_MENU_THUMB,
   where that puts its scrollbar, come from phone.h. */

#if PHONE_GAMES_ICON
/* The main menu's Games icon is animated: its first picture stays for
   GAMES_ICON_FIRST phone ticks, then the next twelve pictures follow every
   GAMES_ICON_STEP, and the first comes back and stays. Measured in MAME,
   where the first picture stays 1.08 s and the steps come 0.19 s apart.
   Showing the entry again starts it over. */
#define GAMES_ICON_X 10
#define GAMES_ICON_Y 23
#define GAMES_ICON_WIDTH 64
#define GAMES_ICON_HEIGHT 16
#define GAMES_ICON_BYTES 128
#define GAMES_ICON_PICTURES 13
#define GAMES_ICON_STEPS 13
#define GAMES_ICON_FIRST 140
#define GAMES_ICON_STEP 24
#else
/* No icon: the main menu is one picture. */
#define GAMES_ICON_PICTURES 1
#define GAMES_ICON_STEPS 0
#define GAMES_ICON_FIRST 0
#define GAMES_ICON_STEP 0
#endif

enum {
    SCREEN_MAIN,
    SCREEN_GAMES,
    SCREEN_GAME,
    SCREEN_LEVEL,
    SCREEN_TOP_SCORE,
    SCREEN_HELP,
    SCREEN_PLAY,
    SCREEN_GAME_OVER,
    SCREEN_LAST_VIEW,
    SCREEN_ABOUT /* the full-screen list's last entry: version and repository */
};

#define MAX_LEVELS 9
#define LEVEL_BASE_Y 36 /* bottom row of the level bars */

#define HELP_Y 7
#define HELP_LINE_HEIGHT 9
#define HELP_LINES 3

/* Microseconds per menu_tick call and per scheduler tick of the phone. */
#define FRAME_US (1000000ul / MENU_TICKS_PER_SECOND)
#define PHONE_TICK_US 7781

/* The Top score and Game over pages close by themselves: after 768 and
   385 phone ticks, about six and three seconds. In menu_tick calls. */
#define TOP_SCORE_TICKS ((uint16_t)(768ul * PHONE_TICK_US / FRAME_US))
#define GAME_OVER_TICKS ((uint16_t)(385ul * PHONE_TICK_US / FRAME_US))

/* The Top score page's animation in its top right corner: stars gather
   into a cup, which then flashes. A new picture every 25 phone ticks; the
   last one stays. */
#define SPARKLE_X 64
#define SPARKLE_WIDTH 21
#define SPARKLE_HEIGHT 24
#define SPARKLE_FRAME_BYTES 84
#define SPARKLE_TICKS 25
#if PHONE_SPARKLE
static const uint8_t sparkle_frames[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 10, 9, 9, 10, 10, 9, 10 };
#define SPARKLE_STEPS (sizeof sparkle_frames)
#else
#define SPARKLE_STEPS 1
#endif

/* Memory's and Logic's cursor changes between plain and inverted this
   often, in phone ticks. */
#define BLINK_TICKS 64
#if PHONE_LOGIC
#define GAME_BLINKS(g) ((g) == GAME_MEMORY || (g) == GAME_LOGIC)
#else
#define GAME_BLINKS(g) ((g) == GAME_MEMORY)
#endif

/* A game's menu in item-number order. The second entry is there only when
   a game is paused (Continue) or has just ended (Last view). */
enum {
    ITEM_LEVEL,
    ITEM_RESUME,
    ITEM_NEW_GAME,
    ITEM_TOP_SCORE,
    ITEM_INSTRUCTIONS
};

enum {
    RESUME_NONE,
    RESUME_CONTINUE,
    RESUME_LAST_VIEW
};

static uint8_t screen;
static uint8_t game;     /* selection in the list of games */
static uint8_t item;     /* selection in a game's menu, counting visible items */
static uint8_t item_top; /* first visible row of a game's menu */
static uint8_t resume;   /* what the menu's second entry is */
static uint8_t resume_game;    /* the game that entry belongs to */
static uint8_t new_top_score;  /* the game just ended beat the top score */
#if PHONE_LOGIC
static uint8_t won;            /* the game just ended was won (Logic) */
#endif
static uint16_t final_score;   /* its score */
static uint16_t play_ticks;    /* phone ticks until the game's next tick; 0 for none */
static uint8_t blink, blink_ticks; /* Memory's cursor is inverted; ticks it has been */
static uint8_t play_changed;   /* a key changed the running game's picture */
static uint8_t seed_fixed;     /* games do not seed rand from the time */
static uint16_t play_us;       /* time not yet turned into phone ticks */
static uint16_t uptime;        /* menu_tick calls so far; seeds rand */
static uint8_t board_drawn;    /* the LCD holds the running game's board */
/* The full-screen menu the LCD holds and the selection drawn on it;
   NO_SCREEN when it holds something else. */
#define NO_SCREEN 0xff
static uint8_t drawn_screen = NO_SCREEN, drawn_selection;
static uint8_t full_screen;    /* the full-screen variant was chosen */
#ifdef NATIVE_PLATFORM_TILES
static uint8_t native_tiled;   /* the platform showed that menu from its tiles */
#endif
static uint8_t view_mode;      /* which of the views below the LCD is set up for */

/* Columns of the phone's LCD a platform with LCD_GAME_ZOOM shows, and how
   many it leaves off on the left: none, the rest left off on the right.
   Rotation's and Memory's biggest boards start in the LCD's first column
   and end five short of its last. The game is drawn with that first shown
   column on a whole cell of the framebuffer, GAME_ZOOM_X, which is what a
   platform can magnify. */
#define GAME_ZOOM_WIDTH (LCD_FB_WIDTH / LCD_GAME_ZOOM < LCD_WIDTH ? LCD_FB_WIDTH / LCD_GAME_ZOOM : LCD_WIDTH)
#define GAME_ZOOM_CUT_LEFT 0
#define GAME_ZOOM_X ((LCD_FB_WIDTH - GAME_ZOOM_WIDTH) / 2 / 8 * 8)

/* The phone's LCD; the full-screen variant's own menus; its board. */
enum {
    VIEW_PHONE,
    VIEW_NATIVE,
    VIEW_BOARD
};

/* The full-screen variant's menus: a title bar, a list with every entry
   visible, and a line of button hints. The port's own design and words; the
   entries and their text are the phone's. A screen with room uses the
   phone's large font (13 rows: 10 above the baseline, 3 below); a small
   one, shown magnified by its platform, uses the phone's menu fonts. */
#if LCD_FB_HEIGHT >= 120
#define NATIVE_FONT font_large_bold
#define NATIVE_BODY_FONT font_large_bold
#define NATIVE_TITLE_HEIGHT 15
#define NATIVE_LIST_Y 21
#define NATIVE_ROW_HEIGHT 15
#define NATIVE_HINT_Y (LCD_FB_HEIGHT - 14)
#define NATIVE_MARGIN 4
#define NATIVE_CURSOR_HEIGHT 9
#define NATIVE_BAR_WIDTH 12
#define NATIVE_BAR_PITCH 15
#define NATIVE_BAR_HEIGHT 16
#define NATIVE_BAR_STEP 8
#define NATIVE_BAR_GAP 12
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
#define NATIVE_BAR_WIDTH 8
#define NATIVE_BAR_PITCH 12
#define NATIVE_BAR_HEIGHT 8
#define NATIVE_BAR_STEP 4
#define NATIVE_BAR_GAP 6
#define NATIVE_NOTE_Y 24
#define NATIVE_NOTE_PITCH 11
#endif
#define NATIVE_CURSOR_WIDTH ((NATIVE_CURSOR_HEIGHT + 1) / 2)
#define NATIVE_TEXT_X (NATIVE_MARGIN + NATIVE_CURSOR_WIDTH + 5)
#define NATIVE_HELP_LINES ((NATIVE_HINT_Y - 3 - NATIVE_LIST_Y) / NATIVE_ROW_HEIGHT)

/* The full-screen variant's time between moves by level, in units of 10 ms.
   The phone's table runs 66 48 38 30 23 18 14 11 9; this one starts at half
   the phone's level 1 and ends a little under its level 9, in even ratios. */
static const uint8_t full_screen_speed[MAX_LEVELS] = { 33, 28, 23, 19, 16, 14, 11, 10, 8 };

static const char text_hint_select[] = "B back   A select";
static const char text_hint_ok[] = "B back   A OK";
static const char text_hint_back[] = "B back";
/* The full-screen list's extra entry and its page. The port's own words. */
static const char text_about[] = "About";
static const char text_about_body[] = PHONE_NAME " games " GAME_VERSION "\ngithub.com/lukesau/\nnokia-gb-games";
static const char text_hint_more[] = "B back   A more";
static uint8_t surround_used;  /* something is drawn around the phone's LCD */

#if LCD_HAS_SURROUND
/* Shown under the phone's LCD on the first screen. The port's own words. */
static const char text_full_screen_hint[] = "START: full screen";
#endif
static struct game_settings settings; /* the chosen game's level and top score */
static uint8_t level_choice;  /* level shown on the Level page */
static uint16_t page_ticks;   /* ticks left on a timed page */
static uint8_t sparkle_step;  /* picture of the Top score page's animation */
static uint8_t sparkle_ticks; /* phone ticks it has been shown */
static uint8_t sparkle_only;  /* nothing else on the page needs drawing */
static uint8_t icon_steps;    /* steps of the Games icon's animation so far */
static uint8_t icon_ticks;    /* phone ticks to its next step */
static uint16_t icon_us;      /* time not yet turned into phone ticks */

/* Shows the main menu, its Games icon starting over. */
static void main_open(void)
{
    screen = SCREEN_MAIN;
    icon_steps = 0;
    icon_ticks = GAMES_ICON_FIRST;
    icon_us = 0;
}
static const char *help_page; /* first character of the Instructions page shown */

/* The two variants of Snake keep separate levels and top scores. The
   other games are the same game in both modes. */
static uint8_t settings_slot(void)
{
    return full_screen && game == GAME_SNAKE ? GAME_SNAKE_FULL : game;
}

static uint8_t level_count(void)
{
#if PHONE_ROTATION
    if (game == GAME_ROTATION)
        return ROTATION_LEVELS;
#endif
#if PHONE_LOGIC
    if (game == GAME_LOGIC)
        return LOGIC_LEVELS;
#endif
    return game == GAME_MEMORY ? MEMORY_LEVELS : MAX_LEVELS;
}

static void settings_load(void)
{
    platform_settings_load(settings_slot(), &settings);
    if (settings.level >= level_count())
        settings.level = 0;
}

static const char *help_text(void)
{
#if PHONE_ROTATION
    if (game == GAME_ROTATION)
        return text_help_rotation;
#endif
#if PHONE_LOGIC
    if (game == GAME_LOGIC)
        return text_help_logic;
#endif
    return game == GAME_MEMORY ? text_help_memory : text_help_snake;
}

/* The list of games is the firmware's table in its order. */
static const char *game_name(uint8_t index)
{
    switch (index) {
#if PHONE_ROTATION
    case GAME_ROTATION:
        return text_rotation;
#endif
#if PHONE_LOGIC
    case GAME_LOGIC:
        return text_logic;
#endif
    case GAME_SNAKE:
        return text_snake;
    default:
        return text_memory;
    }
}

static uint8_t item_count(void)
{
    return resume ? 5 : 4;
}

/* The item at visible position `index`. The phone numbers Level first; the
   full-screen menus list the entries in the order they are used. */
static uint8_t item_id(uint8_t index)
{
    static const uint8_t native_order[] = { ITEM_RESUME, ITEM_NEW_GAME, ITEM_LEVEL, ITEM_TOP_SCORE, ITEM_INSTRUCTIONS };

    if (full_screen)
        return native_order[resume ? index : index + 1];
    return (uint8_t)(resume || index == 0 ? index : index + 1);
}

static const char *item_name(uint8_t id)
{
    switch (id) {
    case ITEM_LEVEL:
        return text_level;
    case ITEM_RESUME:
        return resume == RESUME_CONTINUE ? text_continue : text_last_view;
    case ITEM_NEW_GAME:
        return text_new_game;
    case ITEM_TOP_SCORE:
        return text_top_score;
    default:
        return text_instructions;
    }
}

static void draw_scrollbar(uint8_t thumb)
{
    lcd_fill_rect(SCROLLBAR_X, LIST_Y, 1, SCROLLBAR_HEIGHT, 1);
    lcd_fill_rect(SCROLLBAR_X, LIST_Y + thumb, 3, THUMB_HEIGHT, 0);
    lcd_fill_rect(SCROLLBAR_X, LIST_Y + thumb, 2, 1, 1);
    lcd_fill_rect(SCROLLBAR_X + 2, LIST_Y + thumb + 1, 1, THUMB_HEIGHT - 2, 1);
    lcd_fill_rect(SCROLLBAR_X, LIST_Y + thumb + THUMB_HEIGHT - 1, 2, 1, 1);
}

/* Thumb position for entry `index` of `count`. */
static uint8_t thumb_for(uint8_t index, uint8_t count)
{
    return (uint8_t)(index * (SCROLLBAR_HEIGHT - THUMB_HEIGHT) / (count - 1));
}

/* The menu path in the top right corner, such as 6-1-2. */
static void draw_path(uint8_t depth)
{
    char path[6];
    uint8_t n = 0;

    path[n++] = '0' + GAMES_MENU_NUMBER;
    if (depth > 0) {
        path[n++] = '-';
        path[n++] = (char)('1' + game);
    }
    if (depth > 1) {
        path[n++] = '-';
        path[n++] = (char)('1' + item);
    }
    path[n] = 0;
    font_draw(&font_tiny_plain, LCD_WIDTH - font_text_width(&font_tiny_plain, path), HEADER_Y, path, 1);
}

static void draw_softkey(const char *label)
{
    font_draw(&font_small_bold, (LCD_WIDTH - font_text_width(&font_small_bold, label)) / 2, SOFTKEY_Y, label, 1);
}

static void draw_row(uint8_t row, const char *label, uint8_t selected)
{
    uint8_t y = (uint8_t)(LIST_Y + row * ROW_HEIGHT);

    if (selected)
        lcd_fill_rect(0, y, CONTENT_WIDTH, ROW_HEIGHT, 1);
    font_draw(&font_small_bold, 2, y + 1, label, !selected);
}

/* The hint for the full-screen variant, in the space under the phone's LCD,
   and the port's version in the bottom right corner of the screen. */
static void draw_hint(void)
{
#if LCD_HAS_SURROUND
    lcd_view_full();
    font_draw(&font_small_plain, (LCD_FB_WIDTH - font_text_width(&font_small_plain, text_full_screen_hint)) / 2,
              LCD_BELOW_PHONE + 8, text_full_screen_hint, 1);
    font_draw(&font_small_plain, LCD_FB_WIDTH - font_text_width(&font_small_plain, GAME_VERSION) - 2,
              LCD_FB_HEIGHT - font_small_plain.height - 2, GAME_VERSION, 1);
    lcd_view_phone();
    surround_used = 1;
#endif
}

static void draw_games_icon(void)
{
#if PHONE_GAMES_ICON
    lcd_blit_strips(GAMES_ICON_X, GAMES_ICON_Y, GAMES_ICON_WIDTH, GAMES_ICON_HEIGHT,
                    menu_games_icon + icon_steps % GAMES_ICON_PICTURES * GAMES_ICON_BYTES);
#endif
}

static void draw_main(void)
{
    draw_hint();
    draw_path(0);
    font_draw(&font_large_bold, (CONTENT_WIDTH - font_text_width(&font_large_bold, text_games)) / 2, LIST_Y, text_games, 1);
    draw_games_icon();
    draw_scrollbar(GAMES_MENU_THUMB);
    draw_softkey(text_select);
}

static void draw_games(void)
{
    uint8_t i;

    draw_path(1);
    for (i = 0; i < GAME_COUNT; i++)
        draw_row(i, game_name(i), i == game);
    draw_scrollbar(thumb_for(game, GAME_COUNT));
    draw_softkey(text_select);
}

static void draw_game(void)
{
    uint8_t row;

    draw_path(2);
    for (row = 0; row < VISIBLE_ROWS; row++) {
        uint8_t index = (uint8_t)((item_top + row) % item_count());

        draw_row(row, item_name(item_id(index)), index == item);
    }
    draw_scrollbar(thumb_for(item, item_count()));
    draw_softkey(text_select);
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

/* The selection is a cursor beside the entry, not an inverted row, so
   moving it changes only a few cells of the screen. */
static void native_cursor(uint8_t row, uint8_t on)
{
    uint8_t y = (uint8_t)(NATIVE_LIST_Y + row * NATIVE_ROW_HEIGHT + 2), i;

    lcd_fill_rect(NATIVE_MARGIN, y, NATIVE_CURSOR_WIDTH, NATIVE_CURSOR_HEIGHT, 0);
    if (on)
        for (i = 0; i < NATIVE_CURSOR_WIDTH; i++)
            lcd_fill_rect(NATIVE_MARGIN + i, y + i, 1, NATIVE_CURSOR_HEIGHT - 2 * i, 1);
}

static void native_row(uint8_t row, const char *label, uint8_t selected)
{
    font_draw(&NATIVE_FONT, NATIVE_TEXT_X, NATIVE_LIST_Y + row * NATIVE_ROW_HEIGHT + 1, label, 1);
    native_cursor(row, selected);
}

/* The full-screen list has an About entry after the games. */
static uint8_t list_count(void)
{
    return full_screen ? GAME_COUNT + 1 : GAME_COUNT;
}

static void native_games(void)
{
    uint8_t i;

    native_title(text_games);
    for (i = 0; i < GAME_COUNT; i++)
        native_row(i, game_name(i), i == game);
    native_row(GAME_COUNT, text_about, game == GAME_COUNT);
    native_hint(text_hint_select);
}

static void native_note(const char *title, const char *text, uint16_t number);

static void native_about(void)
{
    native_note(text_about, text_about_body, 0);
    native_hint(text_hint_back);
}

static void native_game(void)
{
    uint8_t i;

    native_title(game_name(game));
    for (i = 0; i < item_count(); i++)
        native_row(i, item_name(item_id(i)), i == item);
    native_hint(text_hint_select);
}

/* One of the bars across the screen, one a level, filled up to the chosen
   level. */
static void native_level_bar(uint8_t i)
{
    uint8_t x = (uint8_t)((LCD_FB_WIDTH - ((level_count() - 1) * NATIVE_BAR_PITCH + NATIVE_BAR_WIDTH)) / 2
                          + i * NATIVE_BAR_PITCH);
    uint8_t height = (uint8_t)(NATIVE_BAR_HEIGHT + i * NATIVE_BAR_STEP);
    uint8_t top = (uint8_t)(NATIVE_HINT_Y - NATIVE_BAR_GAP - height);

    lcd_fill_rect(x, top, NATIVE_BAR_WIDTH, height, 1);
    if (i > level_choice)
        lcd_fill_rect(x + 1, top + 1, NATIVE_BAR_WIDTH - 2, height - 2, 0);
}

static void native_level(void)
{
    uint8_t i;

    native_title(text_level);
    for (i = 0; i < level_count(); i++)
        native_level_bar(i);
    native_hint(text_hint_ok);
}

/* When only the selection moved on a full-screen menu, redraws just that
   and returns nonzero. */
static uint8_t native_update(void)
{
    uint8_t selection;

    if (drawn_screen != screen)
        return 0;
#ifdef NATIVE_PLATFORM_TILES
    /* A made screen: the cursor is laid over it, and another level is
       another screen. */
    if (native_tiled) {
        if (screen != SCREEN_GAMES && screen != SCREEN_GAME)
            return 0;
        selection = screen == SCREEN_GAMES ? game : item;
        platform_native_cursor(drawn_selection, 0);
        platform_native_cursor(selection, 1);
        drawn_selection = selection;
        return 1;
    }
#endif
    switch (screen) {
    case SCREEN_GAMES:
    case SCREEN_GAME:
        selection = screen == SCREEN_GAMES ? game : item;
        native_cursor(drawn_selection, 0);
        native_cursor(selection, 1);
        break;
    case SCREEN_LEVEL:
        selection = level_choice;
        while (drawn_selection < selection)
            native_level_bar(++drawn_selection);
        while (drawn_selection > selection)
            native_level_bar(drawn_selection--);
        break;
    default:
        return 0;
    }
    drawn_selection = selection;
    return 1;
}

/* A note: its lines centred on the screen; %N is the number. */
static void native_note(const char *title, const char *text, uint16_t number)
{
    uint8_t y = NATIVE_NOTE_Y;

    if (title)
        native_title(title);
    for (;;) {
        if (text[0] == '%' && text[1] == 'N') {
            char digits[6];
            uint8_t n = sizeof digits - 1;
            uint16_t value = number;

            digits[n] = 0;
            do {
                digits[--n] = (char)('0' + value % 10);
                value /= 10;
            } while (value);
            font_draw(&NATIVE_FONT, (LCD_FB_WIDTH - font_text_width(&NATIVE_FONT, digits + n)) / 2, y, digits + n, 1);
        } else {
            font_draw(&NATIVE_FONT, (LCD_FB_WIDTH - font_text_width(&NATIVE_FONT, text)) / 2, y, text, 1);
        }
        while (*text && *text != '\n')
            text++;
        if (!*text++)
            break;
        y += NATIVE_NOTE_PITCH;
    }
}

/* One bar per level: an outline that grows by two pixels a level, filled
   up to the chosen level. */
static void draw_level(void)
{
    uint8_t i;

    font_draw(&font_small_bold, 5, 0, text_level_title, 1);
    for (i = 0; i < level_count(); i++) {
        uint8_t x = (uint8_t)(5 + i * 8);
        uint8_t height = (uint8_t)(6 + i * 2);

        lcd_fill_rect(x + 5, LEVEL_BASE_Y - height, 1, height, 1);
        lcd_fill_rect(x + 1, LEVEL_BASE_Y, 5, 1, 1);
        if (i <= level_choice)
            lcd_fill_rect(x, LEVEL_BASE_Y - height - 1, 4, height, 1);
    }
    draw_softkey(text_ok);
}

static void draw_sparkle(void)
{
#if PHONE_SPARKLE
    lcd_blit_strips(SPARKLE_X, 0, SPARKLE_WIDTH, SPARKLE_HEIGHT,
                    top_score_sparkle + sparkle_frames[sparkle_step] * SPARKLE_FRAME_BYTES);
#endif
}

static void draw_number(const struct font *font, int x, int y, uint16_t value)
{
    char digits[6];
    uint8_t n = sizeof digits - 1;

    digits[n] = 0;
    do {
        digits[--n] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    font_draw(font, x, y, digits + n, 1);
}

/* A note in the large font, one line every 15 rows; %N is the number. */
static void draw_note(const char *text, uint16_t number)
{
    uint8_t y = 3;

    for (;;) {
        if (text[0] == '%' && text[1] == 'N')
            draw_number(&font_large_bold, 0, y, number);
        else
            font_draw(&font_large_bold, 0, y, text, 1);
        while (*text && *text != '\n')
            text++;
        if (!*text++)
            break;
        y += 15;
    }
}

/* The whole board, or only the last move's changes when the LCD already
   holds the board. */
static void draw_play(void)
{
    if (!board_drawn)
        lcd_clear();
    switch (game) {
#if PHONE_ROTATION
    case GAME_ROTATION:
        if (board_drawn)
            rotation_draw_changes();
        else
            rotation_draw();
        break;
#endif
#if PHONE_LOGIC
    case GAME_LOGIC:
        /* The firmware clears the LCD before every draw of this game. */
        if (board_drawn)
            lcd_clear();
        logic_draw(blink);
        break;
#endif
    case GAME_MEMORY:
        if (board_drawn)
            memory_draw_changes(blink);
        else
            memory_draw(blink);
        break;
    default:
        if (board_drawn)
            snake_draw_step();
        else
            snake_draw();
        break;
    }
}

/* The board as the game left it. */
static void draw_last_view(void)
{
    switch (game) {
#if PHONE_ROTATION
    case GAME_ROTATION:
        rotation_draw();
        break;
#endif
#if PHONE_LOGIC
    case GAME_LOGIC:
        logic_draw(0);
        break;
#endif
    case GAME_MEMORY:
        memory_draw(0);
        break;
    default:
        snake_draw();
        break;
    }
}

static const struct font *help_font(void)
{
    return full_screen ? &NATIVE_BODY_FONT : &font_small_plain;
}

static uint8_t help_lines(void)
{
    return full_screen ? NATIVE_HELP_LINES : HELP_LINES;
}

/* Returns the start of the line after the one starting at `text`: as many
   whole words as fit across the screen. */
static const char *help_next_line(const char *text)
{
    const char *end = text, *p = text;
    /* Wider than a byte: the full-screen limit is close to 255 and a line
       measured past it must still compare as too long. */
    uint16_t limit = full_screen ? LCD_FB_WIDTH - 2 * NATIVE_MARGIN : LCD_WIDTH;
    uint16_t width = 0;

    for (;;) {
        while (*p && *p != ' ')
            width += font_char_width(help_font(), *p++);
        if (width > limit && end != text)
            break;
        end = p;
        if (!*p)
            return p;
        width += font_char_width(help_font(), *p++);
    }
    return end + 1; /* skip the space the line broke at */
}

static void draw_help_line(int x, int y, const char *text, const char *end)
{
    for (; text != end && *text; text++) {
        char one[2];

        one[0] = *text;
        one[1] = 0;
        x = font_draw(help_font(), x, y, one, 1);
    }
}

static void draw_help(void)
{
    const char *line = help_page;
    uint8_t row;

    if (full_screen)
        native_title(text_instructions);
    for (row = 0; row < help_lines() && *line; row++) {
        const char *next = help_next_line(line);

        if (full_screen)
            draw_help_line(NATIVE_MARGIN, NATIVE_LIST_Y + row * NATIVE_ROW_HEIGHT, line, next);
        else
            draw_help_line(0, HELP_Y + row * HELP_LINE_HEIGHT, line, next);
        line = next;
    }
    if (full_screen)
        native_hint(text_hint_more);
    else
        draw_softkey(text_more);
}

/* More: the next page, or the first one again after the last. */
static void help_more(void)
{
    uint8_t row;

    for (row = 0; row < help_lines() && *help_page; row++)
        help_page = help_next_line(help_page);
    if (!*help_page)
        help_page = help_text();
}

/* The full-screen menus made at build time (native_tiles.h), numbered:
   the list of games, About, each game's menu with each of what its second
   entry can be, each game's Level page at each level, each game's
   instructions page by page, and the first screen with each picture of
   the Games icon. */
#define NATIVE_ID_GAMES 0
#define NATIVE_ID_ABOUT 1
#define NATIVE_RESUMES 3
#define NATIVE_ID_GAME 2
#define NATIVE_ID_LEVEL (NATIVE_ID_GAME + GAME_COUNT * NATIVE_RESUMES)
#define NATIVE_ID_HELP (NATIVE_ID_LEVEL + GAME_COUNT * MAX_LEVELS)
#define NATIVE_HELP_PAGES 16
#define NATIVE_ID_MAIN (NATIVE_ID_HELP + GAME_COUNT * NATIVE_HELP_PAGES)
#define NATIVE_ID_END (NATIVE_ID_MAIN + GAMES_ICON_PICTURES)

/* The instructions page shown, counting from 0; NATIVE_HELP_PAGES if it
   is past those numbered. */
static uint8_t help_page_number(void)
{
    const char *page = help_text();
    uint8_t n, row;

    for (n = 0; n < NATIVE_HELP_PAGES && *page; n++) {
        if (page == help_page)
            return n;
        for (row = 0; row < help_lines() && *page; row++)
            page = help_next_line(page);
    }
    return NATIVE_HELP_PAGES;
}

uint16_t menu_native_id(void)
{
    uint8_t n;

    switch (screen) {
    case SCREEN_MAIN:
        return (uint16_t)(NATIVE_ID_MAIN + icon_steps % GAMES_ICON_PICTURES);
    case SCREEN_GAMES:
        return NATIVE_ID_GAMES;
    case SCREEN_ABOUT:
        return NATIVE_ID_ABOUT;
    case SCREEN_GAME:
        return (uint16_t)(NATIVE_ID_GAME + game * NATIVE_RESUMES + resume);
    case SCREEN_LEVEL:
        return (uint16_t)(NATIVE_ID_LEVEL + game * MAX_LEVELS + level_choice);
    case SCREEN_HELP:
        n = help_page_number();
        if (n >= NATIVE_HELP_PAGES)
            return NATIVE_NONE;
        return (uint16_t)(NATIVE_ID_HELP + game * NATIVE_HELP_PAGES + n);
    default:
        return NATIVE_NONE;
    }
}

/* The host draws them for native_gen.c; the platform that shows them has
   no use for these. */
#ifndef NATIVE_PLATFORM_TILES
uint8_t menu_native_draw(uint16_t id)
{
    uint8_t n;

    if (id >= NATIVE_ID_END)
        return NATIVE_END;
    full_screen = 1;
    /* No selection: the cursor is drawn over the picture. */
    item = 0xff;
    if (id == NATIVE_ID_GAMES) {
        screen = SCREEN_GAMES;
        game = 0xff;
        native_games();
    } else if (id == NATIVE_ID_ABOUT) {
        screen = SCREEN_ABOUT;
        native_about();
    } else if (id < NATIVE_ID_LEVEL) {
        screen = SCREEN_GAME;
        game = (uint8_t)((id - NATIVE_ID_GAME) / NATIVE_RESUMES);
        resume = (uint8_t)((id - NATIVE_ID_GAME) % NATIVE_RESUMES);
        native_game();
    } else if (id < NATIVE_ID_HELP) {
        screen = SCREEN_LEVEL;
        game = (uint8_t)((id - NATIVE_ID_LEVEL) / MAX_LEVELS);
        level_choice = (uint8_t)((id - NATIVE_ID_LEVEL) % MAX_LEVELS);
        if (level_choice >= level_count())
            return NATIVE_SKIP;
        native_level();
    } else if (id >= NATIVE_ID_MAIN) {
        screen = SCREEN_MAIN;
        full_screen = 0;
        icon_steps = (uint8_t)(id - NATIVE_ID_MAIN);
        lcd_view_phone();
        draw_main();
    } else {
        screen = SCREEN_HELP;
        game = (uint8_t)((id - NATIVE_ID_HELP) / NATIVE_HELP_PAGES);
        help_page = help_text();
        for (n = (uint8_t)((id - NATIVE_ID_HELP) % NATIVE_HELP_PAGES); n; n--) {
            uint8_t row;

            for (row = 0; row < help_lines() && *help_page; row++)
                help_page = help_next_line(help_page);
            if (!*help_page)
                return NATIVE_SKIP;
        }
        draw_help();
    }
    return NATIVE_DRAWN;
}

void menu_native_cursor(uint8_t row)
{
    full_screen = 1;
    native_cursor(row, 1);
}
#endif

void menu_init(void)
{
    main_open();
    game = 0;
    resume = RESUME_NONE;
    full_screen = 0;
    settings_load();
}

/* Opens the game's menu on New game, or on Continue or Last view if there. */
static void game_menu_open(void)
{
    screen = SCREEN_GAME;
    item = item_top = full_screen ? 0 : 1;
}

/* Back to the game's menu from the game. The phone keeps the selection's
   item number, so after a game started from the plain menu it is on the
   entry the game put second, Continue or Last view. */
static void game_menu_return(void)
{
    screen = SCREEN_GAME;
    if (full_screen)
        item = item_top = 0;
}

/* Time left over from the last tick is kept, so moves do not drift late. */
static void play_schedule(uint16_t ticks)
{
    play_ticks = ticks;
}

/* The game comes on the screen: at its start and after a pause. */
static void play_resume(void)
{
    screen = SCREEN_PLAY;
    play_us = 0;
    if (game == GAME_SNAKE)
        play_schedule(snake_move_ticks());
#if PHONE_ROTATION
    else if (game == GAME_ROTATION)
        play_schedule(rotation_resume());
#endif
    else
        play_schedule(0);
}

static void play_start(void)
{
    if (!seed_fixed)
        game_srand(uptime);
#if PHONE_ROTATION
    if (game == GAME_ROTATION)
        rotation_init(settings.level);
    else
#endif
#if PHONE_LOGIC
    if (game == GAME_LOGIC)
        logic_init(settings.level);
    else
#endif
    if (game == GAME_MEMORY)
        memory_init(settings.level);
    else if (full_screen)
        snake_init(settings.level, full_screen_speed[settings.level], SNAKE_FULL_COLS, SNAKE_FULL_ROWS);
    else
        snake_init(settings.level, game_speed_table[settings.level], SNAKE_COLS, SNAKE_ROWS);
    play_resume();
}

static void play_over(uint16_t score)
{
    final_score = score;
#if PHONE_LOGIC
    won = 0;
#endif
    new_top_score = score > settings.top_score;
    sound_play(new_top_score ? SOUND_TOP_SCORE : SOUND_GAME_OVER);
    if (new_top_score) {
        settings.top_score = score;
        platform_settings_save(settings_slot(), &settings);
    }
    resume = RESUME_LAST_VIEW;
    resume_game = game;
    screen = SCREEN_GAME_OVER;
    page_ticks = GAME_OVER_TICKS;
}

#if PHONE_LOGIC
/* Logic keeps no score: a right guess ends the game with the phone's
   "YOU WON!" page. */
static void play_won(void)
{
    won = 1;
    new_top_score = 0;
    sound_play(SOUND_GAME_OVER);
    resume = RESUME_LAST_VIEW;
    resume_game = game;
    screen = SCREEN_GAME_OVER;
    page_ticks = GAME_OVER_TICKS;
}
#endif

/* The Game over page's text. */
static const char *game_over_text(void)
{
#if PHONE_LOGIC
    if (won)
        return text_game_over_won;
#endif
    return new_top_score ? text_game_over_top_score : text_game_over_score;
}

void menu_game_step(void)
{
    uint16_t delay;

    if (screen != SCREEN_PLAY)
        return;
    if (game == GAME_SNAKE) {
        delay = snake_step();
        if (delay)
            play_schedule(delay);
        else
            play_over(snake.score);
    }
#if PHONE_ROTATION
    else if (game == GAME_ROTATION) {
        delay = rotation_tick();
        if (rotation.over)
            play_over(rotation.score);
        else
            play_schedule(delay);
    }
#endif
}

/* The phone key a button stands for in the running game, or 0: `key` with
   Start and the console's Select folded into the Navi key, `button` as it
   was pressed. The direction buttons are 2, 4, 6 and 8 in every game.
   Memory turns a card with the Navi button and jumps to the next one face
   down with Start, to the previous one with Select; Rotation turns with
   the clock on the Navi button and against it on Start. Logic changes the
   figure with the Navi button, copies from earlier tries with left, and
   checks the try with Start or Select. */
static char play_phone_key(uint8_t key, uint8_t button)
{
    switch (key) {
    case MENU_KEY_UP:
        return '2';
    case MENU_KEY_DOWN:
        return '8';
    case MENU_KEY_LEFT:
        return '4';
    case MENU_KEY_RIGHT:
        return '6';
    case MENU_KEY_SELECT:
        if (game == GAME_MEMORY)
            return button == MENU_KEY_START ? '#' : button == MENU_KEY_ALT ? '*' : '5';
#if PHONE_ROTATION
        if (game == GAME_ROTATION)
            return button == MENU_KEY_START ? '1' : '3';
#endif
#if PHONE_LOGIC
        if (game == GAME_LOGIC)
            return button == MENU_KEY_SELECT ? '5' : '*';
#endif
        return 0;
    default:
        return 0;
    }
}

static void play_key(uint8_t key, uint8_t button)
{
    char phone_key = play_phone_key(key, button);
#if PHONE_ROTATION
    uint16_t delay;
#endif

    if (key == MENU_KEY_BACK) {
        /* The C key pauses into the game's menu. */
        resume = RESUME_CONTINUE;
        resume_game = game;
        game_menu_return();
    } else if (!phone_key) {
        return;
    } else if (game == GAME_SNAKE) {
        snake_key(phone_key);
    } else if (game == GAME_MEMORY) {
        play_changed = 1;
        if (memory_key(phone_key))
            play_over(memory.score);
    }
#if PHONE_LOGIC
    else if (game == GAME_LOGIC) {
        play_changed = 1;
        if (logic_key(phone_key))
            play_won();
    }
#endif
#if PHONE_ROTATION
    else {
        play_changed = 1;
        delay = rotation_key(phone_key);
        if (delay)
            play_schedule(delay);
    }
#endif
}

static void game_menu_select(void)
{
    switch (item_id(item)) {
    case ITEM_RESUME:
        if (resume == RESUME_CONTINUE) {
            play_resume();
        } else {
            screen = SCREEN_LAST_VIEW;
        }
        break;
    case ITEM_NEW_GAME:
        resume = RESUME_NONE;
        play_start();
        break;
    case ITEM_LEVEL:
        screen = SCREEN_LEVEL;
        level_choice = settings.level;
        break;
    case ITEM_TOP_SCORE:
        screen = SCREEN_TOP_SCORE;
        page_ticks = TOP_SCORE_TICKS;
        play_us = 0;
        sparkle_step = sparkle_ticks = 0;
        break;
    case ITEM_INSTRUCTIONS:
        screen = SCREEN_HELP;
        help_page = help_text();
        break;
    default:
        break;
    }
}

static void game_menu_move(uint8_t key)
{
    uint8_t count = item_count();

    if (key == MENU_KEY_DOWN) {
        item = (uint8_t)((item + 1) % count);
        /* The window follows the selection, wrapping with it. */
        if ((uint8_t)((item + count - item_top) % count) >= VISIBLE_ROWS)
            item_top = (uint8_t)((item + count - (VISIBLE_ROWS - 1)) % count);
    } else if (key == MENU_KEY_UP) {
        item = (uint8_t)((item + count - 1) % count);
        if ((uint8_t)((item + count - item_top) % count) >= VISIBLE_ROWS)
            item_top = item;
    }
}

static void handle_key(uint8_t key)
{
    /* Start picks the full-screen variant on the first screen, where there
       is one. Outside a game it and the console's Select button are more
       Navi keys. */
    uint8_t button = key, start = key == MENU_KEY_START;

    if (start || key == MENU_KEY_ALT)
        key = MENU_KEY_SELECT;
    switch (screen) {
    case SCREEN_MAIN:
        if (key == MENU_KEY_SELECT) {
            full_screen = start && LCD_HAS_SURROUND;
            resume = RESUME_NONE;
            screen = SCREEN_GAMES;
            game = 0;
        }
        break;
    case SCREEN_GAMES:
        if (key == MENU_KEY_DOWN) {
            game = (uint8_t)((game + 1) % list_count());
        } else if (key == MENU_KEY_UP) {
            game = (uint8_t)((game + list_count() - 1) % list_count());
        } else if (key == MENU_KEY_SELECT && game == GAME_COUNT) {
            screen = SCREEN_ABOUT;
        } else if (key == MENU_KEY_SELECT) {
            if (game != resume_game)
                resume = RESUME_NONE;
            settings_load();
            game_menu_open();
        } else {
            main_open();
        }
        break;
    case SCREEN_LEVEL:
        if (key == MENU_KEY_UP) {
            if (level_choice < level_count() - 1)
                level_choice++;
        } else if (key == MENU_KEY_DOWN) {
            if (level_choice > 0)
                level_choice--;
        } else {
            if (key == MENU_KEY_SELECT) {
                settings.level = level_choice;
                platform_settings_save(settings_slot(), &settings);
            }
            screen = SCREEN_GAME;
        }
        break;
    case SCREEN_HELP:
        if (key == MENU_KEY_SELECT)
            help_more();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_GAME;
        break;
    case SCREEN_TOP_SCORE:
        /* Any key closes the page; all but C then act on the menu under it. */
        screen = SCREEN_GAME;
        if (key != MENU_KEY_BACK)
            handle_key(key);
        break;
    case SCREEN_GAME_OVER:
    case SCREEN_LAST_VIEW:
        game_menu_return();
        break;
    case SCREEN_ABOUT:
        screen = SCREEN_GAMES;
        break;
    case SCREEN_PLAY:
        play_key(key, button);
        break;
    default:
        if (key == MENU_KEY_DOWN || key == MENU_KEY_UP)
            game_menu_move(key);
        else if (key == MENU_KEY_SELECT)
            game_menu_select();
        else if (key == MENU_KEY_BACK)
            screen = SCREEN_GAMES;
        break;
    }
}

uint8_t menu_key(uint8_t key)
{
    uint8_t was_screen = screen, was_game = game, was_item = item, was_top = item_top;
    uint8_t was_level = level_choice, was_resume = resume;
    const char *was_page = help_page;

    play_changed = 0;
    sparkle_only = 0;
    handle_key(key);
    return play_changed || screen != was_screen || game != was_game || item != was_item || item_top != was_top
           || level_choice != was_level || resume != was_resume || help_page != was_page;
}

void menu_seed(uint32_t seed)
{
    game_srand(seed);
    seed_fixed = 1;
}

void menu_blink(void)
{
    blink ^= 1;
}

void menu_redraw_all(void)
{
    sparkle_only = 0;
    drawn_screen = NO_SCREEN;
    board_drawn = 0;
}

uint8_t menu_tick(void)
{
    uint8_t changed = 0;

    uptime++;
    switch (screen) {
    case SCREEN_MAIN:
        if (icon_steps == GAMES_ICON_STEPS)
            break;
        for (icon_us += FRAME_US; icon_us >= PHONE_TICK_US; icon_us -= PHONE_TICK_US) {
            if (--icon_ticks)
                continue;
            icon_ticks = GAMES_ICON_STEP;
            sparkle_only = 1;
            changed = 1;
            if (++icon_steps == GAMES_ICON_STEPS)
                break;
        }
        break;
    case SCREEN_TOP_SCORE:
        if (--page_ticks == 0) {
            screen = SCREEN_GAME;
            return 1;
        }
        play_us += FRAME_US;
        while (play_us >= PHONE_TICK_US) {
            play_us -= PHONE_TICK_US;
            if (++sparkle_ticks == SPARKLE_TICKS) {
                sparkle_ticks = 0;
                if (!full_screen && sparkle_step < SPARKLE_STEPS - 1) {
                    sparkle_step++;
                    sparkle_only = 1;
                    changed = 1;
                }
            }
        }
        break;
    case SCREEN_GAME_OVER:
        if (--page_ticks == 0) {
            game_menu_return();
            return 1;
        }
        break;
    case SCREEN_PLAY:
        play_us += FRAME_US;
        while (play_us >= PHONE_TICK_US) {
            play_us -= PHONE_TICK_US;
            if (GAME_BLINKS(game) && ++blink_ticks == BLINK_TICKS) {
                blink_ticks = 0;
                blink ^= 1;
                changed = 1;
            }
            if (play_ticks && --play_ticks == 0) {
                menu_game_step();
                return 1;
            }
        }
        break;
    default:
        break;
    }
    return changed;
}

void menu_draw(void)
{
    /* The full-screen variant has its own menus over the whole framebuffer
       and its board inside a margin; all else is drawn in the phone's LCD.
       Changing between them, or leaving a screen that drew around the LCD,
       clears everything. */
    uint8_t mode = VIEW_PHONE;

    /* Only Snake has a board of its own for the full-screen variant; the
       other games are played on the phone's LCD there too. */
    if (full_screen && screen != SCREEN_MAIN) {
        if (screen != SCREEN_PLAY && screen != SCREEN_LAST_VIEW)
            mode = VIEW_NATIVE;
        else if (game == GAME_SNAKE)
            mode = VIEW_BOARD;
    }
    /* A step of the Games icon's animation changes only the icon, and the
       first screen's hint around the LCD stays as it is. */
    if (sparkle_only && screen == SCREEN_MAIN) {
        sparkle_only = 0;
#ifdef NATIVE_PLATFORM_TILES
        /* Made at build time with each picture: only the icon's cells
           change. */
        if (native_tiled && platform_native_show(menu_native_id()))
            return;
#endif
        lcd_view_phone();
        draw_games_icon();
        return;
    }
    if (mode != view_mode || surround_used) {
        lcd_view_full();
        lcd_clear();
        view_mode = mode;
        surround_used = 0;
        board_drawn = 0;
    }
    /* A platform that magnifies shows the phone's LCD and the board bigger,
       a game on the LCD in the full-screen variant bigger still where it
       can, and the full-screen menus as they are. */
    if (mode == VIEW_PHONE) {
        lcd_view_phone();
        if (LCD_GAME_ZOOM > LCD_ZOOM && full_screen && screen != SCREEN_MAIN) {
            /* A game on the phone's LCD in the full-screen variant: as much
               of the LCD as fits across the screen. */
            lcd_view_set(GAME_ZOOM_X - GAME_ZOOM_CUT_LEFT, LCD_PHONE_Y, LCD_WIDTH, LCD_HEIGHT);
            lcd_zoom_set(GAME_ZOOM_X, LCD_PHONE_Y, GAME_ZOOM_WIDTH, LCD_HEIGHT, LCD_GAME_ZOOM);
        } else if (LCD_ZOOM > 1) {
            lcd_zoom_set(LCD_PHONE_X, LCD_PHONE_Y, LCD_WIDTH, LCD_HEIGHT, LCD_ZOOM);
        } else {
            lcd_zoom_set(0, 0, 0, 0, 1);
        }
    } else if (mode == VIEW_NATIVE) {
        lcd_view_full();
        lcd_zoom_set(0, 0, 0, 0, 1);
    } else {
        lcd_view_set(SNAKE_FULL_X, SNAKE_FULL_Y, SNAKE_FULL_WIDTH, SNAKE_FULL_HEIGHT);
        lcd_zoom_set(SNAKE_AREA_X, SNAKE_AREA_Y, LCD_ZOOM > 1 ? SNAKE_AREA_WIDTH : 0, SNAKE_AREA_HEIGHT, LCD_ZOOM);
    }

    if (mode == VIEW_NATIVE && native_update())
        return;
    /* A step of the Top score page's animation leaves the rest as it is. */
    if (sparkle_only) {
        sparkle_only = 0;
        if (screen == SCREEN_TOP_SCORE && !full_screen) {
            draw_sparkle();
            return;
        }
    }
#ifdef NATIVE_PLATFORM_TILES
    native_tiled = 0;
    if (mode == VIEW_NATIVE || screen == SCREEN_MAIN) {
        uint16_t id = menu_native_id();

        if (id != NATIVE_NONE && platform_native_show(id)) {
            native_tiled = 1;
            /* The first screen draws around the LCD. */
            if (screen == SCREEN_MAIN)
                surround_used = LCD_HAS_SURROUND;
            drawn_screen = screen;
            drawn_selection = screen == SCREEN_GAMES ? game : screen == SCREEN_LEVEL ? level_choice : item;
            if (screen == SCREEN_GAMES || screen == SCREEN_GAME)
                platform_native_cursor(drawn_selection, 1);
            return;
        }
    }
    platform_native_blank();
#endif
#ifdef NATIVE_PLATFORM_FB
    /* Made at build time: unpacked into the framebuffer, the cursor drawn
       over it. */
    if (mode == VIEW_NATIVE || screen == SCREEN_MAIN) {
        uint16_t id = menu_native_id();

        if (id != NATIVE_NONE && native_fb_show(id)) {
            if (screen == SCREEN_MAIN)
                surround_used = LCD_HAS_SURROUND;
            drawn_screen = screen;
            drawn_selection = screen == SCREEN_GAMES ? game : screen == SCREEN_LEVEL ? level_choice : item;
            if (screen == SCREEN_GAMES || screen == SCREEN_GAME)
                native_cursor(drawn_selection, 1);
            return;
        }
    }
#endif
    drawn_screen = NO_SCREEN;

    if (screen != SCREEN_PLAY) {
        lcd_clear();
        board_drawn = 0;
    }
    switch (screen) {
    case SCREEN_MAIN:
        draw_main();
        break;
    case SCREEN_GAMES:
        if (full_screen)
            native_games();
        else
            draw_games();
        break;
    case SCREEN_LEVEL:
        if (full_screen)
            native_level();
        else
            draw_level();
        break;
    case SCREEN_TOP_SCORE:
        if (full_screen) {
            native_note(text_top_score, "%N", settings.top_score);
        } else {
            draw_note(text_top_score_value, settings.top_score);
            draw_sparkle();
        }
        break;
    case SCREEN_GAME_OVER:
        if (full_screen)
            native_note(0, game_over_text(), final_score);
        else
            draw_note(game_over_text(), final_score);
        break;
    case SCREEN_PLAY:
        draw_play();
        board_drawn = 1;
        break;
    case SCREEN_LAST_VIEW:
        draw_last_view();
        break;
    case SCREEN_HELP:
        draw_help();
        break;
    case SCREEN_ABOUT:
        native_about();
        break;
    default:
        if (full_screen)
            native_game();
        else
            draw_game();
        break;
    }
    if (mode == VIEW_NATIVE) {
        drawn_screen = screen;
        drawn_selection = screen == SCREEN_GAMES ? game : screen == SCREEN_LEVEL ? level_choice : item;
    }
}
