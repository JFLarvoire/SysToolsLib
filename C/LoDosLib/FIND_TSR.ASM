	page	60, 132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    FIND_TSR.ASM					      ;
;									      ;
;   DESCRIPTION:    Find a TSR int 2F handle				      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1994/05/09 JFL Extracted from Mini .COM RESET.ASM.			      ;
;									      ;
;      (c) Copyright 1993-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE 	adefine.inc

.286

.code

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		find_tsr					      ;
;									      ;
;	DESCRIPTION:	Find a TSR multiplex ID 			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	BX = Offset of the TSR ID string, NUL terminated.     ;
;									      ;
;	    ON EXIT:	Carry clear: TSR found, AX = handle (0C0H to 0FFH)    ;
;			Carry set:   TSR not found, AX = 0		      ;
;									      ;
;	REGS ALTERED:	AX						      ;
;									      ;
;	HISTORY:							      ;
;									      ;
;=============================================================================;

CFASTPROC	find_tsr

		push	ds
		push	es
		pusha
		mov	bp, sp
		sub	sp, 6		; Make room for a word and a dword

		; Compute the length of the input string

		mov	[bp-2], ds	; Save a pointer to the input string
		mov	[bp-4], bx

		xor	al, al
		mov	cx, -1
		push	ds
		pop	es
		mov	di, bx
		repne	scasb
		neg	cx
		dec	cx
		dec	cx

		mov	[bp-6], cx	; Save the length of the input string

		; Check from handle 0C0h to 0FFh

		mov	ax, 0BF00H	; AH=handle, AL=function 0
next_handle:
		add	ah, 1		; Next handle
		jc	x_find_tsr	; Not found (wrapped past 0FFh)
		push	ax		; Always destroyed by int 2F
		xor	cx, cx
		mov	es, cx		; Clear ES to see if int 2F changes it
		int	2fh		; Identify TSR. This may break all regs.
		cmp	al, -1		; Is there a TSR with this handle?
		pop	ax		; Restore handle and function number
		jne	next_handle	; No such TSR. Try the next handle

		mov	di, es		; Most TSRs return ID segment in ES
		test	di, di		; Did ES change?
		jnz	@F		; Jump if it did
		mov	es, cx		; But some return it in CX instead
@@:
		mov	di, bx		; TSR ID offset
		lds	si, [bp-4]	; Reference string pointer
		mov	cx, [bp-6]	; Reference string length
		cld
		repe	cmpsb		; Is it the TSR we're looking for?
		jne	next_handle	; Jump if it's not.

		mov	al, ah		; Move the handle to AL
		xor	ah, ah		; Clear AH and carry

x_find_tsr:
		mov	[bp+14], ax	; Save AX in stack frame
		mov	sp, bp
		popa			; Restore general regs, including new AX
		pop	es
		pop	ds
		ret

ENDCFASTPROC	find_tsr

end
