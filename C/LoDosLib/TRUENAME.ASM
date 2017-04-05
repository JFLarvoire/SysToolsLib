		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    truename.asm					      ;
;									      ;
;   DESCRIPTION:    Undocumented DOS function 60h: TrueName		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/29 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    TrueName						      ;
;									      ;
;   Description:    Expand the pathname using the undocumented truename func. ;
;									      ;
;   Parameters:     BX	    128 bytes buffer for the canonicalized name       ;
;		    AX	    The relative name to process		      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, ES.						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/27 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	TrueName

		push	si
		push	di

		mov	si, ax		; The relative name
		push	ds
		pop	es
		mov	di, bx		; The true name result

		mov	ah, 60h
		int	21h
		check_error

		pop	di
		pop	si
		ret

ENDCFASTPROC	TrueName

END
