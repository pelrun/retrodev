;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC IM1 interrupt helpers.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_interrupts_im1_asm__
__cpc_interrupts_im1_asm__ equ 1
	;
	; Require macros.ga.asm to be included before this file.
	include "macros.ga.asm"

	; CPC_InstallIM1 -- redirect RST #38 to a user-supplied interrupt handler.
	;
	; Writes JP UserHandler directly at the RST #38 RAM vector (#0038-#003A),
	; so the very next GA interrupt jumps straight to the user's routine with
	; no intermediate stub and no stack overhead beyond the RST #38 push of PC.
	;
	; The CPC firmware runs entirely from this interrupt. Redirecting it stops
	; all firmware activity: keyboard scan, sound tick, event queue, and frame-
	; flyback callbacks all cease.
	;
	; ============================================================
	; How RST #38 redirection works
	; ============================================================
	;
	; The Z80 IM 1 interrupt calls address #0038. The CPC lower ROM (firmware)
	; occupies reads at #0000-#3FFF, so the CPU would execute ROM content at
	; #0038. Writes always go to RAM regardless of ROM state, but both ROMs
	; must be disabled so the Z80 reads back from RAM at #0038-#003A, not ROM.
	; CPC_InstallIM1 disables both ROMs unconditionally -- they are not restored.
	; The user handler lives in RAM above #4000; neither ROM is needed after
	; firmware activity ceases.
	;
	; ============================================================
	; Swapping the handler
	; ============================================================
	;
	; INT_VECTOR_HANDLER (#0039) is the address word of the JP at #0038.
	; To redirect to a different handler at any time, without calling
	; CPC_InstallIM1 again:
	;
	;   ld hl,NewHandler
	;   ld (INT_VECTOR_HANDLER),hl
	;
	; That is all. The next interrupt will jump to NewHandler.
	; ============================================================
	; Stack note
	; ============================================================
	;
	; No stub is imposed between RST #38 and the user handler -- the user
	; routine receives control with only the return address pushed by RST on
	; the stack (2 bytes). This allows the user to freely abuse SP for fast
	; data-poking techniques if desired, as long as SP is restored before the
	; final EI+RETI that closes the interrupt.
	;
	; Entry:
	;   HL = address of the user interrupt service routine
	;   B  = GA mode byte (GA_MODE_n | rom_flags).
	;        Mode bits are preserved as-is. Both ROMs are disabled unconditionally.
	;
	; Exit:  #0038 in RAM contains JP UserHandler; both ROMs disabled;
	;        interrupts enabled (EI issued on exit).
	; Destroys: AF, BC, HL
	; Preserves: DE, IX, IY

	; ============================================================
	; Constants
	; ============================================================

	; Address of the RST #38 RAM vector.
	ifndef INT_VECTOR_ADDR
	INT_VECTOR_ADDR		equ #0038
	endif
	; Address of the handler word inside the JP at #0038.
	; Read this to get the current handler; write it to redirect or chain.
	ifndef INT_VECTOR_HANDLER
	INT_VECTOR_HANDLER	equ #0039
	endif
	ifndef INT_JP_OPCODE
	INT_JP_OPCODE		equ #c3
	endif

	; ============================================================
	; CPC_InstallIM1
	; ============================================================

	; CPC_InstallIM1 -- DI, write JP UserHandler at RST #38, EI.
	; INT_VECTOR_HANDLER (#0039) is the patchable address word.
	; Entry: HL = address of the user interrupt service routine
	;        B  = GA mode byte (GA_MODE_n | rom_flags) -- mode bits preserved; both ROMs disabled unconditionally
	; Exit:  #0038 in RAM contains JP UserHandler; both ROMs disabled; interrupts enabled
	; Destroys: AF, BC, HL  Preserves: DE, IX, IY
	CPC_InstallIM1:
		di
		; Disable both ROMs so reads at #0038-#003A come from RAM, not ROM.
		ld a,b
		or GA_LROM_DISABLE|GA_UROM_DISABLE
		ld b,GA_PORT_CMD_B
		ld c,a
		out (c),c
		im 1
		; Write JP UserHandler at #0038 in RAM (opcode + 16-bit address).
		ld a,INT_JP_OPCODE
		ld (INT_VECTOR_ADDR),a
		ld (INT_VECTOR_HANDLER),hl
		; Both ROMs remain disabled -- firmware is silenced, neither ROM is needed.
		ei
		ret
endif
