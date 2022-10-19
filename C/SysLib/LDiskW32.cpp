/*****************************************************************************\
*                                                                             *
*   Filename:	    LDiskW32.cpp					      *
*									      *
*   Description:    WIN32-specific logical disk access routines		      *
*                                                                             *
*   Notes:	    This code implements the OS-independant logical disk I/O  *
*		    routines for WIN32.					      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For Windows 9X family, all access are done through the    *
*		    LogDisk9XSomething() subroutines, in file LDisk9X.cpp.    *
*									      *
*		    For Windows NT family, all access are done through the    *
*		    LogDiskNTSomething() subroutines, in file LDiskNT.cpp.    *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#pragma warning(disable:4996) /* Disable "'GetVersion': was declared deprecated" warning */

#include "LogDisk.h"	// Public definitions for this module.

#ifdef _DEBUG

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)
extern int iDebug;	// Defined in main module. If TRUE, display debug infos.
extern int iVerbose;	// Defined in main module. If TRUE, display progress infos.
extern int iReadOnly;	// Defined in main module. If TRUE, do not write to disk.
#include <stdio.h>
#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // _DEBUG

#include <windows.h>

#include "LDisk95.cpp"
#include "LDiskNT.cpp"

/*===========================================================================*\
*                                                                             *
"                     OS-independant family, for WIN32                        "
*                                                                             *
\*===========================================================================*/

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
*									      *
\*---------------------------------------------------------------------------*/

HANDLE LogDiskOpen(char cDrive, int iMode)
    {
    if (GetVersion() < 0x80000000)              // Windows NT/2000/XP
        return LogDiskNTOpen(cDrive, iMode);
    else					// Windows 95/98/ME
	return LogDisk95Open(cDrive, iMode);
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

void LogDiskClose(HANDLE hDrive)
    {
    if (GetVersion() < 0x80000000)              // Windows NT/2000/XP
        LogDiskNTClose(hDrive);
    else					// Windows 95/98/ME
	LogDisk95Close(hDrive);
    }

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
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskGetBPB(HANDLE hDrive, BPB *pBpb)
    {
    if (GetVersion() < 0x80000000)              // Windows NT/2000/XP
        return LogDiskNTGetBPB(hDrive, pBpb);
    else					// Windows 95/98/ME
	return LogDisk95GetBPB(hDrive, pBpb);
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

int LogDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    if (GetVersion() < 0x80000000)              // Windows NT/2000/XP
        return LogDiskNTRead(hDrive, qwSector, wNum, pBuf);
    else					// Windows 95/98/ME
	return LogDisk95Read(hDrive, qwSector, wNum, pBuf);
    }

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
*									      *
\*---------------------------------------------------------------------------*/

int LogDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    if (GetVersion() < 0x80000000)              // Windows NT/2000/XP
        return LogDiskNTWrite(hDrive, qwSector, wNum, pBuf);
    else					// Windows 95/98/ME
	return LogDisk95Write(hDrive, qwSector, wNum, pBuf);
    }
