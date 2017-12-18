		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    swapw.asm						      ;
;									      ;
;   DESCRIPTION:    Swap the two words of a dword.			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/25 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    swapw						      ;
;									      ;
;   Description:    Swap words						      ;
;									      ;
;   Parameters:     DX:AX   Dword, which words need to be swapped.	      ;
;									      ;
;   Returns:	    DX:AX   with words swapped. 			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   DX, AX.						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/25 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	swapw

		xchg	ax, dx
		ret

ENDCFASTPROC	swapw

END
