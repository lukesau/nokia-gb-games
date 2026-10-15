# Builds the portable core for the host, the Game Boy and the GBA. Everything
# generated lands in the ignored build/ directory; the assets in it are
# derived from your firmware dump.

# Your own NHM-5 v6.39 dump, as `make dump` writes it; see README.md.
DUMP ?= roms/noki3310/3310f639e.fls

# The Wintesla flash files the dump is made from, and the MAME fork that
# holds the extractor and the emulator (https://github.com/lukesau/nokia-dct3-re).
# This repository is kept in the fork's ports/ directory.
FLASH_FILES ?= roms/3310-nhm5-v639
DCT3_RE ?= ../../..
ROM_SET := roms/noki3310
PMM := 3310 v2 pmm.bin
# What the fork's noki3310 driver declares for BIOS 639.
DUMP_SHA1 := d5da65f417595200314eb0115bf46ca1fbf53128
PMM_SHA1 := 6bfb76a2055617e16016bf1b86efa621859efef6
PHONE_RUN ?= run_phone
PHONE_SECONDS ?= 15
# Keys the phone gets once it is up, e.g. PHONE_KEYS=enter,wait1000,down
PHONE_KEYS ?=

PYTHON ?= python3
CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -pedantic

# Scale factor of the screenshot.
SHOT_SCALE ?= 3

# Game Boy: SDCC (sm83 port) and its makebin. The Game Boy gives the core a
# framebuffer the size of its screen, shows the phone's LCD in the middle
# of it and, in the full-screen variant, the game at 2x. Space Impact's
# data is read from cartridge RAM at SI_DATA_AT (see platform/gb/main.c).
SDCC ?= sdcc
MAKEBIN ?= makebin
SDAS ?= sdasgb
GB_FB := -DLCD_FB_WIDTH=160 -DLCD_FB_HEIGHT=144 -DLCD_GAME_ZOOM=2
GB_CFLAGS = -msm83 --opt-code-speed $(GB_FB) -DSI_DATA_AT=0xa100 '-DPAIRS2_STATE_AT=(SI_DATA_AT + SI_DATA_SIZE)' -DPAIRS2_STATE_END=0xc000 '-DBANTUMI_STATE_AT=(SI_DATA_AT + SI_DATA_SIZE)' -DBANTUMI_STATE_END=0xc000 -DBANTUMI_PLATFORM_SEARCH -DSNAKE2_STATE_AT=SI_DATA_AT '-DSNAKE2_STATE_END=(SI_DATA_AT + SI_DATA_SIZE)' -DLCD_PLATFORM_COLUMNS -DSPRITE_PLATFORM_BAND -DSPRITE_PLATFORM_BITMAP -DSPRITE_PLATFORM_PRESENT -DSI_PLATFORM_FIND_HIT -DSI_SETUP_FAR -DNATIVE_PLATFORM_TILES -DNATIVE_PLATFORM_PALETTE -Icore -I$(COMMON)/core -I$(COMMON)/bantumi -Iplatform/gb -I$(COMMON)/platform/gb -I$(ASSETS)
# The SameBoy clone scripts/setup-sameboy.sh makes: its boot ROM, and its
# core as a library (`make -C $(SAMEBOY) lib`) for tools/gb_run.
SAMEBOY ?= ../tools/SameBoy
SAMEBOY_BOOT ?= $(SAMEBOY)/build/bin/tester/dmg_boot.bin
SAMEBOY_APP ?= /Applications/SameBoy.app
# Screen frames from power-on at which check-gb takes the ROM's screen;
# shot-gb takes it at the last.
GB_FRAMES ?= 700 1000

# GBA: bare arm-none-eabi GCC, no C library. The GBA gives the core a
# framebuffer the size of its screen and shows the phone's LCD at 2x, and
# the game in the full-screen variant at 3x, which its display hardware
# scales (see platform/gba/main.c).
GBA_FB := -DLCD_FB_WIDTH=240 -DLCD_FB_HEIGHT=160 -DLCD_ZOOM=2 -DLCD_GAME_ZOOM=3
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
ASSET_SRC := $(ASSETS)/game_assets.c $(ASSETS)/si_data.c $(ASSETS)/si_tables.c $(ASSETS)/snake2_data.c $(ASSETS)/title_data.c $(ASSETS)/pairs2_data.c $(ASSETS)/bantumi_data.c
CORE_SRC := $(wildcard core/*.c) $(wildcard $(COMMON)/core/*.c) $(COMMON)/bantumi/bantumi.c
CORE_HDR := $(wildcard core/*.h) $(wildcard $(COMMON)/core/*.h) $(wildcard $(COMMON)/bantumi/*.h)
INCLUDES := -Icore -I$(COMMON)/core -I$(COMMON)/bantumi -Iplatform/host -I$(COMMON)/platform/host -I$(ASSETS)

GB_ROM := $(BUILD)/nokia3310.gb
GB_TEST_ROM := $(BUILD)/nokia3310-keys.gb
# The banks of the Game Boy ROM (see platform/gb/far.h).
GB_BANK0 := platform/gb/main.c platform/gb/far.c $(COMMON)/core/font.c $(COMMON)/core/lcd.c core/sprite.c core/games.c core/rand.c core/si_base.c core/sound.c $(ASSETS)/si_tables.c
GB_BANK1 := core/menu.c platform/gb/save.c $(ASSETS)/game_assets.c
GB_BANK2 := core/si.c
GB_BANK3 := core/si_setup.c $(ASSETS)/si_data.c core/title.c $(ASSETS)/title_data.c
GB_BANK4 := core/snake2.c $(ASSETS)/snake2_data.c platform/gb/strip.c
GB_BANK5 := core/pairs2.c $(ASSETS)/pairs2_data.c
GB_BANK6 := $(COMMON)/bantumi/bantumi.c core/fireworks.c $(ASSETS)/bantumi_data.c
GB_BANK7 := $(COMMON)/platform/gb/native_tiles.c $(ASSETS)/native_data.c
GB_BANK8 := $(ASSETS)/native_data.c
gb_rels = $(patsubst %.c,$(BUILD)/gb/$(1)/%.rel,$(notdir $(2)))
GB_RELS := $(call gb_rels,0,$(filter-out platform/gb/main.c,$(GB_BANK0))) $(call gb_rels,1,$(GB_BANK1)) \
	$(call gb_rels,2,$(GB_BANK2)) $(call gb_rels,3,$(GB_BANK3)) $(call gb_rels,4,$(GB_BANK4)) \
	$(call gb_rels,5,$(GB_BANK5)) $(call gb_rels,6,$(GB_BANK6)) $(call gb_rels,7,$(GB_BANK7)) \
	$(call gb_rels,8,$(GB_BANK8))
GBA_ROM := $(BUILD)/nokia3310.gba
GBA_TEST_ROM := $(BUILD)/nokia3310-keys.gba
# Keys a test ROM presses at power-on (see menu_script in core/menu.c), for
# scripted screenshots and the checks against the host's frame for them.
# The default opens Space Impact and starts a game. The ROMs `make gba` and
# `make gb` build press none.
KEYS ?= sdsss
# The game without the menus, for tools that drive it directly.
GAME_SRC := $(filter-out core/menu.c $(COMMON)/core/font.c,$(CORE_SRC))
GBA_SRC := platform/gba/crt0.s platform/gba/main.c $(COMMON)/platform/gba/libc.c $(COMMON)/platform/gba/native_fb.c \
	$(CORE_SRC) $(ASSET_SRC) $(BUILD)/gba/native_data.c

# The recorded Space Impact run the core is checked against: the keys, the
# seed the title animation leaves the games' random generator with, and
# where its frames and events are kept. See "Golden run" in README.md.
GOLDEN ?= golden/space-impact-events1
GOLDEN_SEED := a335
GOLDEN_SECONDS := 40
GOLDEN_KEYS := enter,wait1000,enter,wait1200,up,up,up,up,up,wait800,enter,wait1500,down,wait600,enter,wait2500,enter,wait4000,1,wait400,1,wait400,8,8,8,1,wait400,0,0,0,0,1,wait300,3,wait300,4,wait2000,1,wait500,1,wait6000

.PHONY: help dump install-roms phone phone-window assets test sheet frames golden golden-snake golden-pairs golden-bantumi check-golden check-menus gb check-gb shot-gb run-gb cards gba check-gba shot-gba run-gba clean

help:
	@echo "make dump      rebuild $(DUMP) from the Wintesla files in FLASH_FILES=$(FLASH_FILES)"
	@echo "make phone     boot the dump headlessly in MAME; LCD frames land in $(PHONE_RUN)/"
	@echo "make phone-window open the dump in a MAME window"
	@echo "make test      build and run the host checks (no firmware needed)"
	@echo "make assets    extract the game data from DUMP=$(DUMP) into $(ASSETS)/"
	@echo "make sheet     draw the extracted sprites and tiles to $(BUILD)/sheet_*.pgm"
	@echo "make frames    write the host reference frames to $(BUILD)/frame_*.pgm"
	@echo "make golden    record the reference Space Impact run in MAME into $(GOLDEN)/"
	@echo "make golden-snake record two Snake II games in MAME into golden/snake2-*/"
	@echo "make golden-pairs record four Pairs II games in MAME into golden/pairs2-*/"
	@echo "make golden-bantumi record four Bantumi games in MAME into golden/bantumi-*/"
	@echo "make check-golden replay the recorded runs through the core and compare every frame"
	@echo "make check-menus compare the menu pages with the phone's in golden/menus/"
	@echo "make gb        build $(GB_ROM)"
	@echo "make check-gb  run the Game Boy ROM headlessly and compare its screen with the host's"
	@echo "make shot-gb   run the Game Boy ROM headlessly and write $(BUILD)/nokia3310-gb.png"
	@echo "make run-gb    open the Game Boy ROM in SameBoy"
	@echo "make gba       build $(GBA_ROM)"
	@echo "make check-gba run the GBA ROM headlessly and compare its screen with the host's"
	@echo "make shot-gba  run the GBA ROM headlessly and write $(BUILD)/nokia3310-gba.png"
	@echo "make run-gba   open the GBA ROM in mGBA"
	@echo "make cards     copy the built ROMs to the flash carts' SD cards"
	@echo "make clean     remove $(BUILD)/"

# The 2 MiB flash image: MCU, PPM E and the PMM tail at their addresses.
# Derived from the firmware, so roms/ is ignored.
dump:
	$(PYTHON) $(DCT3_RE)/tools/extract_dct3_wintesla.py \
		--mcu $(FLASH_FILES)/NHM5NY06.390 --ppm $(FLASH_FILES)/NHM5NY06.39E \
		--pmm $(FLASH_FILES)/v.2.pmm \
		--flash-output $(ROM_SET)/3310f639e_mcu_ppm.fls \
		--eeprom-output "$(ROM_SET)/$(PMM)" --expect-eeprom-sha1 $(PMM_SHA1) \
		--combined-output $(DUMP)
	@test "$$(shasum $(DUMP) | cut -d' ' -f1)" = $(DUMP_SHA1) || { echo "$(DUMP) is not NHM-5 v6.39 PPM E"; exit 1; }

# MAME reads its ROM sets from the fork's ignored mame/roms/. The DSP files
# are the fork's placeholders, shared by every phone.
install-roms:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@test -f $(DUMP) || { echo "Missing $(DUMP): run make dump"; exit 1; }
	@mkdir -p $(DCT3_RE)/mame/roms/noki3310
	cp $(DUMP) "$(ROM_SET)/$(PMM)" $(DCT3_RE)/mame/roms/noki3310/
	cp $(DCT3_RE)/mame/roms/noki3210/dsp_prom $(DCT3_RE)/mame/roms/noki3210/dsp_drom $(DCT3_RE)/mame/roms/noki3210/dsp_pdrom $(DCT3_RE)/mame/roms/noki3310/

phone: install-roms
	$(MAKE) -C $(DCT3_RE) run-prebuilt PHONE=noki3310 BIOS=639 RUN_DIR=$(abspath $(PHONE_RUN)) \
		SECONDS=$(PHONE_SECONDS) FRAME_PNG=$(abspath $(PHONE_RUN))/latest.png \
		RUN_ENV='$(if $(PHONE_KEYS),NOKIA_DCT3_POST_READY_KEYS=$(PHONE_KEYS) NOKIA_DCT3_POST_READY_KEY_DELAY_MS=6000 NOKIA_DCT3_POST_READY_KEY_DURATION_MS=200 NOKIA_DCT3_POST_READY_KEY_GAP_MS=200 NOKIA_DCT3_POST_READY_CAPTURE_DELAY_MS=1200)'

phone-window: install-roms
	@mkdir -p $(PHONE_RUN)_window/nvram
	cd $(PHONE_RUN)_window && $(abspath $(DCT3_RE))/mame/mame noki3310 -bios 639 \
		-rompath $(abspath $(DCT3_RE))/mame/roms -nvram_directory nvram \
		-window -resolution 672x384 -keepaspect -skip_gameinfo

assets: $(ASSET_SRC)

$(ASSETS)/game_assets.c: tools/extract_assets.py
	$(PYTHON) tools/extract_assets.py "$(DUMP)" $(ASSETS)

$(ASSETS)/si_data.c $(ASSETS)/si_tables.c $(ASSETS)/snake2_data.c $(ASSETS)/title_data.c $(ASSETS)/pairs2_data.c $(ASSETS)/bantumi_data.c: $(ASSETS)/game_assets.c

$(BUILD)/test_core: tests/test_core.c $(COMMON)/core/lcd.c core/rand.c core/sprite.c $(CORE_HDR)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Icore -I$(COMMON)/core -o $@ tests/test_core.c $(COMMON)/core/lcd.c core/rand.c core/sprite.c

test: $(BUILD)/test_core
	$(BUILD)/test_core

$(BUILD)/asset_sheet: platform/host/asset_sheet.c core/si.c core/si_base.c core/si_setup.c core/sprite.c $(COMMON)/core/lcd.c core/rand.c $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/asset_sheet.c core/si.c core/si_base.c core/si_setup.c core/sprite.c $(COMMON)/core/lcd.c core/rand.c $(ASSET_SRC)

sheet: $(BUILD)/asset_sheet
	$(BUILD)/asset_sheet $(BUILD)

$(BUILD)/frame: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

$(BUILD)/frame_%.pgm: $(BUILD)/frame
	$(BUILD)/frame $* $@

# The same tool with the GBA's framebuffer, for comparing its screen.
$(BUILD)/frame_gba: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(GBA_FB) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

$(BUILD)/gbaframe_%.pgm: $(BUILD)/frame_gba
	$(BUILD)/frame_gba $* $@

frames: $(BUILD)/frame_testcard.pgm $(BUILD)/frame_start.pgm $(BUILD)/frame_run-300.pgm

# The reference run: the firmware playing Space Impact in MAME with scripted
# keys, every LCD frame kept and every event the game was handed logged.
# Derived from the firmware, so the directory is ignored. The MAME fork is
# asked for GAMES_PRODUCT=3310; see its docs/games_applications_3310.md.
golden:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@mkdir -p $(GOLDEN) run_golden
	DCT3_RE=$(abspath $(DCT3_RE)) $(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3310 RUN_DIR=$(abspath run_golden) \
		SECONDS=$(GOLDEN_SECONDS) KEYS=$(GOLDEN_KEYS) FRAME_PNG=$(abspath run_golden)/latest.png \
		RUN_EXTRA_ARGS='-autoboot_script $(abspath tools/mame_event_log.lua) -debug -debugger none'
	cp run_golden/nokia_dct3_lcdmirror_*.pgm $(GOLDEN)/
	awk '/^GEV 1 2b/{on=1} on && /^GEV 1 /{print $$3}' run_golden/error.log | tr '\n' ' ' > $(GOLDEN)/events.txt

$(BUILD)/replay: platform/host/replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ platform/host/replay_main.c $(COMMON)/platform/host/pgm.c $(GAME_SRC) $(ASSET_SRC)

# Snake II's reference runs: the firmware's own game played by the fork's
# probe, which steers the snake to the food and the creatures and, after
# so many meals, into itself. Its steering is written to the game's state
# and logged; the replay hands the game the key for each turn instead. A
# run is NAME:LEVEL:MAZE:MEALS:SECONDS[:SWITCHES], the maze 0 for none and
# the switches more of the probe's, comma-separated. a's ring goes round
# before the end, so its dead snake does not blink; c leaves a creature to
# run out and turns the snake free once in its short tick.
SNAKE_GOLDENS ?= a:5:0:30:150 b:9:2:14:90 c:2:0:6:150:S2_IGNORE_CREATURE=1,S2_SAVE_ONCE=1
SNAKE_KEYS := enter,wait1000,enter,wait1200,up,up,up,up,up,wait800,enter,wait1000,enter,wait1500,enter,wait1500,enter,wait1000

golden-snake:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(SNAKE_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/snake2-$$1; \
		rm -rf run_golden_snake && mkdir -p run_golden_snake $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3310 RUN_DIR=$(abspath run_golden_snake) SECONDS=$$5 KEYS=$(SNAKE_KEYS) \
			RUN_ENV="S2_AUTO=1 S2_AUTO_FROM=17 S2_LEVEL=$$2 S2_MAZE=$$3 S2_DIE_AFTER=$$4 $$(echo $$6 | tr , ' ')" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3310_snake2_probe.lua -debug -debugger none" || exit 1; \
		cp run_golden_snake/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		awk 'BEGIN{k[0]="d";k[1]="b";k[2]="f";k[3]="11"} /^GEV 0 2b/{on=1} on && /^GEV 0 /{print $$3} on && /S2KEY/{print k[$$NF]}' \
			run_golden_snake/error.log | tr '\n' ' ' > $$dir/events.txt && \
		echo "$$2 $$(($$3 + 1))" > $$dir/setup.txt; \
	done

# Pairs II's reference runs: the firmware's own game played by the fork's
# autopilot, which reads the board from RAM and finds the pairs, opening a
# wrong card now and then. A run is NAME:MODE:LEVEL:SECONDS[:SWITCHES], the
# mode 0 for Time trial and 1 for Puzzle, the switches more of the
# autopilot's, comma-separated: a plays all nine boards, b lets the time
# run out on the second, c pauses between boards and d mid-Puzzle, and
# both continue.
PAIRS_GOLDENS ?= a:0:1:500:P2_MISS_EVERY=5 b:0:3:90:P2_MISS_EVERY=3,P2_STOP_BOARD=1 \
	c:0:5:80:P2_MISS_EVERY=4,P2_PAUSE_AT=40 d:1:5:90:P2_MISS_EVERY=3,P2_PAUSE_AT=30
PAIRS_KEYS = enter,wait1000,enter,wait1200,up,up,up,up,up,wait800,enter,wait1000,down,down,down,wait500,enter,wait1500,enter,wait1000,$(if $(filter 1,$(1)),down$(comma)wait500$(comma))enter,wait1200,enter,wait2000
comma := ,

golden-pairs:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(PAIRS_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/pairs2-$$1; \
		if [ $$2 = 1 ]; then keys='$(call PAIRS_KEYS,1)'; else keys='$(call PAIRS_KEYS,0)'; fi; \
		rm -rf run_golden_pairs && mkdir -p run_golden_pairs $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3310 RUN_DIR=$(abspath run_golden_pairs) SECONDS=$$4 KEYS=$$keys \
			RUN_ENV="P2_LEVEL=$$3 $$(echo $$5 | tr , ' ')" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3310_pairs2_bot.lua -debug -debugger none" || exit 1; \
		cp run_golden_pairs/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		awk -v id=$$(($$2 + 3)) '$$1=="GEV" && $$2==id && $$3=="2b"{on=1} on && $$1=="GEV" && $$2==id{print $$3}' \
			run_golden_pairs/error.log | tr '\n' ' ' > $$dir/events.txt && \
		echo "$$3 $$2" > $$dir/setup.txt; \
	done

# Bantumi's reference runs: the firmware's own game played by the fork's
# autopilot, which picks pits with a seeded generator of its own and walks
# the hand there. A run is NAME:LEVEL:SECONDS[:SWITCHES], the switches more
# of the autopilot's, comma-separated: a asks for hints at level 1, c
# pauses and continues.
BANTUMI_GOLDENS ?= a:1:240:B_SEED=1,B_HINT_EVERY=3 b:2:240:B_SEED=7 c:3:150:B_SEED=3,B_PAUSE_AT=60 d:5:150:B_SEED=5
BANTUMI_KEYS := enter,wait1000,enter,wait1200,up,up,up,up,up,wait800,enter,wait1000,down,down,wait500,enter,wait1500,enter,wait1500,enter,wait1000

golden-bantumi:
	@test -x $(DCT3_RE)/mame/mame || { echo "Missing $(DCT3_RE)/mame/mame: build the fork first"; exit 1; }
	@for run in $(BANTUMI_GOLDENS); do \
		set -- $$(echo $$run | tr : ' '); dir=golden/bantumi-$$1; \
		rm -rf run_golden_bantumi && mkdir -p run_golden_bantumi $$dir && \
		$(MAKE) -C $(DCT3_RE) run-keys GAMES_PRODUCT=3310 RUN_DIR=$(abspath run_golden_bantumi) SECONDS=$$3 KEYS=$(BANTUMI_KEYS) \
			RUN_ENV="B_LEVEL=$$2 $$(echo $$4 | tr , ' ')" \
			RUN_EXTRA_ARGS="-autoboot_script $(abspath $(DCT3_RE))/mame_nokia_3310_bantumi_bot.lua -debug -debugger none" || exit 1; \
		cp run_golden_bantumi/nokia_dct3_lcdmirror_*.pgm $$dir/ && \
		awk '$$1=="GEV" && $$2=="2" && $$3=="2b"{on=1} on && $$1=="GEV" && $$2=="2"{print $$3}' \
			run_golden_bantumi/error.log | tr '\n' ' ' > $$dir/events.txt && \
		echo "$$2 0" > $$dir/setup.txt; \
	done

# Skipped when the reference runs have not been recorded. Snake II and
# Pairs II start from the seed the phone has after power-on, 1.
check-golden: $(BUILD)/replay
	@if [ -f $(GOLDEN)/events.txt ]; then \
		out=$$(mktemp -d) && $(BUILD)/replay $(GOLDEN)/events.txt $$out $(GOLDEN_SEED) && \
		$(PYTHON) tools/check_replay.py $$out $(GOLDEN); \
	else echo "no $(GOLDEN)/events.txt: run make golden; skipped"; fi
	@for dir in golden/snake2-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/replay $$dir/events.txt $$out 1 0 $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
		else echo "no $$dir/events.txt: run make golden-snake; skipped"; fi; \
	done
	@for dir in golden/bantumi-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/replay $$dir/events.txt $$out 1 2 $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
		else echo "no $$dir/events.txt: run make golden-bantumi; skipped"; fi; \
	done
	@for dir in golden/pairs2-*; do \
		if [ -f $$dir/events.txt ]; then \
			out=$$(mktemp -d) && $(BUILD)/replay $$dir/events.txt $$out 1 3 $$(cat $$dir/setup.txt) && \
			$(PYTHON) tools/check_replay.py $$out $$dir || exit 1; \
		else echo "no $$dir/events.txt: run make golden-pairs; skipped"; fi; \
	done

# The menu pages against frames of the phone's own, captured in MAME with
# the phone set to English and named after the host frame they must equal.
# Derived from the firmware, so the directory is ignored; skipped when empty.
check-menus: $(BUILD)/frame
	$(PYTHON) tools/check_golden.py $(BUILD)/frame golden/menus

FORCE:

# Game Boy

vpath %.c core $(COMMON)/core $(COMMON)/bantumi platform/gb $(COMMON)/platform/gb $(ASSETS)

# main.c reaches the game at 2x in bank 4 through far.c.
GB_MAIN_FAR := -Dstrip_present=far_strip_present -Dzoom_present=far_zoom_present -Dstrip_leave=far_strip_leave

# games.c reaches the games in banks 2, 4, 5 and 6 through far.c.
$(BUILD)/gb/0/games.rel: GB_EXTRA := -Dsi_handler=far_si_handler -Dsnake2_handler=far_snake2_handler -Dsnake2_redraw=far_snake2_redraw \
	-Dpairs2_handler=far_pairs2_handler -Dpairs2_render=far_pairs2_render \
	-Dbantumi_handler=far_bantumi_handler -Dbantumi_render=far_bantumi_render
# menu.c reaches the titles in bank 3, and the fireworks and the Top score
# and Done pages' animations in bank 6, through far.c.
$(BUILD)/gb/1/menu.rel: GB_EXTRA := -Dtitle_start=far_title_start -Dtitle_elapse=far_title_elapse -Dtitle_draw=far_title_draw \
	-Dfireworks_draw=far_fireworks_draw -Dsparkle_draw=far_sparkle_draw -Ddone_tick_draw=far_done_tick_draw

$(BUILD)/gb/0/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/0
	$(SDCC) $(GB_CFLAGS) $(GB_EXTRA) -c $< -o $@

$(BUILD)/gb/1/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/1
	$(SDCC) $(GB_CFLAGS) $(GB_EXTRA) --codeseg CODE_1 -c $< -o $@

$(BUILD)/gb/2/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/2
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_2 -c $< -o $@

$(BUILD)/gb/3/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/3
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_3 -c $< -o $@

$(BUILD)/gb/4/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/4
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_4 -c $< -o $@

$(BUILD)/gb/5/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/5
	$(SDCC) $(GB_CFLAGS) --codeseg CODE_5 -c $< -o $@

$(BUILD)/gb/6/%.rel: %.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/6
	$(SDCC) $(GB_CFLAGS) $(GB_BANK6_FLAGS) --codeseg CODE_6 -c $< -o $@

# The layer itself twice: once pressing no keys, once pressing KEYS.
$(BUILD)/gb/0/main.rel $(BUILD)/gb/0/main-keys.rel: platform/gb/main.c $(CORE_HDR) $(ASSET_SRC)
	@mkdir -p $(BUILD)/gb/0
	$(SDCC) $(GB_CFLAGS) $(GB_MAIN_FAR) -c $< -o $@

# The keys it presses, in the menus' bank.
$(BUILD)/gb/1/start_keys.rel $(BUILD)/gb/1/start_keys-keys.rel: platform/gb/start_keys.c FORCE
	@mkdir -p $(BUILD)/gb/1
	$(SDCC) $(GB_CFLAGS) '-DSTART_KEYS="$(if $(findstring keys,$@),$(KEYS))"' --codeseg CODE_1 --constseg CODE_1 -c $< -o $@

# Bantumi's search, in its bank.
$(BUILD)/gb/search.rel: $(COMMON)/bantumi/search.s
	@mkdir -p $(BUILD)/gb
	sed 's/_CODE_BANTUMI/_CODE_6/' $< > $(BUILD)/gb/search.s
	$(SDAS) -o $@ $(BUILD)/gb/search.s

$(BUILD)/gb/%.rel: platform/gb/%.s
	@mkdir -p $(BUILD)/gb
	$(SDAS) -o $@ $<

$(BUILD)/gb/%.rel: $(COMMON)/platform/gb/%.s
	@mkdir -p $(BUILD)/gb
	$(SDAS) -o $@ $<

# The screens' half of the made menus' file; the tiles' is in the next bank.
$(BUILD)/gb/7/native_data.rel: GB_PART := -DNATIVE_PART_SCREENS

$(BUILD)/gb/7/%.rel: %.c $(CORE_HDR) $(ASSET_SRC) $(COMMON)/platform/gb/native_gb.h
	@mkdir -p $(BUILD)/gb/7
	$(SDCC) $(GB_CFLAGS) $(GB_PART) --codeseg CODE_7 -c $< -o $@

$(BUILD)/gb/8/%.rel: %.c
	@mkdir -p $(BUILD)/gb/8
	$(SDCC) $(GB_CFLAGS) -DNATIVE_PART_TILES --codeseg CODE_8 -c $< -o $@

# The full-screen menus made at build time, drawn by the host build of the
# core at the Game Boy's size (native_tiles.h).
$(ASSETS)/native_data.c: $(BUILD)/frame_gb
	$(BUILD)/frame_gb native $@

# MBC5 with its rumble pin and battery-backed RAM, sixteen ROM banks (nine
# used) and 8 KiB of RAM. The linker does not mind the first bank running over into the
# second, so its end is checked here.
$(BUILD)/nokia3310%gb: $(BUILD)/gb/crt0.rel $(BUILD)/gb/draw.rel $(BUILD)/gb/band.rel $(BUILD)/gb/search.rel $(BUILD)/gb/native_put.rel $(BUILD)/gb/0/main%rel $(BUILD)/gb/1/start_keys%rel $(GB_RELS)
	$(SDCC) -msm83 --no-std-crt0 -Wl-b_CODE_1=0x14000 -Wl-b_CODE_2=0x24000 -Wl-b_CODE_3=0x34000 -Wl-b_CODE_4=0x44000 -Wl-b_CODE_5=0x54000 -Wl-b_CODE_6=0x64000 -Wl-b_CODE_7=0x74000 -Wl-b_CODE_8=0x84000 \
		-o $(BUILD)/gb/$(basename $(notdir $@)).ihx $^
	@$(PYTHON) $(COMMON)/tools/gb_bank_check.py $(BUILD)/gb/$(basename $(notdir $@)).map
	$(MAKEBIN) -Z -yn NOKIA3310 -yc -yt 0x1e -yo 16 -ya 1 $(BUILD)/gb/$(basename $(notdir $@)).ihx $@

.SECONDARY: $(GB_RELS) $(BUILD)/gb/crt0.rel $(BUILD)/gb/draw.rel $(BUILD)/gb/band.rel $(BUILD)/gb/search.rel $(BUILD)/gb/native_put.rel

gb: $(GB_ROM)

# Headless Game Boy runs with tools/gb_run, built on SameBoy's core.
$(BUILD)/gb_run: tools/gb_run.c
	@test -f "$(SAMEBOY)/build/lib/libsameboy.a" || { echo "Missing $(SAMEBOY)/build/lib/libsameboy.a: run scripts/setup-sameboy.sh, then make -C $(SAMEBOY) lib"; exit 1; }
	@mkdir -p $(BUILD)
	$(CC) -O2 -I$(SAMEBOY) -DGB_VERSION='"x"' -o $@ $< $(SAMEBOY)/build/lib/libsameboy.a -lm

# The host's frame tool with the Game Boy's framebuffer and its 2x game.
$(BUILD)/frame_gb: platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(CORE_HDR) $(ASSET_SRC)
	$(CC) $(CFLAGS) $(GB_FB) $(INCLUDES) -o $@ platform/host/frame_main.c $(COMMON)/platform/host/pgm.c $(COMMON)/tools/native_gen.c $(CORE_SRC) $(ASSET_SRC)

# The ROM presses KEYS at power-on; its screen at each of GB_FRAMES must be
# one of the host's frames for those keys, to the pixel. With the default
# keys that is a game under way in the phone-sized mode; KEYS=adsss is the
# full-screen variant's 2x. Also says how fast the ROM ran the game between
# the screenshots, 100% being the phone's speed.
check-gb: $(GB_TEST_ROM) $(BUILD)/gb_run $(BUILD)/frame_gb
	$(PYTHON) tools/check_gb_run.py $(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(BUILD)/frame_gb "$(KEYS)" $(GB_FRAMES)

# Headless screenshot of the Game Boy ROM after KEYS, as a PNG.
shot-gb: $(GB_TEST_ROM) $(BUILD)/gb_run
	$(BUILD)/gb_run $(GB_TEST_ROM) $(SAMEBOY_BOOT) $(lastword $(GB_FRAMES)) shot:$(BUILD)/nokia3310-gb.pgm
	magick $(BUILD)/nokia3310-gb.pgm -filter point -resize $(SHOT_SCALE)00% $(BUILD)/nokia3310-gb.png

run-gb: $(GB_ROM)
	open -a "$(SAMEBOY_APP)" $(GB_ROM)

# GBA


$(BUILD)/gba/nokia3310.elf $(BUILD)/gba/nokia3310-keys.elf: $(GBA_SRC) $(CORE_HDR) $(COMMON)/platform/gba/gba.ld FORCE
	@mkdir -p $(BUILD)/gba
	$(ARM_CC) $(ARM_CFLAGS) $(GBA_FB) -Icore -I$(COMMON)/core -I$(ASSETS) -I$(COMMON)/platform/gba/include -DNATIVE_PLATFORM_FB '-DSTART_KEYS="$(if $(findstring keys,$@),$(KEYS))"' -nostdlib -T $(COMMON)/platform/gba/gba.ld -Wl,--no-warn-rwx-segments -Wl,-Map,$(basename $@).map -o $@ $(GBA_SRC) -lgcc

# The full-screen menus made at build time, drawn by the host build of the
# core at the GBA's size (native_tiles.h).
$(BUILD)/gba/native_data.c: $(BUILD)/frame_gba
	@mkdir -p $(BUILD)/gba
	$(BUILD)/frame_gba native $@

$(GBA_ROM): $(BUILD)/gba/nokia3310.elf $(COMMON)/tools/gbafix.py FORCE
	$(ARM_OBJCOPY) -O binary $< $@
	$(PYTHON) $(COMMON)/tools/gbafix.py $@ $(if $(GBA_LOGO_FROM),--logo-from "$(GBA_LOGO_FROM)")

$(GBA_TEST_ROM): $(BUILD)/gba/nokia3310-keys.elf $(COMMON)/tools/gbafix.py FORCE
	$(ARM_OBJCOPY) -O binary $< $@
	$(PYTHON) $(COMMON)/tools/gbafix.py $@

gba: $(GBA_ROM)

# Headless GBA runs use mGBA's core library; ../common/scripts/setup-mgba.sh builds it.
MGBA ?= ../tools/mgba
# Screen frames to run before the screenshot; the game starts at power-on.
SHOT_FRAMES ?= 300

$(BUILD)/gba_shot: tools/gba_shot.c
	@test -f "$(MGBA)/build/libmgba.a" || { echo "Missing $(MGBA)/build/libmgba.a: run ../common/scripts/setup-mgba.sh"; exit 1; }
	@mkdir -p $(BUILD)
	$(CC) -O2 -I$(MGBA)/include -I$(MGBA)/build/include -o $@ $< $(MGBA)/build/libmgba.a -lm -framework CoreFoundation

# The ROM presses KEYS at power-on and then runs SHOT_FRAMES frames; its
# screen must be the host's picture after the same keys and about as much
# time. With the default keys that is a game of Space Impact under way. The
# ROM takes some 35 frames to put up its first picture and starts the
# game's clock then, so the host's frames up to 48 earlier are accepted; a
# tick is 5 or 6 frames, so that is one picture out of nine. A screenshot
# may catch the ROM part-way through putting up a picture, so the next few
# frames are tried too.
check-gba: $(GBA_TEST_ROM) $(BUILD)/gba_shot $(BUILD)/frame_gba
	@for later in 0 1 2 3; do \
		$(BUILD)/gba_shot $(GBA_TEST_ROM) $(BUILD)/nokia3310-gba.bmp $$(($(SHOT_FRAMES) + later)) >/dev/null; \
		for back in $$(seq 0 48); do \
			n=$$(($(SHOT_FRAMES) + later - back)); $(BUILD)/frame_gba menu-$(KEYS)+$$n $(BUILD)/gbaframe.pgm >/dev/null; \
			if $(PYTHON) $(COMMON)/tools/check_gb_frame.py $(BUILD)/nokia3310-gba.bmp $(BUILD)/gbaframe.pgm >/dev/null 2>&1; then \
				echo "ok: the ROM's screen $$(($(SHOT_FRAMES) + later)) frames after keys '$(KEYS)' is the host's after $$n"; exit 0; fi; \
		done; \
	done; echo "FAIL: the ROM's screen about $(SHOT_FRAMES) frames after keys '$(KEYS)' matches no host frame near it"; exit 1

shot-gba: $(GBA_TEST_ROM) $(BUILD)/gba_shot
	$(BUILD)/gba_shot $(GBA_TEST_ROM) $(BUILD)/nokia3310-gba.bmp $(SHOT_FRAMES)
	$(PYTHON) $(COMMON)/tools/bmp_to_png.py $(BUILD)/nokia3310-gba.bmp $(BUILD)/nokia3310-gba.png $(SHOT_SCALE)

run-gba: $(GBA_ROM)
	open -a "$(MGBA_APP)" $(GBA_ROM)

# The ROMs to the root of the flash carts' SD cards; see the script.
cards:
	scripts/copy-to-cards.sh

clean:
	rm -rf $(BUILD)
