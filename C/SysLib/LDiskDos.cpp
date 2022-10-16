/*****************************************************************************\
*                                                                             *
*   Filename:	    LDiskDos.cpp					      *
*									      *
*   Description:    MS-DOS logical disk access routines			      *
*                                                                             *
*   Notes:	    This code implements the OS-independant logical disk I/O  *
*		    routines for MS-DOS.				      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through DOS int 25H/26H.  *
*									      *
*		    TO DO: Implement Read-Only mode in LogDiskOpen().	      *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*    2002/04/15 JFL Added write protection flag in handle bit 15.	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "LogDisk.h"	// Public definitions for this module.

#include <memory.h>	// For memset().
#include <stdlib.h>	// For toupper().
#include <stdint.h>	// For intptr_t

// Important: All MS_DOS data structures must be packed on a 
// one-byte boundary. 

#pragma pack(1)

#ifndef __MSDOS_H__

typedef struct tagDISKIO
    {
    DWORD diStartSector;	/* Sector number to start	       */
    WORD  diSectors;		/* Number of sectors		       */
    char FAR *diBuffer; 	/* Address of buffer		       */
    }
    DISKIO;

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskOpen						      |
|									      |
|   Description:    Get a handle for a given logical disk.		      |
|									      |
|   Parameters:     char cDisk	    Logical Disk letter. A=flpy; C=hdisk...   |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The logical disk handle, or NULL if no such logical disk. |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
|    2002/04/15 JFL Added write protection flag in handle bit 15.	      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE LogDiskOpen(char cDrive, int iMode)
    {
    int iErr;
    BPB bpb;
    HANDLE hDrive = (HANDLE)(intptr_t)(toupper(cDrive));
    
#ifdef _DEBUG
    if (iDebug)
        {
        printf("LogDiskOpen(cDrive=%c)\n", cDrive);
	}
#endif // _DEBUG
    
    // Check if it exists
    iErr = LogDiskGetBPB(hDrive, &bpb);
    if (iErr) return NULL;

    if (iMode) *(int*)&hDrive |= 0x8000;
    return hDrive;
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskClose					      |
|									      |
|   Description:    Release resources reserved by LogDiskOpen.		      |
|									      |
|   Parameters:     HANDLE hDisk    logical disk handle.		      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100)   // Ignore the unreferenced formal parameter warning

void LogDiskClose(HANDLE hDrive)
    {
    return;
    }

#pragma warning(default:4100)  // Restore the unreferenced formal parameter warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskGetBPB					      |
|									      |
|   Description:    Get the BIOS Parameter Block of the logical disk.	      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    BPB *pBpb			Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|		    WARNING: The hidden sectors field is only the partition   |
|			     base LBA for primary partition. For extended     |
|			     partitions, it is relative to the extension.     |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskGetBPB(HANDLE hDrive, BPB *pBpb)
    {
    int err;
    char buf[512];

    err = LogDiskRead(hDrive, qwZero, 1, &buf);
    if (err) return err;

    memcpy(pBpb, buf, sizeof(BPB));
    
    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskRead						      |
|									      |
|   Description:    Read N sectors from the logical disk.		      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

int LogDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void far *pBuf)
    {
    int err;
    DISKIO dio;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("LogDiskRead(hDrive=%lX, Sector=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
#endif // _DEBUG

    dio.diStartSector = DWORD0(qwSector);
    dio.diSectors = wNum;
    dio.diBuffer = (char far *)pBuf;

    // First try with DOS 7, FAT32 aware version.
    _asm
	{
	mov	ax, 7305H
	mov	cx, 0FFFFH	    ; Use a DISKIO structure */
	mov	dl, byte ptr hDrive
	sub	dl, '@'		    ; 1=A, 2=B, 3=C, etc...
	mov	si, 0		    ; Read unknown data
	lea	bx, dio 	    ; Assumes ds == ss */
	int	21H
	jc	int25xerr
	xor	ax, ax
int25xerr:
	mov	err, ax
	}

    // If this is unsupported by this version of DOS, try the legacy function
    if (err) _asm		    /* Use MS-DOS Absolute Disk Read int 25H */
	{
	mov	al, byte ptr hDrive
	sub	al, 'A'		    /* 0=A, 1=B, 2=C, etc... */
	lea	bx, dio 	    /* Assume ds == ss */
	mov	cx, 0FFFFH	    /* Use a DISKIO structure */
	int	25H
	jc	int25err	    /* Test carry before popping the flags */
	xor	ax, ax
int25err:
	popf
	mov	err, ax
	}

#ifdef _DEBUG
    if (iDebug && err)
        {
        printf("Error %d reading sector.\n", err);
	}
#endif // _DEBUG
    return err;
    }

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskWrite					      |
|									      |
|   Description:    Write N sectors to the logical disk.		      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
|    2002/04/15 JFL Added write protection management.			      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

int LogDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void far *pBuf)
    {
    int err;
    DISKIO dio;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("LogDiskWrite(hDrive=%lX, Sector=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    if (iReadOnly) printf("Read-only mode! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;
    if ((int)(intptr_t)hDrive & 0x8000) return 3;	// Write protection error

    dio.diStartSector = DWORD0(qwSector);
    dio.diSectors = wNum;
    dio.diBuffer = (char far *)pBuf;

    // First try with DOS 7, FAT32 aware version.
    _asm
	{
	mov	ax, 7305H
	mov	cx, 0FFFFH	    ; Use a DISKIO structure */
	mov	dl, byte ptr hDrive
	sub	dl, '@'		    ; 1=A, 2=B, 3=C, etc...
	mov	si, 1		    ; Write unknown data
	lea	bx, dio 	    ; Assumes ds == ss */
	int	21H
	jc	int26xerr
	xor	ax, ax
int26xerr:
	mov	err, ax
	}

    // If this is unsupported by this version of DOS, try the legacy function
    if (err) _asm		    /* Use MS-DOS Absolute Disk Read int 25H */
	{
	mov	al, byte ptr hDrive
	sub	al, 'A'		    /* 0=A, 1=B, 2=C, etc... */
	lea	bx, dio 	    /* Assume ds == ss */
	mov	cx, 0FFFFH	    /* Use a DISKIO structure */
	int	26H
	jc	int26err	    /* Test carry before popping the flags */
	xor	ax, ax
int26err:
	popf
	mov	err, ax
	}

    return err;
    }

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*===========================================================================*\
*                                                                             *
"                          Spare MS-DOS Routines                              "
*                                                                             *
\*===========================================================================*/

#if 0

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
|   Updates:								      |
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
|   Updates:								      |
|									      |
|    1994/09/04 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int UnlockLogicalVolume(int iDrive)
    {
    return DevIoCtl(0x6A, 0, iDrive+1);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    LockDosVolume					      |
|									      |
|   Description:    Lock an MS-DOS volume, for direct sector access.	      |
|									      |
|   Parameters:     int iDosDrive   MS-DOS drive number 		      |
|									      |
|   Output:	    int number of unlocks to call. -1=Failure.		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1996/08/21 JFL initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

int LockDosVolume(int iDosDrive)
    {
    int iErr;
    int i;

    // Useless up to MS-DOS 6.22
    if (_dos_version() < 0x700) return 0;

    // First attempt a level 0 lock
    iErr = LockLogicalVolume(iDosDrive, 0, 1);
    if (!iErr) return 1;	// Success
    if (iErr != 5) return -1;	// Failure

    // If lock denied, then it's necessary to get locks 1 to 3
    for (i=1; i<=3; i++)
	{
	iErr = LockLogicalVolume(iDosDrive, i, 1);
	if (iErr) break;
	}
    if (i>3) return 3;		// We got all three locks. Success.
    // Else undo partial locks
    UnlockDosVolume(iDosDrive, i-1);
    return -1;			// Failure
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    UnlockDosVolume					      |
|									      |
|   Description:    Unlock an MS-DOS volume, for direct sector access.	      |
|									      |
|   Parameters:     int iDosDrive   MS-DOS drive number 		      |
|		    int iUndo	    Number of locks to undo		      |
|									      |
|   Output:	    None						      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1996/08/21 JFL initial implementation.				      |
|    2000/06/06 JFL Added safety against negative iUndo parameters.	      |
*									      *
\*---------------------------------------------------------------------------*/

void UnlockDosVolume(int iDosDrive, int iUndo)
    {
    while (iUndo-- > 0) UnlockLogicalVolume(iDosDrive);
    return;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskLock						      |
|									      |
|   Description:    Lock the logical disk, to allow direct sector access.     |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2000/09/18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskLock(HANDLE hDrive, HANDLE *phLock)
    {
    int iDisk = (int)(DWORD)hDrive;
    int iErr;
    int iUndoLock = 0;		// Number of locks obtained on the drive

    // To do: Change BIOS drive # into MS-DOS drives #.
    
    // Lock the drive. This is necessary for writing a sector.
    if ((iDosDrive >= 0) && (iOrder < 0))
	{
	iUndoLock = LockDosVolume(iDosDrive);
	if (iUndoLock == -1)
	    {
	    // printf("Cannot lock volume %c:\n", cDrive);
	    return -1;
	    }
#ifdef _DEBUG
	if (iVerbose) printf("Obtained %d locks on drive %c.\n", iUndoLock, cDrive);
#endif // _DEBUG
	// The drive is now locked. It must be unlocked before exiting.
	}

    *phLock = (HANDLE)iUndoLock;
    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskUnlock					      |
|									      |
|   Description:    Unlock the logical disk, to disallow direct sector access.|
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    int iUndoLock					      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2000/09/18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskLock(HANDLE hDrive, HANDLE hLock)
    {
    int iDisk = (int)(DWORD)hDrive;
    int iUndoLock = (int)(DWORD)hLock;

    if (iUndoLock > 0) UnlockDosVolume(iDosDrive, iUndoLock);

    return 0;
    }

#endif // Keep sample lock/unlock code hidden.

