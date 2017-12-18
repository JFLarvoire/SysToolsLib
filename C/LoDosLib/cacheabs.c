/*****************************************************************************\
*									      *
*   File name:	    cacheabs.c						      *
*									      *
*   Description:    Cached Absolute Disk Read/Write			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/09/05 JFL Created this file					      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "msdos.h"
#include "lodos.h"

/* Global variables */

int iCachedDrive = -1;		    /* >= 0 if dp is valid */
DWORD dwCachedSector = NO_SECTOR;
int iCacheDirty = FALSE;
char cSectorCache[1024];
WORD wCachedSectorSize;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CachedAbsDiskRead					      |
|									      |
|   Description:    Read data from the disk, caching the last sector read     |
|									      |
|   Parameters:     int iDrive	    The drive number. 0=A, 1=B, 2=C, etc...   |
|		    DWORD dwSect    The sector number within the partition    |
|		    WORD wOffset    The offset within the sector	      |
|		    WORD wLength    The amount of data to read		      |
|		    void *pBuf	    Where to store the data		      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|                                                                             |
|   Notes:	    The cache is shared with the write function 	      |
|                                                                             |
|		    Works across multiple sectors.			      |
|                                                                             |
|   History:								      |
|									      |
|    1994/08/29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl CachedAbsDiskRead(int iDrive, DWORD dwSector, WORD wOffset,
			WORD wLength, void *pBuf)
    {
    int err;
    DEVICEPARAMS dpCache;
    DEVICEPARAMS *pdp = &dpCache;
    WORD wToCopy;

    if (iDrive != iCachedDrive)
	{
	if (iCacheDirty) CachedAbsDiskFlush();
	iCachedDrive = -1;  /* Assume failure reading the drive parameters */
	err = GetDeviceParams(iDrive, pdp);
	if (err) return err;			    /* Access denied */
	if (pdp->dpBytesPerSec > 1024) return 5;    /* Prevent cache overflow */
	wCachedSectorSize = pdp->dpBytesPerSec;
	iCachedDrive = iDrive;	 /* Flag success: Drive parameters are valid. */
	dwCachedSector = NO_SECTOR;	  // Force reading the new drive
	}

    /* Prevent errors */
    if (wOffset >= wCachedSectorSize) return 5;

    while (wLength)
	{
	if (dwSector != dwCachedSector)   // Don't read twice the same sector
	    {
	    if (iCacheDirty) CachedAbsDiskFlush();
	    err = AbsDiskRead(iDrive, dwSector, 1, cSectorCache);
	    if (err) return err;
	    dwCachedSector = dwSector;
	    }

	wToCopy = min(wLength, wCachedSectorSize - wOffset);
	memcpy(pBuf, cSectorCache+wOffset, wToCopy);

	dwSector += 1;
	wOffset = 0;
	wLength -= wToCopy;
	(char *)pBuf += wToCopy;
	}

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CachedAbsDiskWrite					      |
|									      |
|   Description:    Write data to the CachedAbsDiskRead cache		      |
|									      |
|   Parameters:     int iDrive	    The drive number. 0=A, 1=B, 2=C, etc...   |
|		    DWORD dwSect    The sector number within the partition    |
|		    WORD wOffset    The offset within the sector	      |
|		    WORD wLength    The amount of data to write 	      |
|		    void *pBuf	    Where to get the data from		      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|                                                                             |
|   Notes:	    The cache is shared with the read function. 	      |
|		    Use CachedAbsDiskFlush to make sure the data is written.  |
|		    It is automatically written in case of an access to       |
|		    another sector.					      |
|                                                                             |
|		    Works across multiple sectors.			      |
|                                                                             |
|   History:								      |
|									      |
|    1994/08/29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl CachedAbsDiskWrite(int iDrive, DWORD dwSector, WORD wOffset,
			WORD wLength, void *pBuf)
    {
    int err;
    WORD wToCopy;
    char c;

    /* Make sure the drive is valid, and the sector size is set */
    /* wLength=0 ==> CachedAbsDiskRead will NOT read the sector */
    err = CachedAbsDiskRead(iDrive, 0, 0, 0, NULL);
    if (err) return err;

    /* Prevent errors */
    if (wOffset >= wCachedSectorSize) return 5;

    while (wLength)
	{
	if (dwSector != dwCachedSector)
	    {
	    if (iCacheDirty) CachedAbsDiskFlush();
	    }

	wToCopy = min(wLength, wCachedSectorSize - wOffset);

	if (wToCopy < wCachedSectorSize) /* Less than a full sector? */
	    {	/* May be true for the first and last sectors */
	    /* Force reading the sector into the cache */
	    /* wLength=1 ==> CachedAbsDiskRead WILL read the sector */
	    err = CachedAbsDiskRead(iDrive, dwSector, 0, 1, (char *)&c);
	    if (err) return err;
	    }

	memcpy(cSectorCache+wOffset, pBuf, wToCopy);

	dwCachedSector = dwSector;
	iCacheDirty = TRUE;

	dwSector += 1;
	wOffset = 0;
	wLength -= wToCopy;
	(char *)pBuf += wToCopy;
	}

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CachedAbsDiskFlush					      |
|									      |
|   Description:    Flsuh the CachedAbsDiskRead cache			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|                                                                             |
|   Notes:	    Use CachedAbsDiskFlush to make sure the data is written   |
|                                                                             |
|   History:								      |
|									      |
|    1994/08/29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl CachedAbsDiskFlush(void)
    {
    int err;

    if (!iCacheDirty) return 0;

    err = AbsDiskWrite(iCachedDrive, dwCachedSector, 1, cSectorCache);
    if (!err) iCacheDirty = FALSE;
    return err;
    }
