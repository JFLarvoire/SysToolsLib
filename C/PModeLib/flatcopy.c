/*****************************************************************************\
*                                                                             *
*   File name:	    flatcopy.c						      *
*									      *
*   Description:    Copy a block of extended memory			      *
*									      *
*   Notes:	    Switches to protected mode, and uses 32-bits block move.  *
*									      *
*   History:								      *
*    1995/11/07 JFL Created this file					      *
*    1997/05/23 JFL Added support for doing in under Windows.		      *
*    1998/03/05 JFL Updated comments.					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "lodos.h"
#include "pmode.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FlatCopy						      |
|									      |
|   Description:    Copy data in between two linear addresses		      |
|									      |
|   Parameters:     DWORD dwDest    Linear address of the destination	      |
|		    DWORD dwSource  Linear address of the source	      |
|		    DWORD dwLength  Number of bytes to copy		      |
|									      |
|   Returns:	    int err	    Non 0 if failed to switch modes	      |
|									      |
|   Notes:	    The A20 line MUST be enabled beforehand for this to work. |
|									      |
|   History:								      |
|									      |
|    1995/09/18 JFL Created this routine				      |
|    1995/11/22 JFL Updated to the new RM2PMAndCallBack specifications.       |
|    1995/12/18 JFL The A20 gate management is now the responsability of the  |
|		     caller.						      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

WORD static _cdecl DoFlatCopy(WORD wFlat, WORD wParam)
    {
    _asm
	{
	mov	bx, wParam
	DATASIZE			; Force using 32-bits registers
	mov	di, WORD ptr [bx+0]
	DATASIZE			; Force using 32-bits registers
	mov	si, WORD ptr [bx+4]
	DATASIZE			; Force using 32-bits registers
	mov	cx, WORD ptr [bx+8]
	mov	ax, wFlat		; Selector to a FLAT 4 GB segment
	mov	ds, ax
	mov	es, ax
	mov	ax, cx
	DATASIZE			; Force using 32-bits registers
	shr	cx, 1
	DATASIZE			; Force using 32-bits registers
	shr	cx, 1
	ADRSIZE 			; Force using 32-bits addresses & count
	DATASIZE			; Force moving 32-bits data
	rep	movsw
	and	ax, 3
	jz	copy_done
	mov	cx, ax			; The high half of ECX is already 0
	ADRSIZE 			; Force using 32-bits addresses & count
	rep	movsb
copy_done:
	}
    return 0;
    }

#pragma warning(default:4704)	// Restore the inline assembler etc... warning

WORD static _cdecl DoDosCopy(PDESCRIPTOR pGDT, WORD wParam)
    {
    DoFlatCopy(0x10, wParam);	// PMode.lib 32-bits flat data selector is 0x10.
    return 0;
    }

WORD static _cdecl DoWinCopy(WORD wParam)
    {
    DoFlatCopy(GetFlatDataDesc(), wParam);
    return 0;
    }

int _cdecl FlatCopy(DWORD dwDest, DWORD dwSource, DWORD dwLength)
    {
    int iRet;

    /* Dive into protected mode to do the copy */
    if (dpmi_detect() == 0)
	{		    // In V86 mode with a DPMI server like Windows
	if (GetVmmVersion())	// Under Win9x,
	    {
	    /* Assume all addresses in the top two gigabytes are physical
	       addresses. Convert them to linear addresses before use.
	       This prevents a GPF */
	    if (dwDest >= 0x80000000)
		dwDest = MapPhysToLinear(dwDest, dwLength, 0);
	    if (dwSource >= 0x80000000)
		dwSource = MapPhysToLinear(dwSource, dwLength, 0);
	    /* printf("Linear = from %08lX to %08lX.\n", dwSource, dwDest); */
	    }
	iRet = VM2PMAndCallBack(CGroupOffset(DoWinCopy), (WORD)&dwDest, NULL);
	}
    else if (vcpi_detect() == 0)
	{		    // In V86 mode with a VCPI server like EMM386
	iRet = VCPI2PMAndCallBack(CGroupOffset(DoDosCopy), (WORD)&dwDest, NULL);
	}
    else
	{		    // In real mode.
	iRet = RM2PMAndCallBack(CGroupOffset(DoDosCopy), (WORD)&dwDest, NULL);
	}

    return iRet;
    }
