/*****************************************************************************\
*                                                                             *
*   Filename:	    FDiskDos.cpp					      *
*									      *
*   Description:    MS-DOS floppy disk access routines			      *
*                                                                             *
*   Notes:	    This code implements the OS-independant floppy disk I/O   *
*		    routines for MS-DOS.				      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*		    Sectors are referenced by their DWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through the BIOS.	      *
*		    16-bits-BIOS-dependant routines are called BiosXxxxx().   *
*									      *
*		    TO DO: Implement Read-Only mode in FloppyDiskOpen().      *
*									      *
*   History:								      *
*    2017-07-28 JFL Created this file.					      *
*									      *
*         © Copyright 2017 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "FloppyDisk.h"	// Public definitions for this module.

#include <stdint.h>	// For intptr_t
#include <malloc.h>

#ifndef MINICOMS
#include <memory.h>	// For memset().
#endif

#include "int13.h"	// BIOS interrupt 13H definitions.

#define ISECT0 1	// Index of the first sector in a C/H/S track.

// Important: All MS_DOS data structures must be packed on a
// one-byte boundary.

#ifndef __MSDOS_H__
#pragma pack(1)
typedef struct _MID {
    WORD  midInfoLevel;
    DWORD midSerialNum;
    char  midVolLabel[11];
    char  midFileSysType[8];
} MID, *PMID;
#pragma pack()
#endif // !defined(__MSDOS_H__)

/*===========================================================================*\
*                                                                             *
"                     OS-independant family, for MS-DOS                       "
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskOpen					      |
|									      |
|   Description:    Get a handle for a given floppy disk.		      |
|									      |
|   Parameters:     int iDisk	    Floppy Disk number. 0=1st floppy, etc.    |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The floppy disk handle, or NULL if no such floppy disk.   |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE FloppyDiskOpen(int iDrive, int iMode) {
  int iErr;
  // DDPARMS fdParms;
  FDGEOMETRY fdGeometry;
  HANDLE hDrive = (HANDLE)(intptr_t)iDrive;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskOpen(iDrive=%d)\n", iDrive);
  }
#endif // _DEBUG

  // TO DO: Test if a disk is present, to avoid timeouts

  // Check if it exists
  // iErr = GetBiosDiskParameterTable(iDrive, &fdParms, sizeof(fdParms));
  iErr = FloppyDiskGetGeometry(hDrive, &fdGeometry);
  if (iErr == INT_13_ERR_NO_MEDIA_IN_DRIVE) iErr = 0; // This is not an error for open().
  if (iErr) return NULL;

  if (iMode) WORD0(hDrive) |= 0x8000;	// Report R/O flag
  return hDrive;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskClose					      |
|									      |
|   Description:    Release resources reserved by FloppyDiskOpen.	      |
|									      |
|   Parameters:     HANDLE fDisk    Floppy Disk handle.			      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100) /* Avoid warnings "unreferenced formal parameter" */

void FloppyDiskClose(HANDLE hDrive) {
  return;
}

#pragma warning(default:4100)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskGetGeometry				      |
|									      |
|   Description:    Get the geometry of the floppy disk			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    FDGEOMETRY *pFdGeometry	Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskGetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iErr;
  DDPARMS fdParms;

  // Get the physical parameters for the drive.
  fdParms.lpedd = (EDDPARMS far *)-1L;
  iErr = GetBiosDiskParameterTable(iDisk, &fdParms, sizeof(fdParms));
  if (iErr) goto FloppyDiskGetGeometry_exit;

  pFdGeometry->dwSectors = (DWORD)fdParms.qwTotal;
  pFdGeometry->wSectorSize = fdParms.wBpS;
  pFdGeometry->wCyls = (WORD)fdParms.dwCyls;
  pFdGeometry->wHeads = (WORD)fdParms.dwHeads;
  pFdGeometry->wSects = (WORD)fdParms.dwSects;

  // Then, check if a floppy is actually in the drive.
  // For lack of a better idea, try reading the boot sector, to detect if a floppy is actually there!
  {
  void *pBuf = malloc(pFdGeometry->wSectorSize);
  if (!pBuf) goto FloppyDiskGetGeometry_exit;
  iErr = FloppyDiskRead(hDrive, 0, 1, pBuf);
  free(pBuf);
  }
  if (iErr) iErr = INT_13_ERR_NO_MEDIA_IN_DRIVE;

FloppyDiskGetGeometry_exit:
#if _DEBUG && NEEDED
  // TO DO: Display extended parameters for the curious. (If any?)
  if (iVerbose && (fdParms.lpedd != (EDDPARMS far *)-1L)) {
  }
#endif // _DEBUG

  return iErr;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskLBA2CHS					      |
|									      |
|   Description:    Convert LBA coordinates to Cylinder/Head/Sector.	      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    DWORD dwSector              LBA Sector number.            |
|		    WORD *pCyl                  Cylinder	              |
|		    WORD *pHead                 Head    	              |
|		    WORD *pSect                 Sector  	              |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskLBA2CHS(HANDLE hDrive, DWORD dwSector, WORD *pwCyl, WORD *pwHead, WORD *pwSect) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iErr;
  DDPARMS fdParms;

  // Get the BIOS parameters
  iErr = GetBiosDiskChsParameters(iDisk, &fdParms, sizeof(fdParms));
  if (iErr) return iErr;

  *pwSect = (WORD)(dwSector % fdParms.dwSects) + 1;
  dwSector /= fdParms.dwSects;
  *pwHead = (WORD)(dwSector % fdParms.dwHeads);
  dwSector /= fdParms.dwHeads;
  *pwCyl = (WORD)(dwSector % fdParms.dwCyls);
  dwSector /= fdParms.dwCyls;

  return (dwSector != 0) ? 256 : 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskRead					      |
|									      |
|   Description:    Read N sectors from the floppy disk.		      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    DWORD dwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskRead(HANDLE hDrive, DWORD dwSector, WORD wNum, void far *pBuf) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iErr;
  WORD wCyl;			/* Cylinder */
  WORD wHead;			/* Head */
  WORD wSect;			/* Sector */

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskRead(hDrive=%p, LBA=%lX, N=%X, Buf@=%Fp)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
#endif // _DEBUG

  // First use int 13H function 42H for LBA access.
  iErr = BiosDiskReadLba(iDisk, dwSector, wNum, pBuf);
  if (iErr != 1) return iErr;	// Success, or disk I/O error.

  /* If the BIOS does not support LBA access, use CHS parameters. */
  iErr = FloppyDiskLBA2CHS(hDrive, dwSector, &wCyl, &wHead, &wSect);
  if (iErr) return iErr;

  iErr = BiosDiskReadChs((WORD)iDisk, wCyl, wHead, wSect, wNum, (char far *)pBuf);
  return iErr;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskWrite					      |
|									      |
|   Description:    Write N sectors to the floppy disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    DWORD dwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskWrite(HANDLE hDrive, DWORD dwSector, WORD wNum, void far *pBuf) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iErr;
  WORD wCyl;			/* Cylinder */
  WORD wHead;			/* Head */
  WORD wSect;			/* Sector */

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskWrite(hDrive=%p, LBA=%lX, N=%X, Buf@=%Fp)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
  // if (iReadOnly) { printf("Read-only mode! Write canceled.\n"); return 0; }
  // Unnecessary as the Write security is actually done in BiosXxxx() routines.
#endif // _DEBUG

  // First use int 13H function 43H for LBA access.
  iErr = BiosDiskWriteLba(iDisk, dwSector, wNum, pBuf);
  if (iErr != 1) return iErr;	// Success, or disk I/O error.

  /* If the BIOS does not support LBA access, use CHS parameters. */
  iErr = FloppyDiskLBA2CHS(hDrive, dwSector, &wCyl, &wHead, &wSect);
  if (iErr) return iErr;

  iErr = BiosDiskWriteChs((WORD)iDisk, wCyl, wHead, wSect, wNum, (char far *)pBuf);
  return iErr;
}

#if NEEDED

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskLock					      |
|									      |
|   Description:    Lock the floppy disk, to allow direct sector access.      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskLock(HANDLE hDrive, HANDLE *phLock) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iErr;
  int iUndoLock = 0;		// Number of locks obtained on the drive

  // To do: Change BIOS drive # into MS-DOS drives #.

  // Lock the drive. This is necessary for writing a sector.
  if ((iDosDrive >= 0) && (iOrder < 0)) {
    iUndoLock = LockDosVolume(iDosDrive);
    if (iUndoLock == -1) {
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
|   Function:	    FloppyDiskUnlock					      |
|									      |
|   Description:    Unlock the floppy disk, to disallow direct sector access. |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    int iUndoLock					      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-28 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskLock(HANDLE hDrive, HANDLE hLock) {
  int iDisk = (int)(DWORD)(LPVOID)hDrive & 0x7F;
  int iUndoLock = (int)(DWORD)hLock;

  if (iUndoLock > 0) UnlockDosVolume(iDosDrive, iUndoLock);

  return 0;
}

#endif // Keep sample lock/unlock code hidden.

