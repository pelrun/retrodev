;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC PSG macros.
;
; Purpose: define AY-3-8912 register constants and PSG helper macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __macros_psg_asm__
__macros_psg_asm__ equ 1


    ; AY-3-8912 PSG definitions and helper macros for the Amstrad CPC.
    ;
    ; The AY-3-8912 (General Instrument) is the Programmable Sound Generator
    ; fitted in every CPC model. It provides three square-wave tone channels
    ; (A, B, C), one noise generator, one envelope generator, and one 8-bit
    ; I/O port (Port A, R14) which the CPC uses for keyboard row selection.
    ;
    ; ============================================================
    ; CPC ACCESS MODEL -- the AY is NOT directly port-mapped
    ; ============================================================
    ;
    ; Unlike most Z80 peripherals the AY is not wired to its own I/O address.
    ; It sits behind the 8255 PPI and is reached entirely through PPI ports:
    ;
    ;   PPI Port A  (#F4xx, bidirectional) -- AY data bus.
    ;     As output (PPI control word #82): carries the register index during
    ;     address latch, or the data value during a write.
    ;     As input  (PPI control word #92): reads back the value of the
    ;     selected AY register (only registers that are readable return
    ;     meaningful data -- see register table below).
    ;
    ;   PPI Port C  (#F6xx, output) -- AY bus control lines.
    ;     Bits 7-6 = BDIR / BC2:  the two lines that select the AY bus mode.
    ;     Bits 3-0 = keyboard row select (unrelated to PSG; set to 0 for audio).
    ;
    ;   PPI control register (#F7xx, write-only) -- configures port directions.
    ;     #82 : Port A = output, Port B = input, Port C = output.  (for PSG writes)
    ;     #92 : Port A = input,  Port B = input, Port C = output.  (for PSG reads)
    ;
    ; ============================================================
    ; AY BUS MODES  (PPI Port C bits 7-6)
    ; ============================================================
    ;
    ;   BDIR BC2  Mode
    ;    0    0   Inactive     -- AY ignores the bus
    ;    0    1   Read         -- AY drives its selected register onto Port A
    ;    1    0   Write        -- AY latches Port A as the data for selected register
    ;    1    1   Address latch-- AY latches Port A as the active register index
    ;
    ; Every access follows a strict sequence:
    ;   1. Set PPI Port A = output (#82), place register index on Port A.
    ;   2. Strobe BDIR=1,BC2=1 (#C0) on Port C -- AY latches the register.
    ;   3. Strobe BDIR=0,BC2=0 (#00) on Port C -- return to inactive.
    ;   For a WRITE (steps 4-6):
    ;   4. Place data value on Port A (Port A still output).
    ;   5. Strobe BDIR=1,BC2=0 (#80) on Port C -- AY latches the data.
    ;   6. Strobe BDIR=0,BC2=0 (#00) on Port C -- return to inactive.
    ;   For a READ (steps 4-6):
    ;   4. Reconfigure Port A = input (PPI control word #92).
    ;   5. Strobe BDIR=0,BC2=1 (#40) on Port C -- AY drives register onto Port A.
    ;   6. Read Port A -- data is in A.
    ;   7. Strobe BDIR=0,BC2=0 (#00) on Port C -- return to inactive.
    ;   8. Restore Port A = output (#82) if further writes are needed.

    ; ============================================================
    ; PPI PORT ADDRESSES
    ; ============================================================
    ;
    ; Both 16-bit and 8-bit (_B) forms are provided.
    ; Use the _B form to load B for out (c),c / in a,(c) instructions.
    ;
    PSG_PPI_CTRL		equ #f700	; PPI control register (write-only)
    PSG_PPI_CTRL_B		equ #f7		; high byte of PSG_PPI_CTRL
    PSG_PPI_DATA		equ #f400	; PPI Port A -- AY data bus (bidirectional)
    PSG_PPI_DATA_B		equ #f4		; high byte of PSG_PPI_DATA
    PSG_PPI_PORTC		equ #f600	; PPI Port C -- AY bus control (output)
    PSG_PPI_PORTC_B		equ #f6		; high byte of PSG_PPI_PORTC

    ; ============================================================
    ; PPI CONTROL WORDS
    ; ============================================================
    PSG_PPI_PORTA_OUT	equ #82		; Port A = output, Port B = input, Port C = output
    PSG_PPI_PORTA_IN	equ #92		; Port A = input,  Port B = input, Port C = output

    ; ============================================================
    ; AY BUS MODE BYTES  (written to PPI Port C, bits 7-6)
    ; ============================================================
    PSG_BUS_INACTIVE	equ #00		; BDIR=0, BC2=0 -- AY ignores bus
    PSG_BUS_READ		equ #40		; BDIR=0, BC2=1 -- AY drives selected register onto data bus
    PSG_BUS_WRITE		equ #80		; BDIR=1, BC2=0 -- AY latches data from data bus
    PSG_BUS_ADDR		equ #c0		; BDIR=1, BC2=1 -- AY latches register index from data bus

    ; ============================================================
    ; AY-3-8912 REGISTER MAP
    ; ============================================================
    ;
    ; Register  Name                    Bits  R/W  Notes
    ; --------  ----------------------  ----  ---  ---------------------------
    ;  R0       Channel A Fine Tune      8    R/W  Tone period = (R1:R0) * clock/16
    ;  R1       Channel A Coarse Tune    4    R/W  Bits 3-0 only; bits 7-4 ignored
    ;  R2       Channel B Fine Tune      8    R/W
    ;  R3       Channel B Coarse Tune    4    R/W
    ;  R4       Channel C Fine Tune      8    R/W
    ;  R5       Channel C Coarse Tune    4    R/W
    ;  R6       Noise Period             5    R/W  Bits 4-0; noise freq = clock/(16*R6)
    ;  R7       Mixer / I/O control      8    R/W  See PSG_R7_* bits below
    ;  R8       Channel A Amplitude      5    R/W  Bit 4=envelope mode; bits 3-0=level
    ;  R9       Channel B Amplitude      5    R/W
    ;  R10      Channel C Amplitude      5    R/W
    ;  R11      Envelope Period Fine     8    R/W  Envelope period = (R12:R11) * clock/256
    ;  R12      Envelope Period Coarse   8    R/W
    ;  R13      Envelope Shape           4    W    See PSG_ENV_* below; write resets envelope
    ;  R14      I/O Port A               8    R/W  CPC: output = keyboard row select (0-9)
    ;                                              CPC: input  = keyboard column bits (read-only use)
    ;
    ; Tone period formula (channel A as example):
    ;   period_counts = (R1[3:0] << 8) | R0[7:0]
    ;   frequency_Hz  = AY_CLOCK / (16 * period_counts)
    ;   AY_CLOCK on CPC = 1 MHz (CPU clock / 4)
    ;
    PSG_R0_TONE_A_LO	equ 0		; channel A tone period, low byte
    PSG_R1_TONE_A_HI	equ 1		; channel A tone period, high nibble (bits 3-0)
    PSG_R2_TONE_B_LO	equ 2		; channel B tone period, low byte
    PSG_R3_TONE_B_HI	equ 3		; channel B tone period, high nibble
    PSG_R4_TONE_C_LO	equ 4		; channel C tone period, low byte
    PSG_R5_TONE_C_HI	equ 5		; channel C tone period, high nibble
    PSG_R6_NOISE		equ 6		; noise generator period (bits 4-0)
    PSG_R7_MIXER		equ 7		; mixer control and I/O port direction
    PSG_R8_VOL_A		equ 8		; channel A amplitude (bit 4=env, bits 3-0=level)
    PSG_R9_VOL_B		equ 9		; channel B amplitude
    PSG_R10_VOL_C		equ 10		; channel C amplitude
    PSG_R11_ENV_LO		equ 11		; envelope period, low byte
    PSG_R12_ENV_HI		equ 12		; envelope period, high byte
    PSG_R13_ENV_SHAPE	equ 13		; envelope shape (write resets the envelope counter)
    PSG_R14_PORT_A		equ 14		; I/O Port A -- keyboard row select on CPC

    ; ============================================================
    ; R7 MIXER REGISTER BITS
    ; ============================================================
    ;
    ; A 0 bit enables the corresponding source; a 1 bit disables/mutes it.
    ; Bit 6 sets Port A as output (1) or input (0) -- CPC always sets this to 1.
    ;
    ;   Bit  Name      Effect when 0          Effect when 1
    ;    0   TONE_A    channel A tone on       channel A tone off
    ;    1   TONE_B    channel B tone on       channel B tone off
    ;    2   TONE_C    channel C tone on       channel C tone off
    ;    3   NOISE_A   noise mixed into A      noise not mixed into A
    ;    4   NOISE_B   noise mixed into B      noise not mixed into B
    ;    5   NOISE_C   noise mixed into C      noise not mixed into C
    ;    6   IOA_OUT   Port A = input          Port A = output (always 1 on CPC)
    ;
    PSG_R7_TONE_A_OFF	equ #01		; bit 0: disable channel A tone
    PSG_R7_TONE_B_OFF	equ #02		; bit 1: disable channel B tone
    PSG_R7_TONE_C_OFF	equ #04		; bit 2: disable channel C tone
    PSG_R7_NOISE_A_OFF	equ #08		; bit 3: disable noise on channel A
    PSG_R7_NOISE_B_OFF	equ #10		; bit 4: disable noise on channel B
    PSG_R7_NOISE_C_OFF	equ #20		; bit 5: disable noise on channel C
    PSG_R7_IOA_OUT		equ #40		; bit 6: Port A = output (set on CPC)
    ; Preset mixer values (Port A always output = bit 6 set)
    PSG_MIXER_ALL_OFF	equ #ff		; all channels and noise disabled
    PSG_MIXER_ABC_TONE	equ #40|#38	; channels A+B+C tone only, no noise
    PSG_MIXER_A_TONE	equ #40|#3e	; channel A tone only
    PSG_MIXER_B_TONE	equ #40|#3d	; channel B tone only
    PSG_MIXER_C_TONE	equ #40|#3b	; channel C tone only
    PSG_MIXER_ABC_NOISE	equ #40|#07	; noise only on A+B+C, no tone
    PSG_MIXER_A_NOISE	equ #40|#36	; noise only on channel A
    PSG_MIXER_B_NOISE	equ #40|#35	; noise only on channel B
    PSG_MIXER_C_NOISE	equ #40|#33	; noise only on channel C

    ; ============================================================
    ; AMPLITUDE REGISTER BITS  (R8, R9, R10)
    ; ============================================================
    ;
    ; Bit 4 = 0: fixed level mode  -- bits 3-0 set the volume (0=silent, 15=max).
    ; Bit 4 = 1: envelope mode     -- bits 3-0 are ignored; envelope controls level.
    ;
    PSG_VOL_ENVELOPE	equ #10		; bit 4: use envelope generator for this channel
    PSG_VOL_MAX			equ #0f		; maximum fixed volume level
    PSG_VOL_SILENT		equ #00		; silent (fixed volume = 0)

    ; ============================================================
    ; ENVELOPE SHAPE REGISTER  (R13)
    ; ============================================================
    ;
    ; Writing R13 resets the envelope counter and starts a new envelope cycle.
    ; Bits 3-0 select the shape; bits 7-4 are ignored.
    ;
    ;   Bit 3 = CONT:  0 = one-shot (envelope stops at end),  1 = continuous
    ;   Bit 2 = ATT:   0 = decay (counts down),               1 = attack (counts up)
    ;   Bit 1 = ALT:   0 = no alternate,                      1 = alternate direction each cycle
    ;   Bit 0 = HOLD:  0 = no hold,                           1 = hold last value
    ;
    PSG_ENV_DECAY		equ #00		; single decay to 0, then silence
    PSG_ENV_ATTACK		equ #04		; single attack to max, then silence
    PSG_ENV_DECAY_LOOP	equ #08		; continuous sawtooth decay (repeat)
    PSG_ENV_DECAY_HOLD	equ #09		; decay to 0, hold at 0
    PSG_ENV_DECAY_ALT	equ #0A		; decay then attack, one-shot (triangle once)
    PSG_ENV_DECAY_HOLD2	equ #0B		; decay to max, hold at max
    PSG_ENV_ATTACK_LOOP	equ #0C		; continuous sawtooth attack (repeat)
    PSG_ENV_ATTACK_HOLD	equ #0D		; attack to max, hold at max
    PSG_ENV_ATTACK_ALT	equ #0E		; attack then decay, continuous (triangle loop)
    PSG_ENV_ATTACK_HOLD2 equ #0F	; attack to 0, hold at 0

    ; ============================================================
    ; AY CLOCK
    ; ============================================================
    ;
    ; On all standard CPC models the AY clock is derived from the CPU clock
    ; divided by 4, giving 1 MHz (1,000,000 Hz).
    ; Tone frequency  = AY_CLOCK / (16 * tone_period)
    ; Noise frequency = AY_CLOCK / (16 * noise_period)
    ; Envelope rate   = AY_CLOCK / (256 * envelope_period)
    ;
    PSG_CLOCK			equ 1000000	; AY clock frequency in Hz (1 MHz on CPC)

    ; ============================================================
    ; PSG_Select -- latch a register index as the active AY register.
    ;
    ; Places Reg on the AY address bus and strobes BDIR=1,BC2=1 then
    ; BDIR=0,BC2=0. After this macro the AY holds Reg as its active
    ; register until the next address latch or power cycle.
    ;
    ; Port A is left as output (PPI control word #82) on exit.
    ; Subsequent PSG_Write or PSG_Read calls do not need to re-latch
    ; the register if they immediately follow this macro.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_Select Reg
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_OUT
        out (c),c
        ld bc,PSG_PPI_DATA+{Reg}
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_ADDR
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
    endm

    ; ============================================================
    ; PSG_Write -- select an AY register and write a value to it.
    ;
    ; Performs a full address latch followed by a data write in one macro.
    ; Reg is the register index (0-14); Value is the byte to write.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ;
    ; Speed: 6 * (10 T ld bc,nn + 12 T out) = 132 T (constant, no branching).
    macro PSG_Write Reg, Value
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_OUT
        out (c),c
        ld bc,PSG_PPI_DATA+{Reg}
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_ADDR
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
        ld bc,PSG_PPI_DATA+{Value}
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_WRITE
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
    endm

    ; ============================================================
    ; PSG_WriteReg -- write a value to the already-selected AY register.
    ;
    ; Skips the address latch phase. Use after PSG_Select when writing to
    ; the same register multiple times in sequence.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ;
    ; Speed: 4 * (10 T + 12 T) = 88 T (constant, no branching).
    macro PSG_WriteReg Value
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_OUT
        out (c),c
        ld bc,PSG_PPI_DATA+{Value}
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_WRITE
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
    endm

    ; ============================================================
    ; PSG_Read -- select an AY register and read its current value into A.
    ;
    ; Performs a full address latch, switches Port A to input, drives BC2=1
    ; so the AY presents the register value on the data bus, reads Port A,
    ; then restores Port A to output and returns to inactive bus state.
    ;
    ; Result is in A on exit.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    ;
    ; Speed: 7 * (10 T + 12 T) + 12 T (in a,(c)) = 166 T (constant).
    macro PSG_Read Reg
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_OUT
        out (c),c
        ld bc,PSG_PPI_DATA+{Reg}
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_ADDR
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_IN
        out (c),c
        ld bc,PSG_PPI_PORTC+PSG_BUS_READ
        out (c),c
        ld b,PSG_PPI_DATA_B
        in a,(c)
        ld bc,PSG_PPI_PORTC+PSG_BUS_INACTIVE
        out (c),c
        ld bc,PSG_PPI_CTRL+PSG_PPI_PORTA_OUT
        out (c),c
    endm

    ; ============================================================
    ; PSG_SetTone -- set the tone period for one channel from a 12-bit period value.
    ;
    ; Ch selects the channel: 0=A, 1=B, 2=C.
    ; Period is the 12-bit tone period value (0-4095).
    ;   Low byte  -> register Ch*2     (fine tune, bits 7-0)
    ;   High nibble -> register Ch*2+1 (coarse tune, bits 3-0)
    ;
    ; Both registers are written. The frequency produced is:
    ;   f = PSG_CLOCK / (16 * Period)
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetTone Ch, Period
        PSG_Write {Ch}*2,   {Period} & #ff
        PSG_Write {Ch}*2+1, ({Period} >> 8) & #0f
    endm

    ; ============================================================
    ; PSG_SetVolume -- set the fixed amplitude for one channel.
    ;
    ; Ch selects the channel: 0=A, 1=B, 2=C.
    ; Level is 0-15 (0=silent, 15=maximum). Clears the envelope mode bit.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetVolume Ch, Level
        PSG_Write PSG_R8_VOL_A+{Ch}, {Level} & #0f
    endm

    ; ============================================================
    ; PSG_SetEnvVolume -- set a channel to envelope-controlled amplitude.
    ;
    ; Ch selects the channel: 0=A, 1=B, 2=C.
    ; Sets bit 4 of the amplitude register so the envelope generator
    ; controls the channel level instead of a fixed value.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetEnvVolume Ch
        PSG_Write PSG_R8_VOL_A+{Ch}, PSG_VOL_ENVELOPE
    endm

    ; ============================================================
    ; PSG_SetEnvelope -- program the envelope period and shape.
    ;
    ; Period is the 16-bit envelope period (0-65535).
    ;   Low byte  -> R11 (PSG_R11_ENV_LO)
    ;   High byte -> R12 (PSG_R12_ENV_HI)
    ; Shape is one of the PSG_ENV_* constants.
    ; Writing Shape resets the envelope counter and restarts the cycle.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetEnvelope Period, Shape
        PSG_Write PSG_R11_ENV_LO, {Period} & #ff
        PSG_Write PSG_R12_ENV_HI, ({Period} >> 8) & #ff
        PSG_Write PSG_R13_ENV_SHAPE, {Shape}
    endm

    ; ============================================================
    ; PSG_SetMixer -- write the mixer / I/O control register (R7).
    ;
    ; Value is the full R7 byte. Bit 6 (IOA_OUT) must be 1 on the CPC
    ; to keep Port A as output for the keyboard row-select lines.
    ; Use the PSG_MIXER_* presets or build the value from PSG_R7_* bits.
    ;
    ; Example -- tone on channels A and B only, no noise:
    ;   PSG_SetMixer PSG_R7_IOA_OUT | PSG_R7_TONE_C_OFF | PSG_R7_NOISE_A_OFF | PSG_R7_NOISE_B_OFF | PSG_R7_NOISE_C_OFF
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetMixer Value
        PSG_Write PSG_R7_MIXER, {Value}
    endm

    ; ============================================================
    ; PSG_Silence -- mute all three channels immediately.
    ;
    ; Sets all amplitude registers to 0 (fixed level, silent) and
    ; writes the all-off mixer value.
    ; Does not stop the tone or noise generators -- they continue running
    ; at whatever period is programmed; only the output is silenced.
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_Silence
        PSG_Write PSG_R7_MIXER, PSG_MIXER_ALL_OFF
        PSG_Write PSG_R8_VOL_A,  PSG_VOL_SILENT
        PSG_Write PSG_R9_VOL_B,  PSG_VOL_SILENT
        PSG_Write PSG_R10_VOL_C, PSG_VOL_SILENT
    endm

    ; ============================================================
    ; PSG_SetNoise -- set the noise generator period.
    ;
    ; Period is 0-31 (bits 4-0 of R6). Lower value = higher noise frequency.
    ; f_noise = PSG_CLOCK / (16 * Period)
    ;
    ; Destroys: A, B, C
    ; Preserves: DE, HL, IX, IY
    macro PSG_SetNoise Period
        PSG_Write PSG_R6_NOISE, {Period} & #1f
    endm
endif
