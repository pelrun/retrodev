;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC interrupt disable helpers.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_interrupts_disable_asm__
__cpc_interrupts_disable_asm__ equ 1
	;
	; We require the GA macros
	;
	include "macros.ga.asm"
	; CPC_DisableInterrupts -- install a null IM 1 handler that does nothing.
	;
	; Pokes EI (#FB) + RETI (#ED #4D) directly into the three bytes at #0038,
	; so the GA interrupt keeps firing (preventing any pending /INT from
	; accumulating) but executes no firmware or user code. All firmware
	; activity ceases: keyboard scan, sound tick, event queue, and frame-
	; flyback callbacks are all permanently silenced.
	;
	; This is intentionally NOT a bare DI. Keeping interrupts acknowledged
	; allows the Z80 to remain in a clean state: /INT is never held asserted,
	; and EI can be issued at any time without an immediate spurious handler
	; call. It also means the stack overhead per interrupt is exactly 2 bytes
	; (the RST #38 push of PC) plus the matching RETI pop -- no JP, no stub.
	;
	; Entry:
	;   B = GA mode/ROM command byte (GA_MODE_n | rom_flags).
	;       The GA mode register is write-only with no readback, so the caller
	;       must supply this value explicitly. It is ORed with GA_LROM_DISABLE
	;       and GA_UROM_DISABLE before writing, so both ROMs are disabled
	;       unconditionally. The mode bits from B are preserved as-is.
	;
	; Exit:  #0038 contains EI (#FB) + RETI (#ED #4D) directly in RAM;
	;        both ROMs left disabled; interrupts enabled (null handler active).
	; Destroys: AF, BC
	; Preserves: DE, HL, IX, IY

	; ============================================================
	; Constants
	; ============================================================

	; Address of the RST #38 RAM vector.
	ifndef INT_NULL_VECTOR_ADDR
	INT_NULL_VECTOR_ADDR	equ #0038
	endif

	; ============================================================
	; CPC_DisableInterrupts
	; ============================================================

	; CPC_DisableInterrupts -- DI, poke EI+RETI directly into #0038, re-enable interrupts.
	; Firmware activity ceases; the null handler keeps /INT acknowledged so the Z80 stays clean.
	; Entry: B = GA mode byte (GA_MODE_n | rom_flags) -- mode bits preserved; both ROMs disabled unconditionally
	; Exit:  #0038 = EI (#FB), #0039-#003A = RETI (#ED #4D); both ROMs disabled; interrupts enabled
	; Destroys: AF, BC  Preserves: DE, HL, IX, IY
	CPC_DisableInterrupts:
		di
		; Disable both ROMs so reads at #0038-#003A come from RAM, not ROM.
		ld a,b
		or GA_LROM_DISABLE|GA_UROM_DISABLE
		ld b,GA_PORT_CMD_B
		ld c,a
		out (c),c
		im 1
		; Poke EI (#FB) + RETI (#ED #4D) directly into the RST #38 vector in RAM.
		; No JP needed -- the three bytes fit exactly in #0038-#003A.
		ld a,#fb
		ld (INT_NULL_VECTOR_ADDR),a
		ld a,#ed
		ld (INT_NULL_VECTOR_ADDR+1),a
		ld a,#4d
		ld (INT_NULL_VECTOR_ADDR+2),a
		; Both ROMs remain disabled -- firmware is silenced, neither ROM is needed.
		ei
		ret
endif
