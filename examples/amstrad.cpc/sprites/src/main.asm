;-----------------------------------------------------------------------
;
; Retrodev Examples
;
; sprites -- load palette/sprite data, move a sprite with O/P/Q/A and render it.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

org #4000

jp EntryPoint

DebugProcesses equ 1

;include 'macros.ga.asm'
include 'macros.debug.asm'
include 'macros.keyboard.asm'
include 'cpc.palette.premixed.asm'
include 'cpc.keyboard.scan.asm'
include 'sprites.asm'
include 'cpc.interrupts.disable.asm'

SCREEN_BYTES_PER_LINE	equ 80
CURRENT_SPRITE		equ spr_Walk1_0

EntryPoint:
	ld b,GA_MODE_0
 	call CPC_DISABLEINTERRUPTS
	ld hl,PaletteData
	ld c,16
	call GA_SetPalettePremixed
	GA_SetBorder GA_COLOR_BLACK
	ld hl,CURRENT_SPRITE
	call SpriteGetSize			; D=width bytes, E=height
	ld a,SCREEN_BYTES_PER_LINE
	sub d
	ld (MaxXByte),a
	ld a,200
	sub e
	ld (MaxY),a
	ld a,0
	ld (SpriteX),a
	ld (PrevX),a
	ld a,0
	ld (SpriteY),a
	ld (PrevY),a
MainLoop:
	GA_VSyncWait GA_COLOR_BLACK
	call KeyboardScanner
  GA_SetBorder GA_COLOR_BLACK
	ld a,(SpriteX)
	ld (PrevX),a
	ld a,(SpriteY)
	ld (PrevY),a
  KeyPressedM KEY_O
	jr nc,.no_left
  GA_SetBorder GA_COLOR_BLUE
	ld a,(SpriteX)
	or a
	jr z,.no_left
	dec a
	ld (SpriteX),a
.no_left:
  KeyPressedM KEY_P
	jr nc,.no_right
  GA_SetBorder GA_COLOR_RED
	ld a,(SpriteX)
	ld b,a
	ld a,(MaxXByte)
	cp b
	jr z,.no_right
	ld a,b
	inc a
	ld (SpriteX),a
.no_right:
 	KeyPressedM KEY_Q
	jr nc,.no_up
  GA_SetBorder GA_COLOR_GREEN
	ld a,(SpriteY)
	or a
	jr z,.no_up
	dec a
	ld (SpriteY),a
.no_up:
 	KeyPressedM KEY_A
	jr nc,.no_down
  GA_SetBorder GA_COLOR_MAGENTA
	ld a,(SpriteY)
	ld b,a
	ld a,(MaxY)
	cp b
	jr z,.no_down
	ld a,b
	inc a
	ld (SpriteY),a
.no_down:
	ld a,(PrevX)
	ld b,a
	ld a,(PrevY)
	ld c,a
	ld hl,CURRENT_SPRITE
	call EraseSprite
	GA_DebugProcess GA_COLOR_GREEN
	ld a,(SpriteX)
	ld b,a
	ld a,(SpriteY)
	ld c,a
	ld hl,CURRENT_SPRITE
	call DrawSprite
	GA_DebugProcess GA_COLOR_BLACK
	jp MainLoop

MaxXByte:	defb 0
MaxY:		defb 0
SpriteX:	defb 0
SpriteY:	defb 0
PrevX:		defb 0
PrevY:		defb 0

PaletteData:
	incbin "../out/sprites.pal"

	include "../out/sprites.asm"

RUN EntryPoint
	SAVE 'sprites.bin',#4000,$-#4000,DSK,'out/sprites.dsk'