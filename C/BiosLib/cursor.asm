	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    CURSOR.ASM						      ;
;									      ;
;   DESCRIPTION:    Routines to manage the cursor size	 		      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;									      ;
;      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      ;
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
;  ENTRY POINT NAME:	cursor_off
;
;  DESCRIPTION: 	Turn the cursor off using BIOS int 10h
;			function 01h.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AH, CX
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
;   CBI 01/29/91 Initial implementation.
;
;=============================================================================;

CFASTPROC	cursor_off

		mov	ah, F10_SET_CURSIZE
		mov	cx, 100h		; Set the cursor size to NULL
		int	VIDEO

		ret

ENDCFASTPROC	cursor_off

;=============================================================================;
;
;  ENTRY POINT NAME:	cursor_on
;
;  DESCRIPTION: 	Turn the cursor on using BIOS int 10h
;			function 01h.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AH, CX
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
;   CBI 01/29/91 Initial implementation.
;
;=============================================================================;

CFASTPROC	cursor_on

		mov	ah, F10_SET_CURSIZE	; Set the cursor size
		mov	cx, 607h		; Set the cursor size
		int	VIDEO

		ret

ENDCFASTPROC	cursor_on

END
