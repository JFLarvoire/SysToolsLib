		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    getticks.asm					      ;
;									      ;
;   DESCRIPTION:    BIOS get tick count int 1AH function 00H 		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    2016-04-24 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2016-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _bios_getticks					      ;
;									      ;
;   Description:    BIOS get tick count int 1AH function 00H 		      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    DX:AX = Number of tick counts since last midnight.	      ;
;									      ;
;   Notes:	    The tick frequency is ~= 65536 ticks / hour.	      ;
;	https://blogs.msdn.microsoft.com/oldnewthing/20041202-00/?p=37153     ;
;									      ;
;   Regs altered:   AX, CX, DX.						      ;
;									      ;
;   History:								      ;
;    2016-04-24 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_bios_getticks

		mov	ah, 00H
		int	1AH
		mov	ax, dx
		mov	dx, cx
		ret

ENDCFASTPROC	_bios_getticks

END
