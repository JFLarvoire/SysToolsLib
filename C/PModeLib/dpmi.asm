	page	, 132
;*****************************************************************************;
;									      ;
;	FileName:	dpmi.asm					      ;
;									      ;
;	Contents:	DOS Protected mode routines			      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			94/05/14      ;
;									      ;
;	Notes:								      ;
;									      ;
;	History:							      ;
;	  95/02/16 JFL	Added routine dpmi_detect.			      ;
;			Renamed vm2prot as dpmi2prot.			      ;
;			Changed the include file to pmode.inc.		      ;
;	  95/02/24 JFL	Changed SetLDTSelfDesc to GetLDTSelfDesc	      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.286p

.model small, C

include pmode.inc

DPMI_INT	equ	31H

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		dpmi_detect					      ;
;									      ;
;	DESCRIPTION:	Check if a DPMI server is available		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0	Success 				      ;
;			AX = 1	Failure 				      ;
;			CX = Number of paragraphs of private data required    ;
;			DX = DPMI version				      ;
;			ES:BX = Mode switch routine address		      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;=============================================================================;

dpmi_detect	proc	public uses si di

		; Check is there's an int 2F vector.
		; The ISA POST clears vector 2F.

		xor	ax, ax
		mov	es, ax
		les	ax, es:[4*2FH]
		mov	dx, es
		or	ax, dx			; AX = 0 if no int 2F vector
		jz	dpmi_detect_fail

		; Get DPMI entry point

		mov	ax, 1687H		; Obtain DPMI entry point
		int	2FH
		mov	cx, si			; Overwrite the processor type
		mov	bx, di
		test	ax, ax
		jz	dpmi_detect_exit	; DPMI not loaded
dpmi_detect_fail:
		mov	ax, 1
dpmi_detect_exit:
		ret

dpmi_detect	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		dpmi2prot					      ;
;									      ;
;	DESCRIPTION:	Switch to protected mode using DMPI		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0	    Success				      ;
;			AX != 0     Failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;=============================================================================;

.DATA?

prot_mode_switch dd	?

.CODE

dpmi2prot	proc	public

		; Detect DPMI

		call	dpmi_detect		; Obtain DPMI entry point
		test	ax, ax
		jnz	dpmi2prot_exit		; DPMI not loaded

		mov	word ptr prot_mode_switch, bx
		mov	word ptr prot_mode_switch+2, es

		; Allocate memory for DPMI host

		mov	bx, cx			; Number of paragraphs required
		mov	ah, 48H
		int	21H			; Allocate memory from DOS
		mov	es, ax			; Paragraph address, or error
		mov	ax, 2
		jc	dpmi2prot_exit		; Not enough memory

		; Switch to protected mode

		xor	ax, ax			; Application is 16 bits
		call	prot_mode_switch
		mov	ax, 3
		jc	dpmi2prot_exit		; Can't switch to protected mode
		xor	ax, ax
dpmi2prot_exit:
		ret

dpmi2prot	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetFlatDataDesc 				      ;
;									      ;
;	DESCRIPTION:	Get a descriptor for the flat 4 GB segment	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = Requested selector 			      ;
;			AX = 0 if Failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;=============================================================================;

.DATA?

flat_desc	dw	?			; Initialized as 0

.CODE

GetFlatDataDesc proc	public

		mov	ax, flat_desc
		test	ax, ax
		jnz	GotFlatDataDesc

		; Create an LDT descriptor

		mov	cx, 1
		xor	ax, ax			; Allocate LDT Descriptor
		int	DPMI_INT
		jnc	@F
ErrFlatDataDesc:
		xor	ax, ax
		jmp	GotFlatDataDesc
@@:
		mov	bx, ax			; BX = Selector to update
		mov	cx, -1			; CX:DX = 0FFFFFFFFH
		mov	dx, cx
		mov	ax, 0008H		; Set segment limit
		int	DPMI_INT
		mov	ax, bx			; Restore the selector
		jc	ErrFlatDataDesc

		mov	flat_desc, ax		; Save it for next time
GotFlatDataDesc:
		ret

GetFlatDataDesc endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetLDTSelfDesc					      ;
;									      ;
;	DESCRIPTION:	Get an LDT descriptor to access the LDT itself	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = Selector					      ;
;			AX = 0 if Failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;=============================================================================;

.DATA?

gdt_reg 	df	?
ldt_sel 	dw	?

.CODE

.386p

sdp		macro	reg:REQ     ;; Cast a pointer to (DESCRIPTOR *)
		exitm	<(DESCRIPTOR ptr [reg])>
		endm

GetLDTSelfDesc	proc	public uses esi edi

		; Get the previously created selector, if any

		mov	ax, ldt_sel
		test	ax, ax
		jnz	ExitSetSelf

		; Else create a selector

		invoke	AllocSelector, ds	; Clone our DS selector
		test	ax, ax
		jz	ExitSetSelf
		mov	di, ax			; Save the selector in DI

		; Get the LDT descriptor from the GDT

		sgdt	gdt_reg 		; Not priviledged! Intel bug!
		sldt	si			; LDT selector = offset in GDT
		movzx	esi, si 		; Convert to 32 bits offset
		add	esi, dword ptr gdt_reg+2; + GDT Base = LDT descr. addr.
		call	GetFlatDataDesc 	; Get the 4 GB access selector
		test	ax, ax			; AX:ESI = GDT base?
		jz	ExitSetSelf

		; And copy it into the newly allocated descriptor

		push	ds
		mov	ds, ax			; DS:ESI = GDT base
		mov	es, ax
		mov	ah, sdp(esi).Base_24_31
		mov	al, sdp(esi).Base_16_23
		shl	eax, 16
		mov	ax, sdp(esi).Base_0_15	; EAX = Linear address of LDT
		mov	cx, di			; The result selector
		and	edi, 0FFF8H		; EDI = Offset in LDT
		add	edi, eax		; EDI = Detination linear addr.
		mov	ax, cx			; The result selector
		mov	ecx, 2
		cld
		rep	movsd [edi], [esi]	; Copy the LDT descriptor
		or	sdp(edi-8).Access_Rights, 70H  ; Adjust the DPL to ours
		pop	ds
		mov	ldt_sel, ax		; Save the result selector

ExitSetSelf:
		ret

GetLDTSelfDesc	endp

.286p

;#############################################################################;
;#############################################################################;
;									      ;
;									      ;
;	Windows compatible functions					      ;
;									      ;
;									      ;
;#############################################################################;
;#############################################################################;

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		AllocSelector					      ;
;									      ;
;	DESCRIPTION:	Allocate a descriptor in the LDT using DPMI	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	WORD sel    Selector for template descriptor, or NUL. ;
;									      ;
;	    ON EXIT:	AX = 0	Failure 				      ;
;			AX != 0 Selector to the allocated descriptor	      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;=============================================================================;

AllocSelector	proc	pascal public uses di, sel:WORD
		local	ASBuf:DESCRIPTOR

		mov	bx, sel
		test	bx, bx
		jz	AllocSelector1
		lea	di, ASBuf
		push	ss
		pop	es
		mov	ax, 0BH
		int	DPMI_INT		; Get Descriptor
		jnc	AllocSelector1
AllocSelectorError:
		mov	ax, 0
		jmp	short AllocSelectorExit
AllocSelector1:
		mov	cx, 1
		mov	ax, 0000H		; Allocate LDT Descriptor
		int	DPMI_INT
		jc	short AllocSelectorError

		test	bx, bx
		jz	AllocSelectorExit
		mov	bx, ax
		mov	ax, 0CH
		int	DPMI_INT		; Set Descriptor
		mov	ax, bx
AllocSelectorExit:
		ret

AllocSelector	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		FreeSelector					      ;
;									      ;
;	DESCRIPTION:	Free a descriptor in the LDT using DPMI 	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	WORD sel    Selector for the descriptor to free.      ;
;									      ;
;	    ON EXIT:	AX = 0	Success 				      ;
;			AX != 0 Selector not freed			      ;
;									      ;
;	REGS ALTERED:	AX, BX						      ;
;									      ;
;=============================================================================;

FreeSelector	proc	pascal public uses di, sel:WORD

		mov	bx, sel
		mov	ax, 1
		int	DPMI_INT		; Free Descriptor
		mov	ax, bx			; Assume failure
		jc	@F
		xor	ax, ax
@@:
		ret

FreeSelector	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetSelectorBase 				      ;
;									      ;
;	DESCRIPTION:	Get the base of a segment			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	DX:AX	32 bits base of the segment		      ;
;				-1 = Invalid selector			      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GetSelectorBase proc	pascal public sel:WORD

		mov	bx, sel
		mov	ax, 6
		int	DPMI_INT
		jnc	@F			; Jump if no error
		sbb	cx, cx			; Error. Return -1L.
		sbb	dx, dx
@@:
		mov	ax, dx
		mov	dx, cx
		ret

GetSelectorBase endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GetSelectorLimit				      ;
;									      ;
;	DESCRIPTION:	Get the limit of a segment			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	DX:AX	32 bits base of the segment		      ;
;				0 = Invalid selector			      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GetSelectorLimit proc	pascal public uses es di, sel:WORD
		local	buf:DESCRIPTOR

		push	ss
		pop	es
		lea	di, buf
		mov	bx, sel
		mov	ax, 0BH
		int	DPMI_INT	    ; Get descriptor
		jnc	@F
		xor	ax, ax
		xor	dx, dx
		jmp	short GSLExit
@@:
		mov	ax, buf.Limit_0_15
		mov	dl, buf.Extra_Rights
		mov	cl, dl
		and	dx, 0FH 	    ; Keep only limit_16_19
		test	cl, 80H 	    ; Granularity?
		jz	GSLExit
		mov	cx, 12		    ; Shift in 12 ones from the right
GSLLoop:
		stc
		rcl	ax, 1
		rcl	dx, 1
		loop	GSLLoop
GSLExit:
		ret

GetSelectorLimit endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		SetSelectorBase 				      ;
;									      ;
;	DESCRIPTION:	Set the base of a segment			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	AX = Selector if success			      ;
;			     0 if failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

SetSelectorBase  proc	pascal public uses di, sel:WORD, base:DWORD

		 mov	ax, 7
		 mov	bx, sel
		 mov	cx, word ptr base+2
		 mov	dx, word ptr base
		 int	DPMI_INT
		 mov	ax, bx		    ; Selector value (Assume success)
		 jnc	@F
		 xor	ax, ax
@@:
		 ret

SetSelectorBase  endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		SetSelectorLimit				      ;
;									      ;
;	DESCRIPTION:	Set the limit of a segment			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	AX = 0	if success				      ;
;			   = -1 if failure (Non standard return value)	      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

SetSelectorLimit proc	pascal public uses di, sel:WORD, limit:DWORD

		 mov	ax, 8
		 mov	bx, sel
		 mov	cx, word ptr limit+2
		 mov	dx, word ptr limit
		 int	DPMI_INT
		 sbb	ax, ax
		 ret

SetSelectorLimit endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GlobalPageLock					      ;
;									      ;
;	DESCRIPTION:	Lock the global memory object in physical memory      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	AX = Lock count of the given region. Always 1 in this ;
;			      implementation.				      ;
;			AX = 0 if failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GlobalPageLock	proc	pascal public uses si di, sel:WORD

		INVOKE	GetSelectorLimit, sel
		mov	si, dx
		mov	di, ax

		INVOKE	GetSelectorBase, sel
		mov	bx, dx
		mov	cx, ax

		mov	ax, 0600H
		int	DPMI_INT	    ; Lock linear region

		sbb	ax, ax		    ; 0 if no error, -1 if error
		inc	ax		    ; 1 if no error, 0 if error
		ret

GlobalPageLock	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GlobalPageUnlock				      ;
;									      ;
;	DESCRIPTION:	Unlock the global memory object in physical memory    ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Protected mode selector			      ;
;									      ;
;	    ON EXIT:	AX = Lock count of the given region. Always 0 in this ;
;			      implementation.				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GlobalPageUnlock proc	pascal public uses si di, sel:WORD

		INVOKE	GetSelectorLimit, sel
		mov	si, dx
		mov	di, ax

		INVOKE	GetSelectorBase, sel
		mov	bx, dx
		mov	cx, ax

		mov	ax, 0601H
		int	DPMI_INT	    ; Unlock linear region

		xor	ax, ax		    ; Assume success
		ret

GlobalPageUnlock endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GlobalDosAlloc					      ;
;									      ;
;	DESCRIPTION:	Allocate a block of memory in the base DOS memory     ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Number of bytes to allocate		      ;
;									      ;
;	    ON EXIT:	AX = Selector of the allocated block		      ;
;			DX = Segment of the allocated block		      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GlobalDosAlloc	proc	pascal public, len:DWORD

		mov	bx, word ptr len
		mov	dx, word ptr len+2

		; Convert bytes to paragraphs
		add	bx, 15
		adc	dx, 0
		mov	cx, 4
@@:
		shr	dx, 1
		rcr	bx, 1
		loop	@B

		mov	ax, 0100H
		int	DPMI_INT	    ; Allocate DOS memory block
		xchg	ax, dx		    ; Move select. to AX and seg. to DX
		jnc	@F		    ; Exit if no error

		xor	ax, ax		    ; Failure
		xor	dx, dx
@@:
		ret

GlobalDosAlloc	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		GlobalDosFree					      ;
;									      ;
;	DESCRIPTION:	Free a block of memory in the base DOS memory	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Selector of memory to free		      ;
;									      ;
;	    ON EXIT:	AX = 0	  Success				      ;
;			   = ARG1 Failure				      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX					      ;
;									      ;
;=============================================================================;

GlobalDosFree	proc	pascal public, sel:WORD

		mov	dx, sel
		mov	ax, 0101H
		int	DPMI_INT	    ; Free DOS memory block
		mov	ax, dx		    ; Assume failure
		jc	@F
		xor	ax, ax
@@:
		ret

GlobalDosFree	endp

END
