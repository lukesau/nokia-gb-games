;; Game Boy startup: interrupt vectors, RAM set-up, and a vertical-blank
;; handler that counts frames. makebin fills in the cartridge header.
	.module crt0
	.globl	_main
	.globl	_flush_tiles
	.globl	_frame_count, _pad_last, _pad_latch
	.globl	_sound_frame
	.globl	_staged, _staged_at, _staged_count, _staged_from
	.globl	_lcd_column_fill, _lcd_column_blit, _zoom_tile, _zoom_take, _zoom_right
	.globl	_lcd_column_rows, _lcd_column_color, _lcd_column_bits
	.globl	s__INITIALIZER, s__INITIALIZED, l__INITIALIZER

	.area	_HEADER (ABS)
	.org	0x40		; vertical blank
	jp	vblank
	.org	0x48		; LCD status
	jp	lcd_split
	.org	0x50		; timer
	reti
	.org	0x58		; serial
	reti
	.org	0x60		; joypad
	reti

	.org	0x100
	nop
	jp	init

	.org	0x150
init:
	di
	ld	sp, #0xe000

	;; Clear work RAM.
	ld	hl, #0xc000
	ld	bc, #0x2000
1$:
	xor	a, a
	ld	(hl+), a
	dec	bc
	ld	a, b
	or	a, c
	jr	nz, 1$

	;; Copy the initial values of initialised variables.
	ld	hl, #s__INITIALIZER
	ld	de, #s__INITIALIZED
	ld	bc, #l__INITIALIZER
2$:
	ld	a, b
	or	a, c
	jr	z, 3$
	ld	a, (hl+)
	ld	(de), a
	inc	de
	dec	bc
	jr	2$
3$:
	call	_main
4$:
	halt
	jr	4$

	;; Order of the areas for the linker.
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area	_GSINIT
	.area	_GSFINAL
	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area	_BSS
	.area	_HEAP

	;; The handlers and helpers go with the rest of the code: the header
	;; area above is at fixed addresses and ends where the code begins.
	.area	_HOME
vblank:
	push	af
	push	hl
	ld	hl, #_frame_count
	inc	(hl)
	;; The top of the screen takes its tiles from 0x8000.
	ld	hl, #0xff40
	set	4, (hl)

	;; Read the pad every frame and latch new presses, so none is lost
	;; or delayed while the main loop is drawing. Buttons go in the low
	;; half of the byte and directions in the high half.
	ld	a, #0x10		; select the buttons
	ld	(#0xff00), a
	ld	a, (#0xff00)
	ld	a, (#0xff00)
	cpl
	and	a, #0x0f
	ld	l, a
	ld	a, #0x20		; select the directions
	ld	(#0xff00), a
	ld	a, (#0xff00)
	ld	a, (#0xff00)
	ld	a, (#0xff00)
	ld	a, (#0xff00)
	cpl
	and	a, #0x0f
	swap	a
	or	a, l
	ld	l, a			; keys held now
	ld	a, #0x30
	ld	(#0xff00), a
	ld	a, (#_pad_last)
	cpl
	and	a, l			; keys newly pressed
	ld	h, a
	ld	a, (#_pad_latch)
	or	a, h
	ld	(#_pad_latch), a
	ld	a, l
	ld	(#_pad_last), a

	;; Advance the sound in progress.
	push	bc
	push	de
	call	_sound_frame
	pop	de
	pop	bc
	pop	hl
	pop	af
	reti

;; Raised on the line before the one where the screen's tiles continue at
;; 0x9000. Waits for that line's horizontal blank, so the switch never
;; lands in the middle of a drawn line.
lcd_split:
	push	af
	push	hl
	ld	hl, #0xff41
1$:
	ld	a, (hl)
	and	a, #0x03
	jr	nz, 1$
	ld	hl, #0xff40
	res	4, (hl)
	pop	hl
	pop	af
	reti

;; void flush_tiles(void): copies staged_count 16-byte tiles from staged to
;; the video RAM addresses in staged_at, with the LCD on. Video RAM accepts
;; writes only outside the part of each line where the LCD controller is
;; drawing, so each four bytes wait for a horizontal or vertical blank. The
;; check takes 4 cycles after it reads the status and the writes 12, so even
;; when the blank ends right after the read they land inside the 20 cycles
;; of the following line's OAM scan, which still accepts them. Interrupts
;; are held off between the check and the writes.
_flush_tiles::
	ld	a, (#_staged_count)
	or	a, a
	ret	z
	ld	hl, #_staged
	ld	a, l
	ld	(#_staged_from), a
	ld	a, h
	ld	(#_staged_from + 1), a
	ld	hl, #_staged_at
	ld	a, (#_staged_count)
1$:
	push	af			; tiles left
	ld	a, (hl+)
	ld	c, a
	ld	a, (hl+)
	push	hl			; the next address in staged_at
	ld	h, a
	ld	l, c			; hl = where this tile goes
2$:
	push	hl
	ld	a, (#_staged_from)
	ld	l, a
	ld	a, (#_staged_from + 1)
	ld	h, a			; hl = the next four bytes of data
	ld	a, (hl+)
	ld	b, a
	ld	a, (hl+)
	ld	c, a
	ld	a, (hl+)
	ld	d, a
	ld	a, (hl+)
	ld	e, a
	ld	a, l
	ld	(#_staged_from), a
	ld	a, h
	ld	(#_staged_from + 1), a
	pop	hl
	di
3$:
	ld	a, (#0xff41)
	and	a, #0x02
	jr	nz, 3$			; drawing, or about to: wait
	ld	(hl), b
	inc	l
	ld	(hl), c
	inc	l
	ld	(hl), d
	inc	l
	ld	(hl), e
	inc	l			; tiles start on 16-byte boundaries
	ei
	ld	a, l
	and	a, #0x0f
	jr	nz, 2$
	pop	hl
	pop	af
	dec	a
	jr	nz, 1$
	ret

;; The core's two innermost drawing loops (see core/lcd.h), which the
;; compiler makes many times slower than this. A framebuffer row is 20
;; bytes. Both take the first byte's address in de and the mask in a.

;; void lcd_column_fill(uint8_t *p, uint8_t mask)
_lcd_column_fill::
	ld	c, a
	ld	h, d
	ld	l, e
	ld	de, #20
	ld	a, (#_lcd_column_rows)
	ld	b, a
	ld	a, (#_lcd_column_color)
	or	a, a
	jr	z, 3$
	dec	a
	jr	z, 2$
1$:					; invert
	ld	a, (hl)
	xor	a, c
	ld	(hl), a
	add	hl, de
	dec	b
	jr	nz, 1$
	ret
2$:					; set
	ld	a, (hl)
	or	a, c
	ld	(hl), a
	add	hl, de
	dec	b
	jr	nz, 2$
	ret
3$:					; clear
	ld	a, c
	cpl
	ld	c, a
4$:
	ld	a, (hl)
	and	a, c
	ld	(hl), a
	add	hl, de
	dec	b
	jr	nz, 4$
	ret

;; void lcd_column_blit(uint8_t *p, uint8_t mask)
_lcd_column_blit::
	ld	c, a
	ld	h, d
	ld	l, e
	ld	a, (#_lcd_column_rows)
	ld	b, a
	ld	a, (#_lcd_column_bits)
	ld	d, a
1$:
	ld	a, (hl)
	or	a, c			; set the pixel
	srl	d
	jr	c, 2$
	xor	a, c			; or clear it
2$:
	ld	(hl), a
	ld	a, l
	add	a, #20
	ld	l, a
	jr	nc, 3$
	inc	h
3$:
	dec	b
	jr	nz, 1$
	ret

;; The magnified rectangle's two inner loops (see platform/gb/main.c).

;; uint8_t zoom_take(const uint8_t *src, uint8_t *was): src in de, was in
;; bc. Four rows: lcd_fb's are 20 bytes apart, the copy's 10.
_zoom_take::
	ld	h, b
	ld	l, c
	ld	bc, #0x0004		; b = bits that differed, c = rows left
1$:
	ld	a, (de)
	xor	a, (hl)
	or	a, b
	ld	b, a
	ld	a, (de)
	ld	(hl), a
	ld	a, e
	add	a, #20
	ld	e, a
	jr	nc, 2$
	inc	d
2$:
	ld	a, l
	add	a, #10
	ld	l, a
	jr	nc, 3$
	inc	h
3$:
	dec	c
	jr	nz, 1$
	ld	a, b
	ret

;; void zoom_tile(const uint8_t *src, uint8_t *tile): src in de, tile in bc.
;; Each of four rows gives two tile rows of two bit planes: four bytes.
_zoom_tile::
	call	zoom_row
	call	zoom_row
	call	zoom_row
zoom_row:
	ld	a, (#_zoom_right)
	or	a, a
	ld	a, (de)
	jr	nz, 1$
	swap	a
1$:
	and	a, #0x0f
	add	a, #<doubled
	ld	l, a
	ld	a, #0
	adc	a, #>doubled
	ld	h, a
	ld	a, (hl)
	ld	(bc), a
	inc	bc
	ld	(bc), a
	inc	bc
	ld	(bc), a
	inc	bc
	ld	(bc), a
	inc	bc
	ld	a, e
	add	a, #20
	ld	e, a
	ret	nc
	inc	d
	ret

;; A half byte with each of its pixels doubled.
doubled:
	.db	0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f
	.db	0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff
