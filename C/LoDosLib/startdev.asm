		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    startdev.asm					      ;
;									      ;
;   DESCRIPTION:    Generic device driver startup module		      ;
;									      ;
;   NOTES:	    Must be linked first.				      ;
;									      ;
;		    Calls during the initialization the C routine:	      ;
;			int _cdecl devmain(char far *fpParms)		      ;
;		    If devmain() returns 0, the driver stays resident.	      ;
;		    Else it is unloaded from memory.			      ;
;									      ;
;		    Calls the C routines Read, Write, Peek, RStatus, WStatus. ;
;		    They must be implemented in a resident module with a .r   ;
;		    extension. See DOSDRV.H for prototypes.		      ;
;									      ;
;		    Adaptations:					      ;
;		    - Set the driver name in the device header structure.     ;
;		    - Change the list of supported functions if needed.       ;
;									      ;
;   HISTORY:								      ;
;									      ;
;    1995-08-22 JFL Leveraged this file. Set the driver name to "MARMOTT$".   ;
;    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; BIOS.LIB definitions
		INCLUDE DOSDRV.INC	; MS-DOS device drivers definitions

DGROUP		GROUP	RESID, _TEXT, CONST, _DATA, _BSS, C_COMMON, STACK

public		_acrtused	; Identify this module as the startup module
_acrtused	equ	0	; The actual value does not matter

;=============================================================================;
;			     Structures, equates			      ;
;=============================================================================;

init_request	struc			; Init command request structure

		db	(size RequestHeader) dup (?) ; Standard request header
n_units		db	?		; Number of units
end_address	dd	?		; Driver end address
bpb		dd	?		; parameters / bpb pointer
dev_number	db	?		; Block device number (read only)

init_request	ends

;=============================================================================;
;			     Driver header & data			      ;
;=============================================================================;

RSEG

		ASSUME	DS:NOTHING, ES:NOTHING, SS:NOTHING

;		Device driver header (Must be at offset 0)

		public	deviceHeader
deviceHeader	label	DeviceHeader

		dd	-1			; Only one driver in this file
		dw	08020H			; Character device
		dw	offset strategy		; Pointer to strategy routine
		dw	offset interrupt	; Pointer to interrupt routine
		db	'MARMOTT$'		; 8 byte character device name
		db	0			; Make the device name accessi-
						; -ble by C string functions
		db	0			; Align the rest on a dword

;=============================================================================;
;			      Resident variables			      ;
;=============================================================================;

pRequest	dd	?			; Request pointer

;=============================================================================;
;			       Strategy routine				      ;
;=============================================================================;

strategy	proc	far

		mov	word ptr pRequest , bx
		mov	word ptr pRequest +2, es
		ret

strategy	endp

;=============================================================================;
;			      Interrupt routine				      ;
;=============================================================================;

dispatch	label	word			; Command dispatch table

		dw	offset DGROUP:init	; Command 0 = initialization
		dw	offset DGROUP:unsup	; Command 1 = Media check
		dw	offset DGROUP:unsup	; Command 2 = Build BPB
		dw	offset DGROUP:unsup	; Command 3 = IOCTL input
		dw	offset DGROUP:Read	; Command 4 = Input
		dw	offset DGROUP:Peek	; Command 5 = Peek
		dw	offset DGROUP:RStatus	; Command 6 = Input status
		dw	offset DGROUP:succeed	; Command 7 = Input flush
		dw	offset DGROUP:Write	; Command 8 = Output
		dw	offset DGROUP:Write	; Command 9 = Output & verify
		dw	offset DGROUP:WStatus	; Command 10 = Output status
		dw	offset DGROUP:succeed	; Command 11 = Output flush
		dw	offset DGROUP:unsup	; Command 12 = IOCTL output
		dw	offset DGROUP:succeed	; Command 13 = Open device
		dw	offset DGROUP:succeed	; Command 14 = Close device

DISPATCH_SIZE	equ	$-offset dispatch	; Size of the table

interrupt	proc	far

		push	ax
		push	bx
		push	cx
		push	dx
		push	si
		push	di
		push	bp
		push	ds
		push	es

		cld				; Always useful

		les	bx, pRequest		; Get the request pointer

						; Get the command byte
		mov	al, (RequestHeader PTR es:[bx]).rhFunction
		IFDEF	_DEBUG
		call	DebugStart
		ENDIF
		cbw				; Convert it to a word,
		shl	ax, 1			; then to an offset in the
		mov	si, ax			;  dispatch table.

		cmp	si, DISPATCH_SIZE	; Is this outside of the table?
		mov	ax, 8003H		; Assume failure
		jae	unsupported

		push	cs			; Load segment registers for C
		pop	ds

		push	es			; Push argument for C
		push	bx
		call	word ptr dispatch[si]	; Routine is supported. Call it.
		pop	bx			; Restore ES:BX
		pop	es
unsupported:
		or	ah, 1			; Set "done" bit in status code.
		mov	(RequestHeader PTR es:[bx]).rhStatus, ax

		IFDEF	_DEBUG
		call	DebugStop
		ENDIF

		pop	es
		pop	ds
		pop	bp
		pop	di
		pop	si
		pop	dx
		pop	cx
		pop	bx
		pop	ax

		ret

interrupt	endp

;=============================================================================;
;			  Resident command routines			      ;
;=============================================================================;

; All others have been moved to the C .R modules

succeed		proc	near

		sub	ax, ax

unsup		proc	near

		ret

unsup		endp

succeed		endp

;=============================================================================;
;			 Optional resident DEBUG code			      ;
;=============================================================================;

IFDEF		_DEBUG

		ASSUME	DS:NOTHING, ES:NOTHING, SS:NOTHING

DebugStart	proc	near

		mov	ah, '!'
		xchg	al, ah
		call	outc		    ; Output the I/O character
		mov	al, ah
		call	outal
		mov	al, ah		    ; Restore the function code
		ret

DebugStart	endp


DebugStop	proc	near

		push	ax
		mov	al, '.'
		call	outc		    ; Output the end code
		pop	ax
		ret

DebugStop	endp

THR		equ	0		; Transmitter Holding Register
LSR		equ	5		; Line Status Register

THRE		equ	020H		; LSR Transmit Holding Register Empty

		public	DebugPort	    ; Debug port base address.
DebugPort	dw	0		    ; Default: None

		public	outc
outc		proc	near		; Output a character to a serial port

		push	dx
		push	bx

		mov	bx, DebugPort	    ; Debug port base address
		test	bx, bx
		jz	no_debug

		push	ax
		lea	dx, [bx+LSR]
@@:
		in	al, dx
		test	al, THRE
		jz	@B		    ; Wait til the transmit reg is empty

		pop	ax
		lea	dx, [bx+THR]
		out	dx, al		    ; Send the character
no_debug:
		pop	bx
		pop	dx
		ret

outc		endp

		public	outal
outal		proc	near		; Ouput AL in hexadecimal

		push	ax
		shr	al, 1
		shr	al, 1
		shr	al, 1
		shr	al, 1
		call	outh
		pop	ax
		call	outh
		ret

outal		endp

outh		proc	near		; Output AL lowest nibble in hexadecimal

		push	ax
		and	al, 0FH
		cmp	al, 0AH
		jb	@F
		add	al, ('A'-0AH)-'0'
@@:
		add	al, '0'
		call	outc		    ; Output the function number
		pop	ax
		ret

outh		endp

ENDIF

		ASSUME	DS:NOTHING, ES:NOTHING, SS:NOTHING

;=============================================================================;
;									      ;
;=============================================================================;

ENDRS

;=============================================================================;
;		      Discardable initialization routine		      ;
;=============================================================================;

		.CODE

extrn		devmain:near

init		proc	near

		; Load segment registers for C

		ASSUME	DS:DGROUP   ; ds already set

		; Clear the uninitialized data area and near heap

		mov	dx, cs
		mov	es, dx
		mov	di, offset DGROUP:_BSS	    ; Beginning of the area
		mov	cx, offset DGROUP:STACK     ; end of the area
		sub	cx, di			    ; size in bytes (it's even)
		shr	cx, 1			    ; Idem in words
		sub	ax, ax
		rep	stosw			    ; Clear the area

		; Load a stack for C

		call	SwitchStack

		; Call the C main initialization routine

		les	bx, pRequest
		les	bx, es:[bx].bpb
		push	es	    ; Push a far pointer to the parameter line
		push	bx
		call	devmain
		add	sp, 4	    ; Remove the parameters from the stack

		; Restore the pointer to the request packet

		les	bx, pRequest

		; Switch back to the DOS stack

		pop	cx
		pop	ss
		mov	sp, cx

		test	ax, ax
		jz	success

		; Driver failure

		mov	word ptr es:[bx].end_address, 0
		mov	word ptr es:[bx].end_address+2, cs

		mov	ax, 80FEH	; Initialization failed.

		ret

success:
		mov	word ptr es:[bx].end_address, offset DGROUP:_TEXT
		mov	word ptr es:[bx].end_address+2, cs

		sub	ax, ax		; Initialization succeeded.

		ret

init		endp

;=============================================================================;
;
;  ENTRY POINT NAME:	SwitchStack
;
;  DESCRIPTION: 	Switch to a local C-compatible stack
;
;  INPUT PARAMETERS
;	DS = DGROUP
;
;  OUTPUT PARAMETERS
;	SS:SP = Top of local stack. First DWORD on it = Top of DOS stack
;
;  REGISTERS DESTROYED: AX, BX, CX, DX, ES
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;	The trick with DOS versions allows to be loaded high in small UMBs
;	 under DOS 5 and later versions. The UMB should at least 6 or 7 KB
;	 for a sucessful initialization.
;
;
;  MODIFICATION HISTORY NOTES
;   JFL 93/08/04 Initial implementation.
;
;=============================================================================;

SwitchStack	proc	near

		ASSUME	DS:DGROUP

		mov	dx, ds	    ; dx = DGROUP now on

		; Get DOS version number

		mov	ah, 30H
		int	21H	    ; Get DOS version. Destroys BX, CX

		les	bx, pRequest ; Restore the pointer to request

		; For DOS versions below 5, we must provide a dummy end address

		cmp	al, 5
		jae	@F	    ; Jump if at least DOS 5

		mov	ax, dx	    ; AX = DGROUP
		add	ax, 1000H   ; AX = DGROUP + 64 KB
		mov	word ptr es:[bx+end_address], 0
		mov	word ptr es:[bx+end_address+2], ax
@@:
		; Compute the top of stack address

		mov	ax, word ptr es:[bx+end_address]
		mov	cl, 4
		shr	ax, cl	    ; Convert offset to paragraphs
		add	ax, word ptr es:[bx+end_address+2]  ; Add segment
		sub	ax, dx	    ; Number of paragraph we own
		cmp	ax, 1000H   ; More than 64 KB available?
		jbe	@F
		mov	ax, 1000H   ; If more, use only 64 KB anyway
@@:
		shl	ax, cl	    ; Convert paragraphs to offset of top of st.
		mov	bx, ax	    ; Top of local stack

		; Save the DOS stack top on the new stack

		pop	ax	    ; Return address

		dec	bx
		dec	bx
		mov	[bx], ss
		dec	bx
		dec	bx
		mov	[bx], sp

		; Now switch stack. No need to clear ints for stack switches.

		mov	ss, dx
		mov	sp, bx

		; Cleanup and return

		push	ax	    ; Return address
		xor	bp, bp	    ; Used by Debuggers to end stack back trace

		ret

		ASSUME	DS:NOTHING, ES:NOTHING, SS:NOTHING

SwitchStack	endp

;=============================================================================;
;			       Discardable data				      ;
;=============================================================================;

END
