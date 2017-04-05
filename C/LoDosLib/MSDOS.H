/*****************************************************************************\
*                                                                             *
*   Filename:	    msdos.h						      *
*									      *
*   Description:    MS-DOS structures and constants			      *
*									      *
*   Notes:	    Copied from the MS-DOS Programmer's Reference             *
*									      *
*   History:								      *
*    1995/05/24 JFL Extracted from MAKEPAR.C.				      *
*		    Added structure MID.				      *
*    1995/08/25 JFL Added structure ARENA.				      *
*    1996/08/08 JFL Moved the IBMsUseLessBytes[] field from DEVICEPARAMSEX to *
*		    DEVICEPARAMS. This fixes a bug with MS-DOS 7.1.	      *
*    2000/06/06 JFL Added Master Boot Sector structures.		      *
*    2001/03/09 JFL Added conditional definitions for boot sector structures, *
*		    as they are also defined in other include files.	      *
*    2001/03/22 JFL Use FAR macro instead of far keyword.		      *
*    2001/06/22 JFL Renamed MASTERBOOTSECTOR as MASTERBOOTRECORD, as this is  *
*		    the standard name used in the industry.		      *
*                   Added the mbrDiskSignature field.                         *
*    2002/02/06 JFL Added field deStartClusterHi in DIRENTRY structure.       *
*                                                                             *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __MSDOS_H__   // Prevent multiple inclusions
#define __MSDOS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#ifdef _MSDOS
#define FAR far
#endif
#ifdef _WIN32
#define FAR
#endif

#pragma pack(1)   /* IMPORTANT: All DOS structures are packed! */

#define SECTORSIZE 512		// Standard disk sector size

#ifndef STRUCT_PARTITION
#define STRUCT_PARTITION

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

#endif // STRUCT_PARTITION

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

#endif // STRUCT_MASTERBOOTRECORD

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

#endif // STRUCT_BOOTSECTOR

typedef struct tagDEVICEPARAMS
    {
    BYTE  dpSpecFunc;		/* Special functions		       */
    BYTE  dpDevType;		/* Device type			       */
    WORD  dpDevAttr;		/* Device attribute		       */
    WORD  dpCylinders;		/* Number of cylinders		       */
    BYTE  dpMediaType;		/* Media type			       */
				/* Start of BIOS parameter block (BPB) */
    WORD  dpBytesPerSec;	/* Bytes per sector		       */
    BYTE  dpSecPerClust;	/* Sectors per cluster		       */
    WORD  dpResSectors; 	/* Number of reserved sectors	       */
    BYTE  dpFATs;		/* Number of file allocation tables    */
    WORD  dpRootDirEnts;	/* Number of root directory entries    */
    WORD  dpSectors;		/* Total number of sectors	       */
    BYTE  dpMedia;		/* Media descriptor		       */
    WORD  dpFATsecs;		/* Number of sectors per FAT	       */
    WORD  dpSecPerTrack;	/* Sectors per track		       */
    WORD  dpHeads;		/* Number of heads		       */
    DWORD dpHiddenSecs; 	/* Number of hidden sectors	       */
    DWORD dpHugeSectors;	/* Number of sectors if dpSectors = 0  */
				/* End of BIOS parameter block (BPB)   */
    BYTE  IBMsUselessBytes[6];	/* Fix an error in the documentation   */
    }
    DEVICEPARAMS;

typedef struct tagRWBLOCK
    {
    BYTE  rwSpecFunc;		/* Special function (must be 0)        */
    WORD  rwHead;		/* Head to read/write		       */
    WORD  rwCylinder;		/* Cylinder to read/write	       */
    WORD  rwFirstSector;	/* First sector to read/write	       */
    WORD  rwSectors;		/* Number of sectors to read/write	*/
    DWORD rwBuffer;		/* Address of buffer for read/write data */
    }
    RWBLOCK;

typedef struct tagDISKIO
    {
    DWORD diStartSector;	/* Sector number to start	       */
    WORD  diSectors;		/* Number of sectors		       */
    char FAR *diBuffer; 	/* Address of buffer		       */
    }
    DISKIO;

typedef struct tagDIRENTRY
    {
    char  deName[8];		/* File name			       */
    char  deExtension[3];	/* File name extension		       */
    BYTE  deAttributes; 	/* Attributes			       */
    char  deReserved[8];	/* Reserved. Do not use 	       */
    WORD  deStartClusterHi;	/* Starting cluster high word (FAT32)  */
    WORD  deTime;		/* Time of last change		       */
    WORD  deDate;		/* Date of last change		       */
    WORD  deStartCluster;	/* Number of the starting cluster      */
    DWORD deFileSize;		/* Size 			       */
    }
    DIRENTRY;

typedef struct tagFCB		/* File control block */
    {
    BYTE  fcbDriveID;		/* Drive no (0=default, 1=A, etc...)   */
    char  fcbFileName[8];	/* File name			       */
    char  fcbExtent[3]; 	/* File extension		       */
    WORD  fcbCurBlockNo;	/* Current block number 	       */
    WORD  fcbRecSize;		/* Record size			       */
    DWORD fcbFileSize;		/* File size			       */
    WORD  fcbDate;		/* Date last modified		       */
    WORD  fcbTime;		/* Time last modified		       */
    char  fcbReserved[8];	/* Reserved			       */
    BYTE  fcbCurRecNo;		/* Current record number	       */
    DWORD fcbRandomRecNo;	/* Random record number 	       */
    }
    FCB;

typedef struct tagMID		/* Media ID */
    {
    WORD  midInfoLevel; 	/* Information level */
    DWORD midSerialNum; 	/* Serial number */
    char  midVolLabel[11];	/* Volume label */
    char  midFileSysType[8];	/* File system type */
    }
    MID;

typedef struct tagTRACKLAYOUTENTRY
    {
    WORD  tklSectorNum;
    WORD  tklSectorSize;
    }
    TRACKLAYOUTENTRY;

typedef struct tagTRACKLAYOUT
    {
    WORD	     tklSectors;
    TRACKLAYOUTENTRY tklNumSize[64];
    }
    TRACKLAYOUT;

typedef struct tagDEVICEPARAMSEX    /* Extended device parameters */
    {
    DEVICEPARAMS dp;		/* Standard device parameters	       */
    TRACKLAYOUT  tkl;		/* Track layout 		       */
    }
    DEVICEPARAMSEX;

typedef struct			/* MS-DOS memory arena */
    {		    /* See the DOS programmer's reference for details */
    BYTE arenaSignature;
    WORD arenaOwner;
    WORD arenaSize;
    char arenaReserved[3];
    char arenaName[8];
    }
    ARENA;

#pragma pack()		/* Restore the default structure packing */

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // end if __MSDOS_H__ included
