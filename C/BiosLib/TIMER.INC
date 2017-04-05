;*****************************************************************************;
;									      ;
;	FILENAME:	TIMER.INC			    April 25, 1989    ;
;									      ;
;	AUTHOR:		J.F.Larvoire			    Nov. 30, 1988     ;
;									      ;
;	DESCRIPTION:	Mnemonics for 8254 ports and commands		      ;
;									      ;
;	HISTORY:							      ;
;									      ;
;      (c) Copyright 1988-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

BASE8254	equ	40H		; Timers I/O ports
TIMER0		equ	BASE8254+0
TIMER1		equ	BASE8254+1
TIMER2		equ	BASE8254+2
TIMERSCTRL	equ	BASE8254+3

LATCH		equ	000H		; Latch count. Add SCn.
RWLOW		equ	010H		; R/W low byte. Add MODEx and SCn
RWHIGH		equ	020H		; R/W high byte. Add MODEx and SCn
RWBOTH		equ	030H		; R/W both bytes. Add MODEx and SCn
MODE0		equ	000H		; IT on terminal count: 000011111111...
MODE1		equ	002H		; Hardware 1 shot:	110000111111...
MODE2		equ	004H		; Rate generator:	111011101110...
MODE3		equ	006H		; Square wave:		110011001100...
MODE4		equ	008H		; Soft trigger strobe:	111011111111...
MODE5		equ	00AH		; Hard trigger strobe:	111110111111...
SC0		equ	000H		; Counter n
SC1		equ	040H
SC2		equ	080H

READBACK	equ	0F0H		; Read back counts and/or statusses
					; Add any set in the five following:
STATUS		equ	0F0H		; Read back status
COUNT		equ	0E0H		; Read back count
CNT2		equ	008H		; On counter 2
CNT1		equ	004H		; On counter 1
CNT0		equ	002H		; On counter 0

; SPU control port

SPUCTRL		equ	061H

GATE		equ	001H
SPEAKER		equ	002H
DISABLEPARITY	equ	004H
DISABLENMI	equ	008H
REFRESH		equ	010H
OUTPUT2		equ	020H
IOERROR		equ	040H
PARITYERROR	equ	080H

;*****************************************************************************;
;									      ;
;	Example:							      ;
;									      ;
;	ROUTINE:	beep();						      ;
;									      ;
;	Description:	Beeps.						      ;
;									      ;
;	RETURNS:	0						      ;
;									      ;
;*****************************************************************************;
;
; BEGIN		beep
;
;		push	bp
;		mov	bp,sp
;
;		; Enable counting and sound
;		in	al, SPUCTRL
;		push	ax		; Save so we can restore it later
;		or	al, SPEAKER + GATE
;		out	SPUCTRL, al
;
;		; Start timer 2
;		mov	al, RWBOTH + MODE3 + SC2
;		cli
;		out	TIMERSCTRL, al
;		mov	ax, 381
;		out	TIMER2, al
;		jmp	$+2
;		mov	al, ah
;		out	TIMER2, al
;		sti
;
;		mov	cx, 0200H	; Arm loop
;		sub	ah, ah
; CYCLE:
;		in	al, SPUCTRL
;		and	al, OUTPUT2	; Mask timer 2 output
;		cmp	al, ah		; Same as last?
;		je	CYCLE		; If yes, wait
;		mov	ah, al		; Remember the change
;		loop	CYCLE		; and count the alternances
;
;		pop	ax		; Restore port
;		out	SPUCTRL, al
;
;		xor	ax,ax
;		pop	bp
;		ret
;
; beep		ENDP
;
;*****************************************************************************;
