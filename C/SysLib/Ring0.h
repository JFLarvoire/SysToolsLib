/*****************************************************************************\
*                                                                             *
*   Filename:	    Ring0.h						      *
*									      *
*   Description:    Definitions for Ring0.c (VxD access from WIN32).	      *
*                                                                             *
*   Notes:	    Compilable both in C and C++ without any warning.	      *
*		    							      *
*   History:								      *
*    2001-09-07 JFL Created this file.					      *
*    2001-10-01 JFL Make sure all VMM structures are packed.		      *
*		    Fixed prototype for R0HeapFree().			      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _RING0_H_   // Prevent multiple inclusions
#define _RING0_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#pragma pack(1)

#include <windows.h>	// Windows types
#include <vmm.h>

typedef DWORD (*R0CALLBACK)(DWORD dwRef);

// Manipulate bytes, words and dwords. Can be used both as rvalue and as lvalue.
#define WORD0(dw) (((WORD *)(&(dw)))[0])
#define WORD1(dw) (((WORD *)(&(dw)))[1])
#define BYTE0(dw) (((BYTE *)(&(dw)))[0])
#define BYTE1(dw) (((BYTE *)(&(dw)))[1])
#define BYTE2(dw) (((BYTE *)(&(dw)))[2])
#define BYTE3(dw) (((BYTE *)(&(dw)))[3])

//+-------------------------------------------------------------------------+//
//+				386 Structures				    +//
//+-------------------------------------------------------------------------+//

// Prevent redefinition warnings in vmm.h (Values are redefined identically)
#undef D_PRES
#undef D_DPL1
#undef D_DPL2
#undef D_DPL3
#undef D_SEG
#undef D_GRAN_BYTE
#undef D_GRAN_PAGE
#undef D_DEF16
#undef D_DEF32
#undef SELECTOR_MASK
#undef SEL_LOW_MASK
#undef RPL_CLR

typedef struct
    {
    WORD Limit_0_15;	    // Limit bits (0..15)
    WORD Base_0_15;	    // Base bits (0..15)
    BYTE Base_16_23;	    // Base bits (16..23)
    BYTE Access_Rights;     // 286 access rights
    BYTE Extra_Rights;	    // 386 extra access rights
    BYTE Base_24_31;	    // Base bits (24..31)
    } DESCRIPTOR, *PDESCRIPTOR, far *LPDESCRIPTOR;

typedef struct
    {
    WORD Offset_0_15;	    // Entry point's offset, lower 16
    WORD Selector;	    // Entry point's selector
    BYTE DWord_Count;	    // (D)Word parameter count
    BYTE Access_Rights;     // Present, dpl, system, type
    WORD Offset_16_31;	    // Entry point's offset, upper 16
    } CALLGATEDESCRIPTOR,* PCALLGATEDESCRIPTOR, far *LPCALLGATEDESCRIPTOR;

/*********** Definitions for the access rights byte in a descriptor **********/

// Following fields are common to segment and control descriptors

#define  D_PRES_MASK	0x80	       /* Present bit mask */
#define  D_PRES         0x80           /* present in memory */
#define  D_NOTPRES      0              /* not present in memory */

#define  D_DPL_MASK	0x60	       /* Descriptor Priviledge Level mask */
#define  D_DPL0         0              /* Ring 0 */
#define  D_DPL1         0x20           /* Ring 1 */
#define  D_DPL2         0x40           /* Ring 2 */
#define  D_DPL3         0x60           /* Ring 3 */

#define  D_TYPE_MASK	0x10	       /* Descriptor Type mask */
#define  D_SEG          0x10           /* Segment descriptor */
#define  D_CTRL         0              /* Control descriptor */

// Following fields are specific to control descriptors

#define CTRL_DESC_MASK	0x0f  /* TYPE field for system and gate descriptors */

#define DESC_TYPE_UNUSED            0       // invalid descriptor
#define DESC_TYPE_286_TSS           1       // 80286 TSS descriptor
#define DESC_TYPE_LDT               2
#define DESC_TYPE_BUSY_286_TSS      3
#define DESC_TYPE_286_CALL_GATE     4       // 286 call gate
#define DESC_TYPE_TASK_GATE         5
#define DESC_TYPE_286_INT_GATE      6
#define DESC_TYPE_286_TRAP_GATE     7
#define DESC_TYPE_386_TSS           9       // 80386/80486 TSS
#define DESC_TYPE_BUSY_386_TSS      11      // 80386/80486 busy TSS
#define DESC_TYPE_386_CALL_GATE     12      // 80386/80486 call gate
#define DESC_TYPE_386_INT_GATE      14      // 80386/80486 interrupt gate
#define DESC_TYPE_386_TRAP_GATE     15      // 80386/80486 trap gate

// Following fields are specific to segment descriptors

#define  D_STYPE_MASK	0x10	       /* Descriptor Sub-Type mask */
#define  D_CODE         0x08           /* code */
#define  D_DATA         0              /* data */

#define  D_RX           0x02           /* if code, readable */
#define  D_X            0              /* if code, exec only */
#define  D_W            0x02           /* if data, writable */
#define  D_R            0              /* if data, read only */

#define  D_C            0x04           /* if code, conforming */
#define  D_E            0x04           /* if data, expand-down */

#define  D_ACCESSED     1              /* segment accessed bit */

// Useful combination access rights bytes

#define  GATE32_RING3   (D_PRES+D_DPL3+D_CTRL+DESC_TYPE_386_CALL_GATE)
#define  RW_Data_Type   (D_PRES+D_SEG+D_DATA+D_W)
#define  R_Data_Type    (D_PRES+D_SEG+D_DATA+D_R)
#define  Code_Type      (D_PRES+D_SEG+D_CODE+D_RX)

/*********** Definitions for the Extra rights byte in a descriptor ***********/

#define  D_GRAN_BYTE	000	       /* Segment length is byte granular */
#define  D_GRAN_PAGE    0x80           /* Segment length is page granular */
#define  D_DEF16        000            /* Default operation size is 16 bits */
#define  D_DEF32        0x40           /* Default operation size is 32 bits */

#define  D_PAGE32	(D_GRAN_PAGE+D_DEF32)	   /* 32 bit Page granular */

/************ Definitions for the DWord Count byte in a call gate ************/

#define CALLGATE_DDCOUNT_MASK	    0x1f    // dword count < 32

/************************* Masks for selector fields *************************/

#define  SELECTOR_MASK  0xfff8         /* selector index */
#define  SEL_LOW_MASK   0xf8           /* mask for low byte of sel indx */
#define  TABLE_MASK     0x04           /* table bit */
#define  RPL_MASK       0x03           /* privilige bits */
#define  RPL_CLR        ~0x03          /* clear ring bits */

/***************************** PAGE TABLE EQUATES ****************************/

#define P_SIZE      0x1000      /* page size */

#define P_PRESBIT   0
#define P_PRES      (1 << P_PRESBIT)
#define P_WRITEBIT  1
#define P_WRITE     (1 << P_WRITEBIT)
#define P_USERBIT   2
#define P_USER      (1 << P_USERBIT)
#define P_ACCBIT    5
#define P_ACC       (1 << P_ACCBIT)
#define P_DIRTYBIT  6
#define P_DIRTY     (1 << P_DIRTYBIT)

#define P_AVAIL     (P_PRES+P_WRITE+P_USER) /* avail to user & present */

//+-------------------------------------------------------------------------+//
//+			   End of 386 Structures			    +//
//+-------------------------------------------------------------------------+//

//+-------------------------------------------------------------------------+//
//+		      Prototypes of routines in Ring0.c			    +//
//+-------------------------------------------------------------------------+//

#ifdef __cplusplus
extern "C" {
#endif

// X86 processor information services
WORD R0GetLdtr(void);
DWORD R0GetGdtBase(void);
WORD R0GetGdtLimit(void);
DWORD R0GetIdtBase(void);
WORD R0GetIdtLimit(void);
WORD R0GetCS();
DWORD R0GetDescBase(PDESCRIPTOR pDesc);
DWORD R0GetDescLimit(PDESCRIPTOR pDesc);
WORD R0GetDescRights(PDESCRIPTOR pDesc);
WORD R0StealSelector(PDESCRIPTOR pXDT, WORD wLimit);
void R0ReleaseSelector(PDESCRIPTOR pXDT, WORD wSel);

// Ring 0 Callback mechanism
DWORD R0CallCallBack(R0CALLBACK pCallBack, DWORD dwRef);

// VMM services front ends
DWORD R0GetVmmVersion(void);
DWORD R0GetCurVmHandle(void);
WORD R0AllocLdtSelector(int iCount);
void R0FreeLdtSelector(WORD wSel);
DWORD R0BeginReentrantExecution(void);
void R0EndReentrantExecution(DWORD dwCount);
void *R0HeapAllocate(ULONG nBytes, ULONG flags);	// Allocates a block of memory in the non-pageable pool
DWORD R0HeapFree(void *hAddress, ULONG flags);		// Frees a block of memory in the non-pageable pool

// A set of experimental functions that currently are useless or don't work.
#ifdef _DEBUG
// VMM services front ends
void R0SaveClientState(struct Client_Reg_Struc *pBuf);
void R0RestoreClientState(struct Client_Reg_Struc *pBuf);
void R0BeginNestV86Exec(void);
void R0EndNestExec(void);

// Other VxD services front ends
DWORD R0V86mmgrAllocateBuffer(DWORD dwSize, void *pBuf);
void R0V86mmgrFreeBuffer(DWORD dwSize, void *pBuf);

// Other compound services
DWORD R0CallInt13(DWORD dwEAX, DWORD dwECX, DWORD dwEDX, DWORD dwESBX); // WARNING: Does not work!
#endif // defined(_DEBUG)

#ifdef __cplusplus
}
#endif

#pragma pack()	/* Restore the default packing size */

#endif // _RING0_H_
