		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    extdopen.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS extended open function 6CH			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/09/04 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    ExtendedOpen					      ;
;									      ;
;   Description:    MS-DOS extended open function 6CH			      ;
;									      ;
;   Parameters:     char *pszName   File name				      ;
;		    WORD wMode	    Open mode. 0=R/O, 1=W/O, 2=R/W; +Share    ;
;		    WORD wAttrib    File attributes			      ;
;		    WORD wAction    See DOS Programmer's ref                  ;
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
;    1995/09/04 JFL Created this routine				      ;
;    1996/06/26 JFL Fix bug: Store the handle through pHandle.		      ;
;									      ;
;-----------------------------------------------------------------------------;

ExtendedOpen	proc	near public uses si, \
			pszName:PTR BYTE, wMode:WORD, wAttrib:WORD, \
			wAction:WORD, pHandle:PTR WORD

		mov	si, pszName
		mov	bx, wMode
		mov	cx, wAttrib
		mov	dx, wAction
		mov	ah, 6CH

		int	21h
		jc	@F

		mov	bx, pHandle
		mov	[bx], ax
		xor	ax, ax
@@:
		ret

ExtendedOpen	endp

END
