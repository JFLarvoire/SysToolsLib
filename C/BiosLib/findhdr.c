/*****************************************************************************\
*                                                                             *
*   File name:	    findhdr.c						      *
*									      *
*   Description:    Find a DMI of PnP descriptor in the BIOS		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/08/05 JFL Extracted from PNP.C.				      *
*									      *
*      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"

#define NULL ((void *)0)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FindHeader						      |
|									      |
|   Description:    Find the DMI or PnP descriptor structure in the BIOS.     |
|									      |
|   Parameters:     DWORD dwSignature					      |
|									      |
|   Returns:	    far pointer to the header, or NULL if not found.	      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/06/13 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

LPVOID FindHeader(DWORD dwExpected)
    {
    LPVOID lp;
    int i;

    for (lp = (LPVOID)0xF0000000L; lp < (LPVOID)0xF000FFF0L; (LPBYTE)lp += 0x10)
	{
	LPPNPHEADER lpPossibleHdr = lp;

	if (lpPossibleHdr->dwSignature == dwExpected)
	    {
	    LPBYTE lpb;
	    BYTE b;

	    /* Check the header checksum */
	    lpb = lp;
	    for (i=b=0; i<lpPossibleHdr->bHeaderLength; i++) b += lpb[i];
	    if (b) continue;	/* Invalid checksum */

	    return lp;		/* Found */
	    }
	}

    return NULL;
    }
