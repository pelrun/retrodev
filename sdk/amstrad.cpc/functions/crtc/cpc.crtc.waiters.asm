; ---------------------------------------------------------------------------
;
; Retrodev SDK
;
; Cycle-accurate CRTC raster wait routines for the Amstrad CPC.
;
; (c) TLOTB 2026
;
; ---------------------------------------------------------------------------

ifndef __cpc_crtc_waiters_asm__
__cpc_crtc_waiters_asm__ equ 1

	; ---------------------------------------------------------------------------
	;
	; TIMING BASIS
	;
	; One CPC scanline = 64 character clocks = 256 T-states = 64 NOPs at 4 MHz.
	; All cycle counts in this file use "NOPs" as the unit (1 NOP = 4 T-states).
	; Instruction NOP costs used throughout:
	;   nop          = 1 NOP  (4 T)
	;   ld r,n       = 2 NOPs (7 T)  -- e.g. ld b,n  ld c,n
	;   ld rr,nn     = 3 NOPs (10 T)
	;   inc/dec r    = 1 NOP  (4 T)
	;   jr $+2       = 3 NOPs (12 T)  -- forward jump-to-next, always taken
	;   jr nz,label  = 3 NOPs (12 T) taken / 2 NOPs (8 T) not taken
	;   djnz label   = 4 NOPs (13 T) taken / 3 NOPs (11 T) not taken (B->0)
	;   call nn      = 5 NOPs (20 T)  -- measured on CPC hardware (GA rounds 17T up to 20T)
	;   ret          = 3 NOPs (10 T)
	;
	; ---------------------------------------------------------------------------

	; ---------------------------------------------------------------------------
	;
	; WaitScanlines_Routine / WaitScanlines_Loop
	;
	; Wait approximately B scanlines, where each scanline = 64 NOPs.
	;
	; Entry overhead at the call site: ld b,n (2 NOPs) + call (5 NOPs) = 7 NOPs.
	; The routine is split at two entry points to absorb this overhead on the
	; first iteration without disrupting subsequent iterations:
	;
	;   WaitScanlines_Loop    -- loop-back target; emits a 9-NOP pad (3x jr $+2)
	;                            then falls through to WaitScanlines_Routine.
	;   WaitScanlines_Routine -- shared body; entered directly on the first
	;                            iteration, bypassing the 9-NOP pad.
	;
	; Iteration cost breakdown:
	;
	;   First iteration (enters at WaitScanlines_Routine, 7 NOPs already spent):
	;     7 (entry) + 2 (ld c,12) + 47 (inner loop) + 2 (nop nop) + 1 (dec b)
	;     + 3 (jr nz taken) = 62 NOPs -- two NOPs short of a full scanline.
	;     The first and last iterations are asymmetric by design: first is 2 NOPs
	;     short and last is 2 NOPs over, so they cancel in the total.
	;
	;   Middle iterations (enter at WaitScanlines_Loop, pad absorbs prior jr cost):
	;     9 (pad) + 2 (ld c,12) + 47 (inner loop) + 2 (nop nop) + 1 (dec b)
	;     + 3 (jr nz taken) = 64 NOPs = exactly 1 scanline. Cycle-exact.
	;
	;   Last iteration (jr nz not-taken, then ret):
	;     9 (pad) + 2 + 47 + 2 + 1 + 2 (jr not-taken) + 3 (ret) = 66 NOPs.
	;
	; Total for B scanlines (B >= 1):
	;   First + (B-2) middle + last  [for B >= 2]
	;   = 62 + (B-2)*64 + 66 = 64*B + 0 -- exactly 64*B NOPs.
	;   For B=1: first iter takes jr not-taken + ret: 7+2+47+2+1+2+3 = 64 NOPs. Exact.
	;
	; Minimum call: ld b,1 + call WaitScanlines_Routine = 1 scanline.
	; Maximum:      B = 255 scanlines (limited by 8-bit counter).
	;
	; Destroys: B, C
	; Preserves: A, DE, HL, IX, IY, F (except flags touched by dec b / jr nz)
	;
	; Size: 16 bytes (6 bytes pad + 10 bytes routine body).
	;
	; ---------------------------------------------------------------------------
	WaitScanlines_Loop:
		jr $+2							; 3 NOPs -- \
		jr $+2							; 3 NOPs --  } 9-NOP pad to equalise loop-back cost (call=5 NOPs)
		jr $+2							; 3 NOPs -- /
	WaitScanlines_Routine:
		ld c,12							; 2 NOPs -- prime inner counter (12 iterations)
	.inner:
		dec c							; 1 NOP
		jr nz,.inner					; 3 NOPs taken / 2 NOPs not-taken -> 47 NOPs total inner block
		nop								; 1 NOP -- \
		nop								; 1 NOP -- / fine-tune to reach 64 NOPs per middle iteration
		dec b							; 1 NOP
		jr nz,WaitScanlines_Loop		; 3 NOPs taken / 2 NOPs not-taken
		ret								; 3 NOPs

	; ---------------------------------------------------------------------------
	;
	; WaitNops_Routine
	;
	; Subroutine core for WaitNops macro (see below).
	; Not intended to be called directly -- use the WaitNops macro instead,
	; which selects between inline NOPs and this routine depending on count.
	;
	; Entry:  B = iteration count (must be >= 1; B=0 wraps to 256 and delays ~1033 NOPs)
	; Exit:   B = 0
	;
	; Cost per call (including ld b,n + call overhead of 7 NOPs):
	;   Total NOPs = 4*B + 9
	;
	; Derivation:
	;   Overhead:           ld b,n (2) + call (5)       =  7 NOPs
	;   B-1 taken djnz:     (B-1) * 4                   = 4B-4 NOPs
	;   Final djnz (B->0):  3 NOPs (not-taken)
	;   ret:                3 NOPs
	;   Total = 7 + (4B-4) + 3 + 3 = 4B + 9 NOPs
	;
	; Destroys: B
	; Preserves: A, C, DE, HL, IX, IY, F (flags undefined after djnz)
	;
	; Size: 3 bytes.
	;
	; ---------------------------------------------------------------------------
	WaitNops_Routine:
		djnz WaitNops_Routine			; 4 NOPs taken / 3 NOPs not-taken (B -> 0)
		ret								; 3 NOPs

	; ---------------------------------------------------------------------------
	;
	; WaitNops <nops>
	;
	; Macro that emits a cycle-exact delay of exactly <nops> NOPs (4*nops T-states).
	;
	; For small counts (< 13) inline NOPs are emitted directly -- no subroutine
	; call overhead and no register usage.
	;
	; For larger counts the macro calls WaitNops_Routine with a precomputed B value
	; and appends 0-3 inline NOPs for the remainder:
	;
	;   ld b, (nops - 9) / 4    -- integer quotient
	;   call WaitNops_Routine    -- burns 4*B + 9 NOPs
	;   <0-3 inline NOPs>        -- burns the remainder: (nops - 9) % 4
	;
	; Remainder encoding (minimises bytes):
	;   0 remainder: nothing
	;   1 remainder: nop              (1 byte)
	;   2 remainder: inc bc           (1 byte -- 2 NOPs in 1 byte vs nop+nop = 2 bytes)
	;   3 remainder: jr $+2           (2 bytes -- 3 NOPs in 2 bytes vs 3x nop = 3 bytes)
	;
	; Threshold rationale:
	;   WaitNops_Routine requires B >= 1. Minimum subroutine cost = 4*1+9 = 13 NOPs.
	;   Counts < 13 are handled inline to avoid the B=0 wrap-around hazard and to
	;   avoid calling for fewer cycles than the call overhead itself costs.
	;
	; Destroys: B, C (inc bc used in both paths for density; both are free registers).
	; Preserves: A, DE, HL, IX, IY, F.
	;
	; ---------------------------------------------------------------------------
	macro WaitNops nops
		if {nops} < 13
			; Inline path: inc bc = 2 NOPs in 1 byte; nop for odd remainder.
			repeat {nops} / 2
				inc bc
			rend
			if {nops} % 2 == 1
				nop
			endif
		else
			ld b,({nops} - 9) / 4		; B = quotient: subroutine will burn 4*B + 9 NOPs
			call WaitNops_Routine
			; Cycle-exact padding for remainder: (nops - 9) % 4 in [0..3]
			if ({nops} - 9) % 4 == 1
				nop						; 1 NOP
			endif
			if ({nops} - 9) % 4 == 2
				inc bc					; 2 NOPs in 1 byte
			endif
			if ({nops} - 9) % 4 == 3
				jr $+2					; 3 NOPs in 2 bytes (saves 1 byte vs 3x nop)
			endif
		endif
	mend
endif
