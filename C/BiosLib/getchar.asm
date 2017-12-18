	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GETCHAR.ASM 					      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;    1995/08/29 JFL Made getch() a synonym for getchar().		      ;
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
;  ENTRY POINT NAME:	getchar
;
;  DESCRIPTION:		Get a character from stdin. C standard library.
;			Modified to get it from the console.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AX	ASCII code in AL, 0 in AH.
;		If AX is 0, the next call will return the scancode of a
;		non ASCII key.
;
;  REGISTERS DESTROYED: AX, CX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;	Get an ASCII code from the keyboard buffer.
;	If it's not ASCII,
;	    stuff the scancode back into the keyboard buffer and return NUL.
;	This way, the next call returns it.
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/02/90 Initial implementation.
;   JFL 94/03/11 Removed unused label test_ascii.
;
;=============================================================================;

CFASTPROC	getch
CFASTPROC	getchar

		mov	ah, F16_GET_KEY
		int	KEYBOARD

		test	al, al
		jz	store_non_ascii
		cmp	al, 0FFh
		je	retrieve_non_ascii
		sub	ah, ah			; Clear the scan code if ASCII
		ret

store_non_ascii:
		mov	ch, ah			; scancode
		mov	cl, 0FFH		; flag to retrieve code later
		mov	ah, F16_PUT_KEY
		int	KEYBOARD
		sub	ax, ax			; Return NUL
		ret

retrieve_non_ascii:
		mov	al, ah			; Return scancode as ascii code
		sub	ah, ah
		ret

ENDCFASTPROC	getchar
ENDCFASTPROC	getch

END
