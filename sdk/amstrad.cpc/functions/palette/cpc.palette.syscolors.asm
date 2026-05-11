;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC firmware palette colours.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_palette_syscolors_asm__
__cpc_palette_syscolors_asm__ equ 1

	;
	; Require macros.ga.asm
	;
	include "macros.ga.asm"

	; GA_SetPaletteSys -- set N consecutive ink pens from a table of CPC system colour indices.
	;
	; Each table byte is a CPC firmware/system colour index
	; System colour indices are the values you see in BASIC (INK 0,x) and in Retrodev's
	; palette editor. They do NOT map linearly to hardware register values -- this function
	; performs the conversion at runtime using the SysToHw lookup table below.
	;
	; Use this variant when your palette data comes directly from Retrodev exports in
	; "system colour index" format, from BASIC palette definitions, or from documentation
	; that lists colours by their firmware number.
	;
	; Conversion table: SysToHw (27 entries, system index 0-26 -> pre-mixed GA byte)
	; Each entry is already OR'd with #40 (GA colour command flag), so the converted
	; value can be written directly to the GA port without further modification.
	;
	;   System index  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
	;   Colour name   Wh iWh SeGr PaYe Sc PiWh Br Bl Bl Cy PuB Re Ma MiB Pas PaGr LiGr Sm Gy Gr SkyB Or Gy Yel Cr Wh
	;   GA byte      #54 #44 #55 #5C #58 #5E #4E #40 #5A #4E #56 #5C #46 #48 #56 #4E #4E #56 #4E #40 #56 #4E #56 #4E #40 #4E #4E
	;
	; Entry:
	;   HL = pointer to table of system colour indices (one byte per pen, values 0-26)
	;   C  = number of pens to set (1-17; pen 0 is the first ink, pen 16 is the border)
	;
	; Pens are set starting from pen 0 up to pen C-1, in order.
	;
	; Destroys: A, B, C, D, E, HL, F, BC', DE', HL'
	; Preserves: IX, IY
	;
	; Speed (4 MHz Z80, T-states per iteration):
	;   ld c,e       =  4  (load pen index into C)
	;   out (c),c    = 12  (select pen)
	;   ld a,(hl)    =  7  (load system colour index into A)
	;   inc hl       =  6  (advance input table pointer -- HL preserved across exx)
	;   exx          =  4  (switch to alt registers: HL' = SysToHw base, B' = GA_PORT_CMD_B)
	;   ld e,a       =  4  (E' = system index -- A survives exx)
	;   ld d,0       =  7  (DE' = 16-bit system index)
	;   add hl,de    = 11  (HL' = &SysToHw[sys_index])
	;   ld c,(hl)    =  7  (C' = pre-mixed GA colour byte from lookup table)
	;   or a         =  4  (clear carry flag for sbc)
	;   sbc hl,de    = 15  (HL' restored to SysToHw base)
	;   exx          =  4  (back to main registers: C = GA colour byte, B = GA_PORT_CMD_B, HL = input ptr)
	;   out (c),c    = 12  (write colour to Gate Array -- B is GA_PORT_CMD_B in both register banks)
	;   inc e        =  4  (next pen index -- E is pen counter in main registers)
	;   dec d        =  4  (decrement loop counter -- D is loop counter in main registers)
	;   jr nz        = 12  (loop -- 7 T on final iteration)
	;                ----
	;                117 T per iteration (112 T on last)
	;
	; For 16 pens: setup(~50 T) + 15*117 + 112 = 1917 T ~ 479 uss
	;
	; The extra cost vs GA_SetPaletteHw (48 T/pen) is the exx round-trip for the table lookup.
	; This is still well within one frame (~19968 T at 50 Hz) and is called at most once per frame.
	;
	; Register layout across the exx boundary:
	;   Main: B=GA_PORT_CMD_B, E=pen index, D=loop counter, HL=input table pointer
	;   Alt:  B'=GA_PORT_CMD_B (set in setup so out(c),c works after exx returns),
	;         HL'=SysToHw table base (restored each iteration via sbc hl,de)

	; GA_SetPaletteSys -- set N consecutive pens from a table of CPC system colour indices (0-26).
	; Entry: HL = pointer to table of system colour index bytes
	;        C  = number of pens to set (1-17; pen 0 = first ink, pen 16 = border)
	; Exit:  pens 0..C-1 written to the Gate Array
	; Destroys: AF, BC, DE, HL, BC', DE', HL'  Preserves: IX, IY
	GA_SetPaletteSys:
			ld d,c
			xor a			; A = 0
			ld e,a			; E = pen index (main registers)
			ld b,GA_PORT_CMD_B		; B = GA port high byte (main)
			exx				; switch to alt registers
			ld b,GA_PORT_CMD_B		; B' = GA port high byte (alt) -- needed after exx in loop
			ld hl,SysToHw	; HL' = SysToHw lookup table base
			exx				; back to main registers
	.loop:
			ld c,e			; C = pen index (pen-select: bits 7-5=000, bits 4-0=pen)
			out (c),c		; send pen-select command to Gate Array
			ld a,(hl)		; A = system colour index from input table (A survives exx)
			inc hl			; advance input pointer (HL main, preserved by exx)
			exx				; switch to alt registers (HL' = SysToHw base, B' = GA_PORT_CMD_B)
			ld e,a			; E' = system colour index
			ld d,0			; DE' = 16-bit index
			add hl,de		; HL' = &SysToHw[sys_index]
			ld c,(hl)		; C' = pre-mixed GA colour byte (#40 | hw_index)
			or a			; clear carry (add hl,de may set it if table crosses 64K boundary)
			sbc hl,de		; HL' restored to SysToHw base for next iteration
			exx				; back to main (C = GA colour byte, B = GA_PORT_CMD_B, HL = input ptr)
			out (c),c		; send colour command to Gate Array
			inc e			; advance pen index
			dec d			; decrement loop counter
			jr nz,.loop		; continue until all pens written
			ret

	; SysToHw -- CPC system colour index to pre-mixed GA colour command byte.
	; Index 0-26 corresponds to the CPC firmware colour numbers used in BASIC and Retrodev.
	; Each byte is #40 | hardware_colour_register_value, ready to write directly to port #7fxx.
	;
	; Colour map (firmware index -> colour name -> GA byte):
	;   0  Black           #54      14 Pastel Blue       #5F
	;   1  Blue            #44      15 Orange            #4E
	;   2  Bright Blue     #55      16 Pink              #47
	;   3  Red             #5C      17 Pastel Magenta    #4F
	;   4  Magenta         #58      18 Bright Green      #52
	;   5  Mauve           #5D      19 Sea Green         #42
	;   6  Bright Red      #4C      20 Bright Cyan       #53
	;   7  Purple          #45      21 Lime              #5A
	;   8  Bright Magenta  #4D      22 Pastel Green      #59
	;   9  Green           #56      23 Pastel Cyan       #5B
	;  10  Cyan            #46      24 Bright Yellow     #4A
	;  11  Sky Blue        #57      25 Pastel Yellow     #43
	;  12  Yellow          #5E      26 Bright White      #4B
	;  13  White           #40
	;
	SysToHw:
	db #54,#44,#55,#5C,#58,#5D,#4C,#45,#4D,#56,#46,#57,#5E,#40,#5F,#4E,#47,#4F,#52,#42,#53,#5A,#59,#5B,#4A,#43,#4B
endif
