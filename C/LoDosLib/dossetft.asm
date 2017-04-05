		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dossetft.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS gst file time function 5701H 		      ;
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
;   Function:	    _dos_setftime					      ;
;									      ;
;   Description:    MS-DOS get file time function 5701H 		      ;
;									      ;
;   Parameters:     WORD hFile	    MS-DOS file handle			      ;
;		    WORD wDate	    The new date			      ;
;		    WORD wTime	    The new time			      ;
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

_dos_setftime	proc	near public, \
			hFile:WORD, wDate:WORD, wTime:WORD

		mov	ax, 5701H
		mov	bx, hFile
		mov	cx, wTime
		mov	dx, wDate
		int	21H
		jc	@F
		xor	ax, ax
@@:
		; Return in AX the MS-DOS error. 0 = Success.
		ret

_dos_setftime	endp

END
