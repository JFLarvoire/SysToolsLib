		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    htobcd.asm						      ;
;									      ;
;   DESCRIPTION:    Convert an hexadecimal number to BCD		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/25 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the macro definitions

		.286

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    htobcd						      ;
;									      ;
;   Description:    Convert an hexadecimal value to its BCD equivalent	      ;
;									      ;
;   Parameters:     AL	    Hexadecimal number from 0 to 99		      ;
;									      ;
;   Returns:	    AL	    The same, in BCD.				      ;
;									      ;
;   Notes:	    One of the rare uses of the AAM instructions!	      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1991/06/06 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	htobcd

		aam				; Convert to unpacked BCD

		shl	al, 4			; Pack BCD into AL
		shr	ax, 4

		ret

ENDCFASTPROC	htobcd

END
