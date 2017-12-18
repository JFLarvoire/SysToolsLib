		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    lseek.asm						      ;
;									      ;
;   DESCRIPTION:    MS-DOS move file poionter function 42H		      ;
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
;   Function:	    lseek						      ;
;									      ;
;   Description:    MS-DOS move file pointer function 42H		      ;
;									      ;
;   Parameters:     WORD wHandle    File handle.			      ;
;		    DWORD dwOffset  Number of bytes from origin 	      ;
;		    WORD wOrigin    0=Beginning; 1=Current; 2=End	      ;
;									      ;
;   Returns:	    DX:AX   New absolute position, or -1L if error.	      ;
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

lseek		proc	near public, \
			wHandle:WORD, dwOffset:DWORD, wOrigin:WORD

		mov	bx, wHandle
		mov	cx, word ptr dwOffset+2
		mov	dx, word ptr dwOffset
		mov	al, byte ptr wOrigin
		mov	ah, 42H

		int	21h
		jnc	@F

		sbb	ax, ax
		cwd
@@:
		ret

lseek		endp

END
