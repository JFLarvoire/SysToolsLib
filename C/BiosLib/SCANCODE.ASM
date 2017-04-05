	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   File name	    SCANCODE.ASM					      ;
;									      ;
;   Description     Routines to read scancodes. Supports all AT2 keyboard keys.
;									      ;
;   Notes	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   History								      ;
;    1990/11/02	JFL Initial implementation of autil.asm                       ;
;    1991/02/18	JFL Added routine get_scancode to return an untouched value.  ;
;    1993/10/07	JFL Separated from alibc.c                                    ;
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
;  ENTRY POINT NAME:	get_scancode
;
;  DESCRIPTION: 	Get a code (scancode+asciicode) from the keyboard buffer
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AL	ASCII code
;	AH	Scancode
;
;  REGISTERS DESTROYED: AX
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
;   JFL 11/02/90 Initial implementation.
;
;=============================================================================;

CFASTPROC	get_scancode

		mov	ah, F16_GET_EXT_KEY
		int	KEYBOARD

		ret

ENDCFASTPROC	get_scancode

;=============================================================================;
;
;  ENTRY POINT NAME:	get_keycode
;
;  DESCRIPTION: 	Get a code (scancode+asciicode) from the keyboard buffer
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AL	ASCII code
;	AH	Scancode
;		Notes: o AH is 0 for ASCII keys; AL is 0 for Non-ASCII keys.
;		       o New AT 2 arrows and text processing keys scancodes
;			 are converted to their AT 1 equivalent on the numeric
;			 cursor pad.
;
;  REGISTERS DESTROYED: AX
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
;   JFL 11/02/90 Initial implementation.
;
;=============================================================================;

CFASTPROC	get_keycode	; Return extended key code
				; 00XX for ASCII; YY00 for PC scan code.

		call	get_scancode

		test	al, al			; ASCII?
		jz	scancode_done		; If not, return scan code in AH.
		cmp	al, 0E0H		; AT II extended code?
		je	at2_code		; Yes, do special treatment.
		sub	ah, ah			; Else ASCII. Clear scan code.
		jmp	short scancode_done
at2_code:
		test	ah, ah			; IBM 8 code E0H?
		jz	scancode_done		; Yes, return unchanged.
		sub	al, al			; Else convert to equivalent
						;  AT1 scancode.
scancode_done:
		ret

ENDCFASTPROC	get_keycode

END
