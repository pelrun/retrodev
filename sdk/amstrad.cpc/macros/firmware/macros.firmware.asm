;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC firmware macro definitions.
;
; Purpose: expose firmware jump-block addresses and call helper macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __macros_firmware_asm__
__macros_firmware_asm__ equ 1
    ; Amstrad CPC Firmware jump block -- address equs and documentation.
    ;
    ; All firmware entry points are fixed RST/CALL targets in the low jump block.
    ; To call a routine, simply:
    ;
    ;   call FW_KM_WAIT_KEY     ; wait for a key press, return ASCII in A
    ;
    ; Version notes:
    ;   [464]  -- present on all CPC models (464, 664, 6128, Plus)
    ;   [664+] -- introduced on the 664; present on 6128 and Plus
    ;   [6128+]-- introduced on the 6128; present on Plus
    ;   [Plus] -- CPC Plus / GX4000 only (ASIC firmware)
    ;
    ; Calling convention (all routines unless noted otherwise):
    ;   IX and IY are preserved by all firmware routines.
    ;   Carry set on return typically indicates success; meaning is noted per routine.
    ;   All other registers should be assumed destroyed unless the Out/Destroys line says otherwise.
    ;
    ; The jump block lives at #BB00-#BDF9. Never call addresses in the gaps --
    ; undefined behaviour or hard crash on real hardware.
    ;
    ; Reference: Locomotive Software SOFT968 firmware guide (1985), plus
    ;            Soft968 addendum for 664/6128 extensions.
    ;

    ; ============================================================
    ; KEYBOARD MANAGER  (KM)
    ; ============================================================
    ;
    ; Manages the keyboard matrix scan, key repeat, and character queues.
    ; The firmware scans 10 matrix lines via PPI and AY R14 every 1/50 s.
    ;

    ; FW_KM_INITIALISE -- [464] reset the keyboard manager to its default power-on state.
    ; Flushes all character queues, clears expansion strings, resets repeat timers,
    ; and restores the default key translation tables.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KM_INITIALISE	equ #BB00

    ; FW_KM_RESET -- [464] flush queues and reset key repeat timers, without full re-init.
    ; Less destructive than KM_INITIALISE -- does not restore translation tables.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KM_RESET			equ #BB03

    ; FW_KM_WAIT_KEY -- [464] wait until a key is pressed then return its ASCII value.
    ; Blocks until a character is available in the keyboard queue.
    ; Key expansion strings are processed transparently.
    ; In:      none
    ; Out:     A = ASCII code of key pressed
    ; Destroys: AF, BC, DE, HL
    FW_KM_WAIT_KEY		equ #BB06

    ; FW_KM_READ_KEY -- [464] non-blocking poll for a key press.
    ; Returns immediately: Carry set if a character is ready, Carry clear if not.
    ; In:      none
    ; Out:     Carry set: A = ASCII code of key; Carry clear: no key available (A undefined)
    ; Destroys: AF, BC, DE, HL
    FW_KM_READ_KEY		equ #BB09

    ; FW_KM_CHAR_RETURN -- [464] push a character back onto the front of the key queue.
    ; The next call to KM_WAIT_KEY / KM_READ_KEY will return this character first.
    ; In:      A = character to push back
    ; Out:     Carry set = success; Carry clear = queue is full
    ; Destroys: AF, BC, DE, HL
    FW_KM_CHAR_RETURN	equ #BB0C

    ; FW_KM_SET_EXPAND -- [464] define or update a key expansion string.
    ; Associates a token byte with a string so the token produces the string when typed.
    ; In:      B = token byte (must be in expansion range), C = string length, HL = string address
    ; Out:     Carry set = success; Carry clear = insufficient expansion buffer space
    ; Destroys: AF, BC, DE, HL
    FW_KM_SET_EXPAND	equ #BB0F

    ; FW_KM_GET_EXPAND -- [464] retrieve the expansion string for a given token.
    ; In:      A = token byte
    ; Out:     Carry set: HL = string address, B = string length; Carry clear: token not found
    ; Destroys: AF, BC, DE, HL
    FW_KM_GET_EXPAND	equ #BB12

    ; FW_KM_EXP_BUFFER -- [464] set the address and size of the key expansion buffer.
    ; Must be called before KM_SET_EXPAND if the default buffer is insufficient.
    ; In:      HL = buffer address, BC = buffer length in bytes
    ; Out:     HL = previous buffer address, BC = previous buffer length
    ; Destroys: AF, BC, DE, HL
    FW_KM_EXP_BUFFER	equ #BB15

    ; FW_KM_WAIT_CHAR -- [464] wait for the next character, including from expansion strings.
    ; Like KM_WAIT_KEY but also drains expansion strings character by character.
    ; In:      none
    ; Out:     A = next character
    ; Destroys: AF, BC, DE, HL
    FW_KM_WAIT_CHAR		equ #BB18

    ; FW_KM_READ_CHAR -- [464] non-blocking poll for the next character (including expansions).
    ; In:      none
    ; Out:     Carry set: A = character; Carry clear: nothing available (A undefined)
    ; Destroys: AF, BC, DE, HL
    FW_KM_READ_CHAR		equ #BB1B

    ; FW_KM_CHAR_ABSORB -- [464] discard the next pending character without reading it.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KM_CHAR_ABSORB	equ #BB1E

    ; FW_KM_TEST_KEY -- [464] test whether a specific key is currently held down.
    ; Uses the raw key number (0-79), not the ASCII code.
    ; In:      B = key number (0-79)
    ; Out:     Carry set = key is pressed; A = key matrix row byte, HL = matrix row address
    ; Destroys: AF, BC, HL
    FW_KM_TEST_KEY		equ #BB1F

    ; FW_KM_GET_STATE -- [464] return the current state of the shift, control, and caps-lock keys.
    ; In:      none
    ; Out:     A = state byte: bit 7 = caps lock, bit 5 = shift held, bit 4 = control held
    ; Destroys: AF
    FW_KM_GET_STATE		equ #BB21

    ; FW_KM_GET_JOYSTICK -- [464] read both joystick ports.
    ; In:      none
    ; Out:     A = joystick 0 (fire=bit6, up=bit0, down=bit1, left=bit2, right=bit3)
    ;          B = joystick 1 (same layout)
    ; Destroys: AF, B
    FW_KM_GET_JOYSTICK	equ #BB24

    ; FW_KM_SET_TRANSLATE -- [464] replace the normal (unshifted) key translation table.
    ; In:      HL = address of 80-byte translation table
    ; Out:     HL = address of previous table
    ; Destroys: AF, HL
    FW_KM_SET_TRANSLATE	equ #BB27

    ; FW_KM_GET_TRANSLATE -- [464] return the address of the current normal translation table.
    ; In:      none
    ; Out:     HL = address of current normal translation table
    ; Destroys: AF, HL
    FW_KM_GET_TRANSLATE	equ #BB2A

    ; FW_KM_SET_SHIFT -- [464] replace the shifted key translation table.
    ; In:      HL = address of 80-byte shifted translation table
    ; Out:     HL = address of previous table
    ; Destroys: AF, HL
    FW_KM_SET_SHIFT		equ #BB2D

    ; FW_KM_GET_SHIFT -- [464] return the address of the current shifted translation table.
    ; In:      none
    ; Out:     HL = address of current shifted translation table
    ; Destroys: AF, HL
    FW_KM_GET_SHIFT		equ #BB30

    ; FW_KM_SET_CONTROL -- [464] replace the control key translation table.
    ; In:      HL = address of 80-byte control translation table
    ; Out:     HL = address of previous table
    ; Destroys: AF, HL
    FW_KM_SET_CONTROL	equ #BB33

    ; FW_KM_GET_CONTROL -- [464] return the address of the current control translation table.
    ; In:      none
    ; Out:     HL = address of current control translation table
    ; Destroys: AF, HL
    FW_KM_GET_CONTROL	equ #BB36

    ; FW_KM_SET_REPEAT -- [464] enable or disable auto-repeat for a specific key.
    ; In:      A = key number (0-79), B = 0 to disable repeat / non-zero to enable
    ; Out:     Carry set = success
    ; Destroys: AF, BC, DE, HL
    FW_KM_SET_REPEAT	equ #BB39

    ; FW_KM_GET_REPEAT -- [464] test whether auto-repeat is enabled for a specific key.
    ; In:      A = key number (0-79)
    ; Out:     Carry set = repeat enabled; Carry clear = repeat disabled
    ; Destroys: AF
    FW_KM_GET_REPEAT	equ #BB3C

    ; FW_KM_SET_DELAY -- [464] set the initial delay before key repeat begins.
    ; In:      B = number of 1/50 s frames before first repeat (default = 30)
    ; Out:     B = previous delay value
    ; Destroys: AF, B
    FW_KM_SET_DELAY		equ #BB3F

    ; FW_KM_SET_RATE -- [464] set the rate at which keys repeat after the initial delay.
    ; In:      B = number of 1/50 s frames between repeated characters (default = 2)
    ; Out:     B = previous rate value
    ; Destroys: AF, B
    FW_KM_SET_RATE		equ #BB42

    ; FW_KM_JAM_KEY -- [464] inject a character directly into the key queue.
    ; Useful for simulating key presses from software.
    ; In:      A = character to inject
    ; Out:     Carry set = success; Carry clear = queue is full
    ; Destroys: AF, BC, DE, HL
    FW_KM_JAM_KEY		equ #BB45

    ; FW_KM_BREAK_EVENT -- [464] install a handler to be called when BREAK is pressed.
    ; The handler is called as a KL event. Pass a KL_EVENT block initialised with KL_INIT_EVENT.
    ; In:      HL = address of initialised KL_EVENT block
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KM_BREAK_EVENT	equ #BB48

    ; ============================================================
    ; TEXT VDU MANAGER  (TXT)
    ; ============================================================
    ;
    ; Controls the text output window, cursor, and character rendering.
    ; All text coordinates are 1-based (column 1, row 1 = top-left of window).
    ;

    ; FW_TXT_INITIALISE -- [464] reset the text VDU to its default power-on state.
    ; Restores the full-screen window, default pen/paper colours, cursor position,
    ; default character matrices, and enables the VDU.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_INITIALISE	equ #BB4E

    ; FW_TXT_RESET -- [464] reset text VDU settings without a full re-init.
    ; Resets colours, window, and cursor but does not restore character matrices.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_RESET		equ #BB51

    ; FW_TXT_VDU_ENABLE -- [464] re-enable text output after TXT_VDU_DISABLE.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_TXT_VDU_ENABLE	equ #BB54

    ; FW_TXT_VDU_DISABLE -- [464] suppress all text output (writes are silently ignored).
    ; Useful during screen mode changes or direct video memory access.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_TXT_VDU_DISABLE	equ #BB57

    ; FW_TXT_OUTPUT -- [464] output a character to the text VDU, processing control codes.
    ; Control codes 0-31 perform cursor movement, window clear, etc.
    ; In:      A = character (0-255) or control code (0-31)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_OUTPUT		equ #BB5A

    ; FW_TXT_WR_CHAR -- [464] write a character glyph at the cursor position without moving the cursor.
    ; Does not interpret control codes -- the byte is always rendered as a glyph.
    ; In:      A = character (0-255)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_WR_CHAR		equ #BB5D

    ; FW_TXT_RD_CHAR -- [464] read the character at the current cursor position.
    ; Reconstructs the character by comparing screen pixel data against font matrices.
    ; In:      none
    ; Out:     Carry set: A = character found; Carry clear: A = 0 (character not recognised)
    ; Destroys: AF, BC, DE, HL
    FW_TXT_RD_CHAR		equ #BB60

    ; FW_TXT_SET_GRAPHIC -- [464] set whether text output uses graphic (pixel) coordinates.
    ; In:      A = 0 for normal text mode, A = 1 for graphics text mode
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_GRAPHIC	equ #BB63

    ; FW_TXT_WIN_ENABLE -- [464] define the text output window.
    ; All subsequent text output is clipped to this window.
    ; In:      B = left column, C = right column, D = top row, E = bottom row (all 1-based)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_WIN_ENABLE	equ #BB66

    ; FW_TXT_GET_WINDOW -- [464] return the current text window boundaries.
    ; In:      none
    ; Out:     B = left column, C = right column, D = top row, E = bottom row (all 1-based)
    ; Destroys: AF, BC, DE
    FW_TXT_GET_WINDOW	equ #BB69

    ; FW_TXT_CLEAR_WINDOW -- [464] clear the current text window to the paper colour.
    ; Cursor is moved to column 1, row 1 of the window after clearing.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_CLEAR_WINDOW	equ #BB6C

    ; FW_TXT_SET_COLUMN -- [464] set the cursor column within the current window.
    ; In:      A = column number (1-based, clipped to window bounds)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_COLUMN	equ #BB6F

    ; FW_TXT_SET_ROW -- [464] set the cursor row within the current window.
    ; In:      A = row number (1-based, clipped to window bounds)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_ROW		equ #BB72

    ; FW_TXT_SET_CURSOR -- [464] set both cursor column and row in one call.
    ; In:      H = column number, L = row number (both 1-based, clipped to window)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_CURSOR	equ #BB75

    ; FW_TXT_GET_CURSOR -- [464] return the current cursor position.
    ; In:      none
    ; Out:     H = column, L = row (both 1-based, relative to window origin)
    ; Destroys: AF, HL
    FW_TXT_GET_CURSOR	equ #BB78

    ; FW_TXT_CUR_ENABLE -- [464] allow the cursor to be displayed.
    ; The cursor will appear when the VDU is idle between character writes.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_TXT_CUR_ENABLE	equ #BB7B

    ; FW_TXT_CUR_DISABLE -- [464] prevent the cursor from being displayed.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_TXT_CUR_DISABLE	equ #BB7E

    ; FW_TXT_CUR_ON -- [464] forcibly draw the cursor glyph at the current position.
    ; Shows the cursor immediately regardless of the enable/disable state.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_CUR_ON		equ #BB81

    ; FW_TXT_CUR_OFF -- [464] forcibly erase the cursor glyph from the current position.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_CUR_OFF		equ #BB84

    ; FW_TXT_PLACE_CURSOR -- [464] draw the cursor glyph at the current cursor position.
    ; Lower-level than TXT_CUR_ON -- does not update the internal cursor visible state.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_PLACE_CURSOR	equ #BB87

    ; FW_TXT_REMOVE_CURSOR -- [464] erase the cursor glyph from the current cursor position.
    ; Lower-level than TXT_CUR_OFF -- does not update the internal cursor visible state.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_TXT_REMOVE_CURSOR	equ #BB8A

    ; FW_TXT_SET_PEN -- [464] set the text foreground (ink) pen for subsequent character writes.
    ; In:      A = pen index (0-15)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_PEN		equ #BB8D

    ; FW_TXT_GET_PEN -- [464] return the current text foreground pen index.
    ; In:      none
    ; Out:     A = current ink pen index (0-15)
    ; Destroys: AF
    FW_TXT_GET_PEN		equ #BB90

    ; FW_TXT_SET_PAPER -- [464] set the text background (paper) pen for subsequent character writes.
    ; In:      A = pen index (0-15)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_PAPER	equ #BB93

    ; FW_TXT_GET_PAPER -- [464] return the current text background pen index.
    ; In:      none
    ; Out:     A = current paper pen index (0-15)
    ; Destroys: AF
    FW_TXT_GET_PAPER	equ #BB96

    ; FW_TXT_INVERSE -- [464] swap the current ink and paper pen indices.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_TXT_INVERSE		equ #BB99

    ; FW_TXT_SET_BACK -- [464] set whether text characters are drawn with an opaque or transparent background.
    ; In:      A = 0 for opaque (paper colour fills cell), A = 1 for transparent (background unchanged)
    ; Out:     none
    ; Destroys: AF
    FW_TXT_SET_BACK		equ #BB9C

    ; FW_TXT_GET_BACK -- [464] return the current text background mode.
    ; In:      none
    ; Out:     A = 0 opaque, A = 1 transparent
    ; Destroys: AF
    FW_TXT_GET_BACK		equ #BB9F

    ; FW_TXT_GET_MATRIX -- [464] get the address of the 8-byte font matrix for a character.
    ; In:      A = character code (0-255)
    ; Out:     HL = address of the 8-byte matrix in the active font table
    ; Destroys: AF, HL
    FW_TXT_GET_MATRIX	equ #BBA2

    ; FW_TXT_SET_MATRIX -- [464] overwrite the font matrix for a single character.
    ; The new 8-byte matrix is copied into the user font table (allocated by TXT_SET_M_TABLE).
    ; In:      A = character code (0-255), HL = address of 8-byte replacement matrix
    ; Out:     Carry set = success; Carry clear = no user table installed or char out of range
    ; Destroys: AF, BC, DE, HL
    FW_TXT_SET_MATRIX	equ #BBA5

    ; FW_TXT_SET_M_TABLE -- [464] install a user-defined font matrix table.
    ; Replaces the firmware font for a range of characters with data from the given table.
    ; In:      DE = address of table (8 bytes per character), B = first character, C = last character
    ; Out:     DE = previous table address, BC = previous first/last character range
    ; Destroys: AF, BC, DE, HL
    FW_TXT_SET_M_TABLE	equ #BBA8

    ; FW_TXT_GET_M_TABLE -- [464] return the current user font matrix table address and character range.
    ; In:      none
    ; Out:     HL = table address, B = first character, C = last character
    ;          If no user table is set, HL points to the firmware ROM font
    ; Destroys: AF, BC, HL
    FW_TXT_GET_M_TABLE	equ #BBAB

    ; ============================================================
    ; GRAPHICS VDU MANAGER  (GRA)
    ; ============================================================
    ;
    ; Controls the graphics cursor, line drawing, pixel read/write, and fill.
    ; Graphics coordinates are absolute pixel positions; origin defaults to (0,0) = bottom-left.
    ; All coordinate pairs use DE=X (signed 16-bit), HL=Y (signed 16-bit).
    ;

    ; FW_GRA_INITIALISE -- [464] reset the graphics VDU to its default power-on state.
    ; Clears the graphics window to full screen, resets the origin, cursor, and pen/paper.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_INITIALISE	equ #BBAE

    ; FW_GRA_RESET -- [464] reset graphics settings without a full re-init.
    ; Resets origin, cursor, pen, paper, and clipping window, but does not clear video RAM.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_RESET		equ #BBB1

    ; FW_GRA_MOVE_ABSOLUTE -- [464] move the graphics cursor to an absolute position (no drawing).
    ; In:      DE = X coordinate (signed 16-bit), HL = Y coordinate (signed 16-bit)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_MOVE_ABSOLUTE	equ #BBB4

    ; FW_GRA_MOVE_RELATIVE -- [464] move the graphics cursor by a signed offset (no drawing).
    ; In:      DE = signed X offset, HL = signed Y offset
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_MOVE_RELATIVE	equ #BBB7

    ; FW_GRA_ASK_CURSOR -- [464] return the current graphics cursor position.
    ; In:      none
    ; Out:     DE = X coordinate, HL = Y coordinate (both absolute, relative to current origin)
    ; Destroys: AF, DE, HL
    FW_GRA_ASK_CURSOR	equ #BBBA

    ; FW_GRA_SET_ORIGIN -- [464] set the graphics coordinate origin.
    ; All subsequent coordinate parameters are interpreted relative to this origin.
    ; In:      DE = X origin (absolute pixel), HL = Y origin (absolute pixel)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_SET_ORIGIN	equ #BBBD

    ; FW_GRA_GET_ORIGIN -- [464] return the current graphics origin.
    ; In:      none
    ; Out:     DE = X origin (absolute pixel), HL = Y origin (absolute pixel)
    ; Destroys: AF, DE, HL
    FW_GRA_GET_ORIGIN	equ #BBC0

    ; FW_GRA_WIN_WIDTH -- [464] set the horizontal extent of the graphics clipping window.
    ; In:      DE = left boundary (absolute pixel X), HL = right boundary (absolute pixel X)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_WIN_WIDTH	equ #BBC3

    ; FW_GRA_WIN_HEIGHT -- [464] set the vertical extent of the graphics clipping window.
    ; In:      DE = bottom boundary (absolute pixel Y), HL = top boundary (absolute pixel Y)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_WIN_HEIGHT	equ #BBC6

    ; FW_GRA_GET_W_WIDTH -- [464] return the current graphics clipping window horizontal limits.
    ; In:      none
    ; Out:     DE = left boundary (absolute pixel X), HL = right boundary (absolute pixel X)
    ; Destroys: AF, DE, HL
    FW_GRA_GET_W_WIDTH	equ #BBC9

    ; FW_GRA_GET_W_HEIGHT -- [464] return the current graphics clipping window vertical limits.
    ; In:      none
    ; Out:     DE = bottom boundary (absolute pixel Y), HL = top boundary (absolute pixel Y)
    ; Destroys: AF, DE, HL
    FW_GRA_GET_W_HEIGHT	equ #BBCC

    ; FW_GRA_CLEAR_WINDOW -- [464] clear the graphics clipping window to the current paper colour.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_CLEAR_WINDOW	equ #BBCF

    ; FW_GRA_SET_PEN -- [464] set the graphics foreground (ink) pen for subsequent draw operations.
    ; In:      A = pen index (0-15)
    ; Out:     none
    ; Destroys: AF
    FW_GRA_SET_PEN		equ #BBD2

    ; FW_GRA_GET_PEN -- [464] return the current graphics ink pen index.
    ; In:      none
    ; Out:     A = current graphics ink pen (0-15)
    ; Destroys: AF
    FW_GRA_GET_PEN		equ #BBD5

    ; FW_GRA_SET_PAPER -- [464] set the graphics background (paper) pen.
    ; Used by GRA_CLEAR_WINDOW and as the background colour for GRA_WR_CHAR.
    ; In:      A = pen index (0-15)
    ; Out:     none
    ; Destroys: AF
    FW_GRA_SET_PAPER	equ #BBD8

    ; FW_GRA_GET_PAPER -- [464] return the current graphics paper pen index.
    ; In:      none
    ; Out:     A = current graphics paper pen (0-15)
    ; Destroys: AF
    FW_GRA_GET_PAPER	equ #BBDB

    ; FW_GRA_PLOT_ABSOLUTE -- [464] plot a single pixel at an absolute position in the current ink.
    ; The graphics cursor is moved to the plotted position.
    ; In:      DE = X coordinate (absolute), HL = Y coordinate (absolute)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_PLOT_ABSOLUTE	equ #BBDE

    ; FW_GRA_PLOT_RELATIVE -- [464] plot a single pixel at a position relative to the graphics cursor.
    ; The graphics cursor is moved to the plotted position.
    ; In:      DE = signed X offset from cursor, HL = signed Y offset from cursor
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_PLOT_RELATIVE	equ #BBE1

    ; FW_GRA_TEST_ABSOLUTE -- [464] read the pen index of the pixel at an absolute position.
    ; In:      DE = X coordinate (absolute), HL = Y coordinate (absolute)
    ; Out:     A = pen index at that pixel; Carry set if pixel is within the clipping window
    ; Destroys: AF, BC, DE, HL
    FW_GRA_TEST_ABSOLUTE	equ #BBE4

    ; FW_GRA_TEST_RELATIVE -- [464] read the pen index of the pixel at a relative position.
    ; In:      DE = signed X offset from cursor, HL = signed Y offset from cursor
    ; Out:     A = pen index at that pixel; Carry set if pixel is within the clipping window
    ; Destroys: AF, BC, DE, HL
    FW_GRA_TEST_RELATIVE	equ #BBE7

    ; FW_GRA_LINE_ABSOLUTE -- [464] draw a line from the current graphics cursor to an absolute position.
    ; Uses the current ink pen. The cursor is moved to the end point.
    ; In:      DE = X end coordinate (absolute), HL = Y end coordinate (absolute)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_LINE_ABSOLUTE	equ #BBEA

    ; FW_GRA_LINE_RELATIVE -- [464] draw a line from the current graphics cursor to a relative offset.
    ; Uses the current ink pen. The cursor is moved to the end point.
    ; In:      DE = signed X offset, HL = signed Y offset
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_LINE_RELATIVE	equ #BBED

    ; FW_GRA_WR_CHAR -- [464] render a character glyph at the current graphics cursor position.
    ; Uses the font matrix for the given character code. Does not advance the cursor.
    ; In:      A = character code (0-255)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_GRA_WR_CHAR		equ #BBF0

    ; ============================================================
    ; SCREEN MANAGER  (SCR)
    ; ============================================================
    ;
    ; Controls screen mode, palette, hardware scrolling, and display start address.
    ; Palette colours use hardware indices (0-26); see macros.ga.asm for the full list.
    ;

    ; FW_SCR_INITIALISE -- [464] reset the screen manager to its default power-on state.
    ; Sets mode 1, restores the default 5-colour palette, clears the screen, and
    ; resets the display start address to the default screen base (#C000).
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_INITIALISE	equ #BBF3

    ; FW_SCR_RESET -- [464] reset screen settings without a full re-init.
    ; Restores palette and mode without clearing video RAM or resetting the start address.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_RESET		equ #BBF6

    ; FW_SCR_SET_MODE -- [464] set the screen mode.
    ; Reprograms the Gate Array, clears the screen, and resets the palette to defaults.
    ; In:      A = mode: 0 = 160x200 16col, 1 = 320x200 4col, 2 = 640x200 2col
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_SET_MODE		equ #BBF9

    ; FW_SCR_GET_MODE -- [464] return the current screen mode.
    ; In:      none
    ; Out:     A = current mode (0, 1, or 2)
    ; Destroys: AF
    FW_SCR_GET_MODE		equ #BBFC

    ; FW_SCR_CLEAR -- [464] clear the entire screen to the current paper colour.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_CLEAR		equ #BBFF

    ; FW_SCR_CHAR_LIMITS -- [464] return the number of character columns and rows in the current mode.
    ; In:      none
    ; Out:     B = number of columns, C = number of rows
    ; Destroys: AF, BC
    FW_SCR_CHAR_LIMITS	equ #BC02

    ; FW_SCR_CHAR_POSITION -- [464] convert a text cell position to a screen byte address and bit mask.
    ; In:      H = column (1-based), L = row (1-based)
    ; Out:     HL = screen byte address, C = bit mask for the leftmost pixel of that cell
    ; Destroys: AF, BC, DE, HL
    FW_SCR_CHAR_POSITION	equ #BC05

    ; FW_SCR_DOT_POSITION -- [464] convert a pixel coordinate to a screen byte address and bit mask.
    ; In:      DE = X pixel coordinate, HL = Y pixel coordinate
    ; Out:     HL = screen byte address, C = bit mask for that pixel within the byte
    ; Destroys: AF, BC, DE, HL
    FW_SCR_DOT_POSITION	equ #BC08

    ; FW_SCR_NEXT_BYTE -- [464] advance a screen address one byte to the right.
    ; Handles mode-dependent byte width and the interleaved CPC screen layout.
    ; In:      HL = current screen byte address
    ; Out:     HL = address of next byte to the right; Carry set if wrapped to the next character row
    ; Destroys: AF, HL
    FW_SCR_NEXT_BYTE	equ #BC0B

    ; FW_SCR_PREV_BYTE -- [464] step a screen address one byte to the left.
    ; In:      HL = current screen byte address
    ; Out:     HL = address of previous byte; Carry set if wrapped to the previous character row
    ; Destroys: AF, HL
    FW_SCR_PREV_BYTE	equ #BC0E

    ; FW_SCR_NEXT_LINE -- [464] advance a screen address one scan line down.
    ; Accounts for the CPC interleaved screen layout (8-line banks).
    ; In:      HL = current screen byte address
    ; Out:     HL = address on the next scan line; Carry set if wrapped past the bottom
    ; Destroys: AF, HL
    FW_SCR_NEXT_LINE	equ #BC11

    ; FW_SCR_PREV_LINE -- [464] step a screen address one scan line up.
    ; In:      HL = current screen byte address
    ; Out:     HL = address on the previous scan line; Carry set if wrapped past the top
    ; Destroys: AF, HL
    FW_SCR_PREV_LINE	equ #BC14

    ; FW_SCR_INK_ENCODE -- [464] convert a pen index into the pixel byte pattern for the current mode.
    ; The returned byte has the pen bits replicated into all pixel positions within the byte.
    ; In:      A = pen index (0-15)
    ; Out:     A = encoded screen byte pattern
    ; Destroys: AF, BC, DE, HL
    FW_SCR_INK_ENCODE	equ #BC17

    ; FW_SCR_INK_DECODE -- [464] decode a screen byte back to a pen index.
    ; Extracts the pen index from the first pixel position of the byte.
    ; In:      A = screen byte value
    ; Out:     A = pen index (0-15)
    ; Destroys: AF, BC, DE, HL
    FW_SCR_INK_DECODE	equ #BC1A

    ; FW_SCR_SET_INK -- [464] assign hardware colours to a pen, with optional flash.
    ; To set a solid colour, pass the same hardware colour index in both B and C.
    ; In:      A = pen index (0-15), B = hardware colour 1 (0-26), C = hardware colour 2 (0-26)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_SET_INK		equ #BC1D

    ; FW_SCR_GET_INK -- [464] return the hardware colours assigned to a pen.
    ; In:      A = pen index (0-15)
    ; Out:     B = hardware colour 1 (0-26), C = hardware colour 2 (0-26)
    ; Destroys: AF, BC
    FW_SCR_GET_INK		equ #BC20

    ; FW_SCR_SET_BORDER -- [464] set the border colour, with optional flash.
    ; In:      B = hardware colour 1 (0-26), C = hardware colour 2 (0-26, same as B for solid)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_SET_BORDER	equ #BC23

    ; FW_SCR_GET_BORDER -- [464] return the current border colours.
    ; In:      none
    ; Out:     B = hardware colour 1 (0-26), C = hardware colour 2 (0-26)
    ; Destroys: AF, BC
    FW_SCR_GET_BORDER	equ #BC26

    ; FW_SCR_SET_FLASHING -- [464] set the flash timing for flashing pens.
    ; In:      B = number of 1/50 s frames to display colour 1, C = frames to display colour 2
    ; Out:     none
    ; Destroys: AF, BC
    FW_SCR_SET_FLASHING	equ #BC29

    ; FW_SCR_GET_FLASHING -- [464] return the current flash timing values.
    ; In:      none
    ; Out:     B = colour 1 on-time (frames), C = colour 2 on-time (frames)
    ; Destroys: AF, BC
    FW_SCR_GET_FLASHING	equ #BC2C

    ; FW_SCR_FILL_BOX -- [464] fill a rectangular area of video RAM with an encoded ink pattern.
    ; In:      B = encoded ink byte (from SCR_INK_ENCODE), C = width in bytes,
    ;          D = height in scan lines, HL = top-left screen byte address
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_FILL_BOX		equ #BC2F

    ; FW_SCR_FLOOD_BOX -- [464] XOR-flood a rectangular area using an encoded ink pattern.
    ; Same parameters as SCR_FILL_BOX but XORs the ink into the existing screen data.
    ; In:      B = encoded ink byte, C = width in bytes, D = height in scan lines, HL = top-left address
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_FLOOD_BOX	equ #BC32

    ; FW_SCR_CHAR_INVERT -- [464] XOR-invert one character cell using a pixel mask.
    ; Used internally by the cursor draw/erase routines.
    ; In:      HL = screen address of the top-left byte of the cell, C = pixel mask byte
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_CHAR_INVERT	equ #BC35

    ; FW_SCR_HW_ROLL -- [464] hardware-scroll the screen by adjusting the CRTC display start address.
    ; Does not move video RAM data -- only changes what the CRTC presents to the display.
    ; In:      A = number of character rows to scroll, B = direction: 0 = up, 1 = down
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_HW_ROLL		equ #BC38

    ; FW_SCR_SW_ROLL -- [464] software-scroll the current text window one row by copying video RAM.
    ; In:      B = direction: 0 = up (bottom row cleared), 1 = down (top row cleared)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SCR_SW_ROLL		equ #BC3B

    ; FW_SCR_UNPACK -- [464] unpack a run-length encoded screen data block to video RAM.
    ; Format: alternating count/byte pairs terminated by a zero count byte.
    ; In:      HL = source address (packed data), DE = destination screen address
    ; Out:     HL = address after last source byte read, DE = address after last byte written
    ; Destroys: AF, BC, DE, HL
    FW_SCR_UNPACK		equ #BC3E

    ; FW_SCR_REPACK -- [464] re-pack a region of screen data into run-length encoded format.
    ; In:      HL = source screen address, DE = destination buffer address
    ; Out:     HL and DE advanced past the processed data
    ; Destroys: AF, BC, DE, HL
    FW_SCR_REPACK		equ #BC41

    ; FW_SCR_ACCESS -- [464] set the screen access mode.
    ; In direct mode the firmware does not update its internal screen pointer variables.
    ; In:      A = 0 for normal firmware mode, A = 1 for direct hardware access mode
    ; Out:     none
    ; Destroys: AF
    FW_SCR_ACCESS		equ #BC44

    ; FW_SCR_PIXELS -- [464] return the number of pixels per screen byte in the current mode.
    ; In:      none
    ; Out:     A = pixels per byte (mode 0 = 2, mode 1 = 4, mode 2 = 8)
    ; Destroys: AF
    FW_SCR_PIXELS		equ #BC47

    ; FW_SCR_HORIZONTAL -- [464] return the total horizontal pixel width for the current mode.
    ; In:      none
    ; Out:     HL = pixel width (mode 0 = 160, mode 1 = 320, mode 2 = 640)
    ; Destroys: AF, HL
    FW_SCR_HORIZONTAL	equ #BC4A

    ; FW_SCR_VERTICAL -- [464] return the total vertical pixel height (always 200).
    ; In:      none
    ; Out:     HL = 200
    ; Destroys: AF, HL
    FW_SCR_VERTICAL		equ #BC4D

    ; ============================================================
    ; CASSETTE MANAGER  (CAS)
    ; ============================================================
    ;
    ; Controls the cassette tape interface for reading, writing, and cataloguing.
    ; All I/O routines block the CPU until the tape operation completes.
    ; On machines with a disc interface, the same vectors (#BC77, #BC8C) are
    ; redirected to AMSDOS by the active upper ROM -- see notes under each entry.
    ;

    ; FW_CAS_INITIALISE -- [464] reset the cassette manager to its default state.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_CAS_INITIALISE	equ #BC65

    ; FW_CAS_SET_SPEED -- [464] select between normal and fast cassette baud rates.
    ; In:      A = 0 for normal speed (~1000 baud), A = 1 for fast speed (~2000 baud)
    ; Out:     none
    ; Destroys: AF
    FW_CAS_SET_SPEED	equ #BC68

    ; FW_CAS_NOISY -- [464] enable or disable audio feedback tone during tape operations.
    ; In:      A = 0 for silent operation, A = 1 for audio feedback (beep on each loaded block)
    ; Out:     none
    ; Destroys: AF
    FW_CAS_NOISY		equ #BC6B

    ; FW_CAS_START_MOTOR -- [464] start the cassette motor and save the previous motor state.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC
    FW_CAS_START_MOTOR	equ #BC6E

    ; FW_CAS_STOP_MOTOR -- [464] stop the cassette motor.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC
    FW_CAS_STOP_MOTOR	equ #BC71

    ; FW_CAS_RESTORE_MOTOR -- [464] restore the motor to the state saved by CAS_START_MOTOR.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC
    FW_CAS_RESTORE_MOTOR	equ #BC74

    ; FW_CAS_IN_OPEN -- [464/664+] open a file for reading.
    ; On machines with a disc ROM active, this vector is redirected to AMSDOS disc open.
    ; In:      HL = address of 16-byte filename buffer, DE = address of 64-byte header buffer
    ; Out:     Carry set = success, header buffer filled, A = file type byte;
    ;          Carry clear = file not found or read error
    ; Destroys: AF, BC, DE, HL
    FW_CAS_IN_OPEN		equ #BC77

    ; FW_CAS_IN_CLOSE -- [464] close the currently open input file cleanly.
    ; Stops the motor and verifies the final block checksum.
    ; In:      none
    ; Out:     Carry set = closed cleanly; Carry clear = checksum error on final block
    ; Destroys: AF, BC, DE, HL
    FW_CAS_IN_CLOSE		equ #BC7A

    ; FW_CAS_IN_ABANDON -- [464] abandon the input file without verifying the final block.
    ; Stops the motor. Use after an unrecoverable read error.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_CAS_IN_ABANDON	equ #BC7D

    ; FW_CAS_IN_CHAR -- [464] read one byte from the open input file.
    ; In:      none
    ; Out:     Carry set: A = byte read; Carry clear = EOF or read error
    ; Destroys: AF, BC, DE, HL
    FW_CAS_IN_CHAR		equ #BC80

    ; FW_CAS_IN_DIRECT -- [464] read a block of bytes from the open input file directly to memory.
    ; In:      HL = destination address, DE = number of bytes to read
    ; Out:     Carry set = all bytes read successfully; Carry clear = EOF or error before completion
    ; Destroys: AF, BC, DE, HL
    FW_CAS_IN_DIRECT	equ #BC83

    ; FW_CAS_RETURN -- [464] push one byte back into the input stream (un-get).
    ; The pushed-back byte will be returned by the next CAS_IN_CHAR call.
    ; In:      A = byte to push back
    ; Out:     none
    ; Destroys: AF
    FW_CAS_RETURN		equ #BC86

    ; FW_CAS_TEST_EOF -- [464] test whether the input stream has reached end of file.
    ; In:      none
    ; Out:     Carry set = end of file; Carry clear = more data available
    ; Destroys: AF
    FW_CAS_TEST_EOF		equ #BC89

    ; FW_CAS_OUT_OPEN -- [464/664+] open a file for writing.
    ; On machines with a disc ROM active, this vector is redirected to AMSDOS disc create.
    ; In:      HL = address of 16-byte filename buffer, DE = address of 64-byte header buffer
    ; Out:     Carry set = success; Carry clear = error (e.g. disc full on AMSDOS)
    ; Destroys: AF, BC, DE, HL
    FW_CAS_OUT_OPEN		equ #BC8C

    ; FW_CAS_OUT_CLOSE -- [464] close the output file, writing the final block and EOF marker.
    ; In:      none
    ; Out:     Carry set = closed cleanly; Carry clear = write error on final block
    ; Destroys: AF, BC, DE, HL
    FW_CAS_OUT_CLOSE	equ #BC8F

    ; FW_CAS_OUT_ABANDON -- [464] abandon the output file without writing a terminating block.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_CAS_OUT_ABANDON	equ #BC92

    ; FW_CAS_OUT_CHAR -- [464] write one byte to the open output file.
    ; Bytes are buffered internally and written to tape in 2K blocks.
    ; In:      A = byte to write
    ; Out:     Carry set = success; Carry clear = write error
    ; Destroys: AF, BC, DE, HL
    FW_CAS_OUT_CHAR		equ #BC95

    ; FW_CAS_OUT_DIRECT -- [464] write a block of bytes from memory to the open output file.
    ; In:      HL = source address, DE = number of bytes to write
    ; Out:     Carry set = success; Carry clear = write error
    ; Destroys: AF, BC, DE, HL
    FW_CAS_OUT_DIRECT	equ #BC98

    ; FW_CAS_CATALOG -- [464] read the tape catalogue (list of file headers on tape).
    ; Reads headers sequentially without loading file data. Stops on BREAK or blank tape.
    ; In:      HL = address of buffer to receive directory entries (16 bytes per entry)
    ; Out:     B = number of entries found
    ; Destroys: AF, BC, DE, HL
    FW_CAS_CATALOG		equ #BC9B

    ; ============================================================
    ; SOUND MANAGER  (SND)
    ; ============================================================
    ;
    ; Controls the AY-3-8912 sound chip: tone, noise, envelope, volume.
    ; Sound structs are 6-byte blocks describing channel mask, tone, volume, and envelope.
    ; The firmware mixes up to three channel queues asynchronously via the frame interrupt.
    ;

    ; FW_SND_INITIALISE -- [464] reset the sound manager to its default power-on state.
    ; Silences all three AY channels, flushes all queues, and clears all envelope definitions.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SND_INITIALISE	equ #BCA1

    ; FW_SND_RESET -- [464] reset the sound manager without a full re-init.
    ; Flushes all channel queues and silences the AY but does not clear envelope definitions.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_SND_RESET		equ #BCA4

    ; FW_SND_HOLD -- [464] pause sound output without flushing the queues.
    ; The AY is silenced and queue processing stops until SND_CONTINUE is called.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_SND_HOLD			equ #BCA7

    ; FW_SND_CONTINUE -- [464] resume sound output after SND_HOLD.
    ; Queue processing restarts from where it was paused.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_SND_CONTINUE		equ #BCAA

    ; FW_SND_QUEUE -- [464] add a sound struct to one or more AY channel queues.
    ; The sound struct is a 6-byte block: channel mask, tone period, noise period,
    ; envelope number, volume, and duration (see SOFT968 guide for full format).
    ; In:      HL = address of the 6-byte sound struct
    ; Out:     Carry set = queued successfully; Carry clear = all target queues are full
    ; Destroys: AF, BC, DE, HL
    FW_SND_QUEUE		equ #BCAD

    ; FW_SND_SET_ENV -- [464] define a software volume envelope for use in sound structs.
    ; The envelope is a variable-length data block; up to 15 envelopes can be defined.
    ; In:      A = envelope number (1-15), HL = address of envelope data block
    ; Out:     Carry set = success; Carry clear = invalid envelope number
    ; Destroys: AF, BC, DE, HL
    FW_SND_SET_ENV		equ #BCB0

    ; FW_SND_GET_ENV -- [464] return the address of a previously defined software envelope.
    ; In:      A = envelope number (1-15)
    ; Out:     Carry set: HL = address of envelope data; Carry clear = not defined or invalid number
    ; Destroys: AF, BC, HL
    FW_SND_GET_ENV		equ #BCB3

    ; FW_SND_CHECK -- [464] test whether a channel's sound queue is empty.
    ; In:      A = channel mask (bit 0 = channel A, bit 1 = channel B, bit 2 = channel C)
    ; Out:     Carry set = all selected queues are empty (channel is free);
    ;          Carry clear = at least one selected queue still has pending sounds
    ; Destroys: AF
    FW_SND_CHECK		equ #BCB6

    ; ============================================================
    ; KERNEL / LOW-LEVEL MANAGER  (KL)
    ; ============================================================
    ;
    ; Provides event scheduling, interrupt handling, and memory bank control.
    ; KL_EVENT blocks are 8-byte structures initialised with KL_INIT_EVENT.
    ; Events are dispatched from the frame interrupt (50 Hz) or fast ticker (300 Hz).
    ;

    ; FW_KL_CHOKE_OFF -- [464] disable the event-driven main-loop choke.
    ; Normally BASIC throttles its loop via the choke mechanism. Disabling speeds up
    ; tight polling loops in machine-code programs. Re-enabled at the next BASIC command.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_KL_CHOKE_OFF		equ #BCC3

    ; FW_KL_ROM_WALK -- [464] scan the upper ROM slots and populate the internal ROM table.
    ; Must be called after fitting or swapping ROMs at runtime. Normal boot calls this automatically.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_ROM_WALK		equ #BCC6

    ; FW_KL_INIT_BACK -- [464] initialise the background task co-routine system.
    ; Sets up the internal task queue. Called once during firmware boot; rarely needed directly.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_INIT_BACK		equ #BCC9

    ; FW_KL_LOG_EXT -- [464] register an external (upper) ROM in the kernel ROM table.
    ; In:      B = ROM slot number (0-7), HL = address of the ROM's name string (null-terminated)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_LOG_EXT		equ #BCCC

    ; FW_KL_FIND_COMMAND -- [464] search all registered ROM command tables for a token string.
    ; In:      HL = address of the command token string (uppercase, null-terminated)
    ; Out:     Carry set: A = ROM slot, HL = address of the command's entry in the ROM table;
    ;          Carry clear = token not found in any ROM
    ; Destroys: AF, BC, DE, HL
    FW_KL_FIND_COMMAND	equ #BCCF

    ; FW_KL_NEW_FRAME_FLY -- [464] register a new frame-flyback (50 Hz VSYNC) event.
    ; Initialises and arms the KL_EVENT block so the routine fires every frame.
    ; In:      HL = address of an 8-byte KL_EVENT block (initialised with KL_INIT_EVENT)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_NEW_FRAME_FLY	equ #BCD2

    ; FW_KL_ADD_FRAME_FLY -- [464] add an already-initialised event to the frame-flyback chain.
    ; Unlike KL_NEW_FRAME_FLY, does not reinitialise the block -- use after KL_INIT_EVENT.
    ; In:      HL = address of an initialised 8-byte KL_EVENT block
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_ADD_FRAME_FLY	equ #BCD5

    ; FW_KL_DEL_FRAME_FLY -- [464] remove an event from the frame-flyback chain.
    ; The KL_EVENT block is disarmed and unlinked; the routine will no longer be called.
    ; In:      HL = address of the KL_EVENT block to remove
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_DEL_FRAME_FLY	equ #BCD8

    ; FW_KL_NEW_FAST_TICKER -- [464] register a new fast-ticker (300 Hz) event.
    ; The routine in the KL_EVENT block is called 6* per frame (every ~3.3 ms).
    ; In:      HL = address of an 8-byte KL_EVENT block (initialised with KL_INIT_EVENT)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_NEW_FAST_TICKER	equ #BCDB

    ; FW_KL_ADD_FAST_TICKER -- [464] add an initialised event to the fast-ticker chain.
    ; In:      HL = address of an initialised 8-byte KL_EVENT block
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_ADD_FAST_TICKER	equ #BCDE

    ; FW_KL_DEL_FAST_TICKER -- [464] remove an event from the fast-ticker chain.
    ; In:      HL = address of the KL_EVENT block to remove
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_DEL_FAST_TICKER	equ #BCE1

    ; FW_KL_ADD_TICKER -- [464] add a general-purpose periodic ticker event.
    ; The routine fires every B frames (at 50 Hz resolution). B=1 fires every frame.
    ; In:      B = period in frames (1-255), HL = address of an initialised KL_EVENT block
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_ADD_TICKER		equ #BCE4

    ; FW_KL_DEL_TICKER -- [464] remove a general-purpose ticker event from the ticker chain.
    ; In:      HL = address of the KL_EVENT block to remove
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_DEL_TICKER		equ #BCE7

    ; FW_KL_INIT_EVENT -- [464] initialise a KL_EVENT block.
    ; Must be called before passing the block to any ADD/NEW event routine.
    ; In:      HL = address of an 8-byte KL_EVENT block, BC = address of the handler routine,
    ;          DE = value passed to the handler in DE when it fires
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_INIT_EVENT		equ #BCEA

    ; FW_KL_SET_EVENT -- [464] arm a previously initialised KL_EVENT block.
    ; The event will fire the next time its chain is processed.
    ; In:      HL = address of an initialised KL_EVENT block
    ; Out:     none
    ; Destroys: AF, HL
    FW_KL_SET_EVENT		equ #BCED

    ; FW_KL_RESET_EVENT -- [464] disarm a KL_EVENT block without removing it from the chain.
    ; The block remains linked but the handler will not be called until re-armed with KL_SET_EVENT.
    ; In:      HL = address of the KL_EVENT block
    ; Out:     none
    ; Destroys: AF, HL
    FW_KL_RESET_EVENT	equ #BCF0

    ; FW_KL_EVENT_DISABLE -- [464] globally suspend all event processing.
    ; No events in any chain will fire until KL_EVENT_ENABLE is called.
    ; Calls can be nested -- an equal number of ENABLE calls must follow.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_KL_EVENT_DISABLE	equ #BCF3

    ; FW_KL_EVENT_ENABLE -- [464] re-enable global event processing after KL_EVENT_DISABLE.
    ; In:      none
    ; Out:     none
    ; Destroys: AF
    FW_KL_EVENT_ENABLE	equ #BCF6

    ; FW_KL_DISARM_EVENT -- [464] disarm and unlink a KL_EVENT block in a single operation.
    ; Combines KL_RESET_EVENT and removal from its chain. Safe to call at any time.
    ; In:      HL = address of the KL_EVENT block
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_KL_DISARM_EVENT	equ #BCF9

    ; FW_KL_TIME_PLEASE -- [464] return the system elapsed time counter. Undocumented.
    ; The counter is incremented every frame interrupt (50 Hz).
    ; In:      none
    ; Out:     DE = elapsed seconds, HL = fractional ticks within the current second
    ; Destroys: AF, DE, HL
    FW_KL_TIME_PLEASE	equ #BCFB

    ; FW_KL_TIME_SET -- [464] set the system elapsed time counter. Undocumented.
    ; In:      DE = seconds to set, HL = ticks within the current second
    ; Out:     none
    ; Destroys: AF, DE, HL
    FW_KL_TIME_SET		equ #BCFE

    ; ============================================================
    ; MACHINE PACK  (MC)  -- direct hardware control
    ; ============================================================
    ;
    ; Low-level routines that directly program CPC hardware registers.
    ; Calling these bypasses the firmware managers -- use with care.
    ; None of the MC routines preserve any registers beyond what is noted.
    ;

    ; FW_MC_BOOT_PROGRAM -- [464] perform a warm restart of BASIC.
    ; Equivalent to pressing the reset button. Returns to the BASIC ready prompt.
    ; In:      none
    ; Out:     does not return
    ; Destroys: all
    FW_MC_BOOT_PROGRAM	equ #BD00

    ; FW_MC_START_PROGRAM -- [464] execute machine code at a given address.
    ; Sets the stack pointer to the provided value and jumps to the target address.
    ; In:      HL = execution address, SP = desired stack pointer value
    ; Out:     does not return to caller (runs the program at HL)
    ; Destroys: all
    FW_MC_START_PROGRAM	equ #BD03

    ; FW_MC_WAIT_FLYBACK -- [464] spin-wait for the next vertical flyback (VSYNC) pulse.
    ; Blocks until the CRTC asserts VSYNC on PPI port B bit 0. Useful for tear-free updates.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, B
    FW_MC_WAIT_FLYBACK	equ #BD06

    ; FW_MC_SET_MODE -- [464] set the screen mode by writing directly to the Gate Array.
    ; Does not clear the screen or reset the palette -- for a managed mode change use SCR_SET_MODE.
    ; In:      A = mode: 0 = 160x200 16col, 1 = 320x200 4col, 2 = 640x200 2col
    ; Out:     none
    ; Destroys: AF, BC
    FW_MC_SET_MODE		equ #BD09

    ; FW_MC_SCREEN_OFFSET -- [464] set the CRTC display start address to point to a screen base.
    ; The address must be aligned to a 16K boundary (i.e. #0000, #4000, #8000, or #C000).
    ; In:      HL = screen base address
    ; Out:     none
    ; Destroys: AF, BC, HL
    FW_MC_SCREEN_OFFSET	equ #BD0C

    ; FW_MC_CLEAR_INKS -- [464] reset all Gate Array palette entries to their power-on defaults.
    ; Restores the default 5-colour mode 1 palette (border + pens 0-15).
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_MC_CLEAR_INKS	equ #BD0F

    ; FW_MC_SET_INKS -- [464] write a complete 17-entry palette to the Gate Array.
    ; The table must contain 17 bytes: pen 0-15 hardware colours followed by the border colour.
    ; In:      HL = address of 17-byte palette table (hardware colour indices 0-26)
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_MC_SET_INKS		equ #BD12

    ; FW_MC_RESET_PRINTER -- [464] reset the parallel printer port (strobe and busy lines).
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC
    FW_MC_RESET_PRINTER	equ #BD15

    ; FW_MC_PRINT_CHAR -- [464] send one character to the parallel printer with handshaking.
    ; Waits for the printer to be ready (BUSY low) then pulses the STROBE line.
    ; In:      A = character byte to send
    ; Out:     Carry set = character accepted; Carry clear = printer timeout or not ready
    ; Destroys: AF, BC
    FW_MC_PRINT_CHAR	equ #BD18

    ; FW_MC_BUSY_PRINTER -- [464] test whether the parallel printer is ready to accept data.
    ; In:      none
    ; Out:     Carry set = printer is ready (BUSY line low); Carry clear = printer is busy
    ; Destroys: AF, B
    FW_MC_BUSY_PRINTER	equ #BD1B

    ; FW_MC_SEND_PRINTER -- [464] send a byte to the printer data port with no handshaking.
    ; Writes directly to the PPI output port. Use MC_PRINT_CHAR for safe handshaked output.
    ; In:      A = byte to send to the data port
    ; Out:     none
    ; Destroys: AF, BC
    FW_MC_SEND_PRINTER	equ #BD1E
    ; #BD21 -- #BD2A: MC internal routines (not for direct use)

    ; ============================================================
    ; DISC MANAGER  (DISC / AMSDOS)  -- 664 and above
    ; ============================================================
    ;
    ; Low-level disc sector routines present only when the AMSDOS disc ROM is active.
    ; On a 464 without a disc interface these addresses are undefined -- do not call them.
    ; All disc routines should be invoked via RST #18 (FW_FAR_CALL) to be ROM-safe,
    ; unless you have explicitly selected the disc ROM via KL_ROM_SELECT.
    ;

    ; FW_DISC_INITIALISE -- [664+] reset the disc manager to its default state.
    ; Resets the FDC and clears the internal drive state tables.
    ; In:      none
    ; Out:     none
    ; Destroys: AF, BC, DE, HL
    FW_DISC_INITIALISE	equ #BD2D

    ; FW_DISC_READ -- [664+] read one or more sectors from disc into memory.
    ; In:      A = drive (0 = A, 1 = B), B = track number, C = sector number (1-based),
    ;          D = number of sectors to read, HL = destination buffer address
    ; Out:     Carry set = success; Carry clear = read error (A = FDC error code)
    ; Destroys: AF, BC, DE, HL
    FW_DISC_READ		equ #BD2F

    ; FW_DISC_WRITE -- [664+] write one or more sectors from memory to disc.
    ; In:      A = drive (0 = A, 1 = B), B = track number, C = sector number (1-based),
    ;          D = number of sectors to write, HL = source buffer address
    ; Out:     Carry set = success; Carry clear = write error (A = FDC error code)
    ; Destroys: AF, BC, DE, HL
    FW_DISC_WRITE		equ #BD32

    ; FW_DISC_VERIFY -- [664+] verify that disc sectors match data in memory.
    ; Reads sectors and compares against the buffer; does not modify memory.
    ; In:      A = drive (0 = A, 1 = B), B = track number, C = sector number (1-based),
    ;          D = number of sectors to verify, HL = reference buffer address
    ; Out:     Carry set = verify error; Carry clear = all sectors verified correctly
    ; Destroys: AF, BC, DE, HL
    FW_DISC_VERIFY		equ #BD35

    ; FW_DISC_FORMAT -- [664+] format a single track on disc.
    ; The sector info table provides the sector ID, size, and ordering for the track.
    ; In:      A = drive (0 = A, 1 = B), B = track number,
    ;          HL = address of sector info table (4 bytes per sector: C, H, R, N)
    ; Out:     Carry set = success; Carry clear = format error (A = FDC error code)
    ; Destroys: AF, BC, DE, HL
    FW_DISC_FORMAT		equ #BD38

    ; FW_DISC_SELECT -- [664+] select the active disc drive.
    ; In:      A = drive number: 0 = drive A, 1 = drive B
    ; Out:     none
    ; Destroys: AF, BC
    FW_DISC_SELECT		equ #BD3B

    ; ============================================================
    ; KERNEL EXTENSIONS  -- 6128 and above
    ; ============================================================
    ;
    ; Additional kernel entry points introduced on the 6128.
    ; These provide RAM bank switching and ROM selection for multi-bank code.
    ; Present on 6128 and Plus only; not available on 464 or 664.
    ;

    ; FW_KL_BANK_SWITCH -- [6128+] execute a far call in a different RAM bank configuration.
    ; Sets the GA RAM bank register to the given config, calls the address, then restores.
    ; Requires: called from RAM (not from a ROM page being switched out).
    ; In:      A = GA RAM bank configuration byte (see GA_CMD_RAM_BANK equs),
    ;          HL = address to call within the target bank
    ; Out:     as per the called routine
    ; Destroys: depends on the called routine; bank configuration is restored on return
    FW_KL_BANK_SWITCH	equ #BD3E

    ; FW_KL_ROM_SELECT -- [6128+] select an upper ROM by slot number.
    ; Writes the ROM number to the GA ROM select register.
    ; In:      A = ROM slot number (0-7)
    ; Out:     none
    ; Destroys: AF, BC
    FW_KL_ROM_SELECT	equ #BD41

    ; FW_KL_CURR_SELECTION -- [6128+] return the currently selected upper ROM number.
    ; In:      none
    ; Out:     A = current upper ROM slot number (0-7)
    ; Destroys: AF
    FW_KL_CURR_SELECTION	equ #BD44

    ; ============================================================
    ; RESTART VECTORS  (RST instructions)
    ; ============================================================
    ;
    ; Standard firmware RST targets. RST is a 1-byte opcode (11 T-states) vs
    ; CALL at 3 bytes (17 T-states) -- use RST for frequently-called ROM services.
    ;

    ; FW_LOW_JUMP -- RST #00 -- hard reset; equivalent to power-on.
    ; Jumps to address #0000 which triggers the full firmware cold-start sequence.
    ; In:      none
    ; Out:     does not return
    FW_LOW_JUMP		equ #0000

    ; FW_KL_LOW_PCHL -- RST #08 -- indirect jump through HL (JP (HL)).
    ; Faster than a full CALL when the target address is already in HL.
    ; In:      HL = address to jump to
    ; Out:     as per the routine at HL (does not return to RST call site)
    FW_KL_LOW_PCHL	equ #0008

    ; FW_SIDE_CALL -- RST #10 -- call a routine in a specific upper ROM bank.
    ; The firmware selects the ROM, calls the entry point, then restores the previous ROM.
    ; In:      IYH = ROM slot number, IYL = entry point offset within the ROM's jump table
    ; Out:     as per the called ROM routine
    ; Destroys: depends on the called ROM routine
    FW_SIDE_CALL	equ #0010

    ; FW_FAR_CALL -- RST #18 -- ROM-safe indirect call via IY.
    ; Selects the appropriate ROM, calls the address, and restores state on return.
    ; Use this instead of a direct CALL when invoking disc or other upper ROM routines.
    ; In:      IY = target address (the firmware resolves the ROM from the address range)
    ; Out:     as per the called routine
    ; Destroys: depends on the called routine
    FW_FAR_CALL		equ #0018

    ; FW_RAM_LAM -- RST #20 -- LD A,(HL) with automatic RAM bank switching.
    ; Used internally by ROM routines that need to read from banked RAM transparently.
    ; In:      HL = address to read (may be in any RAM bank)
    ; Out:     A = byte read from (HL)
    ; Destroys: AF
    FW_RAM_LAM		equ #0020

    ; FW_KL_FAR_ICALL -- RST #28 -- indirect far call; target address is read from (HL).
    ; Reads a 2-byte address from the location pointed to by HL, then performs a far call to it.
    ; In:      HL = address of a 2-byte pointer to the routine to call
    ; Out:     as per the called routine
    ; Destroys: depends on the called routine
    FW_KL_FAR_ICALL	equ #0028
endif
