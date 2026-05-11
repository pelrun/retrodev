;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC RAM detection routines.
;
; Purpose: detect memory expansion and report available RAM configuration.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __cpc_detect_ram_asm__
__cpc_detect_ram_asm__ equ 1
    ;
    ; Require macros.ga.asm
    ;
    include "macros.ga.asm"
    ;
    ; CPC_DetectRAM
    ;
    ; Detects total distinct 64K RAM blocks available to the system, accounting
    ; for base memory and Amstrad CPC memory mirror mapping behaviour.
    ;
    ; Unexpanded systems mirror base memory into expansion memory slots,
    ; while expanded systems (e.g. 128K) mirror their expansion pages across
    ; undefined additional expansions (pages 1..7 mirror page 0). To accurately
    ; count independent physical banks without false positives from aliases,
    ; a Unique Identifier approach is used.
    ;
    ; 1. A unique ID is written to Base Memory (Config 0).
    ; 2. Sequential unique IDs are written to Extra Pages 0..7 (Config 1).
    ; 3. Because memory aliases blindly overwrite each other sequentially, reading
    ;    back these 9 test addresses will only match the "last" alias that targeted
    ;    that physical memory block.
    ; 4. Counting the exact matches gives the precise number of discrete 64K blocks!
    ; 5. Original data is non-destructively restored afterwards seamlessly.
    ;
    ; The total RAM size returned is:
    ;   64   -- 64K base RAM only (CPC 464, 664)
    ;   128  -- 128K: Base + 1 extra 64K page confirmed (standard 6128 / CPC+)
    ;   ... plus 64 for each distinct extra page up to a theoretical 576K.
    ;
    ; Important: this routine briefly maps extra RAM into slot 3 (#C000-#FFFF)
    ; displacing the upper ROM / normal bank 3 for a few instructions. Interrupts
    ; MUST be disabled before calling and re-enabled after if required.
    ;
    ; Entry:
    ;   none
    ; Exit:
    ;   HL = total RAM size in kilobytes (64, 128, 192, 256, 320, 384, 448, 512, 576)
    ; Destroys: AF, BC, D, E, HL, IXL
    ; Preserves: IXH, IY
    ; Requirements: interrupts must be disabled (DI) before calling.

    ; CPC_DetectRAM -- detect total RAM fitted (64K..576K) via unique ID alias resolution.
    ; Entry: none  (interrupts must be disabled)
    ; Exit:  HL = total RAM in kilobytes (64..576)
    ; Destroys: AF, BC, DE, HL, IXL  Preserves: IXH, IY
    CPC_DetectRAM:
        ; Phase 0: Save Base RAM byte and tag it
        ld b,#7f
        ld c,GA_CMD_RAM_BANK+GA_RAM_CFG_0
        out (c),c

        ld a,(#c000)
        ld ixl,a      ; Store original Base byte in IXL

        ld a,#50      ; Unique ID 0x50 for Base (ID 0)
        ld (#c000),a

        ; Phase 1: Save Extra RAM bytes (0..7) to stack and tag them sequentially
        ld e,GA_CMD_RAM_BANK+GA_RAM_CFG_1
        ld d,0
    .save_loop:
        ld a,d
        add a,a
        add a,a
        add a,a       ; A = D * 8 (align to GA_RAM_PAGE mapping bits 5-3)
        or e
        ld b,#7f
        ld c,a
        out (c),c     ; Switch extra RAM page into slot 3 (#C000)

        ld a,(#c000)
        push af       ; Save the original byte on the stack

        ld a,d
        add a,#51     ; Unique ID 0x51..0x58 for Pages 0..7
        ld (#c000),a  ; Write unique ID

        inc d
        ld a,d
        cp 8
        jr nz,.save_loop

        ; Phase 2: Read back and accumulate distinct matches
        ld hl,0       ; HL = counted kilobyte accumulator

        ; 2a: Check Base RAM
        ld b,#7f
        ld c,GA_CMD_RAM_BANK+GA_RAM_CFG_0
        out (c),c
        ld a,(#c000)
        cp #50        ; Does Base still hold ID 0x50?
        jr nz,.base_mismatch
        ld bc,64
        add hl,bc     ; Base RAM distinct match!
    .base_mismatch:

        ; 2b: Check Extra RAM Pages (0..7)
        ld d,0
        ld e,GA_CMD_RAM_BANK+GA_RAM_CFG_1
    .check_loop:
        ld a,d
        add a,a
        add a,a
        add a,a
        or e
        ld b,#7f
        ld c,a
        out (c),c     ; Switch extra RAM page

        ld a,d
        add a,#51
        ld c,a        ; C = Expected ID (save in C to avoid corrupting E)

        ld a,(#c000)
        cp c          ; Does it hold its explicitly expected ID?
        jr nz,.extra_mismatch

        ld bc,64
        add hl,bc     ; Extra RAM page distinct match! Add 64K
    .extra_mismatch:
        inc d
        ld a,d
        cp 8
        jr nz,.check_loop

        ; Phase 3: Restore original bytes safely backwards
        ld d,7
        ld e,GA_CMD_RAM_BANK+GA_RAM_CFG_1
    .restore_loop:
        ld a,d
        add a,a
        add a,a
        add a,a
        or e
        ld b,#7f
        ld c,a
        out (c),c

        pop af        ; Recover original byte from stack
        ld (#c000),a  ; Restore memory

        dec d
        jp p,.restore_loop ; Loop backwards until D goes below 0 (-1)

        ; Restore Base RAM byte and standard CFG 0
        ld b,#7f
        ld c,GA_CMD_RAM_BANK+GA_RAM_CFG_0
        out (c),c
        ld a,ixl
        ld (#c000),a

        ret
endif
