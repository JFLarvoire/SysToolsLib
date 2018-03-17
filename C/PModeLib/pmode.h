/*****************************************************************************\
*									      *
*   File name:	    pmode.h						      *
*									      *
*   Description:    PMODE.LIB global definitions			      *
*									      *
*   Notes:	    Many of the structures and constants below are from 386.H *
*		    by Alex Shmidt, Nov 1993. There is no copyright. This is  *
*		    part of source files published in an article on RINGO, a  *
*		    VxD loader for Windows 3.1.                               *
*		    							      *
*   History:								      *
*    1995-02-07 JFL Created by Jean-François LARVOIRE			      *
*    1995-09-18 JFL Updated the definition for RM2PMAndCallBack		      *
*    2015-10-27 JFL Added the generation of a library search record.	      *
*    2018-03-15 JFL Added workaround for warnings when running h2inc.exe.     *
*		    							      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef WINAPI
#include "windef.h"
#endif

#ifndef __PMODE_H__   // Prevent multiple inclusions
#define __PMODE_H__

/* Force linking with the lodos.lib library */
#define _PMODE_LIB "pmode.lib"
#pragma message("Adding pragma comment(lib, \"" _PMODE_LIB "\")")
#pragma comment(lib, _PMODE_LIB)

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------+//
//+				Structures				    +//
//+-------------------------------------------------------------------------+//

#pragma pack(1)

typedef struct
    {
    _WORD Limit_0_15;	    // Limit bits (0..15)
    _WORD Base_0_15;	    // Base bits (0..15)
    _BYTE Base_16_23;	    // Base bits (16..23)
    _BYTE Access_Rights;    // 286 access rights
    _BYTE Extra_Rights;	    // 386 extra access rights
    _BYTE Base_24_31;	    // Base bits (24..31)
    } DESCRIPTOR, *PDESCRIPTOR, far *LPDESCRIPTOR;

typedef struct
    {
    _WORD Offset_0_15;	    // Entry point's offset, lower 16
    _WORD Selector;	    // Entry point's selector
    _BYTE DWord_Count;	    // (D)Word parameter count
    _BYTE Access_Rights;    // Present, dpl, system, type
    _WORD Offset_16_31;	    // Entry point's offset, upper 16
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
//+			       386 Instructions 			    +//
//+-------------------------------------------------------------------------+//

#ifndef _H2INC			    // Prevent warning by H2INC

/* More concise definitions for data emission within the code stream */

#define DB(x) __asm _emit x			// BYTE
#define DW(x) DB((x) & 0xFF) DB((x) >> 8U)	// WORD
#define DD(x) DW((x) & 0xFFFF) DW((x) >> 16U)	// DWORD

/* Instructions not supported by the Microsoft C compiler 8.0 */

#define MOV_EAX_CR0 DB(0x0F) DB(0x20) DB(0xC0)
#define MOV_CR0_EAX DB(0x0F) DB(0x22) DB(0xC0)

#define PAGINATION_OFF MOV_EAX_CR0 \
		       DW(0x2566) DD(0x7FFFFFFFL) /* and eax, 7FFFFFFFH */ \
		       MOV_CR0_EAX

#define PAGINATION_ON  MOV_EAX_CR0 \
		       DW(0x0D66) DD(0x80000000L) /* or eax, 80000000H */ \
		       MOV_CR0_EAX

/* Instructions for use within _asm blocks */

#define DATASIZE DB(0x66)	    // Prefix changes next instr. data size
#define ADRSIZE DB(0x67)	    // Prefix changes next instr. address size

#define cwde	DATASIZE cbw	    // EAX = AX, sign extended
#define wbinvd	DW(090FH)

/* Allow to assemble simple 32 bits instructions directly */

#define eax ax
#define ebx bx
#define ecx cx
#define edx dx
#define esi si
#define edi di
#define esp sp
#define ebp bp

// Use to hide 32-bits addressing encoding
#define at_eax bx+si
#define at_ebx bp+di
#define at_ecx bx+di
#define at_edx bp+si
#define at_edi bx
// at_esi cannot be defined since encoding depends on offset
#define at_ebp di   // Warning: Use only for arguments. Never use [at_ebp+0].

// Pentium instructions

#define rdmsr DW(0x320F)
#define wrmsr DW(0x300F)

#define enable_probe_mode   \
	mov	cx, 1DH     \
	DW(0x8000)	    \
	rdmsr		    \
	or	al, 1	    \
	wrmsr

#endif // ifndef _H2INC

//+-------------------------------------------------------------------------+//
//+			 External functions prototypes			    +//
//+-------------------------------------------------------------------------+//

/* Intrinsic (Faster & more compact) */

#ifndef _disable
void _cdecl _disable(void);
void _cdecl _enable(void);
#pragma intrinsic(_enable, _disable)
// #define _disable() _asm cli
// #define _enable() _asm sti
#endif

/* In .asm files. Call specifiers _cdecl or _fastcall MUST be provided */

#ifndef _H2INC	// h2inc.exe throws away _fastcall declarations with a warning
extern int _fastcall is_a20_enabled(void);
extern int _fastcall isa_enable_a20(void);
extern int _fastcall isa_disable_a20(void);
#endif

typedef int (_fastcall far *LPXMS)();	    // Ellipsis refused by compiler!
#ifndef _H2INC	// h2inc.exe throws away _fastcall declarations with a warning
extern LPXMS _fastcall GetXMSAddress(void);
#endif
extern LPXMS lpXMS;
extern short wlpXMSValid;
#define GetXMSVersion() lpXMS(0x0000)	    // Return version in BCD
#define xms_enable_a20() lpXMS(0x0500)	    // Return TRUE if succeeded
#define xms_disable_a20() lpXMS(0x0600)     // Restore A20's previous state
#define xms_query_a20() lpXMS(0x0700)	    // Return 0=Disabled 1=Enabled

typedef _WORD (_cdecl *PPROTCALLBACK)(PDESCRIPTOR, _WORD);
extern int _cdecl RM2PMAndCallBack(PPROTCALLBACK, _WORD, _WORD *);

extern _DWORD _cdecl _sgdt(void);    // Use in all modes.
extern _WORD _cdecl _sldt(void);     // Use only in PM
#ifndef _H2INC	// h2inc.exe throws away _fastcall declarations with a warning
extern _DWORD _fastcall GetSegmentBase(_WORD); // Get the base of a real mode segment
#endif
extern _DWORD _cdecl GetV86LinearAddress(void far *); // Get a linear address
extern _DWORD _cdecl ReturnEAX(void);	    // Copy EAX to DX:AX

extern int _cdecl identify_processor(void); // 0=8086, 1=80186, 2=80286, etc...

/* In .c files. Functions with a variable number of arguments must be _cdecl */

extern int enable_a20(void);	    // Return TRUE if succeeded
extern int disable_a20(void);	    // Return TRUE if succeeded

/* Idemt for functions with many large size arguments */

int _cdecl FlatCopy(_DWORD dwDest, _DWORD dwSource, _DWORD dwLength);
_DWORD _cdecl MapPhysToLinear(_DWORD dwBase, _DWORD dwLength, _DWORD dwFlags);

//+-------------------------------------------------------------------------+//
//+			   VCPI functions prototypes			    +//
//+-------------------------------------------------------------------------+//

extern int _cdecl vcpi_detect(void);	    // Return 0 if VCPI present
extern void _cdecl vcpi_cleanup(void);	    // Cleanup alloc. by vcpi_detect

typedef _WORD (_cdecl *PVCPICALLBACK)(PDESCRIPTOR, _WORD);
extern int _cdecl VCPI2PMAndCallBack(PVCPICALLBACK, _WORD, _WORD *);

extern int _cdecl vm2real(void);	    // Return 0 if done

typedef _WORD (_cdecl *PRMCALLBACK)(_WORD);
extern int _cdecl VCPI2RMAndCallBack(PRMCALLBACK, _WORD, _WORD *);

extern int _cdecl vm2prot(void);
#define vcpi2prot vm2prot		    // Consistent with dpmi2prot()

//+-------------------------------------------------------------------------+//
//+			   DPMI functions prototypes			    +//
//+-------------------------------------------------------------------------+//

extern int _cdecl dpmi_detect(void);	  // Return 0 if a DPMI server is there
extern int _cdecl dpmi2prot(void);	  // Switch to protected mode
extern _WORD _cdecl GetFlatDataDesc(void); // Get the 4 GB flat data segment sel.
extern _WORD _cdecl GetLDTSelfDesc(void);  // Set an LDT entry to access the LDT
extern _DWORD _cdecl GetPMLinearAddress(void far *); // Get a linear address

typedef _WORD (_cdecl *PDPMICALLBACK)(_WORD);
extern int _cdecl VM2PMAndCallBack(PDPMICALLBACK, _WORD, _WORD *);

typedef _DWORD (_pascal *PRING0CALLBACK)(_DWORD);
typedef _DWORD (_pascal far *LPRING0CALLBACK)(_DWORD);
extern _DWORD PM2Ring0AndCallBack(PRING0CALLBACK, _DWORD);

extern _DWORD PM2AppyAndCallBack(PRING0CALLBACK pCB, void *pParams,
						    _DWORD *pdwRetVal);
/* Windows-compatible functions */

UINT WINAPI AllocSelector(UINT);	  // Allocate a selector in LDT
UINT WINAPI FreeSelector(UINT); 	  // Free a selector in LDT

_DWORD WINAPI GetSelectorBase(UINT);	  // Get the base of a prot mode segment
UINT WINAPI SetSelectorBase(UINT, _DWORD); // Set the base of a prot mode segment
_DWORD WINAPI GetSelectorLimit(UINT);	  // Get the limit of a prot mode seg.
UINT WINAPI SetSelectorLimit(UINT, _DWORD); // Set the limit of a prot mode seg.

UINT WINAPI GlobalPageLock(HGLOBAL);	  // Lock a 4 KB page in memory
UINT WINAPI GlobalPageUnlock(HGLOBAL);	  // Unlock a 4 KB page from memory

_DWORD WINAPI GlobalDosAlloc(_DWORD);	  // Allocate a block of DOS memory
UINT WINAPI GlobalDosFree(UINT);	  // Free a block of DOS memory

//+-------------------------------------------------------------------------+//
//+									    +//
//+-------------------------------------------------------------------------+//

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // end if __PMODE_H__INCLUDED
