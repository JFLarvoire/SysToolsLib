	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    MATH.ASM						      ;
;									      ;
;   DESCRIPTION:    Simple math routines better done in ASM than in C.	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1990/11/02 JFL Initial implementation of alibc.asm.		      ;
;    1991/06/03 JFL Added new routines mul16x16to32 and div32x16to16.	      ;
;    1991/06/11 JFL Added error prevention tests in regle_de_trois and	      ;
;		     div32x16to16. Merged these two routines.		      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;    1995/08/28 JFL Moved routines reverse & h2bcd to their own files.	      ;
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
;  ENTRY POINT NAME:	regle_de_trois
;
;  DESCRIPTION: 	Compute A * B / C with intermediate result on 32 bits
;
;  INPUT PARAMETERS
;	AX = A
;	DX = B
;	BX = C
;
;  OUTPUT PARAMETERS
;	AX = A * B / C
;
;  REGISTERS DESTROYED: AX, DX
;
;  REGISTERS PRESERVED: All others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES		Must use the fastcall convention
;
;			This functions is designed for use in C modules to
;			compute expressions that need to have 32 bits
;			intermediate results, without loading the C library
;			32 bits arithmetic routines.
;
;  MODIFICATION HISTORY NOTES
;   JFL 01/09/91 Initial implementation.
;   JFL 06/12/91 Changed mul/div instruction from signed to unsigned.
;		 Added overflow checking. Return 65535 if divide error.
;
;=============================================================================;

; Merged with the next function

;=============================================================================;
;
;  ENTRY POINT NAME:	div32x16to16
;
;  DESCRIPTION: 	Divide a long integer by a short, return the short quot.
;
;  INPUT PARAMETERS
;	DX:AX	Dividend
;	BX	Divisor
;
;  OUTPUT PARAMETERS
;	AX	Quotient
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
;   JFL 06/03/91 Initial implementation.
;   JFL 06/12/91 Changed div instruction from signed to unsigned.
;		 Added overflow checking. Return 65535 if divide error.
;
;=============================================================================;

CFASTPROC	regle_de_trois

		mul	dx

CFASTPROC	div32x16to16

		cmp	dx, bx		; Will quotient fit in AX? Divide by 0?
		jae	rdt_overflow	; If error, don't divide.

		div	bx
		ret
rdt_overflow:
		mov	ax, -1
		ret

ENDCFASTPROC	div32x16to16

ENDCFASTPROC	regle_de_trois

;=============================================================================;
;
;  ENTRY POINT NAME:	mul16x16to32
;
;  DESCRIPTION: 	Multiply two short integers, return the long product.
;
;  INPUT PARAMETERS
;	AX	First multiplicand
;	DX	Second multiplicand
;
;  OUTPUT PARAMETERS
;	DX:AX	Product
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
;   JFL 06/03/91 Initial implementation.
;   JFL 06/12/91 Changed mul instruction from signed to unsigned.
;
;=============================================================================;

CFASTPROC	mul16x16to32

		mul	dx
		ret

ENDCFASTPROC	mul16x16to32

END
