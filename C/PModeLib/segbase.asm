	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	segbase.asm					      ;
;									      ;
;	Contents:	Get segment and far pointers linear address	      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			94/02/24      ;
;									      ;
;	Notes:								      ;
;									      ;
;	History:							      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE ADEFINE.INC

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetSegmentBase					      ;
;									      ;
;	DESCRIPTION:	Get the base of a segment			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	AX = Real mode segment				      ;
;									      ;
;	    ON EXIT:	DX:AX	32 bits base of the segment		      ;
;									      ;
;	REGS ALTERED:	AX, CX, DX					      ;
;									      ;
;=============================================================================;

CFASTPROC	GetSegmentBase

		xor	dx, dx
		mov	cx, 4
@@:
		shl	ax, 1
		rcl	dx, 1
		loop	@B
		ret

ENDCFASTPROC	GetSegmentBase

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetV86LinearAddress				      ;
;									      ;
;	DESCRIPTION:	Get the linear address of a Virtual 86 address	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	AX = Offset					      ;
;			DX = Segment					      ;
;									      ;
;	    ON EXIT:	DX:AX	32 bits linear address			      ;
;									      ;
;	NOTES:		The routine is implemented with the _cdecl calling    ;
;			convention, even though this is not optimal, because  ;
;			_fastcall routines do not pass far pointers in	      ;
;			registers anyway.				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GetV86LinearAddress proc C public, wOffset:WORD, wSegment:WORD

		mov	ax, wSegment
		call	GetSegmentBase	    ; DX:AX = The V86 segment base
		add	ax, wOffset
		adc	dx, 0
		ret

GetV86LinearAddress endp

END
