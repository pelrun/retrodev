;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC keyboard macros.
;
; Purpose: define keyboard matrix constants, buffers, and helper macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __macros_keyboard_asm__
__macros_keyboard_asm__ equ 1

	; Amstrad CPC keyboard -- shared definitions, buffer, and test macro.
	;
	; Include this file before cpc.keyboard.asm and/or cpc.keys.asm.
	; It may also be included alone when only KEY_PRESSED tests are needed
	; against a buffer filled by either scanner.
	;
	; ============================================================
	; CPC keyboard matrix -- full row / column map
	; ============================================================
	;
	; The matrix has 10 rows (selected via PPI Port C bits 3-0) and 8 columns
	; (read from PPI Port A). A bit value of 0 means the key is pressed.
	;
	;   Row 0 : bit7=FDot       bit6=NumEnter    bit5=F3         bit4=F6       bit3=F9    bit2=CursorDown bit1=CursorRight bit0=CursorUp
	;   Row 1 : bit7=F0         bit6=F2          bit5=F1         bit4=F5       bit3=F8    bit2=F7         bit1=Copy        bit0=CursorLeft
	;   Row 2 : bit7=Ctrl       bit6=Equal       bit5=Shift      bit4=F4       bit3=BracketR bit2=Return bit1=BracketL    bit0=Clr/Home
	;   Row 3 : bit7=Dot        bit6=Slash       bit5=Colon      bit4=Semicl   bit3=P     bit2=At         bit1=Minus       bit0=Caret
	;   Row 4 : bit7=Comma      bit6=M           bit5=K          bit4=L        bit3=I     bit2=O          bit1=9           bit0=0
	;   Row 5 : bit7=Space      bit6=N           bit5=J          bit4=H        bit3=Y     bit2=U          bit1=7           bit0=8
	;   Row 6 : bit7=V          bit6=B           bit5=F          bit4=G        bit3=T     bit2=R          bit1=5           bit0=6
	;   Row 7 : bit7=X          bit6=C           bit5=D          bit4=S        bit3=W     bit2=E          bit1=3           bit0=4
	;   Row 8 : bit7=Z          bit6=CapsLock    bit5=A          bit4=Tab      bit3=Q     bit2=Escape     bit1=2           bit0=1
	;   Row 9 : bit7=Del        bit6=Joy1Fire3   bit5=Joy1Fire2  bit4=Joy1Fire1 bit3=Joy1Right bit2=Joy1Left bit1=Joy1Down bit0=Joy1Up
	;
	; ============================================================
	; Key encoding
	; ============================================================
	;
	; Each key is represented by one byte: bits 7-4 = row (0-9), bits 3-0 = column bit (0-7).
	; Pass a list of these bytes to KeyScanInit to configure the selective scanner,
	; or use KEY_PRESSED after any scanner call to test individual keys.
	;
	; Example: KEY_CURSOR_UP = row 0, bit 0 -> #00
	;          KEY_SPACE     = row 5, bit 7 -> #57
	;
	; ============================================================
	; Row 0
	; ============================================================
	KEY_CURSOR_UP		equ #00
	KEY_CURSOR_RIGHT	equ #01
	KEY_CURSOR_DOWN		equ #02
	KEY_F9				equ #03
	KEY_F6				equ #04
	KEY_F3				equ #05
	KEY_ENTER_NUM		equ #06		; numeric keypad Enter
	KEY_FDOT			equ #07		; numeric keypad dot

	; ============================================================
	; Row 1
	; ============================================================
	KEY_CURSOR_LEFT		equ #10
	KEY_COPY			equ #11
	KEY_F7				equ #12
	KEY_F8				equ #13
	KEY_F5				equ #14
	KEY_F1				equ #15
	KEY_F2				equ #16
	KEY_F0				equ #17

	; ============================================================
	; Row 2
	; ============================================================
	KEY_CLR				equ #20		; Clr / Home
	KEY_BRACKET_L		equ #21
	KEY_RETURN			equ #22
	KEY_BRACKET_R		equ #23
	KEY_DEL_FWD			equ #24		; F4 / forward delete on some layouts
	KEY_SHIFT			equ #25
	KEY_EQUAL			equ #26
	KEY_CTRL			equ #27

	; ============================================================
	; Row 3
	; ============================================================
	KEY_CARET			equ #30
	KEY_MINUS			equ #31
	KEY_AT				equ #32
	KEY_P				equ #33
	KEY_SEMICOLON		equ #34
	KEY_COLON			equ #35
	KEY_SLASH			equ #36
	KEY_DOT				equ #37

	; ============================================================
	; Row 4
	; ============================================================
	KEY_0				equ #40
	KEY_9				equ #41
	KEY_O				equ #42
	KEY_I				equ #43
	KEY_L				equ #44
	KEY_K				equ #45
	KEY_M				equ #46
	KEY_COMMA			equ #47

	; ============================================================
	; Row 5
	; ============================================================
	KEY_8				equ #50
	KEY_7				equ #51
	KEY_U				equ #52
	KEY_Y				equ #53
	KEY_H				equ #54
	KEY_J				equ #55
	KEY_N				equ #56
	KEY_SPACE			equ #57

	; ============================================================
	; Row 6
	; ============================================================
	KEY_6				equ #60
	KEY_5				equ #61
	KEY_R				equ #62
	KEY_T				equ #63
	KEY_G				equ #64
	KEY_F				equ #65
	KEY_B				equ #66
	KEY_V				equ #67

	; ============================================================
	; Row 7
	; ============================================================
	KEY_4				equ #70
	KEY_3				equ #71
	KEY_E				equ #72
	KEY_W				equ #73
	KEY_S				equ #74
	KEY_D				equ #75
	KEY_C				equ #76
	KEY_X				equ #77

	; ============================================================
	; Row 8
	; ============================================================
	KEY_1				equ #80
	KEY_2				equ #81
	KEY_ESCAPE			equ #82
	KEY_Q				equ #83
	KEY_TAB				equ #84
	KEY_A				equ #85
	KEY_CAPS_LOCK		equ #86
	KEY_Z				equ #87

	; ============================================================
	; Row 9
	; ============================================================
	KEY_DEL				equ #97
	KEY_JOY1_FIRE3		equ #96
	KEY_JOY1_FIRE2		equ #95
	KEY_JOY1_FIRE1		equ #94
	KEY_JOY1_RIGHT		equ #93
	KEY_JOY1_LEFT		equ #92
	KEY_JOY1_DOWN		equ #91
	KEY_JOY1_UP			equ #90
	KEY_JOY2_FIRE		equ KEY_JOY1_FIRE3

	; ============================================================
	; Shared keyboard result buffer
	; ============================================================
	;
	; 10 bytes -- one per keyboard matrix row, bit=0 means key pressed.
	; Filled by KeyboardScanner (cpc.keyboard.asm) or by KeyScanRead (cpc.keys.asm).
	; KEY_PRESSED reads from this buffer.
	Keymap	ds 10,#ff

	; ============================================================
	; KeyPressedM -- inline test macro
	; ============================================================
	;
	; Tests whether a single key is pressed according to the last scanner call.
	; Expands to 2-3 instructions with no function call overhead.
	;
	; Usage:
	;   KeyPressedM KEY_SPACE       ; Carry set if Space is pressed
	;
	; After the macro:
	;   Carry set   = key is pressed  (matrix bit was 0)
	;   Carry clear = key is released (matrix bit was 1)
	;
	; Timing:
	;   Bit 7 (one rlca):        13 + 4 + 4 = 21 T
	;   Bit 0 (one rrca):        13 + 4 + 4 = 21 T
	;   Bits 1-6 (2-7 * rrca):  13 + (bit+1)*4 + 4 = 21-45 T
	;
	; How the bit shift works:
	;   The matrix byte has bit K = 0 when the key is pressed.
	;   We want Carry = 1 when pressed. So we shift bit K into carry, then ccf.
	;   rrca after N applications brings old bit (N-1) into carry.
	;   For bit K: apply (K+1) * rrca, then ccf.
	;   For bit 7: rlca shifts bit 7 into carry in one step (saves 7 * rrca).
	macro KeyPressedM key
	ld a,(Keymap + (({key} >> 4) & #0f))
		repeat (({key} & #0f) + 1)
			rrca
		rend
		ccf
	endm
endif
