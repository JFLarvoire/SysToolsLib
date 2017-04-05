/*****************************************************************************\
*                                                                             *
*   File name:	    vm2pm.c						      *
*									      *
*   Description:    Switch to protected mode and back.			      *
*									      *
*   Notes:	    Uses DPMI.						      *
*									      *
*   History:								      *
*    1996/10/08 JFL Created this file					      *
*									      *
*      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "pmode.h"
#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    VM2PMAndCallBack					      |
|									      |
|   Description:    Switch to 16-bits prot mode, and call back. 	      |
|									      |
|   Parameters:     PDPMICALLBACK pCB	Address of the DPMI callback	      |
|		    WORD wParam 	Parameter to pass to the callback     |
|		    WORD *pwRetVal	Where to store the returned value     |
|									      |
|   Returns:	    0 = Switch done; !0 = Switch failed.		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1996/10/08 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int _cdecl VM2PMAndCallBack(PDPMICALLBACK pCB, WORD wParam, WORD *pwRetVal)
    {
    WORD wOldCS;
    WORD wOldDS;
    int iErr;

    wOldCS = GetCS();
    wOldDS = GetDS();

    /* Switch to protected mode */

    iErr = dpmi2prot();
    if (iErr) return iErr;

    *pwRetVal = pCB(wParam);

    /* Switch back to real mode */

    _asm
	{
	push	si
	push	di
	mov	ax, 306H
	int	31H		; Get address of the raw mode switch
	mov	iErr, 4 	; Assume failure
	jc	fin
	mov	iErr, 0 	; success
	push	si		; Push the real mode switch routine address
	push	di

	call	suite
suite:
	pop	di		; Ofset of label "suite" relative to code group
	sub	di, offset suite ; Offset of CODE segment in code group
	add	di, offset fin	; New IP
	mov	si, wOldCS	; New CS
	mov	ax, wOldDS	; New DS
	mov	cx, wOldDS	; New ES
	mov	dx, wOldDS	; New SS
	mov	bx, sp		; New SP
	add	bx, 4		; Pop SI & DI from the new stack
	retf			; Switch to real mode. Preserves BP.
fin:
	pop	di
	pop	si
	}

    return iErr;
    }

#pragma warning(default:4704)	// Ignore the inline assembler etc... warning
