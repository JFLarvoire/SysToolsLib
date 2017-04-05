		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    doswrite.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS write file function 40H			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/04/23 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_write						      ;
;									      ;
;   Description:    MS-DOS write file function 40H			      ;
;									      ;
;   Parameters:     WORD wHandle    File handle 			      ;
;		    void far *lpBuf file buffer 			      ;
;		    WORD wCount     Number of bytes to write		      ;
;		    WORD *pwNumWrit Number of bytes actually written	      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, CX, DX.					      ;
;									      ;
;   History:								      ;
;									      ;
;    1996/04/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

extern		_dos_read_write:near

_dos_write	proc	near public, \
			wHandle:WORD, lpBuf:FAR PTR, wCount:WORD, \
			pwNumWrit:PTR WORD

		mov	ah, 40H
		jmp	_dos_read_write

_dos_write	endp

END
