		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    bcdtoh.asm						      ;
;									      ;
;   DESCRIPTION:    Convert a BCD number to hexadecimal 		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/25 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the macro definitions

		.286

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    bcdtoh						      ;
;									      ;
;   Description:    Convert a BCD number to its hexadecimal equivalent	      ;
;									      ;
;   Parameters:     AL	    Packed BCD number				      ;
;									      ;
;   Returns:	    AL	    The same, in hexadecimal			      ;
;									      ;
;   Notes:	    One of the rare uses of the AAD instructions!	      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1991/06/06 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	bcdtoh

		sub	ah, ah			; Unpack BCD from AL to AX
		shl	ax, 4
		shr	al, 4

		aad				; Convert to hexa

		ret

ENDCFASTPROC	bcdtoh

END
