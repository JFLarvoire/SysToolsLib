	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    POSTERRS.ASM					      ;
;									      ;
;   DESCRIPTION:    Routines to read and write the POST error log in EBDA     ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1990/11/02 JFL Initial implementation of autil.asm.		      ;
;    1991/12/11 MM  Corrected a potential bug in SEARCH_ERROR_CODE.	      ;
;    1993/07/12 JFL Added routine GetPostErrorCodes.			      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
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
;  ENTRY POINT NAME:	GetPostErrorCodes
;
;  DESCRIPTION: 	Get the number and address of POST error codes list
;
;  INPUT PARAMETERS
;	ARG1	Pointer to a far pointer to the list
;
;  OUTPUT PARAMETERS
;	Number of codes in the list; If not 0, pointer set.
;
;  REGISTERS DESTROYED: AX, BX
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
;   JFL 93/07/12 Initial implementation.
;
;=============================================================================;

CPROC		GetPostErrorCodes

		push	bp
		mov	bp, sp

		push	es
		push	di

		mov	ax, 2100H
		int	15H		    ; BX=Number; ES:DI=Address
		jnc	OkCodes
		xor	bx, bx		    ; Unsupported if carry set=>0 codes
OkCodes:
		mov	ax, bx		    ; Return number in AX
		mov	bx, [bp+ARG1]
		mov	[bx], di	    ; Store pointer where specified
		mov	[bx+2], es

		pop	di
		pop	es
		pop	bp
		ret

ENDCPROC	GetPostErrorCodes

;=============================================================================;
;
;  ENTRY POINT NAME:	search_error_code
;
;  DESCRIPTION: 	Returns TRUE if the error code is stored in the EBDA
;
;  INPUT PARAMETERS
;       AX = POST error code number
;
;  OUTPUT PARAMETER
;
;       AX = TRUE  (code found)
;            FALSE (not found)
;
;  REGISTERS DESTROYED: AX
;
;  REGISTERS PRESERVED: All others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;           Int 15H, function 2100H
;           Exit:
;               es:di  ptr to POST error log.
;               bx  number of errors
;		ah  0  (function supported)
;
;  MODIFICATION HISTORY NOTES
;   LA 08/21/91 Initial implementation.
;   MM 12/11/91 Took into account the case of no POST error and corrected a bug.
;               Indeed, the routine returned an error in the following case:
;                 - POST error code #n exists,
;		  - we are looking for the POST error code # y (y <> x)
;
;=============================================================================;

CFASTPROC	search_error_code

		push	es
		push	di

		mov	dx, 0			; Assume error code not found

		push	ax			; Save the error code
		mov	ax, 2100h
		int	15h
		test	ah, ah			; Function supported if ah==0
		pop	ax			; Restore the error code
		jnz	restore_context		; Exit if not supported
next_error:
		test	bx, bx			; Any error code left?
		jz	restore_context 	; Exit if not.
		cmp	ax, word ptr es:[di]	; Compare to the error code
		pushf
		inc	di			; Advance the pointer
		inc	di
		dec	bx			; decrement count
		popf				; Restore the comparison result
		jne	next_error		; Loop if not found
		mov	dx, 1			; Found

restore_context:
		mov	ax, dx			; Return the result in AX
                pop     di
		pop	es
		ret

ENDCFASTPROC	search_error_code

END
