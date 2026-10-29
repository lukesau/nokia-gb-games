# Builds the portable core for the host, the Game Boy and the GBA. Everything
# generated lands in the ignored build/ directory; the assets in it are
# derived from your firmware dump.

# Your own NHM-2 v5.46 dump, as `make dump` writes it; see README.md.
DUMP ?= roms/noki3410/3410f546e.fls

# The Wintesla flash files the dump is made from, and the MAME fork that
# holds the extractor and the emulator (https://github.com/lukesau/nokia-dct3-re).
# This repository is kept in the fork's ports/ directory.
FLASH_FILES ?= roms/3410-nhm2-v546
DCT3_RE ?= ../../..
ROM_SET := roms/noki3410
PMM := 3410 virgin eeprom 005f0000.fls
# What the fork's noki3410 driver declares for BIOS 546e.
DUMP_SHA1 := e650b8a289b434f2c8260c68e44e70e84e41b4cc
PMM_SHA1 := c1cb3a37efc11ea57b96969d2b01ca0f3b0f6bbe

PYTHON ?= python3
CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -pedantic

# Scale factor of the screenshot.
SHOT_SCALE ?= 3

# Game Boy: SDCC (sm83 port) and its makebin. The Game Boy gives the core a
# framebuffer the size of its screen and shows the phone's LCD in the
# middle of it, on whole tiles, the game as it is (see platform/gb/main.c).
SDCC ?= sdcc
MAKEBIN ?= makebin
SDAS ?= sdasgb
GB_FB := -DLCD_FB_WIDTH=160 -DLCD_FB_HEIGHT=144 -DLCD_PHONE_X=32 -DLCD_PHONE_Y=40
GB_CFLAGS = -msm83 --opt-code-speed $(GB_FB) -DSNAKE2_STATE_AT=0xa100 -DSNAKE2_STATE_END=0xc000 \
	-DSI_BANKED -DSI_PLATFORM_ROWS -DSI_ROWS_FAR=9 -DSI_ROWS_AT=0xb300 -DSI_ROWS_END=0xc000 -DSI_ROWS_BLOCK=gb_block_rows -DSI_ROWS_PUT=gb_rows_put -DSI_ROWS_FILL=gb_fill -DSI_ROWS_MADE=gb_rows_made -DSI_ROWS_TERRAIN -DSI_PLATFORM_FIND_HIT -DSI_PICS_AT=0xa100 -DSI_STATE_AT=0xa900 -DSI_STATE_END=0xb300 -DLCD_PLATFORM_COLUMNS -DSPRITE_PLATFORM_BAND -DSPRITE_PLATFORM_PRESENT -DNATIVE_PLATFORM_TILES -DNATIVE_PLATFORM_PALETTE \
	-DMENU_TEXT_FAR -DBANTUMI_BANKED -DBANTUMI_PLATFORM_SEARCH -DBANTUMI_STATE_AT=0xa900 -DBANTUMI_STATE_END=0xab00 -DBANTUMI_SAVED_AT=0xab00 -DBANTUMI_SAVED_END=0xac00 \
	-DMUNKIKI_BANKED -DMK_STATE_AT=0xa100 -DMK_FRAME_SHARED -DMUNKIKI_SOUND_BANK=$(GB_MUNKIKI_BANK) \
	-Icore -I$(COMMON)/core -I$(COMMON)/bantumi -Iplatform/gb -I$(COMMON)/platform/gb -I$(ASSETS)
# The SameBoy clone scripts/setup-sameboy.sh makes: its boot ROM, and its
# core as a library (`make -C $(SAMEBOY) lib`) for tools/gb_run.
SAMEBOY ?= ../tools/SameBoy
SAMEBOY_BOOT ?= $(SAMEBOY)/build/bin/tester/dmg_boot.bin
SAMEBOY_APP ?= /Applications/SameBoy.app
# Screen frames from power-on at which check-gb takes the ROM's screen;
# shot-gb takes it at the last.
GB_FRAMES ?= 700 1000

# GBA: bare arm-none-eabi GCC, no C library, and the GBA layer the 3310
# shares (../common/platform/gba/main.c). The GBA gives the core a
# framebuffer the size of its screen and shows the phone's LCD at 2x in the
# middle of it, the full-screen variant's game too.
GBA_FB := -DLCD_FB_WIDTH=240 -DLCD_FB_HEIGHT=160 -DLCD_ZOOM=2
ARM_CC ?= arm-none-eabi-gcc
ARM_OBJCOPY ?= arm-none-eabi-objcopy
ARM_CFLAGS ?= -std=c99 -O2 -Wall -Wextra -mcpu=arm7tdmi -mthumb -mthumb-interwork -ffreestanding
MGBA_APP ?= /Applications/mGBA.app
# A GBA ROM you own, to copy the boot logo from (needed on real hardware).
GBA_LOGO_FROM ?=

BUILD := build
# The code the three ports share.
COMMON := ../common
ASSETS := $(BUILD)/assets
ASSET_SRC := $(ASSETS)/game_assets.c $(ASSETS)/snake2_data.c $(ASSETS)/game_tables.c $(ASSETS)/si_data.c \
	$(ASSETS)/si_rows_a.c $(ASSETS)/si_rows_b.c $(ASSETS)/si_rows_index.c $(ASSETS)/bantumi_data.c $(ASSETS)/game_apart.c \
	$(ASSETS)/munkiki_data.c $(ASSETS)/munkiki_sounds.c $(ASSETS)/munkiki_text.c $(ASSETS)/munkiki_gfx.c \
	$(ASSETS)/munkiki_pics.c $(ASSETS)/munkiki_sheet.c
# Munkiki's Castles' JAR, which the PMM holds (README.md).
JAR ?= roms/munkikis_castles.jar
CORE_SRC := $(wildcard core/*.c) $(wildcard $(COMMON)/core/*.c) $(COMMON)/bantumi/bantumi.c
CORE_HDR := $(wildcard core/*.h) $(wildcard $(COMMON)/core/*.h) $(wildcard $(COMMON)/bantumi/*.h)
INCLUDES := -Icore -I$(COMMON)/core -I$(COMMON)/bantumi -Iplatform/host -I$(COMMON)/platform/host -I$(ASSETS)

GB_ROM := $(BUILD)/nokia3410.gb
GB_TEST_ROM := $(BUILD)/nokia3410-keys.gb
# The banks of the Game Boy ROM (see platform/gb/far.h).
GB_BANK0 := platform/gb/main.c platform/gb/far.c platform/gb/screen.c $(COMMON)/core/lcd.c core/sprite.c core/games.c core/rand.c core/sound.c $(ASSETS)/game_tables.c \
	platform/gb/save.c $(ASSETS)/si_rows_index.c $(COMMON)/core/font.c
# The menus; font.c, in the first bank, reads the fonts here, which are mapped
# whenever the menus draw.
GB_BANK1 := core/menu.c $(ASSETS)/game_assets.c
GB_BANK2 := core/snake2.c core/title.c $(ASSETS)/snake2_data.c
# With them the Instructions' texts, which the menus copy out when they show
# one, and the menu pages' header with its font, which far.c reaches.
GB_BANK3 := $(COMMON)/platform/gb/native_tiles.c $(ASSETS)/native_data.c $(ASSETS)/game_apart.c core/menu_header.c
GB_BANK4 := $(ASSETS)/native_data.c
# Space Impact: each bank its data first, then a part of the game.
GB_SI_BANKS := 5 6 7 8 11
GB_BANK5 := core/si.c
GB_BANK6 := core/si_move.c
GB_BANK7 := core/si_hit.c
GB_BANK8 := core/si_screens.c core/si_pic.c
GB_BANK11 := core/si_demo.c
# Bantumi: the game, what drives it, its data, the search (search.s) and the graphics
# library's drawing again (core/si_pic.c, SI_PIC_RENDER_ONLY), which reads
# the pictures where they are.
GB_BANTUMI_BANK := 12
GB_BANK12 := $(COMMON)/bantumi/bantumi.c core/bantumi_play.c $(ASSETS)/bantumi_data.c
# Munkiki's Castles, in six banks, each with the data it reads (a bank
# reads no other's, core/munkiki_blit.h): the world with the levels; the
# rasterizer with the geometry and textures; Munkiki's frames with what
# draws them; the game's flow with the sounds; its pages with the text
# and the pages' pictures; the scene with the effects' pictures.
GB_MUNKIKI_BANKS := 13 14 15 16 17 18
GB_MUNKIKI_BANK := 16
GB_BANK13 := core/munkiki.c $(ASSETS)/munkiki_data.c
GB_BANK14 := core/munkiki_3d.c $(ASSETS)/munkiki_gfx.c
GB_BANK15 := core/munkiki_sprite.c $(ASSETS)/munkiki_sheet.c
GB_BANK16 := core/munkiki_game.c $(ASSETS)/munkiki_sounds.c
GB_BANK17 := core/munkiki_pages.c $(ASSETS)/munkiki_text.c
GB_BANK18 := core/munkiki_scene.c $(ASSETS)/munkiki_pics.c
# The pictures as the Game Boy draws them (core/si_rows.h): what draws
# them in the first bank, their rows in two of their own.
GB_ROWS_BANK := 9
gb_rels = $(patsubst %.c,$(BUILD)/gb/$(1)/%.rel,$(notdir $(2)))
GB_RELS := $(call gb_rels,0,$(filter-out platform/gb/main.c,$(GB_BANK0))) $(call gb_rels,1,$(GB_BANK1)) \
	$(call gb_rels,2,$(GB_BANK2)) $(call gb_rels,3,$(GB_BANK3)) $(call gb_rels,4,$(GB_BANK4)) \
	$(BUILD)/gb/$(GB_ROWS_BANK)/si_rows_a.rel $(BUILD)/gb/$(GB_ROWS_BANK)/si_rows_b.rel $(BUILD)/gb/4/si_rows.rel \
	$(foreach n,$(GB_SI_BANKS),$(BUILD)/gb/$(n)/si_data.rel $(call gb_rels,$(n),$(GB_BANK$(n)))) \
	$(call gb_rels,12,$(GB_BANK12)) $(BUILD)/gb/12/bantumi_pic.rel \
	$(foreach n,$(GB_MUNKIKI_BANKS),$(call gb_rels,$(n),$(GB_BANK$(n))))

GBA_ROM := $(BUILD)/nokia3410.gba
GBA_TEST_ROM := $(BUILD)/nokia3410-keys.gba
GBA_SRC := $(COMMON)/platform/gba/crt0.S $(COMMON)/platform/gba/main.c $(COMMON)/platform/gba/libc.c $(COMMON)/platform/gba/native_fb.c \
	$(CORE_SRC) $(ASSET_SRC) $(BUILD)/gba/native_data.c

# Keys a test ROM presses at power-on (see menu_script in core/menu.c), for
# scripted screenshots and the checks against the host's frame for them.
# The default opens Snake II and starts a game. The ROM `make gb` builds
# presses none, nor does `make gba`'s.
KEYS ?= sssss
# The game without the menus, for tools that drive it directly.
GAME_SRC := $(filter-out core/menu.c core/menu_header.c $(COMMON)/core/font.c,$(CORE_SRC))

.PHONY: help dump install-roms assets test frames golden-snake golden-si golden-si-menus golden-si-demos golden-bantumi golden-bantumi-demos check-golden check-menus check-munkiki gb check-gb bench-gb bench-report shot-gb run-gb gba check-gba shot-gba run-gba cards clean

help:
	@echo "make dump      rebuild $(DUMP) from the Wintesla files in FLASH_FILES=$(FLASH_FILES)"
	@echo "make test      build and run the host checks (no firmware needed)"
	@echo "make assets    extract the game data from DUMP=$(DUMP) into $(ASSETS)/"
	@echo "make frames    write the host reference frames to $(BUILD)/frame_*.pgm"
	@echo "make golden-snake record Snake II games in MAME into golden/snake2-*/"
	@echo "make golden-bantumi record Bantumi games in MAME into golden/bantumi-*/"
	@echo "make check-golden replay the recorded runs through the core and compare every frame"
	@echo "make check-menus compare the menu pages with the phone's in golden/menus/"
	@echo "make check-munkiki run tests/munkiki/'s scripts through the JAR and the port and compare"
	@echo "make gb        build $(GB_ROM)"
	@echo "make check-gb  run the Game Boy ROM headlessly and compare its screen with the host's"
	@echo "make shot-gb   run the Game Boy ROM headlessly and write $(BUILD)/nokia3410-gb.png"
	@echo "make run-gb    open the Game Boy ROM in SameBoy"
	@echo "make gba       build $(GBA_ROM)"
	@echo "make check-gba run the GBA ROM headlessly and compare its screen with the host's"
	@echo "make shot-gba  run the GBA ROM headlessly and write $(BUILD)/nokia3410-gba.png"
	@echo "make run-gba   open the GBA ROM in mGBA"
	@echo "make cards     copy the built ROM to the flash carts' SD cards"
	@echo "make clean     remove $(BUILD)/"

# The flash image: MCU, PPM E and the PMM at their addresses, as the MAME
# fork's `make normalize-3410` makes it. Derived from the firmware, so
# roms/ is ignored.
dump:
	@mkdir -p $(ROM_SET)
	$(PYTHON) $(DCT3_RE)/tools/extract_dct3_wintesla.py \
		--mcu $(FLASH_FILES)/NHM2NX05.460 --ppm $(FLASH_FILES)/NHM2NX05.46E \
		--pmm "$(FLASH_FILES)/3410 virgin eeprom.pmm" \
		--flash-output $(DUMP) --eeprom-output "$(ROM_SET)/$(PMM)" \
		--expect-flash-sha1 $(DUMP_SHA1) --expect-eeprom-sha1 $(PMM_SHA1)

# MAME reads its ROM sets from the fork's ignored mame/roms/. The DSP files
# are the fork's placeholders, shared by every phone.
install-roms:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@test -f $(DUMP) || { echo "Missing $(DUMP): run make dump"; exit 1; }
	@mkdir -p $(DCT3_RE)/mame/roms/noki3410
	cp $(DUMP) "$(ROM_SET)/$(PMM)" $(DCT3_RE)/mame/roms/noki3410/
	cp $(DCT3_RE)/mame/roms/noki3210/dsp_prom $(DCT3_RE)/mame/roms/noki3210/dsp_drom $(DCT3_RE)/mame/roms/noki3210/dsp_pdrom $(DCT3_RE)/mame/roms/noki3410/

assets: $(ASSET_SRC)

$(ASSETS)/game_assets.c: tools/extract_assets.py tools/extract_si.py tools/extract_bantumi.py
	$(PYTHON) tools/extract_assets.py "$(DUMP)" $(ASSETS)

$(ASSETS)/snake2_data.c $(ASSETS)/game_tables.c $(ASSETS)/si_data.c $(ASSETS)/si_rows_a.c $(ASSETS)/si_rows_b.c \
	$(ASSETS)/si_rows_index.c $(ASSETS)/bantumi_data.c $(ASSETS)/game_apart.c: $(ASSETS)/game_assets.c

$(ASSETS)/munkiki_data.c: tools/extract_munkiki.py
	$(PYTHON) tools/extract_munkiki.py "$(JAR)" $(ASSETS)

$(ASSETS)/munkiki_sounds.c $(ASSETS)/munkiki_text.c $(ASSETS)/munkiki_gfx.c $(ASSETS)/munkiki_pics.c \
	$(ASSETS)/munkiki_sheet.c $(ASSETS)/munkiki_data.h: $(ASSETS)/munkiki_data.c

$(BUILD)/test_core: tests/test_core.c $(COMMON)/core/lcd.c core/rand.c core/sprite.c $(CORE_HDR)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Icore -I$(COMMON)/core -o $@ tests/test_core.c $(COMMON)/core/lcd.c core/rand.c core/sprite.c

test: $(BUILD)/test_core
	$(BUILD)/test_core

$(BUILD)/frame: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

$(BUILD)/frame_%.pgm: $(BUILD)/frame
	$(BUILD)/frame $* $@

frames: $(BUILD)/frame_testcard.pgm $(BUILD)/frame_start.pgm $(BUILD)/frame_run-300.pgm

$(BUILD)/si_replay: platform/host/si_replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) -DSI_DEBUG $(INCLUDES) -o $@ platform/host/si_replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(ASSET_SRC)

$(BUILD)/bantumi_replay: platform/host/bantumi_replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/bantumi_replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(ASSET_SRC)

$(BUILD)/replay: platform/host/replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(ASSET_SRC)

# Munkiki's Castles alone: the game's files, the LCD, the fonts it draws
# with and the sounds it names, with the replay's own sound hooks.
MUNKIKI_SRC := core/munkiki.c core/munkiki_3d.c core/munkiki_scene.c core/munkiki_sprite.c core/munkiki_game.c core/munkiki_pages.c \
	$(COMMON)/core/lcd.c $(COMMON)/core/font.c $(ASSETS)/game_assets.c $(ASSETS)/game_apart.c \
	$(ASSETS)/munkiki_data.c $(ASSETS)/munkiki_sounds.c $(ASSETS)/munkiki_text.c $(ASSETS)/munkiki_gfx.c $(ASSETS)/munkiki_pics.c $(ASSETS)/munkiki_sheet.c
$(BUILD)/munkiki_replay: platform/host/munkiki_replay_main.c $(COMMON)/platform/host/pgm.c $(MUNKIKI_SRC) $(CORE_HDR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/munkiki_replay_main.c $(COMMON)/platform/host/pgm.c $(MUNKIKI_SRC)

# Snake II's reference runs: the firmware's own game played by the fork's
# probe, which steers the snake to the food and the creatures and, after
# so many meals, into itself. Its steering is written to the game's state
# and logged; the replay hands the game the key for each turn instead. A
# run is NAME:LEVEL_DOWNS:MAZE_DOWNS:MEALS:SECONDS[:SWITCHES]: how many
# times Down is pressed on the Level page and in the list of mazes (the
# phone starts at level 1 and No maze), and the switches more of the
# probe's, comma-separated.
SNAKE_GOLDENS ?= a:4:0:20:150 b:8:2:10:90 c:1:0:6:150:S3_IGNORE_CREATURE=1,S3_SAVE_ONCE=1

golden-snake:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(SNAKE_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/snake2-$$1; \
		levels=; i=0; while [ $$i -lt $$2 ]; do levels="$$levels,up,wait400"; i=$$((i + 1)); done; \
		mazes=; i=0; while [ $$i -lt $$3 ]; do mazes="$$mazes,down,wait400"; i=$$((i + 1)); done; \
		keys="enter,wait1500,up,wait300,up,wait300,up,wait300,up,wait1000,enter,wait1500,enter,wait1500,enter,wait6000,down,wait600,down,wait600,enter,wait1500,enter,wait1500$$mazes,enter,wait1500,down,wait600,enter,wait1500$$levels,enter,wait1500,c,wait1200,up,wait600,up,wait600,enter,wait1000"; \
		rm -rf run_golden_snake && mkdir -p run_golden_snake $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_snake) SECONDS=$$5 KEYS=$$keys \
			RUN_NVRAM_DIR=$(abspath run_golden_snake)/nvram \
			RUN_ENV="S3_AUTO=1 S3_DIE_AFTER=$$4 $$(echo $$6 | tr , ' ')" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3410_snake2_probe.lua -debug -debugger none" || exit 1; \
		cp run_golden_snake/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		$(PYTHON) tools/snake2_events.py run_golden_snake/error.log $$dir; \
	done

# Space Impact's reference runs: the firmware's own game played by the
# fork's autopilot, which lines the ship up with the nearest enemy and
# fires, logging every event the game is handed. A run is NAME:SECONDS
# [:SWITCHES], the switches more of the autopilot's, comma-separated: a
# plays on through its continues, b fires the special weapon and pauses
# once, c stays put without firing and is left to lose, through the
# continue screen to the end of the game; d cannot lose (the autopilot sets
# the lives back, and the replay does the same) and so plays on through the
# chapters. check-golden replays them all (platform/host/si_replay_main.c).
SI_GOLDENS ?= a:120:SI_CONTINUE=1 b:90:SI_SPECIAL_AT=10+25+40,SI_PAUSE_AT=30 c:150:SI_IDLE=1,SI_FIRE_EVERY=0 \
	d:1300:SI_IMMORTAL=1,SI_FIRE_EVERY=8
SI_KEYS := enter,wait1500,up,wait300,up,wait300,up,wait300,up,wait1000,enter,wait1500,enter,wait1500,down,wait500,enter,wait5000,enter,wait3000

golden-si:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(SI_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/si-$$1; \
		rm -rf run_golden_si $$dir && mkdir -p run_golden_si $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_si) SECONDS=$$2 KEYS=$(SI_KEYS) \
			RUN_NVRAM_DIR=$(abspath run_golden_si)/nvram \
			RUN_ENV="$$(echo $$3 | tr , ' ' | tr + ,)" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3410_si_bot.lua -debug -debugger none" || exit 1; \
		cp run_golden_si/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		$(PYTHON) tools/si_events.py run_golden_si/error.log $$dir; \
	done

# Space Impact's title and its High scores page, then a game: recorded from
# the title on (tools/si_events.py --title) into golden/si-m, which
# check-golden replays with the rest.
SI_MENU_KEYS := $(SI_KEYS:,enter,wait3000=),down,wait800,enter,wait6000,enter,wait1500,up,wait800,enter,wait3000

golden-si-menus:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	rm -rf run_golden_si golden/si-m && mkdir -p run_golden_si golden/si-m
	$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_si) SECONDS=60 KEYS=$(SI_MENU_KEYS) \
		RUN_NVRAM_DIR=$(abspath run_golden_si)/nvram \
		RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3410_si_bot.lua -debug -debugger none"
	cp run_golden_si/nokia_dct3_lcdmirror_*.pgm golden/si-m/
	$(PYTHON) tools/si_events.py --title run_golden_si/error.log golden/si-m

# The Instructions with their three demos, each left to run to its end:
# recorded from the first demo on (tools/si_events.py --demos) into
# golden/si-i, which check-golden replays with the rest. The autopilot only
# watches (SI_IDLE): a key would end a demo.
SI_DEMO_KEYS := $(SI_KEYS:,enter,wait3000=),up,wait1000,enter,wait2500,enter,wait2500,enter,wait16000,enter,wait2500,enter,wait2500,enter,wait16000,enter,wait2500,enter,wait10000

golden-si-demos:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	rm -rf run_golden_si golden/si-i && mkdir -p run_golden_si golden/si-i
	$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_si) SECONDS=100 KEYS=$(SI_DEMO_KEYS) \
		RUN_NVRAM_DIR=$(abspath run_golden_si)/nvram RUN_ENV="SI_IDLE=1 SI_FIRE_EVERY=0" \
		RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3410_si_bot.lua -debug -debugger none"
	cp run_golden_si/nokia_dct3_lcdmirror_*.pgm golden/si-i/
	$(PYTHON) tools/si_events.py --demos run_golden_si/error.log golden/si-i

# Bantumi's reference runs: the firmware's own game played by the fork's
# autopilot, which picks pits with a seeded generator of its own and walks
# the hand there, to the game's end. A run is NAME:LEVEL:SECONDS[:SWITCHES],
# the switches more of the autopilot's, comma-separated: a asks for hints
# at level 1 and passes the first move to the phone, c pauses and
# continues. The pits start with the beans the phone's own record says.
BANTUMI_GOLDENS ?= a:1:300:B_SEED=3,B_HINT_EVERY=3,B_PASS=1 b:2:300:B_SEED=7 c:3:300:B_SEED=3,B_PAUSE_AT=40 \
	d:5:600:B_SEED=5
BANTUMI_KEYS := enter,wait1500,up,wait300,up,wait300,up,wait300,up,wait1000,enter,wait1500,enter,wait1500,down,wait500,down,wait500,down,wait500,enter,wait5000,enter,wait3000
# The fork's autopilot.
BANTUMI_BOT ?= $(DCT3_RE)/mame_nokia_3410_bantumi_bot.lua

golden-bantumi:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(BANTUMI_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/bantumi-$$1; \
		rm -rf run_golden_bantumi $$dir && mkdir -p run_golden_bantumi $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_bantumi) SECONDS=$$3 KEYS=$(BANTUMI_KEYS) \
			RUN_NVRAM_DIR=$(abspath run_golden_bantumi)/nvram RUN_ENV="B_LEVEL=$$2 $$(echo $$4 | tr , ' ')" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(BANTUMI_BOT)) -debug -debugger none" || exit 1; \
		cp run_golden_bantumi/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		$(PYTHON) tools/bantumi_events.py run_golden_bantumi/error.log $$dir || exit 1; \
	done

# The Instructions with their five demos, each left to run to its end:
# recorded from the first demo on (tools/bantumi_events.py --demos) into
# golden/bantumi-i, and the title before them into golden/bantumi-t, which
# check-golden replays with the rest.
BANTUMI_DEMO_KEYS := $(BANTUMI_KEYS:,enter,wait3000=),down,wait800,down,wait1500,enter,wait2000$(subst $(eval) ,,$(foreach n,1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16,,enter,wait20000))

golden-bantumi-demos:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	rm -rf run_golden_bantumi golden/bantumi-i golden/bantumi-t && mkdir -p run_golden_bantumi golden/bantumi-i golden/bantumi-t
	$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3410 RUN_DIR=$(abspath run_golden_bantumi) SECONDS=400 KEYS=$(BANTUMI_DEMO_KEYS) \
		RUN_NVRAM_DIR=$(abspath run_golden_bantumi)/nvram \
		RUN_EXTRA_ARGS="-autoboot_script $(abspath $(BANTUMI_BOT)) -debug -debugger none"
	cp run_golden_bantumi/nokia_dct3_lcdmirror_*.pgm golden/bantumi-i/
	$(PYTHON) tools/bantumi_events.py --demos run_golden_bantumi/error.log golden/bantumi-i
	cp run_golden_bantumi/nokia_dct3_lcdmirror_*.pgm golden/bantumi-t/
	$(PYTHON) tools/bantumi_events.py --title run_golden_bantumi/error.log golden/bantumi-t

# Skipped when the reference runs have not been recorded.
check-golden: $(BUILD)/replay $(BUILD)/si_replay $(BUILD)/bantumi_replay
	@for dir in golden/snake2-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/replay $$dir/events.txt $$out $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
		else echo "no $$dir/events.txt: run make golden-snake; skipped"; fi; \
	done
	@for dir in golden/si-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/si_replay $$dir/events.txt $$out $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
			if [ -f $$dir/sounds.txt ]; then \
				diff $$dir/sounds.txt $$out/sounds.txt > $$out/sounds.diff \
					|| { echo "$$dir: sounds differ (phone <, replay >):"; head -20 $$out/sounds.diff; exit 1; }; \
			fi; \
		else echo "no $$dir/events.txt: run make golden-si; skipped"; fi; \
	done
	@for dir in golden/bantumi-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/bantumi_replay $$dir/events.txt $$out $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
			diff $$dir/sounds.txt $$out/sounds.txt > $$out/sounds.diff \
				|| { echo "$$dir: sounds differ (phone <, replay >):"; head -20 $$out/sounds.diff; exit 1; }; \
		else echo "no $$dir/events.txt: run make golden-bantumi; skipped"; fi; \
	done

# Munkiki's Castles against the JAR itself: each script in tests/munkiki/
# is run through the JAR's classes in tools/munkiki_harness (a JVM and the
# JAR unzipped in build/munkiki_jar) and through the port, and the state
# they log after every tick must agree (tools/munkiki_compare.py).
MUNKIKI_SCRIPTS := $(wildcard tests/munkiki/*.txt)

$(BUILD)/munkiki_jar/.unzipped: $(JAR)
	rm -rf $(BUILD)/munkiki_jar && mkdir -p $(BUILD)/munkiki_jar
	unzip -q "$(JAR)" -d $(BUILD)/munkiki_jar
	touch $@

$(ASSETS)/fontmetrics.txt: tools/munkiki_fontmetrics.py $(ASSETS)/game_assets.c
	$(PYTHON) tools/munkiki_fontmetrics.py $(ASSETS) $@

check-munkiki: $(BUILD)/munkiki_replay $(BUILD)/munkiki_jar/.unzipped $(ASSETS)/fontmetrics.txt
	@command -v java >/dev/null || { echo "no java: the JAR cannot be run; skipped"; exit 0; }
	@for script in $(MUNKIKI_SCRIPTS); do \
		out=$$(mktemp -d) && \
		JARDIR=$(abspath $(BUILD)/munkiki_jar) CLASSES=$(abspath $(BUILD)/munkiki_harness) FONTMETRICS=$(abspath $(ASSETS)/fontmetrics.txt) \
			tools/munkiki_harness/run.sh $$script > $$out/jar.txt 2>$$out/jar.err || { cat $$out/jar.err; exit 1; }; \
		$(BUILD)/munkiki_replay $$script $$out > $$out/port.txt || exit 1; \
		echo "$$script:"; $(PYTHON) tools/munkiki_compare.py $$out/port.txt $$out/jar.txt || exit 1; \
	done

# The menu pages against frames of the phone's own, captured in MAME and
# named after the host frame they must equal. Derived from the firmware, so
# the directory is ignored; skipped when empty.
check-menus: $(BUILD)/frame
	$(PYTHON) tools/check_golden.py $(BUILD)/frame golden/menus

FORCE:

# Game Boy

vpath %.c core $(COMMON)/core $(COMMON)/bantumi platform/gb $(COMMON)/platform/gb $(ASSETS)

# games.c reaches the game in bank 2 through far.c, and menu.c the title
# and the game-over picture.
$(BUILD)/gb/0/games.rel: GB_EXTRA := -Dsnake2_handler=far_snake2_handler -Dsnake2_redraw=far_snake2_redraw
$(BUILD)/gb/1/menu.rel: GB_EXTRA := -Dtitle_start=far_title_start -Dtitle_elapse=far_title_elapse -Dtitle_draw=far_title_draw \
	-Dover_start=far_over_start -Dover_elapse=far_over_elapse -Dover_draw=far_over_draw \
	-Dscores_start=far_scores_start -Dscores_elapse=far_scores_elapse -Dscores_draw=far_scores_draw \
	-Ddraw_score_box=far_draw_score_box -Dmenu_header_draw=far_menu_header

$(BUILD)/gb/0/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/0
	$(SDCC) $(GB_CFLAGS) $(GB_EXTRA) -c $< -o $@

# menu.c and menu_header.c hold a banked function each for Munkiki's
# Castles (the fonts, core/munkiki_int.h), so they are told their banks.
$(BUILD)/gb/1/menu.rel: GB_BANK_OF := -bo 1
$(BUILD)/gb/3/menu_header.rel: GB_BANK_OF := -bo 3

$(BUILD)/gb/1/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/1
	$(SDCC) $(GB_CFLAGS) $(GB_EXTRA) $(GB_BANK_OF) --codeseg CODE_1 -c $< -o $@

$(BUILD)/gb/2/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/2
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_2 -c $< -o $@

# The layer itself twice: once pressing no keys, once pressing KEYS.
$(BUILD)/gb/0/main.rel $(BUILD)/gb/0/main-keys.rel: platform/gb/main.c $(CORE_HDR) $(ASSET_SRC) FORCE
	@mkdir -p $(BUILD)/gb/0
	$(SDCC) $(GB_CFLAGS) '-DSTART_KEYS="$(if $(findstring keys,$@),$(KEYS))"' -c $< -o $@

# The screens' half of the made menus' file; the tiles' is in the next bank.
$(BUILD)/gb/3/native_data.rel: GB_PART := -DNATIVE_PART_SCREENS

$(BUILD)/gb/3/%.rel: %.c $(CORE_HDR) $(ASSET_SRC) $(COMMON)/platform/gb/native_gb.h
	@mkdir -p $(BUILD)/gb/3
	$(SDCC) $(GB_CFLAGS) $(GB_PART) $(GB_BANK_OF) --codeseg CODE_3 -c $< -o $@

$(BUILD)/gb/4/%.rel: %.c
	@mkdir -p $(BUILD)/gb/4
	$(SDCC) $(GB_CFLAGS) -DNATIVE_PART_TILES --codeseg CODE_4 -c $< -o $@

# The pictures' drawing, each tick, as Space Impact's code, in the bank of
# the made menus' tiles, which has the room; it is called banked.
$(BUILD)/gb/4/si_rows.rel: core/si_rows.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) $(GB_SI_OPT) --codeseg CODE_4 -bo 4 -c $< -o $@

# The two banks of picture rows, one array each.
$(BUILD)/gb/$(GB_ROWS_BANK)/si_rows_a.rel: $(ASSETS)/si_rows_a.c
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_9 -c $< -o $@

$(BUILD)/gb/$(GB_ROWS_BANK)/si_rows_b.rel: $(ASSETS)/si_rows_b.c
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_10 -c $< -o $@

# Space Impact's code is the most the Game Boy runs in a tick: the compiler
# is given longer to place its registers (a full build's minutes).
GB_SI_OPT ?= --max-allocs-per-node 100000

# Space Impact's banks. The first's copy of the data is the one the code
# names; the others' are the same bytes under other names, which
# tools/gb_mirror_check.py checks land at the same addresses.
define gb_si_bank
$(BUILD)/gb/$(1)/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/$(1)
	$(SDCC) $(GB_CFLAGS) $(GB_SI_OPT) $$(GB_SI_NAMES) --codeseg CODE_$(1) -bo $(1) -c $$< -o $$@
endef
si_mirror_names = $(shell sed -n 's/^extern const .* \(si_[a-z_]*\)\[.*/-D\1=\1_m$(1)/p' $(ASSETS)/si_data.h)
$(foreach n,$(GB_SI_BANKS),$(eval $(call gb_si_bank,$(n))))
$(foreach n,$(filter-out 5,$(GB_SI_BANKS)),$(eval $(BUILD)/gb/$(n)/si_data.rel: GB_SI_NAMES = $$(call si_mirror_names,$(n))))

# The full-screen menus made at build time, drawn by the host build of the
# core at the Game Boy's size (native_tiles.h).
$(ASSETS)/native_data.c: $(BUILD)/frame_gb
	$(BUILD)/frame_gb native $@

# Bantumi's bank: the game draws with the graphics library's drawing built
# again there, under its own name.
$(BUILD)/gb/12/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) -Dsi_pic_render=bantumi_pic_render --codeseg CODE_12 -bo 12 -c $< -o $@

$(BUILD)/gb/12/bantumi_pic.rel: core/si_pic.c $(CORE_HDR)
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) -DSI_PIC_RENDER_ONLY -Dsi_pic_render=bantumi_pic_render --codeseg CODE_12 -bo 12 -c $< -o $@

# Munkiki's Castles' banks: its functions are banked (-bo), called from
# the menus and between the banks through bcall.s.
define gb_mk_bank
$(BUILD)/gb/$(1)/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/$(1)
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_$(1) -bo $(1) -c $$< -o $$@
endef
$(foreach n,$(GB_MUNKIKI_BANKS),$(eval $(call gb_mk_bank,$(n))))

# Bantumi's search, in its bank.
$(BUILD)/gb/search.rel: $(COMMON)/bantumi/search.s
	@mkdir -p $(BUILD)/gb
	sed 's/_CODE_BANTUMI/_CODE_12/' $< > $(BUILD)/gb/search.s
	$(SDAS) -o $@ $(BUILD)/gb/search.s

$(BUILD)/gb/%.rel: platform/gb/%.s
	@mkdir -p $(BUILD)/gb
	$(SDAS) -o $@ $<

$(BUILD)/gb/%.rel: $(COMMON)/platform/gb/%.s
	@mkdir -p $(BUILD)/gb
	$(SDAS) -o $@ $<

# MBC5 with its rumble pin and battery-backed RAM, thirty-two ROM banks
# (19 used) and 8 KiB of RAM. The linker does not mind the first bank
# running over into the second, so its end is checked here.
$(BUILD)/nokia3410%gb: $(BUILD)/gb/crt0.rel $(BUILD)/gb/search.rel $(BUILD)/gb/blocks.rel $(BUILD)/gb/native_put.rel $(BUILD)/gb/band.rel $(BUILD)/gb/bcall.rel $(BUILD)/gb/si_fast.rel $(BUILD)/gb/0/main%rel $(GB_RELS)
	$(SDCC) -msm83 --no-std-crt0 -Wl-b_CODE_1=0x14000 -Wl-b_CODE_2=0x24000 -Wl-b_CODE_3=0x34000 -Wl-b_CODE_4=0x44000 \
		$(foreach n,$(GB_SI_BANKS),-Wl-b_CODE_$(n)=0x$(shell printf %X $(n))4000) -Wl-b_CODE_9=0x94000 -Wl-b_CODE_10=0xA4000 -Wl-b_CODE_12=0xC4000 \
		$(foreach n,$(GB_MUNKIKI_BANKS),-Wl-b_CODE_$(n)=0x$(shell printf %X $(n))4000) \
		-o $(BUILD)/gb/$(basename $(notdir $@)).ihx $^
	@$(PYTHON) $(COMMON)/tools/gb_bank_check.py $(BUILD)/gb/$(basename $(notdir $@)).map
	@$(PYTHON) tools/gb_mirror_check.py $(BUILD)/gb/$(basename $(notdir $@)).map
	$(MAKEBIN) -Z -yn NOKIA3410 -yc -yt 0x1e -yo 32 -ya 1 $(BUILD)/gb/$(basename $(notdir $@)).ihx $@

# A benchmark ROM: a recorded game (BENCH, one of golden/si-*) played as
# fast as the Game Boy goes, drawing whenever the game asks, its events in
# banks 19 on; make bench-report times every step of it in SameBoy.
BENCH ?= golden/si-d
BENCH_DIR := $(BUILD)/bench
GB_BENCH_ROM := $(BUILD)/nokia3410-bench.gb
GB_BASE_RELS := $(BUILD)/gb/crt0.rel $(BUILD)/gb/search.rel $(BUILD)/gb/blocks.rel $(BUILD)/gb/native_put.rel $(BUILD)/gb/band.rel \
	$(BUILD)/gb/bcall.rel $(BUILD)/gb/si_fast.rel $(GB_RELS)

$(BENCH_DIR)/si_bench.h: $(BENCH)/events.txt $(BENCH)/setup.txt tools/si_bench.py
	rm -rf $(BENCH_DIR)
	$(PYTHON) tools/si_bench.py $(BENCH) $(BENCH_DIR)

$(BUILD)/gb/0/main-bench.rel: platform/gb/main.c $(CORE_HDR) $(ASSET_SRC) $(BENCH_DIR)/si_bench.h
	@mkdir -p $(@D)
	$(SDCC) $(GB_CFLAGS) -DSI_BENCH -I$(BENCH_DIR) '-DSTART_KEYS=""' -c $< -o $@

$(GB_BENCH_ROM): $(GB_BASE_RELS) $(BUILD)/gb/0/main-bench.rel $(BENCH_DIR)/si_bench.h
	@set -e; rels=; banks=; for c in $(BENCH_DIR)/si_bench_*.c; do \
		n=$${c##*_}; n=$${n%.c}; b=$$((19 + n)); \
		$(SDCC) $(GB_CFLAGS) -I$(BENCH_DIR) --codeseg CODE_$$b -c $$c -o $(BENCH_DIR)/si_bench_$$n.rel; \
		rels="$$rels $(BENCH_DIR)/si_bench_$$n.rel"; banks="$$banks -Wl-b_CODE_$$b=0x$$(printf %X $$b)4000"; \
	done; \
	$(SDCC) -msm83 --no-std-crt0 -Wl-b_CODE_1=0x14000 -Wl-b_CODE_2=0x24000 -Wl-b_CODE_3=0x34000 -Wl-b_CODE_4=0x44000 \
		$(foreach n,$(GB_SI_BANKS),-Wl-b_CODE_$(n)=0x$(shell printf %X $(n))4000) -Wl-b_CODE_9=0x94000 -Wl-b_CODE_10=0xA4000 -Wl-b_CODE_12=0xC4000 \
		$(foreach n,$(GB_MUNKIKI_BANKS),-Wl-b_CODE_$(n)=0x$(shell printf %X $(n))4000) $$banks \
		-o $(BUILD)/gb/nokia3410-bench.ihx $(BUILD)/gb/crt0.rel $(BUILD)/gb/search.rel $(BUILD)/gb/blocks.rel $(BUILD)/gb/native_put.rel \
		$(BUILD)/gb/band.rel $(BUILD)/gb/bcall.rel $(BUILD)/gb/si_fast.rel $(BUILD)/gb/0/main-bench.rel $(GB_RELS) $$rels
	@$(PYTHON) $(COMMON)/tools/gb_bank_check.py $(BUILD)/gb/nokia3410-bench.map
	$(MAKEBIN) -Z -yn NOKIA3410 -yc -yt 0x1e -yo 32 -ya 1 $(BUILD)/gb/nokia3410-bench.ihx $@

bench-gb: $(GB_BENCH_ROM)

bench-report: $(GB_BENCH_ROM) $(BUILD)/gb_run $(BUILD)/si_replay
	rm -rf $(BENCH_DIR)/host && mkdir -p $(BENCH_DIR)/host
	$(BUILD)/si_replay $(BENCH)/events.txt $(BENCH_DIR)/host $$(cat $(BENCH)/setup.txt) >/dev/null
	$(PYTHON) tools/bench_report.py $(BUILD)/gb_run $(GB_BENCH_ROM) $(SAMEBOY_BOOT) $(BUILD)/gb/nokia3410-bench.map \
		$(BENCH)/events.txt $$(ls $(BENCH_DIR)/host/*.pgm | tail -1)

.SECONDARY: $(GB_RELS) $(BUILD)/gb/search.rel $(BUILD)/gb/crt0.rel $(BUILD)/gb/blocks.rel $(BUILD)/gb/native_put.rel $(BUILD)/gb/band.rel $(BUILD)/gb/bcall.rel $(BUILD)/gb/si_fast.rel

gb: $(GB_ROM)

# Headless Game Boy runs with tools/gb_run, built on SameBoy's core.
$(BUILD)/gb_run: tools/gb_run.c
	@test -f "$(SAMEBOY)/build/lib/libsameboy.a" || { echo "Missing $(SAMEBOY)/build/lib/libsameboy.a: run scripts/setup-sameboy.sh, then make -C $(SAMEBOY) lib"; exit 1; }
	@mkdir -p $(BUILD)
	$(CC) -O2 -I$(SAMEBOY) -DGB_VERSION='"x"' -o $@ $< $(SAMEBOY)/build/lib/libsameboy.a -lm

# The host's frame tool with the Game Boy's framebuffer.
$(BUILD)/frame_gb: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(GB_FB) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

# The ROM presses KEYS at power-on; its screen at each of GB_FRAMES must be
# one of the host's frames for those keys, to the pixel. With the default
# keys that is a game of Snake II under way. Also says how fast the ROM
# ran the game between the screenshots, 100% being the phone's speed.
# Space Impact as well: its title, then a game under way. The Game Boy
# keeps the phone's pace by showing fewer pictures, so the frames compared
# are those it does show.
SI_GB_KEYS := zssdswwwwwwwwwws
SI_GB_FRAMES := 1300 1600 1900
# The Instructions' first demo.
SI_DEMO_GB_KEYS := zssdswwwwwwwwwwussswww
SI_DEMO_GB_FRAMES := 1400 1500 1600
# Bantumi: its title, then a game, one pit sown and the phone's answer.
BT_GB_KEYS := zssdddssswwsww
BT_GB_FRAMES := 1300 1500 1700

check-gb: $(GB_TEST_ROM) $(BUILD)/gb_run $(BUILD)/frame_gb
	$(PYTHON) tools/check_gb_run.py $(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(BUILD)/frame_gb "$(KEYS)" $(GB_FRAMES)
	@$(MAKE) --no-print-directory $(GB_TEST_ROM) KEYS=$(SI_GB_KEYS) >/dev/null
	$(PYTHON) tools/check_gb_run.py $(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(BUILD)/frame_gb "$(SI_GB_KEYS)" $(SI_GB_FRAMES)
	@$(MAKE) --no-print-directory $(GB_TEST_ROM) KEYS=$(SI_DEMO_GB_KEYS) >/dev/null
	$(PYTHON) tools/check_gb_run.py $(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(BUILD)/frame_gb "$(SI_DEMO_GB_KEYS)" $(SI_DEMO_GB_FRAMES)
	@$(MAKE) --no-print-directory $(GB_TEST_ROM) KEYS=$(BT_GB_KEYS) >/dev/null
	$(PYTHON) tools/check_gb_run.py $(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(BUILD)/frame_gb "$(BT_GB_KEYS)" $(BT_GB_FRAMES)

# Headless screenshot of the Game Boy ROM after KEYS, as a PNG.
shot-gb: $(GB_TEST_ROM) $(BUILD)/gb_run
	$(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(lastword $(GB_FRAMES)) shot:$(BUILD)/nokia3410-gb.pgm
	magick $(BUILD)/nokia3410-gb.pgm -filter point -resize $(SHOT_SCALE)00% $(BUILD)/nokia3410-gb.png

run-gb: $(GB_ROM)
	open -a "$(SAMEBOY_APP)" $(GB_ROM)

# GBA

$(BUILD)/gba/nokia3410.elf $(BUILD)/gba/nokia3410-keys.elf: $(GBA_SRC) $(CORE_HDR) $(COMMON)/platform/gba/gba.ld FORCE
	@mkdir -p $(BUILD)/gba
	$(ARM_CC) $(ARM_CFLAGS) $(GBA_FB) -Icore -I$(COMMON)/core -I$(COMMON)/bantumi -I$(ASSETS) -I$(COMMON)/platform/gba/include -DNATIVE_PLATFORM_FB \
		'-DGBA_TITLE="NOKIA3410"' '-DGBA_CODE="N34E"' "-DGBA_SAVE_ID='4'" '-DSTART_KEYS="$(if $(findstring keys,$@),$(KEYS))"' -nostdlib -T $(COMMON)/platform/gba/gba.ld -Wl,--no-warn-rwx-segments -Wl,-Map,$(basename $@).map -o $@ $(GBA_SRC) -lgcc

# The full-screen menus made at build time, drawn by the host build of the
# core at the GBA's size (native_tiles.h).
$(BUILD)/gba/native_data.c: $(BUILD)/frame_gba
	@mkdir -p $(BUILD)/gba
	$(BUILD)/frame_gba native $@

$(GBA_ROM): $(BUILD)/gba/nokia3410.elf $(COMMON)/tools/gbafix.py FORCE
	$(ARM_OBJCOPY) -O binary $< $@
	$(PYTHON) $(COMMON)/tools/gbafix.py $@ $(if $(GBA_LOGO_FROM),--logo-from "$(GBA_LOGO_FROM)")

$(GBA_TEST_ROM): $(BUILD)/gba/nokia3410-keys.elf $(COMMON)/tools/gbafix.py FORCE
	$(ARM_OBJCOPY) -O binary $< $@
	$(PYTHON) $(COMMON)/tools/gbafix.py $@

gba: $(GBA_ROM)

# The host's frame tool with the GBA's framebuffer, for comparing its screen.
$(BUILD)/frame_gba: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(GBA_FB) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

# Headless GBA runs use mGBA's core library; ../common/scripts/setup-mgba.sh builds it.
MGBA ?= ../tools/mgba
# Screen frames to run before the screenshot; the game starts at power-on.
SHOT_FRAMES ?= 300

$(BUILD)/gba_shot: $(COMMON)/tools/gba_shot.c
	@test -f "$(MGBA)/build/libmgba.a" || { echo "Missing $(MGBA)/build/libmgba.a: run ../common/scripts/setup-mgba.sh"; exit 1; }
	@mkdir -p $(BUILD)
	$(CC) -O2 -I$(MGBA)/include -I$(MGBA)/build/include -o $@ $< $(MGBA)/build/libmgba.a -lm -framework CoreFoundation

# The ROM presses KEYS at power-on and then runs SHOT_FRAMES frames; its
# screen must be the host's picture after the same keys and about as much
# time. With the default keys that is a game of Snake II under way;
# SI_GB_KEYS checks Space Impact the same way. The ROM's clock starts once
# the keys are done and its first picture is up: the keys' waits take
# Space Impact's some 75 frames to play through, so the host's frames up to
# GBA_CHECK_BACK earlier are accepted. A screenshot may catch the ROM
# part-way through putting up a picture, so the next few frames are tried
# too.
GBA_CHECK_BACK ?= 96
check-gba: $(BUILD)/gba_shot $(BUILD)/frame_gba
	@for keys in "$(KEYS)" "$(SI_GB_KEYS)"; do \
		$(MAKE) --no-print-directory $(GBA_TEST_ROM) KEYS=$$keys >/dev/null || exit 1; \
		ok=; for later in 0 1 2 3; do \
			$(BUILD)/gba_shot $(GBA_TEST_ROM) $(BUILD)/nokia3410-gba.bmp $$(($(SHOT_FRAMES) + later)) >/dev/null; \
			for back in $$(seq 0 $(GBA_CHECK_BACK)); do \
				n=$$(($(SHOT_FRAMES) + later - back)); $(BUILD)/frame_gba menu-$$keys+$$n $(BUILD)/gbaframe.pgm >/dev/null; \
				if $(PYTHON) $(COMMON)/tools/check_gb_frame.py $(BUILD)/nokia3410-gba.bmp $(BUILD)/gbaframe.pgm >/dev/null 2>&1; then \
					echo "ok: the ROM's screen $$(($(SHOT_FRAMES) + later)) frames after keys '$$keys' is the host's after $$n"; ok=1; break 2; fi; \
			done; \
		done; \
		[ -n "$$ok" ] || { echo "FAIL: the ROM's screen about $(SHOT_FRAMES) frames after keys '$$keys' matches no host frame near it"; exit 1; }; \
	done

shot-gba: $(GBA_TEST_ROM) $(BUILD)/gba_shot
	$(BUILD)/gba_shot $(GBA_TEST_ROM) $(BUILD)/nokia3410-gba.bmp $(SHOT_FRAMES)
	$(PYTHON) $(COMMON)/tools/bmp_to_png.py $(BUILD)/nokia3410-gba.bmp $(BUILD)/nokia3410-gba.png $(SHOT_SCALE)

run-gba: $(GBA_ROM)
	open -a "$(MGBA_APP)" $(GBA_ROM)

# The ROMs to the root of the flash carts' SD cards; see the script.
cards:
	scripts/copy-to-cards.sh

clean:
	rm -rf $(BUILD)
