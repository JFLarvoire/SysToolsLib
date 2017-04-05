		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    getdate.asm						      ;
;									      ;
;   DESCRIPTION:    BIOS get RTC date int 1AH function 04H 		      ;
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
;   Function:	    _bios_getdate					      ;
;									      ;
;   Description:    BIOS get RTC date int 1AH function 04H		      ;
;									      ;
;   Parameters:     BX = Pointer to a _biosdate_t structure		      ;
;									      ;
;   Returns:	    AX = 0 = Success; Updated _biosdate_t structure.	      ;
;									      ;
;   Notes:	    Converts BCD output to binary			      ;
;									      ;
;   Regs altered:   AX, CX, DX.						      ;
;									      ;
;   History:								      ;
;    2016-04-24 JFL Recreated this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

EXTERNCFASTPROC	bcdtoh

CFASTPROC	_bios_getdate

		mov	ah, 04H
		clc				; Some buggy BIOSs do not clear it
		int	1AH
		sbb	ax, ax			; Carry -> AX. 0=Success
		jc	_bios_getdate_failed

		mov	al, dl			; BCD Day
		call	bcdtoh			; Binary day
		mov	BYTE ptr [bx], al

		mov	al, dh			; BCD Month
		call	bcdtoh			; Binary month
		mov	BYTE ptr [bx+1], al

		mov	al, cl			; BCD Year
		call	bcdtoh			; Binary year
		mov	cl, al

		mov	al, ch			; BCD Century
		call	bcdtoh			; Binary century
		mov	ah, 100
		mul	ah			; AX=AL*AH
		xor	ch, ch
		add	ax, cx
		mov	WORD ptr [bx+2], ax	; AX=Year

_bios_getdate_failed:
		ret

ENDCFASTPROC	_bios_getdate

END
