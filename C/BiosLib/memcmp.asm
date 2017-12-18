		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    memcmp.asm						      ;
;									      ;
;   DESCRIPTION:    Compare the contents of two blocks of memory	      ;
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
;   Function:	    memcmp						      ;
;									      ;
;   Description:    Compare the contents of two blocks of memory	      ;
;									      ;
;   Parameters:     BX	    Pointer to the first buffer 		      ;
;		    AX	    Pointer to the second buffer		      ;
;		    DX	    Number of characters to compare		      ;
;									      ;
;   Returns:	    AX	    First difference, or 0 if no difference	      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, DX						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/29 JFL Created this routine, adapted from strcmp.		      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	memcmp

		push	si
		push	cx

		mov	si, bx			; First pointer in SI
		mov	bx, ax			; Second pointer in BX
		mov	cx, dx			; Max count in CX
next_cmp:
		lodsb				; First character in AL
						; and advance SI
		mov	dl, byte ptr [bx]	; Second character in DL
		inc	bx			; Advance BX

		cmp	al, dl			; Compare the characters
		loope	next_cmp		; Continue if they're equal
done_cmp:
		xor	ah, ah			; Convert uchar to signed int
		xor	dh, dh
		sub	ax, dx			; Return signed int difference

		pop	cx
		pop	si
		ret

ENDCFASTPROC	memcmp

END
