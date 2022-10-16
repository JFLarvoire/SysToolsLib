/*****************************************************************************\
*                                                                             *
*   Filename:	    PcUuid.c						      *
*									      *
*   Description:    Get the UUID of the PC				      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2016-04-27 JFL Split this file off of uuid.c.			      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "uuid.h"
#include "smbios.h"

#define FALSE 0
#define TRUE 1

#ifndef NULL
  #ifdef __cplusplus
    #define NULL 0
  #else
    #define NULL ((void *)0)
  #endif
#endif

#ifdef _DEBUG
extern int iDebug;	/* Defined in main module. If TRUE, display debug information */
#endif /* _DEBUG */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetPcUuid						      |
|									      |
|   Description:    Get the UUID of the PC.				      |
|									      |
|   Parameters:     void *pBuf		Where to store the 16-bytes UUID.     |
|									      |
|   Returns:	    TRUE if found, FALSE if not found.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/09/02 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetPcUuid(uuid_t *pBuf)
    {
    HSMBIOS hSMBIOS;
    PSMBIOSSYSTEMINFO pSystemInfo = NULL;	/* Mark it as invalid. */
    int iFound = FALSE;
    int i;

    if (   ((hSMBIOS = SmBiosOpen()) != NULL)
        && (SmBiosVersion(hSMBIOS) >= 0x201)	/* SM-BIOS found and version >= 2.1 */
        && ((pSystemInfo = (PSMBIOSSYSTEMINFO)SmBiosAllocStruct(hSMBIOS)) != NULL)
        && (SmBiosGetStructByType(hSMBIOS, 1, pSystemInfo) >= 0)
        && (pSystemInfo->bLength >= 24)
       ) {
	BYTE b;

	iFound = TRUE;	/* Assume found... */

	/* ... but UUIDs with all 16 zeros or all 16 0xFFs are invalid. */
	b = 0;
	for (i=0; i<16; i++) b |= pSystemInfo->bUUID[i];
	if (b == 0) iFound = FALSE;

	b = 0xFF;
	for (i=0; i<16; i++) b &= pSystemInfo->bUUID[i];
	if (b == 0xFF) iFound = FALSE;

	if (iFound) memcpy(pBuf, pSystemInfo->bUUID, 16);
	}

#ifdef _DEBUG
    if (iDebug && iFound)
    	{
    	printf("PC UUID = ");
    	PrintUuid(pBuf);
    	printf("\n");
    	}
#endif /* _DEBUG */

    free(pSystemInfo);	/* NULL pointers are ignored. */
    SmBiosClose(hSMBIOS);

    return iFound;
    }

