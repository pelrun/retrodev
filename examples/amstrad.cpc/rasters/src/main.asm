;-----------------------------------------------------------------------
;
; Retrodev Examples
;
; rasters -- demonstrates per-scanline raster colour updates and timing on CPC.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

;-----------------------------------------------------------------------
;
; Raster bars demo -- Amstrad CPC
;
; Displays multicolour gradient raster bars (blue/green/red ranges with bright
; highlights) scrolling continuously across all 200 active scanlines.
; Pen 0 and the border colour are
; changed on every scanline via a tight 256T-per-iteration loop.
;
; Technique:
;   - CPC_DisableInterruptsSync installs a null EI+RETI handler at #0038
;     and resyncs the GA interrupt counter to the VSYNC rising edge.
;     Interrupts fire normally but do nothing, keeping the Z80 /INT clean.
;   - BuildColourTable fills ColourTable[0..199] with the GA colour byte
;     for each of the 200 active scanlines. It runs in the upper border window
;     (after VSYNC rising edge, before active display line 0) -- 64 lines =
;     16384T of headroom. Placing it here (after the VSYNC sync point) means
;     it never races the VSYNC signal, eliminating the one-frame jitter that
;     caused blinking. Background lines get RASTER_BG_COLOR; bar lines get
;     the gradient step.
;   - Bars tile every BAR_SPACING scanlines (256 / 8 = 32 lines). All 8
;     virtual bars share one ScrollY counter that advances by RASTER_SPEED
;     each frame, producing continuous downward scrolling. Because 8*32=256
;     the wrapping is exact with 8-bit arithmetic -- no modulo needed.
;   - Each bar gradient uses one of three BAR_SHAPE_HEIGHT profiles:
;     blue (blue->cyan->white), red (orange/brown->red->white), or green
;     (green->cyan->white). The selected shape is painted centred on each bar.
;   - After VSYNC rising edge, BuildColourTable fills the ColourTable in the
;     64-line upper border window (16384T). A small nop filler (ALIGN_FILLER)
;     trims the remaining gap so ScanlineLoop starts exactly at active line 0.
;     ScanlineLoop runs for exactly 200 iterations at 256T each, writing
;     pen 0 and border on every scanline.
;
; Scanline loop timing (256T per iteration = 64 NOPs equivalent, B=#7f throughout):
;   ld a,(hl)          (7) + inc hl   (6)  = 13T  fetch next line colour early
;   push af (11) + pop af (10)             = 21T  timing filler (preserves A)
;   defs 36                                = 144T 36 NOPs
;   ld c,GA_PEN_0      (7) + out(c)c (12)  = 19T  select pen 0 near line end
;   out (c),a          (12)                = 12T  write pen 0 colour
;   ld c,GA_PEN_BORDER (7) + out(c)c (12)  = 19T  select border near line end
;   out (c),a          (12)                = 12T  write border colour
;   dec d (4) + jr nz (12/8)               = 16T  loop control
;   Total: 13+21+144+19+12+19+12+16        = 256T
;
; Memory layout:
;   #4000  jp EntryPoint trampoline
;   #4003+ IntNull_End, CPC_DisableInterrupts, CPC_DisableInterruptsSync
;   #40xx  BarShape, EntryPoint, MainLoop, BuildColourTable, ScrollY
;   #6200  ColourTable (200 bytes, rebuilt each frame)
;
;-----------------------------------------------------------------------

org #4000

; Trampoline at #4000: jumps past the included function libraries to EntryPoint.
; Required so that IntNull_End (EI+RETI) and the disable routines are assembled
; at known addresses that are included in the saved binary (SAVE starts at #4000).
jp EntryPoint

;-----------------------------------------------------------------------
; Includes
;-----------------------------------------------------------------------

include 'macros.ga.asm'
include 'cpc.interrupts.disable.asm'
include 'cpc.interrupts.disable.sync.asm'

;-----------------------------------------------------------------------
; Configuration constants
;-----------------------------------------------------------------------

; Scanline distance between bar centres. Must divide 256 evenly.
; 256 / BAR_SPACING = NUM_BARS virtual bars.
BAR_SPACING			equ 32

; Number of virtual bars tiling the 256-entry scroll space.
NUM_BARS			equ 256 / BAR_SPACING

; Background GA colour byte for scanlines outside any bar.
RASTER_BG_COLOR		equ GA_COLOR_BLACK

; Extra black rows painted before and after the gradient.
; With 2 rows, bar painting clears correctly for up to 2 lines/frame movement.
BAR_CLEAR_ROWS		equ 2

; Height of the gradient bar shape in scanlines (must be <= BAR_SPACING).
BAR_SHAPE_HEIGHT	equ 18 + (BAR_CLEAR_ROWS * 2)

; Vertical movement limits for each bar centre (keep full shape on-screen).
BAR_MIN_CENTRE		equ BAR_SHAPE_HEIGHT/2
BAR_MAX_CENTRE		equ 199 - (BAR_SHAPE_HEIGHT/2)

; Nop filler after BuildColourTable to align beam to active display line 0.
; CPC frame: 312 lines total. VSYNC rising at line ~248. Active display
; line 0 is 64 lines after VSYNC rising = 16384T.
; Fixed overhead from GA_VSyncWaitON exit to ScanlineLoop start (worst case):
;   GA_VSyncWaitON last iter: ld b(7)+in(12)+rra(4)+jr(8)        =  31T
;   ld a/add/ld ScrollY + call BuildColourTable                   =  50T
;   BuildColourTable worst case (all 8 bars fully visible)        = 10626T
;   ld hl,ColourTable + ld d,200 + ld b,GA_PORT_CMD_B             =  24T
;   Total fixed: 10731T. Remaining: 16384 - 10731 = 5653T.
;   5653 / 4T per nop = 1413 nops (5652T, 1T under -- one scanline off at most).
; Tune ALIGN_FILLER by 4T steps if bars appear shifted (add nops = shift bars down, remove = shift up).
ALIGN_FILLER		equ 1413		; 1413 nop = 5652T filler after BuildColourTable

;-----------------------------------------------------------------------
; Bar gradient shapes -- BAR_SHAPE_HEIGHT entries each.
; Bars are colour-family based (blue/red/green), not mixed per-bar.
; Each entry is a GA colour byte ready to write (GA_CMD_COLOR_SET | hw_idx).
; Extra BAR_CLEAR_ROWS black entries at both ends help clear trails at higher speed.
;-----------------------------------------------------------------------

BarShapeBlue:
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra leading black rows
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLUE
	defb GA_COLOR_BLUE
	defb GA_COLOR_BRIGHT_BLUE
	defb GA_COLOR_SKY_BLUE
	defb GA_COLOR_CYAN
	defb GA_COLOR_BRIGHT_CYAN
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_BRIGHT_CYAN
	defb GA_COLOR_CYAN
	defb GA_COLOR_SKY_BLUE
	defb GA_COLOR_BRIGHT_BLUE
	defb GA_COLOR_BLUE
	defb GA_COLOR_BLUE
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra trailing black rows

BarShapeRed:
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra leading black rows
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defb GA_COLOR_ORANGE
	defb GA_COLOR_ORANGE
	defb GA_COLOR_RED
	defb GA_COLOR_BRIGHT_RED
	defb GA_COLOR_BRIGHT_RED
	defb GA_COLOR_PINK
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_PINK
	defb GA_COLOR_BRIGHT_RED
	defb GA_COLOR_BRIGHT_RED
	defb GA_COLOR_RED
	defb GA_COLOR_ORANGE
	defb GA_COLOR_ORANGE
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra trailing black rows

BarShapeGreen:
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra leading black rows
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defb GA_COLOR_GREEN
	defb GA_COLOR_GREEN
	defb GA_COLOR_BRIGHT_GREEN
	defb GA_COLOR_SEA_GREEN
	defb GA_COLOR_PASTEL_GREEN
	defb GA_COLOR_BRIGHT_CYAN
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_BRIGHT_WHITE
	defb GA_COLOR_BRIGHT_CYAN
	defb GA_COLOR_PASTEL_GREEN
	defb GA_COLOR_SEA_GREEN
	defb GA_COLOR_BRIGHT_GREEN
	defb GA_COLOR_GREEN
	defb GA_COLOR_GREEN
	defb GA_COLOR_BLACK
	defb GA_COLOR_BLACK
	defs BAR_CLEAR_ROWS, GA_COLOR_BLACK	; extra trailing black rows

;-----------------------------------------------------------------------
; Entry point
;-----------------------------------------------------------------------

EntryPoint:
	; Set screen mode 1 (320x200, 4 colours).
	GA_SetMode GA_MODE_1
	; Initialise pen 0 and border to background colour.
	GA_SetInk 0, GA_COLOR_BLACK
	GA_SetBorder GA_COLOR_BLACK
	; Build the initial colour table.
	call BuildColourTable
	; Install null interrupt handler and resync GA counter to VSYNC rising edge.
	ld b,GA_MODE_1
	call CPC_DisableInterruptsSync

;-----------------------------------------------------------------------
; MainLoop
;
; Each frame:
;   1. ScanlineLoop has just finished (beam is in lower border, ~48 lines = 12288T).
;   2. GA_VSyncWaitOFF+ON syncs to the VSYNC rising edge (line 248).
;      From VSYNC rising to active display line 0 = 64 lines = 16384T.
;   3. BuildColourTable updates each bar centre/direction independently,
;      then fills ColourTable[0..199].
;      Runs in the post-VSYNC window (upper border). Must complete in <64 lines.
;      Running after the sync point means it never races with VSYNC -- stable every frame.
;   4. ScanlineLoop runs 200 iterations at exactly 256T each.
;      Started as soon as BuildColourTable returns -- no separate alignment delay.
;      BuildColourTable is timed to consume ~16384T (64 lines) exactly so it
;      returns right at line 0. ALIGN_FILLER nops trim the remaining gap.
;
; Register usage in ScanlineLoop:
;   B = GA_PORT_CMD_B (#7f) -- constant, never changed inside the loop.
;   D = scanline counter (200 down to 1).
;   HL = pointer into ColourTable, incremented once per scanline.
;   A = colour byte fetched from ColourTable (reused for border write).
;   C = pen-select command (GA_PEN_0 or GA_PEN_BORDER).
;-----------------------------------------------------------------------

MainLoop:
	; Sync to VSYNC rising edge. Called from lower border so VSYNC is not yet
	; active -- GA_VSyncWaitOFF exits immediately, GA_VSyncWaitON waits for rising edge.
	GA_VSyncWaitOFF
	GA_VSyncWaitON
	; Advance scroll and rebuild colour table in the upper border window.
	; From VSYNC rising to line 0 = 64 lines = 16384T. BuildColourTable + filler = 16384T.
	call BuildColourTable
	; Filler nops to consume remaining cycles so ScanlineLoop starts exactly at line 0.
	; Tune ALIGN_FILLER if bars appear shifted up or down.
	defs ALIGN_FILLER
	; Set up ScanlineLoop registers.
	ld hl,ColourTable
	ld d,200
	ld b,GA_PORT_CMD_B
ScanlineLoop:
	ld a,(hl)			; 7T  -- fetch next scanline colour byte
	inc hl				; 6T  -- advance table pointer
	push af				; 11T -- timing filler (saves A across the NOPs)
	pop af				; 10T -- restore A
	defs 36				; 144T -- 36 x nop, timing filler to reach 256T total
	ld c,GA_PEN_0
	out (c),c			; 12T -- select pen 0 on GA near line end
	out (c),a			; 12T -- write colour to pen 0  (B=#7f, A=colour)
	ld c,GA_PEN_BORDER	; 7T  -- border select command into C
	out (c),c			; 12T -- select border pen on GA near line end
	out (c),a			; 12T -- write same colour to border
	dec d				; 4T  -- decrement scanline counter
	jr nz,ScanlineLoop	; 12T taken / 8T on exit
	jp MainLoop

;-----------------------------------------------------------------------
; BuildColourTable -- update bars and populate ColourTable[0..199] from BarShape.
;
; Algorithm:
;   1. For each of the NUM_BARS virtual bars:
;        centre += delta (delta is signed: up/down speed)
;        bounce at BAR_MIN_CENTRE / BAR_MAX_CENTRE, inverting delta
;        first  = centre - BAR_SHAPE_HEIGHT/2            (may underflow)
;        For each row 0..BAR_SHAPE_HEIGHT-1:
;          scanline = first + row
;          If scanline in 0..199: ColourTable[scanline] = BarShape[row]
;
; Underflow guard: after 'sub BAR_SHAPE_HEIGHT/2' any negative result
; wraps to 249..255 (>= 200), which the 'cp 200; jr nc' check discards.
; Overflow: centres >= (200 + BAR_SHAPE_HEIGHT/2) produce first values
; past 199 before any shape rows land in range, also caught by cp 200.
;
; ColourTable is at #6200 so its high byte is #62. ColourTable[i] address
; = #6200 + i = DE where D=#62, E=i. This avoids a 16-bit add inside the loop.
;
; Each bar state is stored in BarStateTable as (centre, delta).
; The outer bar loop uses B; the inner shape loop uses C.
;
; Entry:  none
; Exit:   ColourTable[0..199] ready for ScanlineLoop.
; Destroys: AF, BC, DE, HL
;-----------------------------------------------------------------------
BuildColourTable:
	; Update each bar centre/direction, then paint each bar's gradient into the table.
	; D = #62 (ColourTable high byte) -- constant throughout, set once here.
	ld d,ColourTable >> 8
	ld hl,BarStateTable
	ld b,NUM_BARS
.bar_loop:
	; Load and update this bar state.
	; [HL+0] = centre, [HL+1] = delta (signed).
	push bc
	ld a,(hl)
	inc hl
	ld e,(hl)
	add a,e
	bit 7,e
	jr nz,.check_min
	cp BAR_MAX_CENTRE+1
	jr c,.state_ok
	ld a,BAR_MAX_CENTRE
	ld c,e
	xor a
	sub c
	ld e,a
	ld a,BAR_MAX_CENTRE
	jr .state_ok
.check_min:
	cp BAR_MIN_CENTRE
	jr nc,.state_ok
	ld a,BAR_MIN_CENTRE
	ld c,e
	xor a
	sub c
	ld e,a
	ld a,BAR_MIN_CENTRE
.state_ok:
	ld (hl),e
	dec hl
	ld (hl),a
	; Compute first scanline = centre - BAR_SHAPE_HEIGHT/2.
	sub BAR_SHAPE_HEIGHT/2		; A = first scanline (wraps if centre < 9)
	; Walk selected BarShape and write into ColourTable.
	push hl						; save bar state pointer
	push af						; preserve first scanline while selecting shape
	ld a,NUM_BARS
	sub b						; bar index 0..7 (first to last)
	cp 1
	jr z,.shape_red
	cp 2
	jr z,.shape_green
	cp 4
	jr z,.shape_red
	cp 5
	jr z,.shape_green
	cp 7
	jr z,.shape_red
	ld hl,BarShapeBlue
	jr .shape_ready
.shape_red:
	ld hl,BarShapeRed
	jr .shape_ready
.shape_green:
	ld hl,BarShapeGreen
.shape_ready:
	pop af						; restore first scanline
	ld c,BAR_SHAPE_HEIGHT
.shape_loop:
	; Guard: skip if scanline A is outside 0..199.
	; Negative (underflow) wraps to 249..255 which is >= 200, so one cp covers both.
	; Skip path pads to near-match write path so timing is nearly frame-invariant.
	; Write: cp(7)+jr c,write(12)+ld e(4)+ld a,(hl)(7)+ld(de),a(7)+ld a,e(4)+tail(27T) = 68T
	; Skip:  cp(7)+jr c,write(8)+defs 6(24)+jr .tail(12)+tail(27T) = 78T -- 10T over write
	; Delta per row = 10T; max jitter: 8*18*10 = 1440T = 5.6 scanlines -- not good enough.
	; Use ex af,af'(4) in skip instead of jr to save 8T: skip=cp(7)+jr(8)+defs6(24)+ex af(4)+tail(27)=70T.
	; Delta per row = 2T; max jitter: 8*18*2 = 288T = ~1 scanline. Accepted.
	cp 200
	jr c,.shape_write
	defs 6						; 6 nop = 24T timing pad
	ex af,af'					; 4T timing pad (skip path total before tail: 7+8+24+4 = 43T; write: 7+12+4+7+7+4 = 41T; delta=2T/row)
	jr .shape_tail
.shape_write:
	ld e,a						; E = scanline index (0..199)
	ld a,(hl)					; A = shape colour byte
	ld (de),a					; ColourTable[scanline] = colour
	ld a,e						; restore scanline index into A
.shape_tail:
	inc hl						; next BarShape row
	inc a						; next scanline
	dec c
	jr nz,.shape_loop
	; Restore bar loop counter and step to next bar state.
	pop hl
	inc hl
	inc hl
	pop bc
	djnz .bar_loop
	ret

;-----------------------------------------------------------------------
; Persistent variables
;-----------------------------------------------------------------------

; Per-bar state: (centre, delta) pairs. Delta is signed (+down, -up).
; Using +/-1/+/-2 with BAR_CLEAR_ROWS=2 avoids stale-colour trails.
BarStateTable:
	defb 16, 1
	defb 38, 2
	defb 62, -1
	defb 84, -2
	defb 112, 1
	defb 134, -2
	defb 160, 2
	defb 186, -1

;-----------------------------------------------------------------------
; ColourTable -- 200 bytes, one GA colour byte per active scanline.
; Rebuilt each frame by BuildColourTable; consumed in order by ScanlineLoop.
;-----------------------------------------------------------------------

org #6200
ColourTable:
	defs 200, GA_COLOR_BLACK

;-----------------------------------------------------------------------
; Save directive
;-----------------------------------------------------------------------

SAVE 'rasters.bin',#4000,$-#4000,DSK,'out/rasters.dsk'
