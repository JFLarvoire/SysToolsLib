/*****************************************************************************\
*									      *
*   File name:	    dosver.c						      *
*									      *
*   Description:    Get MS-DOS version					      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1996/08/08 JFL Created this file					      *
*									      *
*      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _dos_version					      |
|									      |
|   Description:    Get the MS-DOS version				      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    AX = MS-DOS version. Major version in AH; minor in AL.    |
|                                                                             |
|   History:								      |
|									      |
|    1996/08/08 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

WORD _fastcall _dos_version(void)
    {
    _asm
	{
	mov	ah, 30h
	int	21h
	xchg	al, ah
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Ignore the no return value warning

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning
