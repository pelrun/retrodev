;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC key constants and helpers.
;
; Purpose: define key codes and shared key-access utilities.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_keyboard_scanner_partial_asm__
__cpc_keyboard_scanner_partial_asm__ equ 1

	;
	; Require macros.keyboard.asm
	;
	include "macros.keyboard.asm"
	; cpc.keys.asm -- selective keyboard row scanner for the Amstrad CPC.
	;
	; Companion to cpc.keyboard.asm. Where KeyboardScanner always reads all 10
	; keyboard matrix rows, this module lets the caller declare a fixed set of keys
	; and then read only the matrix rows those keys occupy -- skipping every other
	; row entirely. The speed gain is proportional to the number of rows skipped.
	;
	; Key constants (KEY_xxx) and the Keymap result buffer are defined in
	; macros.keyboard.asm, which must be included before this file.
	;
	; ============================================================
	; Implementation overview
	; ============================================================
	;
	; KeyScanInit (call once, or whenever the key set changes):
	;   Receives a list of KEY_xxx constants and patches KeyScan_Body so that
	;   every row referenced by at least one key in the list executes a real PPI
	;   read, and every unreferenced row just advances the buffer pointer.
	;
	; KeyScanRead (call every frame):
	;   Runs the PPI setup sequence, then calls into KeyScan_Body. The patched
	;   unrolled body reads only the needed rows into Keymap.
	;
	; KEY_PRESSED key_constant (inline macro from macros.keyboard.asm, use after KeyScanRead):
	;   Tests a single key from Keymap in 17-21 T with no function call.
	;
	; ============================================================
	; Unrolled body layout
	; ============================================================
	;
	; KeyScan_Body contains 10 fixed-size slots, one per matrix row (row 0 first).
	; Between consecutive slots there is a single 'inc a' byte (#3C) to advance the
	; PPI row selector. There is no trailing 'inc a' after the last slot.
	;
	; Each slot is 6 bytes:
	;
	;   Active slot (row is in the key set):
	;     #42           ld b,d          4 T -- load PPI Port C high byte from cache
	;     #ED #79       out (c),a      12 T -- write row selector to Port C
	;     #43           ld b,e          4 T -- load PPI Port A high byte from cache
	;     #ED #A2       ini            16 T -- read column bits -> (HL), inc HL, dec B
	;
	;   Skipped slot (row is not in the key set):
	;     #23           inc hl          6 T -- advance buf pointer without reading
	;     #00 #00 #00 #00 #00   nop*5  20 T -- padding
	;
	; Total body size: 10 * 6 + 9 * 1 = 69 bytes.
	; Body is called with D=#f6, E=#f4, A=#40, HL->Keymap (set up by KeyScanRead).
	;
	; Slot N starts at offset N * 7 within KeyScan_Body.

	; ============================================================
	; Slot opcode constants (used by KeyScanInit for patching)
	; ============================================================
	KS_ACTIVE_0		equ #42		; ld b,d
	KS_ACTIVE_1		equ #ed		; out (c),a -- prefix byte
	KS_ACTIVE_2		equ #79		; out (c),a -- opcode byte
	KS_ACTIVE_3		equ #43		; ld b,e
	KS_ACTIVE_4		equ #ed		; ini -- prefix byte
	KS_ACTIVE_5		equ #a2		; ini -- opcode byte
	KS_SKIP_0		equ #23		; inc hl
	KS_PAD			equ #00		; nop (padding for skipped slots)
	KS_INC_A		equ #3c		; inc a (inter-slot separator)
	KS_SLOT_STRIDE	equ 7		; bytes between the start of slot N and slot N+1

	; ============================================================
	; Static data
	; ============================================================

	; Unrolled body -- patched at runtime by KeyScanInit.
	KeyScan_Body:
		; Slot 0
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 1
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 2
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 3
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 4
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 5
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 6
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 7
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 8
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_INC_A
		; Slot 9 -- no trailing inc a
		db KS_SKIP_0, KS_PAD, KS_PAD, KS_PAD, KS_PAD, KS_PAD
	KeyScan_BodyEnd:
		ret				; return to KeyScanRead after the body executes

	; ============================================================
	; KeyScanInit
	; ============================================================
	;
	; Resets all 10 slots to the 'skip' pattern, then iterates the caller's key
	; list and patches each referenced row's slot to the 'active' pattern.
	;
	; Entry:
	;   HL = pointer to key list (one KEY_xxx byte per key)
	;   C  = number of keys in the list (1-80)
	;
	; Exit:  KeyScan_Body patched and ready for KeyScanRead
	; Destroys: AF, BC, DE, HL
	; Preserves: IX, IY

	; KeyScanInit -- patch KeyScan_Body to scan only the rows needed by the given key list.
	; Entry: HL = pointer to key list (one KEY_xxx byte per entry)
	;        C  = number of keys in the list (1-80)
	; Exit:  KeyScan_Body patched and ready for KeyScanRead
	; Destroys: AF, BC, DE, HL  Preserves: IX, IY
	KeyScanInit:
		; Save IX and the entry parameters before using HL for body patching.
		push ix
		ld ixl,l
		ld ixh,h			; IX = key list pointer (HL freed for body traversal)
		; B = key count (C may be reused as scratch inside the patch loops).
		ld b,c
		push bc				; save key count in B on the stack; restore after reset loop
		; Reset all 10 slots to the 'skip' pattern.
		; Walk KeyScan_Body slot by slot. Each slot is 6 bytes; inter-slot inc_a bytes
		; are skipped by testing whether more slots remain before advancing past them.
		ld hl,KeyScan_Body
		ld c,10				; C = slots remaining
	.ksi_reset:
		ld (hl),KS_SKIP_0		; slot byte 0: inc hl
		inc hl
		ld (hl),KS_PAD			; slot byte 1
		inc hl
		ld (hl),KS_PAD			; slot byte 2
		inc hl
		ld (hl),KS_PAD			; slot byte 3
		inc hl
		ld (hl),KS_PAD			; slot byte 4
		inc hl
		ld (hl),KS_PAD			; slot byte 5
		inc hl
		dec c
		jr z,.ksi_reset_done		; last slot: no trailing inc_a to skip
		inc hl				; skip the inc_a separator between this slot and the next
		jr .ksi_reset
	.ksi_reset_done:
		; Patch active slots for each key in the list.
		pop bc				; B = key count
		.ksi_patch_loop:
		ld a,(ix+0)			; A = key byte: (row<<4) | col_bit
		inc ix				; advance past this key
		; Extract row number from the upper nibble.
		rrca
		rrca
		rrca
		rrca				; A = row (0-9), now in bits 3-0
		and #0f
		; Compute slot address: KeyScan_Body + row * KS_SLOT_STRIDE (= row * 7).
		; Multiply A by 7 using: A*7 = A*8 - A.
		ld e,a				; save row in E
		add a,a				; A = row * 2
		add a,a				; A = row * 4
		add a,a				; A = row * 8
		sub e				; A = row * 7
		ld e,a
		ld d,0				; DE = row * 7  (row <= 9 -> row*7 <= 63, fits in 8 bits, D=0)
		ld hl,KeyScan_Body
		add hl,de			; HL = address of this row's slot
		; Patch the 6-byte slot to the active read sequence.
		ld (hl),KS_ACTIVE_0		; ld b,d   (#42)
		inc hl
		ld (hl),KS_ACTIVE_1		; out(c),a prefix (#ED)
		inc hl
		ld (hl),KS_ACTIVE_2		; out(c),a opcode (#79)
		inc hl
		ld (hl),KS_ACTIVE_3		; ld b,e   (#43)
		inc hl
		ld (hl),KS_ACTIVE_4		; ini prefix (#ED)
		inc hl
		ld (hl),KS_ACTIVE_5		; ini opcode (#A2)
		djnz .ksi_patch_loop
		pop ix
		ret

	; ============================================================
	; KeyScanRead
	; ============================================================
	;
	; Performs the PPI / AY setup sequence, then calls KeyScan_Body which reads
	; only the rows patched as active by KeyScanInit. Results land in KeyScan_Buf.
	;
	; Entry:  none
	; Exit:   Keymap updated for all rows in the key set
	; Destroys: AF, BC, DE, HL
	; Preserves: IX, IY

	; KeyScanRead -- run the PPI setup and execute KeyScan_Body to read active rows into Keymap.
	; Entry: none  (KeyScanInit must have been called first)
	; Exit:  Keymap updated for all rows in the active key set
	; Destroys: AF, BC, DE, HL  Preserves: IX, IY
	KeyScanRead:
		; PPI / AY setup: identical to the KeyboardScanner preamble.
		; Port A = output, Port C = output.
		ld bc,#f782
		out (c),c
		; Port A <- #0e: place AY R14 index on the AY address bus.
		ld bc,#f40e
		ld e,b				; E = #f4 (PPI Port A high byte -- D/E cached for body use)
		out (c),c
		; Port C <- #c0 (BDIR=1, BC2=1): latch R14 as the active AY register.
		ld bc,#f6c0
		ld d,b				; D = #f6 (PPI Port C high byte -- cached for body use)
		out (c),c
		; Port C <- #00 (BDIR=0, BC2=0): complete the AY strobe.
		xor a
		out (c),a
		; Port A = input (keyboard column bits).
		ld bc,#f792
		out (c),c
		; Arm the body: A = first row selector (#40 = BC2=1, row 0), HL -> result buffer.
		ld a,#40
		ld hl,Keymap
		; Execute the self-patched unrolled read body.
		; KeyScan_BodyEnd holds a 'ret' that returns here.
		call KeyScan_Body
		; Restore PPI to its default state (Port A = output, Port C = output).
		ld bc,#f782
		out (c),c
		ret


endif
