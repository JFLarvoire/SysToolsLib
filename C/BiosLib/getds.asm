		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GetDS.asm						      ;
;									      ;
;   DESCRIPTION:    Get the DS register 				      ;
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
;   Function:	    GetDS						      ;
;									      ;
;   Description:    Get the DS register 				      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    AX = DS						      ;
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

GetDS		proc	public

		mov	ax, ds
		ret

GetDS		endp

end
