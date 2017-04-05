	page	, 132
;*****************************************************************************;
;									      ;
;	FILENAME:	vcpitst.asm					      ;
;									      ;
;	CONTENTS:	VCPI detection and management routines test	      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			94/01/18      ;
;									      ;
;	Usage:		run the vcpitst program 			      ;
;									      ;
;			If the program runs and the PC comes back alive then  ;
;			rejoice: VCPI routines work. Indeed any bug is likely ;
;			to cause a processor shutdown, followed by a reboot.  ;
;									      ;
;	Notes:		This program is independant of NODOS.LIB.	      ;
;			It can be recompiled in any C memory model, to test   ;
;			the VCPI management routines in other memory models.  ;
;			Just change the .model directive below. 	      ;
;									      ;
;	History:							      ;
;	  95/02/15 JFL	Extracted from VCPI.ASM.			      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.model		tiny, C

include 	PMODE.INC

;*****************************************************************************;
;									      ;
;			     Constants and macros			      ;
;									      ;
;*****************************************************************************;

DOS		equ	21H	    ; MS-DOS API interrupt

display 	macro	msg
		local	msg$
.data
msg$		db	msg, "$"
.code
		mov	dx, offset msg$
		mov	ah, 9
		int	DOS
		endm

PG		equ	80000000H   ; Paging bit of CR0

;*****************************************************************************;
;									      ;
;				 Code segment				      ;
;									      ;
;*****************************************************************************;

.386p

.code

		.startup
		call	the_test
		.exit

;*****************************************************************************;
;									      ;
;				 Data segment				      ;
;									      ;
;*****************************************************************************;

.data		; Make sure the DATA class is defined before the stack class
.data?		; Make sure the BSS class is defined before the stack class

.code

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		display_hex_eax 				      ;
;			display_hex_ax					      ;
;			display_hex_3n					      ;
;			display_hex_al					      ;
;			display_hex_1n					      ;
;									      ;
;	DESCRIPTION:	Display an hexadecimal number 8, 4, 3, 2, 1 digit long;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ax = Number to display (_ax and _3n)		      ;
;			al = Number to display (_al and _1n)		      ;
;									      ;
;	    ON EXIT:	None						      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

hex		db	"0123456789ABCDEF"

display_hex_eax proc	near

		push	eax
		shr	eax, 16
		call	display_hex_ax

		pop	eax
		call	display_hex_ax

		ret

display_hex_eax endp

display_hex_ax	proc	near

		push	ax
		mov	al, ah
		call	display_hex_al

		pop	ax
		call	display_hex_al

		ret

display_hex_ax	endp

display_hex_3n	proc	near

		push	ax
		mov	al, ah
		call	display_hex_1n

		pop	ax
		call	display_hex_al

		ret

display_hex_3n	endp

display_hex_al	proc	near

		push	ax
		shr	al, 1
		shr	al, 1
		shr	al, 1
		shr	al, 1
		call	display_hex_1n

		pop	ax
		call	display_hex_1n

		ret

display_hex_al	endp

display_hex_1n	proc	near

		push	dx
		push	bx
		push	ax

		and	ax, 0FH
		mov	bx, ax
		mov	dl, hex[bx]
		mov	ah, 2
		int	21H		    ; display_char

		pop	ax
		pop	bx
		pop	dx

		ret

display_hex_1n	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		my_callback					      ;
;									      ;
;	DESCRIPTION:	Routine to be called back while in protected mode     ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	None						      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

.data

save_eax	dd	?

.code

my_callback	proc

		mov	eax, cr0
		and	eax, NOT PG
		mov	cr0, eax		; Turn pagination off

		mov	ax, 18H 		; FLAT_DES - My_GDT
		mov	es, ax
		mov	ebx, 0FFFFFFF1H
		mov	eax, es:[ebx]
		mov	save_eax, eax

		mov	eax, cr0
		or	eax, PG
		mov	cr0, eax		; Turn pagination back on

		ret

my_callback	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		the_test					      ;
;									      ;
;	DESCRIPTION:	The main VCPI test routine			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	None						      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

the_test	proc

		INVOKE	VCPI2PMAndCallBack, ADDR my_callback
		test	ax, ax
		jz	@F
showerror:
		push	ax
		display <"Error ">
		pop	ax
		call	display_hex_al
		display <0DH, 0AH>
		jmp	short the_test_end
@@:
		INVOKE	vcpi_cleanup

		display <"Read ">
		mov	eax, save_eax
		call	display_hex_eax

		display <0DH, 0AH, "Done", 0DH, 0AH>
the_test_end:
		ret

the_test	endp

.stack

END
