/*****************************************************************************\
*                                                                             *
*   Filename:	    Block.cpp						      *
*									      *
*   Description:    OS-independant block device access routines		      *
*                                                                             *
*   Notes:	    Gives access to hard disks through the HardDiskXxxx()     *
*		    family of routines, and to files through the FileXxxx()   *
*		    family of routines.					      *
*		    All those routines are usable from both C and C++.	      *
*									      *
*   History:								      *
*    2001-02-20 JFL Created this file.					      *
*    2001-03-19 JFL Made the routines usable both in C and C++.		      *
*    2002-02-07 JFL Added support for Logical Volumes. (A:, C:, etc...)	      *
*    2002-04-08 JFL Create the target file if needed.			      *
*    2008-03-20 JFL Include <ctype.h> for toupper definition.		      *
*    2008-04-21 JFL Added WIN32 64-bits file I/O functions.		      *
*    2008-04-22 JFL Moved all file I/O to separate subroutines.		      *
*    2016-04-13 JFL Also allow specifying hard disk names as hdN:             *
*    2017-07-15 JFL Added support for floppys.		   		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>	/* For toupper */

#include "Block.h"
#include "HardDisk.h"
#include "LogDisk.h"
#include "File.h"
#include "FloppyDisk.h"

#ifndef NEW
#ifndef __cplusplus
// Convenient macros for C allowing to make it look like C++.
#define NEW(object) ((object *)malloc(sizeof(object)))
#define DEL(object) free(object)
#else
// Same macros as above, for code compilable as both C and C++.
#define NEW(object) (new object)
#define DEL(object) (delete object)
#endif // __cplusplus
#endif // NEW

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockOpen						      |
|									      |
|   Description:    Open a file or a disk for I/O.			      |
|									      |
|   Parameters:     char *pszName		File, device or volume name.  |
|		    char *pszMode               I/O mode. Same as for fopen().|
|									      |
|   Returns:	    HANDLE hDevice		Identifies the block device.  |
|		    				NULL = Failure.		      |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
|    2001-09-07 JFL Added pszMode processing for hard disks.		      |
|    2002-02-07 JFL Added support for Logical Volumes. (A:, C:, etc...)	      |
|    2008-04-22 JFL Moved file support to FileXxxx subroutines.               |
|    2016-04-13 JFL Also allow specifying hard disk names as hdN:             |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE BlockOpen(char *pszName, char *pszMode)
    {
    SBLOCKDEVICE *pDevice = NEW(SBLOCKDEVICE);
    int iDrive;
    char cDrive;
    char cColon;
    int nFields;
    HANDLE hDrive;
    char cExtra;
    int iLength;
    char c0, c1;
    int iMode = READONLY;
    if (strpbrk(pszMode, "wa+")) iMode = READWRITE;

    if (!pDevice) return NULL;

    // Is this a file or a device? (Device names end with a ':')
    iLength = (int)strlen(pszName);
    if (iLength && (pszName[iLength-1] != ':')) goto its_a_file;

    // First check if it's a logical volume such as "A:" or "C:".
    nFields = sscanf(pszName, "%c%c%c", &cDrive, &cColon, &cExtra);
    cDrive = (char)toupper(cDrive);
    if (   (nFields == 2) && (cColon == ':')
	&& (cDrive >= 'A') && (cDrive <= 'Z') )
        {
	BPB bpb;

        hDrive = LogDiskOpen(cDrive, iMode);
        if (!hDrive)
            {
            DEL(pDevice);
            return NULL;
            }

	LogDiskGetBPB(hDrive, &bpb);

        pDevice->iType = BT_LOGICALVOLUME;
	pDevice->iBlockSize = (int)(bpb.bpbBytesPerSec);
	if (!strncmp(bpb.bpbOemName, "NTFS", 4))
	    pDevice->qwBlocks = bpb.bpbNTFSSectors;
	else
	    pDevice->qwBlocks = bpb.bpbSectors ? bpb.bpbSectors : bpb.bpbHugeSectors;
	pDevice->h = hDrive;

	return (HANDLE)pDevice;
        }

    // If not, check if it's a Linux-style hd0: or hd1: hard disk name
    nFields = sscanf(pszName, "%c%c%d%c", &c0, &c1, &iDrive, &cColon);
    if (   (nFields == 4) && (cColon == ':')
        && ((c0 == 'h') || (c0 == 'H'))
        && ((c1 == 'd') || (c1 == 'D')) )
        {
its_a_hard_disk:
	HDGEOMETRY sHdGeometry;

        hDrive = HardDiskOpen(iDrive, iMode);
        if (!hDrive)
            {
            DEL(pDevice);
            return NULL;
            }

	HardDiskGetGeometry(hDrive, &sHdGeometry);

        pDevice->iType = BT_HARDDISK;
	pDevice->iBlockSize = (int)(sHdGeometry.wSectorSize);
	pDevice->qwBlocks = sHdGeometry.qwSectors;
	pDevice->h = hDrive;

	return (HANDLE)pDevice;
        }

    // If not, check if it's a Linux-style fd0: or fd1: floppy disk name
    if (   (nFields == 4) && (cColon == ':')
        && ((c0 == 'f') || (c0 == 'F'))
        && ((c1 == 'd') || (c1 == 'D')) ) {
its_a_floppy_disk:
	FDGEOMETRY sFdGeometry;

        hDrive = FloppyDiskOpen(iDrive, iMode);
        if (!hDrive) {
            DEL(pDevice);
            return NULL;
        }

	FloppyDiskGetGeometry(hDrive, &sFdGeometry);

        pDevice->iType = BT_FLOPPYDISK;
	pDevice->iBlockSize = (int)(sFdGeometry.wSectorSize);
	pDevice->qwBlocks = sFdGeometry.dwSectors;
	pDevice->h = hDrive;

	return (HANDLE)pDevice;
        }

    // As an alternative, check if it's a BIOS-style hard disk number, such as "80:" or "81:" 
    nFields = sscanf(pszName, "%x%c%c", &iDrive, &cColon, &cExtra);
    if ((nFields == 2) && (cColon == ':')) {
      if (iDrive < 0x80) {
        goto its_a_floppy_disk;
      } else {
	iDrive -= 0x80;	// Convert the DOS-style drive number to a hard disk index.
        goto its_a_hard_disk;
      }
    }

    // If not, assume it's a plain file name.
its_a_file:
    pDevice->h = FileOpen(pszName, iMode);
    if (!pDevice->h)
	{
	DEL(pDevice);
	return NULL;
	}
    pDevice->iType = BT_FILE;
    pDevice->iBlockSize = 1;
    pDevice->qwBlocks = FileSize(pDevice->h);

    return (HANDLE)pDevice;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockClose						      |
|									      |
|   Description:    Close a file or a disk for I/O.			      |
|									      |
|   Parameters:     HANDLE hDevice		Identifies the device.	      |
|		    							      |
|   Returns:	    Nothing						      |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
|    2002-02-07 JFL Added support for Logical Volumes.			      |
|    2008-04-22 JFL Moved file support to FileXxxx subroutines.               |
*									      *
\*---------------------------------------------------------------------------*/

void BlockClose(HANDLE hDevice)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return;

    switch (pDevice->iType)
    	{
    	case BT_FILE:
    	    FileClose(pDevice->h);
    	    break;
    	case BT_HARDDISK:
    	    HardDiskClose(pDevice->h);
    	    break;
    	case BT_LOGICALVOLUME:
    	    LogDiskClose(pDevice->h);
    	    break;
    	case BT_FLOPPYDISK:
    	    FloppyDiskClose(pDevice->h);
    	    break;
    	default:	// Unsupported device type
    	    break;
    	}

    DEL(pDevice);
    return;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockCount						      |
|									      |
|   Description:    Get the number of blocks in the device.		      |
|									      |
|   Parameters:     HANDLE hDevice		Identifies the device.	      |
|		    							      |
|   Returns:	    QWORD qwCount					      |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

QWORD BlockCount(HANDLE hDevice)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return qwZero;

    return pDevice->qwBlocks;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockSize						      |
|									      |
|   Description:    Get the device block size.				      |
|									      |
|   Parameters:     HANDLE hDevice		Identifies the device.	      |
|		    							      |
|   Returns:	    int iSize						      |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int BlockSize(HANDLE hDevice)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return 0;

    return pDevice->iBlockSize;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockType						      |
|									      |
|   Description:    Get the device block type.				      |
|									      |
|   Parameters:     HANDLE hDevice		Identifies the device.	      |
|		    							      |
|   Returns:	    int iType						      |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int BlockType(HANDLE hDevice)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return 0;

    return pDevice->iType;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockIndexName					      |
|									      |
|   Description:    Device-dependant name for the block index number.	      |
|									      |
|   Parameters:     HANDLE hDevice		Identifies the device.	      |
|		    							      |
|   Returns:	    Either "offset" for files (accessible at the BYTE level)  |
|                   or "sector" for disks (accessible at the sector level)    |
|		    							      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

char *BlockIndexName(HANDLE hBlock)
    {
    return (BlockSize(hBlock) > 1) ? "sector" : "offset";
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockRead						      |
|									      |
|   Description:    Read N sectors from a file or a disk.		      |
|									      |
|   Parameters:     HANDLE hDevice		Specifies the input device.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
|    2002-02-07 JFL Added support for Logical Volumes.			      |
|    2008-04-22 JFL Moved file support to FileXxxx subroutines.               |
*									      *
\*---------------------------------------------------------------------------*/

int BlockRead(HANDLE hDevice, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return 1;

    switch (pDevice->iType)
    	{
    	case BT_FILE:
    	    return FileRead(pDevice->h, qwSector, wNum, pBuf);
    	case BT_HARDDISK:
    	    return HardDiskRead(pDevice->h, qwSector, wNum, pBuf);
    	case BT_LOGICALVOLUME:
    	    return LogDiskRead(pDevice->h, qwSector, wNum, pBuf);
    	case BT_FLOPPYDISK:
    	    return FloppyDiskRead(pDevice->h, (DWORD)qwSector, wNum, pBuf);
    	default:	// Unsupported device type
    	    return 1;
    	}
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BlockWrite						      |
|									      |
|   Description:    Write N sectors into a file or a disk.		      |
|									      |
|   Parameters:     HANDLE hDevice		Specifies the input device.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the BlockXxxx family.    |
|									      |
|   History:								      |
|    2001-02-14 JFL Created this routine				      |
|    2002-02-07 JFL Added support for Logical Volumes.			      |
|    2008-04-22 JFL Moved file support to FileXxxx subroutines.               |
*									      *
\*---------------------------------------------------------------------------*/

int BlockWrite(HANDLE hDevice, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    SBLOCKDEVICE *pDevice = BlockPtr(hDevice);

    if (!hDevice) return 1;

    switch (pDevice->iType)
    	{
    	case BT_FILE:
    	    return FileWrite(pDevice->h, qwSector, wNum, pBuf);
    	case BT_HARDDISK:
    	    return HardDiskWrite(pDevice->h, qwSector, wNum, pBuf);
    	case BT_LOGICALVOLUME:
    	    return LogDiskWrite(pDevice->h, qwSector, wNum, pBuf);
    	case BT_FLOPPYDISK:
    	    return FloppyDiskWrite(pDevice->h, (DWORD)qwSector, wNum, pBuf);
    	default:	// Unsupported device type
    	    return 1;
    	}
    }

