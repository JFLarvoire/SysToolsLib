		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dosread.asm 					      ;
;									      ;
;   DESCRIPTION:    MS-DOS read file function 3FH			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/10/23 JFL Created this file.					      ;
;    1996/04/23 JFL Added label _dos_read_write for function _dos_write.      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_read						      ;
;									      ;
;   Description:    MS-DOS read file function 3FH			      ;
;									      ;
;   Parameters:     WORD wHandle    File handle 			      ;
;		    void far *lpBuf file buffer 			      ;
;		    WORD wCount     Number of bytes to read		      ;
;		    WORD *pwNumRead Number of bytes actually read	      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, CX, DX.					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/10/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

_dos_read	proc	near public, \
			wHandle:WORD, lpBuf:FAR PTR, wCount:WORD, \
			pwNumRead:PTR WORD

		mov	ah, 3FH

_dos_read_write::	; _dos_write() identical except for ah=40h
		public	_dos_read_write

		push	ds
		mov	bx, wHandle
		mov	cx, wCount
		lds	dx, lpBuf	       ;; ds:dx -> buffer
		int	21h
		pop	ds
		jc	@F

		mov	bx, pwNumRead
		mov	[bx], ax
		xor	ax, ax
@@:
		ret

_dos_read	endp

END
