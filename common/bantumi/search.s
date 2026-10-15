;; The Game Boy's own version of Bantumi's search (search() in
;; bantumi.c, which says what each part does): a level 5 search takes the
;; phone's hundred steps a tick, more than the compiler's code can do in
;; the Game Boy's time. The same steps on the same nodes, so that the
;; phone's hand sets out on the same tick for the same pit. In Bantumi's
;; bank, which each phone's Makefile puts for _CODE_BANTUMI; its state is
;; bantumi.c's.
	.module search
	.globl	_bantumi_search_gb
	.globl	_bantumi_cur, _bantumi_root, _bantumi_board
	.globl	_bantumi_depth, _bantumi_pick, _bantumi_search_steps, _bantumi_search_done

;; A node (struct node), 24 bytes.
N_SIDE = 0
N_ALPHA = 1
N_BETA = 3
N_BEST = 5
N_PITS = 7
N_FIRST = 21
N_COUNTER = 22
N_EMPTY = 23
N_SIZE = 24

SIDE_PLAYER = 3
SIDE_PHONE = 4
NO_MOVE = 0x0e
STORE = 6
PHONE_STORE = 13

	.area	_DATA
move:
	.ds	1
side:
	.ds	1
base:
	.ds	1
best:
	.ds	1
found:
	.ds	1
terminal:
	.ds	1
pits:
	.ds	2
parent:
	.ds	2
child:
	.ds	2

	.area	_CODE_BANTUMI

;; hl = the node being worked on.
cur_hl:
	ld	hl, #_bantumi_cur
	ld	a, (hl+)
	ld	h, (hl)
	ld	l, a
	ret

;; hl = hl + a.
add_hl_a:
	add	a, l
	ld	l, a
	adc	a, h
	sub	a, l
	ld	h, a
	ret

;; void bantumi_search_gb(void)
_bantumi_search_gb::
	xor	a, a
	ld	(#_bantumi_search_done), a
step:
	call	cur_hl
	ld	a, (#_bantumi_depth)
	or	a, a
	jp	z, not_expanding
	push	hl
	ld	a, #N_COUNTER
	call	add_hl_a
	ld	a, (hl)
	pop	hl
	cp	a, #6
	jp	nc, not_expanding
	; best < beta, signed: with the sign bits flipped, unsigned
	push	hl
	ld	a, #N_BEST
	call	add_hl_a
	ld	a, (hl+)
	ld	c, a
	ld	a, (hl+)
	xor	a, #0x80
	ld	b, a			; bc = best, sign flipped
	pop	hl
	push	hl
	inc	hl			; N_BETA = 3
	inc	hl
	inc	hl
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl)
	xor	a, #0x80
	ld	d, a			; de = beta
	pop	hl
	ld	a, c
	sub	a, e
	ld	a, b
	sbc	a, d
	jp	nc, not_expanding	; best >= beta
	push	hl
	ld	a, #N_EMPTY
	call	add_hl_a
	ld	a, (hl)
	pop	hl
	or	a, a
	jr	z, expanding
	ld	a, #1
	call	close_node
	jp	next_step
expanding:
	call	next_move
	ld	(#move), a
	call	cur_hl
	push	hl
	ld	a, #N_COUNTER
	call	add_hl_a
	ld	a, (hl)
	pop	hl
	cp	a, #7
	jp	nc, next_step		; counter > 6
	ld	a, (#move)
	add	a, #N_PITS
	call	add_hl_a
	ld	a, (hl)
	or	a, a
	jp	z, next_step		; an empty pit
	ld	a, (#move)
	call	make_child
	jp	next_step
not_expanding:
	ld	a, (#_bantumi_root)
	cp	a, l
	jr	nz, closing
	ld	a, (#_bantumi_root + 1)
	cp	a, h
	jr	nz, closing
	ld	a, #1
	ld	(#_bantumi_search_done), a
	ret
closing:
	xor	a, a
	call	close_node
next_step:
	ld	hl, #_bantumi_search_steps
	dec	(hl)
	jp	nz, step
	ret

;; a = the node's next move (next_move): the first-move pick, then the
;; pits in order, skipping it.
next_move:
	call	cur_hl
	ld	a, (hl)
	ld	(#side), a
	ld	a, #N_FIRST
	call	add_hl_a
	ld	a, (hl)
	cp	a, #NO_MOVE
	jr	nz, 3$
	push	hl
	call	first_move
	pop	hl
	ld	b, a
	ld	a, (#side)
	cp	a, #SIDE_PHONE
	ld	a, b
	jr	nz, 1$
	sub	a, #7
1$:
	ld	(hl), a
	ld	a, b
	ret
3$:
	ld	c, a			; first
	inc	hl			; counter
	ld	a, (hl)
	ld	b, a			; i = counter++
	inc	a
	ld	(hl), a
	ld	a, b
	cp	a, c
	jr	nz, 4$
	inc	b			; i == first: the next one
	inc	(hl)
4$:
	ld	a, (#side)
	cp	a, #SIDE_PHONE
	ld	a, b
	ret	nz
	add	a, #7
	ret

;; a = the move to try first (first_move).
first_move:
	call	cur_hl
	ld	a, (hl)
	cp	a, #SIDE_PLAYER
	ld	a, #0
	jr	z, 1$
	ld	a, #7
1$:
	ld	(#base), a
	inc	hl			; N_PITS = 7
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	ld	a, l
	ld	(#pits), a
	ld	a, h
	ld	(#pits + 1), a
	ld	a, (#base)
	call	add_hl_a		; hl = the side's row
	push	hl
	; a move that ends in the store: row[i] == 6 - i, from i = 6 down
	ld	a, #6
	call	add_hl_a
	ld	b, #0			; 6 - i
2$:
	ld	a, (hl-)
	cp	a, b
	jr	z, 3$
	inc	b
	ld	a, b
	cp	a, #7
	jr	nz, 2$
	jr	4$
3$:
	pop	hl
	ld	a, (#base)
	add	a, #6
	sub	a, b
	ret
4$:
	; the last of the greatest captures, read one pit off
	ld	a, #0xff
	ld	(#found), a
	xor	a, a
	ld	(#best), a
	pop	hl
	push	hl
	ld	c, #0			; i
5$:
	ld	a, (hl+)		; row[i]
	or	a, a
	jr	z, 7$
	add	a, c			; j = i + row[i]
	cp	a, #6
	jr	nc, 7$
	ld	b, a
	push	hl
	pop	de
	pop	hl
	push	hl
	push	de
	ld	a, b
	call	add_hl_a
	ld	a, (hl)			; row[j]
	or	a, a
	jr	nz, 6$
	ld	a, (#base)
	add	a, b
	ld	b, a
	ld	a, #13
	sub	a, b			; 13 - (base + j)
	ld	b, a
	ld	a, (#pits)
	ld	l, a
	ld	a, (#pits + 1)
	ld	h, a
	ld	a, b
	call	add_hl_a
	ld	b, (hl)			; the value
	ld	a, (#found)
	inc	a
	jr	z, 8$			; none yet
	ld	a, (#best)
	ld	e, a
	ld	a, b
	cp	a, e
	jr	c, 6$			; less than the best so far
8$:
	ld	a, b
	ld	(#best), a
	ld	a, c
	ld	(#found), a
6$:
	pop	hl
7$:
	inc	c
	ld	a, c
	cp	a, #6
	jr	nz, 5$
	pop	hl
	ld	a, (#found)
	inc	a
	jr	z, 9$
	dec	a
	ld	b, a
	ld	a, (#base)
	add	a, b
	ret
9$:
	; the fullest pit, the first of equals
	ld	a, (hl+)
	ld	b, a			; best
	ld	c, #0			; found
	ld	e, #1			; i
10$:
	ld	a, (hl+)
	cp	a, b
	jr	c, 11$
	jr	z, 11$
	ld	b, a
	ld	c, e
11$:
	inc	e
	ld	a, e
	cp	a, #6
	jr	nz, 10$
	ld	a, (#base)
	add	a, c
	ret

;; Makes the child of the node being worked on for the move in a
;; (child), and works on it.
make_child:
	ld	(#move), a
	ld	hl, #_bantumi_depth
	dec	(hl)
	call	cur_hl
	ld	a, l
	ld	(#parent), a
	ld	a, h
	ld	(#parent + 1), a
	ld	a, (hl)
	ld	(#side), a
	ld	a, #N_SIZE
	call	add_hl_a
	ld	a, l
	ld	(#child), a
	ld	a, h
	ld	(#child + 1), a
	; the pits, copied
	ld	a, #N_PITS
	call	add_hl_a
	ld	d, h
	ld	e, l			; de = the child's pits
	ld	a, (#parent)
	ld	l, a
	ld	a, (#parent + 1)
	ld	h, a
	ld	a, #N_PITS
	call	add_hl_a
	ld	b, #14
1$:
	ld	a, (hl+)
	ld	(de), a
	inc	de
	dec	b
	jr	nz, 1$
	; first, counter, and best -32000
	ld	h, d
	ld	l, e			; N_FIRST
	ld	a, #NO_MOVE
	ld	(hl+), a
	xor	a, a
	ld	(hl), a
	ld	a, (#child)
	ld	l, a
	ld	a, (#child + 1)
	ld	h, a
	ld	a, #N_BEST
	call	add_hl_a
	xor	a, a			; -32000 = 0x8300
	ld	(hl+), a
	ld	a, #0x83
	ld	(hl+), a
	; hl = the child's pits; take the pit's beans
	ld	a, (#move)
	ld	c, a			; c = the pit sown into
	call	add_hl_a
	ld	b, (hl)			; b = the beans
	xor	a, a
	ld	(hl), a
	ld	a, (#child)
	ld	l, a
	ld	a, (#child + 1)
	ld	h, a
	ld	a, #N_PITS
	call	add_hl_a		; hl = the child's pits
	ld	a, (#side)
	cp	a, #SIDE_PLAYER
	jr	z, 20$
	; the phone sows: past its store, over the player's
2$:
	inc	c
	ld	a, c
	cp	a, #14
	jr	nz, 3$
	ld	c, #0
	jr	4$
3$:
	cp	a, #STORE
	jr	nz, 4$
	inc	c
4$:
	push	hl
	ld	a, c
	call	add_hl_a
	inc	(hl)
	pop	hl
	dec	b
	jr	nz, 2$
	ld	a, c
	cp	a, #PHONE_STORE
	jp	z, keep_side
	; the turn passes; the phone's capture tests the live board and goes
	; to the player's store
	ld	a, #SIDE_PLAYER
	call	pass_side
	ld	a, c
	cp	a, #7
	jp	c, child_done
	ld	a, (#_bantumi_board)
	ld	e, a
	ld	a, (#_bantumi_board + 1)
	ld	d, a
	ld	a, c
	add	a, e
	ld	e, a
	adc	a, d
	sub	a, e
	ld	d, a
	ld	a, (de)
	cp	a, #1
	jp	nz, child_done
	call	capture
	jp	child_done
	; the player sows: over the phone's store
20$:
	inc	c
	ld	a, c
	cp	a, #PHONE_STORE
	jr	nz, 21$
	ld	c, #0
21$:
	push	hl
	ld	a, c
	call	add_hl_a
	inc	(hl)
	pop	hl
	dec	b
	jr	nz, 20$
	ld	a, c
	cp	a, #STORE
	jr	z, keep_side
	ld	a, #SIDE_PHONE
	call	pass_side
	ld	a, c
	cp	a, #6
	jr	nc, child_done
	push	hl
	call	add_hl_a
	ld	a, (hl)
	pop	hl
	cp	a, #1
	jr	nz, child_done
	call	capture
	jr	child_done
keep_side:
	; the same side moves again, with the parent's window
	push	hl
	ld	a, (#parent)
	ld	e, a
	ld	a, (#parent + 1)
	ld	d, a
	ld	a, (#child)
	ld	l, a
	ld	a, (#child + 1)
	ld	h, a
	ld	b, #5			; side, alpha, beta
1$:
	ld	a, (de)
	inc	de
	ld	(hl+), a
	dec	b
	jr	nz, 1$
	pop	hl
child_done:
	; whether a row is empty
	push	hl
	ld	b, #6
	xor	a, a
1$:
	or	a, (hl)
	inc	hl
	dec	b
	jr	nz, 1$
	ld	e, #1
	or	a, a
	jr	z, 3$
	inc	hl			; past the player's store
	ld	b, #6
	xor	a, a
2$:
	or	a, (hl)
	inc	hl
	dec	b
	jr	nz, 2$
	or	a, a
	jr	z, 3$
	ld	e, #0
3$:
	pop	hl			; the child's pits
	ld	a, #N_EMPTY - N_PITS
	call	add_hl_a
	ld	(hl), e
	ld	a, (#child)
	ld	(#_bantumi_cur), a
	ld	a, (#child + 1)
	ld	(#_bantumi_cur + 1), a
	ret

;; The child's side is a; its window the parent's, negated and swapped.
;; Keeps hl and c.
pass_side:
	push	hl
	push	bc
	ld	b, a
	ld	a, (#child)
	ld	l, a
	ld	a, (#child + 1)
	ld	h, a
	ld	(hl), b
	inc	hl			; the child's alpha
	ld	a, (#parent)
	ld	e, a
	ld	a, (#parent + 1)
	ld	d, a
	inc	de			; the parent's beta
	inc	de
	inc	de
	ld	a, (de)
	cpl
	add	a, #1
	ld	(hl+), a		; alpha = -beta
	inc	de
	ld	a, (de)
	cpl
	adc	a, #0
	ld	(hl+), a
	dec	de			; the parent's alpha
	dec	de
	dec	de
	ld	a, (de)
	cpl
	add	a, #1
	ld	(hl+), a		; beta = -alpha
	inc	de
	ld	a, (de)
	cpl
	adc	a, #0
	ld	(hl), a
	pop	bc
	pop	hl
	ret

;; The capture at pit c, hl the child's pits: that bean and the opposite
;; pit's to the player's store.
capture:
	push	hl
	ld	a, #12
	sub	a, c
	call	add_hl_a
	ld	a, (hl)
	ld	(hl), #0
	pop	hl
	inc	a
	ld	b, a
	push	hl
	ld	a, #STORE
	call	add_hl_a
	ld	a, (hl)
	add	a, b
	ld	(hl), a
	pop	hl
	push	hl
	ld	a, c
	call	add_hl_a
	ld	(hl), #0
	pop	hl
	ret

;; bc = the node's value (evaluate), hl the node; the row sums when the
;; terminal flag is set. All values are small.
evaluate:
	push	hl
	ld	a, (hl)
	ld	(#side), a
	ld	a, #N_PITS
	call	add_hl_a
	ld	d, h
	ld	e, l
	ld	a, #PHONE_STORE
	call	add_hl_a
	ld	a, (hl)			; the phone's store
	ld	h, d
	ld	l, e
	ld	c, a
	ld	a, #STORE
	call	add_hl_a
	ld	a, c
	sub	a, (hl)			; less the player's
	ld	c, a
	ld	a, (#terminal)
	or	a, a
	jr	z, 3$
	ld	h, d
	ld	l, e
	ld	b, #6
1$:
	ld	a, c
	sub	a, (hl)			; less the player's row
	ld	c, a
	inc	hl
	dec	b
	jr	nz, 1$
	inc	hl
	ld	b, #6
2$:
	ld	a, c
	add	a, (hl)			; and the phone's
	ld	c, a
	inc	hl
	dec	b
	jr	nz, 2$
	ld	a, c
	or	a, a
	jr	z, 3$
	bit	7, a
	jr	nz, 4$
	add	a, #50
	ld	c, a
	jr	3$
4$:
	sub	a, #50
	ld	c, a
3$:
	ld	a, (#side)
	cp	a, #SIDE_PLAYER
	ld	a, c
	jr	nz, 5$
	cpl
	inc	a
5$:
	ld	c, a			; sign-extended to bc
	rla
	sbc	a, a
	ld	b, a
	pop	hl
	ret

;; Closes the node being worked on (close_node), a the terminal flag, and
;; goes back to its parent.
close_node:
	ld	(#terminal), a
	call	cur_hl
	ld	a, l
	sub	a, #N_SIZE
	ld	(#parent), a
	ld	a, h
	sbc	a, #0
	ld	(#parent + 1), a
	ld	a, (#terminal)
	or	a, a
	jr	nz, 1$
	ld	a, (#_bantumi_depth)
	or	a, a
	jr	nz, 2$
1$:
	call	evaluate
	push	hl
	ld	a, #N_BEST
	call	add_hl_a
	ld	(hl), c
	inc	hl
	ld	(hl), b
	pop	hl
	jr	3$
2$:
	push	hl
	ld	a, #N_BEST
	call	add_hl_a
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	pop	hl
3$:
	; bc = the value; negated when the parent's side is the other's
	ld	a, (#parent)
	ld	e, a
	ld	a, (#parent + 1)
	ld	d, a
	ld	a, (de)
	cp	a, (hl)
	jr	z, 4$
	ld	a, c
	cpl
	add	a, #1
	ld	c, a
	ld	a, b
	cpl
	adc	a, #0
	ld	b, a
4$:
	; more than the parent's best (signed)?
	ld	h, d
	ld	l, e
	ld	a, #N_BEST
	call	add_hl_a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	xor	a, #0x80
	ld	d, a			; de = the parent's best, sign flipped
	ld	a, b
	xor	a, #0x80
	ld	b, a			; and the value's
	ld	a, e
	sub	a, c
	ld	a, d
	sbc	a, b
	jr	nc, 7$			; the parent's best >= the value
	ld	a, b
	xor	a, #0x80
	ld	(hl), c			; the parent's best = the value
	inc	hl
	ld	(hl), a
	; at the root, the move that made this node is the best so far
	ld	a, (#_bantumi_root)
	ld	l, a
	ld	a, (#_bantumi_root + 1)
	ld	h, a
	ld	a, (#parent)
	cp	a, l
	jr	nz, 7$
	ld	a, (#parent + 1)
	cp	a, h
	jr	nz, 7$
	ld	a, #N_COUNTER
	call	add_hl_a
	ld	a, (hl-)		; counter
	or	a, a
	jr	nz, 6$
	ld	a, (hl)			; first
	inc	a
6$:
	dec	a
	ld	b, a
	ld	a, (#_bantumi_root)
	ld	l, a
	ld	a, (#_bantumi_root + 1)
	ld	h, a
	ld	a, (hl)
	cp	a, #SIDE_PHONE
	ld	a, b
	jr	nz, 8$
	add	a, #7
8$:
	ld	(#_bantumi_pick), a
7$:
	; the parent's alpha = its best, if more
	ld	a, (#parent)
	ld	l, a
	ld	a, (#parent + 1)
	ld	h, a
	inc	hl			; alpha
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl+)
	xor	a, #0x80
	ld	d, a			; de = alpha, flipped
	inc	hl
	inc	hl			; best
	ld	a, (hl+)
	ld	c, a
	ld	a, (hl)
	ld	b, a
	xor	a, #0x80
	ld	h, a
	ld	a, e
	sub	a, c
	ld	a, d
	sbc	a, h
	jr	nc, 9$			; alpha >= best
	ld	a, (#parent)
	ld	l, a
	ld	a, (#parent + 1)
	ld	h, a
	inc	hl
	ld	(hl), c
	inc	hl
	ld	(hl), b
9$:
	ld	hl, #_bantumi_depth
	inc	(hl)
	ld	a, (#parent)
	ld	(#_bantumi_cur), a
	ld	a, (#parent + 1)
	ld	(#_bantumi_cur + 1), a
	ret
