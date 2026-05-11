// ---------------------------------------------------------------------------
//
// Retrodev Gui
//
// Z80 Assembler language definition and parser for TextEditor.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#include <imgui.text.editor.h>
#include <ctre.hpp>
#include <cstdlib>
#include <deque>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "lang.asm.z80.h"

namespace ImGui {
	namespace TextEditorLangs {
		namespace Z80Asm {
			using ::RetrodevGui::Z80TimingType;
			using ::RetrodevGui::Z80AsmLanguage;
			//
			// Z80 Assembler directives (code/data/assembler features)
			// Based on RASM assembler documentation
			//
			const char* const kZ80Directives[] = {"ALIGN",	  "ASSERT",	   "BANK",	   "BANKSET",  "BUILDSNA", "BUILDCPR", "BUILDSNAPSHOT", "BUILDTAPE",   "BYTE",
												  "CHARSET",  "CLOSEBIN",  "CODE",	   "CONFINE",  "CRTCTYPE", "CRUNCH",   "CRUNCHBANK",	"DB",		   "DD",
												  "DEFB",	  "DEFC",	   "DEFD",	   "DEFF",	   "DEFI",	   "DEFL",	   "DEFM",			"DEFP",		   "DEFS",
												  "DEFW",	  "DL",		   "DM",	   "DS",	   "DW",	   "END",	   "ENDORG",		"ENDRELOCATE", "ENT",
												  "EQU",	  "ERROR",	   "EXPORT",   "FAIL",	   "FIELD",	   "GATE",	   "GATEARRAYBANK", "GATEARRAY",   "HESITANT",
												  "GRAB",	  "INCAPU",	   "INCBIN",   "INCEXO",   "INCL",	   "INCL48",   "INCL49",		"INCLZ4",	   "INCLZSA1",
												  "INCLZSA2", "INCLUDE",   "INCSNA",   "INCZX0",   "INCZX0B",  "INCZX7",   "INSERT",		"LABEL",	   "LET",
												  "LIMIT",	  "LIST",	   "LZ4",	   "LZ48",	   "LZ49",	   "LZ7",	   "LZAPU",			"LZCLOSE",	   "LZEXO",
												  "LZX0",	  "LZX7",	   "LZSA1",	   "LZSA2",	   "MAP",	   "MEMSPACE", "NOCODE",		"NOEXPORT",	   "NOLIST",
												  "NOPSPLIT", "ORG",	   "OUTPUT",   "PAGE",	   "PAGESET",  "PHASE",	   "PRINT",			"PROTECT",	   "PROCEDURE",
												  "PROC",	  "RAMPRESET", "READ",	   "RELOCATE", "RESTORE",  "RUN",	   "RUNTEST",		"SAVE",		   "SAVECPR",
												  "SAVEHFE",  "SAVESNA",   "SAVETAPE", "SECTION",  "SETCPC",   "SETCRTC",  "SETNEXTBANK",	"SETCPCOLD",   "SETGATEARRAY",
												  "SETSTACK", "SKIP",	   "SLOT",	   "SNAPSHOT", "SNAINIT",  "SNARESET", "SNASETSQOP",	"STOP",		   "SUMMEM",
												  "TAG",	  "TAPE",	   "TICKER",   "TRACE",	   "UNDEF",	   "UNGRAB",   "VARIABLE",		"VOID",		   "WARN",
												  "WARNING",  "WRITE",	   "XORMEM",   "ZX0",	   "ZX7"};
			//
			// Z80 preprocessor directives (conditional/structural/macro flow)
			//
			const char* const kZ80PreprocessorDirectives[] = {"ELSE",	"ELSEIF", "ENDIF",	  "IF",		   "IFDEF",		"IFNOT",	 "IFNDEF", "IFUSED",	"IFNUSED",		"MACRO",
															  "MEND",	"ENDM",	  "ENDMACRO", "MODULE",	   "ENDMODULE", "PROCEDURE", "PROC",   "ENDPROC",	"ENDPROCEDURE", "ENDP",
															  "REPEAT", "REPT",	  "ENDREPT",  "ENDREPEAT", "REND",		"UNTIL",	 "STRUCT", "ENDSTRUCT", "WHILE",		"WEND"};
			//
			// Directive descriptions injected as static codelens entries for autocomplete and hover.
			// Each entry is: { name, syntax, description }
			//
			struct DirectiveDef {
				const char* name;
				const char* syntax;
				const char* description;
			};
			//
			// Assembler directives — data, memory and file output
			//
			static const DirectiveDef kZ80AssemblerDirectiveDefs[] = {
				{"ORG", "ORG logical[,physical]",
				 "Set the assembly address.\n"
				 "  logical  — controls label values\n"
				 "  physical — controls where bytes are written (optional)\n"
				 "\n"
				 "Examples:\n"
				 "  ORG #8000              ; logical = physical = #8000\n"
				 "  ORG #8000,#4000        ; labels from #8000, bytes at #4000\n"
				 "  ORG $                  ; re-sync logical = physical\n"
				 "\n"
				 "Use ENDORG to return to logical=physical addressing after a physical override."},
				{"ALIGN", "ALIGN limit[,fill]",
				 "Advance the address counter to the next multiple of limit.\n"
				 "  fill — optional byte value to fill the gap (default: no bytes emitted)\n"
				 "\n"
				 "Examples:\n"
				 "  ALIGN 256             ; align to next 256-byte page\n"
				 "  ALIGN 4,0            ; align to 4-byte boundary, fill gap with 0"},
				{"SAVE", "SAVE 'file',start,size[,format[,'extra']]",
				 "Export binary data to a file. Supports multiple output formats.\n"
				 "\n"
				 "Formats:\n"
				 "  (none)               raw binary\n"
				 "  AMSDOS               with AMSDOS header (Amstrad CPC)\n"
				 "  HOBETA               HOBETA header (ZX Spectrum)\n"
				 "  TAPE,'tape.cdt'      CDT/TZX tape format\n"
				 "  DSK,'floppy.dsk'     file inside an EDSK image\n"
				 "\n"
				 "Examples:\n"
				 "  SAVE 'game.bin',#4000,#4000               ; raw binary\n"
				 "  SAVE 'game.bin',#4000,#4000,AMSDOS        ; AMSDOS header\n"
				 "  SAVE 'game.bin',#4000,$-#4000,TAPE,'tape.cdt'\n"
				 "\n"
				 "Convert raw binary to AMSDOS:\n"
				 "  ORG #100 : RUN #100\n"
				 "  INCBIN 'raw.bin'\n"
				 "  SAVE 'out.bin',#100,$-#100,AMSDOS\n"
				 "\n"
				 "Cartridge creation:\n"
				 "  BUILDCPR\n"
				 "  BANK 0 : INCBIN 'bank0.bin'\n"
				 "  BANK 1 : INCBIN 'bank1.bin'\n"
				 "  ; ... up to BANK 31\n"
				 "\n"
				 "If the DSK does not exist it is created in DATA format.\n"
				 "Use -eo command-line option to overwrite existing files on disk."},
				{"INCBIN", "INCBIN 'file'[,offset[,length]][,flags...]",
				 "Import binary data into the assembled output.\n"
				 "\n"
				 "Flags (order-independent, combinable):\n"
				 "  EXISTS       only load if file exists\n"
				 "  OFF          read into memory without overwrite check\n"
				 "  SKIPHEADER   skip first 128 bytes if AMSDOS header found\n"
				 "  REVERT       read data in reverse order\n"
				 "  REMAP,n      column remapping (n = column count)\n"
				 "  VTILES,n     vertical tile re-ordering (n = lines per tile)\n"
				 "  ITILES,w     interleaved tile layout\n"
				 "  GTILES,w     grouped tile layout\n"
				 "\n"
				 "WAV audio import:\n"
				 "  INCBIN 'sound.wav',SMP        ; raw sample\n"
				 "  INCBIN 'sound.wav',SM2        ; 2-bit sample\n"
				 "  INCBIN 'sound.wav',SM4        ; 4-bit sample\n"
				 "  INCBIN 'sound.wav',DMA,preamp,options,...\n"
				 "\n"
				 "Examples:\n"
				 "  INCBIN 'data.bin'\n"
				 "  INCBIN 'data.bin',#10,#100    ; 256 bytes from offset 16\n"
				 "  INCBIN 'data.bin',0,0,EXISTS  ; only if file exists"},
				{"INCLUDE", "INCLUDE 'file.asm'",
				 "Insert an assembly source file at the current location.\n"
				 "Path is relative to the including file.\n"
				 "Use absolute paths to anchor to the source root.\n"
				 "No recursion limit — guard against double-inclusion with IFDEF:\n"
				 "\n"
				 "  IFNDEF __MYHEADER__\n"
				 "  __MYHEADER__ = 1\n"
				 "    ; ... code ...\n"
				 "  ENDIF"},
				{"DEFB", "DEFB expr[,expr,...]",
				 "Emit one or more 8-bit byte values. Aliases: DB, DM, DEFM, BYTE.\n"
				 "\n"
				 "Examples:\n"
				 "  DEFB #FF,#00,42\n"
				 "  DEFB 'H','i',0          ; ASCII string, null-terminated\n"
				 "  DEFB 'string\\xFF'       ; \\xFF inserts hex byte inline\n"
				 "  DEFB 'r'-'a'+'A'        ; expression as byte"},
				{"DEFW", "DEFW expr[,expr,...]",
				 "Emit one or more 16-bit word values (little-endian). Alias: DW.\n"
				 "\n"
				 "Examples:\n"
				 "  DEFW #1234,my_label\n"
				 "  DEFW $-start            ; word holding a computed size"},
				{"DEFI", "DEFI expr[,expr,...]",
				 "Emit one or more 32-bit double-word values (little-endian). Aliases: DI, DD, DL.\n"
				 "\n"
				 "Example:\n"
				 "  DEFI #DEADBEEF"},
				{"DEFR", "DEFR value",
				 "Emit a 32-bit IEEE 754 floating-point value. Alias: DR.\n"
				 "\n"
				 "Example:\n"
				 "  DEFR 3.14159"},
				{"DEFS", "DEFS count[,fill]",
				 "Reserve count bytes in the output, filled with 0 by default. Alias: DS.\n"
				 "\n"
				 "Examples:\n"
				 "  DEFS 256           ; reserve 256 zero bytes\n"
				 "  DEFS 64,#FF        ; reserve 64 bytes filled with #FF"},
				{"DB", "DB expr[,expr,...]", "Emit 8-bit byte values. Alias of DEFB.\nSee DEFB for full documentation and examples."},
				{"DW", "DW expr[,expr,...]", "Emit 16-bit word values (little-endian). Alias of DEFW.\nSee DEFW for full documentation and examples."},
				{"DD", "DD expr[,expr,...]", "Emit 32-bit double-word values. Alias of DEFI.\nSee DEFI for full documentation and examples."},
				{"DM", "DM expr[,expr,...]", "Emit byte values. Alias of DEFB/DEFM.\nSee DEFB for full documentation and examples."},
				{"DS", "DS count[,fill]", "Reserve bytes, optionally filled. Alias of DEFS.\nSee DEFS for full documentation and examples."},
				{"DL", "DL expr[,expr,...]", "Emit 32-bit values. Alias of DEFI.\nSee DEFI for full documentation and examples."},
				{"BYTE", "BYTE expr[,expr,...]", "Emit 8-bit byte values. Alias of DEFB.\nSee DEFB for full documentation and examples."},
				{"STR", "STR 'string'[,'string',...]",
				 "Emit string bytes, ORing #80 onto the last byte of each string.\n"
				 "This is the classic Amstrad CPC string termination convention.\n"
				 "\n"
				 "Examples:\n"
				 "  STR 'hello'          ; emits h,e,l,l,'o'|#80\n"
				 "  STR 'one','two'      ; two strings, each with high-bit set on last byte"},
				{"EQU", "name EQU expr",
				 "Declare a constant alias. Cannot be redefined after declaration.\n"
				 "\n"
				 "Examples:\n"
				 "  SCREEN_BASE EQU #C000\n"
				 "  MAX_SPRITES EQU 8\n"
				 "\n"
				 "For reassignable values use dynamic variables (name=value).\n"
				 "Alias: DEFC."},
				{"DEFC", "name DEFC expr", "Declare a constant alias. Alias of EQU. Cannot be redefined after declaration.\nSee EQU for full documentation."},
				{"DEFL", "name DEFL expr",
				 "Declare a dynamic variable alias (can be reassigned at any time).\n"
				 "\n"
				 "Example:\n"
				 "  counter DEFL 0\n"
				 "  counter DEFL counter+1"},
				{"DEFP", "DEFP expr[,expr,...]", "Emit packed values."},
				{"LIMIT", "LIMIT address",
				 "Set an upper boundary for the assembly address.\n"
				 "RASM emits an error if the assembled code exceeds this address.\n"
				 "\n"
				 "Example:\n"
				 "  ORG #8000\n"
				 "  LIMIT #C000          ; code must not exceed #C000"},
				{"PROTECT", "PROTECT start,end",
				 "Mark a memory region as write-protected.\n"
				 "RASM emits an error if any data is assembled into the protected range.\n"
				 "\n"
				 "Example:\n"
				 "  PROTECT #0000,#3FFF  ; protect lower 16K ROM area"},
				{"CONFINE", "CONFINE value",
				 "Ensure that the following data block fits within a single 256-byte page.\n"
				 "If padding is needed to align, a warning is emitted with the byte count.\n"
				 "\n"
				 "Example:\n"
				 "  CONFINE 256          ; next data block must fit in current 256-byte page"},
				{"SUMMEM", "SUMMEM start,end",
				 "Emit a 1-byte arithmetic checksum of the byte range start..end.\n"
				 "Does not work inside crunched code sections.\n"
				 "\n"
				 "Example:\n"
				 "  SUMMEM #4000,#8000   ; sum all bytes in that range, emit one byte"},
				{"XORMEM", "XORMEM start,end",
				 "Emit a 1-byte XOR checksum of the byte range start..end.\n"
				 "Does not work inside crunched code sections.\n"
				 "\n"
				 "Example:\n"
				 "  XORMEM #0000,#1000   ; XOR all bytes in ROM, write result here"},
				{"SUM16", "SUM16 start,end",
				 "Emit a 2-byte (word) unsigned checksum of the byte range start..end.\n"
				 "Does not work inside crunched code sections.\n"
				 "\n"
				 "Example:\n"
				 "  SUM16 #4000,#8000    ; 16-bit sum, emits 2 bytes"},
				{"NOCODE", "NOCODE",
				 "Stop writing bytes to the output file.\n"
				 "The address counter still advances normally.\n"
				 "Resume writing with CODE or CODE SKIP.\n"
				 "\n"
				 "See also: CODE"},
				{"CODE", "CODE [SKIP]",
				 "Resume normal byte-write mode after NOCODE.\n"
				 "  CODE       — resume writing at the current address\n"
				 "  CODE SKIP  — resume at the address reached during NOCODE (skips the gap)\n"
				 "\n"
				 "See also: NOCODE"},
				{"RUN", "RUN address[,ga_config]",
				 "Set the entry point (PC) used in snapshot and AMSDOS exports,\n"
				 "and the start address in DSK file headers.\n"
				 "\n"
				 "Example:\n"
				 "  ORG #4000\n"
				 "  RUN #4000\n"
				 "\n"
				 "Alias: ENT."},
				{"BANK", "BANK n",
				 "Switch RASM to memory bank n and activate cartridge mode.\n"
				 "Banks 0-31 for standard cartridge; more with BUILDCPR EXTENDED.\n"
				 "\n"
				 "Example:\n"
				 "  BUILDCPR\n"
				 "  BANK 0 : INCBIN 'bank0.bin'\n"
				 "  BANK 1 : INCBIN 'bank1.bin'\n"
				 "\n"
				 "See also: BUILDCPR, BANKSET"},
				{"BANKSET", "BANKSET n", "Select a 64K bank set for BUILDSNA snapshot mode.\nSee also: BUILDSNA, BANK"},
				{"BUILDCPR", "BUILDCPR [EXTENDED] [SYMBOLS] ['filename']",
				 "Enable cartridge build mode (banks 0-31, or more with EXTENDED).\n"
				 "  EXTENDED  — allow more than 32 banks\n"
				 "  SYMBOLS   — export symbol table\n"
				 "\n"
				 "Example:\n"
				 "  BUILDCPR\n"
				 "  BANK 0 : INCBIN 'bank0.bin'\n"
				 "  BANK 1 : INCBIN 'bank1.bin'\n"
				 "\n"
				 "See also: BANK, SAVECPR, CPRINIT"},
				{"BUILDSNA", "BUILDSNA [V2] ['filename']",
				 "Enable snapshot build mode.\n"
				 "  V2 — generate a version-2 snapshot file\n"
				 "Supports 16K banks 0-259 or BANKSET 64K pages.\n"
				 "\n"
				 "See also: BANKSET, SAVESNA, SNAPINIT"},
				{"BUILDTAPE", "BUILDTAPE ['filename']", "Enable tape CDT output mode.\nSee also: SAVETAPE"},
				{"BUILDZX", "BUILDZX", "Enable ZX Spectrum snapshot mode (banks 0-7)."},
				{"BUILDROM", "BUILDROM [concat]", "Enable ROM creation mode."},
				{"BUILDOBJ", "BUILDOBJ", "Enable binary object output mode (link-editor prototype, WIP)."},
				{"SNAPINIT", "SNAPINIT 'snapshot_file'", "Pre-load memory from a snapshot file before assembly begins.\nAlias: SNAINIT.\nSee also: BUILDSNA"},
				{"CPRINIT", "CPRINIT 'cartridge_file'", "Pre-load memory from a cartridge file before assembly begins.\nSee also: BUILDCPR"},
				{"SNAINIT", "SNAINIT 'snapshot_file'", "Pre-load memory from a snapshot file before assembly. Alias of SNAPINIT."},
				{"SAVECPR", "SAVECPR ['filename']", "Save the assembled cartridge image to file.\nSee also: BUILDCPR"},
				{"SAVESNA", "SAVESNA ['filename']", "Save the assembled snapshot image to file.\nSee also: BUILDSNA"},
				{"SAVETAPE", "SAVETAPE ['filename']", "Save the assembled tape CDT image to file.\nSee also: BUILDTAPE"},
				{"SAVEHFE", "SAVEHFE ['filename']", "Save the assembled HFE floppy image to file."},
				{"SNAPSHOT", "SNAPSHOT", "Mark snapshot output. Alias/variant of BUILDSNA."},
				{"SNARESET", "SNARESET", "Reset the snapshot memory to its initial state."},
				{"SNASETSQOP", "SNASETSQOP", "Set snapshot SQOP register values."},
				{"SETCPC", "SETCPC model", "Set the target CPC model for snapshot/cartridge export.\nSee also: SETCRTC, SETGATEARRAY"},
				{"SETCPCOLD", "SETCPCOLD", "Set the target to a classic (old) CPC model."},
				{"SETCRTC", "SETCRTC type", "Set the CRTC chip type for snapshot export (0-4).\nAlias: CRTCTYPE.\nSee also: SETCPC"},
				{"CRTCTYPE", "CRTCTYPE type", "Set the CRTC chip type. Alias of SETCRTC.\nSee also: SETCPC"},
				{"SETGATEARRAY", "SETGATEARRAY value", "Set the Gate Array configuration for snapshot export.\nAlias: GATEARRAY, GATE."},
				{"GATEARRAY", "GATEARRAY value", "Set the Gate Array value. Alias of SETGATEARRAY."},
				{"GATEARRAYBANK", "GATEARRAYBANK value", "Set the Gate Array bank configuration for snapshot export."},
				{"GATE", "GATE value", "Set Gate Array value. Short alias of SETGATEARRAY."},
				{"SETNEXTBANK", "SETNEXTBANK n", "Set the Next bank register for ZX Spectrum Next snapshot export."},
				{"SETSTACK", "SETSTACK address", "Set the initial SP (stack pointer) value for snapshot export."},
				{"NAMEBANK", "NAMEBANK n,'name'",
				 "Assign a display name to a non-temporary memory bank.\n"
				 "Used for cartridge/snapshot labelling in debuggers/emulators.\n"
				 "\n"
				 "Example:\n"
				 "  NAMEBANK 0,'ROM Init'\n"
				 "  NAMEBANK 1,'Serval screen'"},
				{"MAP", "MAP", "DSK deferred directive: emit a sector map for the DSK image."},
				{"MEMSPACE", "MEMSPACE", "Define a memory space region."},
				{"PAGE", "PAGE n", "Select memory page n."},
				{"PAGESET", "PAGESET n", "Set the active page set."},
				{"SLOT", "SLOT n", "Select memory slot n."},
				{"SECTION", "SECTION name", "Begin a named code/data section."},
				{"CHARSET", "CHARSET ['string',value | start[,end],value | 'str','str']",
				 "Remap ASCII character codes for string literals. Bare CHARSET resets all mappings.\n"
				 "\n"
				 "Forms:\n"
				 "  CHARSET                     ; reset all mappings to defaults\n"
				 "  CHARSET 'string',value      ; map each char of string starting at value\n"
				 "  CHARSET start,value         ; map from ASCII code start\n"
				 "  CHARSET start,end,value     ; map range start..end\n"
				 "  CHARSET 'abc','xyz'         ; char-to-char mapping\n"
				 "\n"
				 "Example — shift lowercase to CPC screen codes:\n"
				 "  CHARSET 'a',1\n"
				 "  DEFB 'hello'              ; emits 8,6,13,13,16 (remapped)"},
				{"UTF8REMAP", "UTF8REMAP 'char',value",
				 "Map a UTF-8 character to an 8-bit code for use in string literals.\n"
				 "Requires -fq command-line option.\n"
				 "\n"
				 "Example:\n"
				 "  UTF8REMAP 'é',50\n"
				 "  DEFB 'café'              ; 'é' emits 50"},
				{"ENUM", "ENUM [prefix[,start[,step]]] ... MEND",
				 "Declare a sequence of aliases with automatic increment.\n"
				 "  prefix — optional string prepended to each name\n"
				 "  start  — initial value (default 0)\n"
				 "  step   — increment per entry (default 1)\n"
				 "\n"
				 "Example:\n"
				 "  ENUM STATE_\n"
				 "    IDLE       ; STATE_IDLE = 0\n"
				 "    WALK       ; STATE_WALK = 1\n"
				 "    JUMP       ; STATE_JUMP = 2\n"
				 "  MEND\n"
				 "\n"
				 "Force a specific value mid-enum: name=value"},
				{"STRUCT", "STRUCT name ... ENDSTRUCT",
				 "Declare a data structure. {sizeof}name gives the byte size.\n"
				 "\n"
				 "Example:\n"
				 "  STRUCT entity\n"
				 "    x    DEFS 2    ; 16-bit x position\n"
				 "    y    DEFS 2    ; 16-bit y position\n"
				 "    type DEFS 1    ; entity type byte\n"
				 "  ENDSTRUCT\n"
				 "\n"
				 "  STRUCT entity player,1   ; declare 1 instance\n"
				 "  ; access: player        => base address\n"
				 "  ;         {sizeof}entity => 5 (byte size)\n"
				 "\n"
				 "Multiple instances are accessed via name+index."},
				{"ENDSTRUCT", "ENDSTRUCT", "Close a STRUCT declaration block. See STRUCT for full documentation."},
				{"LABEL", "LABEL LOCAL|GLOBAL",
				 "Control the ACE-DL label address space.\n"
				 "  LOCAL  — labels compiled at their compile-time address\n"
				 "  GLOBAL — labels in full 64K addressable space\n"
				 "\n"
				 "See also: LOCALISATION"},
				{"LOCALISATION", "LOCALISATION RAM,n | ROM,LOWER | ROM,n",
				 "Assign space labels to a RAM or ROM page for ACE-DL export.\n"
				 "\n"
				 "Examples:\n"
				 "  LOCALISATION RAM,4\n"
				 "  LOCALISATION ROM,LOWER\n"
				 "  LOCALISATION ROM,14"},
				{"NOEXPORT", "NOEXPORT [label1,label2,...]",
				 "Suppress export of symbols to the symbol table.\n"
				 "  (bare)          — suppress all following symbols\n"
				 "  label1,label2   — suppress specific named symbols only\n"
				 "\n"
				 "See also: ENOEXPORT, EXPORT"},
				{"ENOEXPORT", "ENOEXPORT [label1,label2,...]",
				 "Re-enable symbol export after NOEXPORT.\nBare form re-enables all; named form re-enables specific symbols.\nSee also: NOEXPORT"},
				{"EXPORT", "EXPORT label", "Mark a label for explicit export to the symbol table.\nSee also: NOEXPORT"},
				{"MODULE", "MODULE name",
				 "Prefix all following global labels with name_ (separator configurable with -msep).\n"
				 "\n"
				 "Example:\n"
				 "  MODULE soft1\n"
				 "  init:              ; becomes soft1_init\n"
				 "  MODULE soft2\n"
				 "  init:              ; becomes soft2_init\n"
				 "  JP soft1_init      ; cross-module reference\n"
				 "\n"
				 "Module names can contain loop-generated strings: truc{x}.\n"
				 "See also: ENDMODULE"},
				{"ENDMODULE", "ENDMODULE", "End the current MODULE scope.\nSee also: MODULE"},
				{"INSERT", "INSERT 'file'", "Insert file contents at the current position."},
				{"READ", "READ 'file'", "Read a file into memory. Alias of INCBIN/INSERT depending on context."},
				{"WRITE", "WRITE 'file'", "Write assembled output to a named file."},
				{"RELOCATE", "RELOCATE ... ENDRELOCATE",
				 "Mark a relocatable code section. Status: under development.\n"
				 "\n"
				 "RASM assembles the block twice (offset by #102) to detect byte-weight\n"
				 "changes and produces a .rel file with relocation tables.\n"
				 "\n"
				 "Constraints:\n"
				 "  - No MODULE changes inside the section\n"
				 "  - No ORG changes inside the section\n"
				 "  - Variables modified inside must be initialised inside\n"
				 "  - Cannot be used inside crunched segments\n"
				 "\n"
				 "Relocation table output example:\n"
				 "  relocation0:\n"
				 "    .reloc16 DEFW #0101,#0115\n"
				 "    .reloc8h DEFW #0104,#010E\n"
				 "    .reloc8l DEFW #0109"},
				{"ENDRELOCATE", "ENDRELOCATE", "Close a RELOCATE block. See RELOCATE for full documentation."},
				{"ENDORG", "ENDORG", "End a physical ORG override, returning to logical=physical addressing.\nSee also: ORG"},
				{"PHASE", "PHASE address",
				 "Alias of ORG logical,physical — sets the logical address while\n"
				 "keeping the physical write position unchanged.\n"
				 "\n"
				 "Example:\n"
				 "  ORG #8000\n"
				 "  PHASE #0100          ; labels from #0100, bytes still at #8000"},
				{"LZEXO", "LZEXO ... LZCLOSE",
				 "Open a crunched code section using Exomizer 2.0 compression.\n"
				 "Labels before the section are relocated relative to the compressed result.\n"
				 "Sections cannot be nested.\n"
				 "\n"
				 "Example:\n"
				 "  JR later\n"
				 "  LZEXO\n"
				 "    DEFS 256,0\n"
				 "  LZCLOSE\n"
				 "  later: RET\n"
				 "\n"
				 "Use DELAYED_LZEXO to defer compression until after full assembly.\n"
				 "See also: LZCLOSE"},
				{"LZX7", "LZX7 ... LZCLOSE", "Open a crunched code section using ZX7 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE"},
				{"LZX0", "LZX0 ... LZCLOSE", "Open a crunched code section using ZX0 (forward) compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZX0B"},
				{"LZX0B", "LZX0B ... LZCLOSE", "Open a crunched code section using ZX0 (backward) compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZX0"},
				{"LZAPU", "LZAPU ... LZCLOSE", "Open a crunched code section using AP-Ultra compression. See LZEXO for usage pattern.\nSee also: LZCLOSE"},
				{"LZSA1", "LZSA1 ... LZCLOSE", "Open a crunched code section using LZSA1 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZSA2"},
				{"LZSA2", "LZSA2 ... LZCLOSE", "Open a crunched code section using LZSA2 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZSA1"},
				{"LZ4", "LZ4 ... LZCLOSE", "Open a crunched code section using LZ4 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE"},
				{"LZ48", "LZ48 ... LZCLOSE", "Open a crunched code section using LZ48 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZ49"},
				{"LZ49", "LZ49 ... LZCLOSE", "Open a crunched code section using LZ49 compression. See LZEXO for usage pattern.\nSee also: LZCLOSE, LZ48"},
				{"LZ7", "LZ7 ... LZCLOSE", "Open a crunched code section using LZ7 compression. Alias of LZX7.\nSee also: LZCLOSE"},
				{"ZX0", "ZX0 ... LZCLOSE", "Open a crunched code section using ZX0 compression. Alias of LZX0.\nSee also: LZCLOSE, LZX0"},
				{"ZX7", "ZX7 ... LZCLOSE", "Open a crunched code section using ZX7 compression. Alias of LZX7.\nSee also: LZCLOSE, LZX7"},
				{"LZCLOSE", "LZCLOSE",
				 "Close the current crunched code section opened by LZx directives.\n"
				 "\n"
				 "Example — crunch and concatenate multiple binaries:\n"
				 "  BANK\n"
				 "  LZX0\n"
				 "    INCBIN 'first.bin'\n"
				 "    INCBIN 'second.bin'\n"
				 "  LZCLOSE\n"
				 "  SAVE 'combined.lz',...\n"
				 "\n"
				 "See also: LZEXO, LZX0, LZX7, LZAPU, LZSA1, LZSA2, LZ4, LZ48, LZ49"},
				{"CRUNCH", "CRUNCH", "Mark a crunched data region."},
				{"CRUNCHBANK", "CRUNCHBANK n", "Mark a crunched bank region (bank n)."},
				{"INCL", "INCL 'file'", "Include and decompress a crunched binary file on-the-fly."},
				{"INCL48", "INCL48 'file'", "Include and decompress an LZ48-crunched binary file on-the-fly."},
				{"INCL49", "INCL49 'file'", "Include and decompress an LZ49-crunched binary file on-the-fly."},
				{"INCLZ4", "INCLZ4 'file'", "Include and decompress an LZ4-crunched binary file on-the-fly."},
				{"INCLZSA1", "INCLZSA1 'file'", "Include and decompress an LZSA1-crunched binary file on-the-fly."},
				{"INCLZSA2", "INCLZSA2 'file'", "Include and decompress an LZSA2-crunched binary file on-the-fly."},
				{"INCZX0", "INCZX0 'file'", "Include and decompress a ZX0 (forward) crunched binary file on-the-fly."},
				{"INCZX0B", "INCZX0B 'file'", "Include and decompress a ZX0 (backward) crunched binary file on-the-fly."},
				{"INCZX7", "INCZX7 'file'", "Include and decompress a ZX7-crunched binary file on-the-fly."},
				{"INCEXO", "INCEXO 'file'", "Include and decompress an Exomizer-crunched binary file on-the-fly."},
				{"INCAPU", "INCAPU 'file'", "Include and decompress an AP-Ultra-crunched binary file on-the-fly."},
				{"INCSNA", "INCSNA 'file'", "Include memory data from a snapshot file."},
				{"TICKER", "TICKER START,variable\nTICKER STOP,variable",
				 "Measure T-states or NOP cycles between START and STOP.\n"
				 "Result is stored in the given variable.\n"
				 "  Default: NOP cycles\n"
				 "  ZX Spectrum mode: T-states\n"
				 "\n"
				 "Typical use — synchronise code to a raster line:\n"
				 "  TICKER START,measure\n"
				 "    REPEAT 8\n"
				 "      INC B : OUTI\n"
				 "    REND\n"
				 "  TICKER STOP,measure\n"
				 "  DEFS 64-getnop(DEC A : JR NZ)-measure\n"
				 "\n"
				 "See also: PRINT (to display the measured value)"},
				{"TIMESTAMP", "TIMESTAMP \"format_string\"",
				 "Generate a date/time string at assembly time.\n"
				 "\n"
				 "Format codes:\n"
				 "  Y or YYYY  four-digit year\n"
				 "  YY         two-digit year\n"
				 "  M          month (2 digits)\n"
				 "  D          day (2 digits)\n"
				 "  h          hour (2 digits)\n"
				 "  m          minutes (2 digits)\n"
				 "  s          seconds (2 digits)\n"
				 "\n"
				 "Example:\n"
				 "  TIMESTAMP \"[Y-M-D h:m]\"    ; e.g. => [2024-06-15 14:30]\n"
				 "  DEFB $-1                    ; embed in binary output"},
				{"PRINT", "PRINT 'text',value,...",
				 "Display text and values at assembly time (to stdout).\n"
				 "\n"
				 "Format prefixes:\n"
				 "  {hex}         hexadecimal (auto 2 or 4 digits)\n"
				 "  {hex2}        forced 2-digit hex\n"
				 "  {hex4}        forced 4-digit hex\n"
				 "  {hex8}        forced 8-digit hex\n"
				 "  {bin}         binary (auto 8 or 16 bits)\n"
				 "  {bin8}        forced 8-bit binary\n"
				 "  {bin16}       forced 16-bit binary\n"
				 "  {bin32}       forced 32-bit binary\n"
				 "  {int}         rounded integer\n"
				 "\n"
				 "Examples:\n"
				 "  PRINT 'Code size: ',{hex}$-start\n"
				 "  PRINT 'Value: ',myvar\n"
				 "\n"
				 "See also: DELAYED_PRINT (for forward-declared variables), STOP, FAIL"},
				{"DELAYED_PRINT", "DELAYED_PRINT 'text',value,...",
				 "Like PRINT but output is deferred until the end of assembly.\n"
				 "Use this when variables are not yet defined at the point of the call.\n"
				 "\n"
				 "Warning: avoid inside loops — all loop instances see the final value.\n"
				 "\n"
				 "Example:\n"
				 "  DELAYED_PRINT 'Total size: ',{hex}end_label-start_label\n"
				 "\n"
				 "See also: PRINT"},
				{"STOP", "STOP", "Halt assembly immediately. No output file is produced.\nSee also: FAIL, ASSERT"},
				{"FAIL", "FAIL 'text',value,...",
				 "Print a message then halt assembly. Equivalent to PRINT + STOP.\n"
				 "\n"
				 "Example:\n"
				 "  IFDEF DEBUG\n"
				 "    FAIL 'Cannot use debug mode in release build!'\n"
				 "  ENDIF\n"
				 "\n"
				 "See also: PRINT, STOP, ASSERT"},
				{"ASSERT", "ASSERT condition[,'message',...]",
				 "Halt assembly if condition is false. Optional message is printed.\n"
				 "\n"
				 "Examples:\n"
				 "  ASSERT $-code_start < 256\n"
				 "  ASSERT screen_table < #C000,'Screen table must be below #C000'\n"
				 "\n"
				 "See also: FAIL, STOP"},
				{"WARN", "WARN 'text'", "Emit an assembly-time warning message without stopping assembly.\nAlias: WARNING.\nSee also: ERROR, FAIL"},
				{"WARNING", "WARNING 'text'", "Emit an assembly-time warning message. Alias of WARN.\nSee also: ERROR, FAIL"},
				{"ERROR", "ERROR 'text'", "Emit an assembly-time error message and halt assembly.\nSee also: FAIL, ASSERT"},
				{"BRK", "BRK",
				 "Emit hard breakpoint bytes (#ED, #FF) at the current address.\n"
				 "Not handled by all emulators.\n"
				 "\n"
				 "To redefine the bytes emitted:\n"
				 "  REDEFINE_BRK #CF,#CF,#CF\n"
				 "\n"
				 "See also: BREAKPOINT (soft breakpoint, does not modify binary)"},
				{"REDEFINE_BRK", "REDEFINE_BRK b1,b2,...",
				 "Redefine the bytes emitted by the BRK directive.\n"
				 "\n"
				 "Example:\n"
				 "  REDEFINE_BRK #CF,#CF,#CF   ; RST #28 x3 (custom trap)\n"
				 "\n"
				 "See also: BRK"},
				{"BREAKPOINT", "BREAKPOINT [options...]",
				 "Define a soft exportable breakpoint at the current address.\n"
				 "The binary is NOT modified. Breakpoints must be exported via command-line.\n"
				 "\n"
				 "Simple forms:\n"
				 "  BREAKPOINT               ; breakpoint at current address\n"
				 "  BREAKPOINT label         ; breakpoint at label\n"
				 "\n"
				 "Extended ACE-DL options (all optional, order-independent):\n"
				 "  EXEC / MEM / IO          breakpoint type\n"
				 "  READ / WRITE / RW        access mode\n"
				 "  STOP / WATCH             execution mode\n"
				 "  ADDR=address             trigger address\n"
				 "  MASK=mask                address mask (default #FFFF)\n"
				 "  SIZE=size                size for memory breakpoints\n"
				 "  VALUE=value              trigger on byte value\n"
				 "  CONDITION='expr'         ACE-DL condition string\n"
				 "  NAME='label'             display name in emulator\n"
				 "\n"
				 "Export options: -ok, -eb, -sb (see RASM command-line docs)\n"
				 "See also: BRK (hard breakpoint that modifies binary)"},
				{"TRACE", "TRACE", "Enable or disable assembly trace output (verbose mode during assembly)."},
				{"UNDEF", "UNDEF variable",
				 "Remove a variable. Has no effect if the variable is absent.\n"
				 "\n"
				 "Example:\n"
				 "  x=5\n"
				 "  UNDEF x        ; x is no longer defined\n"
				 "  IFDEF x        ; this block is skipped\n"
				 "  ENDIF"},
				{"VOID", "VOID",
				 "Consume a value/token. Used with -void mode to safely call macros with no parameters.\n"
				 "\n"
				 "Example:\n"
				 "  mymacro (void)    ; call macro with void parameter sentinel"},
				{"VARIABLE", "name=value",
				 "Declare or update a dynamic variable. Unlike EQU, variables can be reassigned.\n"
				 "\n"
				 "Examples:\n"
				 "  ang=0\n"
				 "  REPEAT 256\n"
				 "    DEFB 127*sin(ang)\n"
				 "    ang=ang+360/256\n"
				 "  REND\n"
				 "\n"
				 "Indexed variables inside loops:\n"
				 "  REPEAT 256,x\n"
				 "    myvar{x}=x*5\n"
				 "  REND\n"
				 "\n"
				 "See also: EQU (constant, cannot be reassigned), DEFL"},
				{"HESITANT", "HESITANT", "Mark a symbol as hesitant (evaluated lazily, deferred until value is needed)."},
				{"GRAB", "GRAB address", "Grab memory from a given address for output (used to re-read assembled bytes)."},
				{"UNGRAB", "UNGRAB", "Release a grabbed memory region. See also: GRAB"},
				{"NOPSPLIT", "NOPSPLIT", "Enable NOP split mode for timing calculations (splits multi-byte NOP expansions)."},
				{"NOLIST", "NOLIST", "Suppress listing output for the following code.\nSee also: LIST"},
				{"LIST", "LIST", "Re-enable listing output after NOLIST.\nSee also: NOLIST"},
				{"OUTPUT", "OUTPUT 'file'", "Set the output file for assembly results."},
				{"CLOSEBIN", "CLOSEBIN", "Close the current binary output file."},
				{"FIELD", "FIELD offset", "Define a structure field at a given offset (used inside STRUCT blocks)."},
				{"TAG", "TAG name", "Attach a tag/label to the current address for debug export."},
				{"PROCEDURE", "PROCEDURE label", "Declare a procedure address for object export. (WIP)\nSee also: ENDPROCEDURE, PROC"},
				{"EXTERNAL", "EXTERNAL sym1,sym2,...", "Declare external symbols for import in object mode. (WIP)\nSee also: EXPORT"},
				{"PROC", "PROC name", "Begin a procedure block. Alias of PROCEDURE. (WIP)\nSee also: ENDPROC"},
				{"RUNTEST", "RUNTEST", "Execute an assembly-time test."},
				{"SKIP", "SKIP n", "Skip n bytes in the output without changing the address counter."},
				{"RAMPRESET", "RAMPRESET", "Reset RAM state for snapshot export."},
				{"LET", "name LET expr", "Assign a value to a variable. Alias of variable assignment (name=expr).\nSee also: EQU, DEFL"},
				{"ENT", "ENT address", "Set the entry point (PC). Alias of RUN.\nSee also: RUN"},
				{"END", "END", "Signal the end of assembly source. Optional — RASM processes all source regardless."},
			};
			//
			// Preprocessor / flow-control directives
			//
			static const DirectiveDef kZ80PreprocessorDirectiveDefs[] = {
				{"IF", "IF condition ... [ELSEIF condition] ... [ELSE] ... ENDIF",
				 "Conditional assembly. Assembles the block only if condition is true.\n"
				 "\n"
				 "Example:\n"
				 "  IF version >= 2\n"
				 "    DEFB #02\n"
				 "  ELSEIF version == 1\n"
				 "    DEFB #01\n"
				 "  ELSE\n"
				 "    DEFB #00\n"
				 "  ENDIF\n"
				 "\n"
				 "See also: IFNOT, IFDEF, IFNDEF, IFUSED, IFNUSED"},
				{"IFNOT", "IFNOT condition ... ENDIF",
				 "Conditional assembly. Assembles the block only if condition is false.\n"
				 "\n"
				 "Example:\n"
				 "  IFNOT release_mode\n"
				 "    BRK\n"
				 "  ENDIF\n"
				 "\n"
				 "See also: IF, IFDEF, IFNDEF"},
				{"IFDEF", "IFDEF symbol ... ENDIF",
				 "Conditional assembly. Assembles the block only if symbol/variable/alias/macro is defined.\n"
				 "\n"
				 "Example:\n"
				 "  IFDEF production\n"
				 "    OR #80           ; enable production flag\n"
				 "  ENDIF\n"
				 "\n"
				 "Include guard pattern:\n"
				 "  IFNDEF __MYHEADER__\n"
				 "  __MYHEADER__ = 1\n"
				 "    ; ... code ...\n"
				 "  ENDIF\n"
				 "\n"
				 "See also: IFNDEF, IF, UNDEF"},
				{"IFNDEF", "IFNDEF symbol ... ENDIF",
				 "Conditional assembly. Assembles the block only if symbol is NOT defined.\n"
				 "\n"
				 "Example (include guard):\n"
				 "  IFNDEF __MYHEADER__\n"
				 "  __MYHEADER__ = 1\n"
				 "    ; ... header code ...\n"
				 "  ENDIF\n"
				 "\n"
				 "See also: IFDEF, IF"},
				{"IFUSED", "IFUSED symbol ... ENDIF",
				 "Conditional assembly. Assembles the block only if symbol has been referenced.\n"
				 "\n"
				 "See also: IFNUSED, IFDEF"},
				{"IFNUSED", "IFNUSED symbol ... ENDIF",
				 "Conditional assembly. Assembles the block only if symbol has NOT been referenced.\n"
				 "\n"
				 "See also: IFUSED, IFDEF"},
				{"ELSEIF", "ELSEIF condition", "Alternative condition branch inside an IF block.\nSee IF for full documentation and examples."},
				{"ELSE", "ELSE", "Fallback branch inside an IF/IFDEF/IFNDEF/IFUSED/IFNUSED block.\nSee IF for full documentation and examples."},
				{"ENDIF", "ENDIF", "Close an IF/IFDEF/IFNDEF/IFUSED/IFNUSED block.\nSee IF for full documentation and examples."},
				{"SWITCH", "SWITCH expr ... CASE n ... [DEFAULT] ... ENDSWITCH",
				 "C-style switch on an expression.\n"
				 "Multiple CASE blocks with the same value are allowed (partial code sharing).\n"
				 "No implicit fall-through — use BREAK to exit a case early.\n"
				 "\n"
				 "Example:\n"
				 "  SWITCH entity_type\n"
				 "    CASE 3\n"
				 "      DEFB 'A'\n"
				 "    CASE 5\n"
				 "      DEFB 'B'\n"
				 "    CASE 7\n"
				 "      DEFB 'C'\n"
				 "      BREAK\n"
				 "    DEFAULT\n"
				 "      DEFB 'F'\n"
				 "  ENDSWITCH"},
				{"MACRO", "name MACRO [param1,param2,...] ... MEND",
				 "Declare a macro. Parameters are referenced as {param} in the body.\n"
				 "\n"
				 "Example:\n"
				 "  MACRO LDIXREG register,dep\n"
				 "    IF {dep}<-128 || {dep}>127\n"
				 "      PUSH BC,IX\n"
				 "      LD BC,{dep}\n"
				 "      ADD IX,BC\n"
				 "      LD (IX+0),{register}\n"
				 "      POP IX,BC\n"
				 "    ELSE\n"
				 "      LD (IX+{dep}),{register}\n"
				 "    ENDIF\n"
				 "  MEND\n"
				 "\n"
				 "Call: LDIXREG B,5\n"
				 "Safe call with no params: mymacro (void)\n"
				 "\n"
				 "16-bit register decomposition inside macros:\n"
				 "  {R1}.low   — low byte of 16-bit register\n"
				 "  {R1}.high  — high byte of 16-bit register\n"
				 "\n"
				 "Dynamic parameter evaluation:\n"
				 "  test {eval}repeat_counter  ; evaluate counter before passing\n"
				 "\n"
				 "See also: MEND, ENDM, ENDMACRO"},
				{"MEND", "MEND", "Close a MACRO declaration block. Aliases: ENDM, ENDMACRO.\nSee MACRO for full documentation."},
				{"ENDM", "ENDM", "Close a MACRO declaration block. Alias of MEND.\nSee MACRO for full documentation."},
				{"ENDMACRO", "ENDMACRO", "Close a MACRO declaration block. Alias of MEND.\nSee MACRO for full documentation."},
				{"REPEAT", "REPEAT n[,counter[,start[,step]]] ... REND",
				 "Repeat a block n times. All parameters except n are optional.\n"
				 "  counter — variable name tracking the current iteration\n"
				 "  start   — initial counter value (default 1)\n"
				 "  step    — counter increment per iteration (default 1)\n"
				 "\n"
				 "Examples:\n"
				 "  REPEAT 10 : LDI : REND           ; 10x LDI\n"
				 "  REPEAT 10,x : LD A,x : REND      ; LD A,1 ... LD A,10\n"
				 "  REPEAT 5,x,0 : PRINT x : REND    ; 0 1 2 3 4\n"
				 "  REPEAT 5,x,0,2 : PRINT x : REND  ; 0 2 4 6 8\n"
				 "\n"
				 "Conditional loop with UNTIL:\n"
				 "  cpt=90\n"
				 "  REPEAT\n"
				 "    DEFB cpt\n"
				 "    cpt=cpt-2\n"
				 "  UNTIL cpt>0\n"
				 "\n"
				 "Instruction repetition shorthand:\n"
				 "  LDI 16       ; equivalent to REPEAT 16 : LDI : REND\n"
				 "  NOP 5        ; five NOPs\n"
				 "\n"
				 "See also: REND, UNTIL, WHILE"},
				{"REPT", "REPT n[,counter[,start[,step]]] ... REND", "Repeat a block n times. Alias of REPEAT.\nSee REPEAT for full documentation and examples."},
				{"REND", "REND", "Close a REPEAT/REPT block. Aliases: ENDREPT, ENDREPEAT.\nSee REPEAT for full documentation."},
				{"ENDREPT", "ENDREPT", "Close a REPEAT/REPT block. Alias of REND.\nSee REPEAT for full documentation."},
				{"ENDREPEAT", "ENDREPEAT", "Close a REPEAT/REPT block. Alias of REND.\nSee REPEAT for full documentation."},
				{"UNTIL", "UNTIL condition",
				 "Conditional loop terminator inside a REPEAT block.\n"
				 "Stops repeating when condition is true.\n"
				 "\n"
				 "Example:\n"
				 "  cpt=90\n"
				 "  REPEAT\n"
				 "    DEFB cpt\n"
				 "    cpt=cpt-2\n"
				 "  UNTIL cpt>0\n"
				 "\n"
				 "See also: REPEAT, WHILE"},
				{"WHILE", "WHILE condition ... WEND",
				 "Repeat a block while condition is true.\n"
				 "\n"
				 "Example:\n"
				 "  i=0\n"
				 "  WHILE i<8\n"
				 "    DEFB i*16\n"
				 "    i=i+1\n"
				 "  WEND\n"
				 "\n"
				 "See also: WEND, REPEAT, UNTIL"},
				{"WEND", "WEND", "Close a WHILE block.\nSee WHILE for full documentation and examples."},
				{"STRUCT", "STRUCT name ... ENDSTRUCT",
				 "Declare a data structure. {sizeof}name gives the byte size.\n"
				 "\n"
				 "Example:\n"
				 "  STRUCT entity\n"
				 "    x    DEFS 2\n"
				 "    y    DEFS 2\n"
				 "    type DEFS 1\n"
				 "  ENDSTRUCT\n"
				 "\n"
				 "  STRUCT entity player,1   ; declare 1 instance\n"
				 "  ; {sizeof}entity => 5\n"
				 "\n"
				 "See also: ENDSTRUCT"},
				{"ENDSTRUCT", "ENDSTRUCT", "Close a STRUCT declaration block. See STRUCT for full documentation."},
				{"MODULE", "MODULE name",
				 "Prefix all following global labels with name_ (separator configurable with -msep).\n"
				 "\n"
				 "Example:\n"
				 "  MODULE soft1\n"
				 "  label_global           ; becomes soft1_label_global\n"
				 "  MODULE soft2\n"
				 "  label_global           ; becomes soft2_label_global\n"
				 "  JP soft1_label_global  ; cross-module reference\n"
				 "\n"
				 "See also: ENDMODULE"},
				{"ENDMODULE", "ENDMODULE", "End the current MODULE scope.\nSee also: MODULE"},
				{"PROCEDURE", "PROCEDURE label", "Declare a procedure for object export. (WIP)\nSee also: ENDPROCEDURE, PROC, ENDPROC"},
				{"PROC", "PROC name", "Begin a procedure block. Alias of PROCEDURE. (WIP)\nSee also: ENDPROC, ENDPROCEDURE"},
				{"ENDPROC", "ENDPROC", "End a PROC/PROCEDURE block.\nSee also: PROC, PROCEDURE"},
				{"ENDPROCEDURE", "ENDPROCEDURE", "End a PROCEDURE block. Alias of ENDPROC.\nSee also: PROCEDURE, PROC"},
				{"ENDP", "ENDP", "End a procedure block. Short alias of ENDPROC.\nSee also: PROC, PROCEDURE"},
			};
			//
			// Tokenizer functions for Z80 assembly
			//
			static bool TokenizeZ80Identifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				std::string_view sv(in_begin, static_cast<size_t>(in_end - in_begin));
				if (auto m = ctre::starts_with<R"([a-zA-Z_.@][a-zA-Z0-9_.@?]*)">(sv)) {
					out_begin = in_begin;
					out_end = in_begin + m.size();
					return true;
				}
				return false;
			}
			static bool TokenizeZ80Number(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				std::string_view sv(in_begin, static_cast<size_t>(in_end - in_begin));
				//
				// $hex  #hex  %binary  0xhex  0bhex  decimal[hH]
				//
				if (auto m = ctre::starts_with<R"([$#][0-9a-fA-F]+|%[01]+|0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+[hH]?)">(sv)) {
					out_begin = in_begin;
					out_end = in_begin + m.size();
					return true;
				}
				return false;
			}
			static bool TokenizeZ80String(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				std::string_view sv(in_begin, static_cast<size_t>(in_end - in_begin));
				if (auto m = ctre::starts_with<R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')">(sv)) {
					out_begin = in_begin;
					out_end = in_begin + m.size();
					return true;
				}
				return false;
			}
			static bool TokenizeZ80Punctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				(void)in_end;
				switch (*in_begin) {
					case '[':
					case ']':
					case '(':
					case ')':
					case '!':
					case '%':
					case '^':
					case '&':
					case '*':
					case '-':
					case '+':
					case '=':
					case '~':
					case '|':
					case '<':
					case '>':
					case '?':
					case ':':
					case '/':
					case ',':
					case '{':
					case '}':
						out_begin = in_begin;
						out_end = in_begin + 1;
						return true;
				}
				return false;
			}
			//
			// Full Z80 instruction set with timing and opcode information
			//
			// Format: { Inst. Set, Instruction, Timing Z80, Z80+M1, CPC, Opcode, Size }
			//
			// Inst. Set: "S" = Z80 Standard, "N" = Z80N Extended (ZX Spectrum Next)
			// Timing Z80: T-states on standard Z80
			// Z80+M1: T-states including M1 wait states
			// CPC: T-states on Amstrad CPC
			// Opcode: Hexadecimal opcode bytes
			// Size: Instruction size in bytes
			//
			// @see http://map.grauw.nl/resources/z80instr.php
			// @see http://www.cpcwiki.eu/forum/programming/rasm-z80-assembler-in-beta/
			// @see https://wiki.specnext.dev/Extended_Z80_instruction_set#Z80N_instructions_opcodes
			//
			//
			struct Z80Instruction {
				const char* set;
				const char* instruction;
				const char* timingZ80;
				const char* timingZ80M1;
				const char* timingCPC;
				const char* opcode;
				const char* size;
			};
			static const Z80Instruction kZ80InstructionSet[] = {
				{"S", "ADC A,(HL)", "7", "8", "2", "8E", "1"},
				{"S", "ADC A,(IX+o)", "19", "21", "5", "DD 8E o", "3"},
				{"S", "ADC A,(IY+o)", "19", "21", "5", "FD 8E o", "3"},
				{"S", "ADC A,IXH", "8", "10", "2", "DD 8C", "2"},
				{"S", "ADC A,IXL", "8", "10", "2", "DD 8D", "2"},
				{"S", "ADC A,IYH", "8", "10", "2", "FD 8C", "2"},
				{"S", "ADC A,IYL", "8", "10", "2", "FD 8D", "2"},
				{"S", "ADC A,n", "7", "8", "2", "CE n", "2"},
				{"S", "ADC A,B", "4", "5", "1", "88", "1"},
				{"S", "ADC A,C", "4", "5", "1", "89", "1"},
				{"S", "ADC A,D", "4", "5", "1", "8A", "1"},
				{"S", "ADC A,E", "4", "5", "1", "8B", "1"},
				{"S", "ADC A,H", "4", "5", "1", "8C", "1"},
				{"S", "ADC A,L", "4", "5", "1", "8D", "1"},
				{"S", "ADC A,A", "4", "5", "1", "8F", "1"},
				{"S", "ADC HL,BC", "15", "17", "4", "ED 4A", "2"},
				{"S", "ADC HL,DE", "15", "17", "4", "ED 5A", "2"},
				{"S", "ADC HL,HL", "15", "17", "4", "ED 6A", "2"},
				{"S", "ADC HL,SP", "15", "17", "4", "ED 7A", "2"},
				{"S", "ADD A,(HL)", "7", "8", "2", "86", "1"},
				{"S", "ADD A,(IX+o)", "19", "21", "5", "DD 86 o", "3"},
				{"S", "ADD A,(IY+o)", "19", "21", "5", "FD 86 o", "3"},
				{"S", "ADD A,IXH", "8", "10", "2", "DD 84", "2"},
				{"S", "ADD A,IXL", "8", "10", "2", "DD 85", "2"},
				{"S", "ADD A,IYH", "8", "10", "2", "FD 84", "2"},
				{"S", "ADD A,IYL", "8", "10", "2", "FD 85", "2"},
				{"S", "ADD A,n", "7", "8", "2", "C6 n", "2"},
				{"S", "ADD A,B", "4", "5", "1", "80", "1"},
				{"S", "ADD A,C", "4", "5", "1", "81", "1"},
				{"S", "ADD A,D", "4", "5", "1", "82", "1"},
				{"S", "ADD A,E", "4", "5", "1", "83", "1"},
				{"S", "ADD A,H", "4", "5", "1", "84", "1"},
				{"S", "ADD A,L", "4", "5", "1", "85", "1"},
				{"S", "ADD A,A", "4", "5", "1", "87", "1"},
				{"S", "ADD HL,BC", "11", "12", "3", "09", "1"},
				{"S", "ADD HL,DE", "11", "12", "3", "19", "1"},
				{"S", "ADD HL,HL", "11", "12", "3", "29", "1"},
				{"S", "ADD HL,SP", "11", "12", "3", "39", "1"},
				{"S", "ADD IX,BC", "15", "17", "4", "DD 09", "2"},
				{"S", "ADD IX,DE", "15", "17", "4", "DD 19", "2"},
				{"S", "ADD IX,IX", "15", "17", "4", "DD 29", "2"},
				{"S", "ADD IX,SP", "15", "17", "4", "DD 39", "2"},
				{"S", "ADD IY,BC", "15", "17", "4", "FD 09", "2"},
				{"S", "ADD IY,DE", "15", "17", "4", "FD 19", "2"},
				{"S", "ADD IY,IY", "15", "17", "4", "FD 29", "2"},
				{"S", "ADD IY,SP", "15", "17", "4", "FD 39", "2"},
				{"S", "AND (HL)", "7", "8", "2", "A6", "1"},
				{"S", "AND (IX+o)", "19", "21", "5", "DD A6 o", "3"},
				{"S", "AND (IY+o)", "19", "21", "5", "FD A6 o", "3"},
				{"S", "AND IXH", "8", "10", "2", "DD A4", "2"},
				{"S", "AND IXL", "8", "10", "2", "DD A5", "2"},
				{"S", "AND IYH", "8", "10", "2", "FD A4", "2"},
				{"S", "AND IYL", "8", "10", "2", "FD A5", "2"},
				{"S", "AND n", "7", "8", "2", "E6 n", "2"},
				{"S", "AND B", "4", "5", "1", "A0", "1"},
				{"S", "AND C", "4", "5", "1", "A1", "1"},
				{"S", "AND D", "4", "5", "1", "A2", "1"},
				{"S", "AND E", "4", "5", "1", "A3", "1"},
				{"S", "AND H", "4", "5", "1", "A4", "1"},
				{"S", "AND L", "4", "5", "1", "A5", "1"},
				{"S", "AND A", "4", "5", "1", "A7", "1"},
				{"S", "BIT 0,(HL)", "12", "14", "3", "CB 46", "2"},
				{"S", "BIT 1,(HL)", "12", "14", "3", "CB 4E", "2"},
				{"S", "BIT 2,(HL)", "12", "14", "3", "CB 56", "2"},
				{"S", "BIT 3,(HL)", "12", "14", "3", "CB 5E", "2"},
				{"S", "BIT 4,(HL)", "12", "14", "3", "CB 66", "2"},
				{"S", "BIT 5,(HL)", "12", "14", "3", "CB 6E", "2"},
				{"S", "BIT 6,(HL)", "12", "14", "3", "CB 76", "2"},
				{"S", "BIT 7,(HL)", "12", "14", "3", "CB 7E", "2"},
				{"S", "BIT 0,(IX+o)", "20", "22", "6", "DD CB o 46", "4"},
				{"S", "BIT 0,(IX+o),B", "20", "22", "6", "DD CB o 40", "4"},
				{"S", "BIT 0,(IX+o),C", "20", "22", "6", "DD CB o 41", "4"},
				{"S", "BIT 0,(IX+o),D", "20", "22", "6", "DD CB o 42", "4"},
				{"S", "BIT 0,(IX+o),E", "20", "22", "6", "DD CB o 43", "4"},
				{"S", "BIT 0,(IX+o),H", "20", "22", "6", "DD CB o 44", "4"},
				{"S", "BIT 0,(IX+o),L", "20", "22", "6", "DD CB o 45", "4"},
				{"S", "BIT 0,(IX+o),A", "20", "22", "6", "DD CB o 47", "4"},
				{"S", "BIT 1,(IX+o)", "20", "22", "6", "DD CB o 4E", "4"},
				{"S", "BIT 1,(IX+o),B", "20", "22", "6", "DD CB o 48", "4"},
				{"S", "BIT 1,(IX+o),C", "20", "22", "6", "DD CB o 49", "4"},
				{"S", "BIT 1,(IX+o),D", "20", "22", "6", "DD CB o 4A", "4"},
				{"S", "BIT 1,(IX+o),E", "20", "22", "6", "DD CB o 4B", "4"},
				{"S", "BIT 1,(IX+o),H", "20", "22", "6", "DD CB o 4C", "4"},
				{"S", "BIT 1,(IX+o),L", "20", "22", "6", "DD CB o 4D", "4"},
				{"S", "BIT 1,(IX+o),A", "20", "22", "6", "DD CB o 4F", "4"},
				{"S", "BIT 2,(IX+o)", "20", "22", "6", "DD CB o 56", "4"},
				{"S", "BIT 2,(IX+o),B", "20", "22", "6", "DD CB o 50", "4"},
				{"S", "BIT 2,(IX+o),C", "20", "22", "6", "DD CB o 51", "4"},
				{"S", "BIT 2,(IX+o),D", "20", "22", "6", "DD CB o 52", "4"},
				{"S", "BIT 2,(IX+o),E", "20", "22", "6", "DD CB o 53", "4"},
				{"S", "BIT 2,(IX+o),H", "20", "22", "6", "DD CB o 54", "4"},
				{"S", "BIT 2,(IX+o),L", "20", "22", "6", "DD CB o 55", "4"},
				{"S", "BIT 2,(IX+o),A", "20", "22", "6", "DD CB o 57", "4"},
				{"S", "BIT 3,(IX+o)", "20", "22", "6", "DD CB o 5E", "4"},
				{"S", "BIT 3,(IX+o),B", "20", "22", "6", "DD CB o 58", "4"},
				{"S", "BIT 3,(IX+o),C", "20", "22", "6", "DD CB o 59", "4"},
				{"S", "BIT 3,(IX+o),D", "20", "22", "6", "DD CB o 5A", "4"},
				{"S", "BIT 3,(IX+o),E", "20", "22", "6", "DD CB o 5B", "4"},
				{"S", "BIT 3,(IX+o),H", "20", "22", "6", "DD CB o 5C", "4"},
				{"S", "BIT 3,(IX+o),L", "20", "22", "6", "DD CB o 5D", "4"},
				{"S", "BIT 3,(IX+o),A", "20", "22", "6", "DD CB o 5F", "4"},
				{"S", "BIT 4,(IX+o)", "20", "22", "6", "DD CB o 66", "4"},
				{"S", "BIT 4,(IX+o),B", "20", "22", "6", "DD CB o 60", "4"},
				{"S", "BIT 4,(IX+o),C", "20", "22", "6", "DD CB o 61", "4"},
				{"S", "BIT 4,(IX+o),D", "20", "22", "6", "DD CB o 62", "4"},
				{"S", "BIT 4,(IX+o),E", "20", "22", "6", "DD CB o 63", "4"},
				{"S", "BIT 4,(IX+o),H", "20", "22", "6", "DD CB o 64", "4"},
				{"S", "BIT 4,(IX+o),L", "20", "22", "6", "DD CB o 65", "4"},
				{"S", "BIT 4,(IX+o),A", "20", "22", "6", "DD CB o 67", "4"},
				{"S", "BIT 5,(IX+o)", "20", "22", "6", "DD CB o 6E", "4"},
				{"S", "BIT 5,(IX+o),B", "20", "22", "6", "DD CB o 68", "4"},
				{"S", "BIT 5,(IX+o),C", "20", "22", "6", "DD CB o 69", "4"},
				{"S", "BIT 5,(IX+o),D", "20", "22", "6", "DD CB o 6A", "4"},
				{"S", "BIT 5,(IX+o),E", "20", "22", "6", "DD CB o 6B", "4"},
				{"S", "BIT 5,(IX+o),H", "20", "22", "6", "DD CB o 6C", "4"},
				{"S", "BIT 5,(IX+o),L", "20", "22", "6", "DD CB o 6D", "4"},
				{"S", "BIT 5,(IX+o),A", "20", "22", "6", "DD CB o 6F", "4"},
				{"S", "BIT 6,(IX+o)", "20", "22", "6", "DD CB o 76", "4"},
				{"S", "BIT 6,(IX+o),B", "20", "22", "6", "DD CB o 70", "4"},
				{"S", "BIT 6,(IX+o),C", "20", "22", "6", "DD CB o 71", "4"},
				{"S", "BIT 6,(IX+o),D", "20", "22", "6", "DD CB o 72", "4"},
				{"S", "BIT 6,(IX+o),E", "20", "22", "6", "DD CB o 73", "4"},
				{"S", "BIT 6,(IX+o),H", "20", "22", "6", "DD CB o 74", "4"},
				{"S", "BIT 6,(IX+o),L", "20", "22", "6", "DD CB o 75", "4"},
				{"S", "BIT 6,(IX+o),A", "20", "22", "6", "DD CB o 77", "4"},
				{"S", "BIT 7,(IX+o)", "20", "22", "6", "DD CB o 7E", "4"},
				{"S", "BIT 7,(IX+o),B", "20", "22", "6", "DD CB o 78", "4"},
				{"S", "BIT 7,(IX+o),C", "20", "22", "6", "DD CB o 79", "4"},
				{"S", "BIT 7,(IX+o),D", "20", "22", "6", "DD CB o 7A", "4"},
				{"S", "BIT 7,(IX+o),E", "20", "22", "6", "DD CB o 7B", "4"},
				{"S", "BIT 7,(IX+o),H", "20", "22", "6", "DD CB o 7C", "4"},
				{"S", "BIT 7,(IX+o),L", "20", "22", "6", "DD CB o 7D", "4"},
				{"S", "BIT 7,(IX+o),A", "20", "22", "6", "DD CB o 7F", "4"},
				{"S", "BIT 0,(IY+o)", "20", "22", "6", "FD CB o 46", "4"},
				{"S", "BIT 0,(IY+o),B", "20", "22", "6", "FD CB o 40", "4"},
				{"S", "BIT 0,(IY+o),C", "20", "22", "6", "FD CB o 41", "4"},
				{"S", "BIT 0,(IY+o),D", "20", "22", "6", "FD CB o 42", "4"},
				{"S", "BIT 0,(IY+o),E", "20", "22", "6", "FD CB o 43", "4"},
				{"S", "BIT 0,(IY+o),H", "20", "22", "6", "FD CB o 44", "4"},
				{"S", "BIT 0,(IY+o),L", "20", "22", "6", "FD CB o 45", "4"},
				{"S", "BIT 0,(IY+o),A", "20", "22", "6", "FD CB o 47", "4"},
				{"S", "BIT 1,(IY+o)", "20", "22", "6", "FD CB o 4E", "4"},
				{"S", "BIT 1,(IY+o),B", "20", "22", "6", "FD CB o 48", "4"},
				{"S", "BIT 1,(IY+o),C", "20", "22", "6", "FD CB o 49", "4"},
				{"S", "BIT 1,(IY+o),D", "20", "22", "6", "FD CB o 4A", "4"},
				{"S", "BIT 1,(IY+o),E", "20", "22", "6", "FD CB o 4B", "4"},
				{"S", "BIT 1,(IY+o),H", "20", "22", "6", "FD CB o 4C", "4"},
				{"S", "BIT 1,(IY+o),L", "20", "22", "6", "FD CB o 4D", "4"},
				{"S", "BIT 1,(IY+o),A", "20", "22", "6", "FD CB o 4F", "4"},
				{"S", "BIT 2,(IY+o)", "20", "22", "6", "FD CB o 56", "4"},
				{"S", "BIT 2,(IY+o),B", "20", "22", "6", "FD CB o 50", "4"},
				{"S", "BIT 2,(IY+o),C", "20", "22", "6", "FD CB o 51", "4"},
				{"S", "BIT 2,(IY+o),D", "20", "22", "6", "FD CB o 52", "4"},
				{"S", "BIT 2,(IY+o),E", "20", "22", "6", "FD CB o 53", "4"},
				{"S", "BIT 2,(IY+o),H", "20", "22", "6", "FD CB o 54", "4"},
				{"S", "BIT 2,(IY+o),L", "20", "22", "6", "FD CB o 55", "4"},
				{"S", "BIT 2,(IY+o),A", "20", "22", "6", "FD CB o 57", "4"},
				{"S", "BIT 3,(IY+o)", "20", "22", "6", "FD CB o 5E", "4"},
				{"S", "BIT 3,(IY+o),B", "20", "22", "6", "FD CB o 58", "4"},
				{"S", "BIT 3,(IY+o),C", "20", "22", "6", "FD CB o 59", "4"},
				{"S", "BIT 3,(IY+o),D", "20", "22", "6", "FD CB o 5A", "4"},
				{"S", "BIT 3,(IY+o),E", "20", "22", "6", "FD CB o 5B", "4"},
				{"S", "BIT 3,(IY+o),H", "20", "22", "6", "FD CB o 5C", "4"},
				{"S", "BIT 3,(IY+o),L", "20", "22", "6", "FD CB o 5D", "4"},
				{"S", "BIT 3,(IY+o),A", "20", "22", "6", "FD CB o 5F", "4"},
				{"S", "BIT 4,(IY+o)", "20", "22", "6", "FD CB o 66", "4"},
				{"S", "BIT 4,(IY+o),B", "20", "22", "6", "FD CB o 60", "4"},
				{"S", "BIT 4,(IY+o),C", "20", "22", "6", "FD CB o 61", "4"},
				{"S", "BIT 4,(IY+o),D", "20", "22", "6", "FD CB o 62", "4"},
				{"S", "BIT 4,(IY+o),E", "20", "22", "6", "FD CB o 63", "4"},
				{"S", "BIT 4,(IY+o),H", "20", "22", "6", "FD CB o 64", "4"},
				{"S", "BIT 4,(IY+o),L", "20", "22", "6", "FD CB o 65", "4"},
				{"S", "BIT 4,(IY+o),A", "20", "22", "6", "FD CB o 67", "4"},
				{"S", "BIT 5,(IY+o)", "20", "22", "6", "FD CB o 6E", "4"},
				{"S", "BIT 5,(IY+o),B", "20", "22", "6", "FD CB o 68", "4"},
				{"S", "BIT 5,(IY+o),C", "20", "22", "6", "FD CB o 69", "4"},
				{"S", "BIT 5,(IY+o),D", "20", "22", "6", "FD CB o 6A", "4"},
				{"S", "BIT 5,(IY+o),E", "20", "22", "6", "FD CB o 6B", "4"},
				{"S", "BIT 5,(IY+o),H", "20", "22", "6", "FD CB o 6C", "4"},
				{"S", "BIT 5,(IY+o),L", "20", "22", "6", "FD CB o 6D", "4"},
				{"S", "BIT 5,(IY+o),A", "20", "22", "6", "FD CB o 6F", "4"},
				{"S", "BIT 6,(IY+o)", "20", "22", "6", "FD CB o 76", "4"},
				{"S", "BIT 6,(IY+o),B", "20", "22", "6", "FD CB o 70", "4"},
				{"S", "BIT 6,(IY+o),C", "20", "22", "6", "FD CB o 71", "4"},
				{"S", "BIT 6,(IY+o),D", "20", "22", "6", "FD CB o 72", "4"},
				{"S", "BIT 6,(IY+o),E", "20", "22", "6", "FD CB o 73", "4"},
				{"S", "BIT 6,(IY+o),H", "20", "22", "6", "FD CB o 74", "4"},
				{"S", "BIT 6,(IY+o),L", "20", "22", "6", "FD CB o 75", "4"},
				{"S", "BIT 6,(IY+o),A", "20", "22", "6", "FD CB o 77", "4"},
				{"S", "BIT 7,(IY+o)", "20", "22", "6", "FD CB o 7E", "4"},
				{"S", "BIT 7,(IY+o),B", "20", "22", "6", "FD CB o 78", "4"},
				{"S", "BIT 7,(IY+o),C", "20", "22", "6", "FD CB o 79", "4"},
				{"S", "BIT 7,(IY+o),D", "20", "22", "6", "FD CB o 7A", "4"},
				{"S", "BIT 7,(IY+o),E", "20", "22", "6", "FD CB o 7B", "4"},
				{"S", "BIT 7,(IY+o),H", "20", "22", "6", "FD CB o 7C", "4"},
				{"S", "BIT 7,(IY+o),L", "20", "22", "6", "FD CB o 7D", "4"},
				{"S", "BIT 7,(IY+o),A", "20", "22", "6", "FD CB o 7F", "4"},
				{"S", "BIT 0,B", "8", "10", "2", "CB 40", "2"},
				{"S", "BIT 0,C", "8", "10", "2", "CB 41", "2"},
				{"S", "BIT 0,D", "8", "10", "2", "CB 42", "2"},
				{"S", "BIT 0,E", "8", "10", "2", "CB 43", "2"},
				{"S", "BIT 0,H", "8", "10", "2", "CB 44", "2"},
				{"S", "BIT 0,L", "8", "10", "2", "CB 45", "2"},
				{"S", "BIT 0,A", "8", "10", "2", "CB 47", "2"},
				{"S", "BIT 1,B", "8", "10", "2", "CB 48", "2"},
				{"S", "BIT 1,C", "8", "10", "2", "CB 49", "2"},
				{"S", "BIT 1,D", "8", "10", "2", "CB 4A", "2"},
				{"S", "BIT 1,E", "8", "10", "2", "CB 4B", "2"},
				{"S", "BIT 1,H", "8", "10", "2", "CB 4C", "2"},
				{"S", "BIT 1,L", "8", "10", "2", "CB 4D", "2"},
				{"S", "BIT 1,A", "8", "10", "2", "CB 4F", "2"},
				{"S", "BIT 2,B", "8", "10", "2", "CB 50", "2"},
				{"S", "BIT 2,C", "8", "10", "2", "CB 51", "2"},
				{"S", "BIT 2,D", "8", "10", "2", "CB 52", "2"},
				{"S", "BIT 2,E", "8", "10", "2", "CB 53", "2"},
				{"S", "BIT 2,H", "8", "10", "2", "CB 54", "2"},
				{"S", "BIT 2,L", "8", "10", "2", "CB 55", "2"},
				{"S", "BIT 2,A", "8", "10", "2", "CB 57", "2"},
				{"S", "BIT 3,B", "8", "10", "2", "CB 58", "2"},
				{"S", "BIT 3,C", "8", "10", "2", "CB 59", "2"},
				{"S", "BIT 3,D", "8", "10", "2", "CB 5A", "2"},
				{"S", "BIT 3,E", "8", "10", "2", "CB 5B", "2"},
				{"S", "BIT 3,H", "8", "10", "2", "CB 5C", "2"},
				{"S", "BIT 3,L", "8", "10", "2", "CB 5D", "2"},
				{"S", "BIT 3,A", "8", "10", "2", "CB 5F", "2"},
				{"S", "BIT 4,B", "8", "10", "2", "CB 60", "2"},
				{"S", "BIT 4,C", "8", "10", "2", "CB 61", "2"},
				{"S", "BIT 4,D", "8", "10", "2", "CB 62", "2"},
				{"S", "BIT 4,E", "8", "10", "2", "CB 63", "2"},
				{"S", "BIT 4,H", "8", "10", "2", "CB 64", "2"},
				{"S", "BIT 4,L", "8", "10", "2", "CB 65", "2"},
				{"S", "BIT 4,A", "8", "10", "2", "CB 67", "2"},
				{"S", "BIT 5,B", "8", "10", "2", "CB 68", "2"},
				{"S", "BIT 5,C", "8", "10", "2", "CB 69", "2"},
				{"S", "BIT 5,D", "8", "10", "2", "CB 6A", "2"},
				{"S", "BIT 5,E", "8", "10", "2", "CB 6B", "2"},
				{"S", "BIT 5,H", "8", "10", "2", "CB 6C", "2"},
				{"S", "BIT 5,L", "8", "10", "2", "CB 6D", "2"},
				{"S", "BIT 5,A", "8", "10", "2", "CB 6F", "2"},
				{"S", "BIT 6,B", "8", "10", "2", "CB 70", "2"},
				{"S", "BIT 6,C", "8", "10", "2", "CB 71", "2"},
				{"S", "BIT 6,D", "8", "10", "2", "CB 72", "2"},
				{"S", "BIT 6,E", "8", "10", "2", "CB 73", "2"},
				{"S", "BIT 6,H", "8", "10", "2", "CB 74", "2"},
				{"S", "BIT 6,L", "8", "10", "2", "CB 75", "2"},
				{"S", "BIT 6,A", "8", "10", "2", "CB 77", "2"},
				{"S", "BIT 7,B", "8", "10", "2", "CB 78", "2"},
				{"S", "BIT 7,C", "8", "10", "2", "CB 79", "2"},
				{"S", "BIT 7,D", "8", "10", "2", "CB 7A", "2"},
				{"S", "BIT 7,E", "8", "10", "2", "CB 7B", "2"},
				{"S", "BIT 7,H", "8", "10", "2", "CB 7C", "2"},
				{"S", "BIT 7,L", "8", "10", "2", "CB 7D", "2"},
				{"S", "BIT 7,A", "8", "10", "2", "CB 7F", "2"},
				{"S", "CALL C,nn", "17/10", "18/11", "5/3", "DC nn nn", "3"},
				{"S", "CALL M,nn", "17/10", "18/11", "5/3", "FC nn nn", "3"},
				{"S", "CALL NC,nn", "17/10", "18/11", "5/3", "D4 nn nn", "3"},
				{"S", "CALL nn", "17", "18", "5", "CD nn nn", "3"},
				{"S", "CALL NZ,nn", "17/10", "18/11", "5/3", "C4 nn nn", "3"},
				{"S", "CALL P,nn", "17/10", "18/11", "5/3", "F4 nn nn", "3"},
				{"S", "CALL PE,nn", "17/10", "18/11", "5/3", "EC nn nn", "3"},
				{"S", "CALL PO,nn", "17/10", "18/11", "5/3", "E4 nn nn", "3"},
				{"S", "CALL Z,nn", "17/10", "18/11", "5/3", "CC nn nn", "3"},
				{"S", "CCF", "4", "5", "1", "3F", "1"},
				{"S", "CP (HL)", "7", "8", "2", "BE", "1"},
				{"S", "CP (IX+o)", "19", "21", "5", "DD BE o", "3"},
				{"S", "CP (IY+o)", "19", "21", "5", "FD BE o", "3"},
				{"S", "CP IXH", "8", "10", "2", "DD BC", "2"},
				{"S", "CP IXL", "8", "10", "2", "DD BD", "2"},
				{"S", "CP IYH", "8", "10", "2", "FD BC", "2"},
				{"S", "CP IYL", "8", "10", "2", "FD BD", "2"},
				{"S", "CP n", "7", "8", "2", "FE n", "2"},
				{"S", "CP B", "4", "5", "1", "B8", "1"},
				{"S", "CP C", "4", "5", "1", "B9", "1"},
				{"S", "CP D", "4", "5", "1", "BA", "1"},
				{"S", "CP E", "4", "5", "1", "BB", "1"},
				{"S", "CP H", "4", "5", "1", "BC", "1"},
				{"S", "CP L", "4", "5", "1", "BD", "1"},
				{"S", "CP A", "4", "5", "1", "BF", "1"},
				{"S", "CPD", "16", "18", "4", "ED A9", "2"},
				{"S", "CPDR", "21/16", "23/18", "6/4", "ED B9", "2"},
				{"S", "CPI", "16", "18", "4", "ED A1", "2"},
				{"S", "CPIR", "21/16", "23/18", "6/4", "ED B1", "2"},
				{"S", "CPL", "4", "5", "1", "2F", "1"},
				{"S", "DAA", "4", "5", "1", "27", "1"},
				{"S", "DEC (HL)", "11", "12", "3", "35", "1"},
				{"S", "DEC (IX+o)", "23", "25", "6", "DD 35 o", "3"},
				{"S", "DEC (IY+o)", "23", "25", "6", "FD 35 o", "3"},
				{"S", "DEC A", "4", "5", "1", "3D", "1"},
				{"S", "DEC B", "4", "5", "1", "05", "1"},
				{"S", "DEC BC", "6", "7", "2", "0B", "1"},
				{"S", "DEC C", "4", "5", "1", "0D", "1"},
				{"S", "DEC D", "4", "5", "1", "15", "1"},
				{"S", "DEC DE", "6", "7", "2", "1B", "1"},
				{"S", "DEC E", "4", "5", "1", "1D", "1"},
				{"S", "DEC H", "4", "5", "1", "25", "1"},
				{"S", "DEC HL", "6", "7", "2", "2B", "1"},
				{"S", "DEC IX", "10", "12", "3", "DD 2B", "2"},
				{"S", "DEC IXH", "8", "10", "2", "DD 25", "2"},
				{"S", "DEC IXL", "8", "10", "2", "DD 2D", "2"},
				{"S", "DEC IY", "10", "12", "3", "FD 2B", "2"},
				{"S", "DEC IYH", "8", "10", "2", "FD 25", "2"},
				{"S", "DEC IYL", "8", "10", "2", "FD 2D", "2"},
				{"S", "DEC L", "4", "5", "1", "2D", "1"},
				{"S", "DEC SP", "6", "7", "2", "3B", "1"},
				{"S", "DI", "4", "5", "1", "F3", "1"},
				{"S", "DJNZ o", "13/8", "14/9", "4/3", "10 o", "2"},
				{"S", "EI", "4", "5", "1", "FB", "1"},
				{"S", "EX (SP),HL", "19", "20", "6", "E3", "1"},
				{"S", "EX (SP),IX", "23", "25", "7", "DD E3", "2"},
				{"S", "EX (SP),IY", "23", "25", "7", "FD E3", "2"},
				{"S", "EX AF,AF'", "4", "5", "1", "08", "1"},
				{"S", "EX DE,HL", "4", "5", "1", "EB", "1"},
				{"S", "EXX", "4", "5", "1", "D9", "1"},
				{"S", "HALT", "4", "5", "1", "76", "1"},
				{"S", "IM 0", "8", "10", "2", "ED 46", "2"},
				{"S", "IM 1", "8", "10", "2", "ED 56", "2"},
				{"S", "IM 2", "8", "10", "2", "ED 5E", "2"},
				{"S", "IN (C)", "12", "14", "4", "ED 70", "2"}, // alt. IN F,(C)
				{"S", "IN A,(C)", "12", "14", "4", "ED 78", "2"},
				{"S", "IN A,(n)", "11", "12", "3", "DB n", "2"},
				{"S", "IN B,(C)", "12", "14", "4", "ED 40", "2"},
				{"S", "IN C,(C)", "12", "14", "4", "ED 48", "2"},
				{"S", "IN D,(C)", "12", "14", "4", "ED 50", "2"},
				{"S", "IN E,(C)", "12", "14", "4", "ED 58", "2"},
				{"S", "IN F,(C)", "12", "14", "4", "ED 70", "2"}, // alt. IN (C)
				{"S", "IN H,(C)", "12", "14", "4", "ED 60", "2"},
				{"S", "IN L,(C)", "12", "14", "4", "ED 68", "2"},
				{"S", "INC (HL)", "11", "12", "3", "34", "1"},
				{"S", "INC (IX+o)", "23", "25", "6", "DD 34 o", "3"},
				{"S", "INC (IY+o)", "23", "25", "6", "FD 34 o", "3"},
				{"S", "INC A", "4", "5", "1", "3C", "1"},
				{"S", "INC B", "4", "5", "1", "04", "1"},
				{"S", "INC BC", "6", "7", "2", "03", "1"},
				{"S", "INC C", "4", "5", "1", "0C", "1"},
				{"S", "INC D", "4", "5", "1", "14", "1"},
				{"S", "INC DE", "6", "7", "2", "13", "1"},
				{"S", "INC E", "4", "5", "1", "1C", "1"},
				{"S", "INC H", "4", "5", "1", "24", "1"},
				{"S", "INC HL", "6", "7", "2", "23", "1"},
				{"S", "INC IX", "10", "12", "3", "DD 23", "2"},
				{"S", "INC IXH", "8", "10", "2", "DD 24", "2"},
				{"S", "INC IXL", "8", "10", "2", "DD 2C", "2"},
				{"S", "INC IY", "10", "12", "3", "FD 23", "2"},
				{"S", "INC IYH", "8", "10", "2", "FD 24", "2"},
				{"S", "INC IYL", "8", "10", "2", "FD 2C", "2"},
				{"S", "INC L", "4", "5", "1", "2C", "1"},
				{"S", "INC SP", "6", "7", "2", "33", "1"},
				{"S", "IND", "16", "18", "5", "ED AA", "2"},
				{"S", "INDR", "21/16", "23/18", "6/5", "ED BA", "2"},
				{"S", "INI", "16", "18", "5", "ED A2", "2"},
				{"S", "INIR", "21/16", "23/18", "6/5", "ED B2", "2"},
				{"S", "JP (HL)", "4", "5", "1", "E9", "1"},
				{"S", "JP (IX)", "8", "10", "2", "DD E9", "2"},
				{"S", "JP (IY)", "8", "10", "2", "FD E9", "2"},
				{"S", "JP C,nn", "10", "11", "3", "DA nn nn", "3"},
				{"S", "JP M,nn", "10", "11", "3", "FA nn nn", "3"},
				{"S", "JP NC,nn", "10", "11", "3", "D2 nn nn", "3"},
				{"S", "JP nn", "10", "11", "3", "C3 nn nn", "3"},
				{"S", "JP NZ,nn", "10", "11", "3", "C2 nn nn", "3"},
				{"S", "JP P,nn", "10", "11", "3", "F2 nn nn", "3"},
				{"S", "JP PE,nn", "10", "11", "3", "EA nn nn", "3"},
				{"S", "JP PO,nn", "10", "11", "3", "E2 nn nn", "3"},
				{"S", "JP Z,nn", "10", "11", "3", "CA nn nn", "3"},
				{"S", "JR C,o", "12/7", "13/8", "3/2", "38 o", "2"},
				{"S", "JR NC,o", "12/7", "13/8", "3/2", "30 o", "2"},
				{"S", "JR NZ,o", "12/7", "13/8", "3/2", "20 o", "2"},
				{"S", "JR o", "12", "13", "3", "18 o", "2"},
				{"S", "JR Z,o", "12/7", "13/8", "3/2", "28 o", "2"},
				{"S", "LD (BC),A", "7", "8", "2", "02", "1"},
				{"S", "LD (DE),A", "7", "8", "2", "12", "1"},
				{"S", "LD (HL),n", "10", "11", "3", "36 n", "2"},
				{"S", "LD (HL),B", "7", "8", "2", "70", "1"},
				{"S", "LD (HL),C", "7", "8", "2", "71", "1"},
				{"S", "LD (HL),D", "7", "8", "2", "72", "1"},
				{"S", "LD (HL),E", "7", "8", "2", "73", "1"},
				{"S", "LD (HL),H", "7", "8", "2", "74", "1"},
				{"S", "LD (HL),L", "7", "8", "2", "75", "1"},
				{"S", "LD (HL),A", "7", "8", "2", "77", "1"},
				{"S", "LD (IX+o),n", "19", "21", "6", "DD 36 o n", "4"},
				{"S", "LD (IX+o),B", "19", "21", "5", "DD 70 o", "3"},
				{"S", "LD (IX+o),C", "19", "21", "5", "DD 71 o", "3"},
				{"S", "LD (IX+o),D", "19", "21", "5", "DD 72 o", "3"},
				{"S", "LD (IX+o),E", "19", "21", "5", "DD 73 o", "3"},
				{"S", "LD (IX+o),H", "19", "21", "5", "DD 74 o", "3"},
				{"S", "LD (IX+o),L", "19", "21", "5", "DD 75 o", "3"},
				{"S", "LD (IX+o),A", "19", "21", "5", "DD 77 o", "3"},
				{"S", "LD (IY+o),n", "19", "21", "6", "FD 36 o n", "4"},
				{"S", "LD (IY+o),B", "19", "21", "5", "FD 70 o", "3"},
				{"S", "LD (IY+o),C", "19", "21", "5", "FD 71 o", "3"},
				{"S", "LD (IY+o),D", "19", "21", "5", "FD 72 o", "3"},
				{"S", "LD (IY+o),E", "19", "21", "5", "FD 73 o", "3"},
				{"S", "LD (IY+o),H", "19", "21", "5", "FD 74 o", "3"},
				{"S", "LD (IY+o),L", "19", "21", "5", "FD 75 o", "3"},
				{"S", "LD (IY+o),A", "19", "21", "5", "FD 77 o", "3"},
				{"S", "LD (nn),A", "13", "14", "4", "32 nn nn", "3"},
				{"S", "LD (nn),BC", "20", "22", "6", "ED 43 nn nn", "4"},
				{"S", "LD (nn),DE", "20", "22", "6", "ED 53 nn nn", "4"},
				{"S", "LD (nn),HL", "16", "17", "5", "22 nn nn", "3"},
				{"S", "LD (nn),IX", "20", "22", "6", "DD 22 nn nn", "4"},
				{"S", "LD (nn),IY", "20", "22", "6", "FD 22 nn nn", "4"},
				{"S", "LD (nn),SP", "20", "22", "6", "ED 73 nn nn", "4"},
				{"S", "LD A,(BC)", "7", "8", "2", "0A", "1"},
				{"S", "LD A,(DE)", "7", "8", "2", "1A", "1"},
				{"S", "LD A,(HL)", "7", "8", "2", "7E", "1"},
				{"S", "LD A,(IX+o)", "19", "21", "5", "DD 7E o", "3"},
				{"S", "LD A,(IY+o)", "19", "21", "5", "FD 7E o", "3"},
				{"S", "LD A,(nn)", "13", "14", "4", "3A nn nn", "3"},
				{"S", "LD A,I", "9", "11", "3", "ED 57", "2"},
				{"S", "LD A,IXH", "8", "10", "2", "DD 7C", "2"},
				{"S", "LD A,IXL", "8", "10", "2", "DD 7D", "2"},
				{"S", "LD A,IYH", "8", "10", "2", "FD 7C", "2"},
				{"S", "LD A,IYL", "8", "10", "2", "FD 7D", "2"},
				{"S", "LD A,n", "7", "8", "2", "3E n", "2"},
				{"S", "LD A,B", "4", "5", "1", "78", "1"},
				{"S", "LD A,C", "4", "5", "1", "79", "1"},
				{"S", "LD A,D", "4", "5", "1", "7A", "1"},
				{"S", "LD A,E", "4", "5", "1", "7B", "1"},
				{"S", "LD A,H", "4", "5", "1", "7C", "1"},
				{"S", "LD A,L", "4", "5", "1", "7D", "1"},
				{"S", "LD A,A", "4", "5", "1", "7F", "1"},
				{"S", "LD A,R", "9", "11", "3", "ED 5F", "2"},
				{"S", "LD B,(HL)", "7", "8", "2", "46", "1"},
				{"S", "LD B,(IX+o)", "19", "21", "5", "DD 46 o", "3"},
				{"S", "LD B,(IY+o)", "19", "21", "5", "FD 46 o", "3"},
				{"S", "LD B,IXH", "8", "10", "2", "DD 44", "2"},
				{"S", "LD B,IXL", "8", "10", "2", "DD 45", "2"},
				{"S", "LD B,IYH", "8", "10", "2", "FD 44", "2"},
				{"S", "LD B,IYL", "8", "10", "2", "FD 45", "2"},
				{"S", "LD B,n", "7", "8", "2", "06 n", "2"},
				{"S", "LD B,B", "4", "5", "1", "40", "1"},
				{"S", "LD B,C", "4", "5", "1", "41", "1"},
				{"S", "LD B,D", "4", "5", "1", "42", "1"},
				{"S", "LD B,E", "4", "5", "1", "43", "1"},
				{"S", "LD B,H", "4", "5", "1", "44", "1"},
				{"S", "LD B,L", "4", "5", "1", "45", "1"},
				{"S", "LD B,A", "4", "5", "1", "47", "1"},
				{"S", "LD BC,(nn)", "20", "22", "6", "ED 4B nn nn", "4"},
				{"S", "LD BC,nn", "10", "11", "3", "01 nn nn", "3"},
				{"S", "LD C,(HL)", "7", "8", "2", "4E", "1"},
				{"S", "LD C,(IX+o)", "19", "21", "5", "DD 4E o", "3"},
				{"S", "LD C,(IY+o)", "19", "21", "5", "FD 4E o", "3"},
				{"S", "LD C,IXH", "8", "10", "2", "DD 4C", "2"},
				{"S", "LD C,IXL", "8", "10", "2", "DD 4D", "2"},
				{"S", "LD C,IYH", "8", "10", "2", "FD 4C", "2"},
				{"S", "LD C,IYL", "8", "10", "2", "FD 4D", "2"},
				{"S", "LD C,n", "7", "8", "2", "0E n", "2"},
				{"S", "LD C,B", "4", "5", "1", "48", "1"},
				{"S", "LD C,C", "4", "5", "1", "49", "1"},
				{"S", "LD C,D", "4", "5", "1", "4A", "1"},
				{"S", "LD C,E", "4", "5", "1", "4B", "1"},
				{"S", "LD C,H", "4", "5", "1", "4C", "1"},
				{"S", "LD C,L", "4", "5", "1", "4D", "1"},
				{"S", "LD C,A", "4", "5", "1", "4F", "1"},
				{"S", "LD D,(HL)", "7", "8", "2", "56", "1"},
				{"S", "LD D,(IX+o)", "19", "21", "5", "DD 56 o", "3"},
				{"S", "LD D,(IY+o)", "19", "21", "5", "FD 56 o", "3"},
				{"S", "LD D,IXH", "8", "10", "2", "DD 54", "2"},
				{"S", "LD D,IXL", "8", "10", "2", "DD 55", "2"},
				{"S", "LD D,IYH", "8", "10", "2", "FD 54", "2"},
				{"S", "LD D,IYL", "8", "10", "2", "FD 55", "2"},
				{"S", "LD D,n", "7", "8", "2", "16 n", "2"},
				{"S", "LD D,B", "4", "5", "1", "50", "1"},
				{"S", "LD D,C", "4", "5", "1", "51", "1"},
				{"S", "LD D,D", "4", "5", "1", "52", "1"},
				{"S", "LD D,E", "4", "5", "1", "53", "1"},
				{"S", "LD D,H", "4", "5", "1", "54", "1"},
				{"S", "LD D,L", "4", "5", "1", "55", "1"},
				{"S", "LD D,A", "4", "5", "1", "57", "1"},
				{"S", "LD DE,(nn)", "20", "22", "6", "ED 5B nn nn", "4"},
				{"S", "LD DE,nn", "10", "11", "3", "11 nn nn", "3"},
				{"S", "LD E,(HL)", "7", "8", "2", "5E", "1"},
				{"S", "LD E,(IX+o)", "19", "21", "5", "DD 5E o", "3"},
				{"S", "LD E,(IY+o)", "19", "21", "5", "FD 5E o", "3"},
				{"S", "LD E,IXH", "8", "10", "2", "DD 5C", "2"},
				{"S", "LD E,IXL", "8", "10", "2", "DD 5D", "2"},
				{"S", "LD E,IYH", "8", "10", "2", "FD 5C", "2"},
				{"S", "LD E,IYL", "8", "10", "2", "FD 5D", "2"},
				{"S", "LD E,n", "7", "8", "2", "1E n", "2"},
				{"S", "LD E,B", "4", "5", "1", "58", "1"},
				{"S", "LD E,C", "4", "5", "1", "59", "1"},
				{"S", "LD E,D", "4", "5", "1", "5A", "1"},
				{"S", "LD E,E", "4", "5", "1", "5B", "1"},
				{"S", "LD E,H", "4", "5", "1", "5C", "1"},
				{"S", "LD E,L", "4", "5", "1", "5D", "1"},
				{"S", "LD E,A", "4", "5", "1", "5F", "1"},
				{"S", "LD H,(HL)", "7", "8", "2", "66", "1"},
				{"S", "LD H,(IX+o)", "19", "21", "5", "DD 66 o", "3"},
				{"S", "LD H,(IY+o)", "19", "21", "5", "FD 66 o", "3"},
				{"S", "LD H,n", "7", "8", "2", "26 n", "2"},
				{"S", "LD H,B", "4", "5", "1", "60", "1"},
				{"S", "LD H,C", "4", "5", "1", "61", "1"},
				{"S", "LD H,D", "4", "5", "1", "62", "1"},
				{"S", "LD H,E", "4", "5", "1", "63", "1"},
				{"S", "LD H,H", "4", "5", "1", "64", "1"},
				{"S", "LD H,L", "4", "5", "1", "65", "1"},
				{"S", "LD H,A", "4", "5", "1", "67", "1"},
				{"S", "LD HL,(nn)", "16", "17", "5", "2A nn nn", "3"},
				{"S", "LD HL,nn", "10", "11", "3", "21 nn nn", "3"},
				{"S", "LD I,A", "9", "11", "3", "ED 47", "2"},
				{"S", "LD IX,(nn)", "20", "22", "6", "DD 2A nn nn", "4"},
				{"S", "LD IX,nn", "14", "16", "4", "DD 21 nn nn", "4"},
				{"S", "LD IXH,n", "11", "13", "3", "DD 26 n", "3"},
				{"S", "LD IXH,B", "8", "10", "2", "DD 60", "2"},
				{"S", "LD IXH,C", "8", "10", "2", "DD 61", "2"},
				{"S", "LD IXH,D", "8", "10", "2", "DD 62", "2"},
				{"S", "LD IXH,E", "8", "10", "2", "DD 63", "2"},
				{"S", "LD IXH,IXH", "8", "10", "2", "DD 64", "2"},
				{"S", "LD IXH,IXL", "8", "10", "2", "DD 65", "2"},
				{"S", "LD IXH,A", "8", "10", "2", "DD 67", "2"},
				{"S", "LD IXL,n", "11", "13", "3", "DD 2E n", "3"},
				{"S", "LD IXL,B", "8", "10", "2", "DD 68", "2"},
				{"S", "LD IXL,C", "8", "10", "2", "DD 69", "2"},
				{"S", "LD IXL,D", "8", "10", "2", "DD 6A", "2"},
				{"S", "LD IXL,E", "8", "10", "2", "DD 6B", "2"},
				{"S", "LD IXL,IXH", "8", "10", "2", "DD 6C", "2"},
				{"S", "LD IXL,IXL", "8", "10", "2", "DD 6D", "2"},
				{"S", "LD IXL,A", "8", "10", "2", "DD 6F", "2"},
				{"S", "LD IY,(nn)", "20", "22", "6", "FD 2A nn nn", "4"},
				{"S", "LD IY,nn", "14", "16", "4", "FD 21 nn nn", "4"},
				{"S", "LD IYH,n", "11", "13", "3", "FD 26 n", "3"},
				{"S", "LD IYH,B", "8", "10", "2", "FD 60", "2"},
				{"S", "LD IYH,C", "8", "10", "2", "FD 61", "2"},
				{"S", "LD IYH,D", "8", "10", "2", "FD 62", "2"},
				{"S", "LD IYH,E", "8", "10", "2", "FD 63", "2"},
				{"S", "LD IYH,IYH", "8", "10", "2", "FD 64", "2"},
				{"S", "LD IYH,IYL", "8", "10", "2", "FD 65", "2"},
				{"S", "LD IYH,A", "8", "10", "2", "FD 67", "2"},
				{"S", "LD IYL,n", "11", "13", "3", "FD 2E n", "3"},
				{"S", "LD IYL,B", "8", "10", "2", "FD 68", "2"},
				{"S", "LD IYL,C", "8", "10", "2", "FD 69", "2"},
				{"S", "LD IYL,D", "8", "10", "2", "FD 6A", "2"},
				{"S", "LD IYL,E", "8", "10", "2", "FD 6B", "2"},
				{"S", "LD IYL,IYH", "8", "10", "2", "FD 6C", "2"},
				{"S", "LD IYL,IYL", "8", "10", "2", "FD 6D", "2"},
				{"S", "LD IYL,A", "8", "10", "2", "FD 6F", "2"},
				{"S", "LD L,(HL)", "7", "8", "2", "6E", "1"},
				{"S", "LD L,(IX+o)", "19", "21", "5", "DD 6E o", "3"},
				{"S", "LD L,(IY+o)", "19", "21", "5", "FD 6E o", "3"},
				{"S", "LD L,n", "7", "8", "2", "2E n", "2"},
				{"S", "LD L,B", "4", "5", "1", "68", "1"},
				{"S", "LD L,C", "4", "5", "1", "69", "1"},
				{"S", "LD L,D", "4", "5", "1", "6A", "1"},
				{"S", "LD L,E", "4", "5", "1", "6B", "1"},
				{"S", "LD L,H", "4", "5", "1", "6C", "1"},
				{"S", "LD L,L", "4", "5", "1", "6D", "1"},
				{"S", "LD L,A", "4", "5", "1", "6F", "1"},
				{"S", "LD R,A", "9", "11", "3", "ED 4F", "2"},
				{"S", "LD SP,(nn)", "20", "22", "6", "ED 7B nn nn", "4"},
				{"S", "LD SP,HL", "6", "7", "2", "F9", "1"},
				{"S", "LD SP,IX", "10", "12", "3", "DD F9", "2"},
				{"S", "LD SP,IY", "10", "12", "3", "FD F9", "2"},
				{"S", "LD SP,nn", "10", "11", "3", "31 nn nn", "3"},
				{"S", "LDD", "16", "18", "5", "ED A8", "2"},
				{"S", "LDDR", "21/16", "23/18", "6/5", "ED B8", "2"},
				{"S", "LDI", "16", "18", "5", "ED A0", "2"},
				{"S", "LDIR", "21/16", "23/18", "6/5", "ED B0", "2"},
				{"S", "NEG", "8", "10", "2", "ED 44", "2"},
				{"S", "NOP", "4", "5", "1", "00", "1"},
				{"S", "OR (HL)", "7", "8", "2", "B6", "1"},
				{"S", "OR (IX+o)", "19", "21", "5", "DD B6 o", "3"},
				{"S", "OR (IY+o)", "19", "21", "5", "FD B6 o", "3"},
				{"S", "OR IXH", "8", "10", "2", "DD B4", "2"},
				{"S", "OR IXL", "8", "10", "2", "DD B5", "2"},
				{"S", "OR IYH", "8", "10", "2", "FD B4", "2"},
				{"S", "OR IYL", "8", "10", "2", "FD B5", "2"},
				{"S", "OR n", "7", "8", "2", "F6 n", "2"},
				{"S", "OR B", "4", "5", "1", "B0", "1"},
				{"S", "OR C", "4", "5", "1", "B1", "1"},
				{"S", "OR D", "4", "5", "1", "B2", "1"},
				{"S", "OR E", "4", "5", "1", "B3", "1"},
				{"S", "OR H", "4", "5", "1", "B4", "1"},
				{"S", "OR L", "4", "5", "1", "B5", "1"},
				{"S", "OR A", "4", "5", "1", "B7", "1"},
				{"S", "OTDR", "21/16", "23/18", "6/5", "ED BB", "2"},
				{"S", "OTIR", "21/16", "23/18", "6/5", "ED B3", "2"},
				{"S", "OUT (C),0", "12", "14", "4", "ED 71", "2"},
				{"S", "OUT (C),A", "12", "14", "4", "ED 79", "2"},
				{"S", "OUT (C),B", "12", "14", "4", "ED 41", "2"},
				{"S", "OUT (C),C", "12", "14", "4", "ED 49", "2"},
				{"S", "OUT (C),D", "12", "14", "4", "ED 51", "2"},
				{"S", "OUT (C),E", "12", "14", "4", "ED 59", "2"},
				{"S", "OUT (C),H", "12", "14", "4", "ED 61", "2"},
				{"S", "OUT (C),L", "12", "14", "4", "ED 69", "2"},
				{"S", "OUT (n),A", "11", "12", "3", "D3 n", "2"},
				{"S", "OUTD", "16", "18", "5", "ED AB", "2"},
				{"S", "OUTI", "16", "18", "5", "ED A3", "2"},
				{"S", "POP AF", "10", "11", "3", "F1", "1"},
				{"S", "POP BC", "10", "11", "3", "C1", "1"},
				{"S", "POP DE", "10", "11", "3", "D1", "1"},
				{"S", "POP HL", "10", "11", "3", "E1", "1"},
				{"S", "POP IX", "14", "16", "4", "DD E1", "2"},
				{"S", "POP IY", "14", "16", "4", "FD E1", "2"},
				{"S", "PUSH AF", "11", "12", "4", "F5", "1"},
				{"S", "PUSH BC", "11", "12", "4", "C5", "1"},
				{"S", "PUSH DE", "11", "12", "4", "D5", "1"},
				{"S", "PUSH HL", "11", "12", "4", "E5", "1"},
				{"S", "PUSH IX", "15", "17", "5", "DD E5", "2"},
				{"S", "PUSH IY", "15", "17", "5", "FD E5", "2"},
				{"S", "RES 0,(HL)", "15", "17", "4", "CB 86", "2"},
				{"S", "RES 1,(HL)", "15", "17", "4", "CB 8E", "2"},
				{"S", "RES 2,(HL)", "15", "17", "4", "CB 96", "2"},
				{"S", "RES 3,(HL)", "15", "17", "4", "CB 9E", "2"},
				{"S", "RES 4,(HL)", "15", "17", "4", "CB A6", "2"},
				{"S", "RES 5,(HL)", "15", "17", "4", "CB AE", "2"},
				{"S", "RES 6,(HL)", "15", "17", "4", "CB B6", "2"},
				{"S", "RES 7,(HL)", "15", "17", "4", "CB BE", "2"},
				{"S", "RES 0,(IX+o)", "23", "25", "7", "DD CB o 86", "4"},
				{"S", "RES 0,(IX+o),B", "23", "25", "7", "DD CB o 80", "4"},
				{"S", "RES 0,(IX+o),C", "23", "25", "7", "DD CB o 81", "4"},
				{"S", "RES 0,(IX+o),D", "23", "25", "7", "DD CB o 82", "4"},
				{"S", "RES 0,(IX+o),E", "23", "25", "7", "DD CB o 83", "4"},
				{"S", "RES 0,(IX+o),H", "23", "25", "7", "DD CB o 84", "4"},
				{"S", "RES 0,(IX+o),L", "23", "25", "7", "DD CB o 85", "4"},
				{"S", "RES 0,(IX+o),A", "23", "25", "7", "DD CB o 87", "4"},
				{"S", "RES 1,(IX+o)", "23", "25", "7", "DD CB o 8E", "4"},
				{"S", "RES 1,(IX+o),B", "23", "25", "7", "DD CB o 88", "4"},
				{"S", "RES 1,(IX+o),C", "23", "25", "7", "DD CB o 89", "4"},
				{"S", "RES 1,(IX+o),D", "23", "25", "7", "DD CB o 8A", "4"},
				{"S", "RES 1,(IX+o),E", "23", "25", "7", "DD CB o 8B", "4"},
				{"S", "RES 1,(IX+o),H", "23", "25", "7", "DD CB o 8C", "4"},
				{"S", "RES 1,(IX+o),L", "23", "25", "7", "DD CB o 8D", "4"},
				{"S", "RES 1,(IX+o),A", "23", "25", "7", "DD CB o 8F", "4"},
				{"S", "RES 2,(IX+o)", "23", "25", "7", "DD CB o 96", "4"},
				{"S", "RES 2,(IX+o),B", "23", "25", "7", "DD CB o 90", "4"},
				{"S", "RES 2,(IX+o),C", "23", "25", "7", "DD CB o 91", "4"},
				{"S", "RES 2,(IX+o),D", "23", "25", "7", "DD CB o 92", "4"},
				{"S", "RES 2,(IX+o),E", "23", "25", "7", "DD CB o 93", "4"},
				{"S", "RES 2,(IX+o),H", "23", "25", "7", "DD CB o 94", "4"},
				{"S", "RES 2,(IX+o),L", "23", "25", "7", "DD CB o 95", "4"},
				{"S", "RES 2,(IX+o),A", "23", "25", "7", "DD CB o 97", "4"},
				{"S", "RES 3,(IX+o)", "23", "25", "7", "DD CB o 9E", "4"},
				{"S", "RES 3,(IX+o),B", "23", "25", "7", "DD CB o 98", "4"},
				{"S", "RES 3,(IX+o),C", "23", "25", "7", "DD CB o 99", "4"},
				{"S", "RES 3,(IX+o),D", "23", "25", "7", "DD CB o 9A", "4"},
				{"S", "RES 3,(IX+o),E", "23", "25", "7", "DD CB o 9B", "4"},
				{"S", "RES 3,(IX+o),H", "23", "25", "7", "DD CB o 9C", "4"},
				{"S", "RES 3,(IX+o),L", "23", "25", "7", "DD CB o 9D", "4"},
				{"S", "RES 3,(IX+o),A", "23", "25", "7", "DD CB o 9F", "4"},
				{"S", "RES 4,(IX+o)", "23", "25", "7", "DD CB o A6", "4"},
				{"S", "RES 4,(IX+o),B", "23", "25", "7", "DD CB o A0", "4"},
				{"S", "RES 4,(IX+o),C", "23", "25", "7", "DD CB o A1", "4"},
				{"S", "RES 4,(IX+o),D", "23", "25", "7", "DD CB o A2", "4"},
				{"S", "RES 4,(IX+o),E", "23", "25", "7", "DD CB o A3", "4"},
				{"S", "RES 4,(IX+o),H", "23", "25", "7", "DD CB o A4", "4"},
				{"S", "RES 4,(IX+o),L", "23", "25", "7", "DD CB o A5", "4"},
				{"S", "RES 4,(IX+o),A", "23", "25", "7", "DD CB o A7", "4"},
				{"S", "RES 5,(IX+o)", "23", "25", "7", "DD CB o AE", "4"},
				{"S", "RES 5,(IX+o),B", "23", "25", "7", "DD CB o A8", "4"},
				{"S", "RES 5,(IX+o),C", "23", "25", "7", "DD CB o A9", "4"},
				{"S", "RES 5,(IX+o),D", "23", "25", "7", "DD CB o AA", "4"},
				{"S", "RES 5,(IX+o),E", "23", "25", "7", "DD CB o AB", "4"},
				{"S", "RES 5,(IX+o),H", "23", "25", "7", "DD CB o AC", "4"},
				{"S", "RES 5,(IX+o),L", "23", "25", "7", "DD CB o AD", "4"},
				{"S", "RES 5,(IX+o),A", "23", "25", "7", "DD CB o AF", "4"},
				{"S", "RES 6,(IX+o)", "23", "25", "7", "DD CB o B6", "4"},
				{"S", "RES 6,(IX+o),B", "23", "25", "7", "DD CB o B0", "4"},
				{"S", "RES 6,(IX+o),C", "23", "25", "7", "DD CB o B1", "4"},
				{"S", "RES 6,(IX+o),D", "23", "25", "7", "DD CB o B2", "4"},
				{"S", "RES 6,(IX+o),E", "23", "25", "7", "DD CB o B3", "4"},
				{"S", "RES 6,(IX+o),H", "23", "25", "7", "DD CB o B4", "4"},
				{"S", "RES 6,(IX+o),L", "23", "25", "7", "DD CB o B5", "4"},
				{"S", "RES 6,(IX+o),A", "23", "25", "7", "DD CB o B7", "4"},
				{"S", "RES 7,(IX+o)", "23", "25", "7", "DD CB o BE", "4"},
				{"S", "RES 7,(IX+o),B", "23", "25", "7", "DD CB o B8", "4"},
				{"S", "RES 7,(IX+o),C", "23", "25", "7", "DD CB o B9", "4"},
				{"S", "RES 7,(IX+o),D", "23", "25", "7", "DD CB o BA", "4"},
				{"S", "RES 7,(IX+o),E", "23", "25", "7", "DD CB o BB", "4"},
				{"S", "RES 7,(IX+o),H", "23", "25", "7", "DD CB o BC", "4"},
				{"S", "RES 7,(IX+o),L", "23", "25", "7", "DD CB o BD", "4"},
				{"S", "RES 7,(IX+o),A", "23", "25", "7", "DD CB o BF", "4"},
				{"S", "RES 0,(IY+o)", "23", "25", "7", "FD CB o 86", "4"},
				{"S", "RES 0,(IY+o),B", "23", "25", "7", "FD CB o 80", "4"},
				{"S", "RES 0,(IY+o),C", "23", "25", "7", "FD CB o 81", "4"},
				{"S", "RES 0,(IY+o),D", "23", "25", "7", "FD CB o 82", "4"},
				{"S", "RES 0,(IY+o),E", "23", "25", "7", "FD CB o 83", "4"},
				{"S", "RES 0,(IY+o),H", "23", "25", "7", "FD CB o 84", "4"},
				{"S", "RES 0,(IY+o),L", "23", "25", "7", "FD CB o 85", "4"},
				{"S", "RES 0,(IY+o),A", "23", "25", "7", "FD CB o 87", "4"},
				{"S", "RES 1,(IY+o)", "23", "25", "7", "FD CB o 8E", "4"},
				{"S", "RES 1,(IY+o),B", "23", "25", "7", "FD CB o 88", "4"},
				{"S", "RES 1,(IY+o),C", "23", "25", "7", "FD CB o 89", "4"},
				{"S", "RES 1,(IY+o),D", "23", "25", "7", "FD CB o 8A", "4"},
				{"S", "RES 1,(IY+o),E", "23", "25", "7", "FD CB o 8B", "4"},
				{"S", "RES 1,(IY+o),H", "23", "25", "7", "FD CB o 8C", "4"},
				{"S", "RES 1,(IY+o),L", "23", "25", "7", "FD CB o 8D", "4"},
				{"S", "RES 1,(IY+o),A", "23", "25", "7", "FD CB o 8F", "4"},
				{"S", "RES 2,(IY+o)", "23", "25", "7", "FD CB o 96", "4"},
				{"S", "RES 2,(IY+o),B", "23", "25", "7", "FD CB o 90", "4"},
				{"S", "RES 2,(IY+o),C", "23", "25", "7", "FD CB o 91", "4"},
				{"S", "RES 2,(IY+o),D", "23", "25", "7", "FD CB o 92", "4"},
				{"S", "RES 2,(IY+o),E", "23", "25", "7", "FD CB o 93", "4"},
				{"S", "RES 2,(IY+o),H", "23", "25", "7", "FD CB o 94", "4"},
				{"S", "RES 2,(IY+o),L", "23", "25", "7", "FD CB o 95", "4"},
				{"S", "RES 2,(IY+o),A", "23", "25", "7", "FD CB o 97", "4"},
				{"S", "RES 3,(IY+o)", "23", "25", "7", "FD CB o 9E", "4"},
				{"S", "RES 3,(IY+o),B", "23", "25", "7", "FD CB o 98", "4"},
				{"S", "RES 3,(IY+o),C", "23", "25", "7", "FD CB o 99", "4"},
				{"S", "RES 3,(IY+o),D", "23", "25", "7", "FD CB o 9A", "4"},
				{"S", "RES 3,(IY+o),E", "23", "25", "7", "FD CB o 9B", "4"},
				{"S", "RES 3,(IY+o),H", "23", "25", "7", "FD CB o 9C", "4"},
				{"S", "RES 3,(IY+o),L", "23", "25", "7", "FD CB o 9D", "4"},
				{"S", "RES 3,(IY+o),A", "23", "25", "7", "FD CB o 9F", "4"},
				{"S", "RES 4,(IY+o)", "23", "25", "7", "FD CB o A6", "4"},
				{"S", "RES 4,(IY+o),B", "23", "25", "7", "FD CB o A0", "4"},
				{"S", "RES 4,(IY+o),C", "23", "25", "7", "FD CB o A1", "4"},
				{"S", "RES 4,(IY+o),D", "23", "25", "7", "FD CB o A2", "4"},
				{"S", "RES 4,(IY+o),E", "23", "25", "7", "FD CB o A3", "4"},
				{"S", "RES 4,(IY+o),H", "23", "25", "7", "FD CB o A4", "4"},
				{"S", "RES 4,(IY+o),L", "23", "25", "7", "FD CB o A5", "4"},
				{"S", "RES 4,(IY+o),A", "23", "25", "7", "FD CB o A7", "4"},
				{"S", "RES 5,(IY+o)", "23", "25", "7", "FD CB o AE", "4"},
				{"S", "RES 5,(IY+o),B", "23", "25", "7", "FD CB o A8", "4"},
				{"S", "RES 5,(IY+o),C", "23", "25", "7", "FD CB o A9", "4"},
				{"S", "RES 5,(IY+o),D", "23", "25", "7", "FD CB o AA", "4"},
				{"S", "RES 5,(IY+o),E", "23", "25", "7", "FD CB o AB", "4"},
				{"S", "RES 5,(IY+o),H", "23", "25", "7", "FD CB o AC", "4"},
				{"S", "RES 5,(IY+o),L", "23", "25", "7", "FD CB o AD", "4"},
				{"S", "RES 5,(IY+o),A", "23", "25", "7", "FD CB o AF", "4"},
				{"S", "RES 6,(IY+o)", "23", "25", "7", "FD CB o B6", "4"},
				{"S", "RES 6,(IY+o),B", "23", "25", "7", "FD CB o B0", "4"},
				{"S", "RES 6,(IY+o),C", "23", "25", "7", "FD CB o B1", "4"},
				{"S", "RES 6,(IY+o),D", "23", "25", "7", "FD CB o B2", "4"},
				{"S", "RES 6,(IY+o),E", "23", "25", "7", "FD CB o B3", "4"},
				{"S", "RES 6,(IY+o),H", "23", "25", "7", "FD CB o B4", "4"},
				{"S", "RES 6,(IY+o),L", "23", "25", "7", "FD CB o B5", "4"},
				{"S", "RES 6,(IY+o),A", "23", "25", "7", "FD CB o B7", "4"},
				{"S", "RES 7,(IY+o)", "23", "25", "7", "FD CB o BE", "4"},
				{"S", "RES 7,(IY+o),B", "23", "25", "7", "FD CB o B8", "4"},
				{"S", "RES 7,(IY+o),C", "23", "25", "7", "FD CB o B9", "4"},
				{"S", "RES 7,(IY+o),D", "23", "25", "7", "FD CB o BA", "4"},
				{"S", "RES 7,(IY+o),E", "23", "25", "7", "FD CB o BB", "4"},
				{"S", "RES 7,(IY+o),H", "23", "25", "7", "FD CB o BC", "4"},
				{"S", "RES 7,(IY+o),L", "23", "25", "7", "FD CB o BD", "4"},
				{"S", "RES 7,(IY+o),A", "23", "25", "7", "FD CB o BF", "4"},
				{"S", "RES 0,B", "8", "10", "2", "CB 80", "2"},
				{"S", "RES 0,C", "8", "10", "2", "CB 81", "2"},
				{"S", "RES 0,D", "8", "10", "2", "CB 82", "2"},
				{"S", "RES 0,E", "8", "10", "2", "CB 83", "2"},
				{"S", "RES 0,H", "8", "10", "2", "CB 84", "2"},
				{"S", "RES 0,L", "8", "10", "2", "CB 85", "2"},
				{"S", "RES 0,A", "8", "10", "2", "CB 87", "2"},
				{"S", "RES 1,B", "8", "10", "2", "CB 88", "2"},
				{"S", "RES 1,C", "8", "10", "2", "CB 89", "2"},
				{"S", "RES 1,D", "8", "10", "2", "CB 8A", "2"},
				{"S", "RES 1,E", "8", "10", "2", "CB 8B", "2"},
				{"S", "RES 1,H", "8", "10", "2", "CB 8C", "2"},
				{"S", "RES 1,L", "8", "10", "2", "CB 8D", "2"},
				{"S", "RES 1,A", "8", "10", "2", "CB 8F", "2"},
				{"S", "RES 2,B", "8", "10", "2", "CB 90", "2"},
				{"S", "RES 2,C", "8", "10", "2", "CB 91", "2"},
				{"S", "RES 2,D", "8", "10", "2", "CB 92", "2"},
				{"S", "RES 2,E", "8", "10", "2", "CB 93", "2"},
				{"S", "RES 2,H", "8", "10", "2", "CB 94", "2"},
				{"S", "RES 2,L", "8", "10", "2", "CB 95", "2"},
				{"S", "RES 2,A", "8", "10", "2", "CB 97", "2"},
				{"S", "RES 3,B", "8", "10", "2", "CB 98", "2"},
				{"S", "RES 3,C", "8", "10", "2", "CB 99", "2"},
				{"S", "RES 3,D", "8", "10", "2", "CB 9A", "2"},
				{"S", "RES 3,E", "8", "10", "2", "CB 9B", "2"},
				{"S", "RES 3,H", "8", "10", "2", "CB 9C", "2"},
				{"S", "RES 3,L", "8", "10", "2", "CB 9D", "2"},
				{"S", "RES 3,A", "8", "10", "2", "CB 9F", "2"},
				{"S", "RES 4,B", "8", "10", "2", "CB A0", "2"},
				{"S", "RES 4,C", "8", "10", "2", "CB A1", "2"},
				{"S", "RES 4,D", "8", "10", "2", "CB A2", "2"},
				{"S", "RES 4,E", "8", "10", "2", "CB A3", "2"},
				{"S", "RES 4,H", "8", "10", "2", "CB A4", "2"},
				{"S", "RES 4,L", "8", "10", "2", "CB A5", "2"},
				{"S", "RES 4,A", "8", "10", "2", "CB A7", "2"},
				{"S", "RES 5,B", "8", "10", "2", "CB A8", "2"},
				{"S", "RES 5,C", "8", "10", "2", "CB A9", "2"},
				{"S", "RES 5,D", "8", "10", "2", "CB AA", "2"},
				{"S", "RES 5,E", "8", "10", "2", "CB AB", "2"},
				{"S", "RES 5,H", "8", "10", "2", "CB AC", "2"},
				{"S", "RES 5,L", "8", "10", "2", "CB AD", "2"},
				{"S", "RES 5,A", "8", "10", "2", "CB AF", "2"},
				{"S", "RES 6,B", "8", "10", "2", "CB B0", "2"},
				{"S", "RES 6,C", "8", "10", "2", "CB B1", "2"},
				{"S", "RES 6,D", "8", "10", "2", "CB B2", "2"},
				{"S", "RES 6,E", "8", "10", "2", "CB B3", "2"},
				{"S", "RES 6,H", "8", "10", "2", "CB B4", "2"},
				{"S", "RES 6,L", "8", "10", "2", "CB B5", "2"},
				{"S", "RES 6,A", "8", "10", "2", "CB B7", "2"},
				{"S", "RES 7,B", "8", "10", "2", "CB B8", "2"},
				{"S", "RES 7,C", "8", "10", "2", "CB B9", "2"},
				{"S", "RES 7,D", "8", "10", "2", "CB BA", "2"},
				{"S", "RES 7,E", "8", "10", "2", "CB BB", "2"},
				{"S", "RES 7,H", "8", "10", "2", "CB BC", "2"},
				{"S", "RES 7,L", "8", "10", "2", "CB BD", "2"},
				{"S", "RES 7,A", "8", "10", "2", "CB BF", "2"},
				{"S", "RET", "10", "11", "3", "C9", "1"},
				{"S", "RET C", "11/5", "12/6", "4/2", "D8", "1"},
				{"S", "RET M", "11/5", "12/6", "4/2", "F8", "1"},
				{"S", "RET NC", "11/5", "12/6", "4/2", "D0", "1"},
				{"S", "RET NZ", "11/5", "12/6", "4/2", "C0", "1"},
				{"S", "RET P", "11/5", "12/6", "4/2", "F0", "1"},
				{"S", "RET PE", "11/5", "12/6", "4/2", "E8", "1"},
				{"S", "RET PO", "11/5", "12/6", "4/2", "E0", "1"},
				{"S", "RET Z", "11/5", "12/6", "4/2", "C8", "1"},
				{"S", "RETI", "14", "16", "4", "ED 4D", "2"},
				{"S", "RETN", "14", "16", "4", "ED 45", "2"},
				{"S", "RL (HL)", "15", "17", "4", "CB 16", "2"},
				{"S", "RL (IX+o)", "23", "25", "7", "DD CB o 16", "4"},
				{"S", "RL (IX+o),B", "23", "25", "7", "DD CB o 10", "4"},
				{"S", "RL (IX+o),C", "23", "25", "7", "DD CB o 11", "4"},
				{"S", "RL (IX+o),D", "23", "25", "7", "DD CB o 12", "4"},
				{"S", "RL (IX+o),E", "23", "25", "7", "DD CB o 13", "4"},
				{"S", "RL (IX+o),H", "23", "25", "7", "DD CB o 14", "4"},
				{"S", "RL (IX+o),L", "23", "25", "7", "DD CB o 15", "4"},
				{"S", "RL (IX+o),A", "23", "25", "7", "DD CB o 17", "4"},
				{"S", "RL (IY+o)", "23", "25", "7", "FD CB o 16", "4"},
				{"S", "RL (IY+o),B", "23", "25", "7", "FD CB o 10", "4"},
				{"S", "RL (IY+o),C", "23", "25", "7", "FD CB o 11", "4"},
				{"S", "RL (IY+o),D", "23", "25", "7", "FD CB o 12", "4"},
				{"S", "RL (IY+o),E", "23", "25", "7", "FD CB o 13", "4"},
				{"S", "RL (IY+o),H", "23", "25", "7", "FD CB o 14", "4"},
				{"S", "RL (IY+o),L", "23", "25", "7", "FD CB o 15", "4"},
				{"S", "RL (IY+o),A", "23", "25", "7", "FD CB o 17", "4"},
				{"S", "RL B", "8", "10", "2", "CB 10", "2"},
				{"S", "RL C", "8", "10", "2", "CB 11", "2"},
				{"S", "RL D", "8", "10", "2", "CB 12", "2"},
				{"S", "RL E", "8", "10", "2", "CB 13", "2"},
				{"S", "RL H", "8", "10", "2", "CB 14", "2"},
				{"S", "RL L", "8", "10", "2", "CB 15", "2"},
				{"S", "RL A", "8", "10", "2", "CB 17", "2"},
				{"S", "RLA", "4", "5", "1", "17", "1"},
				{"S", "RLC (HL)", "15", "17", "4", "CB 06", "2"},
				{"S", "RLC (IX+o)", "23", "25", "7", "DD CB o 06", "4"},
				{"S", "RLC (IX+o),B", "23", "25", "7", "DD CB o 00", "4"},
				{"S", "RLC (IX+o),C", "23", "25", "7", "DD CB o 01", "4"},
				{"S", "RLC (IX+o),D", "23", "25", "7", "DD CB o 02", "4"},
				{"S", "RLC (IX+o),E", "23", "25", "7", "DD CB o 03", "4"},
				{"S", "RLC (IX+o),H", "23", "25", "7", "DD CB o 04", "4"},
				{"S", "RLC (IX+o),L", "23", "25", "7", "DD CB o 05", "4"},
				{"S", "RLC (IX+o),A", "23", "25", "7", "DD CB o 07", "4"},
				{"S", "RLC (IY+o)", "23", "25", "7", "FD CB o 06", "4"},
				{"S", "RLC (IY+o),B", "23", "25", "7", "FD CB o 00", "4"},
				{"S", "RLC (IY+o),C", "23", "25", "7", "FD CB o 01", "4"},
				{"S", "RLC (IY+o),D", "23", "25", "7", "FD CB o 02", "4"},
				{"S", "RLC (IY+o),E", "23", "25", "7", "FD CB o 03", "4"},
				{"S", "RLC (IY+o),H", "23", "25", "7", "FD CB o 04", "4"},
				{"S", "RLC (IY+o),L", "23", "25", "7", "FD CB o 05", "4"},
				{"S", "RLC (IY+o),A", "23", "25", "7", "FD CB o 07", "4"},
				{"S", "RLC B", "8", "10", "2", "CB 00", "2"},
				{"S", "RLC C", "8", "10", "2", "CB 01", "2"},
				{"S", "RLC D", "8", "10", "2", "CB 02", "2"},
				{"S", "RLC E", "8", "10", "2", "CB 03", "2"},
				{"S", "RLC H", "8", "10", "2", "CB 04", "2"},
				{"S", "RLC L", "8", "10", "2", "CB 05", "2"},
				{"S", "RLC A", "8", "10", "2", "CB 07", "2"},
				{"S", "RLCA", "4", "5", "1", "07", "1"},
				{"S", "RLD", "18", "20", "5", "ED 6F", "2"},
				{"S", "RR (HL)", "15", "17", "4", "CB 1E", "2"},
				{"S", "RR (IX+o)", "23", "25", "7", "DD CB o 1E", "4"},
				{"S", "RR (IX+o),B", "23", "25", "7", "DD CB o 18", "4"},
				{"S", "RR (IX+o),C", "23", "25", "7", "DD CB o 19", "4"},
				{"S", "RR (IX+o),D", "23", "25", "7", "DD CB o 1A", "4"},
				{"S", "RR (IX+o),E", "23", "25", "7", "DD CB o 1B", "4"},
				{"S", "RR (IX+o),H", "23", "25", "7", "DD CB o 1C", "4"},
				{"S", "RR (IX+o),L", "23", "25", "7", "DD CB o 1D", "4"},
				{"S", "RR (IX+o),A", "23", "25", "7", "DD CB o 1F", "4"},
				{"S", "RR (IY+o)", "23", "25", "7", "FD CB o 1E", "4"},
				{"S", "RR (IY+o),B", "23", "25", "7", "FD CB o 18", "4"},
				{"S", "RR (IY+o),C", "23", "25", "7", "FD CB o 19", "4"},
				{"S", "RR (IY+o),D", "23", "25", "7", "FD CB o 1A", "4"},
				{"S", "RR (IY+o),E", "23", "25", "7", "FD CB o 1B", "4"},
				{"S", "RR (IY+o),H", "23", "25", "7", "FD CB o 1C", "4"},
				{"S", "RR (IY+o),L", "23", "25", "7", "FD CB o 1D", "4"},
				{"S", "RR (IY+o),A", "23", "25", "7", "FD CB o 1F", "4"},
				{"S", "RR B", "8", "10", "2", "CB 18", "2"},
				{"S", "RR C", "8", "10", "2", "CB 19", "2"},
				{"S", "RR D", "8", "10", "2", "CB 1A", "2"},
				{"S", "RR E", "8", "10", "2", "CB 1B", "2"},
				{"S", "RR H", "8", "10", "2", "CB 1C", "2"},
				{"S", "RR L", "8", "10", "2", "CB 1D", "2"},
				{"S", "RR A", "8", "10", "2", "CB 1F", "2"},
				{"S", "RRA", "4", "5", "1", "1F", "1"},
				{"S", "RRC (HL)", "15", "17", "4", "CB 0E", "2"},
				{"S", "RRC (IX+o)", "23", "25", "7", "DD CB o 0E", "4"},
				{"S", "RRC (IX+o),B", "23", "25", "7", "DD CB o 08", "4"},
				{"S", "RRC (IX+o),C", "23", "25", "7", "DD CB o 09", "4"},
				{"S", "RRC (IX+o),D", "23", "25", "7", "DD CB o 0A", "4"},
				{"S", "RRC (IX+o),E", "23", "25", "7", "DD CB o 0B", "4"},
				{"S", "RRC (IX+o),H", "23", "25", "7", "DD CB o 0C", "4"},
				{"S", "RRC (IX+o),L", "23", "25", "7", "DD CB o 0D", "4"},
				{"S", "RRC (IX+o),A", "23", "25", "7", "DD CB o 0F", "4"},
				{"S", "RRC (IY+o)", "23", "25", "7", "FD CB o 0E", "4"},
				{"S", "RRC (IY+o),B", "23", "25", "7", "FD CB o 08", "4"},
				{"S", "RRC (IY+o),C", "23", "25", "7", "FD CB o 09", "4"},
				{"S", "RRC (IY+o),D", "23", "25", "7", "FD CB o 0A", "4"},
				{"S", "RRC (IY+o),E", "23", "25", "7", "FD CB o 0B", "4"},
				{"S", "RRC (IY+o),H", "23", "25", "7", "FD CB o 0C", "4"},
				{"S", "RRC (IY+o),L", "23", "25", "7", "FD CB o 0D", "4"},
				{"S", "RRC (IY+o),A", "23", "25", "7", "FD CB o 0F", "4"},
				{"S", "RRC B", "8", "10", "2", "CB 08", "2"},
				{"S", "RRC C", "8", "10", "2", "CB 09", "2"},
				{"S", "RRC D", "8", "10", "2", "CB 0A", "2"},
				{"S", "RRC E", "8", "10", "2", "CB 0B", "2"},
				{"S", "RRC H", "8", "10", "2", "CB 0C", "2"},
				{"S", "RRC L", "8", "10", "2", "CB 0D", "2"},
				{"S", "RRC A", "8", "10", "2", "CB 0F", "2"},
				{"S", "RRCA", "4", "5", "1", "0F", "1"},
				{"S", "RRD", "18", "20", "5", "ED 67", "2"},
				{"S", "RST 0", "11", "12", "4", "C7", "1"},
				{"S", "RST 8H", "11", "12", "4", "CF", "1"},
				{"S", "RST 10H", "11", "12", "4", "D7", "1"},
				{"S", "RST 18H", "11", "12", "4", "DF", "1"},
				{"S", "RST 20H", "11", "12", "4", "E7", "1"},
				{"S", "RST 28H", "11", "12", "4", "EF", "1"},
				{"S", "RST 30H", "11", "12", "4", "F7", "1"},
				{"S", "RST 38H", "11", "12", "4", "FF", "1"},
				{"S", "SBC A,(HL)", "7", "8", "2", "9E", "1"},
				{"S", "SBC A,(IX+o)", "19", "21", "5", "DD 9E o", "3"},
				{"S", "SBC A,(IY+o)", "19", "21", "5", "FD 9E o", "3"},
				{"S", "SBC A,IXH", "8", "10", "2", "DD 9C", "2"},
				{"S", "SBC A,IXL", "8", "10", "2", "DD 9D", "2"},
				{"S", "SBC A,IYH", "8", "10", "2", "FD 9C", "2"},
				{"S", "SBC A,IYL", "8", "10", "2", "FD 9D", "2"},
				{"S", "SBC A,n", "7", "8", "2", "DE n", "2"},
				{"S", "SBC A,B", "4", "5", "1", "98", "1"},
				{"S", "SBC A,C", "4", "5", "1", "99", "1"},
				{"S", "SBC A,D", "4", "5", "1", "9A", "1"},
				{"S", "SBC A,E", "4", "5", "1", "9B", "1"},
				{"S", "SBC A,H", "4", "5", "1", "9C", "1"},
				{"S", "SBC A,L", "4", "5", "1", "9D", "1"},
				{"S", "SBC A,A", "4", "5", "1", "9F", "1"},
				{"S", "SBC HL,BC", "15", "17", "4", "ED 42", "2"},
				{"S", "SBC HL,DE", "15", "17", "4", "ED 52", "2"},
				{"S", "SBC HL,HL", "15", "17", "4", "ED 62", "2"},
				{"S", "SBC HL,SP", "15", "17", "4", "ED 72", "2"},
				{"S", "SCF", "4", "5", "1", "37", "1"},
				{"S", "SET 0,(HL)", "15", "17", "4", "CB C6", "2"},
				{"S", "SET 1,(HL)", "15", "17", "4", "CB CE", "2"},
				{"S", "SET 2,(HL)", "15", "17", "4", "CB D6", "2"},
				{"S", "SET 3,(HL)", "15", "17", "4", "CB DE", "2"},
				{"S", "SET 4,(HL)", "15", "17", "4", "CB E6", "2"},
				{"S", "SET 5,(HL)", "15", "17", "4", "CB EE", "2"},
				{"S", "SET 6,(HL)", "15", "17", "4", "CB F6", "2"},
				{"S", "SET 7,(HL)", "15", "17", "4", "CB FE", "2"},
				{"S", "SET 0,(IX+o)", "23", "25", "7", "DD CB o C6", "4"},
				{"S", "SET 0,(IX+o),B", "23", "25", "7", "DD CB o C0", "4"},
				{"S", "SET 0,(IX+o),C", "23", "25", "7", "DD CB o C1", "4"},
				{"S", "SET 0,(IX+o),D", "23", "25", "7", "DD CB o C2", "4"},
				{"S", "SET 0,(IX+o),E", "23", "25", "7", "DD CB o C3", "4"},
				{"S", "SET 0,(IX+o),H", "23", "25", "7", "DD CB o C4", "4"},
				{"S", "SET 0,(IX+o),L", "23", "25", "7", "DD CB o C5", "4"},
				{"S", "SET 0,(IX+o),A", "23", "25", "7", "DD CB o C7", "4"},
				{"S", "SET 1,(IX+o)", "23", "25", "7", "DD CB o CE", "4"},
				{"S", "SET 1,(IX+o),B", "23", "25", "7", "DD CB o C8", "4"},
				{"S", "SET 1,(IX+o),C", "23", "25", "7", "DD CB o C9", "4"},
				{"S", "SET 1,(IX+o),D", "23", "25", "7", "DD CB o CA", "4"},
				{"S", "SET 1,(IX+o),E", "23", "25", "7", "DD CB o CB", "4"},
				{"S", "SET 1,(IX+o),H", "23", "25", "7", "DD CB o CC", "4"},
				{"S", "SET 1,(IX+o),L", "23", "25", "7", "DD CB o CD", "4"},
				{"S", "SET 1,(IX+o),A", "23", "25", "7", "DD CB o CF", "4"},
				{"S", "SET 2,(IX+o)", "23", "25", "7", "DD CB o D6", "4"},
				{"S", "SET 2,(IX+o),B", "23", "25", "7", "DD CB o D0", "4"},
				{"S", "SET 2,(IX+o),C", "23", "25", "7", "DD CB o D1", "4"},
				{"S", "SET 2,(IX+o),D", "23", "25", "7", "DD CB o D2", "4"},
				{"S", "SET 2,(IX+o),E", "23", "25", "7", "DD CB o D3", "4"},
				{"S", "SET 2,(IX+o),H", "23", "25", "7", "DD CB o D4", "4"},
				{"S", "SET 2,(IX+o),L", "23", "25", "7", "DD CB o D5", "4"},
				{"S", "SET 2,(IX+o),A", "23", "25", "7", "DD CB o D7", "4"},
				{"S", "SET 3,(IX+o)", "23", "25", "7", "DD CB o DE", "4"},
				{"S", "SET 3,(IX+o),B", "23", "25", "7", "DD CB o D8", "4"},
				{"S", "SET 3,(IX+o),C", "23", "25", "7", "DD CB o D9", "4"},
				{"S", "SET 3,(IX+o),D", "23", "25", "7", "DD CB o DA", "4"},
				{"S", "SET 3,(IX+o),E", "23", "25", "7", "DD CB o DB", "4"},
				{"S", "SET 3,(IX+o),H", "23", "25", "7", "DD CB o DC", "4"},
				{"S", "SET 3,(IX+o),L", "23", "25", "7", "DD CB o DD", "4"},
				{"S", "SET 3,(IX+o),A", "23", "25", "7", "DD CB o DF", "4"},
				{"S", "SET 4,(IX+o)", "23", "25", "7", "DD CB o E6", "4"},
				{"S", "SET 4,(IX+o),B", "23", "25", "7", "DD CB o E0", "4"},
				{"S", "SET 4,(IX+o),C", "23", "25", "7", "DD CB o E1", "4"},
				{"S", "SET 4,(IX+o),D", "23", "25", "7", "DD CB o E2", "4"},
				{"S", "SET 4,(IX+o),E", "23", "25", "7", "DD CB o E3", "4"},
				{"S", "SET 4,(IX+o),H", "23", "25", "7", "DD CB o E4", "4"},
				{"S", "SET 4,(IX+o),L", "23", "25", "7", "DD CB o E5", "4"},
				{"S", "SET 4,(IX+o),A", "23", "25", "7", "DD CB o E7", "4"},
				{"S", "SET 5,(IX+o)", "23", "25", "7", "DD CB o EE", "4"},
				{"S", "SET 5,(IX+o),B", "23", "25", "7", "DD CB o E8", "4"},
				{"S", "SET 5,(IX+o),C", "23", "25", "7", "DD CB o E9", "4"},
				{"S", "SET 5,(IX+o),D", "23", "25", "7", "DD CB o EA", "4"},
				{"S", "SET 5,(IX+o),E", "23", "25", "7", "DD CB o EB", "4"},
				{"S", "SET 5,(IX+o),H", "23", "25", "7", "DD CB o EC", "4"},
				{"S", "SET 5,(IX+o),L", "23", "25", "7", "DD CB o ED", "4"},
				{"S", "SET 5,(IX+o),A", "23", "25", "7", "DD CB o EF", "4"},
				{"S", "SET 6,(IX+o)", "23", "25", "7", "DD CB o F6", "4"},
				{"S", "SET 6,(IX+o),B", "23", "25", "7", "DD CB o F0", "4"},
				{"S", "SET 6,(IX+o),C", "23", "25", "7", "DD CB o F1", "4"},
				{"S", "SET 6,(IX+o),D", "23", "25", "7", "DD CB o F2", "4"},
				{"S", "SET 6,(IX+o),E", "23", "25", "7", "DD CB o F3", "4"},
				{"S", "SET 6,(IX+o),H", "23", "25", "7", "DD CB o F4", "4"},
				{"S", "SET 6,(IX+o),L", "23", "25", "7", "DD CB o F5", "4"},
				{"S", "SET 6,(IX+o),A", "23", "25", "7", "DD CB o F7", "4"},
				{"S", "SET 7,(IX+o)", "23", "25", "7", "DD CB o FE", "4"},
				{"S", "SET 7,(IX+o),B", "23", "25", "7", "DD CB o F8", "4"},
				{"S", "SET 7,(IX+o),C", "23", "25", "7", "DD CB o F9", "4"},
				{"S", "SET 7,(IX+o),D", "23", "25", "7", "DD CB o FA", "4"},
				{"S", "SET 7,(IX+o),E", "23", "25", "7", "DD CB o FB", "4"},
				{"S", "SET 7,(IX+o),H", "23", "25", "7", "DD CB o FC", "4"},
				{"S", "SET 7,(IX+o),L", "23", "25", "7", "DD CB o FD", "4"},
				{"S", "SET 7,(IX+o),A", "23", "25", "7", "DD CB o FF", "4"},
				{"S", "SET 0,(IY+o)", "23", "25", "7", "FD CB o C6", "4"},
				{"S", "SET 0,(IY+o),B", "23", "25", "7", "FD CB o C0", "4"},
				{"S", "SET 0,(IY+o),C", "23", "25", "7", "FD CB o C1", "4"},
				{"S", "SET 0,(IY+o),D", "23", "25", "7", "FD CB o C2", "4"},
				{"S", "SET 0,(IY+o),E", "23", "25", "7", "FD CB o C3", "4"},
				{"S", "SET 0,(IY+o),H", "23", "25", "7", "FD CB o C4", "4"},
				{"S", "SET 0,(IY+o),L", "23", "25", "7", "FD CB o C5", "4"},
				{"S", "SET 0,(IY+o),A", "23", "25", "7", "FD CB o C7", "4"},
				{"S", "SET 1,(IY+o)", "23", "25", "7", "FD CB o CE", "4"},
				{"S", "SET 1,(IY+o),B", "23", "25", "7", "FD CB o C8", "4"},
				{"S", "SET 1,(IY+o),C", "23", "25", "7", "FD CB o C9", "4"},
				{"S", "SET 1,(IY+o),D", "23", "25", "7", "FD CB o CA", "4"},
				{"S", "SET 1,(IY+o),E", "23", "25", "7", "FD CB o CB", "4"},
				{"S", "SET 1,(IY+o),H", "23", "25", "7", "FD CB o CC", "4"},
				{"S", "SET 1,(IY+o),L", "23", "25", "7", "FD CB o CD", "4"},
				{"S", "SET 1,(IY+o),A", "23", "25", "7", "FD CB o CF", "4"},
				{"S", "SET 2,(IY+o)", "23", "25", "7", "FD CB o D6", "4"},
				{"S", "SET 2,(IY+o),B", "23", "25", "7", "FD CB o D0", "4"},
				{"S", "SET 2,(IY+o),C", "23", "25", "7", "FD CB o D1", "4"},
				{"S", "SET 2,(IY+o),D", "23", "25", "7", "FD CB o D2", "4"},
				{"S", "SET 2,(IY+o),E", "23", "25", "7", "FD CB o D3", "4"},
				{"S", "SET 2,(IY+o),H", "23", "25", "7", "FD CB o D4", "4"},
				{"S", "SET 2,(IY+o),L", "23", "25", "7", "FD CB o D5", "4"},
				{"S", "SET 2,(IY+o),A", "23", "25", "7", "FD CB o D7", "4"},
				{"S", "SET 3,(IY+o)", "23", "25", "7", "FD CB o DE", "4"},
				{"S", "SET 3,(IY+o),B", "23", "25", "7", "FD CB o D8", "4"},
				{"S", "SET 3,(IY+o),C", "23", "25", "7", "FD CB o D9", "4"},
				{"S", "SET 3,(IY+o),D", "23", "25", "7", "FD CB o DA", "4"},
				{"S", "SET 3,(IY+o),E", "23", "25", "7", "FD CB o DB", "4"},
				{"S", "SET 3,(IY+o),H", "23", "25", "7", "FD CB o DC", "4"},
				{"S", "SET 3,(IY+o),L", "23", "25", "7", "FD CB o DD", "4"},
				{"S", "SET 3,(IY+o),A", "23", "25", "7", "FD CB o DF", "4"},
				{"S", "SET 4,(IY+o)", "23", "25", "7", "FD CB o E6", "4"},
				{"S", "SET 4,(IY+o),B", "23", "25", "7", "FD CB o E0", "4"},
				{"S", "SET 4,(IY+o),C", "23", "25", "7", "FD CB o E1", "4"},
				{"S", "SET 4,(IY+o),D", "23", "25", "7", "FD CB o E2", "4"},
				{"S", "SET 4,(IY+o),E", "23", "25", "7", "FD CB o E3", "4"},
				{"S", "SET 4,(IY+o),H", "23", "25", "7", "FD CB o E4", "4"},
				{"S", "SET 4,(IY+o),L", "23", "25", "7", "FD CB o E5", "4"},
				{"S", "SET 4,(IY+o),A", "23", "25", "7", "FD CB o E7", "4"},
				{"S", "SET 5,(IY+o)", "23", "25", "7", "FD CB o EE", "4"},
				{"S", "SET 5,(IY+o),B", "23", "25", "7", "FD CB o E8", "4"},
				{"S", "SET 5,(IY+o),C", "23", "25", "7", "FD CB o E9", "4"},
				{"S", "SET 5,(IY+o),D", "23", "25", "7", "FD CB o EA", "4"},
				{"S", "SET 5,(IY+o),E", "23", "25", "7", "FD CB o EB", "4"},
				{"S", "SET 5,(IY+o),H", "23", "25", "7", "FD CB o EC", "4"},
				{"S", "SET 5,(IY+o),L", "23", "25", "7", "FD CB o ED", "4"},
				{"S", "SET 5,(IY+o),A", "23", "25", "7", "FD CB o EF", "4"},
				{"S", "SET 6,(IY+o)", "23", "25", "7", "FD CB o F6", "4"},
				{"S", "SET 6,(IY+o),B", "23", "25", "7", "FD CB o F0", "4"},
				{"S", "SET 6,(IY+o),C", "23", "25", "7", "FD CB o F1", "4"},
				{"S", "SET 6,(IY+o),D", "23", "25", "7", "FD CB o F2", "4"},
				{"S", "SET 6,(IY+o),E", "23", "25", "7", "FD CB o F3", "4"},
				{"S", "SET 6,(IY+o),H", "23", "25", "7", "FD CB o F4", "4"},
				{"S", "SET 6,(IY+o),L", "23", "25", "7", "FD CB o F5", "4"},
				{"S", "SET 6,(IY+o),A", "23", "25", "7", "FD CB o F7", "4"},
				{"S", "SET 7,(IY+o)", "23", "25", "7", "FD CB o FE", "4"},
				{"S", "SET 7,(IY+o),B", "23", "25", "7", "FD CB o F8", "4"},
				{"S", "SET 7,(IY+o),C", "23", "25", "7", "FD CB o F9", "4"},
				{"S", "SET 7,(IY+o),D", "23", "25", "7", "FD CB o FA", "4"},
				{"S", "SET 7,(IY+o),E", "23", "25", "7", "FD CB o FB", "4"},
				{"S", "SET 7,(IY+o),H", "23", "25", "7", "FD CB o FC", "4"},
				{"S", "SET 7,(IY+o),L", "23", "25", "7", "FD CB o FD", "4"},
				{"S", "SET 7,(IY+o),A", "23", "25", "7", "FD CB o FF", "4"},
				{"S", "SET 0,B", "8", "10", "2", "CB C0", "2"},
				{"S", "SET 0,C", "8", "10", "2", "CB C1", "2"},
				{"S", "SET 0,D", "8", "10", "2", "CB C2", "2"},
				{"S", "SET 0,E", "8", "10", "2", "CB C3", "2"},
				{"S", "SET 0,H", "8", "10", "2", "CB C4", "2"},
				{"S", "SET 0,L", "8", "10", "2", "CB C5", "2"},
				{"S", "SET 0,A", "8", "10", "2", "CB C7", "2"},
				{"S", "SET 1,B", "8", "10", "2", "CB C8", "2"},
				{"S", "SET 1,C", "8", "10", "2", "CB C9", "2"},
				{"S", "SET 1,D", "8", "10", "2", "CB CA", "2"},
				{"S", "SET 1,E", "8", "10", "2", "CB CB", "2"},
				{"S", "SET 1,H", "8", "10", "2", "CB CC", "2"},
				{"S", "SET 1,L", "8", "10", "2", "CB CD", "2"},
				{"S", "SET 1,A", "8", "10", "2", "CB CF", "2"},
				{"S", "SET 2,B", "8", "10", "2", "CB D0", "2"},
				{"S", "SET 2,C", "8", "10", "2", "CB D1", "2"},
				{"S", "SET 2,D", "8", "10", "2", "CB D2", "2"},
				{"S", "SET 2,E", "8", "10", "2", "CB D3", "2"},
				{"S", "SET 2,H", "8", "10", "2", "CB D4", "2"},
				{"S", "SET 2,L", "8", "10", "2", "CB D5", "2"},
				{"S", "SET 2,A", "8", "10", "2", "CB D7", "2"},
				{"S", "SET 3,B", "8", "10", "2", "CB D8", "2"},
				{"S", "SET 3,C", "8", "10", "2", "CB D9", "2"},
				{"S", "SET 3,D", "8", "10", "2", "CB DA", "2"},
				{"S", "SET 3,E", "8", "10", "2", "CB DB", "2"},
				{"S", "SET 3,H", "8", "10", "2", "CB DC", "2"},
				{"S", "SET 3,L", "8", "10", "2", "CB DD", "2"},
				{"S", "SET 3,A", "8", "10", "2", "CB DF", "2"},
				{"S", "SET 4,B", "8", "10", "2", "CB E0", "2"},
				{"S", "SET 4,C", "8", "10", "2", "CB E1", "2"},
				{"S", "SET 4,D", "8", "10", "2", "CB E2", "2"},
				{"S", "SET 4,E", "8", "10", "2", "CB E3", "2"},
				{"S", "SET 4,H", "8", "10", "2", "CB E4", "2"},
				{"S", "SET 4,L", "8", "10", "2", "CB E5", "2"},
				{"S", "SET 4,A", "8", "10", "2", "CB E7", "2"},
				{"S", "SET 5,B", "8", "10", "2", "CB E8", "2"},
				{"S", "SET 5,C", "8", "10", "2", "CB E9", "2"},
				{"S", "SET 5,D", "8", "10", "2", "CB EA", "2"},
				{"S", "SET 5,E", "8", "10", "2", "CB EB", "2"},
				{"S", "SET 5,H", "8", "10", "2", "CB EC", "2"},
				{"S", "SET 5,L", "8", "10", "2", "CB ED", "2"},
				{"S", "SET 5,A", "8", "10", "2", "CB EF", "2"},
				{"S", "SET 6,B", "8", "10", "2", "CB F0", "2"},
				{"S", "SET 6,C", "8", "10", "2", "CB F1", "2"},
				{"S", "SET 6,D", "8", "10", "2", "CB F2", "2"},
				{"S", "SET 6,E", "8", "10", "2", "CB F3", "2"},
				{"S", "SET 6,H", "8", "10", "2", "CB F4", "2"},
				{"S", "SET 6,L", "8", "10", "2", "CB F5", "2"},
				{"S", "SET 6,A", "8", "10", "2", "CB F7", "2"},
				{"S", "SET 7,B", "8", "10", "2", "CB F8", "2"},
				{"S", "SET 7,C", "8", "10", "2", "CB F9", "2"},
				{"S", "SET 7,D", "8", "10", "2", "CB FA", "2"},
				{"S", "SET 7,E", "8", "10", "2", "CB FB", "2"},
				{"S", "SET 7,H", "8", "10", "2", "CB FC", "2"},
				{"S", "SET 7,L", "8", "10", "2", "CB FD", "2"},
				{"S", "SET 7,A", "8", "10", "2", "CB FF", "2"},
				{"S", "SL1 (HL)", "15", "17", "4", "CB 36", "2"},		 // alt. SLI/SLL (HL)
				{"S", "SL1 (IX+o)", "23", "25", "7", "DD CB o 36", "4"}, // alt. SLI/SLL (IX+o)
				{"S", "SL1 (IY+o)", "23", "25", "7", "FD CB o 36", "4"}, // alt. SLI/SLL (IY+o)
				{"S", "SL1 B", "8", "10", "2", "CB 30", "2"},
				{"S", "SL1 C", "8", "10", "2", "CB 31", "2"},
				{"S", "SL1 D", "8", "10", "2", "CB 32", "2"},
				{"S", "SL1 E", "8", "10", "2", "CB 33", "2"},
				{"S", "SL1 H", "8", "10", "2", "CB 34", "2"},
				{"S", "SL1 L", "8", "10", "2", "CB 35", "2"},
				{"S", "SL1 A", "8", "10", "2", "CB 37", "2"}, // alt. SLI/SLL r
				{"S", "SLA (HL)", "15", "17", "4", "CB 26", "2"},
				{"S", "SLA (IX+o)", "23", "25", "7", "DD CB o 26", "4"},
				{"S", "SLA (IX+o),B", "23", "25", "7", "DD CB o 20", "4"},
				{"S", "SLA (IX+o),C", "23", "25", "7", "DD CB o 21", "4"},
				{"S", "SLA (IX+o),D", "23", "25", "7", "DD CB o 22", "4"},
				{"S", "SLA (IX+o),E", "23", "25", "7", "DD CB o 23", "4"},
				{"S", "SLA (IX+o),H", "23", "25", "7", "DD CB o 24", "4"},
				{"S", "SLA (IX+o),L", "23", "25", "7", "DD CB o 25", "4"},
				{"S", "SLA (IX+o),A", "23", "25", "7", "DD CB o 27", "4"},
				{"S", "SLA (IY+o)", "23", "25", "7", "FD CB o 26", "4"},
				{"S", "SLA (IY+o),B", "23", "25", "7", "FD CB o 20", "4"},
				{"S", "SLA (IY+o),C", "23", "25", "7", "FD CB o 21", "4"},
				{"S", "SLA (IY+o),D", "23", "25", "7", "FD CB o 22", "4"},
				{"S", "SLA (IY+o),E", "23", "25", "7", "FD CB o 23", "4"},
				{"S", "SLA (IY+o),H", "23", "25", "7", "FD CB o 24", "4"},
				{"S", "SLA (IY+o),L", "23", "25", "7", "FD CB o 25", "4"},
				{"S", "SLA (IY+o),A", "23", "25", "7", "FD CB o 27", "4"},
				{"S", "SLA B", "8", "10", "2", "CB 20", "2"},
				{"S", "SLA C", "8", "10", "2", "CB 21", "2"},
				{"S", "SLA D", "8", "10", "2", "CB 22", "2"},
				{"S", "SLA E", "8", "10", "2", "CB 23", "2"},
				{"S", "SLA H", "8", "10", "2", "CB 24", "2"},
				{"S", "SLA L", "8", "10", "2", "CB 25", "2"},
				{"S", "SLA A", "8", "10", "2", "CB 27", "2"},
				{"S", "SLI (HL)", "15", "17", "4", "CB 36", "2"},		 // alt. SL1/SLL (HL)
				{"S", "SLI (IX+o)", "23", "25", "7", "DD CB o 36", "4"}, // alt. SL1/SLL (IX+o)
				{"S", "SLI (IY+o)", "23", "25", "7", "FD CB o 36", "4"}, // alt. SL1/SLL (IY+o)
				{"S", "SLI B", "8", "10", "2", "CB 30", "2"},
				{"S", "SLI C", "8", "10", "2", "CB 31", "2"},
				{"S", "SLI D", "8", "10", "2", "CB 32", "2"},
				{"S", "SLI E", "8", "10", "2", "CB 33", "2"},
				{"S", "SLI H", "8", "10", "2", "CB 34", "2"},
				{"S", "SLI L", "8", "10", "2", "CB 35", "2"},
				{"S", "SLI A", "8", "10", "2", "CB 37", "2"},			 // alt. SL1/SLL r
				{"S", "SLL (HL)", "15", "17", "4", "CB 36", "2"},		 // alt. SL1/SLI (HL)
				{"S", "SLL (IX+o)", "23", "25", "7", "DD CB o 36", "4"}, // alt. SL1/SLI (IX+o)
				{"S", "SLL (IY+o)", "23", "25", "7", "FD CB o 36", "4"}, // alt. SL1/SLI (IY+o)
				{"S", "SLL B", "8", "10", "2", "CB 30", "2"},
				{"S", "SLL C", "8", "10", "2", "CB 31", "2"},
				{"S", "SLL D", "8", "10", "2", "CB 32", "2"},
				{"S", "SLL E", "8", "10", "2", "CB 33", "2"},
				{"S", "SLL H", "8", "10", "2", "CB 34", "2"},
				{"S", "SLL L", "8", "10", "2", "CB 35", "2"},
				{"S", "SLL A", "8", "10", "2", "CB 37", "2"}, // alt. SL1/SLI r
				{"S", "SRA (HL)", "15", "17", "4", "CB 2E", "2"},
				{"S", "SRA (IX+o)", "23", "25", "7", "DD CB o 2E", "4"},
				{"S", "SRA (IX+o),B", "23", "25", "7", "DD CB o 28", "4"},
				{"S", "SRA (IX+o),C", "23", "25", "7", "DD CB o 29", "4"},
				{"S", "SRA (IX+o),D", "23", "25", "7", "DD CB o 2A", "4"},
				{"S", "SRA (IX+o),E", "23", "25", "7", "DD CB o 2B", "4"},
				{"S", "SRA (IX+o),H", "23", "25", "7", "DD CB o 2C", "4"},
				{"S", "SRA (IX+o),L", "23", "25", "7", "DD CB o 2D", "4"},
				{"S", "SRA (IX+o),A", "23", "25", "7", "DD CB o 2F", "4"},
				{"S", "SRA (IY+o)", "23", "25", "7", "FD CB o 2E", "4"},
				{"S", "SRA (IY+o),B", "23", "25", "7", "FD CB o 28", "4"},
				{"S", "SRA (IY+o),C", "23", "25", "7", "FD CB o 29", "4"},
				{"S", "SRA (IY+o),D", "23", "25", "7", "FD CB o 2A", "4"},
				{"S", "SRA (IY+o),E", "23", "25", "7", "FD CB o 2B", "4"},
				{"S", "SRA (IY+o),H", "23", "25", "7", "FD CB o 2C", "4"},
				{"S", "SRA (IY+o),L", "23", "25", "7", "FD CB o 2D", "4"},
				{"S", "SRA (IY+o),A", "23", "25", "7", "FD CB o 2F", "4"},
				{"S", "SRA B", "8", "10", "2", "CB 28", "2"},
				{"S", "SRA C", "8", "10", "2", "CB 29", "2"},
				{"S", "SRA D", "8", "10", "2", "CB 2A", "2"},
				{"S", "SRA E", "8", "10", "2", "CB 2B", "2"},
				{"S", "SRA H", "8", "10", "2", "CB 2C", "2"},
				{"S", "SRA L", "8", "10", "2", "CB 2D", "2"},
				{"S", "SRA A", "8", "10", "2", "CB 2F", "2"},
				{"S", "SRL (HL)", "15", "17", "4", "CB 3E", "2"},
				{"S", "SRL (IX+o)", "23", "25", "7", "DD CB o 3E", "4"},
				{"S", "SRL (IX+o),B", "23", "25", "7", "DD CB o 38", "4"},
				{"S", "SRL (IX+o),C", "23", "25", "7", "DD CB o 39", "4"},
				{"S", "SRL (IX+o),D", "23", "25", "7", "DD CB o 3A", "4"},
				{"S", "SRL (IX+o),E", "23", "25", "7", "DD CB o 3B", "4"},
				{"S", "SRL (IX+o),H", "23", "25", "7", "DD CB o 3C", "4"},
				{"S", "SRL (IX+o),L", "23", "25", "7", "DD CB o 3D", "4"},
				{"S", "SRL (IX+o),A", "23", "25", "7", "DD CB o 3F", "4"},
				{"S", "SRL (IY+o)", "23", "25", "7", "FD CB o 3E", "4"},
				{"S", "SRL (IY+o),B", "23", "25", "7", "FD CB o 38", "4"},
				{"S", "SRL (IY+o),C", "23", "25", "7", "FD CB o 39", "4"},
				{"S", "SRL (IY+o),D", "23", "25", "7", "FD CB o 3A", "4"},
				{"S", "SRL (IY+o),E", "23", "25", "7", "FD CB o 3B", "4"},
				{"S", "SRL (IY+o),H", "23", "25", "7", "FD CB o 3C", "4"},
				{"S", "SRL (IY+o),L", "23", "25", "7", "FD CB o 3D", "4"},
				{"S", "SRL (IY+o),A", "23", "25", "7", "FD CB o 3F", "4"},
				{"S", "SRL B", "8", "10", "2", "CB 38", "2"},
				{"S", "SRL C", "8", "10", "2", "CB 39", "2"},
				{"S", "SRL D", "8", "10", "2", "CB 3A", "2"},
				{"S", "SRL E", "8", "10", "2", "CB 3B", "2"},
				{"S", "SRL H", "8", "10", "2", "CB 3C", "2"},
				{"S", "SRL L", "8", "10", "2", "CB 3D", "2"},
				{"S", "SRL A", "8", "10", "2", "CB 3F", "2"},
				{"S", "SUB (HL)", "7", "8", "2", "96", "1"},
				{"S", "SUB (IX+o)", "19", "21", "5", "DD 96 o", "3"},
				{"S", "SUB (IY+o)", "19", "21", "5", "FD 96 o", "3"},
				{"S", "SUB IXH", "8", "10", "2", "DD 94", "2"},
				{"S", "SUB IXL", "8", "10", "2", "DD 95", "2"},
				{"S", "SUB IYH", "8", "10", "2", "FD 94", "2"},
				{"S", "SUB IYL", "8", "10", "2", "FD 95", "2"},
				{"S", "SUB n", "7", "8", "2", "D6 n", "2"},
				{"S", "SUB B", "4", "5", "1", "90", "1"},
				{"S", "SUB C", "4", "5", "1", "91", "1"},
				{"S", "SUB D", "4", "5", "1", "92", "1"},
				{"S", "SUB E", "4", "5", "1", "93", "1"},
				{"S", "SUB H", "4", "5", "1", "94", "1"},
				{"S", "SUB L", "4", "5", "1", "95", "1"},
				{"S", "SUB A", "4", "5", "1", "97", "1"},
				{"S", "XOR (HL)", "7", "8", "2", "AE", "1"},
				{"S", "XOR (IX+o)", "19", "21", "5", "DD AE o", "3"},
				{"S", "XOR (IY+o)", "19", "21", "5", "FD AE o", "3"},
				{"S", "XOR IXH", "8", "10", "2", "DD AC", "2"},
				{"S", "XOR IXL", "8", "10", "2", "DD AD", "2"},
				{"S", "XOR IYH", "8", "10", "2", "FD AC", "2"},
				{"S", "XOR IYL", "8", "10", "2", "FD AD", "2"},
				{"S", "XOR n", "7", "8", "2", "EE n", "2"},
				{"S", "XOR B", "4", "5", "1", "A8", "1"},
				{"S", "XOR C", "4", "5", "1", "A9", "1"},
				{"S", "XOR D", "4", "5", "1", "AA", "1"},
				{"S", "XOR E", "4", "5", "1", "AB", "1"},
				{"S", "XOR H", "4", "5", "1", "AC", "1"},
				{"S", "XOR L", "4", "5", "1", "AD", "1"},
				{"S", "XOR A", "4", "5", "1", "AF", "1"},
				//
				// Spectrum Next Instructions
				//
				{"N", "ADD BC,A", "8", "", "", "ED 33", "2"},
				{"N", "ADD BC,nn", "16", "", "", "ED 36 nn nn", "4"},
				{"N", "ADD DE,A", "8", "", "", "ED 32", "2"},
				{"N", "ADD DE,nn", "16", "", "", "ED 35 nn nn", "4"},
				{"N", "ADD HL,A", "8", "", "", "ED 31", "2"},
				{"N", "ADD HL,nn", "16", "", "", "ED 34 nn nn", "4"},
				{"N", "BRLC DE,B", "8", "", "", "ED 2C", "2"},
				{"N", "BSLA DE,B", "8", "", "", "ED 28", "2"},
				{"N", "BSRA DE,B", "8", "", "", "ED 29", "2"},
				{"N", "BSRF DE,B", "8", "", "", "ED 2B", "2"},
				{"N", "BSRL DE,B", "8", "", "", "ED 2A", "2"},
				{"N", "JP (C)", "13", "", "", "ED 98", "2"},
				{"N", "LDDRX", "21/16", "", "", "ED BC", "2"},
				{"N", "LDDX", "16", "", "", "ED AC", "2"},
				{"N", "LDIRX", "21/16", "", "", "ED B4", "2"},
				{"N", "LDIX", "16", "", "", "ED A4", "2"},
				{"N", "LDPIRX", "21/16", "", "", "ED B7", "2"},
				{"N", "LDWS", "14", "", "", "ED A5", "2"},
				{"N", "MIRROR", "8", "", "", "ED 24", "2"},
				{"N", "MUL D,E", "8", "", "", "ED 30", "2"},
				{"N", "NEXTREG n,A", "17", "", "", "ED 92 n", "3"},
				{"N", "NEXTREG n1,n2", "20", "", "", "ED 91 n1 n2", "4"},
				{"N", "OUTINB", "16", "", "", "ED 90", "2"},
				{"N", "PIXELAD", "8", "", "", "ED 94", "2"},
				{"N", "PIXELDN", "8", "", "", "ED 93", "2"},
				{"N", "PUSH nn", "23", "", "", "ED 8A nn.h nn.l", "4"},
				{"N", "SETAE", "8", "", "", "ED 95", "2"},
				{"N", "SWAPNIB", "8", "", "", "ED 23", "2"},
				{"N", "TEST n", "11", "", "", "ED 27 n", "3"},
				//
				// Fake instructions
				// Assembler translates these into multiple instructions
				//
				{"F", "RL BC", "16", "20", "4", "CB 11 CB 10", "4"},
				{"F", "RL DE", "16", "20", "4", "CB 13 CB 12", "4"},
				{"F", "RL HL", "16", "20", "4", "CB 15 CB 14", "4"},
				{"F", "RR BC", "16", "20", "4", "CB 18 CB 19", "4"},
				{"F", "RR DE", "16", "20", "4", "CB 1A CB 1B", "4"},
				{"F", "RR HL", "16", "20", "4", "CB 1C CB 1D", "4"},
				{"F", "SLA BC", "16", "20", "4", "CB 21 CB 10", "4"},
				{"F", "SLA DE", "16", "20", "4", "CB 23 CB 12", "4"},
				{"F", "SLA HL", "11", "12", "3", "29", "1"},
				{"F", "SLL BC", "16", "20", "4", "CB 31 CB 10", "4"},
				{"F", "SLL DE", "16", "20", "4", "CB 33 CB 12", "4"},
				{"F", "SLL HL", "16", "20", "4", "CB 35 CB 14", "4"},
				{"F", "SLI BC", "16", "20", "4", "CB 31 CB 10", "4"},
				{"F", "SLI DE", "16", "20", "4", "CB 33 CB 12", "4"},
				{"F", "SLI HL", "16", "20", "4", "CB 35 CB 14", "4"},
				{"F", "SRA BC", "16", "20", "4", "CB 28 CB 19", "4"},
				{"F", "SRA DE", "16", "20", "4", "CB 2A CB 1B", "4"},
				{"F", "SRA HL", "16", "20", "4", "CB 2C CB 1D", "4"},
				{"F", "SRL BC", "16", "20", "4", "CB 38 CB 19", "4"},
				{"F", "SRL DE", "16", "20", "4", "CB 3A CB 1B", "4"},
				{"F", "SRL HL", "16", "20", "4", "CB 3C CB 1D", "4"},
				{"F", "LD BC,BC", "8", "10", "2", "40 49", "2"},
				{"F", "LD BC,DE", "8", "10", "2", "42 4B", "2"},
				{"F", "LD BC,HL", "8", "10", "2", "44 4D", "2"},
				{"F", "LD BC,IX", "16", "20", "4", "DD 44 DD 4D", "4"},
				{"F", "LD BC,IY", "16", "20", "4", "FD 44 FD 4D", "4"},
				{"F", "LD BC,(HL)", "26", "30", "8", "4E 23 46 2B", "4"},
				{"F", "LD BC,(IX+o)", "38", "42", "10", "DD 4E o DD 46 o", "6"},
				{"F", "LD BC,(IY+o)", "38", "42", "10", "FD 4E o FD 46 o", "6"},
				{"F", "LD DE,BC", "8", "10", "2", "50 59", "2"},
				{"F", "LD DE,DE", "8", "10", "2", "52 5B", "2"},
				{"F", "LD DE,HL", "8", "10", "2", "54 5D", "2"},
				{"F", "LD DE,IX", "16", "20", "4", "DD 54 DD 5D", "4"},
				{"F", "LD DE,IY", "16", "20", "4", "FD 54 FD 5D", "4"},
				{"F", "LD DE,(HL)", "26", "30", "8", "5E 23 56 2B", "4"},
				{"F", "LD DE,(IX+o)", "38", "42", "10", "DD 5E o DD 56 o", "6"},
				{"F", "LD DE,(IY+o)", "38", "42", "10", "FD 5E o FD 56 o", "6"},
				{"F", "LD HL,BC", "8", "10", "2", "60 69", "2"},
				{"F", "LD HL,DE", "8", "10", "2", "62 6B", "2"},
				{"F", "LD HL,HL", "8", "10", "2", "64 6D", "2"},
				{"F", "LD HL,IX", "25", "28", "8", "DD E5 E1", "3"},
				{"F", "LD HL,IY", "25", "28", "8", "FD E5 E1", "3"},
				{"F", "LD HL,(IX+o)", "38", "42", "10", "DD 6E o DD 66 o", "6"},
				{"F", "LD HL,(IY+o)", "38", "42", "10", "FD 6E o FD 66 o", "6"},
				{"F", "LD IX,BC", "16", "20", "4", "DD 60 DD 69", "4"},
				{"F", "LD IX,DE", "16", "20", "4", "DD 62 DD 6B", "4"},
				{"F", "LD IX,HL", "25", "28", "8", "E5 DD E1", "3"},
				{"F", "LD IX,IX", "16", "20", "4", "DD 64 DD 6D", "4"},
				{"F", "LD IX,IY", "29", "33", "9", "FD E5 DD E1", "4"},
				{"F", "LD IY,BC", "16", "20", "4", "FD 60 FD 69", "4"},
				{"F", "LD IY,DE", "16", "20", "4", "FD 62 FD 6B", "4"},
				{"F", "LD IY,HL", "25", "28", "8", "E5 FD E1", "3"},
				{"F", "LD IY,IX", "29", "33", "9", "DD E5 FD E1", "4"},
				{"F", "LD IY,IY", "16", "20", "4", "FD 64 FD 6D", "4"},
				{"F", "LD (HL),BC", "26", "30", "8", "71 23 70 2B", "4"},
				{"F", "LD (HL),DE", "26", "30", "8", "73 23 72 2B", "4"},
				{"F", "LD (IX+o),BC", "38", "42", "10", "DD 71 o DD 70 o", "6"},
				{"F", "LD (IX+o),DE", "38", "42", "10", "DD 73 o DD 72 o", "6"},
				{"F", "LD (IX+o),HL", "38", "42", "10", "DD 75 o DD 74 o", "6"},
				{"F", "LD (IY+o),BC", "38", "42", "10", "FD 71 o FD 70 o", "6"},
				{"F", "LD (IY+o),DE", "38", "42", "10", "FD 73 o FD 72 o", "6"},
				{"F", "LD (IY+o),HL", "38", "42", "10", "FD 75 o FD 74 o", "6"},
				{"F", "LDI BC,(HL)", "26", "30", "8", "4E 23 46 23", "4"},
				{"F", "LDI BC,(IX+o)", "58", "66", "16", "DD 4E o DD 23 DD 46 o DD 23", "10"},
				{"F", "LDI BC,(IY+o)", "58", "66", "16", "FD 4E o FD 23 FD 46 o FD 23", "10"},
				{"F", "LDI DE,(HL)", "26", "30", "8", "5E 23 56 23", "4"},
				{"F", "LDI DE,(IX+o)", "58", "66", "16", "DD 5E o DD 23 DD 56 o DD 23", "10"},
				{"F", "LDI DE,(IY+o)", "58", "66", "16", "FD 5E o FD 23 FD 56 o FD 23", "10"},
				{"F", "LDI HL,(IX+o)", "58", "66", "16", "DD 6E o DD 23 DD 66 o DD 23", "10"},
				{"F", "LDI HL,(IY+o)", "58", "66", "16", "FD 6E o FD 23 FD 66 o FD 23", "10"},
				{"F", "LDI (HL),BC", "26", "30", "8", "71 23 70 23", "4"},
				{"F", "LDI (HL),DE", "26", "30", "8", "73 23 72 23", "4"},
				{"F", "LDI (IX+o),BC", "58", "66", "16", "DD 71 o DD 23 DD 70 o DD 23", "10"},
				{"F", "LDI (IX+o),DE", "58", "66", "16", "DD 73 o DD 23 DD 72 o DD 23", "10"},
				{"F", "LDI (IX+o),HL", "58", "66", "16", "DD 75 o DD 23 DD 74 o DD 23", "10"},
				{"F", "LDI (IY+o),BC", "58", "66", "16", "FD 71 o FD 23 FD 70 o FD 23", "10"},
				{"F", "LDI (IY+o),DE", "58", "66", "16", "FD 73 o FD 23 FD 72 o FD 23", "10"},
				{"F", "LDI (IY+o),HL", "58", "66", "16", "FD 75 o FD 23 FD 74 o FD 23", "10"},
				{"F", "LDI A,(BC)", "13", "15", "4", "0A 03", "2"},
				{"F", "LDI A,(DE)", "13", "15", "4", "1A 13", "2"},
				{"F", "LDI A,(HL)", "13", "15", "4", "7E 23", "2"},
				{"F", "LDI B,(HL)", "13", "15", "4", "46 23", "2"},
				{"F", "LDI C,(HL)", "13", "15", "4", "4E 23", "2"},
				{"F", "LDI D,(HL)", "13", "15", "4", "56 23", "2"},
				{"F", "LDI E,(HL)", "13", "15", "4", "5E 23", "2"},
				{"F", "LDI H,(HL)", "13", "15", "4", "66 23", "2"},
				{"F", "LDI L,(HL)", "13", "15", "4", "6E 23", "2"},
				{"F", "LDI A,(IX+o)", "29", "33", "8", "DD 7E o DD 23", "5"},
				{"F", "LDI B,(IX+o)", "29", "33", "8", "DD 46 o DD 23", "5"},
				{"F", "LDI C,(IX+o)", "29", "33", "8", "DD 4E o DD 23", "5"},
				{"F", "LDI D,(IX+o)", "29", "33", "8", "DD 56 o DD 23", "5"},
				{"F", "LDI E,(IX+o)", "29", "33", "8", "DD 5E o DD 23", "5"},
				{"F", "LDI H,(IX+o)", "29", "33", "8", "DD 66 o DD 23", "5"},
				{"F", "LDI L,(IX+o)", "29", "33", "8", "DD 6E o DD 23", "5"},
				{"F", "LDI A,(IY+o)", "29", "33", "8", "FD 7E o FD 23", "5"},
				{"F", "LDI B,(IY+o)", "29", "33", "8", "FD 46 o FD 23", "5"},
				{"F", "LDI C,(IY+o)", "29", "33", "8", "FD 4E o FD 23", "5"},
				{"F", "LDI D,(IY+o)", "29", "33", "8", "FD 56 o FD 23", "5"},
				{"F", "LDI E,(IY+o)", "29", "33", "8", "FD 5E o FD 23", "5"},
				{"F", "LDI H,(IY+o)", "29", "33", "8", "FD 66 o FD 23", "5"},
				{"F", "LDI L,(IY+o)", "29", "33", "8", "FD 6E o FD 23", "5"},
				{"F", "LDD A,(BC)", "13", "15", "4", "0A 0B", "2"},
				{"F", "LDD A,(DE)", "13", "15", "4", "1A 1B", "2"},
				{"F", "LDD A,(HL)", "13", "15", "4", "7E 2B", "2"},
				{"F", "LDD B,(HL)", "13", "15", "4", "46 2B", "2"},
				{"F", "LDD C,(HL)", "13", "15", "4", "4E 2B", "2"},
				{"F", "LDD D,(HL)", "13", "15", "4", "56 2B", "2"},
				{"F", "LDD E,(HL)", "13", "15", "4", "5E 2B", "2"},
				{"F", "LDD H,(HL)", "13", "15", "4", "66 2B", "2"},
				{"F", "LDD L,(HL)", "13", "15", "4", "6E 2B", "2"},
				{"F", "LDD A,(IX+o)", "29", "33", "8", "DD 7E o DD 2B", "5"},
				{"F", "LDD B,(IX+o)", "29", "33", "8", "DD 46 o DD 2B", "5"},
				{"F", "LDD C,(IX+o)", "29", "33", "8", "DD 4E o DD 2B", "5"},
				{"F", "LDD D,(IX+o)", "29", "33", "8", "DD 56 o DD 2B", "5"},
				{"F", "LDD E,(IX+o)", "29", "33", "8", "DD 5E o DD 2B", "5"},
				{"F", "LDD H,(IX+o)", "29", "33", "8", "DD 66 o DD 2B", "5"},
				{"F", "LDD L,(IX+o)", "29", "33", "8", "DD 6E o DD 2B", "5"},
				{"F", "LDD A,(IY+o)", "29", "33", "8", "FD 7E o FD 2B", "5"},
				{"F", "LDD B,(IY+o)", "29", "33", "8", "FD 46 o FD 2B", "5"},
				{"F", "LDD C,(IY+o)", "29", "33", "8", "FD 4E o FD 2B", "5"},
				{"F", "LDD D,(IY+o)", "29", "33", "8", "FD 56 o FD 2B", "5"},
				{"F", "LDD E,(IY+o)", "29", "33", "8", "FD 5E o FD 2B", "5"},
				{"F", "LDD H,(IY+o)", "29", "33", "8", "FD 66 o FD 2B", "5"},
				{"F", "LDD L,(IY+o)", "29", "33", "8", "FD 6E o FD 2B", "5"},
				{"F", "LDI (BC),A", "13", "15", "4", "02 03", "2"},
				{"F", "LDI (DE),A", "13", "15", "4", "12 13", "2"},
				{"F", "LDI (HL),A", "13", "15", "4", "77 23", "2"},
				{"F", "LDI (HL),B", "13", "15", "4", "70 23", "2"},
				{"F", "LDI (HL),C", "13", "15", "4", "71 23", "2"},
				{"F", "LDI (HL),D", "13", "15", "4", "72 23", "2"},
				{"F", "LDI (HL),E", "13", "15", "4", "73 23", "2"},
				{"F", "LDI (HL),H", "13", "15", "4", "74 23", "2"},
				{"F", "LDI (HL),L", "13", "15", "4", "75 23", "2"},
				{"F", "LDI (IX+o),A", "29", "33", "8", "DD 77 o DD 23", "5"},
				{"F", "LDI (IX+o),B", "29", "33", "8", "DD 70 o DD 23", "5"},
				{"F", "LDI (IX+o),C", "29", "33", "8", "DD 71 o DD 23", "5"},
				{"F", "LDI (IX+o),D", "29", "33", "8", "DD 72 o DD 23", "5"},
				{"F", "LDI (IX+o),E", "29", "33", "8", "DD 73 o DD 23", "5"},
				{"F", "LDI (IX+o),H", "29", "33", "8", "DD 74 o DD 23", "5"},
				{"F", "LDI (IX+o),L", "29", "33", "8", "DD 75 o DD 23", "5"},
				{"F", "LDI (IY+o),A", "29", "33", "8", "FD 77 o FD 23", "5"},
				{"F", "LDI (IY+o),B", "29", "33", "8", "FD 70 o FD 23", "5"},
				{"F", "LDI (IY+o),C", "29", "33", "8", "FD 71 o FD 23", "5"},
				{"F", "LDI (IY+o),D", "29", "33", "8", "FD 72 o FD 23", "5"},
				{"F", "LDI (IY+o),E", "29", "33", "8", "FD 73 o FD 23", "5"},
				{"F", "LDI (IY+o),H", "29", "33", "8", "FD 74 o FD 23", "5"},
				{"F", "LDI (IY+o),L", "29", "33", "8", "FD 75 o FD 23", "5"},
				{"F", "LDD (BC),A", "13", "15", "4", "02 0B", "2"},
				{"F", "LDD (DE),A", "13", "15", "4", "12 1B", "2"},
				{"F", "LDD (HL),A", "13", "15", "4", "77 2B", "2"},
				{"F", "LDD (HL),B", "13", "15", "4", "70 2B", "2"},
				{"F", "LDD (HL),C", "13", "15", "4", "71 2B", "2"},
				{"F", "LDD (HL),D", "13", "15", "4", "72 2B", "2"},
				{"F", "LDD (HL),E", "13", "15", "4", "73 2B", "2"},
				{"F", "LDD (HL),H", "13", "15", "4", "74 2B", "2"},
				{"F", "LDD (HL),L", "13", "15", "4", "75 2B", "2"},
				{"F", "LDD (IX+o),A", "29", "33", "8", "DD 77 o DD 2B", "5"},
				{"F", "LDD (IX+o),B", "29", "33", "8", "DD 70 o DD 2B", "5"},
				{"F", "LDD (IX+o),C", "29", "33", "8", "DD 71 o DD 2B", "5"},
				{"F", "LDD (IX+o),D", "29", "33", "8", "DD 72 o DD 2B", "5"},
				{"F", "LDD (IX+o),E", "29", "33", "8", "DD 73 o DD 2B", "5"},
				{"F", "LDD (IX+o),H", "29", "33", "8", "DD 74 o DD 2B", "5"},
				{"F", "LDD (IX+o),L", "29", "33", "8", "DD 75 o DD 2B", "5"},
				{"F", "LDD (IY+o),A", "29", "33", "8", "FD 77 o FD 2B", "5"},
				{"F", "LDD (IY+o),B", "29", "33", "8", "FD 70 o FD 2B", "5"},
				{"F", "LDD (IY+o),C", "29", "33", "8", "FD 71 o FD 2B", "5"},
				{"F", "LDD (IY+o),D", "29", "33", "8", "FD 72 o FD 2B", "5"},
				{"F", "LDD (IY+o),E", "29", "33", "8", "FD 73 o FD 2B", "5"},
				{"F", "LDD (IY+o),H", "29", "33", "8", "FD 74 o FD 2B", "5"},
				{"F", "LDD (IY+o),L", "29", "33", "8", "FD 75 o FD 2B", "5"},
				{"F", "LDI (HL),n", "16", "18", "5", "36 n 23", "3"},
				{"F", "LDI (IX+o),n", "29", "33", "9", "DD 36 o n DD 23", "6"},
				{"F", "LDI (IY+o),n", "29", "33", "9", "FD 36 o n FD 23", "6"},
				{"F", "LDD (HL),n", "16", "18", "5", "36 n 2B", "3"},
				{"F", "LDD (IX+o),n", "29", "33", "9", "DD 36 o n DD 2B", "6"},
				{"F", "LDD (IY+o),n", "29", "33", "9", "FD 36 o n FD 2B", "6"},
				{"F", "SUB HL,BC", "19", "22", "5", "B7 ED 42", "3"},
				{"F", "SUB HL,DE", "19", "22", "5", "B7 ED 52", "3"},
				{"F", "SUB HL,HL", "19", "22", "5", "B7 ED 62", "3"},
				{"F", "SUB HL,SP", "19", "22", "5", "B7 ED 72", "3"},
			};
			//
			// Module-level lookup tables built once at startup
			//
			static std::unordered_set<std::string> sRegisters8Set;
			static std::unordered_set<std::string> sRegisters16Set;
			static std::unordered_set<std::string> sConditionCodesSet;
			static std::unordered_set<std::string> sMnemonicSet;
			static std::unordered_set<std::string> sDirectiveSet;
			static std::unordered_map<std::string, std::string> sAliasMap;
			static std::unordered_map<std::string, const Z80Instruction*> sInstructionMap;
			static std::unordered_set<std::string> sDefinitionDirectivesSet;
			//
			// Extract the mnemonic (first token) from an instruction
			//
			static std::string ExtractMnemonic(const std::string& instruction) {
				std::string upper = instruction;
				for (size_t i = 0; i < upper.length(); i++) {
					if (upper[i] >= 'a' && upper[i] <= 'z')
						upper[i] = upper[i] - 'a' + 'A';
				}
				size_t i = 0;
				while (i < upper.length() && (upper[i] == ' ' || upper[i] == '\t'))
					i++;
				size_t start = i;
				while (i < upper.length() && upper[i] != ' ' && upper[i] != '\t' && upper[i] != ',' && upper[i] != '(')
					i++;
				return upper.substr(start, i - start);
			}
			//
			// Check if a mnemonic is a valid Z80 instruction
			//
			static bool IsValidMnemonic(const std::string& mnemonic) {
				return sMnemonicSet.count(mnemonic) != 0;
			}
			//
			// Static lookup tables for Z80 operand types
			//
			static const char* const kZ80ConditionCodes[] = {"C", "NC", "Z", "NZ", "M", "P", "PE", "PO", nullptr};
			static const char* const kZ80Registers16[] = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'", nullptr};
			static const char* const kZ80Registers8[] = {"A", "B", "C", "D", "E", "H", "L", "I", "R", "IXL", "IXH", "IYL", "IYH", "XH", "XL", "YH", "YL", nullptr};
			struct FunctionBlockState {
				std::string name;
				int startLine = -1;
				std::vector<std::pair<int, std::string>> capturedLines;
				std::string timing;
			};
			struct RepeatBlockState {
				std::string keyword;
				int startLine = -1;
				int repeatCount = 0;
				std::string timing;
				int blockId = -1;
			};
			struct Z80InstructionAliasRule {
				const char* fromInstruction;
				const char* toInstruction;
			};
			static const Z80InstructionAliasRule kZ80InstructionAliasRules[] = {{"EX AF", "EX AF,AF'"},
																				{"EX AF,AF", "EX AF,AF'"},
																				{"EXA", "EX AF,AF'"},
																				{"EXD", "EX DE,HL"},
																				{"JP HL", "JP (HL)"},
																				{"JP IX", "JP (IX)"},
																				{"JP IY", "JP (IY)"},
																				{"ADD (HL)", "ADD A,(HL)"},
																				{"ADD (IX+o)", "ADD A,(IX+o)"},
																				{"ADD (IY+o)", "ADD A,(IY+o)"},
																				{"ADD IXH", "ADD A,IXH"},
																				{"ADD XH", "ADD A,IXH"},
																				{"ADD IXL", "ADD A,IXL"},
																				{"ADD XL", "ADD A,IXL"},
																				{"ADD IYH", "ADD A,IYH"},
																				{"ADD YH", "ADD A,IYH"},
																				{"ADD IYL", "ADD A,IYL"},
																				{"ADD YL", "ADD A,IYL"},
																				{"ADD n", "ADD A,n"},
																				{"ADD B", "ADD A,B"},
																				{"ADD C", "ADD A,C"},
																				{"ADD D", "ADD A,D"},
																				{"ADD E", "ADD A,E"},
																				{"ADD H", "ADD A,H"},
																				{"ADD L", "ADD A,L"},
																				{"ADD A", "ADD A,A"},
																				{"ADC (HL)", "ADC A,(HL)"},
																				{"ADC (IX+o)", "ADC A,(IX+o)"},
																				{"ADC (IY+o)", "ADC A,(IY+o)"},
																				{"ADC IXH", "ADC A,IXH"},
																				{"ADC XH", "ADC A,IXH"},
																				{"ADC IXL", "ADC A,IXL"},
																				{"ADC XL", "ADC A,IXL"},
																				{"ADC IYH", "ADC A,IYH"},
																				{"ADC YH", "ADC A,IYH"},
																				{"ADC IYL", "ADC A,IYL"},
																				{"ADC YL", "ADC A,IYL"},
																				{"ADC n", "ADC A,n"},
																				{"ADC B", "ADC A,B"},
																				{"ADC C", "ADC A,C"},
																				{"ADC D", "ADC A,D"},
																				{"ADC E", "ADC A,E"},
																				{"ADC H", "ADC A,H"},
																				{"ADC L", "ADC A,L"},
																				{"ADC A", "ADC A,A"},
																				{"SBC (HL)", "SBC A,(HL)"},
																				{"SBC (IX+o)", "SBC A,(IX+o)"},
																				{"SBC (IY+o)", "SBC A,(IY+o)"},
																				{"SBC IXH", "SBC A,IXH"},
																				{"SBC XH", "SBC A,IXH"},
																				{"SBC IXL", "SBC A,IXL"},
																				{"SBC XL", "SBC A,IXL"},
																				{"SBC IYH", "SBC A,IYH"},
																				{"SBC YH", "SBC A,IYH"},
																				{"SBC IYL", "SBC A,IYL"},
																				{"SBC YL", "SBC A,IYL"},
																				{"SBC n", "SBC A,n"},
																				{"SBC B", "SBC A,B"},
																				{"SBC C", "SBC A,C"},
																				{"SBC D", "SBC A,D"},
																				{"SBC E", "SBC A,E"},
																				{"SBC H", "SBC A,H"},
																				{"SBC L", "SBC A,L"},
																				{"SBC A", "SBC A,A"},
																				{"SUB A,(HL)", "SUB (HL)"},
																				{"SUB A,(IX+o)", "SUB (IX+o)"},
																				{"SUB A,(IY+o)", "SUB (IY+o)"},
																				{"SUB A,IXH", "SUB IXH"},
																				{"SUB XH", "SUB IXH"},
																				{"SUB A,IXL", "SUB IXL"},
																				{"SUB XL", "SUB IXL"},
																				{"SUB A,IYH", "SUB IYH"},
																				{"SUB YH", "SUB IYH"},
																				{"SUB A,IYL", "SUB IYL"},
																				{"SUB YL", "SUB IYL"},
																				{"SUB A,n", "SUB n"},
																				{"SUB A,B", "SUB B"},
																				{"SUB A,C", "SUB C"},
																				{"SUB A,D", "SUB D"},
																				{"SUB A,E", "SUB E"},
																				{"SUB A,H", "SUB H"},
																				{"SUB A,L", "SUB L"},
																				{"SUB A,A", "SUB A"},
																				{"AND A,(HL)", "AND (HL)"},
																				{"AND A,(IX+o)", "AND (IX+o)"},
																				{"AND A,(IY+o)", "AND (IY+o)"},
																				{"AND A,IXH", "AND IXH"},
																				{"AND XH", "AND IXH"},
																				{"AND A,IXL", "AND IXL"},
																				{"AND XL", "AND IXL"},
																				{"AND A,IYH", "AND IYH"},
																				{"AND YH", "AND IYH"},
																				{"AND A,IYL", "AND IYL"},
																				{"AND YL", "AND IYL"},
																				{"AND A,n", "AND n"},
																				{"AND A,B", "AND B"},
																				{"AND A,C", "AND C"},
																				{"AND A,D", "AND D"},
																				{"AND A,E", "AND E"},
																				{"AND A,H", "AND H"},
																				{"AND A,L", "AND L"},
																				{"AND A,A", "AND A"},
																				{"OR A,(HL)", "OR (HL)"},
																				{"OR A,(IX+o)", "OR (IX+o)"},
																				{"OR A,(IY+o)", "OR (IY+o)"},
																				{"OR A,IXH", "OR IXH"},
																				{"OR XH", "OR IXH"},
																				{"OR A,IXL", "OR IXL"},
																				{"OR XL", "OR IXL"},
																				{"OR A,IYH", "OR IYH"},
																				{"OR YH", "OR IYH"},
																				{"OR A,IYL", "OR IYL"},
																				{"OR YL", "OR IYL"},
																				{"OR A,n", "OR n"},
																				{"OR A,B", "OR B"},
																				{"OR A,C", "OR C"},
																				{"OR A,D", "OR D"},
																				{"OR A,E", "OR E"},
																				{"OR A,H", "OR H"},
																				{"OR A,L", "OR L"},
																				{"OR A,A", "OR A"},
																				{"XOR A,(HL)", "XOR (HL)"},
																				{"XOR A,(IX+o)", "XOR (IX+o)"},
																				{"XOR A,(IY+o)", "XOR (IY+o)"},
																				{"XOR A,IXH", "XOR IXH"},
																				{"XOR XH", "XOR IXH"},
																				{"XOR A,IXL", "XOR IXL"},
																				{"XOR XL", "XOR IXL"},
																				{"XOR A,IYH", "XOR IYH"},
																				{"XOR YH", "XOR IYH"},
																				{"XOR A,IYL", "XOR IYL"},
																				{"XOR YL", "XOR IYL"},
																				{"XOR A,n", "XOR n"},
																				{"XOR A,B", "XOR B"},
																				{"XOR A,C", "XOR C"},
																				{"XOR A,D", "XOR D"},
																				{"XOR A,E", "XOR E"},
																				{"XOR A,H", "XOR H"},
																				{"XOR A,L", "XOR L"},
																				{"XOR A,A", "XOR A"},
																				{"CP A,(HL)", "CP (HL)"},
																				{"CP A,(IX+o)", "CP (IX+o)"},
																				{"CP A,(IY+o)", "CP (IY+o)"},
																				{"CP A,IXH", "CP IXH"},
																				{"CP XH", "CP IXH"},
																				{"CP A,IXL", "CP IXL"},
																				{"CP XL", "CP IXL"},
																				{"CP A,IYH", "CP IYH"},
																				{"CP YH", "CP IYH"},
																				{"CP A,IYL", "CP IYL"},
																				{"CP YL", "CP IYL"},
																				{"CP A,n", "CP n"},
																				{"CP A,B", "CP B"},
																				{"CP A,C", "CP C"},
																				{"CP A,D", "CP D"},
																				{"CP A,E", "CP E"},
																				{"CP A,H", "CP H"},
																				{"CP A,L", "CP L"},
																				{"CP A,A", "CP A"},
																				{"INC XH", "INC IXH"},
																				{"INC XL", "INC IXL"},
																				{"INC YH", "INC IYH"},
																				{"INC YL", "INC IYL"},
																				{"DEC XH", "DEC IXH"},
																				{"DEC XL", "DEC IXL"},
																				{"DEC YH", "DEC IYH"},
																				{"DEC YL", "DEC IYL"}};
			//
			// O(1) set-based lookup
			//
			static bool IsInSet(const std::string& token, const std::unordered_set<std::string>& set) {
				return set.count(token) != 0;
			}
			static bool IsDirective(const std::string& text);
			//
			// Initializes all lookup tables — called once from the language definition constructor
			//
			static void InitLookupTables() {
				for (size_t i = 0; kZ80Registers8[i] != nullptr; i++)
					sRegisters8Set.insert(kZ80Registers8[i]);
				for (size_t i = 0; kZ80Registers16[i] != nullptr; i++)
					sRegisters16Set.insert(kZ80Registers16[i]);
				for (size_t i = 0; kZ80ConditionCodes[i] != nullptr; i++)
					sConditionCodesSet.insert(kZ80ConditionCodes[i]);
				for (const auto& entry : kZ80InstructionSet)
					sMnemonicSet.insert(ExtractMnemonic(entry.instruction));
				for (const auto& d : kZ80Directives)
					sDirectiveSet.insert(d);
				for (const auto& d : kZ80PreprocessorDirectives)
					sDirectiveSet.insert(d);
				for (const auto& rule : kZ80InstructionAliasRules)
					sAliasMap[rule.fromInstruction] = rule.toInstruction;
				for (const auto& entry : kZ80InstructionSet)
					sInstructionMap[entry.instruction] = &entry;
				static const char* const kZ80DefinitionDirectives[] = {
					"EQU", "DEFC", "DEFB", "DEFD", "DEFL", "DEFM", "DEFS", "DEFW",
					"DB",  "DD",   "DL",   "DM",   "DS",   "DW",   "BYTE",
					"INCBIN",  "INCLUDE",  "INSERT",
					"INCAPU",  "INCEXO",   "INCL",    "INCL48",  "INCL49",
					"INCLZ4",  "INCLZSA1", "INCLZSA2","INCSNA",
					"INCZX0",  "INCZX0B",  "INCZX7",
					"LZ4",     "LZ48",     "LZ49",    "LZ7",
					"LZAPU",   "LZEXO",    "LZX0",    "LZX7",    "LZSA1",  "LZSA2",
					"ZX0",     "ZX7",
					nullptr};
				for (size_t i = 0; kZ80DefinitionDirectives[i] != nullptr; i++)
					sDefinitionDirectivesSet.insert(kZ80DefinitionDirectives[i]);
			}
			//
			// Convert string to uppercase
			//
			static std::string ToUpper(const std::string& str) {
				std::string result = str;
				for (size_t i = 0; i < result.length(); i++) {
					if (result[i] >= 'a' && result[i] <= 'z')
						result[i] = result[i] - 'a' + 'A';
				}
				return result;
			}
			//
			// Trim leading and trailing whitespace and tabs
			//
			static std::string Trim(const std::string& str) {
				if (str.empty())
					return str;
				size_t start = 0;
				while (start < str.length() && (str[start] == ' ' || str[start] == '\t'))
					start++;
				if (start >= str.length())
					return "";
				size_t end = str.length() - 1;
				while (end > start && (str[end] == ' ' || str[end] == '\t'))
					end--;
				return str.substr(start, end - start + 1);
			}
			//
			// Split string by delimiter
			//
			static std::vector<std::string> Split(const std::string& str, char delimiter) {
				std::vector<std::string> tokens;
				size_t start = 0;
				while (start < str.length()) {
					while (start < str.length() && (str[start] == ' ' || str[start] == '\t'))
						start++;
					if (start >= str.length())
						break;
					size_t end = start;
					while (end < str.length() && str[end] != ' ' && str[end] != '\t' && str[end] != delimiter)
						end++;
					if (end > start)
						tokens.push_back(str.substr(start, end - start));
					start = end;
					if (start < str.length() && str[start] == delimiter) {
						start++;
						while (start < str.length() && (str[start] == ' ' || str[start] == '\t'))
							start++;
					}
				}
				return tokens;
			}
			//
			// Normalize a single parameter for instruction matching
			//
			static std::string NormalizeParameter(const std::string& p, std::string& prevParam, const std::string& mnemonic) {
				if (p.empty())
					return "";
				//
				// Remap short register aliases to canonical names before any lookup
				//
				const std::string& param = (p == "XH")	 ? std::string("IXH")
										   : (p == "XL") ? std::string("IXL")
										   : (p == "YH") ? std::string("IYH")
										   : (p == "YL") ? std::string("IYL")
														 : p;
				if (param[0] == '(') {
					size_t close = param.find(')');
					if (close == std::string::npos)
						return "(nn)";
					std::string inside = param.substr(1, close - 1);
					if (inside == "C")
						return "(C)";
					bool insideIsNum = !inside.empty() && ((inside[0] >= '0' && inside[0] <= '9') || inside[0] == '$' || inside[0] == '#' || inside[0] == '%');
					if ((mnemonic == "IN" || mnemonic == "OUT") && insideIsNum)
						return "(n)";
					if (inside.find('+') != std::string::npos || inside.find('-') != std::string::npos) {
						size_t plusPos = inside.find('+');
						size_t minusPos = inside.find('-');
						size_t opPos = (plusPos != std::string::npos) ? plusPos : minusPos;
						std::string reg = inside.substr(0, opPos);
						if (IsInSet(reg, sRegisters16Set))
							return "(" + reg + "+o)";
						if (IsInSet(prevParam, sRegisters8Set) && prevParam != "A")
							return "n";
						return "(nn)";
					}
					if (IsInSet(inside, sRegisters16Set))
						return "(" + inside + ")";
					if (IsInSet(prevParam, sRegisters8Set) && prevParam != "A")
						return "n";
					return "(nn)";
				}
				if ((mnemonic == "JP" || mnemonic == "JR" || mnemonic == "CALL" || mnemonic == "RET") && IsInSet(param, sConditionCodesSet))
					return param;
				if (param[0] == '{') {
					if (IsInSet(prevParam, sRegisters16Set))
						return "nn";
					if (prevParam == "(C)")
						return "A";
					return "n";
				}
				if (IsInSet(param, sRegisters16Set))
					return param;
				if (IsInSet(param, sRegisters8Set))
					return param;
				bool isNum = !param.empty() && ((param[0] >= '0' && param[0] <= '9') || param[0] == '$' || param[0] == '#' || param[0] == '%');
				if (isNum) {
					if (mnemonic == "OUT" && prevParam == "(C)" && param == "0")
						return "0";
					if (mnemonic == "SET" || mnemonic == "BIT" || mnemonic == "RES")
						return param;
					if (mnemonic == "IM")
						return param;
					if (mnemonic == "JR" || mnemonic == "DJNZ")
						return "o";
					if (mnemonic == "CALL" || mnemonic == "JP" || mnemonic == "PUSH")
						return "nn";
					if (IsInSet(prevParam, sRegisters16Set))
						return "nn";
					return "n";
				}
				if ((mnemonic == "SET" || mnemonic == "BIT" || mnemonic == "RES") && prevParam.empty())
					return "0";
				if (mnemonic == "JR" || mnemonic == "DJNZ")
					return "o";
				if (mnemonic == "CALL" || mnemonic == "JP" || mnemonic == "PUSH")
					return "nn";
				if (IsInSet(prevParam, sRegisters16Set))
					return "nn";
				return "n";
			}
			//
			// Helper function to normalize instruction for matching
			// Converts to uppercase and replaces operands with wildcards
			//
			static std::string NormalizeInstruction(const std::string& instruction) {
				std::string upper = ToUpper(instruction);
				std::vector<std::string> tokens = Split(upper, ' ');
				if (tokens.empty())
					return "";
				std::string mnemonic;
				size_t paramStartIndex = 0;
				for (size_t i = 0; i < tokens.size(); i++) {
					if (IsValidMnemonic(tokens[i])) {
						mnemonic = tokens[i];
						paramStartIndex = i + 1;
						break;
					}
				}
				if (mnemonic.empty())
					return tokens[0];
				if (paramStartIndex >= tokens.size())
					return mnemonic;
				//
				// Join all remaining tokens to handle cases like "LD A , B ; comment"
				//
				std::string paramsText = "";
				for (size_t i = paramStartIndex; i < tokens.size(); i++)
					paramsText += tokens[i];
				//
				// Remove comment if present
				//
				size_t commentPos = paramsText.find(';');
				if (commentPos != std::string::npos)
					paramsText = paramsText.substr(0, commentPos);
				std::vector<std::string> params = Split(paramsText, ',');
				std::string result = mnemonic;
				std::string prevParam = "";
				for (size_t i = 0; i < params.size(); i++) {
					if (i == 0)
						result += " ";
					else
						result += ",";
					std::string param = Trim(params[i]);
					result += NormalizeParameter(param, prevParam, mnemonic);
					prevParam = ToUpper(Trim(params[i]));
				}
				return result;
			}
			//
			// Finds equivalent normalized instruction syntax using explicit alias rules.
			//
			static std::string FindEquivalentInstruction(const std::string& normalizedInstruction) {
				auto it = sAliasMap.find(normalizedInstruction);
				if (it != sAliasMap.end())
					return it->second;
				return "";
			}
			//
			// Resolve instruction entry, trying equivalent mnemonic forms if needed
			//
			static const Z80Instruction* FindInstructionEntry(const std::string& normalizedInstruction) {
				auto it = sInstructionMap.find(normalizedInstruction);
				if (it != sInstructionMap.end())
					return it->second;
				std::string equivalentInstruction = FindEquivalentInstruction(normalizedInstruction);
				if (equivalentInstruction.empty())
					return nullptr;
				auto it2 = sInstructionMap.find(equivalentInstruction);
				if (it2 != sInstructionMap.end())
					return it2->second;
				return nullptr;
			}
			//
			// Normalizes Z80 symbol names so label forms with trailing ':' map to the same logical symbol.
			//
			static std::string NormalizeZ80SymbolName(const std::string& symbolName) {
				if (!symbolName.empty() && symbolName.back() == ':')
					return symbolName.substr(0, symbolName.size() - 1);
				return symbolName;
			}
			//
			// Builds a synthetic symbol name for repeat codelens entries.
			//
			static std::string BuildRepeatCodeLensSymbolName(const std::string& filePath, int lineNumber) {
				return filePath + ":" + std::to_string(lineNumber) + ":REPEAT";
			}
			//
			// Parse decimal integer from a raw pointer range to avoid string allocations
			//
			static int ParseIntPtr(const char* begin) {
				char* end = nullptr;
				long value = std::strtol(begin, &end, 10);
				if (end == begin)
					return 0;
				return (int)value;
			}
			//
			// Multiply a timing expression by repeat count (supports "a" and "a/b").
			//
			static std::string MultiplyTiming(const std::string& timing, int repeatCount) {
				if (timing.empty())
					return "";
				if (repeatCount <= 0)
					return "0";
				size_t slashPos = timing.find('/');
				if (slashPos == std::string::npos)
					return std::to_string(ParseIntPtr(timing.c_str()) * repeatCount);
				int first = ParseIntPtr(timing.c_str());
				int second = ParseIntPtr(timing.c_str() + slashPos + 1);
				return std::to_string(first * repeatCount) + "/" + std::to_string(second * repeatCount);
			}
			//
			// Add two timing strings (e.g. "17" + "12/7" => "29/24")
			//
			static std::string SumTiming(const std::string& current, const std::string& add) {
				if (add.empty())
					return current;
				auto parseTiming = [](const std::string& value, int& first, int& second, bool& hasAlt) {
					size_t slashPos = value.find('/');
					if (slashPos == std::string::npos) {
						first = ParseIntPtr(value.c_str());
						second = first;
						hasAlt = false;
						return;
					}
					first = ParseIntPtr(value.c_str());
					second = ParseIntPtr(value.c_str() + slashPos + 1);
					hasAlt = true;
				};
				if (current.empty())
					return add;
				int c1 = 0;
				int c2 = 0;
				int a1 = 0;
				int a2 = 0;
				bool cAlt = false;
				bool aAlt = false;
				parseTiming(current, c1, c2, cAlt);
				parseTiming(add, a1, a2, aAlt);
				int sum1 = c1 + a1;
				int sum2 = c2 + a2;
				if (cAlt || aAlt)
					return std::to_string(sum1) + "/" + std::to_string(sum2);
				return std::to_string(sum1);
			}
			//
			// Extract instructions from line text (skip labels and directives)
			//
			static std::vector<std::string> ExtractInstructions(const std::string& lineText) {
				std::vector<std::string> instructions;
				size_t start = 0;
				while (start < lineText.length() && (lineText[start] == ' ' || lineText[start] == '\t'))
					start++;
				if (start >= lineText.length())
					return instructions;
				size_t commentPos = lineText.find(';', start);
				std::string code = commentPos != std::string::npos ? lineText.substr(start, commentPos - start) : lineText.substr(start);
				// Split ':' segments to handle "label: instruction" and chained labels.
				size_t segmentStart = 0;
				while (segmentStart < code.length()) {
					size_t segmentEnd = code.find(':', segmentStart);
					std::string segment = segmentEnd != std::string::npos ? code.substr(segmentStart, segmentEnd - segmentStart) : code.substr(segmentStart);
					segment = Trim(segment);
					if (!segment.empty() && !IsDirective(segment)) {
						std::string mnemonic = ExtractMnemonic(segment);
						if (IsValidMnemonic(mnemonic) || (segmentEnd == std::string::npos && !mnemonic.empty()))
							instructions.push_back(segment);
					}
					if (segmentEnd == std::string::npos)
						break;
					segmentStart = segmentEnd + 1;
				}
				return instructions;
			}
			//
			// Check if a line starts with a directive (not an instruction)
			//
			static bool IsDirective(const std::string& text) {
				std::string upper = text;
				for (size_t i = 0; i < upper.length(); i++) {
					if (upper[i] >= 'a' && upper[i] <= 'z')
						upper[i] = upper[i] - 'a' + 'A';
				}
				size_t i = 0;
				while (i < upper.length() && (upper[i] == ' ' || upper[i] == '\t'))
					i++;
				size_t tokenStart = i;
				while (i < upper.length() && upper[i] != ' ' && upper[i] != '\t')
					i++;
				std::string firstToken = upper.substr(tokenStart, i - tokenStart);
				return sDirectiveSet.count(firstToken) != 0;
			}
			//
			// Returns true when line is a comment line (after trim starts with ';').
			//
			static bool IsCommentLine(const std::string& lineText) {
				size_t i = 0;
				while (i < lineText.size() && (lineText[i] == ' ' || lineText[i] == '\t'))
					i++;
				return i < lineText.size() && lineText[i] == ';';
			}
			//
			// Returns true when line is empty or comment-only.
			//
			static bool IsEmptyOrCommentLine(const std::string& lineText) {
				return ctre::match<R"(\s*(;.*)?)">( lineText).operator bool();
			}
			//
			// Tokenizes a line using the same language tokenizers used by highlighting.
			//
			static std::vector<std::string> ExtractIdentifierTokens(const std::string& lineText) {
				std::vector<std::string> tokens;
				if (lineText.empty())
					return tokens;
				const char* begin = lineText.c_str();
				const char* end = begin + lineText.size();
				const char* p = begin;
				while (p < end) {
					while (p < end && isascii(*p) && isblank(*p))
						p++;
					if (p >= end || *p == ';')
						break;
					const char* tokenBegin = nullptr;
					const char* tokenEnd = nullptr;
					if (TokenizeZ80String(p, end, tokenBegin, tokenEnd) || TokenizeZ80Number(p, end, tokenBegin, tokenEnd) ||
						TokenizeZ80Punctuation(p, end, tokenBegin, tokenEnd)) {
						p = tokenEnd;
						continue;
					}
					if (TokenizeZ80Identifier(p, end, tokenBegin, tokenEnd)) {
						tokens.push_back(ToUpper(std::string(tokenBegin, tokenEnd)));
						p = tokenEnd;
						continue;
					}
					p++;
				}
				return tokens;
			}
			//
			// Parses tokens to detect a macro start declaration (NAME MACRO).
			//
			static bool TryParseMacroStart(const std::vector<std::string>& tokens, std::string& outMacroName) {
				if (tokens.size() < 2)
					return false;
				if (tokens[0] == "MACRO") {
					outMacroName = tokens[1];
					return !outMacroName.empty();
				}
				for (size_t i = 1; i < tokens.size(); i++) {
					if (tokens[i] == "MACRO") {
						outMacroName = tokens[i - 1];
						return !outMacroName.empty();
					}
				}
				return false;
			}
			//
			// Parses tokens to detect macro end directives.
			//
			static bool IsMacroEndLine(const std::string& lineText) {
				return ctre::search<R"((?i)(?:^|\s|:)(ENDM|ENDMACRO|MEND)(?:\s|;|$))">(
					std::string_view(lineText)).operator bool();
			}
			//
			// Parses line text to detect a global function start label (identifier or identifier:).
			//
			//
			// outNeedsLookahead=true: bare label with nothing after it; caller must
			// defer the function-open decision until the next non-empty line is seen,
			// then discard if that line starts with a data-definition directive.
			// outNeedsLookahead=false: label confirmed as a function start (instruction
			// or other non-data content follows on the same line, or inside a function).
			//
			static bool TryParseFunctionStart(const std::string& lineText, std::string& outFunctionName, bool& outNeedsLookahead) {
				std::string_view sv(lineText);
				auto semiPos = sv.find(';');
				if (semiPos != std::string_view::npos)
					sv = sv.substr(0, semiPos);
				auto m = ctre::search<R"(^[a-zA-Z_.@][a-zA-Z0-9_.@?]*)">(sv);
				if (!m)
					return false;
				std::string candidate = ToUpper(std::string(m.get<0>().to_view()));
				if (candidate.empty())
					return false;
				if (candidate[0] == '.' || candidate[0] == '@')
					return false;
				if (IsInSet(candidate, sDirectiveSet) || IsValidMnemonic(candidate))
					return false;
				if (candidate == "MACRO" || candidate == "MEND" || candidate == "ENDM" ||
					candidate == "ENDMACRO" || candidate == "REPEAT" || candidate == "REPT")
					return false;
				// Examine what follows the identifier
				size_t pos = m.get<0>().to_view().size();
				while (pos < sv.size() && (sv[pos] == ' ' || sv[pos] == '\t'))
					pos++;
				if (pos < sv.size() && sv[pos] == ':')
					pos++;
				while (pos < sv.size() && (sv[pos] == ' ' || sv[pos] == '\t'))
					pos++;
				if (pos >= sv.size()) {
					// Nothing after the label — bare label, defer decision
					outFunctionName = candidate;
					outNeedsLookahead = true;
					return true;
				}
				// Something follows. Extract the first token.
				size_t tokStart = pos;
				while (pos < sv.size() && sv[pos] != ' ' && sv[pos] != '\t' && sv[pos] != ',' && sv[pos] != ';')
					pos++;
				std::string nextToken = ToUpper(std::string(sv.substr(tokStart, pos - tokStart)));
				if (IsInSet(nextToken, sDefinitionDirectivesSet) || IsInSet(nextToken, sDirectiveSet))
					return false;  // data or assembler directive on the same line — not a function
				// Instruction (or macro call) after label — confirmed function start
				outFunctionName = candidate;
				outNeedsLookahead = false;
				return true;
			}
			//
			// Detects function end when a return instruction appears.
			//
			static bool ContainsFunctionReturn(const std::string& lineText) {
				return ctre::search<R"((?i)(?:^|[\s:])RET(?:I|N)?(?:[\s;:]|$))">(
					std::string_view(lineText)).operator bool();
			}
			//
			// Parses line text to detect repeat block start (REPEAT/REPT n).
			//
			static bool TryParseRepeatStart(const std::string& lineText, std::string& outKeyword, int& outCount) {
				if (auto m = ctre::search<R"((?i)(?:^|\s|:)(REPEAT|REPT)\s+(\d+))">(std::string_view(lineText))) {
					outKeyword = ToUpper(std::string(m.get<1>().to_view()));
					outCount = 0;
					for (char c : m.get<2>().to_view())
						outCount = outCount * 10 + (c - '0');
					return true;
				}
				return false;
			}
			//
			// Detects repeat block end token (ENDREPEAT/ENDREPT).
			//
			static bool IsRepeatEndLine(const std::string& lineText) {
				return ctre::search<R"((?i)(?:^|\s|:)(ENDREPEAT|ENDREPT|REND)(?:\s|;|$))">(
					std::string_view(lineText)).operator bool();
			}
			//
			// Parses tokens to detect symbol definition lines (identifier + data/constant directive).
			//
			static bool TryParseSymbolDefinition(const std::vector<std::string>& tokens, std::string& outSymbolName) {
				if (tokens.size() < 2)
					return false;
				if (IsInSet(tokens[0], sDirectiveSet))
					return false;
				if (!IsInSet(tokens[1], sDefinitionDirectivesSet))
					return false;
				outSymbolName = tokens[0];
				return !outSymbolName.empty();
			}
			//
			// Returns the appropriate timing string for a given instruction entry and timing mode.
			//
			static const char* GetTimingForEntry(const Z80Instruction* entry, Z80TimingType timingType) {
				switch (timingType) {
					case Z80TimingType::CyclesM1:
						return entry->timingZ80M1;
					case Z80TimingType::Instructions:
						return entry->timingCPC;
					default:
						return entry->timingZ80;
				}
			}

			//
			// Z80AsmParser - stateful per-parse-job implementation.
			// One instance per parse job; owns all parse state as instance members.
			//
			class Z80AsmParser : public ILanguageParser {
			public:
				Z80AsmParser(TextEditorMI* host, const Z80AsmLanguage* langDef)
					: mHost(host), mLangDef(langDef) {}

				void ParseStart(const std::string& filePath) override {
					mCollectErrors = true;
					mCurrentFilePath = filePath;
					InjectDirectiveSymbols();
					mRecentLines.clear();
					mInsideMacro = false;
					mMacroName.clear();
					mMacroStartLine = -1;
					mMacroCommentStartLine = -1;
					mMacroCapturedLines.clear();
					mMacroTiming.clear();
					mMacroBlockId = -1;
					mMacroTracker = mHost->AllocateBlockTracker();
					mInsideFunction = false;
					mFunctionBlocks.clear();
					mPendingFunctionName.clear();
					mPendingFunctionLine = -1;
					mPendingFunctionCommentLine = -1;
					mRepeatBlocks.clear();
					mRepeatTracker = mHost->AllocateBlockTracker();
				}

				void ParseLine(int lineNumber, const std::string& filePath, const std::string& lineText) override {
					mRecentLines.push_back(std::make_pair(lineNumber, lineText));
					if (mRecentLines.size() > 20)
						mRecentLines.pop_front();
					std::vector<std::string> tokens = ExtractIdentifierTokens(lineText);
					if (!mInsideMacro) {
						std::string macroName;
						if (TryParseMacroStart(tokens, macroName)) {
							macroName = NormalizeZ80SymbolName(macroName);
							macroName = ToUpper(macroName);
							mHost->DeleteCodeLensSymbol("*", macroName);
							mInsideFunction = false;
							mFunctionBlocks.clear();
							mInsideMacro = true;
							mMacroName = macroName;
							mMacroStartLine = lineNumber;
							mMacroCommentStartLine = FindLeadingCommentStartLineInRecent();
							mMacroCapturedLines.clear();
							mMacroTiming.clear();
							mMacroBlockId = mHost->OpenBlock(mMacroTracker, lineNumber, 0, "MACRO");
							if (mMacroCommentStartLine >= 0) {
								for (size_t i = 0; i < mRecentLines.size(); i++)
									if (mRecentLines[i].first >= mMacroCommentStartLine)
										mMacroCapturedLines.push_back(mRecentLines[i]);
							} else
								mMacroCapturedLines.push_back(std::make_pair(lineNumber, lineText));
						}
					}
					std::string repeatKeyword;
					int repeatCount = 0;
					bool isRepeatStart = TryParseRepeatStart(lineText, repeatKeyword, repeatCount);
					bool isRepeatEnd = IsRepeatEndLine(lineText);
					if (isRepeatStart) {
						RepeatBlockState block;
						block.keyword = repeatKeyword;
						block.startLine = lineNumber;
						block.repeatCount = repeatCount;
						mRepeatBlocks.push_back(block);
						mRepeatBlocks.back().blockId = mHost->OpenBlock(mRepeatTracker, lineNumber, 0, "REPEAT");
					}
					if (!isRepeatStart && !isRepeatEnd && !mRepeatBlocks.empty() && !IsEmptyOrCommentLine(lineText)) {
						std::string timing = ComputeLineTiming(lineNumber, lineText, false);
						if (timing.empty())
							timing = ResolveMacroTiming(tokens);
						mRepeatBlocks.back().timing = SumTiming(mRepeatBlocks.back().timing, timing);
					}
					if (isRepeatEnd && !mRepeatBlocks.empty()) {
						RepeatBlockState block = mRepeatBlocks.back();
						mRepeatBlocks.pop_back();
						if (block.blockId >= 0)
							mHost->CloseBlock(mRepeatTracker, block.blockId);
						std::string totalTiming = MultiplyTiming(block.timing, block.repeatCount);
						CodeLensSymbolData repeatSymbol;
						repeatSymbol.symbolName = BuildRepeatCodeLensSymbolName(filePath, block.startLine);
						repeatSymbol.lineNumber = block.startLine;
						repeatSymbol.timings = totalTiming;
						repeatSymbol.opcodes = "";
						repeatSymbol.codelensText = std::string("repeat timing: ") + (totalTiming.empty() ? "n/a" : totalTiming);
						repeatSymbol.externalCode = "";
						mHost->AddCodeLensSymbol(filePath, repeatSymbol);
						if (!mRepeatBlocks.empty())
							mRepeatBlocks.back().timing = SumTiming(mRepeatBlocks.back().timing, totalTiming);
						else if (mInsideFunction) {
							for (size_t fi = 0; fi < mFunctionBlocks.size(); fi++)
								mFunctionBlocks[fi].timing = SumTiming(mFunctionBlocks[fi].timing, totalTiming);
						}
					}
					bool isSymbolDefinition = false;
					if (!mInsideMacro) {
						std::string symbolName;
						if (TryParseSymbolDefinition(tokens, symbolName)) {
							symbolName = ToUpper(NormalizeZ80SymbolName(symbolName));
							isSymbolDefinition = true;
							CodeLensSymbolData symbol;
							symbol.symbolName = symbolName;
							symbol.lineNumber = lineNumber;
							symbol.codelensText = "";
							symbol.kind = CodeLensSymbolKind::UserDefined;
							int commentStartLine = FindLeadingCommentStartLineInRecent();
							std::string external;
							if (commentStartLine >= 0) {
								for (size_t i = 0; i < mRecentLines.size(); i++)
									if (mRecentLines[i].first >= commentStartLine) {
										if (!external.empty())
											external += "\n";
										external += mRecentLines[i].second;
									}
							} else
								external = lineText;
							symbol.externalCode = external;
							mHost->DeleteCodeLensSymbol("*", symbolName);
							mHost->AddCodeLensSymbol(filePath, symbol);
						}
					}
					if (!mInsideMacro && !mInsideFunction) {
						// Resolve pending bare label: next non-empty line decides function vs data.
						if (!mPendingFunctionName.empty() && !IsEmptyOrCommentLine(lineText)) {
							if (!tokens.empty() && (IsInSet(tokens[0], sDefinitionDirectivesSet) || IsInSet(tokens[0], sDirectiveSet))) {
								// Data or assembler directive follows — register pending label as resource symbol
								{
									CodeLensSymbolData dataLabel;
									dataLabel.symbolName = mPendingFunctionName;
									dataLabel.lineNumber = mPendingFunctionLine;
									dataLabel.codelensText = "";
									dataLabel.kind = CodeLensSymbolKind::UserDefined;
									std::string external;
									int startLine = (mPendingFunctionCommentLine >= 0) ? mPendingFunctionCommentLine : mPendingFunctionLine;
									for (size_t i = 0; i < mRecentLines.size(); i++) {
										if (mRecentLines[i].first >= startLine) {
											if (!external.empty())
												external += "\n";
											external += mRecentLines[i].second;
										}
									}
									dataLabel.externalCode = external;
									mHost->DeleteCodeLensSymbol("*", mPendingFunctionName);
									mHost->AddCodeLensSymbol(filePath, dataLabel);
								}
								mPendingFunctionName.clear();
								mPendingFunctionLine = -1;
								mPendingFunctionCommentLine = -1;
							} else {
								// Code follows — activate pending label as function start
								if (mFunctionBlocks.size() < kMaxOpenFunctionBlocks) {
									FunctionBlockState block;
									block.name = mPendingFunctionName;
									block.startLine = mPendingFunctionLine;
									mInsideFunction = true;
									if (mPendingFunctionCommentLine >= 0) {
										for (size_t i = 0; i < mRecentLines.size(); i++)
											if (mRecentLines[i].first >= mPendingFunctionCommentLine)
												block.capturedLines.push_back(mRecentLines[i]);
									} else {
										for (size_t i = 0; i < mRecentLines.size(); i++)
											if (mRecentLines[i].first == mPendingFunctionLine) {
												block.capturedLines.push_back(mRecentLines[i]);
												break;
											}
									}
									mFunctionBlocks.push_back(block);
								}
								mPendingFunctionName.clear();
								mPendingFunctionLine = -1;
								mPendingFunctionCommentLine = -1;
							}
						}
						if (!mInsideFunction && !isSymbolDefinition && mRepeatBlocks.empty()) {
							std::string functionName;
							bool needsLookahead = false;
							if (TryParseFunctionStart(lineText, functionName, needsLookahead)) {
								functionName = ToUpper(NormalizeZ80SymbolName(functionName));
								if (needsLookahead) {
									mPendingFunctionName = functionName;
									mPendingFunctionLine = lineNumber;
									mPendingFunctionCommentLine = FindLeadingCommentStartLineInRecent();
								} else if (mFunctionBlocks.size() < kMaxOpenFunctionBlocks) {
									FunctionBlockState block;
									block.name = functionName;
									block.startLine = lineNumber;
									mInsideFunction = true;
									int functionCommentStartLine = FindLeadingCommentStartLineInRecent();
									if (functionCommentStartLine >= 0) {
										for (size_t i = 0; i < mRecentLines.size(); i++)
											if (mRecentLines[i].first >= functionCommentStartLine)
												block.capturedLines.push_back(mRecentLines[i]);
									} else
										block.capturedLines.push_back(std::make_pair(lineNumber, lineText));
									mFunctionBlocks.push_back(block);
								}
							}
						}
						if (mCollectErrors && !mCurrentFilePath.empty() && !IsEmptyOrCommentLine(lineText)) {
							std::vector<std::string> instructions = ExtractInstructions(lineText);
							bool hasKnownMnemonic = false;
							for (size_t i = 0; i < instructions.size(); i++) {
								if (IsValidMnemonic(ExtractMnemonic(instructions[i]))) {
									hasKnownMnemonic = true;
									break;
								}
							}
							if (hasKnownMnemonic)
								ComputeLineTiming(lineNumber, lineText, true);
						}
						return;
					}
					if (mInsideMacro) {
						if (!(mMacroCapturedLines.size() == 1 && mMacroCapturedLines[0].first == lineNumber))
							mMacroCapturedLines.push_back(std::make_pair(lineNumber, lineText));
						if (!IsEmptyOrCommentLine(lineText)) {
							std::string timing = ComputeLineTiming(lineNumber, lineText, false);
							mMacroTiming = SumTiming(mMacroTiming, timing);
						}
						if (IsMacroEndLine(lineText)) {
							CodeLensSymbolData symbol;
							symbol.symbolName = mMacroName;
							symbol.lineNumber = mMacroStartLine;
							symbol.timings = mMacroTiming;
							symbol.opcodes = "";
							symbol.codelensText = std::string("macro timing: ") + (mMacroTiming.empty() ? "n/a" : mMacroTiming);
							symbol.referenceDisplay = true;
							std::string external;
							for (size_t i = 0; i < mMacroCapturedLines.size(); i++) {
								if (!external.empty())
									external += "\n";
								external += mMacroCapturedLines[i].second;
							}
							symbol.externalCode = external;
							mHost->DeleteCodeLensSymbol("*", mMacroName);
							mHost->AddCodeLensSymbol(filePath, symbol);
							if (mMacroBlockId >= 0)
								mHost->CloseBlock(mMacroTracker, mMacroBlockId);
							mInsideMacro = false;
							mMacroName.clear();
							mMacroStartLine = -1;
							mMacroCommentStartLine = -1;
							mMacroCapturedLines.clear();
							mMacroTiming.clear();
							mMacroBlockId = -1;
						}
						return;
					}
					if (mInsideFunction) {
						std::string nestedFunctionName;
						bool nestedNeedsLookahead = false;
						if (TryParseFunctionStart(lineText, nestedFunctionName, nestedNeedsLookahead)) {
							nestedFunctionName = ToUpper(NormalizeZ80SymbolName(nestedFunctionName));
							if (mFunctionBlocks.size() < kMaxOpenFunctionBlocks) {
								FunctionBlockState nestedBlock;
								nestedBlock.name = nestedFunctionName;
								nestedBlock.startLine = lineNumber;
								int nestedCommentStartLine = FindLeadingCommentStartLineInRecent();
								if (nestedCommentStartLine >= 0) {
									for (size_t i = 0; i < mRecentLines.size(); i++)
										if (mRecentLines[i].first >= nestedCommentStartLine)
											nestedBlock.capturedLines.push_back(mRecentLines[i]);
								} else
									nestedBlock.capturedLines.push_back(std::make_pair(lineNumber, lineText));
								mFunctionBlocks.push_back(nestedBlock);
							}
						}
						for (size_t i = 0; i < mFunctionBlocks.size(); i++) {
							FunctionBlockState& block = mFunctionBlocks[i];
							if (block.capturedLines.empty() || block.capturedLines.back().first != lineNumber)
								block.capturedLines.push_back(std::make_pair(lineNumber, lineText));
							if (!IsEmptyOrCommentLine(lineText) && mRepeatBlocks.empty()) {
								std::string timing = ComputeLineTiming(lineNumber, lineText, false);
								if (timing.empty())
									timing = ResolveMacroTiming(tokens);
								block.timing = SumTiming(block.timing, timing);
							}
						}
						if (ContainsFunctionReturn(lineText)) {
							for (size_t functionIndex = 0; functionIndex < mFunctionBlocks.size(); functionIndex++) {
								const FunctionBlockState& block = mFunctionBlocks[functionIndex];
								CodeLensSymbolData symbol;
								symbol.symbolName = block.name;
								symbol.lineNumber = block.startLine;
								symbol.timings = block.timing;
								symbol.opcodes = "";
								symbol.codelensText = std::string("function timing: ") + (block.timing.empty() ? "n/a" : block.timing);
								std::string external;
								size_t maxLines = block.capturedLines.size();
								if (maxLines > 10)
									maxLines = 10;
								for (size_t lineIndex = 0; lineIndex < maxLines; lineIndex++) {
									if (!external.empty())
										external += "\n";
									external += block.capturedLines[lineIndex].second;
								}
								symbol.externalCode = external;
								mHost->AddCodeLensSymbolIfNew(filePath, symbol);
							}
							mInsideFunction = false;
							mFunctionBlocks.clear();
						}
						return;
					}
				}

				void ParseEnd(const std::string& filePath, const std::vector<UnclosedBlock>& unclosedBlocks) override {
					for (size_t i = 0; i < unclosedBlocks.size(); i++) {
						if (unclosedBlocks[i].trackerHandle == mMacroTracker)
							mHost->AddCodeLensError(filePath, unclosedBlocks[i].line, "Macro was not closed (missing ENDM/ENDMACRO)");
						else
							mHost->AddCodeLensError(filePath, unclosedBlocks[i].line, "Repeat block was not closed (missing ENDREPEAT/ENDREPT/REND)");
					}
					//
					// Emit any function blocks still open at end of file (e.g. functions that
					// terminate with jp instead of ret and never trigger ContainsFunctionReturn).
					//
					if (mInsideFunction) {
						for (size_t functionIndex = 0; functionIndex < mFunctionBlocks.size(); functionIndex++) {
							const FunctionBlockState& block = mFunctionBlocks[functionIndex];
							CodeLensSymbolData symbol;
							symbol.symbolName = block.name;
							symbol.lineNumber = block.startLine;
							symbol.timings = block.timing;
							symbol.opcodes = "";
							symbol.codelensText = std::string("function timing: ") + (block.timing.empty() ? "n/a" : block.timing);
							std::string external;
							size_t maxLines = block.capturedLines.size();
							if (maxLines > 10)
								maxLines = 10;
							for (size_t lineIndex = 0; lineIndex < maxLines; lineIndex++) {
								if (!external.empty())
									external += "\n";
								external += block.capturedLines[lineIndex].second;
							}
							symbol.externalCode = external;
							mHost->AddCodeLensSymbolIfNew(filePath, symbol);
						}
					}
					mCollectErrors = false;
					mCurrentFilePath.clear();
					mInsideMacro = false;
					mMacroName.clear();
					mMacroStartLine = -1;
					mMacroCommentStartLine = -1;
					mMacroCapturedLines.clear();
					mMacroTiming.clear();
					mMacroBlockId = -1;
					mInsideFunction = false;
					mFunctionBlocks.clear();
					mPendingFunctionName.clear();
					mPendingFunctionLine = -1;
					mPendingFunctionCommentLine = -1;
					mRepeatBlocks.clear();
				}

				std::string GetLineTiming(const std::string& lineText) const override {
					std::vector<std::string> instructions = ExtractInstructions(lineText);
					if (instructions.empty())
						return "";
					Z80TimingType timingType = mLangDef->mTimingType;
					std::string totalTiming;
					for (const auto& instruction : instructions) {
						std::string mnemonic = ExtractMnemonic(instruction);
						if (!IsValidMnemonic(mnemonic))
							continue;
						std::string normalized = NormalizeInstruction(instruction);
						const Z80Instruction* entry = FindInstructionEntry(normalized);
						if (!entry)
							continue;
						totalTiming = SumTiming(totalTiming, GetTimingForEntry(entry, timingType));
					}
					return totalTiming;
				}

				std::string GetLineBytecode(const std::string& lineText) const override {
					std::vector<std::string> instructions = ExtractInstructions(lineText);
					if (instructions.empty())
						return "";
					std::string opcodes;
					for (const auto& instruction : instructions) {
						std::string normalized = NormalizeInstruction(instruction);
						const Z80Instruction* entry = FindInstructionEntry(normalized);
						if (!entry)
							continue;
						if (!opcodes.empty())
							opcodes += " ";
						opcodes += entry->opcode;
					}
					return opcodes;
				}

				bool SyntaxHighlight(const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd,
					TextEditor::PaletteIndex& paletteIndex) const override {
					paletteIndex = TextEditor::PaletteIndex::Max;
					while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
						inBegin++;
					if (inBegin == inEnd) {
						outBegin = inEnd;
						outEnd = inEnd;
						paletteIndex = TextEditor::PaletteIndex::Default;
					} else if (TokenizeZ80String(inBegin, inEnd, outBegin, outEnd))
						paletteIndex = TextEditor::PaletteIndex::String;
					else if (*inBegin == '$' && (inBegin + 1 == inEnd ||
						!((inBegin[1] >= '0' && inBegin[1] <= '9') || (inBegin[1] >= 'a' && inBegin[1] <= 'f') || (inBegin[1] >= 'A' && inBegin[1] <= 'F')))) {
						outBegin = inBegin;
						outEnd = inBegin + 1;
						paletteIndex = TextEditor::PaletteIndex::Identifier;
					} else if (TokenizeZ80Number(inBegin, inEnd, outBegin, outEnd))
						paletteIndex = TextEditor::PaletteIndex::Number;
					else if (TokenizeZ80Identifier(inBegin, inEnd, outBegin, outEnd))
						paletteIndex = TextEditor::PaletteIndex::Identifier;
					else if (TokenizeZ80Punctuation(inBegin, inEnd, outBegin, outEnd))
						paletteIndex = TextEditor::PaletteIndex::Punctuation;
					return paletteIndex != TextEditor::PaletteIndex::Max;
				}

			private:
				TextEditorMI* mHost;
				const Z80AsmLanguage* mLangDef;
				std::deque<std::pair<int, std::string>> mRecentLines;
				bool mInsideMacro = false;
				std::string mMacroName;
				int mMacroStartLine = -1;
				int mMacroCommentStartLine = -1;
				std::vector<std::pair<int, std::string>> mMacroCapturedLines;
				std::string mMacroTiming;
				int mMacroTracker = -1;
				int mMacroBlockId = -1;
				bool mInsideFunction = false;
				static constexpr size_t kMaxOpenFunctionBlocks = 5;
				std::vector<FunctionBlockState> mFunctionBlocks;
				std::string mPendingFunctionName;
				int mPendingFunctionLine = -1;
				int mPendingFunctionCommentLine = -1;
				bool mCollectErrors = false;
				std::string mCurrentFilePath;
				std::vector<RepeatBlockState> mRepeatBlocks;
				int mRepeatTracker = -1;

				std::string FindSymbolTiming(const std::string& upperName) const {
					const auto& files = mHost->GetCodeLensFiles();
					for (size_t fi = 0; fi < files.size(); fi++)
						for (size_t si = 0; si < files[fi].symbols.size(); si++)
							if (files[fi].symbols[si].symbolName == upperName && !files[fi].symbols[si].timings.empty())
								return files[fi].symbols[si].timings;
					return "";
				}

				std::string ResolveMacroTiming(const std::vector<std::string>& tokens) const {
					for (size_t ti = 0; ti < tokens.size(); ti++) {
						const std::string& t = tokens[ti];
						if (sInstructionMap.count(t) || sRegisters8Set.count(t) ||
							sRegisters16Set.count(t) || sDirectiveSet.count(t))
							continue;
						std::string timing = FindSymbolTiming(t);
						if (!timing.empty())
							return timing;
					}
					return "";
				}

				std::string ComputeLineTiming(int lineNumber, const std::string& lineText, bool reportErrors) const {
					Z80TimingType timingType = mLangDef->mTimingType;
					std::vector<std::string> instructions = ExtractInstructions(lineText);
					if (instructions.empty())
						return "";
					std::string totalTiming;
					bool hasInvalidInstruction = false;
					for (const auto& instruction : instructions) {
						std::string mnemonic = ExtractMnemonic(instruction);
						if (!IsValidMnemonic(mnemonic))
							continue;
						std::string normalized = NormalizeInstruction(instruction);
						const Z80Instruction* entry = FindInstructionEntry(normalized);
						if (!entry) {
							hasInvalidInstruction = true;
							continue;
						}
						totalTiming = SumTiming(totalTiming, GetTimingForEntry(entry, timingType));
					}
					if (hasInvalidInstruction && reportErrors && !mCurrentFilePath.empty() && lineNumber >= 0)
						mHost->AddCodeLensError(mCurrentFilePath, lineNumber, "Invalid instruction");
					return totalTiming;
				}

				int FindLeadingCommentStartLineInRecent() const {
					if (mRecentLines.empty())
						return -1;
					int index = (int)mRecentLines.size() - 2;
					while (index >= 0 && IsCommentLine(mRecentLines[index].second))
						index--;
					index++;
					if (index >= 0 && index < (int)mRecentLines.size() - 1 && IsCommentLine(mRecentLines[index].second))
						return mRecentLines[index].first;
					return -1;
				}

				bool HasDirectivesInjected() const {
					const auto& files = mHost->GetCodeLensFiles();
					for (size_t fi = 0; fi < files.size(); fi++)
						for (size_t si = 0; si < files[fi].symbols.size(); si++)
							if (files[fi].symbols[si].kind == CodeLensSymbolKind::Directive)
								return true;
					return false;
				}

				void InjectDirectiveSymbols() {
					if (HasDirectivesInjected())
						return;
					static const char* const kApiFile = "<z80asm-directives>";
					auto addSym = [&](const DirectiveDef& def) {
						CodeLensSymbolData sym;
						sym.symbolName = def.name;
						sym.lineNumber = -1;
						sym.codelensText = "";
						sym.externalCode = std::string(def.syntax) + "\n\n" + def.description;
						sym.kind = CodeLensSymbolKind::Directive;
						mHost->AddCodeLensSymbolIfNew(kApiFile, sym);
					};
					for (size_t i = 0; i < sizeof(kZ80AssemblerDirectiveDefs) / sizeof(kZ80AssemblerDirectiveDefs[0]); i++)
						addSym(kZ80AssemblerDirectiveDefs[i]);
					for (size_t i = 0; i < sizeof(kZ80PreprocessorDirectiveDefs) / sizeof(kZ80PreprocessorDirectiveDefs[0]); i++)
						addSym(kZ80PreprocessorDirectiveDefs[i]);
				}
			};

		} // namespace Z80Asm
	} // namespace TextEditorLangs
} // namespace ImGui

namespace RetrodevGui {

	Z80AsmLanguage::Z80AsmLanguage() {
		ImGui::TextEditorLangs::Z80Asm::InitLookupTables();
		for (const auto& k : ImGui::TextEditorLangs::Z80Asm::kZ80Directives)
			mKeywords.insert(k);
		for (const auto& k : ImGui::TextEditorLangs::Z80Asm::kZ80PreprocessorDirectives) {
			ImGui::LangIdentifier id;
			id.mDeclaration = "Z80 Preprocessor";
			mPreprocIdentifiers.insert(std::make_pair(std::string(k), id));
		}
		for (size_t i = 0; ImGui::TextEditorLangs::Z80Asm::kZ80Registers8[i] != nullptr; i++) {
			const char* reg = ImGui::TextEditorLangs::Z80Asm::kZ80Registers8[i];
			ImGui::LangIdentifier id;
			id.mDeclaration = "Z80 Register";
			mIdentifiers.insert(std::make_pair(std::string(reg), id));
		}
		for (size_t i = 0; ImGui::TextEditorLangs::Z80Asm::kZ80Registers16[i] != nullptr; i++) {
			const char* reg = ImGui::TextEditorLangs::Z80Asm::kZ80Registers16[i];
			ImGui::LangIdentifier id;
			id.mDeclaration = "Z80 Register";
			mIdentifiers.insert(std::make_pair(std::string(reg), id));
		}
		for (size_t i = 0; ImGui::TextEditorLangs::Z80Asm::kZ80ConditionCodes[i] != nullptr; i++)
			mKeywords.insert(ImGui::TextEditorLangs::Z80Asm::kZ80ConditionCodes[i]);
		for (const std::string& mnemonic : ImGui::TextEditorLangs::Z80Asm::sMnemonicSet) {
			ImGui::LangIdentifier id;
			id.mDeclaration = "Z80 Instruction";
			mIdentifiers.insert(std::make_pair(mnemonic, id));
		}
		mCommentStart = "/*";
		mCommentEnd = "*/";
		mSingleLineComment = ";";
		mCaseSensitive = false;
		mName = "Z80 Assembly";
		mHasTimingSupport = true;
		mHasBytecodeSupport = true;
	}

	ImGui::ILanguageParser* Z80AsmLanguage::CreateParser(ImGui::TextEditorMI* host) const {
		return new ImGui::TextEditorLangs::Z80Asm::Z80AsmParser(host, this);
	}

}
