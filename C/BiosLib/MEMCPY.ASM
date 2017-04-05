		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    memcpy.asm						      ;
;									      ;
;   DESCRIPTION:    Copy a block of memory				      ;
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

		.286

		.code

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    memcpy						      ;
;									      ;
;   Description:    Copy a block of memory				      ;
;									      ;
;   Parameters:     BX	    Pointer to the destination buffer		      ;
;		    AX	    Pointer to the source buffer		      ;
;		    DX	    Number of characters to copy		      ;
;									      ;
;   Returns:	    AX	    The destination pointer			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, ES						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/29 JFL Created this routine, adapted from memcmp.		      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	memcpy

		push	si
		push	di
		push	cx

		push	ds
		pop	es

		mov	di, bx			; Destination
		mov	si, ax			; Source
		mov	cx, dx			; Count in CX
		rep	movsb

		mov	ax, bx			; Destination

		pop	cx
		pop	di
		pop	si
		ret

ENDCFASTPROC	memcpy

END
