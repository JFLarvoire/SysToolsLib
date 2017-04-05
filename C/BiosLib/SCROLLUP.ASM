	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.

;*****************************************************************************;
;									      ;
;   File name	    SCROLLUP.ASM					      ;
;									      ;
;   Description     Routine to scroll the display up			      ;
;									      ;
;   Notes	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   History								      ;
;    1993/10/07	JFL Separated from alibc.c                                    ;
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
;  ENTRY POINT NAME:	scrollup
;
;  DESCRIPTION: 	Scroll a horizontal band of the display up
;
;  INPUT PARAMETERS
;	ARG1 = Top line to scroll (0 to 24)
;	ARG2 = Bottom line to scroll (0 to 24)
;	ARG3 = Number of lines to scroll ( < ARG2-ARG1 )
;	ARG4 = Attribute
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: None
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
;   JFL 11/16/90 Initial implementation.
;
;=============================================================================;

CPROC		scrollup

		push	bp
		mov	bp, sp

		mov	ah, F10_SCROLL_UP
		mov	al, byte ptr [bp+ARG3]	    ; Lines to scroll
		mov	bh, byte ptr [bp+ARG4]	    ; Attribute in blank lines
		mov	ch, byte ptr [bp+ARG1]	    ; Upper left row
		mov	cl, 0			    ; Upper left column
		mov	dh, byte ptr [bp+ARG2]	    ; Lower right row
		mov	dl, 79D 		    ; Lower right column

		int	VIDEO

		pop	bp
		ret

ENDCPROC	scrollup

END
