		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    getdate.asm						      ;
;									      ;
;   DESCRIPTION:    MS-DOS get date function 2AH	 		      ;
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
;   Function:	    _dos_getdate					      ;
;									      ;
;   Description:    MS-DOS get file time function 2AH 			      ;
;									      ;
;   Parameters:     BX = Pointer to a _dosdate_t structure		      ;
;									      ;
;   Returns:	    Updated _dosdate_t structure.			      ;
;									      ;
;   Notes:	    Assumes a _dosdate_t structure different from that of MSVC.
;									      ;
;   Regs altered:   AX, CX, DX.						      ;
;									      ;
;   History:								      ;
;									      ;
;    2001/12/21 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_getdate

		mov	ah, 2AH
		int	21H
		mov	WORD ptr [bx], dx   ; DL=day, DH=month
		mov	WORD ptr [bx+2], cx ; CX=Year
		mov	BYTE ptr [bx+4], al ; AL=day of week
		ret

ENDCFASTPROC	_dos_getdate

END
