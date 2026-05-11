;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC premixed palette colours.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_palette_premixed_asm__
__cpc_palette_premixed_asm__ equ 1

        ;
        ; Require macros.ga.asm
        ;
        include "macros.ga.asm"

        ; GA_SetPalettePremixed -- set N consecutive ink pens from a table of pre-mixed GA colour bytes.
        ;
        ; The table contains the exact byte to write to the GA colour port
        ; (bit 6 set, bits 4-0 = hw colour index).
        ; No conversion is performed -- each byte is written directly to the Gate Array.
        ;
        ; Use this variant when your palette table is exported by the Retrodev export script using
        ; the "pre-mixed GA command" format, or when you have pre-computed the table yourself.
        ;
        ; Entry:
        ;   HL = pointer to table of pre-mixed GA colour bytes (one byte per pen)
        ;   C  = number of pens to set (1-17; pen 0 is the first ink, pen 16 is the border)
        ;
        ; Each table byte must have the form:  %01xhhhhh
        ;   bit 6    = 1 (GA colour command flag, must be set)
        ;   bits 4-0 = hardware colour index (0-26)
        ; Example: hardware colour 0 (white) -> #40 | 0 = #40; see SysToHw table for full mapping.
        ;
        ; Pens are set starting from pen 0 up to pen C-1, in order.
        ; To set a subset of pens, advance HL to the first desired pen and adjust C accordingly,
        ; but note that the pen index written to the GA always starts at 0 and increments -- to set
        ; arbitrary pens, use the GA_SetBorder macro or write your own indexed loop instead.
        ;
        ; Destroys: A, B, C, HL, F
        ; Preserves: DE, IX, IY, BC', DE', HL'
        ;
        ; Speed (4 MHz Z80, T-states per iteration):
        ;   out (c),a    = 12  (select pen -- A = pen index, GA pen-select command bits 7-6 = 00)
        ;   outi         = 16  (OUT (BC),(HL++); B--  -- writes colour byte, advances HL, costs B)
        ;   inc b        =  4  (restore B to GA_PORT_CMD_B after outi's implicit dec b)
        ;   inc a        =  4  (next pen index)
        ;   dec c        =  4  (decrement loop counter)
        ;   jr nz        = 12  (loop -- 7 T on final iteration)
        ;                ----
        ;                 52 T per iteration (47 T on last)
        ;
        ; For 16 pens: setup(~18 T) + 15*52 + 47 = 845 T ~ 211 us
        ;
        ; Compared to the ld/out/out approach (61 T/iter): saves 9 T per pen (~15% faster).
        ; outi = OUT (BC),(HL++) + dec B in 16 T, replacing ld c,(hl)(7) + inc hl(6) + out (c),c(12) = 25 T.
        ; B is held at GA_PORT_CMD_B throughout; outi decrements B each time so inc b restores it.
        ; A carries the pen index (GA_CMD_PEN_SELECT | pen = pen index since GA_CMD_PEN_SELECT = #00).
        ; C is the loop counter; the GA ignores the port low byte so C doubling as counter is safe.

        ; GA_SetPalettePremixed -- set N consecutive pens from a table of pre-mixed GA colour bytes.
        ; Entry: HL = pointer to table of pre-mixed GA colour bytes (bit 6 set, bits 4-0 = hw index)
        ;        C  = number of pens to set (1-17; pen 0 = first ink, pen 16 = border)
        ; Exit:  pens 0..C-1 written to the Gate Array
        ; Destroys: AF, BC, HL  Preserves: DE, IX, IY
        GA_SetPalettePremixed:
                ld b,GA_PORT_CMD_B	; B = &7F -- GA port high byte, held for the entire loop
                xor a			; A = 0 -- pen index starts at pen 0 (GA_CMD_PEN_SELECT | 0)
        .loop:
                out (c),a		; send pen-select command: GA_CMD_PEN_SELECT | pen (bits 7-6 = 00)
                outi			; OUT (BC),(HL++); B-- -- send pre-mixed colour byte, advance table
                inc b			; restore B to GA_PORT_CMD_B after outi's implicit dec b
                inc a			; advance to next pen index
                dec c			; one fewer pen to set (C safe as counter; GA ignores port low byte)
                jr nz,.loop		; continue until all N pens are written
                ret
endif
