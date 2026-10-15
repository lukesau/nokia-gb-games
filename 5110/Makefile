# The Nokia 5110's games: Snake, Memory and Logic, all shared with the 3210
# in ../common/classic, built by ../common/classic/classic.mk.

PHONE := 5110
# Your own NSE-1 v5.28 dump (raw .fls or the swap16 image); see README.md.
DUMP ?= ../../../roms/noki5110/5110f528a.fls
GBA_CODE := N51E
GBA_SAVE_ID := 5
PHONE_SRC :=
CLASSIC_SRC := logic.c

include ../common/classic/classic.mk

# The RE fork, for its Unicorn harness, and a Python with unicorn in it.
DCT3_RE ?= ../../..
HARNESS_PYTHON ?= $(DCT3_RE)/.venv/bin/python

# MAME does not run v5.28, so the goldens come from the fork's harnesses:
# the games menus and Logic's games.
golden: golden-menus golden-logic

golden-menus:
	$(HARNESS_PYTHON) tools/golden_menus.py $(DCT3_RE) "$(DUMP)" $(GOLDEN)

golden-logic:
	$(HARNESS_PYTHON) tools/golden_logic.py $(DCT3_RE) "$(DUMP)" $(GOLDEN)

.PHONY: golden golden-menus golden-logic
