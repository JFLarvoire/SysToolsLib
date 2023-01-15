		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    intdos.asm						      ;
;									      ;
;   DESCRIPTION:    Microsoft C library routine _intdos			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    2023-01-15 JFL Created this file.					      ;
;									      ;
;                  (c) Copyright 2023 Jean-Francois Larvoire                  ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _intdos						      ;
;									      ;
;   Description:    Make a call the basic DOS functions 		      ;
;									      ;
;   Parameters:     BX	    Pointer to a struct REGS with input registers     ;
;		    AX	    Pointer to a struct REGS for output registers     ;
;									      ;
;   Returns:	    AX	    MS-DOS error code				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, ES. 					      ;
;									      ;
;   History:								      ;
;    2023-01-15 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CPROC		_intdos

		push	bp
		mov	bp, sp
		push	si
		push	di

		; Load input register
		mov	bx, [bp+ARG1]
		mov	ax, [bx+0]
		mov	cx, [bx+4]
		mov	dx, [bx+6]
		mov	si, [bx+8]
		mov	di, [bx+10]
		mov	bx, [bx+2]

		int	21H

		; Store output registers
		pushf
		push	bx
		mov	bx, [bp+ARG2]
		mov	[bx+0], ax
		pop	[bx+2]; bx
		mov	[bx+4], cx
		mov	[bx+6], dx
		mov	[bx+8], si
		mov	[bx+10], di
		pop	[bx+12]; cflag

		; TODO: Set _doserrno in case of error

		pop	di
		pop	si
		pop	bp
		ret

ENDCPROC	_intdos

END
