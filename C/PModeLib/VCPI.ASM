	page	, 132
;*****************************************************************************;
;									      ;
;	FILENAME:	vcpi.asm					      ;
;									      ;
;	CONTENTS:	VCPI detection and management routines. 	      ;
;									      ;
;	Author: 	Jean-Francois Larvoire			94/01/18      ;
;									      ;
;	USAGE:		Example:					      ;
;									      ;
;			_disable();					      ;
;			error = vm2prot();				      ;
;			if (error) {_enable(); fail();} 		      ;
;			/* Access protected memory */			      ;
;			prot2vm();					      ;
;			_enable();					      ;
;									      ;
;	Notes:		All routimes assume they're running at least on a 386.;
;			VCPI can only be implemented on a 386 or better       ;
;			processor anyway. It's the responsability of the      ;
;			calling program to verify it beforehand.	      ;
;									      ;
;			This module is compatible with all C memory models.   ;
;			Just reassemble it with a different .model directive. ;
;			There are some restrictions in large code models      ;
;			though. Read the routines headers for more details.   ;
;									      ;
;			Make sure to never load any segment register between  ;
;			calls to vm2prot and prot2vm. This is especially      ;
;			difficult in large memory models.		      ;
;									      ;
;			Accessing physical memory can be done most easily     ;
;			with pagination turned off. Tests have proved that    ;
;			it can be safely turned off temporarily. Of course    ;
;			all interrupts and NMIs must be disabled.	      ;
;									      ;
;			WARNING: Some routine may be dangerous for the health ;
;			of your PC. Please read carefully all notes in the    ;
;			headers of the functions you use.		      ;
;									      ;
;	History:							      ;
;	  95/02/15 JFL	Moved the test code to VCPITST.ASM.		      ;
;			Changed back to small memory model.		      ;
;	  95/11/16 JFL	Modified the PM and RM callbacks to have a WORD       ;
;			 argument and a WORD return code.		      ;
;	  97/07/17 JFL	Use small model, not tiny model for the libraries.    ;
;									      ;
;      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.model		small, C

;*****************************************************************************;
;									      ;
;			     Constants and macros			      ;
;									      ;
;*****************************************************************************;

DOS		equ	21H	    ; MS-DOS API interrupt
EMM		equ	67H	    ; Expanded Memory Manager API interrupt

PG		equ	80000000H   ; Paging bit of CR0

DESC	struc				; Protected mode segment descriptor
    lim_0_15	dw	0		     ; Limit bits (0..15)
    bas_0_15	dw	0		     ; Base bits (0..15)
    bas_16_23	db	0		     ; Base bits (16..23)
    access	db	0		     ; Access byte
    gran	db	0		     ; Granularity byte
    bas_24_31	db	0		     ; Base bits (24..31)
desc	ends

sTSS	struc			; TSS contents
    BLINK	dw	0
		dw	0
    ESP0	dd	0
    SS0		dw	0
		dw	0
    ESP1	dd	0
    SS1		dw	0
		dw	0
    ESP2	dd	0
    SS2 	dw	0
		dw	0
    rCR3	dd	0
    rEIP	dd	0
    rEFLAGS	dd	2
    rEAX	dd	0
    rECX	dd	0
    rEDX	dd	0
    rEBX	dd	0
    rESP	dd	0
    rEBP	dd	0
    rESI	dd	0
    rEDI	dd	0
    rES		dw	0
		dw	0
    rCS 	dw	0
		dw	0
    rSS 	dw	0
		dw	0
    rDS 	dw	0
		dw	0
    rFS		dw	0
		dw	0
    rGS		dw	0
		dw	0
    rLDT	dw	0
		dw	0
    TBIT	dw	0
    IOMAP	dw	-1
sTSS	ends

MOVCS		macro	value		; Load a value into the CS register
		local	loc
		db	0EAH		; Jump far OpCode
		dw	offset loc
		dw	value
loc:
		endm

POPCS		macro			; Pop the CS register off the stack
		local	loc
		push	offset loc
		retf
loc:
		endm

;*****************************************************************************;
;									      ;
;				 Data segment				      ;
;									      ;
;*****************************************************************************;

.data

My_TSS		segment para public use16 'DATA'    ; Task State Segment
		sTSS	<>
My_TSS		ends

DGROUP		group	My_TSS

My_GDT		label	byte
NULL_DES	DESC	<>				; Null descriptor
DGROUP_DES	DESC	<0FFFFH, 0, 0, 92H, 0, 0>	; DGROUP descriptor
FLAT_DES	DESC	<0FFFFH, 0, 0, 92H, 0CFH, 0>	; 32GB flat data segment
FLAT_DES2	DESC	<0FFFFH, 0, 0, 92H, 0CFH, 0>	; 32GB flat data segment
CODE_DES	DESC	<0FFFFH, 0, 0, 9AH, 0, 0>	; Code descriptor
VCPI1_DES	DESC	<>				; First VCPI descriptor
VCPI2_DES	DESC	<>				; 2nd VCPI descriptor
VCPI3_DES	DESC	<>				; 3rd VCPI descriptor
TSS_DES 	DESC	<size sTSS,,,89H,,>		; Task State Segment
IF @CodeSize EQ 1
CALLER_DES	DESC	<0FFFFH, 0, 0, 9AH, 0, 0>	; Caller's code descript
ENDIF
size_My_GDT	equ	$ - My_GDT

My_GDT_fword	label	fword			; Eprom GDT
		dw	size_My_GDT - 1 		; Limit
		dd	?				; Base linear address

NULL_IDT_fword	label	fword			; Null IDT for protected mode
		dw	0				; Limit 0 ==> No ints
		dd	0				; Base 0

Real_IDT_fword	label	fword			; Real mode IDT
		dw	3FFH				; Limit: 256 * 4 bytes
		dd	0				; Base: 0

emm_name	db	'EMMXXXX0', 0
emm_name2	db	'EMMQXXX0', 0
ems_page_handle dw	-1			; Allocated EMS page handle
vcpi_entry	label	fword			; Protected mode 32 bits address
		dd	?
		dw	VCPI1_DES - My_GDT
unused_page	dw	?			; Offset of 1st unused page

switch_structure label	byte		; Structure used to switch to prot.mode
ss_cr3		dd	?			; CR3 register
ss_gdt_base	dd	?			; GDT fword linear address
ss_idt_base	dd	?			; IDT fword linear address
ss_ldtr 	dw	0			; LDT selector
ss_tr		dw	TSS_DES - My_GDT	; TR selector
ss_eip		dd	?			; Offset to jump to
ss_cs		dw	CODE_DES - My_GDT	; Segment to jump to

prepare_done	dw	0		; != 0 if vcpi_prepare succeeded

.data?

My_CS		dw	?
My_DS		dw	?
My_ESP		dd	?		; Temp. storage for stack pointer
vm_flags	dw	?		; Temp. storage for virtual mode flags

		public	page_directory
page_directory	dw	?		; Beginning of 4KB page table buffer
page_table0	dw	?		; Beginning of 4KB page table buffer

pages		db	(3 * 4096) dup (?) ; Room for 2 page-aligned page tables

prot_cr0	dd	?		; Protected mode CR0
prot_cr3	dd	?		; Protected mode CR3
prot_gdtr	df	?		; Protected mode GDTR
prot_idtr	df	?		; Protected mode IDTR

;*****************************************************************************;
;									      ;
;				 Code segment				      ;
;									      ;
;*****************************************************************************;

.386p

.code

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		vcpi_detect					      ;
;									      ;
;	DESCRIPTION:	Test if a VCPI server is present		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0		VCPI server detected		      ;
;			AX = -1 	No VCPI server detected 	      ;
;									      ;
;	NOTES:		It is not possible to call function DE00 directly     ;
;			because interrupt vector 67H is undefined if there    ;
;			is no EMS or VCPI server installed.		      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX					      ;
;									      ;
;=============================================================================;

vcpi_detect	proc	public

		; Was this called before?

		mov	dx, ems_page_handle
		cmp	dx, -1			; Is the handle valid?
		jne	emm_enabled		; Jump if it is

		; Check for EMS driver present

		lea	dx, emm_name		; Try the standard name
		call	check_device
		jnc	@F

		lea	dx, emm_name2		; Try the alternate name
		call	check_device		; This is for EMM386.EXE NOEMS
		jc	no_vcpi
		jmp	emm_enabled		; Obviously can't allocate EMS
@@:
		; Now allocate an EMS page to make sure the EMM is enabled

		mov	bx, 1
		mov	ah, 43H 		; Allocate 1 EMS page
		int	EMM
		cmp	ah, 0
		jne	no_vcpi 		; Jump if error
		mov	ems_page_handle, dx
emm_enabled:
		; Now make the VCPI detection call

		mov	ax, 0DE00H		; Is VCPI supported?
		int	EMM
		cmp	ah, 0
		je	@F			; Jump if VCPI supported
no_vcpi:
		mov	ax, -1
		jmp	short exit_detect
@@:
		; This is it, VCPI is available!

		xor	ax, ax			; Flag success

		; Exit
exit_detect:
		ret

vcpi_detect	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		vcpi_cleanup					      ;
;									      ;
;	DESCRIPTION:	Cleanup the VCPI ressources allocated so far	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	None						      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX					      ;
;									      ;
;=============================================================================;

vcpi_cleanup	proc	public

		mov	dx, ems_page_handle
		cmp	dx, -1			; Is the handle valid?
		je	@F
		mov	ah, 45H 		; Free EMS handle
		int	EMM
@@:
		ret

vcpi_cleanup	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		vcpi_prepare					      ;
;									      ;
;	DESCRIPTION:	Prepare entry in protected mode using VCPI	      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0		Done				      ;
;			AX = -1 	No VCPI server detected 	      ;
;			AX > 0		AL = Error returned by VCPI in AH     ;
;					AH = 0				      ;
;									      ;
;	NOTES:		Assumes vcpi_detect has already been called.	      ;
;									      ;
;			This function needs only be called once, for multiple ;
;			switches back and forth between virtual and protected ;
;			modes.						      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX					      ;
;									      ;
;=============================================================================;

vcpi_prepare	proc	near	uses si di es

		push	ds
		pop	es			; Make sure ES=DS

		; Was this called before?

		mov	ax, prepare_done
		test	ax, ax			; Is the handle valid?
		jnz	prepare_success 	; Jump if it is

		; Allocate the page directory and table 0 in 4K physical pages

		mov	ax, offset DGROUP:pages
		mov	bx, ax
		call	get_linear_address
		and	ax, 0FFFH
		jz	@F			; Jump if already aligned
		sub	bx, ax			; Round to previous 4K boundary
		add	bx, 1000H		; Round to next 4K boundary
@@:
		mov	page_directory, bx
		add	bx, 1000H
		mov	page_table0, bx

		; Get VCPI protected mode interface

		mov	di, page_table0 	; Actually ES:DI
		mov	si, offset VCPI1_DES	; Actually DS:SI
		mov	ax, 0DE01H
		int	EMM
		cmp	ah, 0
		je	@F
vcpi_error:
		mov	al, ah			; Move the VCPI error to AX
		xor	ah, ah
		jmp	exit_prepare
@@:
		mov	unused_page, di 	; First page we can change
		mov	dword ptr vcpi_entry, ebx

		; Invalidate the rest of page table 0

		mov	cx, page_table0
		add	cx, 1000H		; Offset of the end of the table
		sub	cx, di			; Number of bytes left unused
		shr	cx, 2			; Convert # bytes to # dwords
		jcxz	@F			; Skip if table already full
		xor	eax, eax		; Bit 0 = 0 ==> Invalid entry
		rep	stosd
@@:
		; Prepare my GDT and related structures

		; Compute My_GDT linear address
		mov	ax, offset My_GDT
		call	get_linear_address
		mov	dword ptr My_GDT_fword + 2, eax

		; Compute and set the TSS segment linear base
		; mov	  ax, seg My_TSS	; Best but illegal in Tiny model
		mov	ax, ds			; Compute seg My_TSS
		mov	bx, offset DGROUP:My_TSS
		shr	bx, 4
		add	ax, bx
		mov	bx, offset TSS_DES
		call	Set_descriptor_base

		; Compute and set the CODE16 segment linear base
		mov	ax, cs
		mov	bx, offset CODE_DES
		call	Set_descriptor_base

		; Compute and set the DGROUP segment linear base
		mov	ax, ds
		mov	bx, offset DGROUP_DES
		call	Set_descriptor_base

		; Prepare the switch_structure

		; Prepare CR3 to point to the page directory
		mov	ax, page_directory
		call	get_linear_address
		call	get_physical_address
		mov	ss_cr3, eax

		; Initialize the page directory with only table0
		mov	ax, page_table0
		call	get_linear_address
		call	get_physical_address
		or	ax, 67H 		; Dirty, acc, User, R/W, Present
		mov	di, page_directory
		stosd				; First page directory entry
		mov	cx, 03FFH
		xor	eax, eax
		rep	stosd			; All others marked invalid

		; Compute My_GDT_fword linear address
		mov	ax, offset My_GDT_fword
		call	get_linear_address
		mov	ss_gdt_base, eax

		; Compute My_IDT_fword linear address
		mov	ax, offset NULL_IDT_fword
		call	get_linear_address
		mov	ss_idt_base, eax

		; Done
		mov	prepare_done, 1

prepare_success:
		xor	ax, ax			; Flag success
exit_prepare:
		ret

vcpi_prepare	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		vm2prot 					      ;
;									      ;
;	DESCRIPTION:	Switch from Virtual Mode to Protected mode using VCPI ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0		Done. Interrupts are OFF. No IDT.     ;
;			AX = -1 	No VCPI server detected 	      ;
;			AX > 0		AL = Error returned by VCPI in AH     ;
;									      ;
;	NOTES:		Keep interrupts off while in protected mode.	      ;
;									      ;
;			Use prot2vm to return to virtual mode.		      ;
;									      ;
;			This has to be a near routine since the segment       ;
;			address on the stack would not mean the same thing    ;
;			for the call and the return, which are in different   ;
;			processor modes.				      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX, ES, FS, GS				      ;
;									      ;
;=============================================================================;

vm2prot 	proc	near public

		pushf
		pop	[vm_flags]		; Save flags before PM entry

		; Look for VCPI

		call	vcpi_detect
		test	ax, ax
		jnz	exit			; Exit if not detected

		; Prepare the VCPI structures and variables

		call	vcpi_prepare
		test	ax, ax
		jnz	exit			; Exit if vcpi error

		; Save Segments

		mov	My_CS, cs
		mov	My_DS, ds
		mov	My_ESP, esp		; Save the stack itself

		; Switch to protected mode

		mov	ss_eip, offset protentry ; Set the entry point

		mov	ax, offset switch_structure
		call	get_linear_address
		mov	esi, eax
		mov	ax, 0DE0CH
		cli				; Disable interrupt
		int	EMM			; Make the big jump
						; Failed to switch to prot.mode
		mov	al, ah			; Move the VCPI error to AX
		xor	ah, ah
		jmp	short exit

protentry:	; Entry point in protected mode
		; Interrupts are off. All selectors are invalid.

		; Load valid values into all selectors

		MOVCS	<CODE_DES - My_GDT>
		mov	ax, DGROUP_DES - My_GDT
		mov	ds, ax
		mov	es, ax
		mov	fs, ax
		mov	gs, ax
		mov	ss, ax			; Assume SS=DS
		mov	esp, My_ESP

		; Done

		xor	ax, ax		; Flag success
exit:
		ret

vm2prot 	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		prot2vm 					      ;
;									      ;
;	DESCRIPTION:	Switch from Protected Mode to Virtual Mode using VCPI ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0		Done				      ;
;			AX != 0 	Error				      ;
;									      ;
;	NOTES:		This has to be a near routine since the segment       ;
;			address on the stack would not mean the same thing    ;
;			for the call and the return, which are in different   ;
;			processor modes.				      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX, ES, FS, GS				      ;
;									      ;
;=============================================================================;

prot2vm 	proc	near public

		cli				; Make sure interrupts are off

		; Save the stack itself

		mov	My_ESP, esp		; Note: SS=DS=DGROUP

		; Return to virtual 86 mode

		xor	eax, eax
		push	eax			; GS
		push	eax			; FS
		mov	ax, My_DS
		push	eax			; DS
		push	eax			; ES
		push	eax			; SS
		push	My_ESP			; ESP
		pushfd				; Eflags
		mov	ax, My_CS		; .code segment name
		push	eax			; CS
		mov	ax, offset vmentry
		push	eax			; EIP

		mov	ax, 0DE0CH
		call	vcpi_entry		; Make the big jump
						; Failed to switch to virt.mode
		mov	al, ah			; Move the VCPI error to AX
		xor	ah, ah
		jmp	short exit

vmentry:
		xor	ax, ax			; ~~jfl 950911 Before flag resto
		push	[vm_flags]
		popf				; Restore flags before PM entry
exit:
		ret

prot2vm 	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		VCPI2PMAndCallBack				      ;
;									      ;
;	DESCRIPTION:	Switch from VM to PM and back. Call a routine in PM.  ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Routine address				      ;
;									      ;
;	    ON EXIT:	AX = 0		Done				      ;
;			AX != 0 	Error				      ;
;									      ;
;	NOTES:		Especially useful in large memory model, where	      ;
;			vm2prot and prot2vm cannot be called directly.	      ;
;									      ;
;	The callback routine...
;	- Receives a pointer to the GDT in BX and on stack.
;	  The descriptors are:
;	    0000 = NULL selector. Reserved.
;	    0008 = Selector for DGROUP. Do not change.
;	    0010 = Selector for a flat 4 GB segment. May be updated.
;	    0018 = Selector for a flat 4 GB segment. May be updated.
;	    0020 and up = Reserved. Do not change.
;	- Must preserve EBP, ESI, EDI
;	- Gets all segment registers loaded with valid PM selectors
;	- Must not change the contents of CS, unless it's own code descriptors
;	   are set in the GDT.
;	- Runs with interrupts disabled.
;	- Must not execute any "int NN" instruction. This would cause a
;	   processor shutdown. Same thing for any exception (ex: divide by 0).
;									      ;
;			The first four GDT selectors are the same as those    ;
;			in the RM2PMAndCallBack routine GDT.		      ;
;									      ;
;	REGS ALTERED:	TBD						      ;
;									      ;
;	HISTORY:							      ;
;									      ;
;	 1995/11/16 JFL Changed the parameters to be compatible with the      ;
;			 callback of RM2PMAndCallBack.			      ;
;	 1995/11/22 JFL Discard the callback result if the pointer is NULL.   ;
;									      ;
;=============================================================================;

TPROC	TYPEDEF PROTO
PPROC	TYPEDEF PTR TPROC

VCPI2PMAndCallBack	proc	public, callback:PPROC, \
					wParm:WORD, pwRet:PTR WORD
		call	vm2prot
		test	ax, ax
		jnz	exit_callback

		IF	@CodeSize EQ 1
		mov	ax, word ptr callback+2
		mov	bx, offset CALLER_DES
		call	Set_descriptor_base

		mov	word ptr callback+2, CALLER_DES - My_GDT
		ENDIF

		mov	ax, wParm
		push	ax		    ; Support both _cdecl & _fastcall
		IF	@DataSize EQ 1
		push	ds
		ENDIF
		mov	bx, offset My_GDT   ; GDT offset in BX for _fastcall
		push	bx		    ; and on stack for _cdecl functions

		call	callback
		IF	@DataSize EQ 1
		les	bx, pwRet
		mov	cx, es		    ; ~~jfl 95/11/22 Discard if NUL ptr
		or	cx, bx
		jz	@F
		mov	WORD ptr es:[bx], ax
@@:
		ELSE
		mov	bx, pwRet
		test	bx, bx		    ; ~~jfl 95/11/22 Discard if NUL ptr
		jz	@F
		mov	WORD ptr [bx], ax
@@:
		ENDIF

		pop	bx		    ; Cleanup the stack
		IF	@DataSize EQ 1
		pop	bx
		ENDIF
		pop	bx

		call	prot2vm
exit_callback:
		ret

VCPI2PMAndCallBack	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		vm2real 					      ;
;									      ;
;	DESCRIPTION:	Switch from Virtual Mode to Real Mode using VCPI      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = 0		Done				      ;
;			AX != 0 	Error				      ;
;									      ;
;	NOTES:		This routine should not exist. Use it with caution.   ;
;			It is written in the VCPI norm that we should not do  ;
;			this. And they're right: The PC will eventually crash.;
;									      ;
;			This only works because of a weakness of VCPI,	      ;
;			which allows clients to switch to protected mode in   ;
;			priviledged mode. We abuse from our power, and switch ;
;			back to real mode. However the EMM driver is still    ;
;			there, and it assumes that it is still running in     ;
;			virtual 86 mode. So expect it to crash as soon as it  ;
;			gets a chance to run.				      ;
;									      ;
;			This routine should only be called by applications    ;
;			that expect to reboot the PC very soon. They should   ;
;			make no DOS or BIOS calls once in real mode, since    ;
;			the EMM driver may intercept them.		      ;
;									      ;
;			Real mode is entered with interrupts off.	      ;
;									      ;
;			This routine only works if run in the system DOS box, ;
;			where linear and physical addresses are equal.	      ;
;			This is not a problem with EMM386/Windows since VCPI  ;
;			is only supported in the system DOS box anyway.       ;
;									      ;
;			This routine is only compatible with tiny and small   ;
;			memory models.					      ;
;									      ;
;	REGS ALTERED:	BX, CX, DX					      ;
;									      ;
;	HISTORY:							      ;
;	 1995/09/11 JFL Do not restore the interrupts before returning.       ;
;									      ;
;=============================================================================;

vm2real 	proc	public

		push	cs			; Save the V86 segment registers
		push	ss
		push	ds
		push	es
		push	fs
		push	gs

		; Switch to priviledged protected mode

		call	vm2prot
		test	ax, ax
		jnz	fail			; Exit if not detected

		; Save critical registers for return to protected mode

		mov	eax, cr0
		mov	prot_cr0, eax
		sidt	prot_idtr
		sgdt	prot_gdtr

		; Turn off pagination, and switch back to real mode

		mov	eax, cr0
		and	eax, NOT 80000009H	; Clear PG, TS & PE bits
		mov	cr0, eax
		jmp	$+2		; Clear prefetch queue

		; Flush the address translation caches as a double precaution

		mov	eax, cr3
		mov	prot_cr3, eax
		mov	cr3, eax		; Loading CR3 flushes them

		; Restore real mode segment registers

		pop	gs
		pop	fs
		pop	es
		pop	ds
		pop	ss			; Note: ESP is already good
		POPCS

		; Prepare real mode descriptor tables

		lidt	Real_IDT_fword	; Normal real mode table

		; Done

		xor	ax, ax		; Flag success
exit:
		ret

fail:
		add	sp, 12		; Cleanup the stack
		ret

vm2real 	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		VCPI2RMAndCallBack				      ;
;									      ;
;	DESCRIPTION:	Switch from VM to RM and back. Call a routine in RM.  ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ARG1 = Routine address				      ;
;									      ;
;	    ON EXIT:	AX = 0		Done				      ;
;			AX != 0 	Error				      ;
;									      ;
;	NOTES:		See warning in vm2real! 			      ;
;									      ;
;	The callback routine...
;	- Must preserve BP, SI, DI, the high halves of 32 bits registers,
;	   GDTR, LDTR, TR, etc...
;	- Runs with interrupts disabled.
;									      ;
;	REGS ALTERED:	TBD						      ;
;									      ;
;	HISTORY:							      ;
;									      ;
;	 1995/11/16 JFL Added a callback argument and return value.	      ;
;	 1995/11/22 JFL Discard the callback result if the pointer is NULL.   ;
;	 1995/11/29 JFL Reload 32-bits GDT & IDT bases. The default is 24-bits;
;									      ;
;=============================================================================;

VCPI2RMAndCallBack	proc	public, \
				callback:PPROC, wParam:WORD, pRetVal:PTR WORD
		pushf
		cli				; vm2real does not clear ints.

		; Switch to real mode

		call	vm2real
		test	ax, ax
		jnz	exit			; Exit if not switched

		; Call the user callback

		mov	ax, wParam
		push	ax
		call	callback
		pop	bx			; Cleanup the arguments
		push	ax			; Save the return code

		; Return to protected mode

		mov	eax, prot_cr3
		mov	cr3, eax
		mov	eax, prot_cr0
		mov	cr0, eax		; Reenable PM and pagination
		jmp	$+2

		; Restore system registers, in case they were destroyed,
		;   such as when returning from hybernation.

		db	66h			; Force loading a 32-bits base
		lidt	prot_idtr
		db	66h			; Force loading a 32-bits base
		lgdt	prot_gdtr
		lldt	ss_ldtr 		; Only accessible in PM
		and	TSS_DES.access, NOT 2	; Clear the busy bit in TSS des.
		ltr	ss_tr			; Only accessible in PM

		; Reload valid values into all selectors

		MOVCS	<CODE_DES - My_GDT>
		mov	ax, DGROUP_DES - My_GDT
		mov	ds, ax
		mov	es, ax
		mov	fs, ax
		mov	gs, ax
		mov	ss, ax			; Assume SS=DS

		; Save the return code

		pop	ax
		mov	bx, pRetVal
		test	bx, bx		; ~~jfl 95/11/22 Discard if NULL ptr
		jz	@F
		mov	word ptr [bx], ax
@@:
		; Return to V86 mode

		call	prot2vm

		; Done
exit:
		popf
		ret

VCPI2RMAndCallBack	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		Set_descriptor_base				      ;
;									      ;
;	DESCRIPTION:	Set the linear base address of a segment descriptor   ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ax = Real mode segment value			      ;
;			ds:bx = Address of the descriptor to update	      ;
;									      ;
;	    ON EXIT:	None						      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

Set_descriptor_base	proc	near

		push	eax
		movzx	eax, ax 	; Convert to 32 bits
		shl	eax, 4
		mov	[bx.bas_0_15], ax
		shr	eax, 16
		mov	[bx.bas_16_23], al
		mov	[bx.bas_24_31], ah
		pop	eax
		ret

Set_descriptor_base	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		get_physical_address				      ;
;									      ;
;	DESCRIPTION:	Get the physical address of a data in the 1st MB      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	eax = Linear address				      ;
;									      ;
;	    ON EXIT:	eax = Physical address				      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

get_physical_address	proc	near

		push	ecx
		push	edx

		shr	eax, 12
		mov	cx, ax
		mov	ax, 0DE06H		; Get physical address
		int	EMM
		mov	eax, edx

		pop	edx
		pop	ecx
		ret

get_physical_address	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		get_linear_address				      ;
;									      ;
;	DESCRIPTION:	Get the linear address of a data		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	ax = Offset in real-mode DGROUP 		      ;
;									      ;
;	    ON EXIT:	eax = Linear address				      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

get_linear_address	proc	near

		push	ebx
		movzx	ebx, ax 	    ; Offset, extended to 32 bits
		mov	ax, ds		    ; Segment
		movzx	eax, ax
		shl	eax, 4		    ; Segment linear base address
		add	eax, ebx	    ;  + offset = linear base
		pop	ebx
		ret

get_linear_address	endp

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		check_device					      ;
;									      ;
;	DESCRIPTION:	Check if a DOS device is present		      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	dx = Offset of device name			      ;
;									      ;
;	    ON EXIT:	Carry clear: Success				      ;
;			Carry set: Error, device not found		      ;
;									      ;
;	REGS ALTERED:	None						      ;
;									      ;
;=============================================================================;

check_device	proc	near

		mov	ax, 3D00H
		int	DOS
		jc	check_exit		; Can't open file
		mov	bx, ax			; MS-DOS file handle

		mov	ax, 4400H
		int	DOS			; Get device information
		jc	check_fail
		test	dx, 80H 		; Is it a file or a device?
		jz	check_fail		; Jump if file

		mov	ax, 4407H
		int	DOS			; Check output status
		jc	check_fail
		cmp	al, 0FFH
		jne	check_fail		; Jump if device not ready

		mov	ah, 3EH
		int	DOS			; Close file
		clc				; Flag success
		jmp	short check_exit
check_fail:
		mov	ah, 3EH
		int	DOS			; Close file
		stc
check_exit:
		ret

check_device	endp

end
