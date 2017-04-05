		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    annalshl.asm					      ;
;									      ;
;   DESCRIPTION:    C compiler long math library			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/11/06 JFL Created this file.					      ;
;    1997/08/25 JFL Return the result in DX:AX too.			      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C

		.code

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _aNNalshl						      ;
;									      ;
;   Description:    Unsigned indirect long left shift			      ;
;									      ;
;   Parameters:     ARG1    Pointer to the 32 bits data 		      ;
;		    ARG2    Shift count 				      ;
;									      ;
;   Returns:	    DX:AX   Copy of the result				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   CX							      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/30 JFL Created this routine.				      ;
;    1997/08/25 JFL Return the result in DX:AX too.			      ;
;									      ;
;-----------------------------------------------------------------------------;

_aNNalshl	proc	public uses BX, pw:PTR WORD, iCount:WORD

		mov	bx, pw
		mov	cx, iCount
		jcxz	done
encore:
		sal	word ptr [bx], 1
		rcl	word ptr [bx+2], 1
		loop	encore
done:
		mov	ax, WORD ptr [bx]
		mov	dx, WORD ptr [bx+2]
		ret	4

_aNNalshl	endp

END
