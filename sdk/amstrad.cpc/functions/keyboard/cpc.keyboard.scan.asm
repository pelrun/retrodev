;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC keyboard scanner routines.
;
; Purpose: scan keyboard matrix and update key state buffers.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_keyboard_scanner_asm__
__cpc_keyboard_scanner_asm__ equ 1

	;
	; Require macros.keyboard.asm
	;
	include "macros.keyboard.asm"

	; KeyboardScanner
	;
	; The CPC keyboard is read entirely through the 8255 PPI. The AY-3-8912 is NOT
	; performing audio work here -- its I/O Port A (R14) is physically wired to the keyboard
	; matrix row-select lines and is used purely as a GPIO output expander. The confusion
	; arises because the AY is accessed through the PPI ports (PPI Port A carries the AY
	; address/data bus; PPI Port C bits 7-6 carry the BDIR/BC2 control lines), so every
	; operation appears as a PPI write even when it is targeting the AY.
	;
	; CPC port map used in this routine:
	;   #f7xx -- PPI control register (write-only). Configures direction of Ports A, B, C.
	;   #f4xx -- PPI Port A (bidirectional). Used as AY data bus output during setup,
	;            then reconfigured as keyboard column input during the scan loop.
	;   #f6xx -- PPI Port C (output). Bits 7-6 = BDIR/BC2 (AY bus control).
	;            Bits 3-0 = keyboard matrix row select (0-9), decoded on the motherboard
	;            by a row-address demultiplexer that drives the matrix lines directly.
	;
	; AY bus modes (driven by PPI Port C bits 7-6):
	;   BDIR=0, BC2=0  (#00) -- inactive
	;   BDIR=1, BC2=0  (#80) -- write data to selected AY register
	;   BDIR=0, BC2=1  (#40) -- read selected AY register onto data bus
	;   BDIR=1, BC2=1  (#c0) -- latch address (select AY register)
	;
	; Setup sequence (runs once before the scan loop):
	;   1. PPI control word #82: Port A = output, Port C = output.
	;   2. PPI Port A <- #0e: places AY register index 14 on the AY address bus.
	;   3. PPI Port C <- #c0 (BDIR=1,BC2=1): latches R14 as the active AY register.
	;   4. PPI Port C <- #00 (BDIR=0,BC2=0): returns AY to inactive -- strobe complete.
	;   5. PPI control word #92: Port A = input, Port C = output.
	;      Port A now reads the 8 keyboard column bits driven by the matrix.
	;
	; Scan loop (10 iterations, one per keyboard row):
	;   - PPI Port C <- #40 | row (BDIR=0,BC2=1 = AY read, bits 3-0 = row index 0-9).
	;     The row bits in Port C drive the motherboard row-address lines directly.
	;     The BC2=1 state also causes the AY to present its R14 value on the data bus,
	;     but since Port A is now an input this does not interfere with the key read.
	;   - INI reads PPI Port A (#f4) -> (HL): captures the 8 column bits for this row.
	;     Bit = 0 means the key is pressed; bit = 1 means released.
	;
	; Register allocation:
	;   D  = #f6  (PPI Port C high byte -- cached to avoid ld bc,nn inside loop)
	;   E  = #f4  (PPI Port A high byte -- cached for the same reason)
	;   HL = Keymap pointer (advanced by INI each iteration)
	;   A  = current row selector (#40 | row, range #40-#49)
	;   C  = #4a  (loop sentinel = #40 + 10 rows)
	;   B  = set each iteration from D or E as needed for the OUT/INI port address
	;
	; Destroys: A, B, C, D, E, HL, F
	; Preserves: IX, IY
	;
	; Speed (4 MHz Z80, T-states):
	;   Setup  : approximately 120 T
	;   Loop   : 54 T * 10 iterations = 540 T
	;   Restore: 22 T
	;   Total  : approximately 682 T ~ 170 uss
	;
	;   Loop body breakdown per iteration:
	;     ld b,d       =  4  (PPI Port C high byte from cache -> B)
	;     out (c),a    = 12  (write row selector + BC2=1 to PPI Port C)
	;     ld b,e       =  4  (PPI Port A high byte from cache -> B)
	;     ini          = 16  (read column bits from PPI Port A -> (HL), inc HL, dec B)
	;     inc a        =  4  (advance row selector)
	;     cp c         =  4  (test against sentinel)
	;     jp c,loop    = 10  (always 10 T -- faster than jr c for 10-iteration loops)
	;                  ----
	;                   54 T
	;
	; Optimization notes:
	;   - D/E cache the port high bytes (#f6, #f4) -- avoids ld bc,nn (10 T) inside the
	;     loop, saving 100 T over 10 iterations.
	;   - jp c chosen over jr c: jp = 10 T always; jr c = 12 T taken / 7 T not taken,
	;     costing 9*12+7 = 115 T vs jp's flat 100 T -- jp wins for this loop count.
	;   - INI's dec B side effect is intentionally discarded: B is reloaded from the
	;     cache at the top of every iteration anyway.
	;   - No free register remains for a djnz counter: D/E hold port constants, HL is
	;     consumed by INI, A carries the row selector. The inc/cp/jp sequence (18 T) is
	;     the minimum overhead achievable without loop unrolling.
	;   - Full unroll (10 * out+ini, no loop control) would save 180 T at the cost of
	;     roughly 60 extra bytes. Worth considering only in extremely tight raster code.

	; KeyboardScanner -- scan all 10 CPC keyboard matrix rows into Keymap.
	; Entry: none  (Keymap must be defined in macros.keyboard.asm)
	; Exit:  Keymap[0..9] updated; bit = 0 means key pressed, bit = 1 means released
	; Destroys: AF, BC, DE, HL  Preserves: IX, IY
	KeyboardScanner:
		ld bc,#f782
		ld hl,Keymap	; HL -> keymap buffer start (INI advances this each iteration)
		xor a			; A = 0 (used to deactivate AY bus below; also clears flags)
		out (c),c		; write control word #82 to PPI control register (#f7)
		ld bc,#f40e		; PPI Port A (#f4), value #0e = AY register index 14
		ld e,b			; cache PPI Port A high byte (#f4) in E for use inside loop
		out (c),c		; write #0e to PPI Port A: places R14 index on AY address bus
		ld bc,#f6c0		; PPI Port C (#f6), value #c0 = BDIR=1,BC2=1 (latch AY address)
		ld d,b			; cache PPI Port C high byte (#f6) in D for use inside loop
		out (c),c		; strobe AY: R14 is now the active register
		out (c),a		; PPI Port C <- 0 (BDIR=0,BC2=0 = inactive): complete the strobe
		ld bc,#f792		; PPI control word -> Port A = input (reads key columns), Port C = output
		out (c),c		; write new control word: Port A is now the keyboard column input
		ld a,#40		; A = first row selector: bits 7-6=#01 (BC2=1), bits 3-0=row 0
		ld c,#4a		; C = sentinel (#40 + 10 rows)
	.KeyboardScanner_Loop:
		ld b,d			; B = #f6 (PPI Port C) -- from cache
		out (c),a		; PPI Port C <- row selector: sets row address lines and BC2=1
		ld b,e			; B = #f4 (PPI Port A) -- from cache
		ini				; read 8 column bits from PPI Port A -> (HL), then inc HL, dec B
		inc a			; next row selector
		cp c			; all 10 rows done?
		jp c,.KeyboardScanner_Loop	; no -- continue (jp = 10 T flat, faster than jr here)
		ld bc,#f782		; PPI control word -> restore Port A = output, Port C = output
		out (c),c		; write restored control word
		ret
endif
