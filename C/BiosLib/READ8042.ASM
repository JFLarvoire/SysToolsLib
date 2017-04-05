	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.

;*****************************************************************************;
;									      ;
;   File name	    READ8042.ASM					      ;
;									      ;
;   Description     Low-level routines to read data from the keyboard cntrlr. ;
;									      ;
;   Notes	    See the corresponding routines in WRIT8042.ASM	      ;
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
;  ENTRY POINT NAME:	read_8042
;
;  DESCRIPTION: 	Read a byte from the 8042 output buffer.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AX  =	   -1	    = ERROR. Timeout waiting for data.
;	    =	 0 to 255   = Data read
;	Carry flag Set	    = ERROR. Timeout waiting for data.
;		   Clear    = Data read
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
;	Will wait up to 100 ms for data.
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/28/91  Initial implementation.
;     JFL 04/24/91  Turn off interrupts to prevent an ISR to read the output
;		     buffer before we get a chance.
;
;=============================================================================;

CFASTPROC	read_8042

		pushf
		cli

		call	wait_8042_obf
		jz	read_error	; Error. Then return -1

		in	al, IO_8042_DATA

		popf			; Restore interrupts
		xor	ah, ah		; mov ah,0 and clear carry.
		ret
read_error:
		popf			; Restore interrupts
		stc
		sbb	ax, ax		; mov ax,-1 and set carry.
		ret


ENDCFASTPROC	read_8042

;=============================================================================;
;
;  ENTRY POINT NAME:	wait_8042_obf
;
;  DESCRIPTION: 	Wait for 8042 output buffer to be full.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	Z-flag: (1) = ERROR
;		(0) = Output buffer full - data available
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
;	Wait for 8042 output buffer to be full.  Will set z-flag if
;	it isn't full after a while.  This routine is processor speed
;       independent.
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/28/91  Initial implementation.
;     JFL 11/21/91  Bypass call to wait_refresh on entry. Improves performance.
;
;=============================================================================;

CFASTPROC	wait_8042_obf

		push	cx
		push	ax

		mov	cx, 6666		; Wait up to 100ms
		jmp	short wait_obf_test
wait_obf_loop:
		call	wait_refresh		; Be CPU speed independant
wait_obf_test:
		in	al, IO_8042_STATUS	; Wait for the keyboard output
		test	al, I8042_OBF		;  buffer to be full
		loopz	wait_obf_loop		; If the buffer is still empty

		pop	ax
		pop	cx
		ret

ENDCFASTPROC	wait_8042_obf

END
