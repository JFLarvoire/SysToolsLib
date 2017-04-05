		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    annaulsr.asm					      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/01/03 JFL Created this file.					      ;
;    1997/08/25 JFL Return the result in DX:AX too.			      ;
;									      ;
;      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

		.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNNaulshr						      ;
;									      ;
;   Description:    Unsigned long >>=					      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    WORD    Pointer to the DWORD data and result	      ;
;		    WORD    Shift count 				      ;
;									      ;
;   Returns:	    DX:AX   Copy of the result				      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   AX, CX, DX						      ;
;									      ;
;   History:								      ;
;									      ;
;    1996/01/03 JFL Created this routine.				      ;
;    1997/08/25 JFL Return the result in DX:AX too.			      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNNaulshr	proc	public

		pop	dx		; Return address

		pop	ax		; Pointer to the DWORD data and result
		pop	cx		; Shift count

		push	bx		; Must be preserved
		mov	bx, ax

		shr	DWORD ptr [bx], cl

		pop	cx		; Saved BX
		push	dx		; Return address
		mov	ax, WORD ptr [bx]
		mov	dx, WORD ptr [bx+2]
		mov	bx, cx		; Restore BX

		ret

_aNNaulshr	endp

END
