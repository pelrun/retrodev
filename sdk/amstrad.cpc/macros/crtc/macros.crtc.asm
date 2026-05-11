;-----------------------------------------------------------------------
;
; Retrodev SDK -- Amstrad CPC CRTC macro definitions.
;
; Purpose: provide reusable macros to configure HD6845/compatible CRTC
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __macros_crtc_asm__
__macros_crtc_asm__ equ 1
	; CRTC port addresses
	;
	CRTC_PORT_ADDR		equ #bc00	; address port -- write register index here to select it
	CRTC_PORT_DATA		equ #bd00	; data port    -- write value here after selecting a register

	; CRTC register indices (HD6845 / compatible)
	;
	CRTC_R0_HTOT		equ 0		; Horizontal Total
	CRTC_R1_HDIS		equ 1		; Horizontal Displayed
	CRTC_R2_HSYNC		equ 2		; Horizontal Sync Position
	CRTC_R3_SYNC_W		equ 3		; Sync Width (horizontal + vertical)
	CRTC_R4_VTOT		equ 4		; Vertical Total
	CRTC_R5_VTADJ		equ 5		; Vertical Total Adjust
	CRTC_R6_VDIS		equ 6		; Vertical Displayed
	CRTC_R7_VSYNC		equ 7		; Vertical Sync Position
	CRTC_R8_INTERLACE	equ 8		; Interlace and Skew
	CRTC_R9_MAXRAS		equ 9		; Maximum Raster Address
	CRTC_R12_ADDR_H		equ 12		; Display Start Address (high byte)
	CRTC_R13_ADDR_L		equ 13		; Display Start Address (low byte)

	; Disable flags -- define any of these symbols before including this file to suppress
	; the corresponding register write entirely (the macro will assemble to zero bytes).
	;
	; Example: to skip the VSYNC position write, add this before the include:
	;   CRTCDisableVSyncPos equ 1
	;
	; Available disable symbols:
	;   CRTCDisableHTOT
	;	CRTCDisableHDIS
	;	CRTCDisableHSyncPos
	;	CRTCDisableSyncWidth
	;   CRTCDisableVTOT
	;	CRTCDisableVTAdj
	;	CRTCDisableVDIS
	;	CRTCDisableVSyncPos
	;   CRTCDisableMaxRaster
	;	CRTCDisableInterlaceSkew
	;	CRTCDisableAddress

	; ============================================================
	; SPEED REFERENCE
	; ============================================================
	;
	; Speed notes (CPC timing in µs, where 1 µs = 4 CPC T-states = 1 NOP):
	;   ld bc,nn    =  3 µs  -- load 16-bit immediate (used to address the CRTC port)
	;   ld b,n      =  2 µs  -- load 8-bit immediate into B (Reg variants use this)
	;   out (c),c   =  4 µs  -- write C to port addressed by BC
	;   out (c),r   =  4 µs  -- write any 8-bit register r to port addressed by BC
	;   inc/dec r   =  1 µs  -- used in CRTC_SetAddress to step between ports
	;
	; Immediate macro speed (ld bc+out+ld bc+out):  3+4+3+4 = 14 µs = 56 CPC T-states = 14 NOPs
	; Register macro speed (ld bc+out+ld b+out):    3+4+2+4 = 13 µs = 52 CPC T-states = 13 NOPs
	; CRTC_SetAddress speed:                         (3+4)×4 = 28 µs = 112 CPC T-states = 28 NOPs

	; ============================================================
	; HORIZONTAL TOTAL (HTOT) -- Register 0
	; ============================================================
	;
	; Effect is immediate. Default value is 63.
	; Total scanline length in characters, including borders and hsync.
	; Must be >= HDIS + HSYNC width. Typical CPC value: 63.
	; Define CRTCDisableHTOT to suppress both macros (assembles to zero bytes).

	; CRTC_SetHTOT HTOT -- set Horizontal Total from an immediate value.
	; Speed: 14 µs = 56 CPC T-states = 14 NOPs (constant, no branching)
	; Breakdown: ld bc,nn (3µs) + out (c),r (4µs) + ld bc,nn (3µs) + out (c),r (4µs) = 14µs.
	;
	macro CRTC_SetHTOT HTOT
		ifndef CRTCDisableHTOT
			ld bc,CRTC_PORT_ADDR+CRTC_R0_HTOT	; 10 T -- select R0
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{HTOT}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetHTOTReg Reg -- register variant of CRTC_SetHTOT.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 13 µs = 52 CPC T-states = 13 NOPs (constant, no branching)
	; Breakdown: ld bc,nn (3µs) + out (c),r (4µs) + ld r,n (2µs) + out (c),r (4µs) = 13µs.
	;
	macro CRTC_SetHTOTReg Reg
		ifndef CRTCDisableHTOT
			ld bc,CRTC_PORT_ADDR+CRTC_R0_HTOT	; 10 T -- select R0
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; HORIZONTAL DISPLAYED (HDIS) -- Register 1
	; ============================================================
	;
	; Effect is immediate. Default value is 40.
	; Number of character columns visible on screen per scanline (excluding borders).
	; Increasing this widens the display area. Typical CPC value: 40.
	; Define CRTCDisableHDIS to suppress both macros (assembles to zero bytes).

	; CRTC_SetHDIS HDIS -- set Horizontal Displayed from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetHDIS HDIS
		ifndef CRTCDisableHDIS
			ld bc,CRTC_PORT_ADDR+CRTC_R1_HDIS	; 10 T -- select R1
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{HDIS}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetHDISReg Reg -- register variant of CRTC_SetHDIS.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetHDISReg Reg
		ifndef CRTCDisableHDIS
			ld bc,CRTC_PORT_ADDR+CRTC_R1_HDIS	; 10 T -- select R1
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; HORIZONTAL SYNC POSITION (HSYNC) -- Register 2
	; ============================================================
	;
	; Effect is immediate. Default value is 46.
	; Character position at which the HSYNC pulse begins.
	; Must be greater than HDIS so the sync pulse falls outside the visible area.
	; Define CRTCDisableHSyncPos to suppress both macros (assembles to zero bytes).

	; CRTC_SetHSyncPos HSyncPos -- set Horizontal Sync Position from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetHSyncPos HSyncPos
		ifndef CRTCDisableHSyncPos
			ld bc,CRTC_PORT_ADDR+CRTC_R2_HSYNC	; 10 T -- select R2
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{HSyncPos}		; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetHSyncPosReg Reg -- register variant of CRTC_SetHSyncPos.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetHSyncPosReg Reg
		ifndef CRTCDisableHSyncPos
			ld bc,CRTC_PORT_ADDR+CRTC_R2_HSYNC	; 10 T -- select R2
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; SYNC PULSE WIDTH (horizontal + vertical) -- Register 3
	; ============================================================
	;
	; Effect is immediate. Default value is #8e (8 lines of vsync, 14 chars of hsync).
	; Bits 7-4: vertical sync width in scan lines (CRTC0 only; ignored on CRTC1/CRTC2 which fix it at 16).
	;           A value of 0 means 16 lines of vsync pulse on CRTC0.
	; Bits 3-0: horizontal sync width in characters. A value of 0 disables hsync.
	; Define CRTCDisableSyncWidth to suppress both macros (assembles to zero bytes).

	; CRTC_SetSyncWidth -- set Sync Width from A (value must be loaded into A by the caller).
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetSyncWidth SyncWidth
		ifndef CRTCDisableSyncWidth
			ld bc,CRTC_PORT_ADDR+CRTC_R3_SYNC_W	; 10 T -- select R3
			out (c),c								; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{SyncWidth}		; 10 T -- B=#BD, C=0 (data port base)
			out (c),c								; 12 T -- write A to data port
		endif
	endm

	; CRTC_SetSyncWidthReg Reg -- register variant of CRTC_SetSyncWidth.
	; Use to write from any 8-bit register instead of A. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetSyncWidthReg Reg
		ifndef CRTCDisableSyncWidth
			ld bc,CRTC_PORT_ADDR+CRTC_R3_SYNC_W	; 10 T -- select R3
			out (c),c								; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8					; 7 T  -- B=#BD (data port)
			out (c),{Reg}							; 12 T -- write register value
		endif
	endm

	; ============================================================
	; VERTICAL TOTAL (VTOT) -- Register 4
	; ============================================================
	;
	; Effect is immediate (takes effect at the next frame). Default value is 38.
	; Total number of character rows per frame, including borders and vsync.
	; Must be >= VDIS.
	; Define CRTCDisableVTOT to suppress both macros (assembles to zero bytes).

	; CRTC_SetVTOT VTOT -- set Vertical Total from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetVTOT VTOT
		ifndef CRTCDisableVTOT
			ld bc,CRTC_PORT_ADDR+CRTC_R4_VTOT	; 10 T -- select R4
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{VTOT}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetVTOTReg Reg -- register variant of CRTC_SetVTOT.
	; Use when the vertical total is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetVTOTReg Reg
		ifndef CRTCDisableVTOT
			ld bc,CRTC_PORT_ADDR+CRTC_R4_VTOT	; 10 T -- select R4
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; VERTICAL TOTAL ADJUST (VTADJ) -- Register 5
	; ============================================================
	;
	; Effect is immediate (takes effect at the next frame). Default value is 0.
	; Adds extra scan lines (0-31) after VTOT rows to fine-tune frame timing.
	; Define CRTCDisableVTAdj to suppress both macros (assembles to zero bytes).

	; CRTC_SetVTAdj VTADJ -- set Vertical Total Adjust from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetVTAdj VTADJ
		ifndef CRTCDisableVTAdj
			ld bc,CRTC_PORT_ADDR+CRTC_R5_VTADJ	; 10 T -- select R5
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{VTADJ}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetVTAdjReg Reg -- register variant of CRTC_SetVTAdj.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetVTAdjReg Reg
		ifndef CRTCDisableVTAdj
			ld bc,CRTC_PORT_ADDR+CRTC_R5_VTADJ	; 10 T -- select R5
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; VERTICAL DISPLAYED (VDIS) -- Register 6
	; ============================================================
	;
	; Effect is immediate (takes effect at the next frame). Default value is 25.
	; Number of character rows visible on screen (excluding borders).
	; Increasing this shows more rows. Must be <= VTOT.
	; Define CRTCDisableVDIS to suppress both macros (assembles to zero bytes).

	; CRTC_SetVDIS VDIS -- set Vertical Displayed from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetVDIS VDIS
		ifndef CRTCDisableVDIS
			ld bc,CRTC_PORT_ADDR+CRTC_R6_VDIS	; 10 T -- select R6
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{VDIS}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetVDISReg Reg -- register variant of CRTC_SetVDIS.
	; Use when the row count is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetVDISReg Reg
		ifndef CRTCDisableVDIS
			ld bc,CRTC_PORT_ADDR+CRTC_R6_VDIS	; 10 T -- select R6
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; VERTICAL SYNC POSITION (VSYNC) -- Register 7
	; ============================================================
	;
	; Effect is immediate (takes effect at the next frame). Default value is 30.
	; Character row at which the VSYNC pulse begins.
	; Must be greater than VDIS so the sync pulse falls outside the visible area.
	; Define CRTCDisableVSyncPos to suppress both macros (assembles to zero bytes).

	; CRTC_SetVSyncPos VSYNCPOS -- set Vertical Sync Position from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetVSyncPos VSYNCPOS
		ifndef CRTCDisableVSyncPos
			ld bc,CRTC_PORT_ADDR+CRTC_R7_VSYNC	; 10 T -- select R7
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{VSYNCPOS}		; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetVSyncPosReg Reg -- register variant of CRTC_SetVSyncPos.
	; Use when the VSYNC position is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetVSyncPosReg Reg
		ifndef CRTCDisableVSyncPos
			ld bc,CRTC_PORT_ADDR+CRTC_R7_VSYNC	; 10 T -- select R7
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; INTERLACE AND SKEW -- Register 8
	; ============================================================
	;
	; Effect is immediate. Default value is #00 (no interlace, no skew -- CPC firmware default).
	;
	; Bit layout:
	;   Bits 1-0 -- Interlace mode:
	;     00 = Normal / non-interlace (default)
	;     01 = Interlace Sync
	;     10 = Normal (reserved, same as 00)
	;     11 = Interlace Sync and Video
	;   Bits 3-2 -- unused, must be 0
	;   Bits 5-4 -- Display skew (delays DISPEN signal by N character clocks):
	;     00 = no skew (default)
	;     01 = 1 character clock delay
	;     10 = 2 character clock delay
	;     11 = display output disabled
	;   Bits 7-6 -- Cursor skew (same encoding as bits 5-4, applies to cursor only)
	;
	; CRTC compatibility:
	;   Type 0 (HD6845S)  -- fully supported: interlace + display skew + cursor skew
	;   Type 1 (UM6845R)  -- interlace bits work; display and cursor skew bits are ignored
	;   Type 2 (MC6845)   -- fully supported
	;   Type 3 (CPC+ ASIC)-- R8 is ignored entirely; behaviour is fixed in hardware
	;   Type 4 (MC6845B)  -- fully supported
	;
	; Practical notes for CPC development:
	;   - The CPC firmware initialises R8 to #00 and most software never changes it.
	;   - Interlace is rarely useful on CPC -- it halves the effective frame rate (25 Hz)
	;     and is not supported by the GA palette or the firmware display routines.
	;   - Display skew (bits 5-4) is relevant on CRTC type 0 when pushing horizontal
	;     timing to the limit: if HDIS is set very close to HTOT, a skew of 1 can
	;     prevent a single garbage pixel column at the right edge of the display.
	;   - Changing R8 mid-frame is not recommended; apply it during VSYNC.
	; Define CRTCDisableInterlaceSkew to suppress both macros (assembles to zero bytes).

	; CRTC_SetInterlaceSkew Value -- set Interlace and Skew from an immediate value.
	; Example -- 1-char display skew, no interlace: CRTC_SetInterlaceSkew #10
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetInterlaceSkew Value
		ifndef CRTCDisableInterlaceSkew
			ld bc,CRTC_PORT_ADDR+CRTC_R8_INTERLACE	; 10 T -- select R8
			out (c),c								; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{Value}			; 10 T -- B=#BD, C=value
			out (c),c								; 12 T -- write value
		endif
	endm

	; CRTC_SetInterlaceSkewReg Reg -- register variant of CRTC_SetInterlaceSkew.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetInterlaceSkewReg Reg
		ifndef CRTCDisableInterlaceSkew
			ld bc,CRTC_PORT_ADDR+CRTC_R8_INTERLACE	; 10 T -- select R8
			out (c),c								; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8					; 7 T  -- B=#BD (data port)
			out (c),{Reg}							; 12 T -- write register value
		endif
	endm

	; ============================================================
	; MAXIMUM RASTER ADDRESS (MAXRAS) -- Register 9
	; ============================================================
	;
	; Effect is immediate (takes effect at the next frame). Default value is 7.
	; Defines the number of scan lines per character row minus 1.
	; Standard CPC values: 7 (Mode 0/1/2, 8 lines/row), 3 (hardware double-height trick).
	; Define CRTCDisableMaxRaster to suppress both macros (assembles to zero bytes).

	; CRTC_SetMaxRaster Max -- set Maximum Raster Address from an immediate value.
	; Speed: 10 + 12 + 10 + 12 = 44 T (constant, no branching)
	;
	macro CRTC_SetMaxRaster Max
		ifndef CRTCDisableMaxRaster
			ld bc,CRTC_PORT_ADDR+CRTC_R9_MAXRAS	; 10 T -- select R9
			out (c),c							; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+{Max}			; 10 T -- B=#BD, C=value
			out (c),c							; 12 T -- write value
		endif
	endm

	; CRTC_SetMaxRasterReg Reg -- register variant of CRTC_SetMaxRaster.
	; Use when the value is computed at runtime. Reg = any 8-bit register (A,B,C,D,E,H,L).
	; Note: if Reg is A then B is still clobbered; A survives unchanged.
	; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching)
	;
	macro CRTC_SetMaxRasterReg Reg
		ifndef CRTCDisableMaxRaster
			ld bc,CRTC_PORT_ADDR+CRTC_R9_MAXRAS	; 10 T -- select R9
			out (c),c							; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8				; 7 T  -- B=#BD (data port)
			out (c),{Reg}						; 12 T -- write register value
		endif
	endm

	; ============================================================
	; DISPLAY START ADDRESS -- Registers 12 (high) and 13 (low)
	; ============================================================
	;
	; Effect is latched at the next VSYNC -- change during the frame for smooth scrolling.
	; The 14-bit address (bits 13-0) maps to the CRTC address space (each unit = 2 bytes on CPC).
	; VMA structure: MA[13:12] = page select (R12[5:4]), MA[11:10] = size (R12[3:2]),
	;                MA[9:0] = 10-bit offset { R12[1:0], R13[7:0] }
	; Define CRTCDisableAddress to suppress both macros (assembles to zero bytes).
	;
	; CRTC_SetAddress addr -- set display start address from an immediate label/address.
	; The assembler extracts high() and low() from the address automatically.
	; Speed: 28 µs = 112 CPC T-states = 28 NOPs (constant, no branching)
	; Breakdown: (ld bc,nn 3µs + out (c),r 4µs) × 4 = 28µs. (Sets R12 and R13, each pair = 7µs)
	;
	macro CRTC_SetAddress addr
		ifndef CRTCDisableAddress
			ld bc,CRTC_PORT_ADDR+CRTC_R12_ADDR_H		; 10 T -- select R12
			out (c),c									; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+((hi({addr})) & 0x3F) ; 10 T -- R12: HI byte [5:0]
			out (c),c									; 12 T -- write value
			ld bc,CRTC_PORT_ADDR+CRTC_R13_ADDR_L		; 10 T -- select R13
			out (c),c									; 12 T -- write register index
			ld bc,CRTC_PORT_DATA+(lo({addr}))			; 10 T -- R13: LO byte
			out (c),c									; 12 T -- write value
		endif
	endm

	; CRTC_SetAddressReg regHi, regLo -- register variant of CRTC_SetAddress.
	; Use when address is computed at runtime into two 8-bit registers.
	; regHi contains { pp[1:0] oo[1:0] aaaa[9:6] }, regLo contains aaaa[5:0].
	; Speed: 26 µs = 104 CPC T-states = 26 NOPs (constant, no branching)
	; Breakdown: (ld bc,nn 3µs + out (c),r 4µs + ld r,n 2µs + out (c),r 4µs) × 2 = 26µs.
	;
	macro CRTC_SetAddressReg regHi, regLo
		ifndef CRTCDisableAddress
			ld bc,CRTC_PORT_ADDR+CRTC_R12_ADDR_H		; 10 T -- select R12
			out (c),c									; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8						; 7 T  -- B = #BD (data port)
			out (c),{regHi}								; 12 T -- write high byte
			ld bc,CRTC_PORT_ADDR+CRTC_R13_ADDR_L		; 10 T -- select R13
			out (c),c									; 12 T -- write register index
			ld b,CRTC_PORT_DATA>>8						; 7 T  -- B = #BD (data port)
			out (c),{regLo}								; 12 T -- write low byte
		endif
	endm
endif
