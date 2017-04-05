		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    annalmul.asm					      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1997/08/25 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

		.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNNalmul						      ;
;									      ;
;   Description:    Signed long *=					      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    WORD    Pointer to the Multiplier and result	      ;
;		    DWORD   Multiplicand				      ;
;									      ;
;   Returns:	    DX:AX   Copy of the result				      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   EAX, EDX, CX					      ;
;									      ;
;   History:								      ;
;									      ;
;    1997/08/25 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNNalmul	proc	public

		pop	cx		; Return address

		pop	ax		; Pointer to the Multiplier and result
		pop	edx		; Multiplicand
		push	bx		; Must be preserved
		mov	bx, ax		; Pointer to the Multiplier and result
		mov	eax, DWORD ptr [bx]
		imul	edx
		mov	DWORD ptr [bx], eax
		pop	bx		; Restore the initial value

		shld	edx, eax, 16	; mov dx:ax, eax
		jmp	cx

_aNNalmul	endp

END
