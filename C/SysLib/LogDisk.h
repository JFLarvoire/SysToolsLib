/*****************************************************************************\
*                                                                             *
*   Filename:	    LogDisk.h						      *
*									      *
*   Description:    OS-independant Logical disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*									      *
*   History:								      *
*    2002-02-07 JFL Created this file.					      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _LOGDISK_H_
#define _LOGDISK_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/*****************************************************************************\
*                                                                             *
*                          Basic type definitions                             *
*                                                                             *
\*****************************************************************************/

#define TRUE 1
#define FALSE 0

/* Redefine these basic types. Compatible with Windows and EFI. */
#include "qword.h"		/* Also defines BYTE, WORD, DWORD. */

typedef void FAR *LPVOID;
typedef void * HANDLE;		/* Must be a near pointer */

/********************* BIOS parameter Block structures ***********************/

#pragma pack(1)   /* IMPORTANT: All following structures are packed! */

#define SECTORSIZE 512		/* Standard disk sector size */

#pragma warning(disable:4201) /* Avoid warnings "nonstandard extension used : nameless struct/union" */

typedef struct tagBPB
    {
    BYTE  bpbJump[3];		/* 00: E9 XX XX or EB XX 90 	       */
    char  bpbOemName[8]; 	/* 03: OEM name and version 	       */
				/* Start of BIOS parameter block (BPB) */
    WORD  bpbBytesPerSec;	/* 0B: Bytes per sector		       */
    BYTE  bpbSecPerClust;	/* 0D: Sectors per cluster	       */
    WORD  bpbResSectors; 	/* 0E: Number of reserved sectors      */
    BYTE  bpbFATs;		/* 10: Number of file allocation tables*/
    WORD  bpbRootDirEnts;	/* 11: Number of root directory entries*/
    WORD  bpbSectors;		/* 13: Total number of sectors	       */
    BYTE  bpbMedia;		/* 15: Media descriptor		       */
    WORD  bpbFATsecs;		/* 16: Number of sectors per FAT       */
    WORD  bpbSecPerTrack;	/* 18: Sectors per track	       */
    WORD  bpbHeads;		/* 1A: Number of heads		       */
    DWORD bpbHiddenSecs; 	/* 1C: Number of hidden sectors	       */
    DWORD bpbHugeSectors;	/* 20: Nbr of sectors if bpbSectors=0  */
				/* End of BIOS parameter block (BPB)   */
    union
	{
	struct		/* FAT12/16 BPB extension. */
	    {
	    BYTE  bpbDriveNumber;	/* 24: Drive number (~80h)	       */
	    BYTE  bpbReserved1;		/* 25: Reserved			       */
	    BYTE  bpbBootSignature;	/* 26: Extended boot signature (=29h)  */
	    DWORD bpbVolumeID;		/* 27: Volume ID number		       */
	    char  bpbVolumeLabel[11];	/* 2B: Volume label 		       */
	    char  bpbFileSysType[8];	/* 36: File system type		       */
	    };
	struct		/* FAT32 BPB extension. */
	    {
	    DWORD bpbFAT32Size;		/* 24: Number of sectors per FAT       */
	    WORD  bpbFAT32ExtFlags;	/* 28: Defines FAT mirroring options   */
	    WORD  bpbFAT32Version;	/* 2A: 0 = FAT32 spec 1.03	       */
	    DWORD bpbFAT32RootClust;	/* 2C: Cluster for root directory. (~2)*/
	    WORD  bpbFAT32Info;		/* 30: Sector # of FSINFO struct. (~1) */
	    WORD  bpbFAT32BkBootSec;	/* 32: Sector # of backup boot sect.(~6)*/
	    char  bpbFAT32reserved[12]; /* 34: Reserved			       */
	    BYTE  bpbFAT32DriveNumber;	/* 40: Drive number (~80h)	       */
	    char  bpbFAT32reserved2;	/* 41: Reserved			       */
	    BYTE  bpbFAT32BootSignature;/* 42: Extended boot signature (=29h)  */
	    DWORD bpbFAT32VolumeID;	/* 43: Volume ID number		       */
	    char  bpbFAT32VolumeLabel[11];/* 47: Volume label 		       */
	    char  bpbFAT32FileSysType[8];/*52: File system type		       */
	    };
	struct		/* NTFS BPB extension */
	    {
	    WORD  bpbNTFSDriveNumber1;	/* 24: Always 00 80?		       */
	    WORD  bpbNTFSDriveNumber2;	/* 26: Always 00 80?		       */
	    QWORD bpbNTFSSectors;	/* 28: Nbr of sectors in the partition */
	    QWORD bpbNTFSMftLcn;	/* 30: LCN of VCN 0 of $MFT 	       */
	    QWORD bpbNTFSMftMirrLcn;	/* 38: LCN of VCN 0 of $MFTMirr        */
	    DWORD bpbNTFSClustPerRec;	/* 40: FILE record size in clusters    */
	    DWORD bpbNTFSClustPerIndex; /* 44: Index buffer size in clusters   */
	    DWORD bpbNTFSSerialNumber;	/* 48: Volume serial number 	       */
	    };   
        };
    }
    BPB;

#pragma warning(default:4201)

/***************** End of BIOS Parameter Block structures ********************/

/* OS-independant routines */
HANDLE LogDiskOpen(char cDisk, int iMode);
/* Definitions for iMode bits. */
#define READWRITE 0	/* Bit 0 = Write protection. */
#define READONLY 1
void LogDiskClose(HANDLE hDrive);
int LogDiskGetBPB(HANDLE hDrive, BPB *pBpb);
int LogDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int LogDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);

/* Windows 9X specific routines */
#ifdef _WIN32
HANDLE LogDisk95Open(char cDisk, int iMode);
void LogDisk95Close(HANDLE hDrive);
int LogDisk95GetBPB(HANDLE hDrive, BPB *pBpb);
int LogDisk95Read(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int LogDisk95Write(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

/* Windows NT specific routines */
#ifdef _WIN32
HANDLE LogDiskNTOpen(char cDisk, int iMode);
void LogDiskNTClose(HANDLE hDrive);
int LogDiskNTGetBPB(HANDLE hDrive, BPB *pBpb);
int LogDiskNTRead(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
int LogDiskNTWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, LPVOID pBuf);
#endif /* _WIN32 */

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

#endif /* _LOGDISK_H_ */
