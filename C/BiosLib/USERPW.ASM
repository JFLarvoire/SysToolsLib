	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    USERPW.ASM						      ;
;									      ;
;   DESCRIPTION:    Routines to manage the 8042 user password.		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;    1994/03/11 JFL Added include file IO8042.INC.			      ;
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
include 	io8042.inc	; 8042-specific definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

EXTERNCFASTPROC read_8042
EXTERNCFASTPROC write_8042
EXTERNCFASTPROC disable_kbd_n_mouse
EXTERNCFASTPROC restore_kbd_n_mouse

;=============================================================================;
;
;  ENTRY POINT NAME:	download_user_pw
;
;  DESCRIPTION:  Downloads a user password to the 8042.
;
;  INPUT PARAMETERS
;	 BX = Pointer to NUL terminated password string of scancodes.
;
;	      Note: Passwords normally end by an <Enter> make scancode (1CH).
;		    Else the last character of the password would unlock the
;		    keyboard, not the <Enter> following it as expected.
;		    The BIOS appends one <Enter> make scancode automatically
;		    to the end of the password configured EEPROM or CMOS.
;
;  OUTPUT PARAMETERS
;	 AX  =	0 = Password successfully downloaded
;	     = -1 = ERROR
;	 Carry flag Set      = ERROR. Timeout waiting.
;		    Clear    = Data read by the 8042.
;
;  REGISTERS DESTROYED:
;	 AX
;
;  REGISTERS PRESERVED:
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   LA	06/06/91 Initial implementation.
;   JFL 11/21/91 Removed the automatic appending of an <Enter> scancode. It is
;		  now the responsability of the calling routine to append it.
;
;=============================================================================;

CFASTPROC	download_user_pw

		push	si
		push	dx

		mov	si, bx

; Disable the keyboard and the mouse and save they initial state on the stack

		call	disable_kbd_n_mouse
		jc	error_pw_load1

		push	ax			; Save the command byte

; Inform the 8042 that a password is coming

		mov	al, LOAD_PW_CMD 	; Tell 8042 that password is coming
		mov	dx, IO_8042_CMD
		call	write_8042
		jc	error_pw_load2

; Download the password

		mov	dx, IO_8042_DATA
pw_more_bytes:
		lodsb
		cmp	al, 0
		jz	short pw_end		; NULL marks the end of the PW

		call	write_8042		;  No: download this scan code and
		jc	error_pw_load2		;  try the others ...
		jmp	pw_more_bytes
pw_end:
		call	write_8042		; Write the final NUL
		jc	error_pw_load2

; Restore the keyboard and mouse initial state

		pop	ax			; Get old command byte off stack
		call	restore_kbd_n_mouse
		jc	error_pw_load3

		xor	ax, ax			; Report success, clear carry
		jmp	short exit_pw_load

error_pw_load2:
		pop	ax			; Restore stack & k&m state
		call	restore_kbd_n_mouse	; Restore k&m state. Ignore errs
error_pw_load1:
error_pw_load3:
		stc				; Flag error
		sbb	ax, ax			; ax = -1 and set carry

exit_pw_load:
		pop	dx
		pop	si
		ret

ENDCFASTPROC	download_user_pw

END
