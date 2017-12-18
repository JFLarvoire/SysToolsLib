	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GOTOXY.ASM						      ;
;									      ;
;   DESCRIPTION:    Routine to move the cursor anywhere on screen	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993-10-07 JFL Separated from alibc.c.				      ;
;									      ;
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
;  ENTRY POINT NAME:	gotoXY
;
;  DESCRIPTION: 	Move the cursor to given coordinates
;
;  INPUT PARAMETERS
;	AX =	Column number (0 to 79)
;	DX =	Row number (0 to 24)
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX, BX, DX
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
;   JFL 93/10/06 Use the BIOS page variable.
;
;=============================================================================;

CFASTPROC	gotoXY			; Move cursor to column ax, row DX.

		push	bp
		mov	bp, sp

		push	ds
		mov	bx, seg BDA
		mov	ds, bx
		ASSUME	ds:BDA

		mov	dh, dl			; Row
		mov	dl, al			; Column
		mov	bh, video_page		; Active video page
		mov	ah, F10_SET_CURPOS
		int	VIDEO

		pop	ds
		ASSUME	ds:@data

		pop	bp
		ret

ENDCFASTPROC	gotoXY

END
