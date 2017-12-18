	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    PUTCHAR.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;    1994/05/10 JFL Manage tabulations.					      ;
;		    Always display on the current page.			      ;
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

TAB		equ	09H	; Tabulation ASCII code

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	putchar
;
;  DESCRIPTION:		Put a character to stdout. C standard library.
;			Modified to put it to the console.
;
;  INPUT PARAMETERS
;	AX	Ascii code of the character to output
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
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/02/90 Initial implementation.
;   JFL 94/05/10 Manage tabulations.
;		 Always display on the current page.
;
;=============================================================================;

CFASTPROC	putchar

		push	ds
		push	seg BDA
		pop	ds
		ASSUME	ds:BDA

		mov	bh, video_page		; Active video page

		pop	ds
		ASSUME	ds:@data

		cmp	al, TAB
		je	@F

		mov	ah, F10_WR_CHARTEL
bl_7_int_video:
		mov	bl, 07H			; White on black
		int	VIDEO

		ret
@@:
		mov	ah, F10_GET_CURPOS
		int	VIDEO			; Get cursor infos into cx/dx

		and	dx, 7			; dx %= 8
		mov	cx, 8
		sub	cx, dx			; Number of spaces to write
		mov	ax, (F10_WR_CHARATR SHL 8) + ' '
		jmp	bl_7_int_video

ENDCFASTPROC	putchar

END
