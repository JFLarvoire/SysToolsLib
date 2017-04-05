		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    anlrem.asm						      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/30 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

		.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNlrem						      ;
;									      ;
;   Description:    Signed long remainder				      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    DWORD 1 Dividand					      ;
;		    DWORD 2 Divisor					      ;
;									      ;
;   Returns:	    DX:AX   The result					      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   EAX, EDX						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/30 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNlrem 	proc	public

		push	bp
		mov	bp, sp

		mov	eax, dword ptr [bp+4]
		cdq
		idiv	dword ptr [bp+8]
		imul	dword ptr [bp+8]
		neg	eax
		add	eax, dword ptr [bp]+4
		; Don't compute edx since we know it would be 0

		shld	edx, eax, 16	; mov dx:ax, eax
		pop	bp
		ret	8

_aNlrem 	endp

END
