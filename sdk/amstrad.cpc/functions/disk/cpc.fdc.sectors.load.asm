; ---------------------------------------------------------------------------
;
; Retrodev Examples
;
; minislug loader -- FDC disc-read subroutines.
;
; (c) TLOTB 2026
;
; ---------------------------------------------------------------------------

ifndef __cpc_fdc_sectors_load__
__cpc_fdc_sectors_load__ equ 1

    ;
    ; Include FDC definitions
    ;
    include "macros.fdc.asm"
    ;
    ; Define this symbol in your source if you want to activate debugs
    ; It automatically forces the include for the GateArray Macros
    ; FDCDebugBorderColor equ 1
    ;
    ifdef FDCDebugBorderColor
        include "macros.ga.asm"
    endif
    ;
    ; States and variables
    ;
    FDCDrive:       db 0
    FDCResultST0:   db 0
    FDCResultST1:   db 0
    FDCResultST2:   db 0
    FDCBytesLimit:  dw 0        ; bytes FDCOpExecutionPhase will write this sector

    ;
    ; FDCInitLoader -- enable motor and recalibrate to track 0.
    ;
    ; Must be called before the first LoadBlock and after any code that may
    ; have left the FDC in an unknown state (e.g. after executing loaded code).
    ;
    ; On entry:
    ;   A = drive number (0 = drive A, 1 = drive B)
    ;
    ; Destroys: A, A', B, C, F, F', HL
    ; Preserves: DE, IX, IY
    ; ---------------------------------------------------------------------------
    FDCInitLoader:
        ; FDC poll loops store FDC_PORT_STATUS in BC -- an interrupt clobbering BC corrupts the read port.
        di
        ld (FDCDrive),a
        ; If we want to support drives with two heads
        ; We may want to add here the drive + head mixes
        ;
        call FDCOpMotorOn
        ;
        ; Debug color. If we stuck on recalibrate then border is RED.
        ;
        ifdef FDCDebugBorderColor
            push af : push bc : GA_SetBorder GA_COLOR_RED : pop bc : pop af
        endif
        call FDCOpRecalibrate
        ret

    ;
    ; EndLoader -- stop the motor and re-enable interrupts.
    ;
    ; Must be called after the last LoadBlock.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ; ---------------------------------------------------------------------------
    FDCEndLoader:
        call FDCOpMotorOff
        ei
        ret

    ;
    ; LoadBlock -- read one or more consecutive sectors from disc into memory.
    ;
    ; Must be called after FDCInitLoader. Sectors are read one at a time via
    ; FDCOpReadSector. When the sector number advances past the last sector
    ; (#C1+9-1 = #C9), it wraps to #C1 and the track is incremented.
    ;
    ; Interrupts are disabled for the duration of the sector loop and re-enabled
    ; on exit. Motor and recalibrate are handled by FDCInitLoader / FDCEndLoader.
    ;
    ; On entry:
    ;   HL = destination buffer address
    ;   D  = starting track  (0..39)
    ;   E  = starting sector ID (#C1..#C9, AMSDOS convention)
    ;   BC = number of bytes to read
    ;   (FDCDrive must be set via a prior FDCInitLoader call)
    ;
    ; On exit (success):
    ;   carry = 1
    ;   HL = first address past the last byte written
    ;   D  = track of the sector after the last one read
    ;   E  = sector ID of the sector after the last one read
    ;   BC = 0
    ;
    ; On exit (error):
    ;   carry = 0
    ;   A = 1: no disc / drive not ready
    ;   A = 2: read error (CRC, overrun, sector not found, wrong cylinder)
    ;
    ; Destroys: A, A', B, C, D, E, H, L, F, F'
    ; ---------------------------------------------------------------------------
    FDCLoadBlock:
            ; Interrupts off for the entire load: FDC_ExecPhase polls the FDC in a
            ; tight loop and will overrun if an interrupt fires between bytes.
            di
        .sectorLoop:
            ; Tell FDCOpExecutionPhase how many bytes to write this sector.
            ; For a full sector (BC >= 512) write all 512 bytes.
            ; For the last partial sector (BC < 512) write only BC bytes and let
            ; FDCOpExecutionPhase drain the rest: the FDC always outputs 512 bytes
            ; and we must read them all or get an overrun (ST1 OR flag).
            ld a,b
            cp 2
            jr nc,.full_sector
            ld a,c
            ld (FDCBytesLimit),a        ; low byte of remaining count
            ld a,b
            ld (FDCBytesLimit+1),a      ; high byte of remaining count
            jr .do_read
        .full_sector:
            xor a
            ld (FDCBytesLimit),a        ; low byte = 0
            ld a,2
            ld (FDCBytesLimit+1),a      ; high byte = 2  →  512
        .do_read:
            ; BC (remaining byte count) is saved around FDCOpReadSector because
            ; the FDC routines destroy BC. HL advances by bytes written inside the call.
            push bc
            call FDCOpReadSector
            pop bc
            ; Check for disc/read errors (Arkos READSECT pattern).
            ; ST0 bit 7 (IC1): IC=1x means drive not ready or invalid command.
            ; ST0 bit 3 (NR):  drive not ready (no disc, motor off).
            ; ST0 bit 4 (EC):  equipment check (head failed to position).
            ; ST1 mask excludes bit 7 (EN=end-of-cylinder) which is normal on CPC
            ;   because TC is unconnected and the FDC always signals abnormal end.
            ld a,(FDCResultST0)
            bit 7,a
            jr nz,.ls_disc_error
            bit 3,a
            jr nz,.ls_disc_error
            bit 4,a
            jr nz,.ls_read_error
            ld a,(FDCResultST1)
            and %00110111
            jr nz,.ls_read_error
            ld a,(FDCResultST2)
            and %00110000
            jr nz,.ls_read_error
            ;
            ; Debug border color. After a successful read we set the border
            ; color to WHITE to track if we get stuck in the loadblock
            ;
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_WHITE : pop bc : pop af
            endif
            ; Subtract 512 (=#0200) from remaining byte count.
            ; Low byte of 512 is 0 so only B (the high byte) changes; C is untouched.
            ; sub sets carry if B < 2: the remaining count was < 512 -- last sector done.
            ld a,b
            sub 2
            jr c,.ls_done
            ; B was >= 2: store updated high byte and check whether new BC == 0.
            ; jr nz: B still non-zero means at least 256 bytes remain -- loop.
            ; Fall through (B == 0): check C; if C == 0 too then BC hit zero -- done.
            ld b,a
            jr nz,.ls_advance
            ld a,c
            or a
            jr z,.ls_done
        .ls_advance:
            ; Advance sector ID; when it exceeds the last sector on the track
            ; wrap back to the first sector ID and step to the next track.
            inc e
            ld a,e
            cp FDC_DISC_SECTOR_BASE+FDC_DISC_SECTORS
            jr c,.ls_no_wrap
            ld e,FDC_DISC_SECTOR_BASE
            inc d
        .ls_no_wrap:
            jp .sectorLoop
        .ls_done:
            ;
            ; Debug color. Once everything is loaded border set to BLACK
            ;
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_BLACK : pop bc : pop af
            endif
            ei
            xor a
            scf
            ret
        .ls_disc_error:
            ei
            ld a,1
            or a
            ret
        .ls_read_error:
            ei
            ld a,2
            or a
            ret

    ;
    ; FDCOpReadSector -- seek to track D and read sector E into (HL).
    ;
    ; Executes one complete read cycle: SEEK -> READ DATA command -> result phase.
    ; Interrupts must be disabled by the caller (FDCOpExecutionPhase requires it).
    ;
    ; On entry:
    ;   D  = track (0..39)
    ;   E  = sector ID (#C1..#C9, AMSDOS convention)
    ;   HL = destination buffer address
    ;   (FDCDrive must be set via a prior FDCInitLoader call)
    ;   Interrupts must be disabled
    ;
    ; On exit:
    ;   HL = first address past the last byte written
    ;   DE = unchanged
    ;   FDCResultST0 = ST0 from result phase
    ;
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, IX, IY
    ; ---------------------------------------------------------------------------
    FDCOpReadSector:
            ;
            ; Debug color. Set to yellow to control if we stuck on seek
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_YELLOW : pop bc : pop af
            endif
            call FDCOpSeek
            ;
            ; Issue the CMD read. Border color set to BRIGHT BLUE
            ;
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_BRIGHT_BLUE : pop bc : pop af
            endif
            ld a,FDC_CMD_READ
            call FDCOpWaitRQMWrite
            ld a,(FDCDrive)
            call FDCOpWaitRQMWrite
            ld a,d
            call FDCOpWaitRQMWrite
            xor a
            call FDCOpWaitRQMWrite
            ld a,e
            call FDCOpWaitRQMWrite
            ld a,FDC_DISC_SECTOR_N
            call FDCOpWaitRQMWrite
            ld a,e
            call FDCOpWaitRQMWrite
            ld a,FDC_DISC_GPL_RW
            call FDCOpWaitRQMWrite
            ld a,FDC_DISC_DTL
            call FDCOpWaitRQMWrite
            ;
            ; Execution Phase. We will fetch the bytes from the FDC
            ; Border color set to LIME
            ;
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_LIME : pop bc : pop af
            endif
            ; DE is free here: track/sector not needed until after FDCOpReadSector returns.
            ; Load byte limit so FDCOpExecutionPhase knows when to stop writing.
            push de
            ld de,(FDCBytesLimit)
            call FDCOpExecutionPhase
            pop de
            ;
            ; Result phase. Once all the bytes are taken, we set the border
            ; color to MAGENTA 
            ;
            ifdef FDCDebugBorderColor
                push af : push bc : GA_SetBorder GA_COLOR_MAGENTA : pop bc : pop af
            endif
            call FDCOpResultPhase
            ret


    ;
    ; FDCOpExecutionPhase -- transfer execution-phase bytes from FDC into (HL).
    ;
    ; Reads all bytes the FDC outputs during the READ DATA execution phase.
    ; The first DE bytes are written to (HL++); bytes past the limit are read
    ; and silently discarded. The FDC always outputs 512 bytes regardless of
    ; how many the caller wants; all must be consumed to avoid an overrun (ST1 OR).
    ;
    ; Two separate loops:
    ;   .write -- store bytes into (HL), count down DE; falls into .drain when DE=0
    ;   .drain -- read and discard remaining bytes until NDM=0
    ;
    ; Write loop: ~90T per byte. Drain loop: ~63T per byte.
    ; Eliminates ex af,af' from the original design (saved ~21T per byte) to stay
    ; comfortably inside the 128T window at 250 kbps MFM with CPC gate array waits.
    ; JP (10T) for all taken branches; JR Z (7T not-taken) for the NDM check.
    ;
    ; On entry:
    ;   HL = destination address
    ;   DE = bytes to write (set by FDCOpReadSector from FDCBytesLimit)
    ;   Interrupts must be disabled
    ;
    ; On exit:
    ;   HL = first address past the last byte written
    ;   BC = FDC_PORT_STATUS
    ;
    ; Destroys: A, B, C, D, E, F
    ; Preserves: IX, IY
    ; ---------------------------------------------------------------------------
    FDCOpExecutionPhase:
            ld bc,FDC_PORT_STATUS
            ld a,d
            or e
            jp z,.drain
        .write:
            in a,(c)
            jp p,.write             ; RQM=0: FDC not ready yet
            and FDC_MSR_NDM
            jr z,.done              ; NDM=0: execution phase over
            inc c
            in a,(c)                ; A = byte from FDC (must always read)
            dec c
            ld (hl),a
            inc hl
            dec de
            ld a,d
            or e
            jp nz,.write
        .drain:
            in a,(c)
            jp p,.drain             ; RQM=0: FDC not ready
            and FDC_MSR_NDM
            jr z,.done              ; NDM=0: done
            inc c
            in a,(c)                ; read and discard
            dec c
            jp .drain
        .done:
            ret

    ; FDC_MotorOn -- switch the motor on for the selected drive and wait for spin-up.
    ;
    ; There are no control for motors on independent drives
    ; Only motor on (all drives) / motor off (all drives)
    ;
    ; After writing the motor latch
    ; the macro busy-waits for FDC_MOTOR_SPINUP_LOOPS iterations (~1000 ms).
    ; This is the minimum safe delay before issuing any read or write command.
    ;
    ; Destroys: A, B, C, HL (used as outer loop counter)
    ; Preserves: DE, IX, IY
    ;
    ; Speed: ~1000 ms at 4 MHz (deterministic delay, not bus-dependent).
    ;
    ; ---------------------------------------------------------------------

    FDCOpMotorOn:
            ld bc,FDC_PORT_MOTOR
            ld a, 1
            out (c),a
            ; This should be defined in an equ we can tune easily
            ;
            ld hl,FDC_MOTOR_SPINUP_LOOPS
        .outer:
            ld b,0
        .inner:
            djnz .inner
            dec hl
            ld a,h
            or l
            jr nz,.outer
            ret

    ;
    ; FDC_MotorOff -- switch all drive motors off.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ; -----------------------------------------------------------
    FDCOpMotorOff:
            ld bc,FDC_PORT_MOTOR
            xor a
            out (c),a
            ret

    ;
    ; FDCRecalibrate -- seek drive to track 0, drive number from FDCDrive.
    ;
    ; Issues RECALIBRATE twice and waits for SE=1 each time.
    ; Two passes guarantee track 0 on 80-track drives where one pass
    ; may only step 77 tracks (SE=1 with head short of track 0).
    ;
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ; -----------------------------------------------------------
    FDCOpRecalibrate:
            call FDCOpRecalibrateOnce
            call FDCOpRecalibrateOnce
            ret

    ;
    ; FDCOpRecalibrateOnce -- issue one RECALIBRATE and spin until SE=1.
    ;
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ; -----------------------------------------------------------
    FDCOpRecalibrateOnce:
            ld a,FDC_CMD_RECALIBRATE
            call FDCOpWaitRQMWrite
            ld a,(FDCDrive)
            call FDCOpWaitRQMWrite
            call FDCOpWaitEnd
            ret


    ;
    ; FDCOpResultPhase -- read the 7 result bytes from a READ DATA command.
    ;
    ; READ DATA always returns exactly 7 bytes: ST0, ST1, ST2, C, H, R, N.
    ; ST0/ST1/ST2 are stored in FDCResultST0/ST1/ST2 for the caller to inspect.
    ; C, H, R, N are read and discarded -- they echo the command parameters.
    ; Matches Arkos RSFIN structure: one FDCOpWaitRQMRead call per result byte.
    ;
    ; On exit:
    ;   FDCResultST0 = ST0
    ;   FDCResultST1 = ST1
    ;   FDCResultST2 = ST2
    ;
    ; Destroys: A, B, C, F
    ; Preserves: DE, HL, IX, IY
    ; ---------------------------------------------------------
    FDCOpResultPhase:
            call FDCOpWaitRQMRead
            ld (FDCResultST0),a
            call FDCOpWaitRQMRead
            ld (FDCResultST1),a
            call FDCOpWaitRQMRead
            ld (FDCResultST2),a
            call FDCOpWaitRQMRead    ; C (cylinder) -- discard
            call FDCOpWaitRQMRead    ; H (head)     -- discard
            call FDCOpWaitRQMRead    ; R (sector)   -- discard
            call FDCOpWaitRQMRead    ; N (size)     -- discard
            ret

    ;
    ; FDCOpWaitRQM -- spin until the FDC Main Status Register shows RQM=1.
    ;
    ; Polls FDC_PORT_STATUS until bit 7 (RQM) is set, indicating the FDC is
    ; ready for the next data register transfer. Must be called before every
    ; read or write of the data register during command and result phases.
    ;
    ; Does not check DIO -- the caller is responsible for transferring in the
    ; correct direction after this macro returns.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ;
    ; Speed: non-deterministic. Each poll = 12 T (in a,(c)) + 4 T (rla) + branch.
    ; --------------------------------------------------------------------------
    FDCOpWaitRQM:
            ld bc,FDC_PORT_STATUS
        .wait:
            in a,(c)
            rla
            jr nc,.wait
            ret

    ;
    ; FDCOpWaitRQMRead -- spin until RQM=1, then read one byte from the data register.
    ;
    ; Only checks RQM (bit 7 via sign flag, jp p) -- does not check DIO. Matches
    ; Arkos GETFDC exactly. Checking DIO caused a hang on WinAPE (and likely some
    ; real hardware) where the MSR does not always reflect DIO=1 in the result phase.
    ;
    ; On exit: A = byte read, BC = FDC_PORT_STATUS
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ; --------------------------------------------------------------------------
    FDCOpWaitRQMRead:
            ld bc,FDC_PORT_STATUS
        .wait:
            in a,(c)
            jp p,.wait
            inc c
            in a,(c)				; read data byte -- A = byte on return
            dec c
            ret

    ;
    ; FDCOpWaitRQMWrite -- spin until RQM=1, then write one byte to the data register.
    ;
    ; Does NOT check DIO -- checking DIO causes an infinite loop when the FDC has
    ; stale result bytes (DIO=1). Just waiting for RQM=1 matches all three reference
    ; implementations (fdcload PUTFDC, DDFDC ddfdccom, ReadSectors PUTFDC).
    ; The byte to write must be in A on entry; it is preserved across the RQM poll
    ; via ex af,af'. The write goes to the data port via the inc c / dec c trick
    ; (C: &7E -> &7F -> &7E) without changing B, identical to the references.
    ;
    ; On entry: A = byte to write
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ;
    FDCOpWaitRQMWrite:
            ex af,af'
            ld bc,FDC_PORT_STATUS
        .wait:
            in a,(c)
            jp p,.wait
            ex af,af'
            inc c
            out (c),a
            dec c
            ret

    ;
    ; FDCOpRead -- read one byte from the data register without waiting.
    ;
    ; Caller must ensure RQM=1 and DIO=1 before calling.
    ; On exit A = byte read, BC = FDC_PORT_STATUS.
    ;
    ; On exit:  A = byte read, BC = FDC_PORT_STATUS
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ; --------------------------------------------------------------------------
    FDCOpRead:
            ld bc,FDC_PORT_DATA
            in a,(c)
            dec c
            ret

    ;
    ; FDCOpWrite -- write one byte to the data register without waiting.
    ;
    ; Caller must ensure RQM=1 and DIO=0 before calling.
    ; On exit BC = FDC_PORT_STATUS.
    ;
    ; On entry: A = byte to write
    ; On exit:  BC = FDC_PORT_STATUS
    ; Destroys: B, C
    ; Preserves: A, DE, HL, IX, IY
    ; --------------------------------------------------------------------------
    FDCOpWrite:
            ld bc,FDC_PORT_DATA
            out (c),a
            dec c
            ret

    ;
    ; FDCSenseInt -- issue SENSE INTERRUPT STATUS and read result into FDCResultST0.
    ;
    ; Sends FDC_CMD_SENSE_INT, then calls FDCOpResultPhase to read ST0 (and drain PCN).
    ; Callers use FDCResultST0 to inspect IC, SE, EC after return.
    ;
    ; On exit:  FDCResultST0 = ST0, BC = FDC_PORT_STATUS
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ; --------------------------------------------------------------------------
    FDCSenseInt:
            ld a,FDC_CMD_SENSE_INT
            call FDCOpWaitRQMWrite
            call FDCOpWaitRQMRead	; read ST0 -- SENSE_INT always returns exactly ST0 + PCN
            ld (FDCResultST0),a
            call FDCOpWaitRQMRead	; read PCN (discard)
            xor a
            ld (FDCResultST1),a		; SENSE_INT does not return ST1/ST2; zero them
            ld (FDCResultST2),a
            ret

    ;
    ; FDCOpSeek -- seek to track D, drive from FDCDrive.
    ;
    ; On entry: D = target track (0..39)
    ;
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ; -----------------------------------------------------------
    FDCOpSeek:
            ld a,FDC_CMD_SEEK
            call FDCOpWaitRQMWrite
            ld a,(FDCDrive)
            call FDCOpWaitRQMWrite
            ld a,d
            call FDCOpWaitRQMWrite
            call FDCOpWaitEnd
            ret

    ;
    ; FDCOpWaitEnd -- spin SENSE_INT until SE=1 (current command complete).
    ;
    ; Issues SENSE_INT via FDCSenseInt in a loop until SE=1.
    ; Call after all command bytes for SEEK or RECALIBRATE are written.
    ;
    ; On exit:  FDCResultST0 = ST0 with SE=1
    ; Destroys: A, A', B, C, F, F'
    ; Preserves: DE, HL, IX, IY
    ; --------------------------------------------------------------------------
    FDCOpWaitEnd:
            call FDCSenseInt
            ld a,(FDCResultST0)
            bit FDC_ST0_SE_BIT,a
            jr z,FDCOpWaitEnd
            ret

endif