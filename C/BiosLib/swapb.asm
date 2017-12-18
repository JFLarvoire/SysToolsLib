		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    swapb.asm						      ;
;									      ;
;   DESCRIPTION:    Swap the two bytes of a word			      ;
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

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    reverse / swapb					      ;
;									      ;
;   Description:    Swap bytes						      ;
;									      ;
;   Parameters:     AX	    Word, which bytes need to be swapped.	      ;
;									      ;
;   Returns:	    AX	    with bytes swapped. 			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1990/12/14 JFL Created routine reverse				      ;
;    1995/08/25 JFL Created routine swapb				      ;
;    1995/08/28 JFL Merged the two, and moved them to a separate file.	      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	reverse
CFASTPROC	swapb

		xchg	al, ah
		ret

ENDCFASTPROC	swapb
ENDCFASTPROC	reverse

END
