	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	GetVxdEP.asm					      ;
;									      ;
;	Contents:	Get a VxD V86 entry point			      ;
;									      ;
;	Notes:								      ;
;									      ;
;	History:							      ;
;	 1995/11/10 JFL Created this file				      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE ADEFINE.INC

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetVxdEntryPoint				      ;
;									      ;
;	DESCRIPTION:	Get a VxD V86 entry point			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	AX = VxD ID					      ;
;									      ;
;	    ON EXIT:	DX:AX far pointer to the entry point.		      ;
;									      ;
;	REGS ALTERED:	AX, BX, ES					      ;
;									      ;
;=============================================================================;

CFASTPROC	GetVxdEntryPoint

		push	di

		xor	di, di
		mov	es, di		; ES:DI = NULL on entry
		mov	bx, ax		; Device ID
		mov	ax, 1684H	; Function number
		int	2Fh		; Get protected mode API address
		mov	ax, di
		mov	dx, es

		pop	di
		ret

ENDCFASTPROC	GetVxdEntryPoint

END
