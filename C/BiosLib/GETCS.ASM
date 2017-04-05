		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GetCS.asm						      ;
;									      ;
;   DESCRIPTION:    Get the CS register 				      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/09/26 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.MODEL small, C

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    GetCS						      ;
;									      ;
;   Description:    Get the CS register 				      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    AX = CS						      ;
;									      ;
;   Notes:	    Avoids warnings in C code by using the inline assembler.  ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1994/05/14 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

GetCS		proc	public

		mov	ax, cs
		ret

GetCS		endp

end
