		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dosgetft.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS get file time function 5700H 		      ;
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
;   Function:	    _dos_getftime					      ;
;									      ;
;   Description:    MS-DOS get file time function 5700H 		      ;
;									      ;
;   Parameters:     WORD hFile	    MS-DOS file handle			      ;
;		    WORD *pwDate    Where to store the date		      ;
;		    WORD *pwTime    Where to store the time		      ;
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

_dos_getftime	proc	near public, \
			hFile:WORD, pwDate:PTR WORD, pwTime:PTR WORD

		mov	ax, 5700H
		mov	bx, hFile
		int	21H
		jc	@F
		mov	bx, pwDate
		mov	WORD ptr [bx], dx   ; Modification date
		mov	bx, pwTime
		mov	WORD ptr [bx], cx   ; Modification time
		xor	ax, ax
@@:
		; Return in AX the MS-DOS error. 0 = Success.
		ret

_dos_getftime	endp

END
