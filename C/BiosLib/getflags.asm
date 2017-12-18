		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GetFlags.asm					      ;
;									      ;
;   DESCRIPTION:    Get the flags register				      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/09/26 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.MODEL small, C

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    GetFlags						      ;
;									      ;
;   Description:    Get the flags register				      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    AX = flags						      ;
;									      ;
;   Notes:	    Avoids warnings in C code by using the inline assembler.  ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/05/18 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

GetFlags	proc	public

		pushf
		pop ax
		ret

GetFlags	endp

end
