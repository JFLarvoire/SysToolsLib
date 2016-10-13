/*****************************************************************************\
*                                                                             *
*   Filename:	    LDiskNT.cpp						      *
*									      *
*   Description:    Windows NT-specific logical disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant logical disk I/O  *
*		    routines for Windows NT/2000/XP.			      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    Logical disk accesses are done through file I/O to        *
*		    \\.\X: virtual file.				      *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include <windows.h>

#include "LogDisk.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskNTOpen					      |
|									      |
|   Description:    Get a handle for a given logical disk.		      |
|									      |
|   Parameters:     char cDrive	    Logical Disk letter. A=flpy; C=hdisk...   |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The logical disk handle, or NULL if no such logical disk. |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE LogDiskNTOpen(char cDrive, int iMode)
    {
    HANDLE hDrive;
    char szDriveName[8];
    DWORD dwAccessMode;
    DWORD dwShareMode;
    
    dwAccessMode = GENERIC_READ;
    dwShareMode = FILE_SHARE_READ;
    if (!(iMode & READONLY)) 
        {	// If any write mode is requested
        dwAccessMode |= GENERIC_WRITE;
        }
    dwShareMode |= FILE_SHARE_WRITE;	// Documented as necessary in all cases for \\.\A: etc...

#ifdef _DEBUG
    if (iDebug)
        {
        printf("LogDiskNTOpen(cDrive=%c, iMode=%x)\n", cDrive, iMode);
	}
#endif // _DEBUG

    wsprintf(szDriveName, "\\\\.\\%c:", cDrive);

    hDrive = CreateFile (szDriveName,			// device name
			 dwAccessMode,			// access mode
			 dwShareMode,			// share mode
			 NULL,				// Default security
			 OPEN_EXISTING,			// 
			 0,				// file attributes
			 NULL);				// No template

    if (hDrive == INVALID_HANDLE_VALUE) return NULL;

    return hDrive;
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskNTClose					      |
|									      |
|   Description:    Release resources reserved by LogDiskNTOpen.	      |
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

void LogDiskNTClose(HANDLE hDrive)
    {
    CloseHandle(hDrive);
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskNTGetBPB					      |
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
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#include <winioctl.h>

int LogDiskNTGetBPB(HANDLE hDrive, BPB *pBpb)
    {
    char buf[512];
    DWORD dwErr;
    char *pBuf = buf;

#ifdef _DEBUG
    if (iDebug)
        {
        printf("LogDiskNTGetBPB(hDrive=%p)\n", hDrive);
	}
#endif // _DEBUG

    dwErr = LogDiskNTRead(hDrive, 0, 1, pBuf);
    if (dwErr) return (int)dwErr;
    memcpy(pBpb, pBuf, sizeof(BPB));

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskNTRead					      |
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

int LogDiskNTRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    LARGE_INTEGER li;
    DWORD dwRead;
    BOOL bDone;
    DWORD dwDone;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("LogDiskNTRead(hDrive=%p, Sector=%s, N=%X, Buf@=%Fp)\n", 
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
    if (!bDone) 
	{
	DWORD dwErr = GetLastError();
	return (int)dwErr;
	}

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskNTWrite					      |
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
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskNTWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    LARGE_INTEGER li;
    DWORD dwRead;
    BOOL bDone;
    DWORD dwDone;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("LogDiskNTWrite(hDrive=%p, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    if (iReadOnly) printf("Read-only! Write canceled.\n");
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

