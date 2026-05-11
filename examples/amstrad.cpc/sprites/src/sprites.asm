;-----------------------------------------------------------------------
;
; Retrodev Examples
;
; sprites -- draw/erase mode 0 sprites at byte-X and pixel-Y coordinates.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

; CalcScreenAddr
;
; Input:
;   B = X in bytes (0..79)
;   C = Y in scanlines (0..199)
;
; Output:
;   HL = screen address for (X, Y)
;
; Destroys: AF, DE
; Note: ScreenLineTable lookup -- no multiply, no push/pop.
CalcScreenAddr:
	ld l,c
	ld h,0
	add hl,hl
	ld de,ScreenLineTable
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld e,b
	ld d,0
	add hl,de
	ret

; DrawSpriteRaw
;
; Input:
;   HL = pointer to sprite bytes (row-major, no width/height header)
;   B  = X in bytes
;   C  = Y in scanlines
;   D  = bytes per row
;   E  = height in rows
;
; Destroys: AF, BC, DE, HL, IX
; Note: IXH=row counter, IXL=X, IYH=bpr -- all survive inline row advance.
;   First row uses CalcScreenAddr; subsequent rows advance D by +#08,
;   with set 7/set 6 carry fix keeping D in #C0..#FF screen range.
DrawSpriteRaw:
	ld ixh,e
	ld ixl,b
	ld iyh,d
	; first row: get screen address via table
	push hl
	call CalcScreenAddr
	ex de,hl
	pop hl
	ld b,0
	; DE=screen dest, HL=src
.row_draw:
	ld a,ixh
	or a
	ret z
	dec ixh
	; copy one row: src=HL, dest=DE, BC=bpr
	push de
	ld c,iyh
	ldir
	; restore row_start into DE (pre-ldir value)
	pop de
	; advance DE to next scanline: DE += #800, fix wrap into next bank
	ld a,d
	add a,#08
	jr nc,.no_wrap_draw
	ld a,e
	add SCREEN_BYTES_PER_LINE
	ld e,a
	ld a,d
	adc a,#c0
.no_wrap_draw:
	ld d,a
	jr .row_draw

; SpriteGetSize
;
; Input:
;   HL = pointer to sprite header (byte 0 = width in bytes, byte 1 = height)
;
; Output:
;   D = width in bytes
;   E = height in rows
;
; Destroys: AF
SpriteGetSize:
	ld a,(hl)
	ld d,a
	inc hl
	ld e,(hl)
	ret

; DrawSprite
;
; Input:
;   HL = pointer to sprite with 2-byte header (width,height)
;   B  = X in bytes
;   C  = Y in scanlines
;
; Destroys: AF, BC, DE, HL, IX, IY
DrawSprite:
	push hl
	call SpriteGetSize
	pop hl
	inc hl
	inc hl
	jp DrawSpriteRaw

; EraseSprite
;
; Input:
;   HL = pointer to sprite with 2-byte header (width,height)
;   B  = X in bytes
;   C  = Y in scanlines
;
; Destroys: AF, BC, DE, HL, IX, IY
EraseSprite:
	push hl
	call SpriteGetSize
	pop hl
	jp EraseSpriteRaw

; EraseSpriteRaw
;
; Input:
;   B  = X in bytes
;   C  = Y in scanlines
;   D  = bytes per row
;   E  = height in rows
;
; Destroys: AF, BC, DE, HL, IX, IY
; Note: IXH=row counter, IXL=X, IYH=bpr -- same inline advance as DrawSpriteRaw.
EraseSpriteRaw:
	ld ixh,e
	ld ixl,b
	ld iyh,d
	; first row: get screen address via table
	call CalcScreenAddr
	; HL=screen dest
.row_clear:
	ld a,ixh
	or a
	ret z
	dec ixh
	; zero IYH bytes from HL; save row_start first
	push hl
	ld b,iyh
	xor a
.clear_loop:
	ld (hl),a
	inc hl
	djnz .clear_loop
	; restore row_start
	pop hl
	; advance HL to next scanline: HL += #800, fix wrap into next bank
	ld a,h
	add a,#08
	jr nc,.no_wrap_clear
	ld a,l
	add SCREEN_BYTES_PER_LINE
	ld l,a
	ld a,h
	adc a,#c0
.no_wrap_clear:
	ld h,a
	jr .row_clear

; ScreenLineTable
;
; 200 word entries: ScreenLineTable[y] = #C000 + (y&7)*#800 + (y>>3)*80
; Generated at assemble time -- zero runtime cost.
ScreenLineTable:
	repeat 200,y,0
	defw #C000 + ((y & 7) * #800) + ((y >> 3) * 80)
	rend
