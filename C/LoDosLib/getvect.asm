		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    getvect.asm 					      ;
;									      ;
;   DESCRIPTION:    Microsoft C library routine _dos_getvect		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/23 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _dos_getvect					      ;
;									      ;
;   Description:    Get an interrupt vector				      ;
;									      ;
;   Parameters:     AX	    Interrupt number				      ;
;									      ;
;   Returns:	    DX:AX   The handler address.			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, ES. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_getvect

		get_vector  al
		mov	ax, bx
		mov	dx, es

		ret

ENDCFASTPROC	_dos_getvect

END
