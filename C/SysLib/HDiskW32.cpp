/*****************************************************************************\
*                                                                             *
*   Filename:	    HDiskW32.cpp					      *
*									      *
*   Description:    WIN32-specific hard disk access routines		      *
*                                                                             *
*   Notes:	    This code implements the OS-independant hard disk I/O     *
*		    routines for WIN32.					      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For Windows 9X family, all access are done through the    *
*		    HardDisk9XSomething() subroutines, in file HDisk9X.cpp.   *
*									      *
*		    For Windows NT family, all access are done through the    *
*		    HardDiskNTSomething() subroutines, in file HDiskNT.cpp.   *
*									      *
*   History:								      *
*    2000-09-21 JFL Created file harddisk.cpp.				      *
*    2001-01-08 JFL Added support for read-only mode.			      *
*    2001-02-20 JFL Added function HardDiskOpen().			      *
*    2001-02-26 JFL Removed function HardDiskGetNext().			      *
*		    Added WIN32 (NT only) version of the functions.	      *
*    2001-09-05 JFL Created this file to dispatch NT and 9X function calls.   *
*    2015-11-02 JFL Made the Win9x support optional, compiled in only if the  *
*		    Windows 98 DDK is available.			      *
*    2015-11-06 JFL And if WINVER > 0x500, the .exe won't run in Win95, so    *
*		    we don't need the special methods for it anyway.	      *
*    2016-04-14 JFL Fixed the HardDiskDISPATCH macro.                         *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "HardDisk.h"	// Public definitions for this module.

#include <windows.h>

#if WINVER < 0x500	// For Win95 and NT4 (both WINVER 0x400), we need two distinct methods

#if HAS_98DDK /* If the Windows 98 DDK is available, compile support for both Win9x and standard Win32 versions. */
#pragma message("Including HDisk95.cpp for Win95 hard disk support")
#include "HDisk95.cpp"
#pragma warning(disable:4127) /* Avoid warnings "conditional expression is constant" due to the while (0) below. */
#define HardDiskDISPATCH(RETURN, proc, args) do {			\
    if (GetVersion() < 0x80000000)           /* Windows NT/2000/XP */   \
        RETURN HardDiskNT##proc args;                                   \
    else				     /* Windows 95/98/ME */     \
	RETURN HardDisk95##proc args;                                   \
} while (0)
#else // !HAS_98DDK /* Else only the standard WIN32 support is available, as part of the standard Windows SDK */
#pragma warning("No Win9x support in the absence of the Windows 98 DDK (98DDK)")
#define HardDiskDISPATCH(RETURN, proc, args) return HardDiskNT##proc args
#endif // HAS_98DDK

#else // WINVER >= 0x500	// For 2000 and later, only the standard Win32 method is necessary

#define HardDiskDISPATCH(RETURN, proc, args) return HardDiskNT##proc args

#endif // WINVER < 0x500

#include "HDiskNT.cpp"

/*===========================================================================*\
*                                                                             *
"                     OS-independant family, for WIN32                        "
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskOpen					      |
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
|    2001-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE HardDiskOpen(int iDrive, int iMode)
    {
    HardDiskDISPATCH(return, Open, (iDrive, iMode));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskClose					      |
|									      |
|   Description:    Release resources reserved by HardDiskOpen.		      |
|									      |
|   Parameters:     HANDLE hDisk    Hard Disk handle.			      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|    2001-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void HardDiskClose(HANDLE hDrive)
    {
    HardDiskDISPATCH(, Close, (hDrive));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskGetGeometry					      |
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
|    2001-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskGetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry)
    {
    HardDiskDISPATCH(return, GetGeometry, (hDrive, pHdGeometry));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskRead					      |
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
|    2001-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    HardDiskDISPATCH(return, Read, (hDrive, qwSector, wNum, pBuf));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskWrite					      |
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
|    2001-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    HardDiskDISPATCH(return, Write, (hDrive, qwSector, wNum, pBuf));
    }

