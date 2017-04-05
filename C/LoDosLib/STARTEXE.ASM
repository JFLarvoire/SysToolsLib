	page	,132
	title	EXE Startup module
	subttl	startexe.asm

;*****************************************************************************;
;									      ;
;   FILE NAME:	    startexe.asm					      ;
;									      ;
;   DESCRIPTION:    Mini EXE program startup module, for use with BIOS.LIB.   ;
;									      ;
;   NOTES:	    This module is the startup module for .EXE C programs.    ;
;		    The C files must be compiled in the small memory model.   ;
;		    Note: As far as I know, this is the same as compiling in  ;
;		    the tiny memory model.				      ;
;									      ;
;		    Programs that fit in 64 KB of code+data+stack can be .COM ;
;		    programs. For them use instead the module startup.asm.    ;
;									      ;
;		    It won't accept to run on a 8088/8086, since BIOS.LIB     ;
;		    is compiled using 286-specific instructions.	      ;
;									      ;
;   HISTORY:								      ;
;    1991-04-11 JFL Initial implementation				      ;
;    1991-05-28 JFL Removed obsolete comments. Added the notes above.	      ;
;		    Renamed the C main routine main() to be more generic.     ;
;    1991-10-18 JFL Changed the STACKSIZE to 2000H because experience shows   ;
;		     that the memory allocated between the data and stack can ;
;		     collide with the stack during HD autodetection.	      ;
;		    Allow to have up to 64 KB of far data after the stack.    ;
;		    Extend DGROUP to 64 KB, and move this far data segment    ;
;		     afterwards.					      ;
;    1994-04-14 JFL Adapted from RomSetup's STARTEXE.ASM.                     ;
;    1995-08-22 JFL Adapted from LLKBD's LLKSTART.ASM.                        ;
;    1995-09-01 JFL Made generic for the small memory mode.		      ;
;    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              ;
;									      ;
;      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include 	adefine.inc

DOS		equ	21H	; MS-DOS software interrupt

public		_acrtused	; Identify this module as the startup module
_acrtused	equ	0	; The actual value does not matter

;-----------------------------------------------------------------------------;
;									      ;
;   Program Segment Prefix						      ;
;									      ;
;-----------------------------------------------------------------------------;

PSP		SEGMENT AT 0	; Use dummy absolute address

		org	002H
		public	EndOfAllocMem
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
		public	SegEnv
SegEnv		label	word

		org	080H
		public	ArgLineSize
ArgLineSize	label	byte

		org	081H
		public	ArgLine
ArgLine 	label	byte

PSP		ENDS

;-----------------------------------------------------------------------------;
;									      ;
;   Local data								      ;
;									      ;
;-----------------------------------------------------------------------------;

		.data

		; Error messages
msg_286		db	"Requires a 80286 or better", 0DH, 0AH, 0
msg_mem 	db	"Not enough memory", 0DH, 0AH, 0

		public	_psp
_psp		dw	?			; PSP segment

;-----------------------------------------------------------------------------;
;									      ;
;   Startup code							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

extern		cmain:near

begin:
		ASSUME	DS:NOTHING, ES:PSP, SS:STACK

		cld				; Always useful

		; Load segment registers for C

		mov	ax, DGROUP
		mov	ds, ax

		ASSUME	DS:DGROUP

		mov	_psp, es		; Save PSP segment

		; Is this a 80286 or better?

		push	sp			; The 8086 pushes SP-2
		pop	bp			; The newer ÊP push SP
		sub	bp, sp			; =-2 on a 8086; =0 on newer ÊP.

		jz	ok_286

		mov	si, offset DGROUP:msg_286
		call	write_string
		jmp	short exit
ok_286:
		.286

		; Check the available memory

		mov	bx, EndOfAllocMem
		sub	bx, ax
		cmp	bx, 1000H		; We want at least 64 KB free
		jae	enough_mem		;  for DATA and STACK

		mov	si, offset DGROUP:msg_mem
		call	write_string
		jmp	short exit
enough_mem:
		; Switch to the final stack

		mov	ss, ax			; DGROUP
		sub	sp, sp			; Use 64 KB of data+stack space

		ASSUME	SS:DGROUP

		; Free memory to DOS

		mov	bx, ax			; DGROUP base
		mov	ax, es
		sub	bx, ax			; DGROUP - PSP
		add	bx, 1000H		; + DGROUP size
		mov	ah, 4AH
		int	DOS			; Set block (ES = PSP)
						; NO-OP when run from ROM
		push	ds
		pop	es			; Now on, we don't need the PSP

		ASSUME	ES:DGROUP

		; Clear the uninitialized data area and near heap

		mov	di, offset DGROUP:_BSS	; Beginning of the area
		mov	cx, sp			; end of the area
		sub	cx, di			; size of the area in bytes
		shr	cx, 1			; Idem in words
		sub	ax, ax
		rep	stosw			; Clear the area

		; Call the C main routine

		call	cmain

		; Exit
exit:
		push	_psp
		push	0
		retf

		.8086

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    write_string					      ;
;									      ;
;   Description:    Output a string to the display			      ;
;									      ;
;   Parameters:     DS:SI   String address. Must be terminated by a NUL.      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:	    Loops using the write character to teletype video BIOS    ;
;		    call. This is guarantied to work even on original PCs     ;
;		    and compatibles.					      ;
;									      ;
;   Regs altered:   AX, BX, SI. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1990-02-11 JFL Created this routine				      ;
;    1995-08-22 JFL Moved here the definition of MakeFP because this is the   ;
;		     only near return in startexe.asm.			      ;
;									      ;
;-----------------------------------------------------------------------------;

write_string	proc	near

		lodsb
		test	al, al
		jz	short write_end

		mov	ah, 0EH
		mov	bx, 0007H
		int	VIDEO
		jmp	write_string
write_end:

CFASTPROC	MakeFP

		ret

ENDCFASTPROC	MakeFP

write_string	endp

;-----------------------------------------------------------------------------;
;									      ;
;   initial stack							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.stack

		; Reserve space for the beginning of the initialization
		; The stack will then move to the and of the 64K DGROUP.
		; This space will then be overlayed by the local heap growth
		; managed by routine malloc().

end		begin
