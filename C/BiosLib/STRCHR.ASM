		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    strchr.asm						      ;
;									      ;
;   DESCRIPTION:    Search a character in a string			      ;
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
;   Function:	    strchr						      ;
;									      ;
;   Description:    Search a character in a string			      ;
;									      ;
;   Parameters:     BX	    Pointer to the NUL-terminated string	      ;
;		    AL	    character to look for			      ;
;									      ;
;   Returns:	    AX	    The address of the first match, or NULL if none.  ;
;									      ;
;   Notes:	    The NUL character is a valid target too.		      ;
;									      ;
;   Regs altered:   AX							      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/29 JFL Created this routine, adapted from strcmp.		      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	strchr

		push	si

		mov	si, bx			; String pointer in SI
		mov	ah, al			; Reference character in AH
next_cmp:
		lodsb				; Next character in AL
		cmp	al, ah			; Compare the characters
		je	done_cmp		; Done if they're equal
		test	al, al			; End of string reached?
		jnz	next_cmp		; Loop if not.
		mov	si, 1
done_cmp:
		dec	si			; Address of the matching char.
		mov	ax, si			; Result

		pop	si
		ret

ENDCFASTPROC	strchr

END
