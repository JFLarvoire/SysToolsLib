/*****************************************************************************\
*                                                                             *
*   Filename:	    HardDisk.h						      *
*									      *
*   Description:    OS-independant hard disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*									      *
*   History:								      *
*    2000-09-21 JFL Created this file.					      *
*    2001-02-20 JFL Added function HardDiskOpen().			      *
*    2001-09-07 JFL Added second argument to function HardDiskOpen().	      *
*		    Added prototypes for Win95 and NT subfunctions.	      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2017-07-06 JFL Fixed warnings when included in a C source.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _HARDDISK_H_
#define _HARDDISK_H_

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

/******************** Legacy partition table structures **********************/

#pragma pack(1)   /* IMPORTANT: All following structures are packed! */

#define SECTORSIZE 512		/* Standard disk sector size */

#ifndef STRUCT_PARTITION
#define STRUCT_PARTITION

#pragma warning(disable:4214)	/* Ignore the warning: nonstandard extension used : bit field types other than int */
typedef struct tagPARTITION {	/* Partition table entry in the boot sector */
  BYTE boot; 			/* 0x80 = Bootable; 0x00 = not */
  BYTE beg_head;		/* Beginning head number */
  WORD beg_sect	:6;		/* Beginning sector number */
  WORD beg_hcyl	:2;		/* Beginning cylinder number bits <9:8> */
  WORD beg_lcyl	:8;		/* Beginning cylinder number bits <7:0> */
  BYTE type; 			/* Type. 0 = None, 1 = DOS FAT 12 bits, etc...*/
  BYTE end_head;		/* End head number */
  WORD end_sect	:6;		/* End sector number */
  WORD end_hcyl	:2;		/* End cylinder number bits <9:8> */
  WORD end_lcyl	:8;		/* End cylinder number bits <7:0> */
  DWORD first_sector;	 	/* Index of first sector from drive sector 0 */
  DWORD n_sectors;		/* Total number of sectors */
} PARTITION;
#pragma warning(default:4214)	/* Restore the warning: nonstandard extension used : bit field types other than int */

#endif /* STRUCT_PARTITION */

#ifndef STRUCT_MASTERBOOTSECTOR
#define STRUCT_MASTERBOOTSECTOR

typedef struct tagMASTERBOOTSECTOR {	/* Boot sector structure */
  BYTE mbsCode[0x1BE];		/* Code and garbage up to offset 1BE */
  PARTITION mbsPart[4];		/* 4 partition table entries */
  WORD mbsSignature;		/* AA55 marks a valid boot sector */
} MASTERBOOTSECTOR;

#endif /* STRUCT_MASTERBOOTSECTOR */

/**************** End of Legacy partition table structures *******************/

/* Hard disk sector direct access */

typedef struct tagHDGEOMETRY {
  QWORD qwSectors;
  WORD wSectorSize;
  DWORD dwCyls;		/* number of physical cylinders on drive */
  DWORD dwHeads;	/* number of physical heads on drive */
  DWORD dwSects;	/* number of physical sectors per track */
  DWORD dwXlatCyls;	/* number of translated cylinders on drive */
  DWORD dwXlatHeads;	/* number of translated heads on drive */
  DWORD dwXlatSects;	/* number of translated sectors per track */
} HDGEOMETRY;

/* OS-independant routines */
HANDLE HardDiskOpen(int iDisk, int iMode);
/* Definitions for iMode bits. */
#define READWRITE 0	/* Bit 0 = Write protection. */
#define READONLY 1
void HardDiskClose(HANDLE hDrive);
int HardDiskGetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry);
int HardDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int HardDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);

/* Windows 9X specific routines */
#ifdef _WIN32
HANDLE HardDisk95Open(int iDisk, int iMode);
void HardDisk95Close(HANDLE hDrive);
int HardDisk95GetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry);
int HardDisk95Read(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int HardDisk95Write(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

/* Windows NT specific routines */
#ifdef _WIN32
HANDLE HardDiskNTOpen(int iDisk, int iMode);
void HardDiskNTClose(HANDLE hDrive);
int HardDiskNTGetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry);
int HardDiskNTRead(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int HardDiskNTWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

/* MS-DOS specific routines */
#ifdef _MSDOS
#define HardDiskGetInt13DriveNumber(hDrive) ((BYTE)(DWORD)(hDrive))
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

#endif /* _HARDDISK_H_ */
