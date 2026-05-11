;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC floppy drive detection routines.
;
; Purpose: detect available disk drives and related hardware capabilities.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------
ifndef  __cpc_detect_drive_asm__
__cpc_detect_drive_asm__ equ 1
    ; CPC_DetectDrive -- detect the drive letter used to load the running program.
    ;
    ; Determines which disc drive (A or B) AMSDOS used when loading the current
    ; program, and whether a disc interface is present at all.
    ;
    ; On the Amstrad CPC, AMSDOS stores the last-used drive index in its workspace
    ; in upper RAM. When a the user switches drive, AMSDOS sets this byte to
    ; the drive index there
    ;
    ; AMSDOS workspace layout (relevant fields):
    ;   #A700 -- current drive index: 0 = drive A, 1 = drive B
    ;   Note: It is unreliable in some cases since ROM addition could move the
    ;   address downwards in the memory.
    ;
    ; Disc interface detection:
    ;   The NEC uPD765 FDC (floppy disc controller) present in 664 and 6128 disc
    ;   units exposes its main status register at port #FB7E (read-only). On a
    ;   machine with a disc interface the FDC asserts bit 7 (RQM = Request for
    ;   Master, controller ready) when idle. On a 464 without disc, or with the
    ;   disc interface absent, the port floats to #FF on the data bus. While #FF
    ;   would also mean bit 7 set, the full byte #FF (all bits set) is never a
    ;   valid FDC idle status -- a real FDC in idle state returns a value with
    ;   bits 6-4 clear and bits 3-0 indicating drive busy states (all zero when
    ;   all drives idle), giving a typical idle value of #80. Values of #C0, #A0,
    ;   or similar are also possible. The value #FF indicates the port is absent.
    ;
    ;
    ; Return values:
    ;   CPC_DRIVE_A      = 0   loaded from drive A (or drive A is the default)
    ;   CPC_DRIVE_B      = 1   loaded from drive B
    ;   CPC_DRIVE_NONE   = #FF no disc interface detected (tape or 464 without disc)
    ;
    ; Entry:
    ;   none
    ; Exit:
    ;   A = CPC_DRIVE_A, CPC_DRIVE_B, or CPC_DRIVE_NONE
    ; Destroys: AF, BC
    ; Preserves: DE, HL, IX, IY
    ;
    ; Notes: This function must be called before destroying Amsdos memory
    ;
    CPC_DRIVE_A		equ 0		; loaded from / current drive is A
    CPC_DRIVE_B		equ 1		; loaded from / current drive is B
    CPC_DRIVE_NONE	equ #ff		; no disc interface present

    ; AMSDOS workspace address of the current drive index byte.
    AMSDOS_CURRENT_DRIVE	equ #a700

    ; FDC main status register port (NEC uPD765, read-only).
    ; Returns #FF when the disc interface is absent (floating bus).
    FDC_STATUS_PORT		equ #fb7e

    ; CPC_DetectDrive -- detect whether a disc interface is present and which drive was used to boot.
    ; Entry: none
    ; Exit:  A = CPC_DRIVE_A (0), CPC_DRIVE_B (1), or CPC_DRIVE_NONE (#FF)
    ; Destroys: AF, BC  Preserves: DE, HL, IX, IY
    CPC_DetectDrive:
        ; Read the FDC main status register.
        ; On a machine with a disc interface the FDC is always present and returns
        ; a valid status byte (never all bits set). On a 464 / tape-only machine
        ; the port is absent and the data bus floats to #FF.
        ld bc,FDC_STATUS_PORT
        in a,(c)
        ; #FF = port absent, no disc interface fitted.
        cp #ff
        jr z,.no_drive
        ; Disc interface is present -- read the AMSDOS current drive byte.
        ; AMSDOS sets this to 0 (drive A) or 1 (drive B) before launching a program.
        ld a,(AMSDOS_CURRENT_DRIVE)
        ; Mask to bit 0 only: drive index is always 0 or 1; upper bits are undefined.
        and #01
        ret
    .no_drive:
        ld a,CPC_DRIVE_NONE
        ret
endif
