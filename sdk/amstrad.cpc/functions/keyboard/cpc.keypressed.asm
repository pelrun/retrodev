;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC check for a key into the keymap (function version).
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_keyboard_check_asm__
__cpc_keyboard_check_asm__ equ 1

    include "macros.keyboard.asm"

    ; ---------------------------------------------------------------------------
    ;
    ; KeyPressedF
    ;
    ; Checks if a key is currently pressed in the hardware matrix state.
    ;
    ; On entry:
    ;   A = key code (bits 4-7: row, bits 0-3: bit index)
    ; On exit:
    ;   Carry set if key is pressed, clear if not.
    ; Destroys: A, B, C, D, E, H, L, F
    ;
    ; ---------------------------------------------------------------------------
    KeyPressedF:
        ld c,a
        rlca
        rlca
        rlca
        rlca
        and #0f
        ld e,a
        ld d,0
        ld hl,Keymap
        add hl,de
        ld a,c
        and #0f
        inc a
        ld b,a
        ld a,(hl)
    .ck_loop:
        rrca
        djnz .ck_loop
        ccf
        ret
endif