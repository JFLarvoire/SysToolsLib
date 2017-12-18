/*****************************************************************************\
*									      *
*   File name:	    fmemcmp.c						      *
*									      *
*   Description:    Compare far memory blocks				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/07/08 JFL Created this file.					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

typedef unsigned char BYTE;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fmemcmp						      |
|									      |
|   Description:    Compare two far blocks of memory			      |
|									      |
|   Parameters:     Standard C library					      |
|									      |
|   Returns:	    0 = Equal. Else different.				      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/11/04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _fmemcmp(void far *lp1, void far *lp2, size_t l)
    {
    int iDiff;

    while (l--)
	{
	iDiff = (int)*(((BYTE far *)lp1)++) - (int)*(((BYTE far *)lp2)++);
	if (iDiff) return iDiff;
	}

    return 0;
    }
