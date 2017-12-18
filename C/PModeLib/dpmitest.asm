	page	60, 132
;*****************************************************************************;
;									      ;
;	FILENAME:	dpmitest.asm					      ;
;									      ;
;	CONTENTS:	Generic assembly language program. Reuse this code.   ;
;									      ;
;	Author:		Jean-Francois Larvoire			1994	      ;
;									      ;
;	USAGE:								      ;
;									      ;
;	History:							      ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.XLIST
INCLUDE		DOS.INC
.LIST
; INCLUDED	DOS.INC

.386

;*****************************************************************************;
;									      ;
;			     Constants and macros			      ;
;									      ;
;*****************************************************************************;

DEBUG		equ	TRUE

display_msg	macro	msg
		local	m
STRINGS 	segment para public use16 'CODE'
m:
		db	msg, "$"
STRINGS 	ends
		push	dx
		mov	dx, offset DGROUP:m
		call	display_string_msg
		pop	dx
		endm

CRLF		equ	<0DH, 0AH>

		page
;*****************************************************************************;
;									      ;
;			    Program segment prefix			      ;
;									      ;
;*****************************************************************************;

DGROUP		group	PSP

PSP		segment para public use16 'CODE'

		org	000H
zero:
		org	002H
EndOfAllocMem	label	word

		org	006H
F_Dispatcher	label	dword
		org	00AH
Terminate_Addr	label	dword
		org	00EH
Ctrl_C_Exit	label	dword
		org	012H
Hard_Err_Exit	label	dword

		org	02CH
segenv		label	word

		org	080H
argline		label	byte

		org	100H
OneHundred:
		jmp	begin

		page
;*****************************************************************************;
;									      ;
;			Data part of the program segment		      ;
;									      ;
;*****************************************************************************;

dpmi_es 	dw	?	; DPMI private data segment
mode_switch	dd	?	; Far pointer to the DPMI mode switch routine

dgroup_sel	dw	?
callback	dd	?	; VxD callback address

		page
;*****************************************************************************;
;									      ;
;				Main procedure				      ;
;									      ;
;*****************************************************************************;

		ASSUME		ds:DGROUP, es:DGROUP, SS:DGROUP

begin:
		cld				; Always useful

		; Free unused memory

		set_block   1000H	    ; Keep only the 64 KB a .COM needs

		; Verify DPMI is there

		mov	ax, 1687H
		int	2FH
		or	ax, ax
		jz	dpmi_present

		display_msg <"Error: No DPMI server.", CRLF>
		jmp	exit
dpmi_present:
		mov	word ptr mode_switch, di
		mov	word ptr mode_switch+2, es
		mov	dpmi_es, 0	    ; Assume no DPMI segment
		or	si, si
		jz	no_private_data

		mov	bx, si
		mov	ah, 48h
		int	21h
		jnc	ok_private_data

		display_msg <"Error: Out of memory (1)", CRLF>
		jmp	exit
ok_private_data:
		mov	dpmi_es, ax
no_private_data:

		; Switch to protected mode using DPMI

		mov	es, dpmi_es
		xor	ax, ax
		call	mode_switch		; Switch to protected mode
		jnc	@F
		display_msg <"Failed to switch to protected mode.", CRLF>
		jmp	exit
@@:
		mov	dgroup_sel, ds

		out	80H, al
		mov	ax, 0BA31H
		int	15H
		mov	callback, eax
		mov	ax, 0BA30H
		call	callback

		; The end

		display_msg <"Done.", CRLF>
exit:
		end_process	0		; End process

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		display_dec_ax					      ;
;			display_hex_ax					      ;
;			display_hex_3n					      ;
;			display_hex_al					      ;
;			display_hex_1n					      ;
;									      ;
;	DESCRIPTION:	Display ax in decimal.				      ;
;			Display an hexadecimal number 4, 3, 2, 1 digit long   ;
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

display_dec_ax	proc	near		; Display AX in decimal, format "%d".

		push	dx
		push	bx
		push	ax

		sub	dx, dx		; Don't use cwd. No signed numbers.
		mov	bx, 10
		div	bx		; AX = AX/10
		test	ax, ax		; Is quotient nul? If so, the remainder
		jz	ret_dec 	; is the first digit to display.

		call	display_dec_ax	; Display the quotient first.
ret_dec:
		add	dl, "0" 	; Convert the remainder into ASCII
		display_char dl 	; Display the remainder

		pop	ax
		pop	bx
		pop	dx

		ret

display_dec_ax	endp

hex		db	"0123456789ABCDEF"

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
		display_char hex[bx]

		pop	ax
		pop	bx
		pop	dx

		ret

display_hex_1n	endp

display_string_msg  proc    near    ; Input: SS:DX = String.
				    ; Modified: None
		push	ds
		push	ss
		pop	ds
		push	ax
		display dx
		pop	ax
		pop	ds
		ret

display_string_msg  endp

show_limit	proc	near	    ; AX = selector

		movzx	eax, ax
		lsl	eax, eax
		jnz	no_limit

		ror	eax, 16
		call	display_hex_ax
		ror	eax, 16
		call	display_hex_ax
exit_limit:
		display_msg <CRLF>

		ret
no_limit:
		display_msg <"Unknown.">
		jmp	exit_limit

show_limit	endp

PSP		ends

		end	OneHundred
