		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    beep.asm						      ;
;									      ;
;   DESCRIPTION:    Emit a tone on the internal speaker 		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1989/10/16 JFL Created this file.					      ;
;    1995/10/17 JFL Moved it into NODOSLIB.				      ;
;									      ;
;      (c) Copyright 1989-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE TIMER.INC	; For 8054 definitions

		.286

		.code

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    beep						      ;
;									      ;
;   Description:    Emit a tone on the PC internal speaker		      ;
;									      ;
;   Parameters:     AX	    Frequency (Hertz)				      ;
;		    DX	    Duration (milli-seconds)			      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, CX, DX					      ;
;									      ;
;   History:								      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	beep

		mov	bx, ax		; BX = Frequency
		mov	cx, dx		; CX = Duration

		mov	dx, 18
		mov	ax, 13519	; dx:ax = 1,193,166 Hz
		div	bx		; period in (1/1.19MHz = 838ns) unit.

		xchg	ax, bx		; AX=frequency; BX=period in 838ns u.

		mul	cx
		mov	cx, 500 	; "* 2 / 1000" = "/ 500"
		div	cx
		mov	dx, ax		; Number of alternances

		; Enable counting and sound
		in	al, SPUCTRL
		jmp	$+2
		push	ax		; Save so we can restore it later
		or	al, SPEAKER + GATE
		out	SPUCTRL, al

		; Start timer 2
		mov	al, RWBOTH + MODE3 + SC2
		pushf
		cli
		out	TIMERSCTRL, al
		mov	ax, bx
		out	TIMER2, al
		jmp	$+2
		mov	al, ah
		out	TIMER2, al
		popf

		mov	cx, dx		; Arm loop
		sub	ah, ah
CYCLE:
		in	al, SPUCTRL
		and	al, OUTPUT2	; Mask timer 2 output
		cmp	al, ah		; Same as last?
		je	CYCLE		; If yes, wait
		mov	ah, al		; Remember the change
		loop	CYCLE		; and count the alternances

		pop	ax		; Restore port
		out	SPUCTRL, al

		ret

ENDCFASTPROC	beep

END
