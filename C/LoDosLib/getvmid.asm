	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	GetVmID.asm					      ;
;									      ;
;	Contents:	Get the current virtual machine ID		      ;
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
;	NAME:		GetVMID 					      ;
;									      ;
;	DESCRIPTION:	Get the current virtual machine ID		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX=VM ID. 0=None. 1=System VM. Other=Virtual DOS box  ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

GetVMID 	proc	public

		mov	ax, 1683H
		int	2FH
		cmp	ax, 1600H	; Is the call supported?
		mov	ax, bx		; Get the ID, assuming it is supported.
		je	@F		; Jump if it is
		xor	ax, ax		; Else return 0 if it's not supported.
@@:
		ret

GetVMID 	endp

END
