/*****************************************************************************\
*									      *
*   File name:	    memset.c						      *
*									      *
*   Description:    Fill a block of memory				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/07/08 JFL Created this file.					      *
*									      *
*      (c) Copyright 1999-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    memset						      |
|									      |
|   Description:    Fill memory with a constant value			      |
|									      |
|   Parameters:     void *pbuf		Pointer to the buffer to fill	      |
|		    int c		Character to set		      |
|		    size_t len		buffer length			      |
|									      |
|   Returns:	    None						      |
|									      |
|   Notes:	    Standard C library					      |
|									      |
|   History:								      |
|									      |
|    1999/07/01 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

void memset(void *pBuf, int c, size_t len)
    {
    _asm
	{
	push	ds
	pop	es
	mov	di, pBuf
	mov	al, BYTE ptr c
	mov	cx, len
	jcxz	skip
	rep	stosb
skip:
	}
    }

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning
