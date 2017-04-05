	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    PUTCHCOL.ASM					      ;
;									      ;
;   DESCRIPTION:    Routine to display a string with a given color attribute  ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
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
;  ENTRY POINT NAME:	putch_color
;
;  DESCRIPTION: 	Output a character with a given attribute
;
;  INPUT PARAMETERS
;	ARG1	Attribute
;	ARG2	ASCII code in low byte
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX, BX, CX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;	Use video BIOS calls "write character attribute" for printable
;	characters, and "write character teletype" for control characters.
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 90/11/02 Initial implementation.
;   JFL 93/10/06 Use the BIOS page variable.
;
;=============================================================================;

CPROC		putch_color	       ; Use color ARG1 to display char. ARG2

		push	bp
		mov	bp, sp

		push	ds
		mov	ax, seg BDA
		mov	ds, ax
		ASSUME	ds:BDA

		; Characters 07, 08, 09, 0A and 0D are ascii commands,
		; and have no color.
		mov	al, [bp+ARG2]		; Character
		cmp	al, 0DH
		je	write_chartel
		cmp	al, 07H
		jb	setch_color
		cmp	al, 0AH
		jbe	write_chartel
setch_color:
		mov	ah, F10_WR_CHARATR
		mov	al, " "                 ; Space
		mov	bh, video_page		; Active video page
		mov	bl, [bp+ARG1]		; Color
		mov	cx, 1			; 1 character
		int	VIDEO
write_chartel:
		mov	ah, F10_WR_CHARTEL
		mov	al, [bp+ARG2]		; Character
		mov	bh, video_page		; Active video page
		mov	bl, [bp+ARG1]		; Color
		int	VIDEO

		pop	ds
		ASSUME	ds:@data

		pop	bp
		ret

ENDCPROC	putch_color

END
