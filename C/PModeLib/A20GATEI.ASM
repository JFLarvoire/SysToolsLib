	PAGE	,132
	TITLE	Protected Mode access routines, part of PMODE.LIB

;*****************************************************************************;
;									      ;
;   FileName:	    A20GateI.ASM					      ;
;									      ;
;   Contents:	    A20 Gate using the ISA method (Through the keybd contrlr) ;
;									      ;
;   Notes:	    Routines isa_enable_a20 and isa_disable_a20.	      ;
;									      ;
;		    Before using isa_enable_a20, make sure that...	      ;
;		    - HIMEM.SYS is not loaded. Else use XMS to toggle	      ;
;		       the A20 line.					      ;
;		    - Interrupts are disabled. Any interrupt chain depending  ;
;		      on the 1MB wrap around is potentially deadly to the     ;
;		      system.						      ;
;		    							      ;
;   History:								      ;
;    1989/07/21	JFL Initial implementation in prot.asm.			      ;
;    1995/02/06	JFL Split from prot.asm, and adapted to NODOS.LIB.	      ;
;		    The return value is no longer in the Zero Flag, but	      ;
;		     in the carry flag and the AX register.		      ;
;    1997/05/21	JFL Fixed a bug in isa_disable_a20(), which caused a '7'      ;
;		     to appear in the keyboard input buffer		      ;
;    1998/03/05	JFL Do not rely on the NUL command to know if the gate	      ;
;		     is enabled or disabled. instead, compare the memory      ;
;		     at adresses 0 and 1M.				      ;
;		    Added routine is_a20_enabled().			      ;
;									      ;
;      (c) Copyright 1989-2017 Hewlett Packard Enterprise Development LP      ;
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

EXTERNCFASTPROC write_8042

PAGE

page
;===FUNCTION HEADER=========================================================
;
; NAME: is_a20_enabled
;
; DESCRIPTION:
;	Test if A20 is enabled.
;
; NOTE:
;
; ENTRY:
;       none
;
; EXIT:
;	AX = 0	and  ZF = 1	A20 is disabled
;	AX = 1	and  ZF = 0	A20 is enabled
;
; MODIFIED:
;	AX, CX
;
;===END HEADER==============================================================

CFASTPROC	is_a20_enabled

	push	ds
	push	es
	push	si
	push	di

	xor	ax, ax		; Assume it's disabled

	mov	si, ax
	mov	ds, ax		; Compare from 0:0
	mov	di, 0FFFFH
	mov	es, di
	mov	di, 10H 	; to FFFF:10
	mov	cx, 10H 	; Assume 20 bytes is enough
	repe	cmpsw

	je	@F		; Jump if *0 == *1MB. A20 is disabled.
	inc	ax		; Different ==> A20 is enabled.
@@:
	pop	di
	pop	si
	pop	es
	pop	ds
        ret                             ; return status in the zero flag

ENDCFASTPROC	is_a20_enabled


page
;===FUNCTION HEADER=========================================================
;
; NAME:	isa_enable_a20
;
; DESCRIPTION:
;       This routine turns the A20 gate on using the double byte ISA command
;	to the 8042 keyboard controller.
;
; NOTE:
;	Interrupts must be disabled by the calling routine while A20 is enabled.
;	Any interrupt chain depending on the 1MB wrap around is potentially
;	deadly to the system.
;
; ENTRY:
;       none
;
; EXIT:
;	AX  =  0 = Data read by the 8042
;	    = -1 = ERROR
;	Carry flag Set	    = ERROR. Timeout waiting.
;		   Clear    = Data read by the 8042.
;
; MODIFIED:
;	AX, DX
;
;===END HEADER==============================================================

CFASTPROC	isa_enable_a20

	mov	al, I8042_WRITE_OUTPUT	; send command to write to the 8042
	mov	dx, IO_8042_CMD 	;   output port
	call	write_8042		; if 8042 failed to accept the command
	jc	isa_enable_exit 	;   then return error

	mov	al, SET_A20_COMMAND	; turn on A20 gate by writing a new
	mov	dx, IO_8042_DATA	;   value to the 8042 output port
	call	write_8042		; if 8042 failed to accept the command
	jc	isa_enable_exit 	;   then return error
@@:
	call	is_a20_enabled
	jz	@B

isa_enable_exit:
        ret                             ; return status in the zero flag

ENDCFASTPROC	isa_enable_a20





page
;===FUNCTION HEADER=========================================================
;
; NAME:	isa_disable_a20
;
; DESCRIPTION:
;       This routine turns the A20 gate off using the double byte ISA command
;       to the 8042 keyboard controller.
;
; ENTRY:
;       none
;
; EXIT:
;	AX  =  0 = Data read by the 8042
;	    = -1 = ERROR
;	Carry flag Set	    = ERROR. Timeout waiting.
;		   Clear    = Data read by the 8042.
;
; MODIFIED:
;	AX, DX
;
;===END HEADER==============================================================

CFASTPROC	isa_disable_a20

	mov	al, I8042_WRITE_OUTPUT	; send command to write to the 8042
	mov	dx, IO_8042_CMD 	;   output port
	call	write_8042		; if 8042 failed to accept the command
	jc	isa_disable_exit	;   then return error

	mov	al, RST_A20_COMMAND	; turn off A20 gate by writing a new
	mov	dx, IO_8042_DATA	;   value to the 8042 output port
	call	write_8042		; if 8042 failed to accept the command
	jc	isa_disable_exit	;   then return error
@@:
	call	is_a20_enabled
	jnz	@B

isa_disable_exit:
        ret                             ; return status in the zero flag

ENDCFASTPROC	isa_disable_a20

END
