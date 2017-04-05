		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    remove.asm						      ;
;									      ;
;   DESCRIPTION:    MS-DOS delete file function 41H			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1996/09/26 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    remove						      ;
;									      ;
;   Description:    MS-DOS delete file function 41H			      ;
;									      ;
;   Parameters:     char *pszPathname	    The file name		      ;
;									      ;
;   Returns:	    AX = MS-DOS error code.				      ;
;			    0 = Success 				      ;
;			    1 = Invalid function			      ;
;			    2 = Program not found			      ;
;			    3 = Path not found				      ;
;			    5 = Access denied				      ;
;									      ;
;   Notes:	    The standard C library remove function return -1 on error.;
;									      ;
;   Regs altered:   AX, CX, DX. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1996/09/26 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	remove

		mov	dx, bx		; pszPathname
		mov	ah, 41H
		int	21h
		jc	@F
		xor	ax, ax
@@:
		ret

ENDCFASTPROC	remove

END
