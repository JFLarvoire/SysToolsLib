		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    doscomit.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS commit file function 68H			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1998/05/24 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_commit 					      ;
;									      ;
;   Description:    MS-DOS commit file function 68H			      ;
;									      ;
;   Parameters:     WORD hFile	    MS-DOS file handle			      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, CX, DX.					      ;
;									      ;
;   History:								      ;
;									      ;
;    1998/05/24 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

_dos_commit	proc	near public, \
			hFile:WORD

		mov	ah, 68H
		mov	bx, hFile
		int	21h
		jc	@F
		xor	ax, ax
@@:
		ret

_dos_commit	endp

END
