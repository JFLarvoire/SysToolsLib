/*****************************************************************************\
*                                                                             *
*   Filename:	    RelocMgr.c						      *
*									      *
*   Description:    RESID segment relocation manager.			      *
*                                                                             *
*   Notes:	    Creates and manages new instances of the RESID segment.   *
*									      *
*   History:								      *
*    2001/08/20 JFL Created this file.					      *
*									      *
*      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"		// "resident" attribute definition

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    RunRelocated					      |
|									      |
|   Description:    Execute a routine in the relocated instance of RSEG.      |
|									      |
|   Parameters:     void *pProc		Routine offset relative to wSeg       |
|		    WORD wSeg		Routine segment			      |
|									      |
|   Returns:	    The routine's optional return value, in DX:AX.	      |
|									      |
|   Notes:	    The two arguments on stack form a far pointer to the      |
|		    relocated instance of the routine.			      |
|		    Maybe this can be exploited eventually.		      |
|									      |
|		    To do: Manage routines with arguments.		      |
|									      |
|   History:								      |
|									      |
|    2001/08/02 JFL Created this routine				      |
|    2001/08/07 JFL Added support for case when the RSEG segment is not at    |
|		    offset 0 within the program image.			      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

void resident __cdecl __far RelocationRelay(void *pProc)
    {
    __asm
        {
        call	word ptr pProc
        }
    }

DWORD __cdecl RunRelocated(void *pProc, WORD wSeg)
    {
    __asm
        {
        push	ds
        mov	ds, wSeg	; Relocated instance will assume DS=CS.
        push	pProc		; Put parameter on stack
        ; Put on stack the far return address of routine RelocationRelay().
        push	cs
        push	offset return_from_RSEG
        ; Put on stack the address of the relocated instance of RelocationRelay().
        push	ds
        push	offset RelocationRelay
        ; Jump to that relocated instance.
        retf
return_from_RSEG:
        pop	cx		; Remove pProc parameter from stack
        pop	ds
        }
#pragma warning(disable:4035)   // Ignore the No return value... warning
    }
#pragma warning(default:4035)   // Ignore the No return value... warning

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FixRelocationParallax				      |
|									      |
|   Description:    Get segment to use for accessing relocated RESID instance.|
|									      |
|   Parameters:     WORD wReloc		Segment of the relocated instance     |
|					0 = Use local instance.		      |
|									      |
|   Returns:	    WORD wSeg		Adjusted base segment		      |
|									      |
|   Notes:	    This routine is useful when the RESID segment is not at   |
|		    the beginning of the program image.			      |
|		    In this case, resident data and code offsets being	      |
|		    relative to the _TEXT base, it is necessary to use a      |
|		    dummy relocated base to access them using the same	      |
|		    offsets.						      |
|									      |
|   History:								      |
|									      |
|    2001/08/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

WORD FixRelocationParallax(WORD wReloc)
    {
    if (!wReloc)       // If using local instance of resident code, then no parallax.
        {
	return GetCS();
        }
    else               // Else if using another instance, then adjust for RESID parallax.
        {
       	return wReloc - BYTE2PARA((WORD)&Beg_of_RSEG);        // Parallax
        }
    }

