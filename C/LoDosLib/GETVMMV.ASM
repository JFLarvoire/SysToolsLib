	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	GetVmmV.asm					      ;
;									      ;
;	Contents:	Get Virtual Machine Manager version		      ;
;									      ;
;	Notes:								      ;
;									      ;
;	History:							      ;
;	 1995/11/10 JFL Created this file				      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.model small, C

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetVmmVersion					      ;
;									      ;
;	DESCRIPTION:	Get Virtual Machine Manager version		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = Version. For example 0x030B for Windows 3.11     ;
;			     0 ==> Not running under Windows enhanced.	      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

GetVmmVersion	proc	public

		mov	ax, 1600h
		int	2fh
		xchg	al, ah
		cmp	ah, 0
		je	fail
		cmp	ah, 80H
		je	fail
		ret
fail:
		xor	ax, ax
		ret

GetVmmVersion	endp

END
