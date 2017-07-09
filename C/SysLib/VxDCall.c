/*****************************************************************************\
*                                                                             *
*   Filename	    VxDCall.c						      *
*									      *
*   Description	    Call VxD WIN32 services & IO Controls under Windows 95/98.*
*                                                                             *
*   Notes	    VxDs have several distinct APIs, two of which are usable  *
*		    by WIN32 applications:				      *
*		    - WIN32 services, callable via one of the VxDCall()	procs.*
*		    - DeviceIoControl (DIOC for short), callable directly.    *
*		    							      *
*   History								      *
*    2017-07-07 JFL Created this file.					      *
*		    							      *
*        (C) Copyright 2017 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "VxDCall.h"

// VxD call routines in kernel32.dll
DWORD (WINAPI *VxDCall0)(DWORD service);
DWORD (WINAPI *VxDCall1)(DWORD service, DWORD);
DWORD (WINAPI *VxDCall2)(DWORD service, DWORD, DWORD);
DWORD (WINAPI *VxDCall3)(DWORD service, DWORD, DWORD, DWORD);
DWORD (WINAPI *VxDCall4)(DWORD service, DWORD, DWORD, DWORD, DWORD);
DWORD (WINAPI *VxDCall5)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD);
DWORD (WINAPI *VxDCall6)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
DWORD (WINAPI *VxDCall7)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
DWORD (WINAPI *VxDCall8)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

// 16-bits library management routines in kernel32.dll
HINSTANCE (WINAPI *LoadLibrary16)(char *);
void  (WINAPI *FreeLibrary16)(HINSTANCE);
DWORD (WINAPI *GetProcAddress16)(HINSTANCE, char *);
void  (WINAPI *QT_Thunk)(void);

// DOS memory management routines in the 16-bits kernel.exe
DWORD GlobalDosAlloc16;
DWORD GlobalDosFree16;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetK32ProcAddress					      |
|									      |
|   Description	    Get a KERNEL32.DLL procedure address by ordinal	      |
|									      |
|   Parameters	    int ord		    Ordinal value for the procedure   |
|									      |
|   Returns	    The procedure address, or 0 if not found.		      |
|									      |
|   Notes	    From Andrew Schulman sample code K32EXP.C, from	      |
|    http://www.digiater.nl/openvms/decus/vmslt96a/net96a/w95-k32exp.c	      |
|									      |
|   History								      |
|    2017-06-29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define ENEWHDR     0x003CL         /* offset of new EXE header */
#define EMAGIC      0x5A4D          /* old EXE magic id:  'MZ'  */
#define PEMAGIC     0x4550          /* NT portable executable */

#define GET_DIR(x)  (hdr->OptionalHeader.DataDirectory[x].VirtualAddress)

FARPROC WINAPI GetK32ProcAddress(int ord) {
    static HANDLE hmod = 0;
    IMAGE_NT_HEADERS *hdr;
    IMAGE_EXPORT_DIRECTORY *exp;
    DWORD *AddrFunc;
    WORD enewhdr, *pw;
    BYTE *moddb;
    static int done = 0;

    if (hmod == 0)      // one-time static init
        hmod = GetModuleHandle("KERNEL32");
    if (hmod == 0)      // still
        return 0;

    moddb = (BYTE *) hmod;
    pw = (WORD *) &moddb[0];
    if (*pw != EMAGIC)
        return 0;
    pw = (WORD *) &moddb[ENEWHDR];
    enewhdr = *pw;
    pw = (WORD *) &moddb[enewhdr];
    if (*pw != PEMAGIC)
        return 0;
    hdr = (IMAGE_NT_HEADERS *) pw;

    // Note: offset from moddb, *NOT* from hdr!
    exp = (IMAGE_EXPORT_DIRECTORY *) (((DWORD) moddb) +
        ((DWORD) GET_DIR(IMAGE_DIRECTORY_ENTRY_EXPORT)));
    AddrFunc = (DWORD *) (moddb + (DWORD) exp->AddressOfFunctions);

    // should verify that e.g.:
    // GetProcAddress(hmod, "VirtualAlloc") == GetK32ProcAddress(710);
    // JFL: Actually ordinals change from DLL version to DLL version, so testing a fixed ordinal is an error.
    /*
    if (!done) {
      done = 1;
      printf("# of functions = %d\n", exp->NumberOfFunctions);
      printf("GetProcAddress(VirtualAlloc) = %p\n", GetProcAddress(hmod, "VirtualAlloc"));
      printf("GetK32ProcAddress(710)       = %p\n", GetK32ProcAddress(710));
    }
    */

    ord--;  // table is 0-based, ordinals are 1-based
    if ((unsigned)ord < exp->NumberOfFunctions)
#pragma warning(disable:4055)	// Ignore the warning: 'type cast' : from data pointer 'void *' to function pointer 'FARPROC'
        return (FARPROC)(void *)(moddb + AddrFunc[ord]);
#pragma warning(default:4055)	// Restore the warning: 'type cast' : from data pointer 'void *' to function pointer 'FARPROC'
    else
        return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    Init_VxDCall					      |
|									      |
|   Description	    Initialize the VxDCall pointers			      |
|									      |
|   Parameters	    None					 	      |
|									      |
|   Returns	    Nothing						      |
|									      |
|   Notes	    See Matt Pietrek's "Windows 95 System Programming Secrets"|
|		    chapter 6 for details.				      |
|		    in Windows 98, all these functions have the same address; |
|		    But they differ in the number of arguments that they pop  |
|		    off the stack.					      |
|		    							      |
|   History								      |
|    2017-07-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

void Init_VxDCall() {
  if (!VxDCall8) {
#pragma warning(disable:4113)	// Ignore the warning: 'FARPROC' differs in parameter lists from ...
#pragma warning(disable:4057)	// Ignore the warning: ... slightly different base types from 'FARPROC'
    VxDCall0 = GetK32ProcAddress(1);
    VxDCall1 = GetK32ProcAddress(2);
    VxDCall2 = GetK32ProcAddress(3);
    VxDCall3 = GetK32ProcAddress(4);
    VxDCall4 = GetK32ProcAddress(5);
    VxDCall5 = GetK32ProcAddress(6);
    VxDCall6 = GetK32ProcAddress(7);
    VxDCall7 = GetK32ProcAddress(8);
    VxDCall8 = GetK32ProcAddress(9);
#pragma warning(default:4057)	// Restore the warning: ... slightly different base types from 'FARPROC'
#pragma warning(default:4113)	// Restore the warning: 'FARPROC' differs in parameter lists from ...
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    VWin32IoControl					      |
|									      |
|   Description	    Use WIN32 DeviceIoControls				      |
|									      |
|   Parameters     						 	      |
|									      |
|   Returns	    							      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-07-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

// Open the VWIN32 VxD
HANDLE OpenVWin32() {
  return CreateFile("\\\\.\\VWIN32", 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

// Close the VWIN32 VxD
void CloseVWin32(HANDLE hDevice) {
  CloseHandle(hDevice);
}

// DeviceIoControl the VWIN32 VxD
HRESULT VWin32IoControl(HANDLE hDevice, DWORD nCode, DIOC_REGISTERS* pReg) {
  BOOL fResult;
  DWORD nReturned;

  if (!pReg) return (int)E_INVALIDARG;

  fResult = DeviceIoControl(hDevice,
			    nCode,
			    pReg, sizeof(DIOC_REGISTERS),
			    pReg, sizeof(DIOC_REGISTERS),
			    &nReturned, 0);

  if (!fResult) return E_FAIL;

  if (pReg->reg_Flags & CF_MASK) {	// Carry set
    return pReg->reg_EAX;		// DOS error code
  }

  return S_OK;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GlobalDosAlloc / GlobalDosFree			      |
|									      |
|   Description	    Manage DOS memory blocks				      |
|									      |
|   Parameters     						 	      |
|									      |
|   Returns	    							      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-07-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL InitDosMemRoutines(void) {
  if (!QT_Thunk) {
    HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
#pragma warning(disable:4047)	// Ignore the warning: ... differs in levels of indirection from 'FARPROC'
    LoadLibrary16 = (HINSTANCE (WINAPI *)(char *))GetK32ProcAddress(35);
    FreeLibrary16 = (void (WINAPI *)(HINSTANCE))GetK32ProcAddress(36);
    GetProcAddress16 = (DWORD (WINAPI *)(HINSTANCE,char *))GetK32ProcAddress(37);
    QT_Thunk = (void (WINAPI *)(void))GetProcAddress(hKernel32, "QT_Thunk");
#pragma warning(default:4047)	// Restore the warning: ... differs in levels of indirection from 'FARPROC'
  }
  if (!GlobalDosFree16) {
    HINSTANCE hKernel16 = LoadLibrary16("kernel.exe");
    if ((DWORD)hKernel16 < 32) {
      // fprintf(stderr, "Can't load the 16-bits kernel.exe module\n");
      return FALSE;
    }
    GlobalDosAlloc16 = GetProcAddress16(hKernel16, "GlobalDosAlloc");
    GlobalDosFree16 = GetProcAddress16(hKernel16, "GlobalDosFree");
    FreeLibrary16(hKernel16); // Decrement the reference count. kernel.exe is always in memory anyway.
  }
  return TRUE;
}

// Thunk to GlobalDosAlloc16
#pragma warning(disable:4101)	// Ignore the warning: unreferenced local variable
DWORD GlobalDosAlloc(DWORD dwSize) {
  char cTemp[0x40];			// Used by QT_Thunk. DO NOT REMOVE.
  if (!GlobalDosAlloc16) if (!InitDosMemRoutines()) return 0;
  _asm {
    mov	    eax, dwSize
    mov	    edx, [GlobalDosAlloc16]	; DWORD FAR PASCAL GlobalDosAlloc(DWORD cbAlloc);
    push    eax
    call    dword ptr [QT_Thunk]	; Returns AX=Selector, DX=Segment
    movzx   eax, ax			; EAX = 0:Selector
    shl	    edx, 16			; EDX = Segment:0
    or	    eax, edx			; EAX = Segment:Selector
  }
  // Returns EAX top:bottom 16 bits = Segment:Selector
}

// Thunk to GlobalDosFree16
void GlobalDosFree(DWORD dwSelector) {
  char cTemp[0x40];			// Used by QT_Thunk. DO NOT REMOVE.
  if (!GlobalDosFree16) if (!InitDosMemRoutines()) return;
  _asm {
    mov	    	eax, dwSelector
    mov	    	edx, [GlobalDosFree16]	; UINT FAR PASCAL GlobalDosFree (UINT uSelector);
    push    	eax
    call    	dword ptr [QT_Thunk]
    movzx	eax, ax			; Make sure the top word is 0
  }
  // Returns EAX = 0 if success, else dwSelector if failed
}
#pragma warning(default:4101)	// Restore the warning: unreferenced local variable

void FarMemCopy(DWORD dwToOffset, DWORD dwToSelector, DWORD dwFromOffset, DWORD dwFromSelector, DWORD dwSize) {
  _asm {
    push ds
    push esi
    push es
    push edi
    mov ecx, dwSize
    mov edi, dwToOffset
    mov eax, dwToSelector
    mov es, ax
    mov esi, dwFromOffset
    mov eax, dwFromSelector
    mov ds, ax
    mov eax, ecx	; # of BYTEs to transfer
    shr ecx, 2	; # of DWORDs to transfer
    cld
    rep movsd
    and eax, 3	; # of leftover BYTEs
    jz  fmemcopy_done
    mov ecx, eax
    rep movsb
fmemcopy_done:
    pop edi
    pop es
    pop esi
    pop ds
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DPMI_Int13						      |
|									      |
|   Description	    Invoke a real mode interrupt 13H using a DPMI call	      |
|									      |
|   Parameters     						 	      |
|									      |
|   Returns	    							      |
|		    							      |
|   Notes	    Supports floppys & hard disks.			      |
|		    							      |
|   History								      |
|    2017-07-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int DPMI_Int13(REAL_MODE_REGS *pRegs) {
  DWORD dwEAX, dwECX, dwFlags;
  if (!VxDCall2) Init_VxDCall();
  dwEAX = 0x0300;		// DPMI function 300h: Simulate Real Mode Interrupt
  dwECX = 0;			// # of stack-words
  _asm {
    mov	    edi, pRegs		; Real mode registers
    mov	    ebx, 0x13		; BL = Interrupt# ; BH = flags
  }
  dwEAX = VxDCall2(VWIN32_Int31Dispatch, dwEAX, dwECX);
  SAVE_FLAGS(dwFlags);		// Carry set in case of error
  if (!(dwFlags & CF_MASK)) dwEAX = 0;	// Success
  return dwEAX;			// EAX = DPMI error code, 0=Success
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DIOC_Int13						      |
|									      |
|   Description	    Invoke a real mode interrupt 13H using a VWIN32_DIOC call |
|									      |
|   Parameters     						 	      |
|									      |
|   Returns	    							      |
|		    							      |
|   Notes	    Supports floppys only.				      |
|		    							      |
|   History								      |
|    2017-07-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int DIOC_Int13(HANDLE hDevice, DIOC_REGISTERS *pRegs) {
  if (hDevice == INVALID_HANDLE_VALUE) return -1;

  pRegs->reg_Flags = 0x0001; // Recommended, to detect unsupported calls on old BIOSs

  return VWin32IoControl(hDevice, VWIN32_DIOC_DOS_INT13, pRegs);
}
