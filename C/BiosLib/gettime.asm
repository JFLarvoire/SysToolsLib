		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    gettime.asm						      ;
;									      ;
;   DESCRIPTION:    BIOS get RTC date int 1AH function 02H 		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    2016-04-24 JFL Recreated this file.				      ;
;									      ;
;      (c) Copyright 2016-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _bios_gettime					      ;
;									      ;
;   Description:    BIOS get RTC date int 1AH function 02H 		      ;
;									      ;
;   Parameters:     BX = Pointer to a _biostime_t structure		      ;
;									      ;
;   Returns:	    AX = 0 = Success; Updated _biosdate_t structure.	      ;
;									      ;
;   Notes:	    							      ;
;									      ;
;   Regs altered:   AX, CX, DX.						      ;
;									      ;
;   History:								      ;
;    2016-04-24 JFL Recreated this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

EXTERNCFASTPROC	bcdtoh

CFASTPROC	_bios_gettime

		mov	ah, 02H
		clc				; Some buggy BIOSs do not clear it
		int	1AH
		sbb	ax, ax			; Carry -> AX. 0=Success
		jc	_bios_gettime_failed

		mov	al, dh			; BCD seconds
		call	bcdtoh			; Binary seconds
		mov	dh, al

		mov	al, cl			; BCD minutes
		call	bcdtoh			; Binary minutes
		mov	cl, al

		mov	al, ch			; BCD hours
		call	bcdtoh			; Binary hours
		mov	ch, al

		mov	WORD ptr [bx], dx	; DL=DST Flag, DH=seconds
		mov	WORD ptr [bx+2], cx	; CL=minutes, CH=hours

_bios_gettime_failed:
		ret

ENDCFASTPROC	_bios_gettime

END
