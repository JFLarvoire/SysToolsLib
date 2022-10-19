/*****************************************************************************\
*                                                                             *
*   Filename:	    PrintUuid.c						      *
*									      *
*   Description:    OS-independant UUID print routine			      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2016-04-27 JFL Split this file off of uuid.c.			      *
*    2021-11-05 JFL Fixed warnings with gcc.				      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "uuid.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PrintUuid						      |
|									      |
|   Description:    Print a UUID in the standard format.		      |
|									      |
|   Parameters:     uuid_t *pUuid	Where to find the 16-bytes UUID.      |
|									      |
|   Returns:	    The number of character printed.  			      |
|									      |
|   Notes:	    The first three fields are a DWORD and two WORDs, and so  |
|		    are traditionally displayed with their bytes swapped.     |
|		    							      |
|   History:								      |
|    2000-09-25 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int PrintUuid(uuid_t *pUuid)
    {
    DWORD *pdw = (DWORD *)pUuid;
    WORD *pw = (WORD *)pUuid;
    int n=0;
    int i;

    n = printf("%08lX-%04X-%04X-", (unsigned long)(pdw[0]), (unsigned int)(pw[2]), (unsigned int)(pw[3]));
    for (i=8; i<10; i++) n += printf("%02X", BYTE_AT(pUuid,i));
    n += printf("-");
    for (i=10; i<16; i++) n += printf("%02X", BYTE_AT(pUuid,i));

    return n;
    }

