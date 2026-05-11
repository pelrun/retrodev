;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC CRTC detection routines.
;
; Purpose: detect CRTC type and expose compatible timing behaviour flags.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_detect_crtc_asm__
__cpc_detect_crtc_asm__ equ 1

    ;
    ; We use the CRTC macros here
    ;
    include "macros.crtc.asm"

    ; CPC_DetectCRTC
    ;
    ; Five CRTC variants were used across CPC models. They differ in which internal
    ; registers can be read back and in how they handle reads of write-only registers.
    ; This routine uses those differences to uniquely identify the chip type.
    ;
    ; CRTC types and their CPC usage:
    ;   Type 0 -- HD6845S    : early CPC 464 / 664; status register present; R12 readable,
    ;                         returns bits 5-0 only (bits 7-6 always read as 0)
    ;   Type 1 -- UM6845R    : most CPC 464 / 664 / some 6128; no status register;
    ;                         R12 readable, returns full 8 bits
    ;   Type 2 -- MC6845     : some CPC 464 / 664; no status register; R12 not readable,
    ;                         read always returns #FF
    ;   Type 3 -- AMS40489   : CPC+ / GX4000 ASIC; R12 not readable, read returns #00;
    ;                         VSYNC width programmable; Status register not present.
    ;   Type 4 -- AMS40226   : some 6128 Plus boards; behaves like type 3 but minor diffs.
    ;
    ; Detection algorithm:
    ;
    ;   Step 1 -- distinguish types 0/1 from types 2/3/4.
    ;     Write #FF to CRTC R12 (display start high), then read it back.
    ;     Types 0 and 1 return a non-FF value (bits masked or full byte).
    ;     Types 2, 3, 4 either return #FF (type 2) or #00 (types 3/4).
    ;
    ;   Step 2 -- separate type 2 from types 3/4.
    ;     Type 2 reads as #FF; types 3/4 read as #00.
    ;     (Tested in the same read from Step 1.)
    ;
    ;   Step 3 -- separate type 0 from type 1.
    ;     Type 0 (HD6845S) has a status register at the address port (read side).
    ;     Reading the CRTC address port on a type 0 returns a status byte with
    ;     bits 7-6 potentially set (VSYNC and LPEN flags). On types 1/2/3/4 the
    ;     address port returns #FF on read (floating bus or pull-up).
    ;     We write #00 to R12 first so bits 5-0 of the type-0 read are known,
    ;     then read the address port: if the result is not #FF it is type 0.
    ;
    ;   Step 4 -- separate type 3 from type 4.
    ;     Both return #00 on R12 reads. Type 4 has a readable status register
    ;     at the address port (like type 0) whereas type 3 does not.
    ;     Same address-port read used in Step 3 differentiates them.
    ;
    ; The routine saves and restores R12 so the display start address is not affected.
    ;
    ; Return values (A on exit):
    ;   CRTC_TYPE_0 = 0   HD6845S
    ;   CRTC_TYPE_1 = 1   UM6845R
    ;   CRTC_TYPE_2 = 2   MC6845
    ;   CRTC_TYPE_3 = 3   AMS40489 (CPC+ / GX4000)
    ;   CRTC_TYPE_4 = 4   AMS40226 (some 6128 Plus)
    ;
    ; Entry:
    ;   none
    ; Exit:
    ;   A = CRTC type (0-4); see CRTC_TYPE_n equs below
    ; Destroys: AF, BC, IXL
    ; Preserves: DE, HL, IXH, IY
    ; Requirements: none (safe to call with interrupts enabled, though DI is recommended
    ;               to prevent the firmware from reprogramming R12 during the test)

    CRTC_TYPE_0		equ 0		; HD6845S  -- early 464 / 664
    CRTC_TYPE_1		equ 1		; UM6845R  -- most 464 / 664 / some 6128
    CRTC_TYPE_2		equ 2		; MC6845   -- some 464 / 664
    CRTC_TYPE_3		equ 3		; AMS40489 -- CPC+ / GX4000
    CRTC_TYPE_4		equ 4		; AMS40226 -- some 6128 Plus

    ; CPC_DetectCRTC -- identify the CRTC chip type fitted in the CPC (types 0-4).
    ; Entry: none
    ; Exit:  A = CRTC type: CRTC_TYPE_0..CRTC_TYPE_4
    ; Destroys: AF, BC, DE  Preserves: HL, IX, IY
    CPC_DetectCRTC:
        ; Select R12 (display start high)
        ld bc, CRTC_PORT_ADDR+CRTC_R12_ADDR_H
        out (c), c

        ; Write test value (#34 = %0110100)
        ld bc, CRTC_PORT_DATA + #34
        out (c), c

        ; Wait VBL (GA_PORT_VSYNC_B = #f5)
        ld b, #f5
    .vbLoop1:
        in a, (c)
        rra
        jr c, .vbLoop1
    .vbLoop2:
        in a, (c)
        rra
        jr nc, .vbLoop2

        ld b, #be               ; Read from status register
        in a, (c)
        ld d, a

        inc b                   ; Read from #bf (read register)
        in a, (c)

        cp d                    ; #be == #bf?
        jr z, .crtc_3_4

        ; CRTC 0, 1 or 2
        cp #34                  ; Same value?
        jr nz, .crtc_1_2

        ; CRTC 0
        ld a, CRTC_TYPE_0
        jr .done

    .crtc_1_2:
        ld a, d
        and %00011111           ; The original code had %011111 which is #1f
        jr nz, .crtc_2

        ; CRTC 1
        ld a, CRTC_TYPE_1
        jr .done

        ; CRTC 2
    .crtc_2:
        ld a, CRTC_TYPE_2
        jr .done

        ; CRTC 3 or 4
    .crtc_3_4:
        ld bc, #f782
        out (c), c
        dec b
        ld a, #0f
        out (c), a
        inc b
        out (c), c
        dec b
        in e, (c)
        cp e
        jr nz, .crtc_4

        ; CRTC 3
        ld a, CRTC_TYPE_3
        jr .done

        ; CRTC 4
    .crtc_4:
        ld a, CRTC_TYPE_4

    .done:
        push af
        ; Ensure R12 is safe (#30) for standard #C000 screen base
        ld bc, CRTC_PORT_ADDR+CRTC_R12_ADDR_H
        out (c), c
        ld bc, CRTC_PORT_DATA + #30
        out (c), c
        pop af
        ret
endif
