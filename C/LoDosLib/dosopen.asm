		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dosopen.asm 					      ;
;									      ;
;   DESCRIPTION:    MS-DOS open file function 3DH			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/10/23 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_open						      ;
;									      ;
;   Description:    MS-DOS open file function 3DH			      ;
;									      ;
;   Parameters:     char *pszName   File name				      ;
;		    WORD wMode	    Open mode. 0=R/O, 1=W/O, 2=R/W; +Share    ;
;		    WORD *pHandle   Where to store the file handle, if any.   ;
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

_dos_open	proc	near public, \
			pszName:PTR BYTE, wMode:WORD, pHandle:PTR WORD

		mov	dx, pszName
		mov	ax, wMode
		mov	ah, 3DH

		int	21h
		jc	@F

		mov	bx, pHandle
		mov	[bx], ax
		xor	ax, ax
@@:
		ret

_dos_open	endp

END
