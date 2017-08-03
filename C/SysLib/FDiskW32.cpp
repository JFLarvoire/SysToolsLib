/*****************************************************************************\
*                                                                             *
*   Filename:	    FDiskW32.cpp					      *
*									      *
*   Description:    WIN32-specific floppy disk access routines		      *
*                                                                             *
*   Notes:	    This code implements the OS-independant floppy disk I/O   *
*		    routines for WIN32.					      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*		    Sectors are referenced by their DWORD LBA number.	      *
*									      *
*		    For Windows 9X family, all access are done through the    *
*		    FloppyDisk9XSomething() subroutines, in file FDisk9X.cpp. *
*									      *
*		    For Windows NT family, all access are done through the    *
*		    FloppyDiskNTSomething() subroutines, in file FDiskNT.cpp. *
*									      *
*   History:								      *
*    2017-07-17 JFL Created this file.					      *
*		    							      *
*        (C) Copyright 2017 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#define _CRT_SECURE_NO_WARNINGS

#include "FloppyDisk.h"	// Public definitions for this module.

#include <windows.h>

#if WINVER < 0x500	// For Win95 and NT4 (both WINVER 0x400), we need two distinct methods

#if HAS_98DDK /* If the Windows 98 DDK is available, compile support for both Win9x and standard Win32 versions. */
#pragma message("Including FDisk95.cpp for Win95 floppy disk support")
#include "FDisk95.cpp"
#pragma warning(disable:4127) /* Avoid warnings "conditional expression is constant" due to the while (0) below. */
#define FloppyDiskDISPATCH(RETURN, proc, args) do {			\
    if (GetVersion() < 0x80000000)           /* Windows NT/2000/XP */   \
        RETURN FloppyDiskNT##proc args;                                 \
    else				     /* Windows 95/98/ME */     \
	RETURN FloppyDisk95##proc args;                                 \
} while (0)
#else // !HAS_98DDK /* Else only the standard WIN32 support is available, as part of the standard Windows SDK */
#pragma warning("No Win9x support in the absence of the Windows 98 DDK (98DDK)")
#define FloppyDiskDISPATCH(RETURN, proc, args) RETURN FloppyDiskNT##proc args
#endif // HAS_98DDK

#else // WINVER >= 0x500	// For 2000 and later, only the standard Win32 method is necessary

#define FloppyDiskDISPATCH(RETURN, proc, args) RETURN FloppyDiskNT##proc args

#endif // WINVER < 0x500

#include "FDiskNT.cpp"

/*===========================================================================*\
*                                                                             *
"                     OS-independant family, for WIN32                        "
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
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE FloppyDiskOpen(int iDrive, int iMode) {
  FloppyDiskDISPATCH(return, Open, (iDrive, iMode));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskClose					      |
|									      |
|   Description:    Release resources reserved by FloppyDiskOpen.	      |
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

void FloppyDiskClose(HANDLE hDrive) {
  FloppyDiskDISPATCH(, Close, (hDrive));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskGetGeometry				      |
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

int FloppyDiskGetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry) {
  FloppyDiskDISPATCH(return, GetGeometry, (hDrive, pFdGeometry));
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
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskRead(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  FloppyDiskDISPATCH(return, Read, (hDrive, dwSector, wNum, pBuf));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FloppyDiskWrite					      |
|									      |
|   Description:    Write N sectors to the floppy disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the floppy disk.      |
|		    DWORD dwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FloppyDiskXxxx family. |
|									      |
|   History:								      |
|    2017-07-17 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FloppyDiskWrite(HANDLE hDrive, DWORD dwSector, WORD wNum, void FAR *pBuf) {
  FloppyDiskDISPATCH(return, Write, (hDrive, dwSector, wNum, pBuf));
}

