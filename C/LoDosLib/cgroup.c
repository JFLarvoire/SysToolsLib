/*****************************************************************************\
*                                                                             *
*   File name:	    cgroup.c						      *
*									      *
*   Description:    Manage the code group from C modules		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/11/06 JFL Created this file					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CGroupOffset					      |
|									      |
|   Description:    Convert an offset in the _TEXT segment to one in the CODE |
|		     group.						      |
|									      |
|   Parameters:     void *pCode     Pointer to a routine, relative to _TEXT.  |
|									      |
|   Returns:	    void *	    The same pointer, relative to CS.	      |
|									      |
|   Notes:	    This is useful with LODOS.LIB because the CS register     |
|		    points to the base of the CODE group. This group is made  |
|		    of both the RESID and the _TEXT segments.		      |
|		    Unfortunately, the C compiler doesn't know about this,    |
|		    and only generates offsets relative to the _TEXT segment. |
|		    All function pointers are thus incorrect, if the RESID    |
|		    segment contains more than 15 bytes. To make them valid,  |
|		    this routine adds to a function pointer the difference    |
|		    between the bases of RESID and _TEXT.		      |
|									      |
|   History:								      |
|									      |
|    1995/09/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

void *CGroupOffset(void *pCode)
    {
    _asm
	{
	call	next		// Pushes the return address relative to CS
next:
	pop	ax		// Address of the next: label, relative to CS
	sub	ax, offset next // minus the same address relative to _TEXT
	add	ax, pCode	// plus the requested address relative to _TEXT
	}			// = The requested address relative to CS

#pragma warning(disable:4035)	// Ignore the no return value etc... warning
    }
#pragma warning(default:4035)	// Restore the no return value etc... warning

#pragma warning(default:4704)	// Restore the inline assembler etc... warning
