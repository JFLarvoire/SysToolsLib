	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	x86.asm 					      ;
;									      ;
;	Contents:	x86 system instructions for C programs		      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			94/05/14      ;
;									      ;
;	notes:		These instructions should have been proviledged,      ;
;			but are not. This can be considered as an x86 bug.    ;
;									      ;
;			Using DPMI, it is possible to access the system's     ;
;			GDT and LDT, and then break any security.	      ;
;									      ;
;	History:							      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.286p

.model small, C

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		_sldt						      ;
;									      ;
;	DESCRIPTION:	Get the current LDT selector			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = LDT selector				      ;
;									      ;
;	NOTES:		This routine does not work in real or virtual 86 modes;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

_sldt		proc	public

		sldt	ax
		ret

_sldt		endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		_sgdt						      ;
;									      ;
;	DESCRIPTION:	Get the current GDT base address		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	DX:AX = GDT base address			      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

.DATA?

_sgdt_buf	df	?

.CODE

_sgdt		proc	public

		sgdt	_sgdt_buf
		mov	ax, word ptr _sgdt_buf+2
		mov	dx, word ptr _sgdt_buf+4
		ret

_sgdt		endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		ReturnEAX					      ;
;									      ;
;	DESCRIPTION:	Copy EAX to DX:AX for use in C programs 	      ;
;									      ;
;	NOTES:								      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	DX = High word of EAX				      ;
;									      ;
;	REGS ALTERED:	EDX						      ;
;									      ;
;=============================================================================;

.386

ReturnEAX	proc	public

		shld	edx, eax, 16
		ret

ReturnEAX	endp

END
