		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dossetat.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS set file attribute function 4301H		      ;
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
;   Function:	    _dos_setfileattr					      ;
;									      ;
;   Description:    MS-DOS set file attributes function 4301H		      ;
;									      ;
;   Parameters:     char *pszPathname	    The file name		      ;
;		    WORD wAttrib	    Set of attribute flags. _A_XXXX.  ;
;									      ;
;   Returns:	    AX = MS-DOS error code.				      ;
;			    0 = Success 				      ;
;			    1 = Invalid function			      ;
;			    2 = Program not found			      ;
;			    3 = Path not found				      ;
;			    5 = Access denied				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, CX, DX. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1996/09/26 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_setfileattr

		mov	dx, bx		; pszPathname
		mov	cx, ax		; wAttrib
		mov	ax, 4301H
		int	21h
		jc	@F
		xor	ax, ax
@@:
		ret

ENDCFASTPROC	_dos_setfileattr

END
