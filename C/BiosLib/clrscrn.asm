	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    CLRSCRN.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;		    Made compatible with screens of any size.		      ;
;		    							      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc	; All assembly language definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	_clearscreen
;
;  DESCRIPTION:		Clear the display. (Microsoft C library emulation)
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX, BX, CX, DX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;	Scroll the display 25 lines up.
;	Move the cursor to the upper left corner.
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 90/11/02 Initial implementation.
;   JFL 93/10/07 Made compatible with screens of any size.
;
;=============================================================================;

CFASTPROC	_clearscreen

		push	ds
		mov	ax, seg BDA
		mov	ds, ax
		ASSUME	ds:BDA

		mov	ax, video_screen_size		; Nbr of bytes/screen
		shr	ax, 1				; Nbr of chars/screen
		mov	dl, byte ptr video_colums	; Number of columns
		div	dl
		mov	dh, al				; Number of lines
		dec	dl				; Index of last column
		dec	dh				; Index of last line
		mov	ax, (F10_SCROLL_UP shl 8) + 0	; Scroll all window
		mov	bh, 07H 			; White on black
		mov	cx, (0 shl 8) + 0		; From row 0 column 0
		int	VIDEO

		mov	ah, F10_SET_CURPOS		; Set cursor position
		mov	bh, video_page			; Page 0
		mov	dx, (0 shl 8) + 0		; Row 0 column 0
		int	VIDEO

		pop	ds
		ASSUME	ds:@data

		ret

ENDCFASTPROC	_clearscreen

END
