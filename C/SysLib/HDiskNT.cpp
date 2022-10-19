/*****************************************************************************\
*                                                                             *
*   Filename:	    HDiskNT.cpp						      *
*									      *
*   Description:    Windows NT-specific hard disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant hard disk I/O     *
*		    routines for Windows NT/2000/XP.			      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through file I/O to       *
*		    \\.\PhysicalDriveX device drivers.			      *
*									      *
*   History:								      *
*    2000/09/21 JFL Created file harddisk.cpp.				      *
*    2001/09/05 JFL Moved all WinNT-specific routines into file HDiskNT.cpp.  *
*    2001/12/21 JFL Use wsprintf instead of printf.			      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include <windows.h>

#include "HardDisk.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskNTOpen					      |
|									      |
|   Description:    Get a handle for a given hard disk.			      |
|									      |
|   Parameters:     int iDisk	    Hard Disk number. 0=1st hard disk, etc.   |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The hard disk handle, or NULL if no such hard disk.	      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
|    2010/10/08 JFL Added an extra debug message.			      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE HardDiskNTOpen(int iDrive, int iMode)
    {
    HANDLE hDrive;
    char szDriveName[32];
    DWORD dwAccessMode;
    DWORD dwShareMode;
    
    dwAccessMode = GENERIC_READ;
    dwShareMode = FILE_SHARE_READ;
    if (!(iMode & READONLY)) 
        {	// If any write mode is requested
        dwAccessMode |= GENERIC_WRITE;
        dwShareMode |= FILE_SHARE_WRITE;
        }
    
#ifdef _DEBUG
    if (iDebug)
        {
        printf("HardDiskNTOpen(iDrive=%d, iMode=%x)\n", iDrive, iMode);
	}
#endif // _DEBUG

    wsprintf(szDriveName, "\\\\.\\PhysicalDrive%d", iDrive);

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
|   Function:	    HardDiskNTClose					      |
|									      |
|   Description:    Release resources reserved by HardDiskNTOpen.	      |
|									      |
|   Parameters:     HANDLE hDisk    Hard Disk handle.			      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void HardDiskNTClose(HANDLE hDrive)
    {
    CloseHandle(hDrive);
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskNTGetGeometry				      |
|									      |
|   Description:    Get the geometry of the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    HDGEOMETRY *pHdGeometry	Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#include <winioctl.h>

int HardDiskNTGetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry)
    {
    DISK_GEOMETRY dg;
    DWORD dwOut;
    BOOL bDone;
    
    // Get the physical parameters for the drive.
    bDone = DeviceIoControl(hDrive,	                   // handle to device
			    IOCTL_DISK_GET_DRIVE_GEOMETRY, // dwIoControlCode
			    NULL,                          // lpInBuffer
			    0,                             // nInBufferSize
			    (LPVOID)&dg,	           // output buffer
			    sizeof(DISK_GEOMETRY),         // size of output buffer
			    &dwOut,		           // number of bytes returned
			    NULL);			   // OVERLAPPED structure
    if (!bDone) return (int)GetLastError();

    pHdGeometry->qwSectors = dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack;
    pHdGeometry->wSectorSize = (WORD)(dg.BytesPerSector);
    pHdGeometry->dwCyls = dg.Cylinders.LowPart;
    pHdGeometry->dwHeads = dg.TracksPerCylinder;
    pHdGeometry->dwSects = dg.SectorsPerTrack;
    // Assume no translation.
    pHdGeometry->dwXlatCyls = dg.Cylinders.LowPart;
    pHdGeometry->dwXlatHeads = dg.TracksPerCylinder;
    pHdGeometry->dwXlatSects = dg.SectorsPerTrack;

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskNTRead					      |
|									      |
|   Description:    Read N sectors from the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskNTRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    LARGE_INTEGER li;
    DWORD dwRead;
    BOOL bDone;
    DWORD dwDone;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDiskNTRead(hDrive=%p, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
#endif // _DEBUG

    li.QuadPart = 512 * qwSector;
    dwDone = SetFilePointer(hDrive,	// handle to file
		   li.LowPart,		// bytes to move pointer
		   &li.HighPart,	// bytes moved
		   FILE_BEGIN);		// Starting point
    if (dwDone == 0xFFFFFFFF)
	{
	DWORD dwErr = GetLastError();
	if (dwErr != NO_ERROR) return (int)dwErr;
	}

    bDone = ReadFile(hDrive,		// handle to file
		     pBuf,		// data buffer
		     512 * wNum,	// number of bytes to read
		     &dwRead,		// number of bytes read
		     NULL);		// No overlapped buffer
    if (!bDone) return (int)GetLastError();

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskNTWrite					      |
|									      |
|   Description:    Write N sectors to the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskNTWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    LARGE_INTEGER li;
    DWORD dwRead;
    BOOL bDone;
    DWORD dwDone;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDiskNTWrite(hDrive=%p, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    if (iReadOnly) printf("Read-only mode! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;

    li.QuadPart = 512 * qwSector;
    dwDone = SetFilePointer(hDrive,	// handle to file
		   li.LowPart,		// bytes to move pointer
		   &li.HighPart,	// bytes moved
		   FILE_BEGIN);		// Starting point
    if (dwDone == 0xFFFFFFFF)
	{
	DWORD dwErr = GetLastError();
	if (dwErr != NO_ERROR) return (int)dwErr;
	}

    bDone = WriteFile(hDrive,		// handle to file
		     pBuf,		// data buffer
		     512 * wNum,	// number of bytes to read
		     &dwRead,		// number of bytes read
		     NULL);		// No overlapped buffer
    if (!bDone) return (int)GetLastError();

    return 0;
    }

