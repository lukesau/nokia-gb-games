/* Munkiki's Castles: the game's flow, after the JAR's MIDlet class KKM
   and its canvas class b: the title, the game's own menu and the other
   lists the phone's MIDP draws for it, the text pages before a level,
   the game, the level's end with its bonus count, the map of the
   castles, game over, the High scores page, the Instructions with their
   demos, and the Options; their drawing is in munkiki_pages.c. The JAR's
   names are in the comments. */
#include "munkiki_int.h"

#include <string.h>

#include "games.h"
#include "sound.h"

#include "munkiki_flow.h"

const int8_t mk_help_order[16] = { 7, 8, 9, -90, 10, -91, 11, -92, 12, -93, 13, -94, 14, -95, 15, 16 };
/* The key a demo presses on its 15th tick, by page. */
static const int8_t demo_keys[16] = { -1, -1, -1, 2, -1, 5, -1, 0, -1, 2, -1, 5, -1, 3, -1, -1 };

#define TICK_US 100000UL

struct mk_flow mkf = { SCREEN_LIST, LIST_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint32_t elapsed_us;
static uint8_t back_held;              /* the C key, which acts when released */
static uint8_t record_changed;         /* for the port to save it */
/* What the JAR saves at each level's start (d.ax), for Continue after
   the game ends. */
static int8_t saved_lives;
static uint16_t saved_score;
static uint8_t saved_keys, saved_level, saved_deaths;


/* The port's record. */
void munkiki_record_set(const struct munkiki_record *in) MK_FAR
{
    mkf.top_score = in->top;
    mkf.second_score = in->second;
    mkf.level_reached = in->level_reached ? in->level_reached : 1;
    mkf.sounds_on = in->sounds;
    mkf.lights_on = in->lights;
}

void munkiki_record_get(struct munkiki_record *out) MK_FAR
{
    out->top = mkf.top_score;
    out->second = mkf.second_score;
    out->level_reached = mkf.level_reached;
    out->sounds = mkf.sounds_on;
    out->lights = mkf.lights_on;
}

/* KKM.i: a sound, when they are on. */
void mk_play(uint8_t sound) MK_FAR
{
    if (mkf.sounds_on && games_options.sounds)
        sound_play_notes(munkiki_sounds + munkiki_sound_at[sound], MUNKIKI_SOUND_BANK);
}

/* d.aw with d.ax: the level loaded, and the game as the JAR saves it
   there. */
static void level_load(uint8_t level)
{
    mk_world_load(level);
    if (mkf.screen != SCREEN_DEMO)
        mkf.game_saved = 1;
    saved_lives = mk.lives;
    saved_score = mk.score;
    saved_keys = mk.keys;
    saved_level = level;
    saved_deaths = mk.deaths;
}

/* d's construction: a new game, or the one put away (d.ay: Munkiki at
   the level's start, the level as it was). */
static void game_new(uint8_t fresh)
{
    uint8_t over = (uint8_t)(mk.lives < 0 || mkf.part == PART_OVER);

    if (fresh || over) {
        memset(&mk, 0, sizeof mk);
        mk.demo_key = -1;
        if (fresh) {
            mk_lives_reset();
            mk.lives = 3;
            mk.keys = 0;
            mk.score = 0;
            level_load(mkf.level_reached);
        } else {
            /* After the game ended, as the JAR saved it when the level began. */
            mk.lives = saved_lives;
            mk.score = saved_score;
            mk.keys = saved_keys;
            mk.deaths = saved_deaths;
            level_load(saved_level);
        }
    } else {
        /* The JAR reloads the level from its record, which snaps moving
           blocks to their cells and forgets the movers; here the level
           goes on as it was. */
        mk.land = 0;
        mk.ride = 0;
        mk.riding = 0;
        mk.idle = 0;
        mk.demo = 0;
        mk.demo_key = -1;
        mk.look_pitch = mk.look_yaw = 0;
        mk.ended = 0;
        mk.vz = mk.vx = mk.speed = mk.vy = mk.vyaw = 0;
        mk.left = 0;
        mk_world_resume();
    }
    mkf.game_exists = 1;
}

static void list_open(uint8_t which);
static void part_open(uint8_t which);
static void scene_render(void);

/* KKM.p: a screen. */
static void screen_open(int8_t which)
{
    mkf.list = LIST_NONE;
    mkf.screen = which;
    switch (which) {
    case SCREEN_TITLE:
        mk_play(MK_SOUND_TITLE);
        break;
    case SCREEN_GAME:
    case SCREEN_DEMO:
        game_new((uint8_t)(mkf.counter == 1));
        if (mkf.counter == 1)
            mk.deaths = 0;
        break;
    case SCREEN_SCORES:
        mkf.counter = 0;
        break;
    default:
        break;
    }
}

/* KKM.q: a part of the game. */
static void part_open(uint8_t which)
{
    mkf.list = LIST_NONE;
    mkf.part = which;
    switch (which) {
    case PART_HINT:
        mkf.counter = (int16_t)-((65 - mk_page_height(PART_HINT)) >> 1);
        if (mkf.counter > 0)
            mkf.counter = 0;
        mk.level = (uint8_t)(mkf.level_reached - 1);
        break;
    case PART_LEVEL:
        mkf.counter = (int16_t)-((65 - mk_page_height(PART_LEVEL) - 14) >> 1);
        if (mkf.counter > 0)
            mkf.counter = 0;
        break;
    case PART_BONUS:
        mkf.level_reached = (uint8_t)(mk.level + 1);
        if (mk.level > 30)
            mkf.level_reached = 30;
        record_changed = 1;
        mkf.counter = 0;
        break;
    case PART_MAP:
        mkf.counter = 0;
        break;
    case PART_OVER: {
        uint8_t n = mk.level;

        mkf.level_reached = mk.level;
        record_changed = 1;
        mk_play(MK_SOUND_GAME_OVER);
        mkf.counter = mk.lives < 0 ? 0 : 20;
        if (mk.lives < 0)
            n--;
        mkf.percent = (int16_t)(n * 90 / 30 + mk.score / 1000);
        break;
    }
    default:
        break;
    }
}

/* KKM.r: one of the phone's pages. Opened over the game, the game is
   painted once more first (KKM.s has the canvas repaint). */
static void list_open(uint8_t which)
{
    if (mkf.list == LIST_NONE && mkf.game_exists && (mkf.part == PART_PLAY || mkf.part == PART_DONE))
        scene_render();
    mkf.list = which;
    mkf.selection = mkf.list_top = 0;
    switch (which) {
    case LIST_LEVEL:
        mkf.list_count = 2;
        break;
    case LIST_PAUSE:
        mkf.list_count = 3;
        break;
    case LIST_OPTIONS:
        mkf.screen = SCREEN_LIST;
        mkf.option_row = 0;
        break;
    case LIST_HELP:
        mkf.counter = 0;
        mkf.help_page++;
        mkf.demo_fresh = 0;
        if (mkf.help_page > 16) {
            list_open(LIST_MAIN);
            return;
        }
        if (mk_help_order[mkf.help_page - 1] < 0) {
            mkf.counter = 1;
            screen_open(SCREEN_DEMO);
            part_open(PART_PLAY);
            level_load((uint8_t)-mk_help_order[mkf.help_page - 1]);
            mkf.demo_fresh = 1;
            mkf.list = LIST_NONE;
            if (mk_help_order[mkf.help_page - 1] != -95)
                mk.demo = 1;
            return;
        }
        mkf.screen = SCREEN_HELP;
        mkf.text_scroll = 0;
        break;
    default:
        mkf.list = LIST_MAIN;
        mkf.screen = SCREEN_LIST;
        mkf.can_continue = mkf.game_saved;
        mkf.list_count = (uint8_t)(4 + mkf.can_continue);
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
    if (mk.score > mkf.top_score) {
        mkf.second_score = mkf.top_score;
        mkf.top_score = mk.score;
    } else {
        mkf.second_score = mk.score;
    }
}

/* KKM.m: a tick. */
static void game_tick(void)
{
    if (mkf.list != LIST_NONE)
        return;
    switch (mkf.screen) {
    case SCREEN_TITLE:
        if (mk_any_pressed() || ++mkf.counter > 25)
            list_open(LIST_MAIN);
        break;
    case SCREEN_SCORES:
        if (mk_any_pressed() || ++mkf.counter == 64)
            list_open(LIST_MAIN);
        break;
    case SCREEN_DEMO:
        if (mkf.demo_fresh) {
            mkf.counter = 1;
            mkf.demo_fresh = 0;
        }
        mkf.counter++;
        if ((mk_pressed & (1 << 5)) || mkf.counter > 50) {
            mk_pressed &= (uint16_t)~(1 << 5);
            list_open(LIST_HELP);
            break;
        }
        mk_pressed = 0;
        if (mkf.counter == 15 && demo_keys[mkf.help_page - 1] >= 0)
            mk_pressed = (uint16_t)(1 << demo_keys[mkf.help_page - 1]);
        /* then as the game */
        /* fall through */
    case SCREEN_GAME:
        if (mkf.part == PART_HINT || mkf.part == PART_LEVEL) {
            uint8_t head = mkf.part == PART_LEVEL ? 14 : 0;
            uint8_t height = mk_page_height(mkf.part);

            if (height > 65 - head) {
                if (mk_pressed & (1 << 2)) {
                    mkf.counter -= 8;
                    if (mkf.counter < 0)
                        mkf.counter = 0;
                    mk_pressed &= (uint16_t)~(1 << 2);
                }
                if (mk_pressed & (1 << 8)) {
                    int16_t most = (int16_t)(height + head - 65);

                    mkf.counter += 8;
                    if (mkf.counter > most)
                        mkf.counter = most;
                    mk_pressed &= (uint16_t)~(1 << 8);
                }
            }
        }
        switch (mkf.part) {
        case PART_HINT:
            if (mk_any_pressed())
                part_open(PART_MAP);
            break;
        case PART_LEVEL:
            if (mk_any_pressed()) {
                part_open(PART_PLAY);
                mk_world_tick();
            }
            break;
        case PART_PLAY:
            mk_world_tick();
            if (mkf.screen == SCREEN_DEMO)
                mk_pressed &= (uint16_t)~(1 << 5);
            if (mk.ended == 1) {
                life_lost();
                break;
            }
            if (mk.ended == 0 || mk.level >= 90)
                break;
            mkf.counter = 0;
            part_open(PART_DONE);
            mkf.bonus_key = mk.ended == MK_THING_KEY ? 500 : 0;
            mkf.bonus_warp = mk.ended == MK_THING_WARP ? (int16_t)((5 - mk.level % 5) * 400) : 0;
            mkf.bonus_time = (mkf.bonus_warp + mkf.bonus_key == 0 && mk.time < mk.moves)
                             ? (int16_t)(190L * mk.time / (2L * mk.moves - mk.time) + 10)
                             : 200;
            while (mk.deaths > 0) {
                mkf.bonus_time = (int16_t)(mkf.bonus_time * 3 >> 2);
                mk.deaths--;
            }
            if (mk.level < 26)
                mkf.bonus_time >>= 1;
            mkf.bonus_last = mk.level == 30 ? 400 : 0;
            break;
        case PART_DONE: {
            int16_t n = 38;

            if (25 + mk.keys == mk.level) {
                mkv.curtain = (uint8_t)mkf.counter;
                mkv.curtain_kind = mk.level == 30 ? 2 : 1;
                n = 58;
            }
            mk.anim = 11;
            mk_world_tick();
            if (++mkf.counter > n)
                part_open(PART_BONUS);
            break;
        }
        case PART_BONUS:
            if (++mkf.counter <= 8)
                break;
            if (mkf.bonus_time > 0) {
                if ((mkf.bonus_time = bonus_step(mkf.bonus_time)) == 0)
                    mkf.counter = 0;
            } else if (mkf.bonus_key > 0) {
                if ((mkf.bonus_key = bonus_step(mkf.bonus_key)) == 0)
                    mkf.counter = 0;
            } else if (mkf.bonus_warp > 0) {
                if ((mkf.bonus_warp = bonus_step(mkf.bonus_warp)) == 0) {
                    mkf.counter = 0;
                    mkf.bonus_warp = -100;
                }
            } else if (mkf.bonus_last > 0) {
                if ((mkf.bonus_last = bonus_step(mkf.bonus_last)) == 0)
                    mkf.counter = 0;
            } else if (mkf.counter > 20) {
                if (mk.level < 25 + mk.keys)
                    part_open(PART_MAP);
                else
                    part_open(PART_OVER);
            }
            break;
        case PART_MAP:
            if (mk_any_pressed() || ++mkf.counter > 50) {
                mk.level++;
                if (mkf.bonus_warp == -100 && mk.level % 5 != 0) {
                    mkf.bonus_warp = 100;
                    part_open(PART_BONUS);
                    break;
                }
                level_load(mk.level);
                mk.deaths = 0;
                part_open(PART_LEVEL);
            }
            break;
        case PART_OVER:
            if (++mkf.counter > 50) {
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

/* The game's picture is made as the JAR paints it, after every tick its
   canvas is shown with a game under way: even under the High scores page,
   whose paint then clears it (so the world turns on there too). */
static void scene_render(void)
{
    mk_scene_draw();
    mk_hud_draw();
}

void munkiki_open(void) MK_FAR
{
    elapsed_us = 0;
    mk_pressed = mk_released = 0;
    back_held = 0;
    mkf.help_page = 0;
    mkf.can_continue = 0;
    mkf.game_saved = 0;
    mkf.game_exists = 0;
    mkf.list = LIST_NONE;
    mkf.counter = 0;
    screen_open(SCREEN_TITLE);
}

/* The lists' keys (KKM.commandAction and the List's own). */
static uint8_t list_key(uint8_t key)
{
    if (mkf.list == LIST_OPTIONS) {
        if (key == MK_KEY_UP || key == MK_KEY_DOWN) {
            mkf.option_row ^= 1;
        } else if (key == MK_KEY_SELECT || key == 4 || key == 6) {
            if (mkf.option_row == 0) {
                mkf.sounds_on ^= 1;
                if (mkf.sounds_on)
                    mk_play(MK_SOUND_NOTE);
            } else {
                mkf.lights_on ^= 1;
            }
        } else if (key == MK_KEY_BACK) {
            record_changed = 1;
            list_open(LIST_MAIN);
        }
        return MK_DRAW;
    }
    if (mkf.screen == SCREEN_HELP) {
        if (key == MK_KEY_SELECT) {
            list_open(LIST_HELP);
        } else if (key == MK_KEY_BACK) {
            list_open(LIST_MAIN);
        } else if (key == MK_KEY_DOWN) {
            if (mk_help_lines() > mkf.text_scroll + 4)
                mkf.text_scroll++;
        } else if (key == MK_KEY_UP && mkf.text_scroll) {
            mkf.text_scroll--;
        }
        return MK_DRAW;
    }
    if (key == MK_KEY_UP || key == MK_KEY_DOWN) {
        if (key == MK_KEY_UP)
            mkf.selection = (uint8_t)(mkf.selection ? mkf.selection - 1 : mkf.list_count - 1);
        else
            mkf.selection = (uint8_t)(mkf.selection + 1 < mkf.list_count ? mkf.selection + 1 : 0);
        if (mkf.selection < mkf.list_top)
            mkf.list_top = mkf.selection;
        if (mkf.selection >= mkf.list_top + 4)
            mkf.list_top = (uint8_t)(mkf.selection - 3);
        return MK_DRAW;
    }
    if (key == MK_KEY_BACK) {
        if (mkf.list == LIST_MAIN)
            return MK_CLOSED;
        list_open(LIST_MAIN);
        return MK_DRAW;
    }
    if (key != MK_KEY_SELECT)
        return 0;
    switch (mkf.list) {
    case LIST_LEVEL:
        if (mkf.selection == 0)
            mkf.level_reached = 1;
        mkf.counter = 1;
        screen_open(SCREEN_GAME);
        part_open(PART_HINT);
        break;
    case LIST_PAUSE:
        if (mkf.selection == 0) {
            part_open(PART_PLAY);
        } else if (mkf.selection == 1) {
            mkf.list = LIST_NONE;
            life_lost();
        } else {
            list_open(LIST_MAIN);
        }
        break;
    default: {
        int8_t n = (int8_t)(mkf.selection - mkf.can_continue);

        if (n == -1) {
            mkf.counter = 0;
            screen_open(SCREEN_GAME);
            part_open(PART_PLAY);
        } else if (n == 0) {
            if (mkf.level_reached > 1) {
                list_open(LIST_LEVEL);
            } else {
                mkf.counter = 1;
                screen_open(SCREEN_GAME);
                part_open(PART_HINT);
            }
        } else if (n == 1) {
            screen_open(SCREEN_SCORES);
        } else if (n == 2) {
            mkf.help_page = 0;
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
    if (mkf.list != LIST_NONE || mkf.screen == SCREEN_HELP) {
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
        if (mkf.screen == SCREEN_GAME) {
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

uint8_t munkiki_button(uint8_t button, uint8_t down) MK_FAR
{
    static const uint8_t in_game[8] = { 2, 8, 5, 0, 4, 6, MK_KEY_BACK, MK_KEY_HASH };
    static const uint8_t on_list[8] = { MK_KEY_UP, MK_KEY_DOWN, MK_KEY_SELECT, MK_KEY_BACK, 0xff, 0xff, MK_KEY_SELECT,
                                        MK_KEY_SELECT };
    uint8_t key;

    if (button >= 8)
        return 0;
    key = mkf.list != LIST_NONE || mkf.screen == SCREEN_HELP ? on_list[button] : in_game[button];
    if (key == 0xff)
        return 0;
    return munkiki_key(key, down);
}

uint8_t munkiki_tick(void) MK_FAR
{
    uint8_t what = MK_DRAW;

    game_tick();
    /* b.e: keys that came up are forgotten before the paint, which draws
       the game whenever it is on the canvas (b.paint). */
    mk_pressed &= (uint16_t)~mk_released;
    if (mkf.list == LIST_NONE && mkf.game_exists && (mkf.part == PART_PLAY || mkf.part == PART_DONE))
        scene_render();
    /* KKM.j: * still held while the game paints opens the pause list. */
    if (mkf.screen == SCREEN_GAME && mkf.part == PART_PLAY && mkf.list == LIST_NONE && (mk_pressed & MK_BIT_STAR)) {
        mk_pressed &= (uint16_t)~MK_BIT_STAR;
        list_open(LIST_PAUSE);
    }
    if (record_changed) {
        record_changed = 0;
        what |= MK_RECORD;
    }
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
    mkf.selection = entry;
    mkf.list_top = entry >= 4 ? (uint8_t)(entry - 3) : 0;
    munkiki_key(MK_KEY_SELECT, 1);
    munkiki_key(MK_KEY_SELECT, 0);
}

int munkiki_state_text(char *out, int size)
{
    int n = 0;
    uint8_t i;

    n += snprintf(out + n, size - n, "K aj=%d ai=%d ab=%d ad=%d ag=%d r=%d o=%d q=%d p=%d n=%d m=%d w=%d af=%d\n", mkf.screen,
                  mkf.list, mkf.part, mkf.counter, mkf.help_page, mkf.bonus_time, mkf.bonus_key, mkf.bonus_warp, mkf.bonus_last, mkf.top_score, mkf.second_score,
                  mkf.level_reached, mkf.can_continue);
    n += snprintf(out + n, size - n, "C bt=%d bs=%d br=%d bh=%d bg=%d bf=%d be=%d bc=%d a7=%d a6=%d a5=%d a4=%d\n",
                  (int8_t)mkv.counter, mkv.wobble, mkv.show, (int8_t)mkv.sprite, mkv.dx, mkv.dy, mk.block_count, mk.thing_count,
                  mkv.sparkle, mkv.splash, mkv.curtain, mkv.curtain_kind);
    if (mkf.game_exists) {
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
