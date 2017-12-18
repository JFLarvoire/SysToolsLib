	PAGE	,132
	TITLE	Protected Mode access routines, part of PMODE.LIB

;*****************************************************************************;
;									      ;
;	FileName:	XMS.ASM 					      ;
;									      ;
;	Contents:	XMS access routines				      ;
;									      ;
;	Author: 	Jean-Francois LARVOIRE			1995/02/07    ;
;									      ;
;	Notes:		These instructions should have been proviledged,      ;
;			but are not. This can be considered as an x86 bug.    ;
;									      ;
;			Using DPMI, it is possible to access the system's     ;
;			GDT and LDT, and then break any security.	      ;
;									      ;
;	History:							      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc	; All assembly language definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Global data 							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.data

		PUBLIC	lpXMS, wlpXMSValid

lpXMS		dd	0		; XMS Entry point. NULL = No XMS.
wlpXMSValid	dw	0		; TRUE if the above pointer is valid


;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

page
;===FUNCTION HEADER=========================================================
;
; NAME: GetXMSAddress
;
; DESCRIPTION:
;	Get the XMS entry point
;
; NOTE:
;
;
;
; ENTRY:
;       none
;
; EXIT:
;	DX:AX = XMS Driver entry point.
;		0 means Error, no XMS Driver loaded.
;
; MODIFIED:
;	AX, BX, DX, ES
;
;===END HEADER==============================================================

CFASTPROC	GetXMSAddress

	; Check is there's an int 2F vector. The ISA POST clears vector 2F.
	xor	ax, ax
	mov	es, ax
	les	ax, es:[4*2FH]
	mov	dx, es
	or	ax, dx
	jz	XMSDone 		; Jump if no XMS

	; Check if an XMS driver is loaded
	mov	ax, 4300H
	int	2FH
	cmp	al, 80H
	jne	XMSDone 		; Jump if AL was not equal to 80H

	; If so, get the entry point
	mov	ax, 4310H
	int	2FH
	mov	word ptr lpXMS, bx	; Save the XMS entry point
	mov	word ptr lpXMS+2, es
XMSDone:
	or	wlpXMSValid, 1		; Validate lpXMS as is
	les	ax, lpXMS		; Get the return value
	mov	dx, ax
	ret

ENDCFASTPROC	GetXMSAddress

END
