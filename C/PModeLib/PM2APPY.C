/*****************************************************************************\
*                                                                             *
*   File name:	    pm2appy.c						      *
*									      *
*   Description:    Execute a 32-bits ring 0 routine in an appy time callback *
*									      *
*   Notes:	    Uses DPMI.						      *
*									      *
*   History:								      *
*    1997/01/10 JFL Created this file					      *
*									      *
*      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "utildef.h"
#include "lodos.h"
#include "pmode.h"

//+-------------------------------------------------------------------------+//
//+				VMM interface.				    +//
//+-------------------------------------------------------------------------+//

// Avoids including VMM.H since this breaks MSVC 1.5's inline assembler

#define _VMMCall(service)   \
	int	20H	    \
	DD(service)

#define _CopyPageTable 0x00010061
#define  Get_Cur_VM_Handle 0x00010001
#define _MapPhysToLinear 0x0001006C
#define  Create_Semaphore 0x00010025
#define  Destroy_Semaphore 0x00010026
#define  Signal_Semaphore 0x00010028
#define  Wait_Semaphore 0x00010027

#define _SHELL_CallAtAppyTime 0x0017000E

#define _VPOWERD_Set_System_Power_State 0x00260005

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PM2AppyAndCallBack					      |
|									      |
|   Description:    Execute a 32-bits ring 0 routine in an appy time callback |
|									      |
|   Parameters:     PRING0CALLBACK pCB	Address of the 32-bits Ring 0 callback|
|		    void *pParam	Parameter list to pass to the callback|
|		    DWORD *pdwRetVal	Where to store the returned value     |
|									      |
|   Returns:	    0 = Callback ran; !0 = Switch failed or schedule failed.  |
|									      |
|   Notes:	    pParam is modified to point to the high VM image.	      |
|		    It is not possible to pass a constant by casting it to    |
|		     a (void *) since it will be modified.		      |
|		    Instead, pass a pointer to the constant in your program.  |
|									      |
|		    Does not use a semaphore for synchronization as this is   |
|		    making use of non-reentrant VMM functions, and causes     |
|		    Windows to crash eventually.			      |
|									      |
|   History:								      |
|									      |
|    1997/01/10 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

// Friendly names for VSleep32 stack frame variables
#define dwParam32 [at_ebp+12]

DWORD _pascal VAppy32(DWORD dw) // To be called in 32-bits Ring 0 mode
    {
    _asm
	{
	push	ds		    ; Save my program's Data segment

	push	ss		    ; Move Ring 0's Data segment...
	pop	ds		    ;  ... to DS (Necessary for us)
	push	ss		    ; Move Ring 0's Data segment...
	pop	es		    ;  ... to ES (Necessary for VMM calls)

	; Adjust all pointers to refer to the high VM address
	_VMMCall(Get_Cur_VM_Handle) ; Returns hVM in EBX
	mov	ecx, [at_ebx+4]     ; [ebx.CB_High_Linear]
	mov	edx, dwParam32
	add	edx, ecx	    ; Adjust the relay array address
	mov	dwParam32, edx
	add	[at_edx], ecx	    ; Adjust the callback address
	add	[at_edx+4], ecx     ; Adjust the parameter list address

	; Schedule an Appy-time callback
	; VxDCall _SHELL_CallAtAppyTime, <<OFFSET32 SimSuspendATCB>,
	;				      hSemaphore, CAAFL_RING0, 0>
	xor	eax, eax
	push	eax
	mov	al, 1
	push	eax
	push	dwParam32
;	 lea	 eax, MyAtCb
;	 push	 eax
	call	skip_MyAtCb-2	    ; Pushes the return address on the stack
	DW(0)			    ; 2 extra bytes here explain -2 above

	; This return address happens to be the linear address of the Appy
	;  callback, that follows.

; MyAtCb:
	push	ebp
	mov	ebp, esp	    ; The appy callback stack frame
	mov	ebx, [at_ebp+8]     ; dwRelay array

	; Call the user callback
	push	ebx		    ; Save dwRelay address
	push	[at_ebx+4]	    ; dwParam
	call	[at_ebx]	    ; Call the callback. Cleans up its params.
	pop	ebx		    ; Restore dwRelay address

	; Unblock the ring 3
	mov	[at_ebx+12], eax    ; Save the return code
	inc	[at_ebx+8]	    ; Set the flag to unblock ring 3

	pop	ebp
	ret

	; End of the Appy callback

error:
	mov	eax, 5
	add	al, 5
	jmp	success-2
	DW(0)			    ; 2 extra bytes here explain -2 above

skip_MyAtCb:
	pop	eax		    ; Pop the return address
	add	eax, ecx	    ; Convert it to its equivalent in the high
	push	eax		    ;  VM image
	_VMMCall(_SHELL_CallAtAppyTime)
	add	esp, 10H
	; mov	  hAppyTimeCallback, eax
	test	eax, eax
	jz	error

	; Done. Return success
	xor	eax, eax
success:
	pop	ds		    ; Restore our Data segment

	pop	esi
	pop	edi		    ; Pushed by the built-in assembler
	leave
	retf	4
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)

#pragma warning(default:4704)	// Ignore the inline assembler etc... warning

// To be called in 16-bits protected mode
DWORD PM2AppyAndCallBack(PRING0CALLBACK pCB, void *pParam, DWORD *pdwRetVal)
    {
    DWORD dwRelay[4];
    DWORD dwLA;
    DWORD dwRet;
    void far *lpcb;

    WORD0(lpcb) = (WORD)CGroupOffset(pCB);
    WORD1(lpcb) = GetCS();
    dwRelay[0] = GetPMLinearAddress(lpcb);
    dwRelay[1] = GetPMLinearAddress(pParam);
    dwRelay[2] = 0;
    dwLA = GetPMLinearAddress(dwRelay);
    dwRet = PM2Ring0AndCallBack(CGroupOffset(VAppy32), dwLA);
    if (dwRet) return dwRet;
    while (dwRelay[2] == 0) ReleaseTimeSlice();
    *pdwRetVal = dwRelay[3];
    return 0;
    }
