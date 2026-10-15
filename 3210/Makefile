# The Nokia 3210's games: Rotation here, Snake and Memory shared with the
# 5110 in ../common/classic, built by ../common/classic/classic.mk.

PHONE := 3210
# Your own NSE-8/9 v6.00 dump (raw .fls or the swap16 image); see README.md.
DUMP ?= ../../../roms/3210f600a.fls
GBA_CODE := N32E
# The save signature's second byte; the 3210 keeps the one its saves have.
GBA_SAVE_ID := 3
PHONE_SRC := core/rotation.c
CLASSIC_SRC :=

include ../common/classic/classic.mk
