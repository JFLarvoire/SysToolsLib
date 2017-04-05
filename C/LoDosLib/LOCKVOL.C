/*****************************************************************************\
*									      *
*   File name:	    lockvol.c						      *
*									      *
*   Description:    Lock/Unlock MS-DOS volumes, using the MS-DOS 7 method     *
*									      *
*   Notes:	    Important: Accessing tracks to disks with more than 36    *
*		    sectors per track using IOCtl calls fails, unless the     *
*		    device parameters for the drives are updated with an      *
*		    extended device parameters structure (DEVICEPARAMSEX).    *
*									      *
*		    This structure must be filled with a trivial track layout,*
*		    that is consecutive entries with numbers from 1 to max,   *
*		    and size always 512.				      *
*									      *
*   History:								      *
*    1995/09/05 JFL Created this file					      *
*    1996/08/08 JFL Added MS-DOS version testing in LockLogicalVolume().      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    DevIoCtl						      |
|									      |
|   Description:    Simple device IOCtrl calls				      |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|									      |
|   Notes:								      |
|                                                                             |
|   History:								      |
|									      |
|    1994/08/30 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)

static int _fastcall DevIoCtl(int function, WORD wDx, WORD wBx)
    {
    _asm
	{
	mov	cl, al
	mov	ax, 440DH
	mov	ch, 8
	int	21H
	jc	skip_xor
	xor	ax, ax
skip_xor:
	}
#pragma warning(disable:4035)
    }
#pragma warning(default:4035)

#pragma warning(default:4704)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LockLogicalVolume					      |
|									      |
|   Description:    Perform a logical volume lock			      |
|									      |
|   Parameters:     int iDrive	    DOS drive number. 0=A; 1=B; 2=C; etc...   |
|		    WORD wLockLevel 0, 1, 2, or 3.			      |
|		    WORD wPermissions bit 0 = Allow writes		      |
|				      bit 1 = Prevent file mapping (?)	      |
|				      bit 2 = Allow formatting		      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|									      |
|   Notes:	    See the Win32 SDK, Guides, Programmer's Guide to Win 95,  |
|		     Using MS-DOS extensions, Exclusive Volume Locking.       |
|                                                                             |
|   History:								      |
|									      |
|    1994/09/04 JFL Created this routine.				      |
|    1996/08/08 JFL Added MS-DOS version testing.			      |
*									      *
\*---------------------------------------------------------------------------*/

int LockLogicalVolume(int iDrive, WORD wLockLevel, WORD wPermissions)
    {
    if (_dos_version() < 0x700) return 1;   // Unsupported before Win95.
    return DevIoCtl(0x4A, wPermissions, (wLockLevel << 8) | (BYTE)(iDrive+1));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    UnlockLogicalVolume 				      |
|									      |
|   Description:    Perform a logical volume unlock			      |
|									      |
|   Parameters:     int iDrive	    DOS drive number. 0=A; 1=B; 2=C; etc...   |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|									      |
|   Notes:	    See the Win32 SDK, Guides, Programmer's Guide to Win 95,  |
|		     Using MS-DOS extensions, Exclusive Volume Locking.       |
|                                                                             |
|   History:								      |
|									      |
|    1994/09/04 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int UnlockLogicalVolume(int iDrive)
    {
    return DevIoCtl(0x6A, 0, iDrive+1);
    }
