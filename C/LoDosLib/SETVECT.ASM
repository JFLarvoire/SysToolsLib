		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    setvect.asm 					      ;
;									      ;
;   DESCRIPTION:    Microsoft C library routine _dos_setvect		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/23 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_setvect					      ;
;									      ;
;   Description:    Get an interrupt vector				      ;
;									      ;
;   Parameters:     AX			Interrupt number		      ;
;		    DWORD on stack	Handler 			      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:	    Unfortunately, the DWORD argument is passed on the stack  ;
;		    by the _fastcall calling convention, and not in BX:DX as  ;
;		    I initially expected.				      ;
;									      ;
;   Regs altered:   AX, DX						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_setvect

		push	bp
		mov	bp, sp

		push	ds
		lds	dx, DWORD PTR [bp+4]
		mov	ah, 25H
		int	21H
		pop	ds

		pop	bp
		ret	4

ENDCFASTPROC	_dos_setvect

END
