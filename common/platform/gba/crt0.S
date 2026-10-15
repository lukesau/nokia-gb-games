@ GBA cartridge header and startup. tools/gbafix.py fills in the header
@ checksum (and the logo, when given one) after linking. The port's
@ Makefile names the cartridge: GBA_TITLE, up to 12 characters, and
@ GBA_CODE, 4.
    .section .crt0, "ax"
    .arm
    .global _start
_start:
    b       reset
    .fill   156, 1, 0           @ logo
.Ltitle:
    .ascii  GBA_TITLE
    .fill   12 - (. - .Ltitle), 1, 0
    .ascii  GBA_CODE            @ game code
    .ascii  "00"                @ maker code
    .byte   0x96                @ fixed value
    .byte   0                   @ main unit code
    .byte   0                   @ device type
    .fill   7, 1, 0
    .byte   0                   @ software version
    .byte   0                   @ header checksum
    .fill   2, 1, 0
    @ Bytes 0xc0 to 0xdf: the cartridge's general-purpose port, which the
    @ rumble motor hangs off, answers at 0xc4 to 0xc9 (see main.c), so no
    @ code goes here.
    b       reset
    .fill   28, 1, 0

reset:
    @ IRQ mode stack, then system mode stack.
    mov     r0, #0x12
    msr     cpsr_c, r0
    ldr     sp, =__sp_irq
    mov     r0, #0x1f
    msr     cpsr_c, r0
    ldr     sp, =__sp_usr

    @ Copy .data from ROM to IWRAM.
    ldr     r0, =__data_lma
    ldr     r1, =__data_start
    ldr     r2, =__data_end
1:  cmp     r1, r2
    ldrlo   r3, [r0], #4
    strlo   r3, [r1], #4
    blo     1b

    @ Zero .bss.
    ldr     r1, =__bss_start
    ldr     r2, =__bss_end
    mov     r3, #0
2:  cmp     r1, r2
    strlo   r3, [r1], #4
    blo     2b

    ldr     r0, =main
    mov     lr, pc
    bx      r0
3:  b       3b

@ Interrupt handler, entered from the BIOS in ARM mode with r0-r3, r12 and
@ lr saved. Acknowledges whatever fired, counts vertical blanks and moves
@ the sound in progress on at each interrupt of timer 2.
    .global irq_handler
irq_handler:
    mov     r0, #0x04000000
    add     r0, r0, #0x200
    ldr     r1, [r0]            @ IE in the low half, IF in the high half
    and     r1, r1, r1, lsr #16
    strh    r1, [r0, #2]        @ acknowledge in IF
    ldr     r2, =0x03007ff8     @ and in the BIOS's copy
    ldrh    r3, [r2]
    orr     r3, r3, r1
    strh    r3, [r2]
    tst     r1, #1              @ vertical blank
    beq     1f
    ldr     r2, =frame_count
    ldr     r3, [r2]
    add     r3, r3, #1
    str     r3, [r2]
    @ Read the pad every frame and latch new presses, so none is lost or
    @ delayed while the main loop is drawing.
    ldr     r2, =0x04000130
    ldrh    r3, [r2]
    mvn     r3, r3              @ keys held now (the register is active low)
    ldr     r2, =pad_last
    ldr     r0, [r2]
    str     r3, [r2]
    bic     r0, r3, r0          @ keys newly pressed
    ldr     r2, =pad_latch
    ldr     r3, [r2]
    orr     r3, r3, r0
    str     r3, [r2]
1:  tst     r1, #0x20           @ timer 2
    bxeq    lr
    stmfd   sp!, {lr}
    ldr     r0, =sound_tick
    mov     lr, pc
    bx      r0
    ldmfd   sp!, {lr}
    bx      lr
    .pool
