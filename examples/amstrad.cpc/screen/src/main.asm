;-----------------------------------------------------------------------
;
; Retrodev Examples
;
; screen -- demonstrates loading and displaying a converted bitmap on CPC.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

;-----------------------------------------------------------------------
;
; Tiny test. Just loads a converted bitmap and show it on screen
;
; Disable system code and set palette taken from CPCTelera.
; Sorry folks, not so much time to write examples :)
;
;-----------------------------------------------------------------------


org #2000


firmware_RST_jp equ #38  ;; Memory address were a jump (jp) to the firmware code is stored.
GA_port_byte equ #7f
PAL_INKR equ #40


EntryPoint:
	ld a,0
	call #bc0e

	di                            
	ld   hl, (firmware_RST_jp+1)   
	im    1                       
	ld   hl, #C9FB                 
	ld (firmware_RST_jp), hl       
	ei                             

	ld hl,screentest
	ld de,#c000
	ld bc,#3fff
	ldir

	ld hl,palettetest
	ld de,16
	call set_palette

stuck:
	jp stuck


set_palette:
	dec   e
	add  hl, de

	ld    b, GA_port_byte
svp_setPens_loop:
	out (c), e

	ld    a, (hl)
	and   #1F
	or    PAL_INKR
	out (c), a
						
	dec   hl
	dec   e
	jp p, svp_setPens_loop

	ret
				
palettetest: incbin "../out/screen.pal"

screentest: incbin "../out/screen.bin"

SAVE 'screen.bin',EntryPoint,$-EntryPoint,DSK,'out/screen.dsk'

