/*****************************************************************************\
*                                                                             *
*   Filename	    FDisk95.cpp						      *
*									      *
*   Description	    Windows 9X-specific floppy disk access routines	      *
*                                                                             *
*   Notes	    This code implements the OS-independant floppy disk I/O   *
*		    routines for Windows 95/98/ME.			      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*		    Sectors are referenced by their DWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through Virtual Machine   *
*		    Manager's I/O subsystem, doing VWIN32.VxD DeviceIoControl *
*		    calls to VWIN32_DIOC_DOS_INT13.			      *
*		    Note that this DeviceIoControl supports floppys, but not  *
*		    hard disks.						      *
*		    							      *
*		    TO DO: Performance may be improved by reading/writing     *
*		    more than one sector at a time. Ralph Brown list reports  *
*		    that fct 42/43 can safely read/write up to 127 at a time. *
*		    							      *
*   History								      *
*    2017-07-17 JFL Created this file.					      *
*    2017-08-02 JFL Use fcts 2/3/8 instead of function 42H/43H/48H.	      *
*    									      *
*        (C) Copyright 2017 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "FloppyDisk.h"

#include "VxDCall.h"
#include "int13.h"
#include <malloc.h>

// #ifdef _DEBUG
#include <stdio.h>
// #endif // _DEBUG

// Structure behind a FloppyDisk95 handle
typedef struct {
  HANDLE hVWin32;
  int iBiosDrive;
  int iMode;
  WORD wCyls;
  BYTE bHeads;
  BYTE bSects;
} FLOPPYDISK95;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FloppyDisk95Open					      |
|									      |
|   Description	    Get a handle for a given floppy disk.		      |
|									      |
|   Parameters	    int iDisk	    Floppy Disk number. 0=1st floppy, etc.    |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns	    The floppy disk handle, or NULL if no such floppy disk.   |
|									      |
|   Notes	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History								      |
|    2017-07-30 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE FloppyDisk95Open(int iDrive, int iMode) {
  FLOPPYDISK95 *pFD95 = NULL;
  int iErr;
  FDGEOMETRY fdGeometry;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDisk95Open(iDrive=%d, iMode=%x)\n", iDrive, iMode);
  }
#endif // _DEBUG

  pFD95 = (FLOPPYDISK95 *)calloc(sizeof(FLOPPYDISK95), 1);
  if (!pFD95) goto FloppyDisk95Open_exit;
  pFD95->iBiosDrive = iDrive;
  pFD95->iMode = iMode;

  pFD95->hVWin32 = OpenVWin32();
  if (pFD95->hVWin32 == INVALID_HANDLE_VALUE) {
    free(pFD95);
    pFD95  = NULL;
    goto FloppyDisk95Open_exit;
  }

  iErr = FloppyDisk95GetGeometry((HANDLE)pFD95, &fdGeometry);
  if (iErr == INT_13_ERR_NO_MEDIA_IN_DRIVE) iErr = 0; // This is not an error for open().
  if (iErr) {
    FloppyDisk95Close((HANDLE)pFD95);
    pFD95  = NULL;
    goto FloppyDisk95Open_exit;
  }

FloppyDisk95Open_exit:
#ifdef _DEBUG
    if (iDebug) {
      printf("  return %p\n", pFD95);
    }
#endif // _DEBUG
  return (HANDLE)pFD95;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FloppyDisk95Close					      |
|									      |
|   Description	    Release resources reserved by FloppyDiskOpen.	      |
|									      |
|   Parameters	    HANDLE hDisk    Floppy Disk handle.			      |
|									      |
|   Returns	    Nothing.						      |
|									      |
|   Notes	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History								      |
|    2017-07-30 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void FloppyDisk95Close(HANDLE hDrive) {
  FLOPPYDISK95 *pFD95 = (FLOPPYDISK95 *)hDrive;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDisk95Close(%p)\n", pFD95);
  }
#endif // _DEBUG

  if (pFD95) {
    if (pFD95->hVWin32) CloseVWin32(pFD95->hVWin32);
    free(pFD95);
  }
  return;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FloppyDisk95GetGeometry				      |
|									      |
|   Description	    Get the geometry of the floppy disk.		      |
|									      |
|   Parameters	    HANDLE hDrive		Specifies the floppy disk.    |
|		    FDGEOMETRY *pFdGeometry	Buffer for the results.	      |
|									      |
|   Returns	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History								      |
|    2017-07-30 JFL Created this routine				      |
|    2017-08-02 JFL Use fct 08 instead of function 48H, which does not work.  |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDisk95GetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry) {
  FLOPPYDISK95 *pFD95 = (FLOPPYDISK95 *)hDrive;
  DIOC_REGISTERS regs;
  int iErr;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDisk95GetGeometry(hDrive=%p, Buf@=%p)\n", hDrive, pFdGeometry);
  }
#endif // _DEBUG

  // Use function 08H, which is supportted by all systems AND Virtualized by VWIN32
  regs.reg_EAX = 0x0800;		// AH = 08H - Get Drive Parameters
  regs.reg_EDX = pFD95->iBiosDrive;	// DL = Drive #
  regs.reg_EDI = 0;			// Allows checking if the drive parameter table is set

  iErr = DIOC_Int13(pFD95->hVWin32, &regs);
  if (iErr) goto FloppyDisk95GetGeometry_exit;

  pFD95->wCyls = ((WORD0(regs.reg_ECX) & 0x00C0) << 2) + BYTE1(regs.reg_ECX) + 1;
  pFD95->bHeads = BYTE1(regs.reg_EDX) + 1;
  pFD95->bSects = BYTE0(regs.reg_ECX) & 0x003F;
  pFdGeometry->wCyls = pFD95->wCyls;
  pFdGeometry->wHeads = pFD95->bHeads;
  pFdGeometry->wSects = pFD95->bSects;
  pFdGeometry->dwSectors = (DWORD)pFdGeometry->wCyls * (pFdGeometry->wHeads * pFdGeometry->wSects);
  pFdGeometry->wSectorSize = 0x200; // Default is 512 if we don't know any better
  if (regs.reg_EDI) {
    pFdGeometry->wSectorSize = 0x80 << ((BYTE *)(regs.reg_EDI))[3];
  }

  // Then, check if a floppy is actually in the drive.
  // For lack of  better idea, try reading the boot sector, to detect if a floppy is actually there!
  void *pBuf = malloc(pFdGeometry->wSectorSize);
  if (!pBuf) goto FloppyDisk95GetGeometry_exit;
  iErr = FloppyDisk95Read(hDrive, 0, 1, pBuf);
  free(pBuf);
  if (iErr) iErr = INT_13_ERR_NO_MEDIA_IN_DRIVE;

FloppyDisk95GetGeometry_exit:
#ifdef _DEBUG
    if (iDebug) {
      printf("  return 0x%X\n", iErr);
    }
#endif // _DEBUG
  return iErr;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FloppyDisk95Read					      |
|									      |
|   Description	    Read N sectors from the physical disk (Win9X version).    |
|									      |
|   Parameters	    HANDLE hDrive		Specifies the floppy disk.    |
|		    DWORD dwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History								      |
|    2017-07-30 JFL Created this routine				      |
|    2017-08-02 JFL Use fct 02 instead of function 42H, which does not work.  |
*									      *
\*---------------------------------------------------------------------------*/

void FloppyDisk95Lba2Chs(HANDLE hDrive, DWORD dwSector, DWORD *pECX, DWORD *pEDX) {
  FLOPPYDISK95 *pFD95 = (FLOPPYDISK95 *)hDrive;
  BYTE bSect = (BYTE)(dwSector % pFD95->bSects) + 1;
  dwSector /= pFD95->bSects;
  BYTE bHead = (BYTE)(dwSector % pFD95->bHeads);
  dwSector /= pFD95->bHeads;
  WORD wCyl = (WORD)dwSector;

  BYTE0(*pEDX) = BYTE0(pFD95->iBiosDrive);
  BYTE1(*pEDX) = bHead;
  WORD1(*pEDX) = 0;
  BYTE0(*pECX) = bSect + (BYTE1(wCyl) << 6);
  BYTE1(*pECX) = BYTE0(wCyl);
  WORD1(*pECX) = 0;
}

int FloppyDisk95Read(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  FLOPPYDISK95 *pFD95 = (FLOPPYDISK95 *)hDrive;
  int iErr = 0;
  WORD wCount;
  DIOC_REGISTERS regs = {0};

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDisk95Read(hDrive=%p, LBA=0x%lX, N=0x%X, Buf@=%p)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
#endif // _DEBUG

  for (wCount = 0; wCount < wNum; wCount++, dwSector++, pBuf = (BYTE *)pBuf + 512) {
    regs.reg_EAX = 0x0201;		// AH = 02H (Read Sectors) ; AL # of sectors
    regs.reg_EBX = (DWORD)pBuf;		// Buffer address
    FloppyDisk95Lba2Chs(hDrive, dwSector, &regs.reg_ECX, &regs.reg_EDX);
    iErr = DIOC_Int13(pFD95->hVWin32, &regs);
    if (iErr) {
#ifdef _DEBUG
      if (iDebug) {
	printf("VWIN32 failed to run int 13H fct 02H. Error 0x%X\n", iErr);
      }
#endif // _DEBUG
      goto FloppyDisk95Read_exit;
    }
  }

FloppyDisk95Read_exit:
#ifdef _DEBUG
    if (iDebug) {
      printf("  return 0x%X\n", iErr);
    }
#endif // _DEBUG
  return iErr;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FloppyDiskWrite					      |
|									      |
|   Description	    Write N sectors to the floppy disk.			      |
|									      |
|   Parameters	    HANDLE hDrive		Specifies the floppy disk.    |
|		    DWORD dwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History								      |
|    2017-07-30 JFL Created this routine				      |
|    2017-08-02 JFL Use fct 03 instead of function 43H, which does not work.  |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDisk95Write(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  FLOPPYDISK95 *pFD95 = (FLOPPYDISK95 *)hDrive;
  int iErr = 0;
  WORD wCount;
  DIOC_REGISTERS regs = {0};

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDisk95Write(hDrive=%p, LBA=0x%lX, N=0x%X, Buf@=%p)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
  if (iReadOnly) printf("  // Read-only mode! Write canceled.\n");
#endif // _DEBUG
  if (iReadOnly) { // But do not report an error, because this is used for testing all writes to be done.
    iErr = 0;
    goto FloppyDisk95Write_exit;
  }
  if (pFD95->iMode & 1) {	// This is a real error
    iErr = 3;				// BIOS Write protect error.
    goto FloppyDisk95Write_exit;
  }

  for (wCount = 0; wCount < wNum; wCount++, dwSector++, pBuf = (BYTE *)pBuf + 512) {
    regs.reg_EAX = 0x0301;		// AH = 03H (Write Sectors) ; AL # of sectors
    regs.reg_EBX = (DWORD)pBuf;		// Buffer address
    FloppyDisk95Lba2Chs(hDrive, dwSector, &regs.reg_ECX, &regs.reg_EDX);
    iErr = DIOC_Int13(pFD95->hVWin32, &regs);
    if (iErr) {
#ifdef _DEBUG
      if (iDebug) {
	printf("VWIN32 failed to run int 13H fct 03H. Error 0x%X\n", iErr);
      }
#endif // _DEBUG
      goto FloppyDisk95Write_exit;
    }
  }

FloppyDisk95Write_exit:
#ifdef _DEBUG
    if (iDebug) {
      printf("  return 0x%X\n", iErr);
    }
#endif // _DEBUG
  return iErr;
}
