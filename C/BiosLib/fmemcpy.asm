		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    fmemcpy.asm 					      ;
;									      ;
;   DESCRIPTION:    Copy a far block of memory				      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/09/14 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

		.286

		.code

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _fmemcpy						      ;
;									      ;
;   Description:    Copy a far block of memory				      ;
;									      ;
;   Parameters:     ARG1    Far pointer to the destination buffer	      ;
;		    ARG2    Far pointer to the source buffer		      ;
;		    ARG3    Number of characters to copy		      ;
;									      ;
;   Returns:	    DX:AX   The destination pointer			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, CX, ES						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/09/14 JFL Created this routine, adapted from memcpy.		      ;
;									      ;
;-----------------------------------------------------------------------------;

_fmemcpy	proc	C public, lpDEST:DWORD, lpSrc:DWORD, wCount:WORD

		push	si
		push	di
		push	ds

		lds	si, lpSrc		; Source
		les	di, lpDest		; Destination
		mov	cx, wCount		; Count in CX

		mov	ax, di			; Copy destination into output
		mov	dx, es

		rep	movsb

		pop	ds
		pop	di
		pop	si
		ret

_fmemcpy	endp

END
