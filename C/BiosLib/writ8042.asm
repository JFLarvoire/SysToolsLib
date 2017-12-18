	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   File name	    WRIT8042.ASM					      ;
;									      ;
;   Description     Low-level routines to write to the keyboard controller    ;
;									      ;
;   Notes	    See the corresponding routines in READ8042.ASM	      ;
;									      ;
;   History								      ;
;    1990/11/02	JFL Initial implementation of autil.asm                       ;
;    1991/03/29	JFL Added routines read_8042, write_8042, interrupts_off,     ;
;   	    	    interrupts_back.                                          ;
;   	    	    Removed routine wait_8042, and replaced it with local     ;
;   	    	    routine wait_8042_ibe. Changed calls to it in routine     ;
;   	    	    reset.                                                    ;
;    1991/04/24	JFL Turn off interrupt while reading or writing the 8042      ;
;   	    	    to prevent side effects with keyboard ISRs.               ;
;    1991/11/21	JFL Slight performance improvement in wait_8042_xxx routines. ;
;    1993/10/07	JFL Separated from alibc.c                                    ;
;    1994/05/10	JFL Added include file IO8042.INC.                            ;
;   	    	    Moved routine wait_refresh to file wrefresh.asm.          ;
;   	    	    Improve write_8042 performance.                           ;
;    1994/05/11	JFL Split IO8042.ASM into READ8042.ASM and WRIT8042.ASM.      ;
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

EXTERNCFASTPROC wait_refresh

;=============================================================================;
;
;  ENTRY POINT NAME:	write_8042
;
;  DESCRIPTION: 	Write a byte to the 8042 input buffer.
;
;  INPUT PARAMETERS
;	AL  =  data byte to send
;	DX  =  8042 port to write ah data
;
;  OUTPUT PARAMETERS
;	AX  =  0 = Data read by the 8042
;	    = -1 = ERROR
;	Carry flag Set	    = ERROR. Timeout waiting.
;		   Clear    = Data read by the 8042.
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
;	  Sends specified data to specified 8042 port.	This routine waits
;	for the 8042 input buffer to be empty before sending the command
;	and also waits for the cmd/data to be accepted.  The zero-flag will
;	be set if it was unable to send the cmd/data for any reason.
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/28/91  Initial implementation.
;     JFL 04/24/91  Turn off interrupts just in case the user presses CapsLock
;		     after we call wait_8042_ibe, and before we output to dx.
;		     The BIOS ISR would output a keyboard command to set the
;		     LEDs, and the 8042 input buffer would NOT be empty anymore!
;     JFL 94/05/10  Remove wait for 8042 to read what we wrote to improve perf.
;
;=============================================================================;

CFASTPROC	write_8042

		pushf
		cli

		call	wait_8042_ibe
		jnz	write_error	; Error. Then return with zero flag set.

		out	dx, al		; Output the data to the specified port.

; JFL 94/05/10	 call	 wait_8042_ibe	 ; Wait for 8042 to read its input buffer
; JFL 94/05/10	 jnz	 write_error	 ; Error. Then return with zero flag set.

		popf			; Restore interrupts
		xor	ax, ax		; mov ax,0 and clear carry.
		ret
write_error:
		popf			; Restore interrupts
		stc
		sbb	ax, ax		; mov ax,-1 and set carry.
		ret

ENDCFASTPROC	write_8042

;=============================================================================;
;
;  ENTRY POINT NAME:	wait_8042_ibe
;
;  DESCRIPTION: 	Wait for 8042 input buffer to be empty.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	Z-flag: (1) = Input buffer empty - 8042 ready for command
;		(0) = ERROR
;
;  REGISTERS DESTROYED: None
;
;  REGISTERS PRESERVED: All
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;	Wait for 8042 input buffer to be empty.  Will clear z-flag if
;	it isn't empty after a while.   This routine is processor speed
;       independent.
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/28/91  Initial implementation.
;     JFL 11/21/91  Bypass call to wait_refresh on entry. Improves performance.
;
;=============================================================================;

CFASTPROC	wait_8042_ibe

		push	cx
		push	ax

		mov	cx, 6666		; Wait up to 100ms
		jmp	short wait_ibe_test
wait_ibe_loop:
		call	wait_refresh		; Be CPU speed independant
wait_ibe_test:
		in	al, IO_8042_STATUS	; Wait for the keyboard input
		test	al, I8042_IBF		;  buffer to be empty
		loopnz	wait_ibe_loop		; If the buffer is still full
						; then return zero flag cleared
		pop	ax
		pop	cx
		ret

ENDCFASTPROC	wait_8042_ibe

END
