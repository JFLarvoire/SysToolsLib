/*****************************************************************************\
*                                                                             *
*   Filename:	    FDiskNT.cpp						      *
*									      *
*   Description:    Windows NT-specific floppy disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant floppy disk I/O   *
*		    routines for Windows NT/2000/XP.			      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*		    Sectors are referenced by their DWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through file I/O to       *
*		    \\.\X: device drivers. (In the absence of a \\.\FloppyN   *
*		    symbolic link)					      *
*		    							      *
*   History:								      *
*    2017-07-17 JFL Created this file.					      *
*									      *
*         © Copyright 2017 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>

#include "FloppyDisk.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskNTOpen					      |
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
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE FloppyDiskNTOpen(int iDrive, int iMode) {
  HANDLE hDrive;
  char szDriveName[32];
  DWORD dwAccessMode;
  DWORD dwShareMode;
  char cDrive;
  int iFound;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskNTOpen(iDrive=%d, iMode=%x)\n", iDrive, iMode);
  }
#endif // _DEBUG

  // Find the DOS link for Floppy N
  iFound = FALSE;
  for (cDrive = 'A'; cDrive <= 'Z'; cDrive++) {
    DWORD dwSize;
    int i;
    char szDosDevice[3] = "A:";
    szDosDevice[0] = cDrive;
    dwSize = QueryDosDevice(szDosDevice, szDriveName, sizeof(szDriveName));
    if (!dwSize) continue; // No such device
    if (sscanf(szDriveName, "\\Device\\Floppy%d", &i) && (i == iDrive)) {
      iFound = TRUE;
      break;
    }
  }
  if (!iFound) return NULL; // Not found
  sprintf(szDriveName, "\\\\.\\%c:", cDrive);

  dwAccessMode = GENERIC_READ;
  dwShareMode = FILE_SHARE_READ;
  if (!(iMode & READONLY)) {	// If any write mode is requested
    dwAccessMode |= GENERIC_WRITE;
    dwShareMode |= FILE_SHARE_WRITE;
  }

  hDrive = CreateFile (szDriveName,			// device name
		       dwAccessMode,			// access mode
		       dwShareMode,			// share mode
		       NULL,				// Default security
		       OPEN_EXISTING,			//
		       0,				// file attributes
		       NULL);				// No template

  if (hDrive == INVALID_HANDLE_VALUE) {
#ifdef _DEBUG
    if (iDebug) {
      printf("  return NULL (%s not found or not accessible)\n", szDriveName);
    }
#endif // _DEBUG
    return NULL;
  }

  return hDrive;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskNTClose					      |
|									      |
|   Description:    Release resources reserved by FloppyDiskNTOpen.	      |
|									      |
|   Parameters:     HANDLE hDisk    Floppy Disk handle.			      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void FloppyDiskNTClose(HANDLE hDrive) {
  CloseHandle(hDrive);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskNTGetGeometry				      |
|									      |
|   Description:    Get the geometry of the floppy disk.		      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.    |
|		    FDGEOMETRY *pFdGeometry	Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family.
|									      |
|   History:								      |
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#include <winioctl.h>

int FloppyDiskNTGetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry) {
  DISK_GEOMETRY dg;
  DWORD dwOut;
  BOOL bDone;

  // First check if a drive is present, to avoid a timeout below
  bDone = DeviceIoControl(hDrive,			 // Handle to device
                          IOCTL_STORAGE_CHECK_VERIFY2,   // dwIoControlCode
                          NULL, 0,			 // No input buffer
                          NULL, 0,			 // No output buffer
			  &dwOut,			 // number of bytes returned
			  NULL);			 // Synchronous I/O
  if (!bDone) return (int)GetLastError();

  // Get the physical parameters for the drive.
  bDone = DeviceIoControl(hDrive,			 // Handle to device
			  IOCTL_DISK_GET_DRIVE_GEOMETRY, // dwIoControlCode
			  NULL,                          // lpInBuffer
			  0,                             // nInBufferSize
			  (LPVOID)&dg,			 // Output buffer
			  sizeof(DISK_GEOMETRY),	 // Size of output buffer
			  &dwOut,			 // Number of bytes returned
			  NULL);			 // Synchronous I/O
  if (!bDone) return (int)GetLastError();

  pFdGeometry->dwSectors = dg.Cylinders.LowPart * dg.TracksPerCylinder * dg.SectorsPerTrack;
  pFdGeometry->wSectorSize = (WORD)(dg.BytesPerSector);
  pFdGeometry->wCyls = (WORD)(dg.Cylinders.LowPart);
  pFdGeometry->wHeads = (WORD)(dg.TracksPerCylinder);
  pFdGeometry->wSects = (WORD)(dg.SectorsPerTrack);

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskNTRead					      |
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
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskNTRead(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  LARGE_INTEGER li;
  DWORD dwRead;
  BOOL bDone;
  DWORD dwDone;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskNTRead(hDrive=%p, LBA=%lX, N=%X, Buf@=%Fp)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
#endif // _DEBUG

  li.QuadPart = 512 * dwSector;
  dwDone = SetFilePointer(hDrive,	// handle to file
		 li.LowPart,		// bytes to move pointer
		 &li.HighPart,		// bytes moved
		 FILE_BEGIN);		// Starting point
  if (dwDone == 0xFFFFFFFF) {
    DWORD dwErr = GetLastError();
    if (dwErr != NO_ERROR) return (int)dwErr;
  }

  bDone = ReadFile(hDrive,		// handle to file
		   pBuf,		// data buffer
		   512 * wNum,		// number of bytes to read
		   &dwRead,		// number of bytes read
		   NULL);		// No overlapped buffer
  if (!bDone) return (int)GetLastError();

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskNTWrite					      |
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
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskNTWrite(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  LARGE_INTEGER li;
  DWORD dwRead;
  BOOL bDone;
  DWORD dwDone;

#ifdef _DEBUG
  if (iDebug) {
    printf("FloppyDiskNTWrite(hDrive=%p, LBA=%lX, N=%X, Buf@=%Fp)\n",
		    hDrive, dwSector, wNum, pBuf);
  }
  if (iReadOnly) printf("Read-only! Write canceled.\n");
#endif // _DEBUG
  if (iReadOnly) return 0;

  li.QuadPart = 512 * dwSector;
  dwDone = SetFilePointer(hDrive,	// handle to file
		 li.LowPart,		// bytes to move pointer
		 &li.HighPart,		// bytes moved
		 FILE_BEGIN);		// Starting point
  if (dwDone == 0xFFFFFFFF) {
    DWORD dwErr = GetLastError();
    if (dwErr != NO_ERROR) return (int)dwErr;
  }

  bDone = WriteFile(hDrive,		// handle to file
		   pBuf,		// data buffer
		   512 * wNum,		// number of bytes to read
		   &dwRead,		// number of bytes read
		   NULL);		// No overlapped buffer
  if (!bDone) return (int)GetLastError();

  return 0;
}

