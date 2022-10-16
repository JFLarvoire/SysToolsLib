/*****************************************************************************\
*                                                                             *
*   Filename:	    IsMBR.h						      *
*									      *
*   Description:    Definitions for IsMBR.c				      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2001-12-19 JFL Created this file.					      *
*    2002-07-01 JFL Added a second parameter to IsBS().			      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _ISMBR_H_
#define _ISMBR_H_

#include "SysLib.h"		/* SysLib Library core definitions */

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#pragma pack(1)   /* IMPORTANT: All DOS structures are packed! */

#define SECTORSIZE 512		/* Standard disk sector size */

#ifndef STRUCT_PARTITION
#define STRUCT_PARTITION

#if defined(_MSC_VER)
#pragma warning(disable:4214) /* Avoid warnings "nonstandard extension used : bit field types other than int" */
#endif /* defined(_MSC_VER) */

typedef struct tagPARTITION	/* Partition table entry in the Master Boot Record */
{
    BYTE boot; 			/* 0x80 = Bootable; 0x00 = not */
    BYTE beg_head;		/* Beginning head number */
    WORD beg_sect	:6;	/* Beginning sector number */
    WORD beg_hcyl	:2;	/* Beginning cylinder number bits <9:8> */
    WORD beg_lcyl	:8;	/* Beginning cylinder number bits <7:0> */
    BYTE type; 			/* Type. 0 = None, 1 = DOS FAT 12 bits, etc...*/
    BYTE end_head;		/* End head number */
    WORD end_sect	:6;	/* End sector number */
    WORD end_hcyl	:2;	/* End cylinder number bits <9:8> */
    WORD end_lcyl	:8;	/* End cylinder number bits <7:0> */
    DWORD first_sector; 	/* Index of first sector from drive sector 0 */
    DWORD n_sectors;		/* Total number of sectors */
} PARTITION;

#if defined(_MSC_VER)
#pragma warning(default:4214)
#endif /* defined(_MSC_VER) */

#endif /* STRUCT_PARTITION */

#ifndef STRUCT_MASTERBOOTRECORD
#define STRUCT_MASTERBOOTRECORD

typedef struct tagMASTERBOOTRECORD	/* Master Boot Record structure */
{
    BYTE mbrCode[0x1B8];	/* Code and garbage up to offset 440 */
    DWORD mbrDiskSignature;	/* Unique disk signature (Optional) */
    WORD mbrUndefined;		/* Code may end here on some disks */
    PARTITION mbrPart[4];	/* 4 partition table entries */
    WORD mbrSignature;		/* AA55 marks a valid boot sector */
} MASTERBOOTRECORD;

#endif /* STRUCT_MASTERBOOTRECORD */

#ifndef STRUCT_BOOTSECTOR
#define STRUCT_BOOTSECTOR

typedef struct tagBOOTSECTOR
    {
    BYTE  bsJump[3];		/* E9 XX XX or EB XX 90 	       */
    char  bsOemName[8]; 	/* OEM name and version 	       */
				/* Start of BIOS parameter block (BPB) */
    WORD  bsBytesPerSec;	/* Bytes per sector		       */
    BYTE  bsSecPerClust;	/* Sectors per cluster		       */
    WORD  bsResSectors; 	/* Number of reserved sectors	       */
    BYTE  bsFATs;		/* Number of file allocation tables    */
    WORD  bsRootDirEnts;	/* Number of root directory entries    */
    WORD  bsSectors;		/* Total number of sectors	       */
    BYTE  bsMedia;		/* Media descriptor		       */
    WORD  bsFATsecs;		/* Number of sectors per FAT	       */
    WORD  bsSecPerTrack;	/* Sectors per track		       */
    WORD  bsHeads;		/* Number of heads		       */
    DWORD bsHiddenSecs; 	/* Number of hidden sectors	       */
    DWORD bsHugeSectors;	/* Number of sectors if bsSectors = 0  */
				/* End of BIOS parameter block (BPB)   */
    BYTE  bsDriveNumber;	/* Drive number (80h)		       */
    BYTE  bsReserved1;		/* Reserved			       */
    BYTE  bsBootSignature;	/* Extended boot signature (29h)       */
    DWORD bsVolumeID;		/* Volume ID number		       */
    char  bsVolumeLabel[11];	/* Volume label 		       */
    char  bsFileSysType[8];	/* File system type		       */
    }
    BOOTSECTOR;

#endif /* STRUCT_BOOTSECTOR */

#ifdef __cplusplus
extern "C" {
#endif

int IsBS(char *pBuf, unsigned int uBufSize);
int IsMBR(char *pBuf);

#ifdef __cplusplus
}
#endif

#pragma pack()	/* Restore the default packing size */

#endif /* _ISMBR_H_ */
