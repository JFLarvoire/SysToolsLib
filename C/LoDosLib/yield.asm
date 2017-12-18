		page	,132
;*****************************************************************************;
;									      ;
;   File Name:	    yield.asm						      ;
;									      ;
;   Description:    MS-DOS idle signalling				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   History:								      ;
;    1997/01/10 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.MODEL	small, C

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    ReleaseTimeSlice					      ;
;									      ;
;   Description:    Release the current VM time slice			      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1997/01/10 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

ReleaseTimeSlice  proc	public

		mov	ax, 1680H
		int	2Fh
		ret

ReleaseTimeSlice  endp

END
