		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    anlmul.asm						      ;
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
;   Function:	    _aNlmul						      ;
;									      ;
;   Description:    Signed long multiply				      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    DWORD 1 Multiplier					      ;
;		    DWORD 2 Multiplicand				      ;
;									      ;
;   Returns:	    DX:AX   The result					      ;
;									      ;
;   Notes:	    Uses 386 instructions. May not be used on old machines.   ;
;									      ;
;   Regs altered:   EAX, EDX, CX					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/30 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNlmul 	proc	public

		pop	cx

		pop	eax
		pop	edx
		imul	edx

		shld	edx, eax, 16	; mov dx:ax, eax
		jmp	cx

_aNlmul 	endp

END
