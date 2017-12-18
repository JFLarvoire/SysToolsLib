	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.

;*****************************************************************************;
;									      ;
;   FILE NAME:	    ACTIVEPG.ASM					      ;
;									      ;
;   DESCRIPTION:    Routines to manage the display active page		      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
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
;  ENTRY POINT NAME:	get_active_page
;
;  DESCRIPTION: 	Get the active display page before changing it to run the 
;                       Setup in another page
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AX = video page	number
;
;  REGISTERS DESTROYED: AX, BX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;    Int 10H, function 0Fh
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   MM  05/12  Initial implementation.
;
;=============================================================================;

CFASTPROC	get_active_page

		mov	ah, F10_GET_MODE
        	int	VIDEO
                mov	al, bh 		;present active page returned to setup.c in al
		xor     ah,ah

		ret

ENDCFASTPROC	get_active_page

;=============================================================================;
;
;  ENTRY POINT NAME:	set_active_page
;
;  DESCRIPTION: 	Set the new active display page (determined in setup.c)
;
;  INPUT PARAMETERS
;	AX = video page number
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX, BX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;    Int 10H, function 05h
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   MM  05/12  Initial implementation.
;
;=============================================================================;

CFASTPROC	set_active_page

		mov	ah, F10_SET_PAGE
		int	VIDEO

		ret

ENDCFASTPROC	set_active_page

END
