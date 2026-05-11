;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC machine type detection routines.
;
; Purpose: identify target machine/model characteristics for runtime setup.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_detect_type_asm__
__cpc_detect_type_asm__ equ 1

    include "macros.ga.asm"
    include "macros.psg.asm"

    ; CPC_DetectType
    ;
    ; Discriminates between the main machine variants by probing hardware
    ; differences in a fixed order: firmware version first, then ASIC presence,
    ; then RAM configuration.
    ;
    ; Returned model constants (A on exit):
    ;   CPC_MODEL_464    = 0   CPC 464  -- 64K RAM,  CRTC type 0/1/2, ROM version 0
    ;   CPC_MODEL_664    = 1   CPC 664  -- 64K RAM,  CRTC type 0/1/2, ROM version 1
    ;   CPC_MODEL_6128   = 2   CPC 6128 -- 128K RAM, CRTC type 0/1/2, ROM version 3
    ;   CPC_MODEL_464P   = 3   CPC 464+ -- 128K RAM, ASIC (type 3/4), no 6128 extra page
    ;   CPC_MODEL_6128P  = 4   CPC 6128+-- 192K RAM, ASIC (type 3/4), 6128 extra page present
    ;   CPC_MODEL_GX4000 = 5   GX4000   -- 64K RAM,  ASIC (type 3/4), Amsdos ROM redirection
    ;
    ; Detection strategy (executed in this order):
    ;
    ;   Step 1 -- Firmware version detection via KL_ROM_PROBE (&B915)
    ;     Returns: 0 = 464, 1 = 664, 2/3/4 = 6128/Plus
    ;     Based on this, branch to appropriate detection path
    ;
    ;   Step 2 -- For 464 branch:
    ;     Directly return CPC_MODEL_464
    ;
    ;   Step 3 -- For 664 branch:
    ;     Directly return CPC_MODEL_664
    ;
    ;   Step 4 -- For 6128/Plus branch:
    ;     Check for ASIC presence via PPI behavior (classic 8255 vs ASIC PPI)
    ;     If ASIC present (Plus/GX4000 branch):
    ;       Check for AMSDOS ROM redirection (GX4000 lacks it) to distinguish GX4000 vs Plus
    ;     Then check for 6128 extra RAM page (464+ vs 6128+)
    ;
    ; Entry:
    ;   none
    ; Exit:
    ;   A = model constant (CPC_MODEL_464 .. CPC_MODEL_GX4000)
    ; Destroys: AF, BC, D, E, IXL
    ; Preserves: HL, IXH, IY
    ; Requirements: interrupts must be disabled (DI) before calling.
    ;               The lower ROM must be enabled (GA Mode/ROM byte bit 2 = 0).

    CPC_MODEL_464    equ 0   ; CPC 464
    CPC_MODEL_664    equ 1   ; CPC 664
    CPC_MODEL_6128   equ 2   ; CPC 6128
    CPC_MODEL_464P   equ 3   ; CPC 464+
    CPC_MODEL_6128P  equ 4   ; CPC 6128+
    CPC_MODEL_GX4000 equ 5   ; GX4000

    ; CPC_DetectType -- identify the specific CPC model (464, 664, 6128, 464+, 6128+, GX4000).
    ; Entry: none  (interrupts must be disabled; lower ROM must be enabled)
    ; Requirements: MUST be called at the very beginning of the booting process, 
    ;               before disabling ROMs or trashing memory, since it relies on 
    ;               reading literal bytes from the firmware OS ROM.
    ; Exit:  A = model constant: CPC_MODEL_464..CPC_MODEL_GX4000
    ; Destroys: AF, BC, DE, IXL  Preserves: HL, IXH, IY
    CPC_DetectType:
        ; -------------------------------------------------------
        ; Step 1: probe firmware version via KL_ROM_PROBE (&B915)
        ; -------------------------------------------------------
        ld c,0
        call #b915          ; KL_ROM_PROBE
        ld a,h
        cp 0                ; 0 = 464
        jr z, .is_464
        cp 1                ; 1 = 664
        jr z, .is_664
        cp 2                ; 2 = 6128/Plus
        jr z, .check_6128_plus
        cp 3                ; Looks like 3 is valid for CPC464+
        jr z,.check_6128_plus
        cp 4                ; Looks like 4 is valid for CPC6128+
        jr z,.check_6128_plus
        ; Unknown value - treat as 464
        ld a, CPC_MODEL_464
        ret
        ; -------------------------------------------------------
        ; Step 2: 464 branch
        ; -------------------------------------------------------
    .is_464:
        ld a, CPC_MODEL_464
        ret

        ; -------------------------------------------------------
        ; Step 3: 664 branch
        ; -------------------------------------------------------
    .is_664:
        ld a, CPC_MODEL_664
        ret

        ; -------------------------------------------------------
        ; Step 4: 6128/Plus branch
        ; -------------------------------------------------------
    .check_6128_plus:
        ; -------------------------------------------------------------------------
        ; Use PPI-based ASIC detection (highly reliable across all emulators/hardware)
        ; A classic 8255 PPI resets its output latch when the control register is 
        ; rewritten, returning 0 on read. The Amstrad Plus ASIC PPI retains the value.
        ; -------------------------------------------------------------------------

        ; B = #F7 (PPI Control), C = #82 (Port A out, B in, C out)
        ld bc, PSG_PPI_CTRL_B * 256 + PSG_PPI_PORTA_OUT
        ld e, c             ; E = #82 (save control byte)
        out (c), c          ; Set PPI mode

        dec b               ; B = #F6 (PPI Port C)
        ld c, #0f           ; Test value
        out (c), c          ; Write #0F to Port C

        inc b               ; B = #F7 (PPI Control)
        ld a, e             ; A = #82
        out (c), a          ; Rewrite PPI mode (this resets old 8255 latch)

        dec b               ; B = #F6 (PPI Port C)
        in a, (c)           ; Read back Port C

        ; ASIC (CPC Plus/GX4000) retains the #0F. Old 8255 returns 0.
        cp c                 ; Value retained? (#0F)
        jr z, .plus_branch
        or a                 ; Value reset? (0)
        jr z, .cpc_old

    .cpc_old:
        ; Classic 6128
        ld a, CPC_MODEL_6128
        ret

        ; -------------------------------------------------------
        ; Step 5: Plus / GX4000 branch
        ; -------------------------------------------------------
    .plus_branch:
        ; On 464+/6128+, logical page 7 (AMSDOS) explicitly activates physical page 3.
        ; On GX4000, this redirection hardware is absent, so logical 7 maps to physical 1.
        ; By reading the physical page 3 (#DF83) and comparing it against logical page 7 (#DF07):
        ; - If they MATCH: We are on a 464+/6128+ (logical 7 correctly redirected to phys 3)
        ; - If they DIFFER: We are on a GX4000 (logical 7 failed to redirect to phys 3)
        ; Enable Upper ROM in memory (Mode 0, Lower ROM enabled, Upper ROM enabled)
        ; We use defined constants to avoid hex math errors (GA_UROM_ENABLE = bit 3 zero)
        ld bc, GA_PORT_CMD + GA_CMD_MODE_ROM + GA_LROM_ENABLE + GA_UROM_ENABLE + GA_MODE_0
        out (c), c
        ; Select logical page 7 (#DF07)
        ld bc, ROM_PORT_UPPER + 7
        out (c), c
        ld hl, (#c000)      ; Read 2 bytes
        ; Select physical page 3 (#DF83 = 10000011 binary = bit 7 set for physical + page 3)
        ld bc, ROM_PORT_UPPER + #83
        out (c), c
        ld de, (#c000)      ; Read 2 bytes
        ; Return to logical page 0
        ld bc, ROM_PORT_UPPER + 0
        out (c), c
        ; Disable Upper ROM (Mode 0, Lower ROM enabled, Upper ROM disabled)
        ld bc, GA_PORT_CMD + GA_CMD_MODE_ROM + GA_LROM_ENABLE + GA_UROM_DISABLE + GA_MODE_0
        out (c), c
        ; Compare HL and DE
        or a
        sbc hl, de
        jr nz, .is_gx4000

        ; -------------------------------------------------------
        ; Step 6: 464+ vs 6128+
        ; Detection by extra RAM presence. The 6128+ has an extra 64K
        ; (GA_RAM_PAGE_0 / GA_RAM_CFG_4 maps it into slot 1 at #4000).
        ; The 464+ has no extra RAM -- the banking command is ignored.
        ; Write #AA to the extra bank, #55 to the normal bank, then read
        ; back through the extra bank: #AA = 6128+, anything else = 464+.
        ; -------------------------------------------------------
        ld a, (#4000)
        ld d, a                     ; D = save normal bank 1 byte
        ld bc, GA_PORT_CMD + GA_CMD_RAM_BANK + GA_RAM_PAGE_0 + GA_RAM_CFG_4
        out (c), c                  ; map extra bank into slot 1
        ld a, (#4000)
        ld e, a                     ; E = save extra bank byte
        ld a, #AA
        ld (#4000), a               ; write marker to extra bank
        ld bc, GA_PORT_CMD + GA_CMD_RAM_BANK + GA_RAM_PAGE_0 + GA_RAM_CFG_0
        out (c), c                  ; restore normal bank mapping
        ld a, #55
        ld (#4000), a               ; write different marker to normal bank
        ld bc, GA_PORT_CMD + GA_CMD_RAM_BANK + GA_RAM_PAGE_0 + GA_RAM_CFG_4
        out (c), c                  ; switch to extra bank again
        ld a, (#4000)               ; read back: #AA = extra RAM distinct
        push af                     ; save result
        ld a, e
        ld (#4000), a               ; restore extra bank original byte
        ld bc, GA_PORT_CMD + GA_CMD_RAM_BANK + GA_RAM_PAGE_0 + GA_RAM_CFG_0
        out (c), c                  ; restore normal bank mapping
        ld a, d
        ld (#4000), a               ; restore normal bank original byte
        pop af
        cp #AA
        jr z, .is_6128p
    .is_464p:
        ; No extra RAM -> 464 Plus
        ld a, CPC_MODEL_464P
        ret

    .is_6128p:
        ld a, CPC_MODEL_6128P
        ret

    .is_gx4000:
        ld a, CPC_MODEL_GX4000
        ret

endif