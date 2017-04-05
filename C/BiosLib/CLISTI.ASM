	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    CLISTI.ASM	 					      ;
;									      ;
;   DESCRIPTION:    Interrupt flag manipulation routines		      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1994/05/09 JFL Extracted this file from AUTIL.ASM.			      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
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
;  ENTRY POINT NAME:	interrupts_off
;
;  DESCRIPTION: 	Turn off interrupts and return the initial flags state
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AX  =  Initial flags state
;
;  REGISTERS DESTROYED: AX
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
;     JFL 03/29/91  Initial implementation.
;
;=============================================================================;

CFASTPROC	interrupts_off

		pushf
		cli
		pop	ax
		ret


ENDCFASTPROC	interrupts_off

;=============================================================================;
;
;  ENTRY POINT NAME:	interrupts_back
;
;  DESCRIPTION: 	Restore interrupts from the saved flags
;
;  INPUT PARAMETERS
;	AX  =  Initial flags state
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: None
;
;  REGISTERS PRESERVED: All
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/29/91  Initial implementation.
;
;=============================================================================;

CFASTPROC	interrupts_back

		push	ax
		popf
		ret


ENDCFASTPROC	interrupts_back

END
