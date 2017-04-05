/*****************************************************************************\
*                                                                             *
*   File name:	    MapP2L.c						      *
*									      *
*   Description:    Map physical to linear addresses			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1997/09/09 JFL Extracted from older programs.			      *
*    1999/02/19 JFL Make MapPhysToLinear bi-modal: Callable in V86 and PM16.  *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "pmode.h"
#include "lodos.h"	// For CGroupOffset()

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MapPhysToLinear					      |
|									      |
|   Description:    MapPhysical memory to linear memory 		      |
|									      |
|   Parameters:     DWORD dwBase	Physical memory base		      |
|		    DWORD dwLength	Number of bytes to map		      |
|		    DWORD dwFlags	Flags. Must be 0.		      |
|									      |
|   Returns:	    0=Success; -1=Failure				      |
|                                                                             |
|   Notes:	    Calls VMM _MapPhysToLin service			      |
|		    Must be called from a virtual DOS machine under VMM.      |
|                                                                             |
|   History:								      |
|									      |
|    1995/11/10 JFL Created this routine				      |
|    1999/02/19 JFL Make MapPhysToLinear bi-modal: Callable in V86 and PM16.  |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

DWORD _pascal MPTL32(DWORD dwRef)
    {
    _asm
	{
	push ds
	push es
	push ss
	push ss
	pop es
	pop ds

	DB(0x8B);
	DB(0x5D);
	DB(0x0C);		// mov	ebx, dwRef

	DB(0xFF);
	DB(0x73);
	DB(0x08);		// push [ebx+8]

	DB(0xFF);
	DB(0x73);
	DB(0x04);		// push [ebx+4]

	DB(0xFF);
	DB(0x33);		// push [ebx]

	int 20H 		// Dynamic Link to...
	DD(0x0001006C); 	// _MapPhysToLinear. Returns result in EAX.

	add sp, 12
	pop es
	pop ds
	leave
	retf 4
	DW(0);
	}
#pragma warning(disable:4035)   // Ignore the no return value warning
    }
#pragma warning(default:4035)

#pragma warning(default:4704)	// Ignore the inline assembler etc... warning

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning
int InProtMode(void)
    {
    MOV_EAX_CR0;
    _asm and ax, 1;
#pragma warning(disable:4035)   // Ignore the no return value warning
    }
#pragma warning(default:4035)

#pragma warning(default:4704)	// Ignore the inline assembler etc... warning

WORD _cdecl PmMapPhysToLinear(WORD wRef)
    {
    DWORD *psParms;

    psParms = (DWORD *)wRef;
    // Switch to 32-bits Ring 0 to get the result
    psParms[0] = PM2Ring0AndCallBack(CGroupOffset(MPTL32), psParms[1]);
    return 0;
    }

DWORD _cdecl MapPhysToLinear(DWORD dwBase, DWORD dwLength, DWORD dwFlags)
    {
    if (InProtMode())	// If in protected mode
	{
	// Switch to 32-bits Ring 0 immediately to get the result
	return PM2Ring0AndCallBack(CGroupOffset(MPTL32), GetPMLinearAddress(&dwBase));
	}
    else		// Else in V86 mode
	{
	struct
	    {
	    DWORD dwLin;
	    DWORD dwParms;
	    } sParms;
	WORD wRet;

	sParms.dwParms = GetV86LinearAddress(&dwBase);
	// Switch first to 16-bits Protected Mode
	VM2PMAndCallBack(CGroupOffset(PmMapPhysToLinear), (WORD)(&sParms), &wRet);
	return sParms.dwLin;
	}
    }
