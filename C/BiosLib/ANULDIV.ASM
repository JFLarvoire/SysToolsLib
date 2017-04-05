		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    anuldiv.asm 					      ;
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
;   Function:	    _aNuldiv						      ;
;									      ;
;   Description:    Unsigned long divide				      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    DWORD 1 Dividand					      ;
;		    DWORD 2 Divisor					      ;
;									      ;
;   Returns:	    DX:AX   The result					      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   EAX, ECX, EDX					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/30 JFL Created this routine.				      ;
;    1995/08/31 JFL Rewritten to preserve BX. Trashing CX is OK.	      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNuldiv	proc	public

		pop	dx

		pop	eax
		pop	ecx

		push	dx

		xor	edx, edx
		div	ecx

		shld	edx, eax, 16	; mov dx:ax, eax
		ret

_aNuldiv	endp

END
