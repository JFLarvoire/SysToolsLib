/*****************************************************************************\
*                                                                             *
*   File name:	    getpsp.c						      *
*									      *
*   Description:    Get the Program Segment Prefix segment		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/06/29 JFL Created this file					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetPsp						      |
|									      |
|   Description:    Get the program segment prefix for the current process    |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    The PSP segment					      |
|									      |
|   History:								      |
|									      |
|    1995/06/19 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler warning

WORD GetPsp(void)
    {
    _asm
	{
	mov	ah, 51h
	int	21h
	mov	ax, bx
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)
#pragma warning(default:4704)	// Restore the inline assembler warning
