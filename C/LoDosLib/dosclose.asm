		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    dosclose.asm					      ;
;									      ;
;   DESCRIPTION:    MS-DOS close file handle function			      ;
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
;   Function:	    _dos_close						      ;
;									      ;
;   Description:    MS-DOS close file handle function			      ;
;									      ;
;   Parameters:     AX	    File handle 				      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX.						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/09/04 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_close

		mov	bx, ax
		mov	ah, 3EH

		int	21h
		check_error

		ret

ENDCFASTPROC	_dos_close

END
