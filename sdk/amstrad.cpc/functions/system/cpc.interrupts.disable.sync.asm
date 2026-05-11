;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC interrupt disable sync helpers.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_interrupts_disable_sync_asm__
__cpc_interrupts_disable_sync_asm__ equ 1
	;
	; We require the GA macros
	;
	include "macros.ga.asm"
	; Address of the RST #38 RAM vector.
	ifndef INT_NULL_VECTOR_ADDR
	INT_NULL_VECTOR_ADDR	equ #0038
	endif

	; CPC_DisableInterruptsSync -- DI, poke EI+RETI into #0038, resync GA counter to VSYNC, EI.
	;
	; Combines CPC_DisableInterrupts with a VSYNC-edge GA counter reset, so the
	; null interrupt windows are phase-locked to the display from the first frame.
	; Use this when raster timing uses HALT in the main loop to step through the
	; 6 interrupt windows per frame (EI-raster technique).
	;
	; The null handler at #0038 (EI+RETI) re-enables interrupts instantly, so
	; each HALT in the caller wakes on the next GA interrupt and returns to the
	; instruction after HALT with no overhead.
	;
	; Entry:
	;   B  = GA mode/ROM command byte (GA_MODE_n | rom_flags).
	;        Mode bits are preserved as-is. Both ROMs are disabled unconditionally.
	;
	; Exit:  #0038 = EI (#FB), #0039-#003A = RETI (#ED #4D); both ROMs disabled;
	;        GA counter reset to VSYNC phase; interrupts enabled.
	; Destroys: AF, BC, DE  Preserves: HL, IX, IY
	CPC_DisableInterruptsSync:
		di
		; Save caller's mode byte -- B will be repurposed for port addresses.
		ld d,b
		; Disable both ROMs 
		ld a,d
		or GA_LROM_DISABLE|GA_UROM_DISABLE
		ld b,GA_PORT_CMD_B
		ld c,a
		out (c),c
		im 1
		; Poke EI (#FB) + RETI (#ED #4D) directly into the RST #38 vector in RAM.
		ld a,#fb
		ld (INT_NULL_VECTOR_ADDR),a
		ld a,#ed
		ld (INT_NULL_VECTOR_ADDR+1),a
		ld a,#4d
		ld (INT_NULL_VECTOR_ADDR+2),a
		; Resync GA interrupt counter to VSYNC rising edge.
		; Phase 1: wait for VSYNC to go inactive (skip any currently active pulse).
		ld b,GA_PORT_VSYNC_B
	.wait_vsync_off:
		in a,(c)
		rra
		jr c,.wait_vsync_off
		; Phase 2: wait for VSYNC rising edge.
	.wait_vsync_on:
		in a,(c)
		rra
		jr nc,.wait_vsync_on
		; Phase 3: reset GA interrupt counter at the VSYNC leading edge.
		ld a,d
		or GA_IRQ_RESET |  GA_LROM_DISABLE | GA_UROM_DISABLE
		ld b,GA_PORT_CMD_B
		ld c,a
		out (c),c
		ei
		ret
endif
