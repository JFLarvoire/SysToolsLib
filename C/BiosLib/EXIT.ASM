	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    EXIT.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1994/05/09 JFL Initial implementation				      ;
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
;   Global data 							      ;
;									      ;
;-----------------------------------------------------------------------------;

.data

extrn		_psp:word	; Initialized in startup module

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	exit
;
;  DESCRIPTION: 	Exit immediately
;
;			Warning: Absolutely no cleanup done.
;
;  INPUT PARAMETERS
;	AX	Value to return in AX
;
;  OUTPUT PARAMETERS
;	N/A
;
;  REGISTERS DESTROYED: N/A
;
;  REGISTERS PRESERVED: N/A
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 94/05/09 Initial implementation.
;
;=============================================================================;

CFASTPROC	exit

		push	_psp
		push	0
		retf

ENDCFASTPROC	exit

END
