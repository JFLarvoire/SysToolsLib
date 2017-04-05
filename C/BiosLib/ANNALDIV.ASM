		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    annaldiv.asm					      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/06/26 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

		.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNNaldiv						      ;
;									      ;
;   Description:    Signed indirect long division			      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    WORD    Pointer to the 32 bits dividend		      ;
;		    DWORD   Divisor					      ;
;									      ;
;   Returns:	    DX:AX   Copy of the result				      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   EAX, EDX, CX					      ;
;									      ;
;   History:								      ;
;									      ;
;    1996/06/26 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNNaldiv	proc	public

		push	bp
		mov	bp, sp

		push	bx		; Must be preserved

		mov	bx, [bp+4]	; Pointer to the Multiplier and result
		mov	eax, DWORD ptr [bx]
		xor	edx, edx
		idiv	dword ptr [bp+6]
		mov	DWORD ptr [bx], eax

		shld	edx, eax, 16	; mov dx:ax, eax

		pop	bx		; Restore the initial value
		pop	bp
		ret	6

_aNNaldiv	endp

END
