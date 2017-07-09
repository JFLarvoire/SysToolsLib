/*****************************************************************************\
*                                                                             *
*   Filename:	    VxDCall.h						      *
*									      *
*   Description:    Definitions for VxDCall.c                          	      *
*                                                                             *
*   Notes:	    VxDs have several distinct APIs, two of which are usable  *
*		    by WIN32 applications:				      *
*		    - WIN32 services, callable via one of the VxDCall()	procs.*
*		    - DeviceIoControl (DIOC for short), callable directly.    *
*		    							      *
*   History:								      *
*    2017-07-07 JFL Created this file.					      *
*									      *
*        (C) Copyright 2017 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _VXDCALL_H_	// Prevent multiple inclusions
#define _VXDCALL_H_

#include "SysLib.h"	// SysLib Library core definitions

#include <windows.h>	// Windows types
#include <vmm.h>	// Windows 9X Virtual Machine Manager definitions

// Manipulate bytes, words and dwords. Can be used both as rvalue and as lvalue.
#define WORD0(dw) (((WORD *)(&(dw)))[0])
#define WORD1(dw) (((WORD *)(&(dw)))[1])
#define BYTE0(dw) (((BYTE *)(&(dw)))[0])
#define BYTE1(dw) (((BYTE *)(&(dw)))[1])
#define BYTE2(dw) (((BYTE *)(&(dw)))[2])
#define BYTE3(dw) (((BYTE *)(&(dw)))[3])

// DPMI "Simulate Real Mode Interrupt" function call registers:
#pragma pack(push, 1)
typedef struct {
  DWORD	rmEDI;
  DWORD	rmESI;
  DWORD	rmEBP;
  DWORD	rmres0;
  DWORD	rmEBX;
  DWORD	rmEDX;
  DWORD	rmECX;
  DWORD	rmEAX;
  WORD	rmFlags;
  WORD	rmES;
  WORD	rmDS;
  WORD	rmFS;
  WORD	rmGS;
  WORD	rmIP;
  WORD	rmCS;
  WORD	rmSP;
  WORD	rmSS;
} REAL_MODE_REGS;
#pragma pack(pop)

//+-------------------------------------------------------------------------+//
//+			     VxD WIN32 Service IDs			    +//
//+-------------------------------------------------------------------------+//

// VxD Device IDs are defined in vmm.h
// VxD WIN32 Service IDs are undocumented. See Matt Pietrek's "Windows 95 System Programming Secrets" chapter 6 for some information.
#define WIN32_SERVICE_ID(VxDID, SvcID) (((DWORD)((WORD)(VxDID)) << 16) | (WORD)(SvcID))

// The VWIN32 VxD is logically the one with the most WIN32 Services. These are of particular interest.
#define VWIN32_GetVersion	WIN32_SERVICE_ID(VWIN32_DEVICE_ID, 0x00)
#define VWIN32_Int21Dispatch	WIN32_SERVICE_ID(VWIN32_DEVICE_ID, 0x10)
#define VWIN32_Int31Dispatch	WIN32_SERVICE_ID(VWIN32_DEVICE_ID, 0x29)
#define VWIN32_LeaveCrst	WIN32_SERVICE_ID(VWIN32_DEVICE_ID, 0x41)
#define VWIN32_EnterCrst	WIN32_SERVICE_ID(VWIN32_DEVICE_ID, 0x42)

//+-------------------------------------------------------------------------+//
//+		     VxD WIN32 Device IO Control functions		    +//
//+-------------------------------------------------------------------------+//

#define VWIN32_DIOC_DOS_IOCTL	  1	// Performs the specified MS-DOS device I/O control function (Interrupt 21h Function 4400h through 4411h)
#define VWIN32_DIOC_DOS_INT25	  2	// Performs the Absolute Disk Read command (Interrupt 25h)
#define VWIN32_DIOC_DOS_INT26	  3	// Performs the Absolute Disk Write command (Interrupt 26h)
#define VWIN32_DIOC_DOS_INT13 	  4	// Performs Interrupt 13h commands
#define VWIN32_DIOC_SIMCTRLC      5	// Simulate a Ctrl-C
#define VWIN32_DIOC_DOS_DRIVEINFO 6	// Performs Interrupt 21h Function 730X commands. This value is supported in Windows 95 OEM Service Release 2 and later.

#pragma pack(push, 1)
typedef struct _DIOC_REGISTERS {
  DWORD reg_EBX;
  DWORD reg_EDX;
  DWORD reg_ECX;
  DWORD reg_EAX;
  DWORD reg_EDI;
  DWORD reg_ESI;
  DWORD reg_Flags;
} DIOC_REGISTERS;

typedef struct _DIOC_DISKIO {
  DWORD	diStartSector;
  WORD	diSectors;
  BYTE*	diBuffer;
} DIOC_DISKIO;

typedef struct _DIOC_MID {
  WORD midInfoLevel;
  DWORD midSerialNum;
  BYTE midVolLabel[11];
  BYTE midFileSysType[8];
} DIOC_MID;

typedef struct _DRIVE_MAP_INFO {
  BYTE dmiAllocationLength;
  BYTE dmiInfoLength;
  BYTE dmiFlags;
  BYTE dmiInt13Unit;
  DWORD dmiAssociatedDriveMap;
  LARGE_INTEGER   dmiPartitionStartRBA;
} DRIVE_MAP_INFO;

#pragma pack(pop)

//+-------------------------------------------------------------------------+//
//+		      Pointers to external library routines		    +//
//+-------------------------------------------------------------------------+//

// VxD call routines in kernel32.dll
extern DWORD (WINAPI *VxDCall0)(DWORD service);
extern DWORD (WINAPI *VxDCall1)(DWORD service, DWORD);
extern DWORD (WINAPI *VxDCall2)(DWORD service, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall3)(DWORD service, DWORD, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall4)(DWORD service, DWORD, DWORD, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall5)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall6)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall7)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);
extern DWORD (WINAPI *VxDCall8)(DWORD service, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

// 16-bits library management routines in kernel32.dll
extern HINSTANCE (WINAPI *LoadLibrary16)(char *);
extern void  (WINAPI *FreeLibrary16)(HINSTANCE);
extern DWORD (WINAPI *GetProcAddress16)(HINSTANCE, char *);
extern void  (WINAPI *QT_Thunk)(void);

//+-------------------------------------------------------------------------+//
//+		      Prototypes of routines in VxDCall.c		    +//
//+-------------------------------------------------------------------------+//

#ifdef __cplusplus
extern "C" {
#endif

// Get kernel32.dll procedures address by ordinal
FARPROC WINAPI GetK32ProcAddress(int ordinal);

// Manage DOS memory
DWORD GlobalDosAlloc(DWORD dwSize);
void GlobalDosFree(DWORD dwSelector);
void FarMemCopy(DWORD dwToOffset, DWORD dwToSelector, DWORD dwFromOffset, DWORD dwFromSelector, DWORD dwSize);

// Manage VxD Calls
extern void Init_VxDCall();	// Initialize the VxDCallN pointers

// Invoke a real mode interrupt 13H
int DPMI_Int13(REAL_MODE_REGS *pRegs);			// Using DPMI (Supports floppys & hard disks)
int DIOC_Int13(HANDLE hDevice, DIOC_REGISTERS *pRegs);	// Using VWIN32 DeviceIoControl (Only supported for floppys)

#ifdef __cplusplus
}
#endif

#endif // _VXDCALL_H_
