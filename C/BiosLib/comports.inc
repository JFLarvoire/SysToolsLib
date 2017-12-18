;*****************************************************************************;
;									      ;
;	FILENAME:	comports.inc				94/09/08      ;
;									      ;
;	CONTENTS:	Serial ports management equates.		      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			93/06/22      ;
;									      ;
;	USAGE:								      ;
;									      ;
;	History:							      ;
;									      ;
;      (c) Copyright 1993-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

COM1		equ	3F8H		; I/O addresses
COM2		equ	2F8H
COM3		equ	3E8H
COM4		equ	2E8H

INTCOM1 	equ	0CH		; Interrupt number
INTCOM2 	equ	0BH
INTCOM3 	equ	72H
INTCOM4 	equ	73H

; UART registers. The names below are those in the UART manual

RBR		equ	0		; Receiver Buffer Register
THR		equ	0		; Transmitter Holding Register
IER		equ	1		; Interrupt Enable Register
IIR		equ	2		; Interrupt Identification Register
LCR		equ	3		; Line Control Register
MCR		equ	4		; Modem Control Register
LSR		equ	5		; Line Status Register
MSR		equ	6		; Modem Status Register
SCR		equ	7		; Scratch Register
DLL		equ	0		; Divisor Latch Least significant
DLM		equ	1		; Divisor Latch Most significant
FCR		equ	2		; Fifo Control Register

; UART registers bits

ERBFI		equ	001H		; IER enable Received data available I.
ETBEI		equ	002H		; IER Enable Transm. Hold. reg. empty I
ELSI		equ	004H		; IER Enable Receiver Line Status I.
EDSSI		equ	008H		; IER Enable MODEM Status I.

IIRIP		equ	001H		; IIR No interrupt pending bit
RLSIP		equ	006H		; IIR Receiver Line Status I. Pending
RDAIP		equ	004H		; IIR Received Data Available I. P.
THREIP		equ	002H		; IIR Trans. Hold. Reg. Empty I. P.
MSIP		equ	000H		; IIR Modem Status I. Pending
FFST		equ	0C0H		; IIR FiFo STatus bits. 0 if no FIFO.

FFE		equ	001H		; FCR FiFo Enable
FFRR		equ	002H		; FCR FiFo Receiver Reset
FFXR		equ	004H		; FCR FiFo Transmitter Reset
FFDMA		equ	008H		; FCR FiFo DMA mode select
FFRT		equ	0C0H		; FCR FiFo Receiver Trigger

WLS5		equ	000H		; LCR Word Length 5 bits
WLS6		equ	001H		; LCR Word Length 6 bits
WLS7		equ	002H		; LCR Word Length 7 bits
WLS8		equ	003H		; LCR Word Length 8 bits
STB1		equ	000H		; LCR 1 stop bits
STB2		equ	004H		; LCR 1.5 (5 bits) or 2 (6-8) stop bits
PEN		equ	008H		; LCR Parity ENable
EPS		equ	010H		; LCR Even Parity Select
LCRSP		equ	020H		; LCR Stick Parity
LCRSB		equ	040H		; LCR Set Break
DLAB		equ	080H		; LCR Divisor Latch Access Bit
PARN		equ	000H		; Parity None
PARE		equ	PEN  + EPS	; Parity Even
PARO		equ	PEN		; Parity Odd
PAR1		equ	PEN  + LCRSP	; Parity Mark
PAR0		equ	PAR1 + EPS	; Parity Space

DTR		equ	001H		; MCR Data Terminal Ready
RTS		equ	002H		; MCR Request to send
OUT1		equ	004H		; MCR
OUT2		equ	008H		; MCR
IEN		equ	OUT2		; MCR Interrupt Enable on PC compatib.
LLM		equ	010H		; MCR Local Loopback Mode

DR		equ	001H		; LSR Data Ready
OE		equ	002H		; LSR Overrun error
PE		equ	004H		; LSR Parity error
FE		equ	008H		; LSR Framing Error
BI		equ	010H		; LSR Break Interrupt
THRE		equ	020H		; LSR Transmit Holding Register Empty
TEMT		equ	040H		; LSR Transmitter Empty

DCTS		equ	001H		; MSR Delta Clear To Send
DDSR		equ	002H		; MSR Delta Data Set Ready
TERI		equ	004H		; MSR Trailing Edge Ring Indicator
DDCD		equ	008H		; MSR Delta Data Carrier Detected
CTS		equ	010H		; MSR Clear to Send
DSR		equ	020H		; MSR Data Set Ready
RI		equ	040H		; MSR Ring Indicator
DCD		equ	080H		; MSR Data Carrier Detected

; Macros

slowin		macro	reg, port	; Give time to react to slow old UARTs
		in	reg, port
		jmp	$+2
		endm

slowout 	macro	port, reg	; Give time to react to slow old UARTs
		out	port, reg
		jmp	$+2
		endm

popff		macro			; Pop flags. Fixes early 286s bug, and
		local	SkipSti, SkipCli ; 386 virtual 86 mode problem.
		push	bp
		mov	bp, sp
		xchg	ax, [bp+2]	; Exchange AX and the old flags
		push	ax		; Push the old flags again on stack
		test	ah, 2		; Test the interrupt flag
		jz	SkipSti
		popf			; Restores all flags, but possibly IF
		sti			; Make sure the IF is set if it must
		jmp	short SkipCli
SkipSti:
		popf			; Restores all flags, but possibly IF
		cli			; Make sure the IF is clear if it must
SkipCli:
		pop	bp
		pop	ax
		endm

; Common handshake characters

XOFF		equ	13H
XON		equ	11H

;-----------------------------------------------------------------------------;
