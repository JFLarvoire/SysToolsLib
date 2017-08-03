/*****************************************************************************\
*                                                                             *
*   Filename:	    FloppyDisk.h					      *
*									      *
*   Description:    OS-independant floppy disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*									      *
*   History:								      *
*    2017-07-15 JFL Created this file.					      *
*									      *
*        (C) Copyright 2017 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _FLOPPYDISK_H_
#define _FLOPPYDISK_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/*****************************************************************************\
*                                                                             *
*                          Basic type definitions                             *
*                                                                             *
\*****************************************************************************/

#define TRUE 1
#define FALSE 0

/* Redefine these basic types. Compatible with Windows and EFI. */
#include "qword.h"	/* Defines BYTE, WORD, DWORD, QWORD, etc */

typedef void * HANDLE;		/* Must be a near pointer */

/* Floppy disk sector direct access */

typedef struct tagFDGEOMETRY {
  DWORD dwSectors;
  WORD wSectorSize;
  WORD wCyls;		/* number of physical cylinders on drive */
  WORD wHeads;		/* number of physical heads on drive */
  WORD wSects;		/* number of physical sectors per track */
} FDGEOMETRY;

/* OS-independant routines */
HANDLE FloppyDiskOpen(int iDisk, int iMode);
/* Definitions for iMode bits. */
#define READWRITE 0	/* Bit 0 = Write protection. */
#define READONLY 1
void FloppyDiskClose(HANDLE hDrive);
int FloppyDiskGetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry);
int FloppyDiskRead(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);
int FloppyDiskWrite(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);

/* Windows 9X specific routines */
#ifdef _WIN32
HANDLE FloppyDisk95Open(int iDisk, int iMode);
void FloppyDisk95Close(HANDLE hDrive);
int FloppyDisk95GetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry);
int FloppyDisk95Read(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);
int FloppyDisk95Write(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

/* Windows NT specific routines */
#ifdef _WIN32
HANDLE FloppyDiskNTOpen(int iDisk, int iMode);
void FloppyDiskNTClose(HANDLE hDrive);
int FloppyDiskNTGetGeometry(HANDLE hDrive, FDGEOMETRY *pFdGeometry);
int FloppyDiskNTRead(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);
int FloppyDiskNTWrite(HANDLE hDrive, DWORD dwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

/* MS-DOS specific routines */
#ifdef _MSDOS
#define FloppyDiskGetInt13DriveNumber(hDrive) ((BYTE)(DWORD)(hDrive))
#endif /* _MSDOS */

#pragma pack()	/* Restore the default packing size */

/*************** Global variables to be defined by the caller ****************/

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef _DEBUG
extern int iDebug;	/* Defined in main module. If TRUE, display debug infos. */
extern int iVerbose;	/* Defined in main module. If TRUE, display progress infos. */
#include <stdio.h>	/* For printf(). */
#endif /* _DEBUG */

extern int iReadOnly;	/* Defined in main module. If TRUE, do not write to disk. */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _FLOPPYDISK_H_ */
