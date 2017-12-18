	page	,132
;*****************************************************************************;
;									      ;
;   File Name:	    STARTCOM.ASM					      ;
;									      ;
;   Description:    Generic tiny model C programs and TSRs startup module     ;
;									      ;
;   Notes:	    This module is the startup module for .COM C programs.    ;
;  		    It must be linked first.                                  ;
;  		    The C files must be compiled in the tiny memory model.    ;
;  		    As far as I know, this is the same as compiling in        ;
;  		    the small memory model.                                   ;
;                                                                             ;
;  		    Programs that can't fit in 64 KB of code+data+stack can't ;
;  		    be .COM programs and must be .EXE programs. For them use  ;
;  		    instead the RomSetup module startexe.asm.                 ;
;                                                                             ;
;   History:								      ;
;    1993/10/06 JFL Adapted from the EMU startup module                       ;
;    1993/10/08 JFL Save the PSP address                                      ;
;    1994/04/27 JFL Renamed the main C routine as cmain since it does not     ;
;		    pass the standard argc/argv arguments.                    ;
;		    Renamed this file as STARTCOM.ASM.                        ;
;		    Made it more consistent with STARTEXE.ASM.                ;
;    2001/03/27 JFL Make sure RESIDEND is ordered as it should.               ;
;									      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc

; Add the RESID & _TEXT segments to the DGROUP group, so that the segments
; initialization work as expected in the tiny memory model.

DGROUP		GROUP	RESID, _TEXT

;-----------------------------------------------------------------------------;
;									      ;
;   Local data								      ;
;									      ;
;-----------------------------------------------------------------------------;

public		_acrtused	; Identify this module as the startup module
_acrtused	equ	0	; The actual value does not matter

; Error messages

		.data

msg_286 	db	"Requires a 80286 or better", 0DH, 0AH, 0


		.data?

		public	_psp
_psp		dw	?

;-----------------------------------------------------------------------------;
;									      ;
;   Program Segment Prefix, and startup code				      ;
;									      ;
;-----------------------------------------------------------------------------;

		RSEG				; Code segment

extrn		cmain:near

		; Program segment prefix area

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

		; End of program segment prefix

		org	100H			; Necessary for .COM programs
one_hundred:
		jmp	near ptr begin		; Jump around rest of RESID seg.

		ENDRS

RESIDEND	SEGMENT WORD PUBLIC 'CODE'	; Reserved to compute RESID size
End_of_RSEG	label	near			; Force segment instanciation in the right order.
RESIDEND	ENDS

		.code
begin:
		cld				; Always useful

		; Load segment registers for C

			; We're using the C tiny model, so segments are
			; already good.

		ASSUME	ds:DGROUP

		mov	_psp, es		; Save our PSP base

		; Is this a 80286 or better?

		push	sp			; The 8086 pushes SP-2
		pop	ax			; The newer ÊP push SP
		sub	ax, sp			; =-2 on a 8086; =0 on newer ÊP.

		jz	ok_286

		mov	si, offset DGROUP:msg_286
		jmp	write_string		; Return directly to Op Sys.
ok_286:
		.286

		; Check the available memory

			; All DOS memory allocated for .COM programs

		; Switch to the final stack

			; Stack already at the end of the 64 KB segment

		ASSUME	SS:DGROUP

		; Free memory to DOS
IF 0
		mov	bx, ds			; DGROUP base
		mov	ax, sp			; DGROUP size in bytes
		dec	ax			; Prepare rounding
		shr	ax, 4			; Convert to paragraphs
		inc	ax			; Round DGROUP size in paragraph
		add	bx, ax			; + DGROUP size
		mov	ah, 4AH
		int	DOS			; Set block (ES = PSP)
ENDIF
		ASSUME	ES:DGROUP

		; Clear the uninitialized data area and near heap

		mov	di, offset DGROUP:_BSS	; Beginning of the area
		mov	cx, sp			; end of the area
		sub	cx, di			; size of the area in bytes
		shr	cx, 1			; Idem in words
		sub	ax, ax
		rep	stosw			; Clear the area

		; Call the C main routine

		jmp	cmain			; ~~jfl 2001/03/27 Use jump instead of call. Saves 1 byte.

		; Exit

		; retn	; ~~jfl 2001/03/27 cmain() makes a near return to the invoking routine

;====================  HP SETUP IN ROM PROCEDURE HEADER  =====================;
;
;  ENTRY POINT NAME:	write_string
;
;  DESCRIPTION:		Output a string on display
;
;  INPUT PARAMETERS
;	DS:SI		String address. The string must be terminated by a NUL.
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX, BX, SI
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;	Loops using the write character to teletype video BIOS call.
;	This is guarantied to work even on original PCs and compatibles.
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/02/90 Initial implementation.
;
;=============================================================================;

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

		retn

ENDCFASTPROC	MakeFP

write_string	endp

end		one_hundred
