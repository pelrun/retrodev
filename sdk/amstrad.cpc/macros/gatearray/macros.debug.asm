;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC Gate Array debug macros.
;
; Purpose: provide debug-colour and border-flash instrumentation macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

ifndef __macros_debug_asm__
__macros_debug_asm__ equ 1
    ; Gate Array debug-colour macros for the Amstrad CPC.
    ;
    ; This file provides border-flash debug aids that depend on the Gate Array
    ; command protocol. Include this file instead of macros.ga.asm when you
    ; need the debug macros -- it pulls in macros.ga.asm automatically.
    ;
    ; All macros in this file assemble to zero bytes unless the corresponding
    ; guard symbol is defined before this file is included:
    ;
    ;   DebugInterrupt  equ 1   -- enables GA_DebugInterrupt
    ;   DebugProcesses  equ 1   -- enables GA_DebugProcess
    ;
    include "macros.ga.asm"


    ; GA_DebugInterrupt -- flash the border colour at interrupt entry (debug aid).
    ;
    ; Expands to a GA_SetBorder-equivalent write only when DebugInterrupt is defined.
    ; Zero cost when DebugInterrupt is not defined (assembles to nothing).
    ; Intended to be placed at the top of an interrupt service routine to make the
    ; interrupt firing and its duration visible on a monitor or through a logic analyser.
    ;
    ; Destroys (when DebugInterrupt is defined): B, C
    ;
    ; Speed (when enabled): 41 T -- identical to GA_SetBorder (constant, no branching).
    ;
    macro GA_DebugInterrupt Color
        ifdef DebugInterrupt
            ld bc,GA_PORT_CMD+GA_PEN_BORDER		; B=#7f (GA port), C=GA_PEN_BORDER
            out (c),c							; send pen select to Gate Array
            ld c,GA_CMD_COLOR_SET|{Color}		; C = GA colour command (#40 | hw colour index)
            out (c),c							; send colour command to Gate Array
        endif
    endm


    ; GA_DebugProcess -- flash the border colour at a process/task switch point (debug aid).
    ;
    ; Expands to a GA_SetBorder-equivalent write only when DebugProcesses is defined.
    ; Zero cost when DebugProcesses is not defined (assembles to nothing).
    ; Intended to be placed at the entry of each process/task to give each one a distinct
    ; border colour, making scheduling behaviour visible on the monitor.
    ;
    ; Destroys (when DebugProcesses is defined): B, C
    ;
    ; Speed (when enabled): 41 T -- identical to GA_SetBorder (constant, no branching).
    ;
    macro GA_DebugProcess Color
        ifdef DebugProcesses
            ld bc,GA_PORT_CMD+GA_PEN_BORDER		; B=#7f (GA port), C=GA_PEN_BORDER
            out (c),c							; send pen select to Gate Array
            ld c,GA_CMD_COLOR_SET|{Color}		; C = GA colour command (#40 | hw colour index)
            out (c),c							; send colour command to Gate Array
        endif
    endm
endif
