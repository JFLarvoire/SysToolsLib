		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    canonic.asm 					      ;
;									      ;
;   DESCRIPTION:    Convert a far pointer to its canonical representation     ;
;									      ;
;   NOTES:								      ;
;									      ;
;   History:								      ;
;    2001/08/02 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

		.286

		.code

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    canonicalize					      ;
;									      ;
;   Description:    Convert a far pointer to its canonical representation     ;
;									      ;
;   Parameters:     DX:AX	Far pointer to convert			      ;
;									      ;
;   Returns:	    DX:AX	Same address, with offset <= 15		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   None						      ;
;									      ;
;   History:								      ;
;    2001/08/02 JFL Created this routine.				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	canonicalize

		push	ax		; Save offset
		shr	ax, 4		; Number of paragraphs
		add	dx, ax		; Update segment base
		pop	ax		; Restore initial offset
		and	ax, 0FH		; Offset relative to last paragraph
		ret

ENDCFASTPROC	canonicalize

END
