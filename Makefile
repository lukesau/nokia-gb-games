# Builds and checks the three phones' ports from the top. Each port is a
# project of its own in its directory with its own Makefile; these targets
# run a target in every port that has it, all of them even when one fails,
# and then name the ports that failed. `make -C 3310 <target>` runs one.

PORTS := 3210 3310 3410
GBA_PORTS := 3210 3310 3410

.PHONY: help gb gba test check-golden check-menus check-gb check-gba cards setup clean

help:
	@echo "make gb             the Game Boy ROMs of all three ports"
	@echo "make gba            the GBA ROMs"
	@echo "make test           the host checks of every port"
	@echo "make check-golden   every port's games against recorded runs of the firmware"
	@echo "make check-menus    the 3310's and 3410's menu pages against the phones' own"
	@echo "make check-gb       each Game Boy ROM, run headlessly, against the host"
	@echo "make check-gba      each GBA ROM, run headlessly, against the host"
	@echo "make setup          clone and build SameBoy and mGBA into tools/"
	@echo "make cards          copy every built ROM to the flash carts' SD cards"
	@echo "make -C 3310 help   one port's own targets"

each = @failed=; for p in $(1); do $(MAKE) -C $$p $@ || failed="$$failed $$p"; done; \
	test -z "$$failed" || { echo "$@ failed in:$$failed"; exit 1; }

gb test check-golden check-gb cards clean:
	$(call each,$(PORTS))

gba check-gba:
	$(call each,$(GBA_PORTS))

check-menus:
	$(call each,3310 3410)

setup:
	3310/scripts/setup-sameboy.sh
	common/scripts/setup-mgba.sh
