		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    gettime.asm						      ;
;									      ;
;   DESCRIPTION:    MS-DOS get time function 2CH	 		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    2001/12/21 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_gettime					      ;
;									      ;
;   Description:    MS-DOS get file time function 2CH	 		      ;
;									      ;
;   Parameters:     BX = Pointer to a _dostime_t structure		      ;
;									      ;
;   Returns:	    Updated _dostime_t structure.			      ;
;									      ;
;   Notes:	    Assumes a _dostime_t structure different from that of MSVC.
;									      ;
;   Regs altered:   AX, CX, DX.						      ;
;									      ;
;   History:								      ;
;									      ;
;    2001/12/21 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_gettime

		mov	ah, 2CH
		int	21H
		mov	WORD ptr [bx], dx   ; DL=hundredth, DH=seconds
		mov	WORD ptr [bx+2], cx ; CL=minutes, CH=hours
		ret

ENDCFASTPROC	_dos_gettime

END
