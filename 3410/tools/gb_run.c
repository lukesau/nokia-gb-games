/* Runs a Game Boy ROM headlessly in SameBoy's core with scripted buttons.

   Usage: gb_run ROM.gb BOOT_ROM STEP...

   The boot ROM picks the model: cgb_boot.bin a Game Boy Color, agb_boot.bin
   a Game Boy Advance, anything else the original Game Boy.

   Each STEP is one of:
     N            run N screen frames
     +B / -B      press / release a button: a b s(elect) t(start) u d l r
     shot:FILE    write the screen as a binary PGM (160x144)
     rumble       from here on, print "rumble FRAME SHARE" for every frame
                  in which the cartridge's rumble motor was on, SHARE being
                  the part of the frame it was on for
     audio:FILE   from here on, write the sound's left channel to FILE as
                  raw signed 16-bit samples at 32768 Hz
     peek:ADDR:N  print N bytes of memory from hexadecimal ADDR
     writes:LOW:HIGH:COUNT  print the next COUNT writes to the hexadecimal
                  address range, and the instruction that made each
     hits:N:ADDR  run N screen frames and print how often the instruction
                  at hexadecimal ADDR was reached; ADDR@BANK counts it in
                  that ROM bank only
     spmin:N      run N screen frames and print the lowest the stack
                  pointer went
     times:N:ADDR,ADDR...  run N screen frames and print the cycle at
                  which each of the instructions at those addresses is
                  reached; one written !ADDR ends the step when reached
     prof:N:FILE  run N screen frames, then write where the time went: a
                  line "BANK ADDR CYCLES" per instruction address, the bank
                  being the ROM bank mapped at 0x4000 (see common/tools/gb_profile.py)

   Built against the library `make lib` makes in the SameBoy clone. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Core/gb.h>

static uint32_t pixels[160 * 144];

static uint32_t rgb(GB_gameboy_t *gb, uint8_t r, uint8_t g, uint8_t b)
{
    (void)gb;
    (void)g;
    (void)b;
    return r;
}

/* For the audio step. */
static FILE *audio;

/* For the rumble step: frames run so far. */
static unsigned long frames_run;

static void on_rumble(GB_gameboy_t *gb, double amplitude)
{
    (void)gb;
    if (amplitude > 0)
        printf("rumble %lu %.2f\n", frames_run, amplitude);
}

static void on_sample(GB_gameboy_t *gb, GB_sample_t *sample)
{
    (void)gb;
    if (audio)
        fwrite(&sample->left, sizeof sample->left, 1, audio);
}

/* For the writes step. */
static unsigned watch_low, watch_high, watch_left;

static bool on_write(GB_gameboy_t *gb, uint16_t addr, uint8_t data)
{
    if (addr >= watch_low && addr <= watch_high && watch_left) {
        watch_left--;
        printf("write %04x = %02x from %04x (hl %04x de %04x)\n", addr, data, GB_get_registers(gb)->pc,
               GB_get_registers(gb)->hl, GB_get_registers(gb)->de);
    }
    return true;
}

static int button(char c)
{
    switch (c) {
    case 'a': return GB_KEY_A;
    case 'b': return GB_KEY_B;
    case 's': return GB_KEY_SELECT;
    case 't': return GB_KEY_START;
    case 'u': return GB_KEY_UP;
    case 'd': return GB_KEY_DOWN;
    case 'l': return GB_KEY_LEFT;
    case 'r': return GB_KEY_RIGHT;
    }
    fprintf(stderr, "unknown button %c\n", c);
    exit(2);
}

int main(int argc, char **argv)
{
    GB_gameboy_t *gb;
    int i;

    if (argc < 3) {
        fprintf(stderr, "usage: gb_run ROM.gb BOOT_ROM STEP...\n");
        return 2;
    }
    /* The model the boot ROM is for: a Game Boy Color's or a Game Boy
       Advance's (cgb_boot.bin, agb_boot.bin), else the original. */
    gb = GB_init(GB_alloc(), strstr(argv[2], "agb_boot") ? GB_MODEL_AGB
                             : strstr(argv[2], "cgb_boot") ? GB_MODEL_CGB_E : GB_MODEL_DMG_B);
    if (GB_load_boot_rom(gb, argv[2]) || GB_load_rom(gb, argv[1])) {
        fprintf(stderr, "cannot load %s or %s\n", argv[2], argv[1]);
        return 1;
    }
    GB_set_pixels_output(gb, pixels);
    GB_set_rgb_encode_callback(gb, rgb);
    GB_set_sample_rate(gb, 32768);
    /* As fast as it goes, not at the Game Boy's own pace. */
    GB_set_turbo_mode(gb, true, true);
    GB_apu_set_sample_callback(gb, on_sample);
    for (i = 3; i < argc; i++) {
        const char *step = argv[i];

        if (step[0] == '+' || step[0] == '-') {
            GB_set_key_state(gb, button(step[1]), step[0] == '+');
        } else if (strncmp(step, "shot:", 5) == 0) {
            FILE *out = fopen(step + 5, "wb");
            int p;

            if (!out) {
                perror(step + 5);
                return 1;
            }
            fprintf(out, "P5\n160 144\n255\n");
            for (p = 0; p < 160 * 144; p++)
                fputc((int)(pixels[p] & 0xff) < 128 ? 0 : 255, out);
            fclose(out);
        } else if (strcmp(step, "rumble") == 0) {
            GB_set_rumble_mode(gb, GB_RUMBLE_CARTRIDGE_ONLY);
            GB_set_rumble_callback(gb, on_rumble);
        } else if (strncmp(step, "audio:", 6) == 0) {
            audio = fopen(step + 6, "wb");
            if (!audio) {
                perror(step + 6);
                return 1;
            }
        } else if (strncmp(step, "peek:", 5) == 0) {
            unsigned addr = 0, n = 1, k;

            sscanf(step + 5, "%x:%u", &addr, &n);
            printf("%04x:", addr);
            for (k = 0; k < n; k++)
                printf(" %02x", GB_read_memory(gb, (uint16_t)(addr + k)));
            printf("\n");
        } else if (strncmp(step, "prof:", 5) == 0) {
            static uint32_t spent[32][0x10000];
            char *colon = NULL;
            long frames = strtol(step + 5, &colon, 10);
            uint64_t left = (uint64_t)frames * 70224 * 2; /* 8 MHz units in a frame */
            FILE *out = fopen(colon && *colon ? colon + 1 : "", "w");
            unsigned bank, pc;

            if (!out) {
                perror(step);
                return 1;
            }
            while (left > 0) {
                uint16_t mapped = 0;
                unsigned cycles;

                pc = GB_get_registers(gb)->pc;
                GB_get_direct_access(gb, GB_DIRECT_ACCESS_ROM, NULL, &mapped);
                cycles = GB_run(gb);
                spent[mapped & 31][pc] += cycles;
                left = left > cycles ? left - cycles : 0;
            }
            for (bank = 0; bank < 32; bank++)
                for (pc = 0; pc < 0x10000; pc++)
                    if (spent[bank][pc])
                        fprintf(out, "%u %04x %u\n", bank, pc, spent[bank][pc]);
            fclose(out);
        } else if (strncmp(step, "writes:", 7) == 0) {
            /* writes:LOW:HIGH:COUNT prints the next COUNT writes to the
               hexadecimal address range, with where they came from. */
            sscanf(step + 7, "%x:%x:%u", &watch_low, &watch_high, &watch_left);
            GB_set_write_memory_callback(gb, on_write);
        } else if (strncmp(step, "hits:", 5) == 0) {
            /* hits:N:ADDR runs N frames and prints how often the instruction
               at hexadecimal ADDR was reached. */
            char *colon = NULL, *at = NULL;
            long frames = strtol(step + 5, &colon, 10);
            unsigned addr = colon && *colon ? (unsigned)strtoul(colon + 1, &at, 16) : 0, hits = 0;
            int bank = at && *at == '@' ? (int)strtol(at + 1, NULL, 10) : -1; /* ADDR@BANK: in that ROM bank only */
            uint64_t left = (uint64_t)frames * 70224 * 2;

            while (left > 0) {
                unsigned cycles;
                uint16_t mapped = 0;

                if (GB_get_registers(gb)->pc == addr) {
                    if (bank >= 0)
                        GB_get_direct_access(gb, GB_DIRECT_ACCESS_ROM, NULL, &mapped);
                    if (bank < 0 || mapped == bank)
                        hits++;
                }
                cycles = GB_run(gb);
                left = left > cycles ? left - cycles : 0;
            }
            printf("%04x: reached %u times in %ld frames\n", addr, hits, frames);
        } else if (strncmp(step, "spmin:", 6) == 0) {
            /* spmin:N runs N frames and prints the lowest the stack pointer
               went. */
            long frames = strtol(step + 6, NULL, 10);
            uint64_t left = (uint64_t)frames * 70224 * 2;
            unsigned low = 0xffff;

            while (left > 0) {
                unsigned cycles, sp = GB_get_registers(gb)->sp;

                if (sp < low)
                    low = sp;
                cycles = GB_run(gb);
                left = left > cycles ? left - cycles : 0;
            }
            printf("stack pointer's lowest: %04x in %ld frames\n", low, frames);
        } else if (strncmp(step, "times:", 6) == 0) {
            /* times:N:ADDR,ADDR... runs N frames and prints the cycle,
               counted from the step's start, at which each of the
               instructions at those hexadecimal addresses is reached. */
            char *p = NULL;
            long frames = strtol(step + 6, &p, 10);
            unsigned addrs[8], n = 0, i, stop = 0x10000;
            uint64_t left = (uint64_t)frames * 70224 * 2, at = 0;

            while (p && *p && n < 8) {
                if (p[1] == '!') {
                    /* !ADDR: stop when it is reached. */
                    p++;
                    stop = (unsigned)strtoul(p + 1, &p, 16);
                    addrs[n++] = stop;
                    continue;
                }
                addrs[n++] = (unsigned)strtoul(p + 1, &p, 16);
            }
            while (left > 0) {
                unsigned cycles, pc = GB_get_registers(gb)->pc, sp = GB_get_registers(gb)->sp, now_sp;

                cycles = GB_run(gb);
                /* Reached, unless an interrupt was taken instead, which
                   comes back to it later: its return address, this one,
                   on the stack two lower. */
                now_sp = GB_get_registers(gb)->sp;
                if (!(now_sp == ((sp - 2) & 0xffff)
                      && (GB_safe_read_memory(gb, (uint16_t)now_sp)
                          | GB_safe_read_memory(gb, (uint16_t)(now_sp + 1)) << 8) == pc)) {
                    for (i = 0; i < n; i++)
                        if (pc == addrs[i])
                            printf("%04x at %llu\n", pc, (unsigned long long)(at / 2));
                    if (pc == stop)
                        break;
                }
                at += cycles;
                left = left > cycles ? left - cycles : 0;
            }
        } else {
            long frames = strtol(step, NULL, 10);

            while (frames-- > 0) {
                GB_run_frame(gb);
                frames_run++;
            }
        }
    }
    if (audio)
        fclose(audio);
    return 0;
}
