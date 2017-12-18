	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    DISABKBD.ASM					      ;
;									      ;
;   DESCRIPTION:    Keyboard enable/disable routines, for test purpose.	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;    1994/05/11 JFL Include IO8042.INC.					      ;
;									      ;
;      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc	; All assembly language definitions
include 	io8042.inc	; All 8042-specific definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

EXTERNCFASTPROC read_8042
EXTERNCFASTPROC write_8042

;=============================================================================;
;
;  ENTRY POINT NAME:	disable_kbd_n_mouse
;
;  DESCRIPTION:  Disable the keyboard and mouse
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	Carry flag set, and AX = -1	Error. Keyboard & mouse not disabled.
;	Carry flag clear, and AX >= 0	OK. AX = Old 8042 command byte.
;
;  REGISTERS DESTROYED:
;	AX
;
;  REGISTERS PRESERVED:
;	All others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/29/91 Initial implementation.
;
;=============================================================================;

CFASTPROC	disable_kbd_n_mouse

		push	dx

; Read the current 8042 command byte and save it on the stack

		pushf
		cli				; Prevent IRQ1 before read_8042

		mov	al, READ_CMD_BYTE	; Tell 8042 to get current 8042
		mov	dx, IO_8042_CMD
		call	write_8042
		jc	dknm_error

		call	read_8042		; Get 8042 command byte
		jc	dknm_error		;  if failure:	exit.

		popf				; Restore interrupts

		push	ax			; Save the command byte

; Disable the keyboard and the mouse

		mov	al, AUX_DISABLE_DEVICE	; Disable auxillary device
		call	write_8042
		; Assume no error

		mov	al, I8042_DISABLE_KBD_INTRF	; Disable keyboard
		call	write_8042
		; Assume no error

		pop	ax			; Restore the command byte in AL
		xor	ah, ah			; Clear AH and carry
		jmp	short dknm_exit

dknm_error:
		popf
		stc
		sbb	ax, ax			; AX=-1 and set carry

dknm_exit:
		pop	dx
		ret

ENDCFASTPROC	disable_kbd_n_mouse

;=============================================================================;
;
;  ENTRY POINT NAME:	restore_kbd_n_mouse
;
;  DESCRIPTION:  Restore the 8042 command byte
;
;  INPUT PARAMETERS
;	AX = 8042 command byte
;
;  OUTPUT PARAMETERS
;	Carry flag set, and AX = -1	Error.
;	Carry flag clear, and AX == 0	Done.
;
;  REGISTERS DESTROYED:
;	AX
;
;  REGISTERS PRESERVED:
;	All others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/29/91 Initial implementation.
;
;=============================================================================;

CFASTPROC	restore_kbd_n_mouse

		push	dx

; Restore the 8042 initial command byte

		push	ax			; Save the 8042 command byte
		mov	al, I8042_WRITE_CMD	; Tell 8042 to expect command byte data
		mov	dx, IO_8042_CMD
		call	write_8042
		pop	ax			; Restore the 8042 command byte
		jc	rknm_error

		mov	dx, IO_8042_DATA	;  and send it to the 8042.
		call	write_8042
		jc	rknm_error

		xor	ax, ax			; Report success, clear carry
		jmp	short rknm_exit

rknm_error:
		sbb	ax, ax			; AX=-1 and set carry

rknm_exit:
		pop	dx
		ret

ENDCFASTPROC	restore_kbd_n_mouse

END
