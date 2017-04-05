	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.

;*****************************************************************************;
;									      ;
;   FILE NAME:	    STRCMP.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1990/11/02 JFL Initial implementation of alibc.asm.                      ;
;    1991/02/15 JFL Added strcmp.                                             ;
;    1991/06/24 JFL Added routine strncmp.                                    ;
;    1993/10/07 JFL Separated from alibc.c                                    ;
;									      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc	; All assembly language definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	strcmp & strncmp
;
;  DESCRIPTION: 	Compare two strings
;
;  INPUT PARAMETERS
;	ARG1	BX = Pointer to the first string
;	ARG2	AX = Pointer to the second string
;      [ARG3]	DX = Max number of characters to compare (strncmp only)
;
;  OUTPUT PARAMETERS
;	AX	First difference, or 0 if they match
;
;  REGISTERS DESTROYED: AX, BX, DX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 02/15/91 Initial implementation.
;   JFL 06/24/91 Added strncmp
;
;=============================================================================;

CFASTPROC	strcmp

		xor	dx, dx			; Compare up to 64 KB chars

		; Fall through into strncmp

ENDCFASTPROC	strcmp

CFASTPROC	strncmp

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

		test	al, al			; First character NUL?
		jz	done_cmp		; Yes, return difference
		test	dl, dl			; Second character NUL?
		jz	done_cmp		; Yes, return difference

		cmp	al, dl			; Compare the characters
		loope	next_cmp		; Continue if they're equal
done_cmp:
		xor	ah, ah			; Convert uchar to signed int
		xor	dh, dh
		sub	ax, dx			; Return signed int difference

		pop	cx
		pop	si
		ret

ENDCFASTPROC	strncmp

END
