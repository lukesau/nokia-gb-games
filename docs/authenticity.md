# How faithful the ports are

Each port is written in C, not run from the phone's code, so "the same as
the phone" has to be earned part by part. This page says, for each game and
for the screens around it, which of four things a part is:

- **From the code.** The C follows a routine in the firmware, read in a
  decompile or the disassembly. This is the real re-implementation.
- **From the data.** Read out of your firmware dump at build time: the
  pictures, fonts, text, level tables and sound scripts.
- **From watching the phone.** Built to match the firmware running in MAME,
  with the code that does it never read. Right as far as the recorded runs
  go, and a guess beyond them.
- **The port's own.** Something the phone has no counterpart for, or a
  deliberate difference.

The headless checks (`make check-golden`, `make check-menus`) compare the
port's pictures with recordings of the phone. They prove the pictures
match; they cannot tell the first category from the third, since a part
built from screenshots passes a screenshot test by construction. Only
reading the firmware tells those apart, and that is what this page records.
The addresses, the per-feature ledger and the list of what to trace next
are in [`authenticity-audit.md`](authenticity-audit.md).

## The short version

The games are from the code. The screens around them are from watching
the phone.

Every game's rules, timing, scoring, collisions, level tables and drawing
routines were traced from the firmware: Snake, Memory and Rotation on the
3210; Snake II, Space Impact, Bantumi and Pairs II on the 3310; Snake II,
Space Impact and Bantumi on the 3410. Some of that could not have come
from pictures at all, such as Bantumi's computer player with its two
mistakes, Space Impact's bosses, and Pairs II's shuffle.

The menus, Settings pages, Instructions paging, Game over pages, Top score
page and fireworks were built by watching the phone. Their pixels are
checked; the code behind them was never read. The rules that decide what
happens after a game, such as whether a score beats the record, are guesses
that match every run seen.

## Nokia 3210: Snake, Memory, Rotation

**From the code.** Snake's start position, speed table, key handling, the
grace move after a collision, growth, scoring, food placement and drawing.
Memory's board sizes, deal, cursor, scoring and what happens to a mismatched
pair. Rotation's init, keys, turn phases, opening scramble, solved test,
score and clock. The 3x5 digits, the random generator and the LCD fill and
blit routines.

**From the data.** Every picture, the fonts, the text, Snake's speed table
and food picture, Memory's tiles and board sizes, the Top score sparkle.

**From watching the phone.** All of the menus: the pages, the scrollbar,
the Level bars, Instructions paging. The Games icon animation. The Top
score page and how long it stays. The Game over page and its close after a
key or about three seconds. The game over and new top score sounds. Pause
and Continue. The Last view and how it is dismissed.

**The port's own.** The random seed: the phone never seeds it, so the first
game after power-on is always the same, and the port seeds from uptime
instead. The rule for a new top score. The settings record's check byte. The
Game Boy draws only what changed each tick where the phone redraws the
whole board; a test proves the two agree.

**Not done or not checked.** Snake's board-full ending (+100 and game over)
is traced but not implemented. Snake eating has no recorded run to check
against. Keys 1/3/7/9 are traced but have no console button.

## Nokia 3310

### The framework and the menus

**From the code.** The event loop, return codes, timers and key repeat. The
sprite list with its layers and modes. Terrain strips and pixel collision.
The Sounds and Shakes switches and where they are read. The vibrator call.

**From the data.** Pictures, fonts, text, the sound scripts, the Done note
pictures, the Games icon, the fireworks pictures, the Top score sparkle.

**From watching the phone.** The main menu, the Games list and its path
numbers. The Games icon's timing. The Settings pages. The Done note's
timing. Instructions paging. The Top score page and how long it stays. The
Game over page and its close. The fireworks' sequence, their sound and
that keys are ignored during them. The buzzer's pitch, note length, rest
and repeat commands.

**The port's own.** Sounds start On; the phone starts Off. Lights does
nothing on either. Rumble pulses are half the phone's length on purpose.
Whether a score beats the record and how it is saved. Settings, top scores
and levels live in cartridge RAM. The screen is redrawn whole each frame
where the phone redraws only dirty rectangles. The timer unit is the
3210's value; the 3310's was never settled and MAME measures it a little
shorter.

### Space Impact

**From the code.** Nearly all of it: new game and level load, the HUD, the
ship, its shield, fire and missile, the wall and beam, the spawn script and
enemy groups, movement patterns, enemy fire, collisions, damage, scoring,
bonuses and the score cap, lives and the continue screen, every boss, the
boss explosions, level exits and the ending after level 8.

**From the data.** The spawn lists, level headers, pictures and sounds.

**From watching the phone.** The title's star field and logo timing. The
menu with Continue.

**The port's own.** Pause resumes exactly where it stopped; the phone gives
the ship a short shield on resume. The top score starts at 0.

**Not checked.** Only the first few seconds of level 1 have a recorded run.
Everything past that, the bosses above all, is right by reading.

### Snake II

**From the code.** The board and frame, maze building, speeds, new game,
keys, tick order, collision, edge wrap, the grace tick, growth, the head,
neck, corner and tail pieces, food placement, the bonus creature's spawn
and countdown, and death. The large creature is traced but the phone can
never reach it, so neither can the port.

**From the data.** The mazes, the speed table, the pictures and sounds.

**From watching the phone.** The death blinks' timing. The sounds'
lengths. The Level and Mazes pages, the "selected" note and the
Instructions.

**The port's own.** The game over rumble is a full pulse; the phone cuts
it almost at once. Resume keeps the period but not the phone's suspend
packing.

**Not checked.** Mazes 4 and 5 were never run on the phone.

### Pairs II

**From the code.** The deal, the shuffle, the board shapes, the times, the
Puzzle grid, scoring, the fuse and its explosion, the bonus, Puzzle's card
removal, the level periods and resume.

**From the data.** The layout tables, pictures and sounds.

**From watching the phone.** The timing of the saloon door and wipe, the
cursor and the enlarged pictures; their drawing is from the code. The mode
list, the Level page and the records per mode.

**The port's own.** The "new top score" text after a game is inferred.

**Not checked.** Resume in the middle of a Time trial game: the port and
the map disagree about the flame and no run settles it. Only the slowest
and fastest level periods were measured.

### Bantumi

**From the code.** The rules: sowing, captures, the extra turn and the end.
The hand's paths. The computer player's search, including how far it
looks, how it orders moves, its two mistakes and the hint, checked on all
112 searches seen in a run. The winner's store blinking.

**From the data.** The pictures, digits and text.

**From watching the phone.** The hand's widths. The game over pages. The
Level page and the Instructions.

**The port's own.** The search is rewritten in Game Boy assembly, checked
against the C only. The loser's store is drawn steady where the phone
blinks it. The power-on value of a carried-over draw mode is a guess.

**Not checked.** Level 4 was never run.

## Nokia 3410

### Snake II

**From the code.** The title animation, the board, maze building, speeds,
new game, tick, collision, the grace tick, head and tail, food, creatures,
scoring, death, the game over picture and box, its sounds, the top score
per maze and the High scores page.

**From the data.** The mazes, speeds, pictures, fonts and sounds. The
sound ids go through two lookup tables in the firmware, both traced.

**From watching the phone.** The game over blink and how long the page
stays; the traced code counts a little less than what MAME shows. The
random generator. The menu pages and their geometry.

**Borrowed from the 3310.** The key handling, the framework's events and
the timer unit are the 3310's tracing; the 3410's own were not read.

**The port's own.** The death vibration's length is a round
figure between traced on and off points. The last game's record is kept
in RAM where the phone stores it. The title ends on any key but Back.

**Not checked.** Only No maze and Tunnel were run on the phone. The
recorded runs check the order of frames, not their timing, and inject
directions rather than keys.

### Space Impact

The game itself is the 3310's tracing with the 3410's data and drawing;
see above. Its sounds, vibrator and the three Instructions demos are from
the 3410's code and checked against recordings, sound for sound. The
Instructions pages around the demos (which keys page, start a demo or
leave) are from watching the phone.

### Bantumi

The rules and the search are the 3310's code, shared. From the 3410's own
code: the drawing, digits and hand mask, the intro, the game over pause,
blink and slide out, pass and hint, Pause and Continue, the sounds, the
title and the Instructions demos. From watching the phone: the Instructions
text pages, the menu, Game options, No. of beans and Level pages, and when
the framework redraws.

### Munkiki's Castles

The phone's one Java game, a MIDlet its KVM runs; the port is a
re-implementation in C from a decompile of its JAR
(`3410/docs/munkiki.md`).

**From the code.** The world and its rules (class `d`: Munkiki's moves,
jumps, falls, pushing, breaking, riding, the notes, keys, warps and lives,
the levels' end and the bonus count), the flow of screens (`KKM` and the
canvas `b`: title, lists, hint and level texts, map, game over, High
scores, Options, the six Instructions demos), the timer's 100 ms tick,
and the camera: `make check-munkiki` runs the JAR's own classes on a JVM
beside the port and compares the whole state after every tick.

**From the data.** The levels, the pictures, Munkiki's frames, the 3D
geometry and its grays, the animation tables, the English texts and the
twelve ringing tones, all read from the JAR at build time.

**From watching the phone.** The fonts the KVM draws the game's text in
and where a glyph sits on its line; the style of the game's lists (title,
rows, bar, scrollbar, soft keys); the dither patterns and their phase; the
horizon's row and where the floor's edges fall, which fixed the
projection's row scale at 49 where the frustum gives 48.

**The port's own.** The rasterizer: the phone's M3D was not read, so the
picture is the same scene from the same camera in the same grays, drawn
by the port's own fixed-point filler and painter's order, not the phone's
pixels. Its place in the menus: sixth in Select game, where the phone has
it under Games, More games, as "Munkiki" (there is no More games here).
Continue goes on with this session's game rather than the JAR's saved
snapshot. The Instructions' text pages and Options are drawn in the list
style, their Form pages not having been captured. The tones' tempo is
assumed (none is set in the data).

**Not checked.** The picture against the phone beyond what is listed:
the 3D frames are compared by eye. The phone's rendering of the tones
was not heard (MAME's DSP does not play them). The Game Boy keeps the
phone's pace by showing fewer pictures when a tick's picture takes longer
than the tick, which it does: about three a second at the first level's
start.

### The pages around the games

The Games page, Settings pages and Instructions layout are carried over
from the 3310 port. The 3410's own pages for these were never mapped or
measured, and the main menu's Games icon is missing, which `make
check-menus` reports.

## What would move the most

Reading the post-game code on each phone would turn the Game over page,
the Top score page, the fireworks and the score-beats-record rule from
watching into code. Reading the menu framework would do the same for every
menu page, though their pixels are already checked. The list, in order, is
at the end of [`authenticity-audit.md`](authenticity-audit.md).
