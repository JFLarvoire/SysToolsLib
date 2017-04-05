		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    annaulrm.asm					      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    2001/03/27 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

		.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNNaulrem						      ;
;									      ;
;   Description:    Unsigned long %=					      ;
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
;    2001/03/27 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNNaulrem	proc	public

		push	bp
		mov	bp, sp

		push	bx		; Must be preserved

		mov	bx, [bp+4]	; Pointer to the dividend and result
		mov	eax, DWORD ptr [bx]
		xor	edx, edx
		div	dword ptr [bp+6]
		mov	eax, edx	; Remainder
		mov	DWORD ptr [bx], eax

		shld	edx, eax, 16	; mov dx:ax, eax

		pop	bx		; Restore the initial value
		pop	bp
		ret	6

_aNNaulrem	endp

END
