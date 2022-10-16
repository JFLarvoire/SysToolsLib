/*****************************************************************************\
*                                                                             *
*   Filename:	    Gpt.h						      *
*									      *
*   Description:    OS-independant GPT access routines			      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2001-02-20 JFL Created this file.					      *
*    2001-12-19 JFL Renamed EFI_HYBRID_LEGACY_MBS_GUID			      *
*			 as EFI_HYBRID_LEGACY_MBR_GUID.			      *
*		    Renamed guidMBS as guidMbr.				      *
*		    Renamed guidHeaderBackup as guidMbrBackup.		      *
*		    Renamed guidLegacy as guidHardDiskImage.		      *
*		    Added guidFloppyImage.				      *
*		    Added EFI_PART_TYPE_RELAY_GUID 			      *
*		    Added EFI_PART_TYPE_REPAIR_GUID 			      *
*		    Added EFI_PART_TYPE_MBR_BACKUP_GUID			      *
*		    Added EFI_BOOT_MENU_HEADER structure		      *
*    2002-01-03 JFL Added EFI_GUID guidRelay external reference.	      *
*    2002-02-04 JFL Updated the bootmenu header, as per MBR+GPT spec v0.97.   *
*    2002-05-27 JFL Added prototypes for GptBlockRead() and GptBlockWrite().  *
*		    Added members iSectorSize and iSectPerSect to GPTREF.     *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _GPT_H_
#define _GPT_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#ifndef _EFI_INCLUDE_
#pragma warning(disable:4096) /* Avoid warnings "'cdecl' must be used with '...'" */
// Prevent conflict with homonym in winnt.h include files
#define _LIST_ENTRY _EFI_LIST_ENTRY
#define  LIST_ENTRY  EFI_LIST_ENTRY
#include "efi.h"	/* Main EFI definitions */
/* #include "lib.h"	/* EFI library functions. Includes efigpt.h. */
#include "efilib.h"	/* EFI library functions. */
#include "efigpt.h"	/* GPT definitions */
#undef  _LIST_ENTRY
#undef   LIST_ENTRY
#endif /* ifndef _EFI_INCLUDE_ */

#include "Block.h"

/* Custom EFI definitions, used for the Daedalus project (Hybrid legacy/GPT boot) */

/* Floppy disk image GUID: Bootable by loading the first sector */
#define EFI_PART_TYPE_LEGACY_BS_GUID   \
    { 0x3C0A9D61, 0x3F0A, 0x11D5, 0x93, 0x62, 0x38, 0x33, 0xC4, 0xCA, 0x38, 0x98 }

/* Hard disk image GUID: Idem, but first sector contains a legacy partition table. */
/* Standard EFI_PART_TYPE_LEGACY_MBR_GUID = 024DEE41-33E7-11d3-9D69-0008C781F39F */

/* Relay partition GUID			// 35186be0-fd5a-418f-a1fd-5d2490245a8d */
#define EFI_PART_TYPE_RELAY_GUID   \
    { 0x35186be0, 0xfd5a, 0x418f, 0xA1, 0xFD, 0x5D, 0x24, 0x90, 0x24, 0x5A, 0x8D }

/* Boot menu partition GUID		// 2351c5e5-e0ff-4e9e-a40a-b86fb58641a9 */
#define EFI_PART_TYPE_BOOT_MENU_GUID \
    { 0x2351c5e5, 0xe0ff, 0x4e9e, 0xa4, 0x0a, 0xb8, 0x6f, 0xb5, 0x86, 0x41, 0xa9 }

/* Check and Repair program GUID	// 81d19441-d1cb-11d4-b250-419b839d4125 */
#define EFI_PART_TYPE_REPAIR_GUID \
    { 0x81d19441, 0xd1cb, 0x11d4, 0xb2, 0x50, 0x41, 0x9b, 0x83, 0x9d, 0x41, 0x25 }

/* GPT-aware Master Boot Record GUID	// b7732b9b-48fe-4a5b-a24e-4f127abb621c  */
#define EFI_HYBRID_LEGACY_MBR_GUID \
    { 0xb7732b9b, 0x48fe, 0x4a5b, 0xa2, 0x4e, 0x4f, 0x12, 0x7a, 0xbb, 0x62, 0x1c }

/* MBR backup partition GUID		// 6bfa2289-65a9-c74e-bc20-545b2e56572b */
#define EFI_PART_TYPE_MBR_BACKUP_GUID \
    { 0x6bfa2289, 0x65a9, 0xc74e, 0xbc, 0x20, 0x54, 0x5b, 0x2e, 0x56, 0x57, 0x2b }

#define GUIDCMP(g1, g2) memcmp((void *)&(g1), (void *)&(g2), sizeof(EFI_GUID)) /* Compare GUIDs */


typedef struct
    {
    EFI_TABLE_HEADER Header;
    EFI_PARTITION_ENTRY Partition;
    } EFI_PARTITION_RELAY;

typedef struct
    {
    EFI_LBA StartingLBA;	/* StartingLBA of the next boot program. */
    UINT32 DriveNumber;		/* Next boot program BIOS drive number. 0 = Boot drive (recommended). */
    UINT8 AuthorSignature[4];	/* Author signature. Example: "HP\0\0", "Msft". */
    UINT32 Version;		/* Code revision. Must increase monotonically. */
    } EFI_BOOT_PROGRAM_RELAY;
    
typedef struct
    {
    EFI_TABLE_HEADER Header;
    EFI_BOOT_PROGRAM_RELAY NextBootProg;
    } EFI_MBR_HEADER;

typedef struct
    {
    EFI_TABLE_HEADER Header;
    EFI_PARTITION_ENTRY Partition;
    EFI_BOOT_PROGRAM_RELAY NextBootProg;
    } EFI_BOOT_PROGRAM_HEADER;

typedef struct
    {
    EFI_TABLE_HEADER Header;
    EFI_PARTITION_ENTRY Partition;
    EFI_BOOT_PROGRAM_RELAY NextBootProg;
    UINT32 Timeout;		/* Timeout in seconds. Default = 2. */
    EFI_GUID DefaultGUID;	/* UniquePartitionGUID of last user choice. Default = 0. */
    EFI_GUID BootOnce;		/* UniquePartitionGUID of partition to boot once on. */
    EFI_GUID LastBootGUID;	/* UniquePartitionGUID of last partition booted on. */
    } EFI_BOOT_MENU_HEADER;	/* This is a subclass of EFI_BOOT_PROGRAM_HEADER. The beginning should be the same. */


/* Global variables */
extern EFI_GUID guidFloppyImage;
extern EFI_GUID guidHardDiskImage;
extern EFI_GUID guidBootMenu;
extern EFI_GUID guidMbr;
extern EFI_GUID guidMbrBackup;
extern EFI_GUID guidRelay;


/* Gpt Access */
typedef struct
    {
    HANDLE hBlockDev;
    EFI_PARTITION_TABLE_HEADER *pGptHdr;	/* Buffer with the full sector! */
    int iSectorSize;				/* Logical sector size. */
    int iSectPerSect;				/* Number of logical sectors per physical sector */
    } GPTREF, *HGPT;

HGPT GptOpen(HANDLE hBlockDev);
void GptClose(HGPT hGPT);
int GptBlockRead(HGPT hGPT, QWORD qwSector, WORD wNum, void FAR *pBuf);
int GptBlockWrite(HGPT hGPT, QWORD qwSector, WORD wNum, void FAR *pBuf);
int GptReadEntry(HGPT hGPT, int iPartition, EFI_PARTITION_ENTRY *pGptEntryBuf);
int GptWriteEntry(HGPT hGPT, int iPartition, EFI_PARTITION_ENTRY *pGptEntryBuf);
int GptAllocSectors(HGPT hGPT, EFI_PARTITION_ENTRY *pGptEntryBuf);
int GptWriteHeader(HGPT hGPT);
int GptAllocFileCopy(HGPT hGPT, char *pszFileName, EFI_PARTITION_ENTRY *pNewEntry);
int GptBootProgramList(HGPT hGPT);
int GptBootProgramAdd(HGPT hGPT, char *pBuf);
int GptBootProgramDelete(HGPT hGPT, QWORD qwLBA);

#endif /* ifndef _GPT_H_ */
