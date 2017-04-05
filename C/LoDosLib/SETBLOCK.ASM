	page	, 132
;*****************************************************************************;
;									      ;
;   FileName:	    setblock.asm					      ;
;									      ;
;   Contents:	    Set a DOS memory block size 			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   History:								      ;
;    1995/02/24 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE ADEFINE.INC

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		_dos_setblock					      ;
;									      ;
;	DESCRIPTION:	Set a DOS memory block size			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	AX = Size in paragraphs 			      ;
;			DX = Segment base				      ;
;			BX = Where to store the max size if failure	      ;
;									      ;
;	    ON EXIT:	AX = DOS error code, or 0 if none		      ;
;									      ;
;	REGS ALTERED:	N/A						      ;
;									      ;
;=============================================================================;

CFASTPROC	_dos_setblock

		mov	es, dx		; ES = Segment
		mov	dx, bx		; Save the pointer into DX
		mov	bx, ax		; BX = paragraphs
		mov	ah, 4AH
		int	21H
		jc	@F
		xor	ax, ax
		ret
@@:
		xchg	bx, dx		; BX = Pointer; DX = Max paragraphs
		mov	word ptr [bx], dx ; Store max parags
		ret			; Return DOS error

ENDCFASTPROC	_dos_setblock

END
