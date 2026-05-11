;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC IM1 interrupt sync helpers.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_interrupts_im1_sync_asm__
__cpc_interrupts_im1_sync_asm__ equ 1
	;
	; Require macros.ga.asm to be included
	include "macros.ga.asm"

	; This file provides one VSYNC-synchronised IM 1 handler install function.
	;
	;   CPC_InstallIM1Sync -- DI + JP UserHandler at #0038 + resync + EI.
	;       Use this when a full interrupt handler is needed (e.g. music replay or
	;       other per-interrupt work). Identical to CPC_InstallIM1 plus the resync.
	;
	; For EI-raster technique (HALT-based main loop timing) use
	; cpc.interrupts.disable.sync.asm (CPC_DisableInterruptsSync).
	;
	; ============================================================
	; Why synchronisation is needed
	; ============================================================
	;
	; The GA fires an IM 1 interrupt every 52 HSYNC pulses. With the standard
	; CRTC configuration (R4=38, R5=0, giving 312 lines per frame at 50 Hz)
	; exactly 6 interrupts fire per frame. The GA maintains an independent
	; 6-bit counter that wraps at 52 and has no fixed phase relationship to
	; VSYNC unless forced. Without synchronisation the first interrupt after
	; install can fall anywhere within a 52-line window and all subsequent
	; interrupts carry the same unpredictable offset, making raster effects,
	; palette splits, and per-scanline timing impossible to rely on.
	;
	; ============================================================
	; Synchronisation method
	; ============================================================
	;
	; Writing GA_CMD_MODE_ROM | GA_IRQ_RESET to the GA resets the 52-HSYNC
	; counter to 0 and cancels any pending interrupt. Performing this write
	; at the VSYNC rising edge restarts the counter at the same scanline
	; every frame, so interrupt N of each frame always falls on the same
	; raster line.
	;
	; Resync sequence (executed after patching RST #38):
	;
	;   1. Wait for VSYNC inactive -- spin while PPI Port A bit 0 is high.
	;      Ensures we do not catch a mid-pulse edge and get a truncated first
	;      interrupt window.
	;   2. Wait for VSYNC rising edge -- spin while bit 0 is low.
	;   3. Write GA_CMD_MODE_ROM | GA_IRQ_RESET at the leading edge.
	;      The GA counter resets to 0 at this exact scanline.
	;
	; With standard CRTC settings the 6 interrupts per frame then fall at
	; these approximate lines relative to VSYNC start:
	;   Int 1: line  +52     Int 4: line +208
	;   Int 2: line +104     Int 5: line +260
	;   Int 3: line +156     Int 6: line +312 (coincides with next VSYNC)
	;
	; GA_IRQ_RESET is a bit inside the GA_CMD_MODE_ROM group, so writing it
	; requires a full mode/ROM command byte. The caller's mode byte (B on
	; entry) is ORed with GA_IRQ_RESET for the resync write, preserving the
	; screen mode throughout. Both ROMs are disabled unconditionally.

	; ============================================================
	; Constants
	; ============================================================

	; Shared with cpc.interrupts.im1.asm -- defined here if that file is not included.
	ifndef INT_VECTOR_ADDR
	INT_VECTOR_ADDR		equ #0038
	endif
	ifndef INT_VECTOR_HANDLER
	INT_VECTOR_HANDLER	equ #0039
	endif
	ifndef INT_JP_OPCODE
	INT_JP_OPCODE		equ #c3
	endif

	; ============================================================
	; CPC_InstallIM1Sync
	; ============================================================

	; CPC_InstallIM1Sync -- DI, install JP UserHandler at #0038, resync the GA counter, EI.
	; Entry: HL = address of the user interrupt service routine
	;        B  = GA mode byte (GA_MODE_n | rom_flags) -- mode bits preserved; both ROMs disabled unconditionally
	; Exit:  #0038 in RAM contains JP UserHandler; both ROMs disabled; GA counter reset to VSYNC phase; interrupts enabled
	; Destroys: AF, BC, DE, HL  Preserves: IX, IY
	CPC_InstallIM1Sync:
		di
		; Save caller's mode byte -- B will be repurposed for port addresses.
		ld d,b
		; Disable both ROMs so reads at #0038-#003A come from RAM, not ROM.
		ld a,d
		or GA_LROM_DISABLE|GA_UROM_DISABLE
		ld b,GA_PORT_CMD_B
		ld c,a
		out (c),c
		; Set the interrupt mode 1 
		; Note: For CPC+ this could be revisited
		im 1
		; Write JP UserHandler at #0038 in RAM (opcode + 16-bit address).
		ld a,INT_JP_OPCODE
		ld (INT_VECTOR_ADDR),a
		ld (INT_VECTOR_HANDLER),hl
		; Both ROMs remain disabled -- firmware is silenced, neither ROM is needed.
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
