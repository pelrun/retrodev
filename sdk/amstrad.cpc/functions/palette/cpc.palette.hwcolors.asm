;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC hardware palette colours.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_palette_hwcolors_asm__
__cpc_palette_hwcolors_asm__ equ 1

	;
	; Require macros.ga.asm
	;
	include "macros.gam.asm"

	; GA_SetPaletteHw -- set N consecutive ink pens from a table of raw hardware colour indices.
	;
	; Each table byte is a raw hardware colour index in the range 0-26.
	; This function applies the GA colour command flag (bit 6) at runtime before writing,
	; so the table format is simpler to generate than the pre-mixed variant.
	;
	; Use this variant when your palette table stores bare hardware indices (e.g. exported
	; by Retrodev in "hardware colour index" format, or built from the CPC hardware colour map).
	;
	; CPC hardware colour indices (0-26) do NOT follow a linear RGB order -- they are the
	; physical register values of the Amstrad GA. Use the SysToHw lookup in
	; cpc.palette.syscolors.asm to convert from firmware/system colour numbers to hw indices.
	;
	; Entry:
	;   HL = pointer to table of hardware colour indices (one byte per pen, values 0-26)
	;   C  = number of pens to set (1-17; pen 0 is the first ink, pen 16 is the border)
	;
	; Pens are set starting from pen 0 up to pen C-1, in order.
	;
	; Destroys: A, B, C, D, E, HL, F
	; Preserves: IX, IY, BC', DE', HL'
	;
	; Speed (4 MHz Z80, T-states per iteration):
	;   ld c,e       =  4  (load pen index)
	;   out (c),c    = 12  (select pen)
	;   ld c,(hl)    =  7  (load raw hw colour index)
	;   inc hl       =  6  (advance pointer)
	;   set 6,c      =  8  (apply GA colour command flag: C |= GA_CMD_COLOR_SET)
	;   out (c),c    = 12  (write colour command)
	;   inc e        =  4  (next pen index)
	;   dec d        =  4  (decrement counter)
	;   jr nz        = 12  (loop -- 7 T on final iteration)
	;                ----
	;                 69 T per iteration (64 T on last)
	;
	; For 16 pens: setup(~30 T) + 15*69 + 64 = 1129 T ~ 282 uss
	;
	; Optimization note: B=GA_PORT_CMD_B is loaded once and held for the entire loop.

	; GA_SetPaletteHw -- set N consecutive pens from a table of raw hardware colour indices (0-26).
	; Entry: HL = pointer to table of hardware colour index bytes
	;        C  = number of pens to set (1-17; pen 0 = first ink, pen 16 = border)
	; Exit:  pens 0..C-1 written to the Gate Array
	; Destroys: AF, BC, DE, HL  Preserves: IX, IY
	GA_SetPaletteHw:
			ld d,c
			xor a			; A = 0 -- pen index starts at 0
			ld e,a			; E = pen index
			ld b,GA_PORT_CMD_B	; B = GA port high byte, held for entire loop
	.loop:
			ld c,e			; C = pen index (pen-select command, bits 7-5 = 000)
			out (c),c		; send pen-select command to Gate Array
			ld c,(hl)		; C = raw hardware colour index (0-26)
			inc hl			; advance table pointer
			set 6,c			; apply GA colour command flag: C = GA_CMD_COLOR_SET | hw_index
			out (c),c		; send colour command to Gate Array
			inc e			; next pen index
			dec d			; decrement counter
			jr nz,.loop		; continue until all pens written
			ret
endif
