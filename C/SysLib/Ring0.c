/*****************************************************************************\
*                                                                             *
*   Filename:	    Ring0						      *
*									      *
*   Description:    Ring0 access routines from WIN32 under Windows 95.	      *
*                                                                             *
*   Notes:	    Important: This code used to work in 2001 on Win95 PCs.   *
*		    As of 2017 is does not work anymore on Win98 VMs.	      *
*                   Yet, as far as I can tell, the source had not been changed.
*                   I've corrected a few minor issues, but the real problem   *
*                   remains.                                                  *
*		    Experiments with SoftICE show that R0GetGdtBase() return  *
*		    an incorrect value for the GDT base, then subsequent      *
*		    attempts at patching the GDT trigger protection errors.   *
*		    Apparently this is a known problem with virtual machines, *
*		    as the SGDT instruction is not privileged, and thus it's  *
*		    the host OS's that's returned, and not the VM's.	      *
*		    The SIDT instruction has the same problem.		      *
*		    Is there any alternate mechanism for getting the VM's     *
*		    GDT and IDT that we could use?			      *
*		    							      *
*		    Funny finding: Stepping over the SGDT instruction using   *
*		    a p command _does_ store the VM's GDT base. Executing the *
*		    same SGDT instruction using a g command does _not_. Which *
*		    seems to prove that SoftICE p command emulates the SGDT   *
*		    instruction, whereas the g command actually runs it.      *
*		    							      *
*		    Compilable both in C and C++ without any warning.	      *
*		    							      *
*		    Many experimental routines are left in this source.       *
*		    But they are compiled-in only in _DEBUG mode.	      *
*		    The routines compiled-in in release mode have been        *
*		    tested thoroughly.					      *
*		    							      *
*   History:								      *
*    2001-05-07 JFL Created this file.					      *
*    2001-09-07 JFL Moved definitions to Ring0.h.			      *
*		    Moved I/O Supervisor access routines to R0IOS.c.	      *
*		    Compile-in experimental routines only if _DEBUG is defined*
*    2017-06-29 JFL Fixed compilation warnings. 			      *
*		    Fixed support for x86_64 processors.		      *
*		    Fixed support for new C compilers, with security cookie   *
*		    at [ebp-4].						      *
*    2017-07-07 JFL Fixed more compilation warnings. 			      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "Ring0.h"

static WORD wGate32Sel = 0;

/*---------------------------------------------------------------------------*\
*                                                                             *
|			X86 processor information services		      |
*									      *
\*---------------------------------------------------------------------------*/

WORD R0GetLdtr(void)
    {
    WORD wLdt;

    _asm sldt wLdt

    return wLdt;
    }

DWORD R0GetGdtBase(void)
    {
    char cGdtDesc[10];	// x86_32 processors require 6 bytes; x86_64 processors require 10.

    _asm sgdt cGdtDesc

    return *(DWORD *)(cGdtDesc+2);
    }

WORD R0GetGdtLimit(void)
    {
    char cGdtDesc[10];	// x86_32 processors require 6 bytes; x86_64 processors require 10.

    _asm sgdt cGdtDesc

    return *(WORD *)(cGdtDesc+0);
    }

DWORD R0GetIdtBase(void)
    {
    char cIdtDesc[10];	// x86_32 processors require 6 bytes; x86_64 processors require 10.

    _asm sidt cIdtDesc

    return *(DWORD *)(cIdtDesc+2);
    }

WORD R0GetIdtLimit(void)
    {
    char cIdtDesc[10];	// x86_32 processors require 6 bytes; x86_64 processors require 10.

    _asm sidt cIdtDesc

    return *(WORD *)(cIdtDesc+0);
    }

WORD R0GetCS()
    {
    _asm mov ax, cs
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

DWORD R0GetDescBase(PDESCRIPTOR pDesc)
    {
    DWORD dwBase;

    WORD0(dwBase) = pDesc->Base_0_15;
    BYTE2(dwBase) = pDesc->Base_16_23;
    BYTE3(dwBase) = pDesc->Base_24_31;

    // To do: Account for expand-down segments

    return dwBase;
    }

DWORD R0GetDescLimit(PDESCRIPTOR pDesc)
    {
    DWORD dwLimit;

    WORD0(dwLimit) = pDesc->Limit_0_15;
    WORD1(dwLimit) = pDesc->Extra_Rights & 0x0F;
    if (pDesc->Extra_Rights & 0x80) // If big segment
        {
        dwLimit <<= 12;
        dwLimit |= 0xFFF;
        }

    // To do: Account for expand-down segments

    return dwLimit;
    }

WORD R0GetDescRights(PDESCRIPTOR pDesc)
    {
    WORD wRights;

    BYTE0(wRights) = pDesc->Access_Rights;
    BYTE1(wRights) = pDesc->Extra_Rights;

    return wRights;
    }

WORD R0StealSelector(PDESCRIPTOR pXDT, WORD wLimit)
    {
    WORD wSel;
    int iLdt = 0;

    if ((DWORD)pXDT != R0GetGdtBase())	// If it's not the GDT, assume it's the LDT.
        iLdt = 4;			// Then all selectors must be flagged.

    // VERY dirty trick to bootstrap the Ring 0 process:
    // Find the first empty descriptor in the 2nd half of the table, and use it.
    // It MUST be replaced asap by a legally allocated selector!

    wLimit += 1;	// Ceiling
    for (wSel = wLimit/2; wSel < wLimit; wSel+=8)
        {
        DWORD *pdw;
        WORD wIndex;

        wIndex = wSel / 8;
        pdw = (DWORD *)(pXDT+wIndex);
        if ((!pdw[0]) && (!pdw[1])) return wSel+(WORD)iLdt;
        }

    return 0;
    }

void R0ReleaseSelector(PDESCRIPTOR pXDT, WORD wSel)
    {
    int i = (int)(wSel / 8);
    DWORD *pdw;

    pdw = (DWORD *)(pXDT+i);
    pdw[0] = pdw[1] = 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|			   Ring 0 Callback mechanism			      |
*									      *
\*---------------------------------------------------------------------------*/

// Common entry point for all ring 0 callbacks.
// Initializes registers, and dispatches the call to the final callback.

// ~~JFL 2001/10/01 Added directive __declspec(naked) to avoid surprises
//		    with various compiler options that change the prolog.

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

void __declspec(naked) __stdcall R0CallBackEntry(DWORD dw1, DWORD dw2, DWORD dw3)
    {
    // Prepare Ring0 segment registers
    _asm
        {
	// ~~JFL 2001/10/01. Redefine a standard prolog.
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi
	// Now add our stuff
	push	ds
	push	es
	push	fs
	push	gs
	push	ss
	push	ss
	push	ss
	pop	es
	pop	ds
	pop	gs
	push	78H	; TO DO: How to remove this hard-coded constant for FS?
	pop	fs
        }

    // Call the actual callback.
    // Note that dw1 is actually the segment register for the invoking routine.
    // Note Win32 apps space is NOT duplicated in the Hi VM area space.
    ((R0CALLBACK)(dw2))(dw3);

    // Restore initial registers
    _asm
        {
        pop	gs
        pop	fs
        pop	es
        pop	ds
        // The following were saved by the code prolog automatically inserted by the compiler
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	// Force a far return. There's no _far keyword anymore :-(
	retf	12
        }
    }

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

// Cleanup routine to be called at program exit.

void R0FreeCallGate(void)
    {
    R0FreeLdtSelector(wGate32Sel);
    }

// Ring 3 gateway, used to call Ring 0 callbacks

DWORD R0CallCallBack(R0CALLBACK pCallBack, DWORD dwRef)
    {
    DWORD dwFarPtr[2];

    // printf("CallCallBack(%p, %X)\n", pCallBack, dwRef);

    if (!wGate32Sel)	// Do the initialization if it was not done already.
        {
	PDESCRIPTOR pGdt;
	PDESCRIPTOR pLdt;
	WORD wLdtLimit;
	PCALLGATEDESCRIPTOR pCG;
	WORD wNew;
	int i;

        // Locate the GDT
        pGdt = (PDESCRIPTOR)R0GetGdtBase();
        // Locate the LDT
        i = (int)R0GetLdtr() / 8;
        pLdt = (PDESCRIPTOR)R0GetDescBase(pGdt+i);
        wLdtLimit = (WORD)R0GetDescLimit(pGdt+i);
        // Allocate a temporary selector in the LDT
        wGate32Sel = R0StealSelector(pLdt, wLdtLimit);	// Allocate a new selector
        // printf("Temporary CG Sel = %04X.\n", wGate32Sel);
        // i = (int)(wGate32Sel & 0xFFF8);
        // DumpBuf(pLdt, i, i+8);
        // Build a temporary Call Gate to our Ring 0 routine

        if (!wGate32Sel) return (DWORD)-1L;
        pCG = (PCALLGATEDESCRIPTOR)(pLdt+(wGate32Sel/8));
        pCG->Offset_0_15 = (WORD)(DWORD)R0CallBackEntry;
        pCG->Selector = 0x28;			// Standard VMM code selector
        pCG->DWord_Count = 3;
        pCG->Access_Rights = GATE32_RING3;
        pCG->Offset_16_31 = (WORD)((DWORD)R0CallBackEntry >> 16);
        // Use it to allocate the final Call Gate
        wNew = R0AllocLdtSelector(1);
	// Move the temporary call gate to its final place
	if (wNew != wGate32Sel) // (only if it's not the same)
	    {
	    ((PCALLGATEDESCRIPTOR)pLdt)[wNew/8] = *pCG;	// Copy the descriptor

            R0ReleaseSelector(pLdt, wGate32Sel);	// Cleanup the temporary instance
            wGate32Sel = wNew;                          // Memorize the final selector
            }

        // printf("Final CG Sel = %04X.\n", wGate32Sel);
        atexit(R0FreeCallGate);	// And don't forget to free in at exit() time.
        }

    dwFarPtr[0] = 0;
    dwFarPtr[1] = wGate32Sel;

    _asm
        {
        push	0
        push	dwRef
        push	pCallBack
        call	fword ptr dwFarPtr
        }

#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|			   VMM services front ends			      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0GetVmmVersionCB(DWORD dw)
    {
    VMMCall(Get_VMM_Version);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

DWORD R0GetVmmVersion(void)
    {
    return R0CallCallBack(R0GetVmmVersionCB, 0);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0GetCurVmHandleCB(DWORD dw)
    {
    VMMCall(Get_Cur_VM_Handle);
    _asm mov eax, ebx
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

DWORD R0GetCurVmHandle(void)
    {
    return R0CallCallBack(R0GetCurVmHandleCB, 0);
    }

DWORD R0AllocLdtSelectorCB(DWORD dwCount)
    {
    // VMMCall _Allocate_LDT_Selector, <VM, DescDWORD1, DescDWORD2, Count, flags>
    _asm
    	{
    	push	0
	push 	dwCount
	push	000CF9300H	; Descriptor for 4 GB flat data access
	push	00000FFFFH
	}
    VMMCall(Get_Cur_VM_Handle);
    _asm push	ebx
    VMMCall(_Allocate_LDT_Selector)
    _asm add esp, 20

    // Make sure the selector is DPL 0
    _asm and eax, 0FFFCH
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

WORD R0AllocLdtSelector(int iCount)
    {
    return (WORD)R0CallCallBack(R0AllocLdtSelectorCB, iCount);
    }

DWORD R0FreeLdtSelectorCB(DWORD dwSel)
    {
    // VMMCall _Free_LDT_Selector, <VM, Selector, flags>
    _asm
    	{
    	push	0
	push 	dwSel
	}
    VMMCall(Get_Cur_VM_Handle);
    _asm push	ebx
    VMMCall(_Free_LDT_Selector)
    _asm add esp, 12
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

void R0FreeLdtSelector(WORD wSel)
    {
    R0CallCallBack(R0FreeLdtSelectorCB, wSel);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0BeginReentrantExecutionCB(DWORD dwRef)
    {
    VMMCall(Begin_Reentrant_Execution);
    _asm mov eax, ecx		; Reentrancy count
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

DWORD R0BeginReentrantExecution(void)
    {
    return R0CallCallBack(R0BeginReentrantExecutionCB, 0);
    }

DWORD R0EndReentrantExecutionCB(DWORD dwCount)
    {
    _asm mov ecx, dwCount	; Reentrancy count
    VMMCall(End_Reentrant_Execution);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

void R0EndReentrantExecution(DWORD dwCount)
    {
    R0CallCallBack(R0EndReentrantExecutionCB, dwCount);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0HeapAllocate					      |
|									      |
|   Description:    Front end to VMM _HeapAllocate(ULONG nbytes, ULONG flags) |
|									      |
|   Parameters:     ULONG nbytes					      |
|		    ULONG flags		Typically 0 or HEAPZEROINIT	      |
|									      |
|   Returns:	    Pointer to the block, or NULL			      |
|									      |
|   Notes:	    Allocates a block of memory in the non-pageable pool      |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

DWORD R0HeapAllocateCB(DWORD dwRef)
    {
    ULONG *pul = (ULONG *)dwRef;

    // _HeapAllocate(ULONG nbytes, ULONG flags);
    return (DWORD)_HeapAllocate(pul[0], pul[1]);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

void *R0HeapAllocate(ULONG nBytes, ULONG flags)
    {
    return (void *)R0CallCallBack(R0HeapAllocateCB, (DWORD)&nBytes);
    }

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0HeapFree						      |
|									      |
|   Description:    Front end to VMM _HeapFree(ULONG hAddress, ULONG flags)   |
|									      |
|   Parameters:     ULONG hAddress					      |
|		    ULONG flags		Must be 0			      |
|									      |
|   Returns:	    0 = Failed; !0 = succeeded				      |
|									      |
|   Notes:	    Frees a block of memory in the non-pageable pool	      |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

DWORD R0HeapFreeCB(DWORD dwRef)
    {
    ULONG *pul = (ULONG *)dwRef;

    // _HeapFree(ULONG hAddress, ULONG flags);
    return _HeapFree((void *)(pul[0]), pul[1]);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0HeapFree(void *hAddress, ULONG flags)
    {
    return R0CallCallBack(R0HeapFreeCB, (DWORD)&hAddress);
    }

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|			  Other VxD services front ends			      |
*									      *
\*---------------------------------------------------------------------------*/

// Currently all this is useless. It was defined for routine R0CallInt13() below, which does not work yet.

#ifdef _DEBUG

// There's no V86MMGR.H file in Win98 DDK. Convert table from V86MMGR.INC.

#define	V86MMGR_Service	Declare_Service

Begin_Service_Table	(V86MMGR, VxD)

  V86MMGR_Service(V86MMGR_Get_Version, LOCAL)
  V86MMGR_Service(V86MMGR_Allocate_V86_Pages, LOCAL)
  V86MMGR_Service(V86MMGR_Set_EMS_XMS_Limits, LOCAL)
  V86MMGR_Service(V86MMGR_Get_EMS_XMS_Limits, LOCAL)
//
//  Services used for API mapping.
//
  V86MMGR_Service(V86MMGR_Set_Mapping_Info, VxD_ICODE)
  V86MMGR_Service(V86MMGR_Get_Mapping_Info, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Xlat_API, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Load_Client_Ptr, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Allocate_Buffer, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Free_Buffer, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Get_Xlat_Buff_State, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Set_Xlat_Buff_State, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Get_VM_Flat_Sel, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Map_Pages, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Free_Page_Map_Region, VxD_PAGEABLE_CODE)
//***********************************************************
// END OF 3.00 level services
//
  V86MMGR_Service(V86MMGR_LocalGlobalReg, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_GetPgStatus, LOCAL)
  V86MMGR_Service(V86MMGR_SetLocalA20, VxD_ICODE)
  V86MMGR_Service(V86MMGR_ResetBasePages, LOCAL)
  V86MMGR_Service(V86MMGR_SetAvailMapPgs, VxD_ICODE)
  V86MMGR_Service(V86MMGR_NoUMBInitCalls, VxD_ICODE)

#ifndef	WIN31COMPAT

//   Services added post Win 3.1
  V86MMGR_Service(V86MMGR_Get_EMS_XMS_Avail, LOCAL)
  V86MMGR_Service(V86MMGR_Toggle_HMA, VxD_PAGEABLE_CODE)
  V86MMGR_Service(V86MMGR_Dev_Init, LOCAL)
  V86MMGR_Service(V86MMGR_Alloc_UM_Page, LOCAL)

#endif //	WIN31COMPAT

End_Service_Table	(V86MMGR, VxD)

typedef struct
    {
    DWORD dwSize;
    void *pBuf;
    } V86MMGRALLOCPARMS;

#pragma warning(disable:4731)	// Ignore the 'frame pointer register 'ebp' modified by inline assembly code' warning

DWORD R0V86mmgrAllocateBufferCB(DWORD dwRef)
    {
    V86MMGRALLOCPARMS *pParms = (V86MMGRALLOCPARMS *)dwRef;

    VMMCall(Get_Cur_VM_Handle)	; mov ebx, vmHandle
    VMMCall(Get_Next_VM_Handle)	; ~~jfl 2001/05/17 Test with 2 VMs.
    _asm
        {
	push	fs
	push	ds
	pop	fs
	mov	edi, pParms
	mov	esi, dword ptr [edi+4]		; extended memory to copy if carry set
	xor	eax, eax
	cmp	eax, esi			; Set carry if esi != 0
	mov     ecx, dword ptr [edi]		; mov     ecx, NumBytes
        push	ebp
	mov     ebp, [ebx.CB_Client_Pointer]	; OFFSET32 ClientRegisters
	}
    VxDCall(V86MMGR_Allocate_Buffer);
    _asm
        {
	pop	ebp
	pop	fs
	sbb	eax, eax	// EAX=-1 if error
	jc      error_handler
	mov	ebx, pParms
	mov     [ebx], ecx	// Bytes copied
	mov     [ebx+4], edi	// FarPtrBuffer
error_handler:
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4731)	// Restore the 'frame pointer register 'ebp' modified by inline assembly code' warning

// Allocate a buffer. Optionnally fill it with provided buffer contents if pBuf != NULL.

DWORD R0V86mmgrAllocateBuffer(DWORD dwSize, void *pBuf)
    {
    V86MMGRALLOCPARMS parms;
    DWORD dwErr;

    parms.pBuf = pBuf;
    parms.dwSize = dwSize;

    dwErr = R0CallCallBack(R0V86mmgrAllocateBufferCB, (DWORD)&parms);

    // To do: Manage the case where they allocate a buffer smaller than requested.
    // cf V86MMGR_Allocate_Buffer doc.
    return dwErr ? 0 : (DWORD)(parms.pBuf);
    }

#pragma warning(disable:4731)	// Ignore the 'frame pointer register 'ebp' modified by inline assembly code' warning

DWORD R0V86mmgrFreeBufferCB(DWORD dwRef)
    {
    V86MMGRALLOCPARMS *pParms = (V86MMGRALLOCPARMS *)dwRef;

    VMMCall(Get_Cur_VM_Handle)	; mov ebx, vmHandle
    VMMCall(Get_Next_VM_Handle)	; ~~jfl 2001/05/17 Test with 2 VMs.
    _asm
        {
	push	fs
	push	ds
	pop	fs
	mov	edi, pParms
	mov	esi, dword ptr [edi+4]		; extended memory to copy if carry set
	xor	eax, eax
	cmp	eax, esi			; Set carry if esi != 0
	mov     ecx, dword ptr [edi]		; mov     ecx, NumBytes
        push	ebp
	mov     ebp, [ebx.CB_Client_Pointer]	; OFFSET32 ClientRegisters
	}
    VxDCall(V86MMGR_Free_Buffer);
    _asm
        {
	pop	ebp
	pop	fs
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4731)	// Restore the 'frame pointer register 'ebp' modified by inline assembly code' warning

void R0V86mmgrFreeBuffer(DWORD dwSize, void *pBuf)
    {
    V86MMGRALLOCPARMS parms;

    parms.pBuf = pBuf;
    parms.dwSize = dwSize;

    R0CallCallBack(R0V86mmgrFreeBufferCB, (DWORD)&parms);
    }

DWORD R0SaveClientStateCB(DWORD dwBuf)
    {
    _asm mov edi, dwBuf
    _asm int 3;
    VMMCall(Save_Client_State);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

void R0SaveClientState(struct Client_Reg_Struc *pBuf)
    {
    R0CallCallBack(R0SaveClientStateCB, (DWORD)pBuf);
    }

DWORD R0RestoreClientStateCB(DWORD dwBuf)
    {
    _asm mov esi, dwBuf
    _asm int 3;
    VMMCall(Restore_Client_State);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

void R0RestoreClientState(struct Client_Reg_Struc *pBuf)
    {
    R0CallCallBack(R0RestoreClientStateCB, (DWORD)pBuf);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0BeginNestV86ExecCB(DWORD dwRef)
    {
    _asm int 3;
    VMMCall(Begin_Nest_V86_Exec);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

void R0BeginNestV86Exec(void)
    {
    R0CallCallBack(R0BeginNestV86ExecCB, 0);
    }

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0EndNestExecCB(DWORD dwRef)
    {
    _asm int 3;
    VMMCall(End_Nest_Exec);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

void R0EndNestExec(void)
    {
    R0CallCallBack(R0EndNestExecCB, 0);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0CallInt13						      |
|									      |
|   Description:    Experimental function attempting to call the BIOS	      |
|									      |
|   Parameters:     DWORD dwEAX		Input register EAX to pass to int 13H |
|		    DWORD dwECX		Input register ECX to pass to int 13H |
|		    DWORD dwEDX		Input register EDX to pass to int 13H |
|		    DWORD dwESBX	Buffer address ES:BX to pass to int 13|
|									      |
|   Returns:	    Pointer to the block, or NULL			      |
|									      |
|   Notes:	    WARNING: This function currently does not work!!!!!!      |
|		    It crashes due to a memory map problem in the system VM:  |
|		    During WIN32 tasks execution, the 1st MB of address space |
|		    is not mapped in.					      |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4731)	// Ignore the 'frame pointer register 'ebp' modified by inline assembly code' warning

DWORD R0CallInt13CB(DWORD dwRef)
    {
    struct Client_Reg_Struc csRegsBackup;

    _asm int 3;

    _asm lea edi, csRegsBackup;
    VMMCall(Save_Client_State);
    VMMCall(Get_Cur_VM_Handle)	; mov ebx, vmHandle
    VMMCall(Get_Next_VM_Handle)	; ~~jfl 2001/05/17 Test with 2 VMs.
    VMMCall(Begin_Nest_V86_Exec);
    _asm
    	{
    	mov	esi, dwRef
        push	ebp
	mov     ebp, [ebx.CB_Client_Pointer]	; OFFSET32 ClientRegisters
    	mov	eax, [esi]	; dwEAX
    	mov	[ebp.Client_EAX], eax
    	mov	eax, [esi+4]	; dwECX
    	mov	[ebp.Client_ECX], eax
    	mov	eax, [esi+8]	; dwEDX
    	mov	[ebp.Client_EDX], eax
    	mov	eax, [esi+12]	; dwESBX
    	mov	[ebp.Client_BX], ax
    	shr	eax, 16
    	mov	[ebp.Client_ES], ax
	}
    _asm mov eax, 13H
    VMMCall(Exec_Int);
    _asm push [ebp.Client_EAX]
    VMMCall(End_Nest_Exec);
    _asm
    	{
    	pop	eax
    	pop	ebp
    	mov	esi, dwRef
    	mov	[esi], eax
    	}
    _asm lea esi, csRegsBackup
    VMMCall(Restore_Client_State);
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4731)	// Restore the 'frame pointer register 'ebp' modified by inline assembly code' warning

#pragma warning(disable:4100)	// Disable the "unreferenced formal parameter" warning

DWORD R0CallInt13(DWORD dwEAX, DWORD dwECX, DWORD dwEDX, DWORD dwESBX)
    {
    return R0CallCallBack(R0CallInt13CB, (DWORD)&dwEAX);
    }

#endif // defined(_DEBUG)

#pragma warning(default:4100)	// Restore the "unreferenced formal parameter" warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    TestRing0						      |
|									      |
|   Description:    Test Ring 0 access routines				      |
|									      |
|   Parameters:     None					 	      |
|									      |
|   Returns:	    Nothing						      |
|									      |
|   History:								      |
|									      |
|    2001/02/22 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _DEBUG

#include <stdio.h>

int TestRing0(void)
    {
    int i;
    PDESCRIPTOR pGdt;
    PDESCRIPTOR pLdt;
    PCALLGATEDESCRIPTOR pIdt;
    DWORD dwBase;
    DWORD dwLimit;
    WORD wRights;
    PDESCRIPTOR pDesc;
    WORD wCS;
    WORD wLdtLimit;
    DWORD dw;

    // Locate the GDT and dumps its beginning
    pGdt = (PDESCRIPTOR)R0GetGdtBase();
    printf("GDT base = %08X, limit = %04X\n", pGdt, R0GetGdtLimit());
    for (i=0; i<8; i++)
        {
        dwBase = R0GetDescBase(pGdt+i);
        dwLimit = R0GetDescLimit(pGdt+i);
        wRights = R0GetDescRights(pGdt+i);
        printf("  %08X: Base = %08X, Limit = %08X, Rights = %04X\n", i*8, dwBase, dwLimit, wRights);
        }

    // Locate the IDT and dumps its beginning
    pIdt = (PCALLGATEDESCRIPTOR)R0GetIdtBase();
    printf("IDT base = %08X, limit = %04X\n", pIdt, R0GetIdtLimit());
    for (i=0; i<8; i++)
        {
        WORD0(dwBase) = pIdt[i].Offset_0_15;
        WORD1(dwBase) = pIdt[i].Offset_16_31;
        printf("  %08X: Offset = %08X, Segment = %04X, Rights = %02X, Dwords = %x\n",
        	i*8, dwBase, pIdt[i].Selector, pIdt[i].Access_Rights, pIdt[i].DWord_Count);
        }

    // Locate the LDT and dumps its beginning
    i = (int)R0GetLdtr() / 8;
    dwBase = R0GetDescBase(pGdt+i);
    dwLimit = R0GetDescLimit(pGdt+i);
    wLdtLimit = (WORD)dwLimit;
    printf("LDTR = %04X, Base = %08X, Limit = %04X\n", R0GetLdtr(), dwBase, dwLimit);
    pLdt = (PDESCRIPTOR)dwBase;
    for (i=0; i<8; i++)
        {
        dwBase = R0GetDescBase(pLdt+i);
        dwLimit = R0GetDescLimit(pLdt+i);
        wRights = R0GetDescRights(pLdt+i);
        printf("  %08X: Base = %08X, Limit = %08X, Rights = %04X\n", i*8, dwBase, dwLimit, wRights);
        }

    // Locate the current code segment
    wCS = R0GetCS();
    pDesc = (wCS & 0x0004) ? pLdt : pGdt;
    i = (int)(wCS / 8);
    dwBase = R0GetDescBase(pDesc+i);
    dwLimit = R0GetDescLimit(pDesc+i);
    wRights = R0GetDescRights(pDesc+i);
    printf("CS = %04X, Base = %08X, Limit = %08X, Rights = %04X\n", wCS, dwBase, dwLimit, wRights);

    /* Make selected VxD calls */

    dw = R0GetCurVmHandle();
    printf("hVM = %08X.\n", dw);

    dw = R0GetVmmVersion();
    printf("VMM Version = %08X.\n", dw);

    dw = R0AllocLdtSelector(1);
    printf("LDT Sel = %08X.\n", dw);

    R0FreeLdtSelector((WORD)dw);

    return 0;
    }

#endif // defined(_DEBUG)
