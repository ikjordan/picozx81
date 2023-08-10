format zx81
;labelusenumeric
;LISTOFF

	// hardware options to be set and change defaults in ZX81DEF.INC
	MEMAVL = MEM_16K		// can be MEM_1K, MEM_2K, MEM_4K, MEM_8K, MEM_16K, MEM_32K, MEM_48K
					// default value is MEM_16K
	STARTMODE  EQU	SLOW_MODE	// SLOW or FAST
	DFILETYPE  EQU	AUTO		// COLLAPSED or EXPANDED or AUTO

	include 'SINCL-ZX\ZX81.INC'	// definitions of constants
;LISTON

30	REM _asm


begin_5:

	ld ix,traceproc_5
	out (0xfe), a			; Enable NMI generation, Reset Vsync

loop_around_5:

	ld b, 4
blub:
	ld hl, lflag_5
	ld (hl),0			; Set to zero to block until signalled
tag_5:
	bit 0, (hl)
	jr z, tag_5			; holds until signalled by askd

	push bc
	ld a,0
	ld (buffer40), a		; ld (0x4000), a
	ld de, buffer40 + 1		; ld de, #0x4001
	ld hl,buffer40			; ld hl, #0x4000
	ld bc, 24
	ldir				; Move the contents of buffer down 1 in memory
	pop bc				
	djnz blub			; Back to hold	- will loop 4 times

	ld a, (scrolloff)
	or a
	jr z, loop_around_5		; Loop again until scrolloff counted down from 255 to 1
	dec a
	ld (scrolloff), a		; Put 255 back in the scroll counter

	jr loop_around_5		; loop once again, this loop lasts for ever

lflag_5:
	db 0

drawproc2:				; alternative draw that delays for 1 line
	ld b, 16
	jr hr_5 			; delays by extra 15*13 + 13 = 208

drawproc_5:				; Main draw procedure
	ld b,1				; no delay

hr_5:
	djnz hr_5			; delay for 0 or 1 line
;       nop
	ld a, (scrolloff)		; scroll offset counted down in loop_around_5
	cp 190				
	jr c, yiek			; : 12 if met 7 if not 
	ld a, 190			; if a > 190 set a to 190 : 7
	jp ziep 			; : 10
yiek:
	nop				; Got here 10 + 7 - (12-7) quicker, so 3*4 to balance 
	nop
	nop

ziep:	     
	or a	
	jr z, dok_5			; scroll offset is 0 :	5 more if met
	ld c, a 			; if not zero, use as counter : 4

nline:	      
	ld b, 14			; 13 * 13 + 7 + 8 = 184

wline:
	djnz wline
	dec c				; repeat for scroll times : 4
	ld r,a				; : 9
	jp nz, nline			; 10 = 184 + 4 + 10 + 9 = 207 :-)
	jr strt_5			; 12
dok_5:	      
	ld a,0				; 7 + 4 + 5 above = 16 matches 16 if not zero
	nop

strt_5: exx
	ld hl, bmpdata
	ld de, buffer40 		; 0x4000
	exx
	srl a				; scroll offset /2
	neg				; invert bits bit 7 set
	ld b, 96
	add a,b 			; Will carry
	ld b,a
	ld ix, endofline_5
	ld a, buffer40_highbyte 	; 0x40
	ld i, a 			; point to start of data
	xor a				; clear a

nextline_5:
	exx					; hl=bmpdata and de=start of buffer
	jp oneline_5+0x8000			; 10 + execute the line
oneline_5:	  
	ld r,a					; 9
	db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	; 64  16x4
	db 0,0,0,0,0,0,0,0			; 32   8x4
	jp (ix) 				; 8 : ix = endofline_5
endofline_5:					; = 111
	ld b,(hl)				; no of pairs on line : 7
	ld c,6					; for counter : 7
	inc hl					; 6
nextpos:
	ld e,(hl)				; set offset into buffer : 7
	inc hl					; 6
	ldi					; copy byte from bmp into buffer, with offset: 16
	djnz nextpos				; Repeat for the number of bytes defined at start of line :  13

	ld b,c					; set b to 6 : 7
zorg:
	ld a,0
	ld r,a
	ld r,a
	nop
	djnz zorg				; loop 6 times : 13

	exx					; get back value of b set in strt_5 
	nop
	djnz nextline_5 			; loop on value of b : 13
	ld b, 14

waiter_2:
	djnz waiter_2				; : 13 * 13 + 8 

	inc hl
	ld b, 8
	ld a, 0x1e				; back to ROM characters
	ld i,a
	ld ix, endoftext

textlines:	  
	jp textline+0x8000			; 10
textline:	 
	db 0x28,0x26,0x31,0x34,0x29,0x34,0x3d,0x00     ; "CALODOX "
	db 0x16,0x00,0x3c,0x2a,0x00,0x34,0x3c,0x33     ; "- WE OWN"
	db 0x00,0x3f,0x3d,0x24,0x1d,0,0,0	       ; " ZX81   "
	jp (ix) 				; 8
endoftext:	  
	ld c,4

askd:
	dec c
	jr nz, askd				; short delay
	inc hl					; delay : 6
	nop
	nop
	djnz textlines				; 8 lines
	ld hl, lflag_5
	ld (hl), 1				; Allow main loop to run
	ld ix, traceproc_5			; Next time set vsync
	jr doret_5

traceproc_5:
	ld a, (scrolloff)			; : 13
	or a					; : 4
	jr nz, vup				; : 13 or 8  = 30 or 25

	ld b, 188				; Burn 11 or 12 lines
	ld a, (interlace)			; : 13
	or a					; : 4
	jr nz, a_delay				; : 13 or 8
	ld b, 172				; : 7

a_delay:
	djnz a_delay				; count down from 188 -> 7 + 13 + 4 + 13 + 187*13 + 8 = 2476 = (12 * 207) - 8
						; count down from 172 -> 7 + 13 + 4 + 8 + 7 + 169*13 + 8  = 2270 = (11 * 207) - 7
	add a, (hl)				; End up 1 tstate out for 12 line case

vup:
	in a, (0xfe)				; If NMI disabled set VSync
	ld b, 81

here_5:
	djnz here_5				; count down from 81 -> 80*13 + 8 = 1048 = 5 * 207 + 13
						; delay 5 lines to keep vsync high and moves vsync lower into hsync

	ld a, (interlace)			; toggle the interlace flag
	xor 1
	ld (interlace), a			; set it back

	jp z, altv				; Set routine, drawproc_2 starts a line lower
	ld ix, drawproc_5
	jr fini
altv:
	ld ix, drawproc2
	ld a, (interlace)			; balance the extra jr

fini:

	out (0xff),a				; Reset VSync

doret_5:
	ld a, 45
	jp 0x029e

interlace:	  
	db 0					; Toggled to delay start by one line

scrolloff:	  
	db 255

bmpdata:
;-------------------------------------------------
; Big ThreeHeads by fred
;-------------------------------------------------

	; ThreeHeads
	db 3,12,15,13,255,14,128
	db 2,12,127,14,254
	db 4,11,1,12,255,14,255,15,224
	db 2,11,15,15,254
	db 3,11,255,15,255,16,192
	db 2,10,15,16,248
	db 2,10,127,16,255
	db 3,9,3,10,255,17,192
	db 2,9,15,17,224
	db 2,9,63,17,248
	db 2,9,255,17,252
	db 1,17,254
	db 1,17,255
	db 2,8,1,18,128
	db 1,18,192
	db 1,18,192
	db 1,18,224
	db 2,8,0,18,240
	db 1,8,0
	db 2,9,127,18,248
	db 1,9,63
	db 1,9,63
	db 5,3,1,4,255,5,248,9,31,18,252
	db 5,3,31,5,255,6,128,9,15,18,248
	db 3,3,255,6,240,9,3
	db 3,2,3,6,252,9,1
	db 4,2,15,6,254,9,0,18,240
	db 3,2,31,6,255,10,127
	db 3,2,63,10,31,18,224
	db 4,2,255,7,128,10,15,18,192
	db 2,10,7,18,128
	db 3,1,1,10,3,18,0
	db 4,1,3,7,0,10,255,17,252
	db 3,1,7,9,1,17,248
	db 1,17,224
	db 4,1,15,6,254,9,0,17,128
	db 5,1,31,6,252,16,252,17,0,21,240
	db 4,1,63,10,127,20,127,21,254
	db 4,19,7,20,255,21,255,22,192
	db 5,1,127,6,248,16,254,19,63,22,240
	db 5,6,240,10,63,16,255,19,255,22,248
	db 4,1,255,10,31,18,3,22,254
	db 2,0,1,18,15
	db 5,0,3,6,224,10,15,16,254,18,63
	db 5,0,7,10,7,16,252,18,127,22,255
	db 5,0,15,10,3,16,248,17,1,18,255
	db 4,0,31,10,1,16,240,23,192
	db 5,0,15,6,240,10,0,11,127,16,192
	db 5,0,1,11,31,16,0,17,3,23,224
	db 5,0,0,1,63,6,248,11,7,15,252
	db 5,1,3,6,252,11,15,15,240,17,1
	db 5,1,0,2,31,6,248,11,127,15,192
	db 5,2,7,6,240,10,1,11,255,15,0
	db 5,2,1,6,252,10,7,14,252,23,240
	db 5,2,0,6,255,10,31,14,240,17,0
	db 5,3,63,7,128,10,127,14,192,23,248
	db 5,3,31,7,192,10,255,14,0,23,240
	db 5,7,224,9,3,14,192,18,127,23,224
	db 5,3,15,7,240,9,7,14,224,23,192
	db 3,9,15,14,240,23,0
	db 3,7,248,14,248,22,252
	db 5,3,31,9,31,14,252,18,63,22,240
	db 4,7,252,9,63,14,254,22,128
	db 5,3,63,9,127,18,127,21,254,22,0
	db 5,3,127,7,254,17,1,18,255,21,240
	db 4,3,255,9,255,17,15,21,224
	db 3,2,1,17,31,21,192
	db 4,2,3,8,1,17,255,21,0
	db 4,2,7,7,255,16,3,20,240
	db 3,2,31,16,7,20,128
	db 5,2,127,8,131,16,15,19,254,20,0
	db 4,1,1,2,255,16,63,19,252
	db 3,1,3,16,127,19,248
	db 5,1,15,8,195,15,1,16,255,19,240
	db 3,1,31,8,227,15,3
	db 5,1,63,8,231,14,255,15,7,19,248
	db 3,1,127,8,255,15,15
	db 2,15,143,19,252
	db 3,1,255,15,159,19,254
	db 2,0,1,15,255
	db 1,19,255
	db 1,20,128
	db 1,20,192
	db 1,20,224
	db 1,20,224
	db 1,20,240
	db 1,20,248
	db 1,20,248
	db 3,0,0,1,127,20,252
	db 1,0,0
	db 1,0,0
	db 1,0,0
	db 1,0,0
	db 1,20,254
	db 1,20,254

	db 1,20,254

	END _asm

	RAND USR #begin_5
;LISTOFF

	include 'SINCL-ZX\ZX81DISP.INC' 	 ; include D_FILE and needed memory areas
;LISTON
AUTORUN:
	RAND USR #begin_5

VARS_ADDR:
	db 80h

WORKSPACE:

BUFFER_SIZE	=0x40
buffer		=0x4000

buffer40	= buffer + 0x0000
buffer40_highbyte	= 0x40 + 0


assert ($-MEMST)<MEMAVL
// end of program
