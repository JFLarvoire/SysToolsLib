	PAGE	,132
	TITLE	Protected Mode access routines, part of PMODE.LIB

;*****************************************************************************;
;									      ;
;   FileName:	    RM2PM.ASM						      ;
;									      ;
;   Contents:	    Switch from real mode to protected mode and back	      ;
;									      ;
;   Notes:	    							      ;
;		    							      ;
;   History:								      ;
;    1989/07/21	JFL Initial implementation as part of PROT.ASM		      ;
;    1995/02/09	JFL Rewritten as a subroutine with a callback.		      ;
;    1995/02/17	JFL Changed the return value to 0=Success / Else error to     ;
;		     be consistent with VCPI's VCPI2PMAndCallBack.	      ;
;    1995/09/18	JFL Added the second argument.				      ;
;		    Changed the calling convention to _cdecl.		      ;
;		    Preserve the initial interrupt flag state.		      ;
;		    Preserve the contents of the GDT register.		      ;
;    1995/11/16	JFL Changed the callback type to be the same as that	      ;
;		     of VCPI2PMAndCallBack				      ;
;    1995/11/21	JFL Discard the callback result if NULL pointer.	      ;
;    1997/07/10	JFL Added a code segment descriptor.			      ;
;		    Load the code and data selectors with valid values	      ;
;		    while in protected mode. Restore RM values afterwards.    ;
;    1997/09/05	JFL Changed again macros POPCS and MOVCS to be compatible     ;
;		     with both the small and tiny memory models.	      ;
;    1998/03/05	JFL Restore A20 to its initial state.			      ;
;    1998/05/15 JFL Corrected case in function name.			      ;
;									      ;
;      (c) Copyright 1989-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286p

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include 	adefine.inc	; All assembly language definitions

DESC	struc
    lim_0_15	dw	0	; Limit bits (0..15)
    bas_0_15	dw	0	; Base bits (0..15)
    bas_16_23	db	0	; Base bits (16..23)
    access	db	0	; Access byte
    gran	db	0	; Granularity byte
    bas_24_31	db	0	; Base bits (24..31)
DESC	ends

; Note: The following two macros could more simple, if it were not for the
;	problem of the code group which is "CGROUP group RESID, _TEXT" in
;	small memory model, or DGROUP itself in tiny memory model.
;	Because of this, we can't write an offset directive that is the same
;	for both cases. Instead, we need to delay the offset calculation to
;	run time. Hence the apparently overcomplex macros.

MOVCS		macro	value		; Load a value into the CS register
		local	loc1, loc2
		push	value
		jmp	short loc2
loc1:
		retf
loc2:
		call	loc1
		endm

POPCS		macro			; Pop the CS register off the stack
		local	loc1, loc2
		jmp	short loc2
loc1:
		retf
loc2:
		call	loc1
		endm

;-----------------------------------------------------------------------------;
;									      ;
;   Global data 							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.data

Temp_GDT	label	byte
NULL_DES	DESC	<>				; Null descriptor
DGROUP_DES	DESC	<0FFFFH, 0, 0, 92H, 0, 0>	; 64KB data segment at 0
FLAT_DES	DESC	<0FFFFH, 0, 0, 92H, 0CFH, 0>	; 4GB flat data segment
FLAT2_DES	DESC	<0FFFFH, 0, 0, 92H, 0CFH, 0>	; 4GB flat data segment
CODE_DES	DESC	<0FFFFH, 0, 0, 9AH, 0, 0>	; 64KB code segment at 0
size_Temp_GDT	equ	$ - Temp_GDT

Temp_GDT_pword	label	fword			; Eprom GDT
		dw	size_Temp_GDT - 1		; Limit
		dd	?				; Base linear address

NULL_IDT_pword	label	fword			; Null IDT for protected mode
		dw	0				; Limit 0 ==> No ints
		dd	0				; Base 0

Real_IDT_pword	label	fword			; Real mode IDT
		dw	3FFH				; Size = 256 * 4 bytes
		dd	0				; Base: 0

		.data?

Old_GDT_pword	df	?

PAGE

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

EXTERNCFASTPROC is_a20_enabled
EXTERNCFASTPROC enable_a20
EXTERNCFASTPROC disable_a20

page
;===FUNCTION HEADER=========================================================
;
; NAME: RM2PMAndCallBack
;
; DESCRIPTION:
;	Switch from real mode to protected mode and back. Call callback in PM.
;
; NOTE:
;	Using this routine will trigger a general protection fault if the
;	 processor is in Virtual 86 mode. This is because it uses priviledged
;	 instructions for the mode switch. Before attempting to use it, make
;	 sure the processor is in Real Mode. Else use DPMI or VCPI functions
;	 to switch modes.
;
;	This routine manages the A20 line automatically, using XMS if available.
;
;	The callback routine...
;	- Receives a pointer to the GDT in BX. The descriptors are:
;	    0000 = NULL selector. Reserved.
;	    0008 = Selector for DGROUP. Do not change.
;	    0010 = Selector for a flat 4 GB segment. May be updated.
;	    0018 = Selector for a flat 4 GB segment. May be updated.
;	  The last two descriptors are only valid on a 80386 and later proc.
;	- Must preserve BP, SI, DI
;	- Must not push and pop segment registers. They can still be used to
;	   access the same segment they were addressing in real mode though.
;	   This is because these segment registers still contain their real
;	   mode value, which is meaningless in protected mode. But the hidden
;	   part of these registers contains a valid descriptor for the segment
;	   they were addressing in RM, and thus they continue to be accessible
;	   in PM.
;	- Must not change the contents of CS. No far calls or far returns.
;	   Same reason as for the other segment registers.
;	- If FS or GS is modified, it MUST be reloaded with the DGROUP
;	   selector before returning to real mode. This restores limits and
;	   access rights compatible with real mode, and contrary to DS and ES
;	   it is not done automatically by RM2PMAndCallBack.
;	- Runs with interrupts disabled.
;	- Must not execute any "int NN" instruction. This would cause a
;	   processor shutdown. Same thing for any exception (ex: divide by 0).
;
;	The first four GDT selectors are the same as those in the
;	 VCPI2PMAndCallBack routine GDT.
;
; ENTRY:
;	ARG1 = pointer to the address of the routine to call in PM.
;	ARG2 = WORD parameter to be passed to the callback.
;	ARG3 = Where to store the callback result. NULL=discard.
;
; EXIT:
;	AX = 0	Success
;	     1	A20 switch failure
;
; MODIFIED:
;	AX, CX, DX, ES
;
; HISTORY:
;  1995/09/18 JFL   Added the second argument.
;		    Changed the calling convention to _cdecl.
;		    Preserve the initial interrupt flag state.
;		    Preserve the contents of the GDT register.
;  1995/11/16 JFL   Changed the callback type to be the same as that
;		     of VCPI2PMAndCallBack
;  1998/03/05 JFL   Restore A20 to its initial state.
;
;===END HEADER==============================================================

RM2PMAndCallBack proc	C public uses ds, \
			callback:WORD, param:WORD, pRetVal:PTR WORD
		local	last_a20:BYTE

		; Compute the CODE segment linear address
		mov	ax, cs
		xor	dx, dx
		mov	cx, 4
@@:
		shl	ax, 1
		rcl	dx, 1
		loop	@B
		mov	CODE_DES.bas_0_15, ax
		mov	CODE_DES.bas_16_23, dl
		; mov	  CODE_DES.bas_24_31, dh  ; Always 0 for 1st MB

		; Compute the DGROUP linear address
		mov	ax, ds
		xor	dx, dx
		mov	cx, 4
@@:
		shl	ax, 1
		rcl	dx, 1
		loop	@B
		mov	DGROUP_DES.bas_0_15, ax
		mov	DGROUP_DES.bas_16_23, dl
		; mov	  DGROUP_DES.bas_24_31, dh  ; Always 0 for 1st MB

		; Compute the Temp_GDT linear address
		add	ax, offset Temp_GDT
		adc	dx, 0
		mov	word ptr Temp_GDT_pword + 2, ax
		mov	word ptr Temp_GDT_pword + 4, dx

		; Turn off all interrupts
		pushf
		cli
		mov	al, 80H
		out	70H, al		; Clear NMIs

		; Clear A20 gate
		call	is_a20_enabled
		mov	last_a20, al
		jnz	@F
		call	enable_a20
		test	ax, ax
		jz	restore_ints
@@:
		; Prepare protected mode descriptor tables
		lidt	NULL_IDT_pword	; Force shutdown on interrupt
		sgdt	Old_GDT_pword
		lgdt	Temp_GDT_pword	; Load GDT descriptor for temp. GDT

		push	cs

		; switch to protected mode
		smsw	ax
		or	ax, 1		; Set PE bit
		lmsw	ax
;		 jmp	 $+2		 ; Clear prefetch queue

		; Load segment registers with PM selectors
		MOVCS	<CODE_DES - Temp_GDT>
		mov	ax, DGROUP_DES - Temp_GDT
		mov	ds, ax

		; Call the callback
		mov	bx, offset Temp_GDT
		mov	ax, param
		push	ax		; Support both _fastcall and _cdecl
		push	bx
		call	callback	; Call the callback
		pop	bx		; Cleanup the stack
		pop	bx

		; Reset ds and es to 64K segment, RW access, etc...
		mov	dx, DGROUP_DES - Temp_GDT
		mov	ds, dx
		mov	es, dx

		; Store the return code ; ~~jfl 95/11/16 Added this section
		mov	bx, pRetVal
		test	bx, bx		; ~~jfl 95/11/22 Discard if NULL ptr
		jz	@F
		mov	WORD ptr [bx], ax
@@:
		; switch back to real mode. Not possible on a 286!
		.386p
		mov	eax, cr0
		and	ax, NOT 1	; Clear PE bit
		mov	cr0, eax
		jmp	$+2		; Clear prefetch queue
		POPCS

		; Prepare real mode descriptor tables
		lidt	Real_IDT_pword	; Normal real mode table
		lgdt	Old_GDT_pword

		; Gate A20
		cmp	last_a20, 0
		jnz	@F
		call	disable_a20
@@:
restore_ints:
		; Restore interrupts
		xchg	al, ah		; AL=0; AH=A20 switch status
		out	70H, al
		popf
		xchg	al, ah		; AX=A20 switch status (0=Fail; 1=OK)

		; Restore regs and return
		xor	al, 1		; 0=Success; 1=A20 failure
		ret

RM2PMAndCallBack endp

END
