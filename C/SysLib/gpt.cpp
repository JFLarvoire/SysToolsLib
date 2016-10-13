/*****************************************************************************\
*                                                                             *
*   Filename:	    gpt.cpp						      *
*									      *
*   Description:    OS-independant GPT access routines			      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2001-02-20 JFL Created this file.					      *
*    2001-12-19 JFL Renamed EFI_HYBRID_LEGACY_MBS_GUID			      *
*			 as EFI_HYBRID_LEGACY_MBR_GUID.			      *
*		    Renamed guidMBS as guidMbr.				      *
*		    Renamed guidHeaderBackup as guidMbrBackup.		      *
*		    Renamed guidLegacy as guidHardDiskImage.		      *
*		    Added guidFloppyImage.				      *
*		    Updated GptAllocFileCopy() to the the partition type GUID *
*		     from the file header if possible.			      *
*    2002-01-03 JFL Updated to support Legacy+GPT spec version 0.96.	      *
*		    Recognize relays, and update their GUID and CRC.	      *
*    2002-05-21 JFL Added support for dumping GPTs from track 0 image files.  *
*    2002-06-28 JFL Fixed a bug in the backup header contents.		      *
*    2003-01-17 JFL Added #define NO_QWORD_CONSTRUCTORS before qword.h.	      *
*    2016-07-01 JFL Removed warning.                                   	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <malloc.h>

#include "IsMBR.h"	// Boot sector and MBR identification

/************************ Win32-specific definitions *************************/

#ifdef _WIN32           // Automatically defined by MSVC 5.0

#include <windows.h>

#define OS_NAME "WIN32"

// Equivalent to MS-DOS-specific routines.
#define _fmalloc malloc
#define _ffree free
#define _memavl() 999999

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		// Automatically defined by MSVC 1.5

#define OS_NAME "DOS"

#define NO_QWORD_CONSTRUCTORS	// Work around class in structure bug.
#include "qword.h"

#endif

/********************** End of OS-specific definitions ***********************/

// Custom definitions
#include "gpt.h"	// Also includes standard EFI definitions
#include "uuid.h"
#include "crc32.h"

#ifdef _DEBUG

extern "C" {
extern int iDebug;	// Defined in main module. If TRUE, display debug infos.
extern int iVerbose;	// Defined in main module. If TRUE, display progress infos.
extern int iReadOnly;	// Defined in main module. If TRUE, do not write to disk.
}

extern void DumpBuf(void far *fpBuf, WORD wStart, WORD wStop);

#endif // _DEBUG

static QWORD qwOne = _QWORD(1); // Useful constant, avoids having to use _QWORD(1) multiple times.

EFI_GUID guidFloppyImage = EFI_PART_TYPE_LEGACY_BS_GUID;
EFI_GUID guidHardDiskImage = EFI_PART_TYPE_LEGACY_MBR_GUID;
EFI_GUID guidBootMenu = EFI_PART_TYPE_BOOT_MENU_GUID;
EFI_GUID guidMbr = EFI_HYBRID_LEGACY_MBR_GUID;
EFI_GUID guidMbrBackup = EFI_PART_TYPE_MBR_BACKUP_GUID;
EFI_GUID guidRelay = EFI_PART_TYPE_RELAY_GUID;

// C++ like construct, for use in C-compatible routines
#ifndef NEW
#define NEW(object) ((object *)malloc(sizeof(object)))
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptOpen						      |
|									      |
|   Description:    Open a GPT for reading/writing			      |
|									      |
|   Parameters:     HANDLE hBlockDev	Drive where the GPT resides	      |
|		    							      |
|   Returns:	    HGPT hGPT		Handle to access that GPT	      |
|                   			NULL ==> No valid GPT found.          |
|                                                                             |
|   Notes:	                                                              |
|		                                                              |
|   History:								      |
|									      |
|    2001/01/08 JFL Created this routine.				      |
|    2002/05/21 JFL Added support for image files of HD track 0.	      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptOpenFail()  { goto open_return; }

HGPT GptOpen(HANDLE hBlockDev)
    {
    int iErr;
    // To be cleaned up eventually
    GPTREF *hGPT;
    // End of clean-up zone
    
#ifdef _DEBUG
    if (iDebug) printf("GptOpen(hBlockDev=%p)\n", hBlockDev);
#endif

    // Create a new GPTREF object
    hGPT = NEW(GPTREF);
    if (!hGPT) return 0;
    memset(hGPT, 0, sizeof(GPTREF));		// Clear everything
    hGPT->hBlockDev = hBlockDev;

    // Manage logical sectors that may be different from physical sectors in the case of image files.
    hGPT->iSectorSize = BlockSize(hGPT->hBlockDev);	// Sector size. 1 for files; 512 for disks.
    hGPT->iSectPerSect = 1;	    // Number of logical sectors per physical sector
    if (BlockType(hBlockDev) == 0)  // If it's a file image of a GPT...
	{			    // ... Force all file accesses per 512-byte blocks
	hGPT->iSectPerSect = 512 / hGPT->iSectorSize;	// Logical sectors per physical sector
	hGPT->iSectorSize = 512;			// New logical sector size
	}

    // Allocate buffers to access drive data.
    hGPT->pGptHdr = (EFI_PARTITION_TABLE_HEADER *)malloc(hGPT->iSectorSize);
    if (!hGPT->pGptHdr) GptOpenFail();
    	
    // Get the GPT header.
    iErr = GptBlockRead(hGPT, qwOne, 1, hGPT->pGptHdr);
    if (iErr) GptOpenFail();
    
    // Debug. TO BE REMOVED.
    CheckCrc(hGPT->iSectorSize, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x5C, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x58, &(hGPT->pGptHdr->Header));
    if (!strncmp((char *)&(hGPT->pGptHdr->Header.Signature), "TRAP IFE", 8))
        {
        strncpy((char *)&(hGPT->pGptHdr->Header.Signature), EFI_PTAB_HEADER_ID, 8);
        }
//    DumpBuf(&(hGPT->pGptHdr->Header), 0, hGPT->pGptHdr->Header.HeaderSize);
    CheckCrc(hGPT->iSectorSize, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x5C, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x58, &(hGPT->pGptHdr->Header));
    hGPT->pGptHdr->PartitionEntryArrayCRC32 = 0;
    CheckCrc(hGPT->iSectorSize, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x5C, &(hGPT->pGptHdr->Header));
    CheckCrcAltSize(hGPT->iSectorSize, 0x58, &(hGPT->pGptHdr->Header));
        
    // Check if the header is valid
    if ((   strncmp((char *)&(hGPT->pGptHdr->Header.Signature), EFI_PTAB_HEADER_ID, 8)
	 && strncmp((char *)&(hGPT->pGptHdr->Header.Signature), "TRAP IFE", 8))
//	|| (!CheckCrc(hGPT->iSectorSize, &(hGPT->pGptHdr->Header)))
       ) GptOpenFail();

    // Success
    return hGPT;	// HGPT is same type as GPTREF*
    
open_return:
    GptClose(hGPT);
    
    return (HGPT)NULL;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptClose						      |
|									      |
|   Description:    Free-up resources allocated by GptOpen()		      |
|									      |
|   Parameters:     HGPT hGPT		Handle to access that GPT	      |
|		    							      |
|   Returns:	    Nothing						      |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/08 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _BIOS
#pragma warning(disable:4100) /* Avoid warnings "'hGPT' : unreferenced formal parameter" */
#endif

void GptClose(HGPT hGPT)
    {
#ifdef _DEBUG
    if (iDebug) printf("GptClose(hBlockDev=%p)\n", hGPT->hBlockDev);
#endif

    free(hGPT->pGptHdr);
    free(hGPT);
    }

#ifdef _BIOS
#pragma warning(default:4100)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptBlockRead					      |
|									      |
|   Description:    Read N sectors from a real disk or a GPT image file.      |
|									      |
|   Parameters:     HANDLE hDevice		Specifies the input device.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    Front end to the BlockRead routine.			      |
|									      |
|   History:								      |
|									      |
|    2002/05/27 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GptBlockRead(HGPT hGPT, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    QWORD qw = qwSector;
    qw *= hGPT->iSectPerSect;

    return BlockRead(hGPT->hBlockDev, qw, (WORD)(wNum*hGPT->iSectPerSect), pBuf);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptBlockWrite					      |
|									      |
|   Description:    Write N sectors to a real disk or to a GPT image file.    |
|									      |
|   Parameters:     HANDLE hDevice		Specifies the input device.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    Front end to the BlockWrite routine.		      |
|									      |
|   History:								      |
|									      |
|    2002/05/27 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GptBlockWrite(HGPT hGPT, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    QWORD qw = qwSector;
    qw *= hGPT->iSectPerSect;

    return BlockWrite(hGPT->hBlockDev, qw, (WORD)(wNum*hGPT->iSectPerSect), pBuf);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptReadEntry					      |
|									      |
|   Description:    Read a partition entry from a GPT			      |
|									      |
|   Parameters:     HANDLE hBlockDev	Drive to allocate space on	      |
|		    int iPartition	Index of the entry to read	      |
|		    void *pGptEntryBuf	Buffer for the entry read	      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/05 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptReadEntryFail(n)  { iErr = n; goto read_return; }

int GptReadEntry(HGPT hGPT, int iPartition, EFI_PARTITION_ENTRY *pGptEntryBuf)
    {
    int iIndex;
    QWORD qwNSect;
    int iErr;
    EFI_PARTITION_ENTRY *pSectEntry;
    char *pSect;
    QWORD qwLBA;
    
#ifdef _DEBUG
    if (iDebug) printf("GptReadEntry(hBlockDev=%p, iPartition=%d)\n", hGPT->hBlockDev, iPartition);
#endif
    // Compute the number of partition entries per sector.
    int iEntryPerSect = hGPT->iSectorSize / sizeof(EFI_PARTITION_ENTRY);
        
    // Allocate a buffer to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptReadEntryFail(2);
    pSectEntry = (EFI_PARTITION_ENTRY *)pSect;
    	
    // Is the request possible?
    if ((unsigned)iPartition >= hGPT->pGptHdr->NumberOfPartitionEntries) GptReadEntryFail(1);
    
    // Locate the requested entry
    qwNSect = iPartition / iEntryPerSect;
    iIndex = iPartition % iEntryPerSect;
    qwLBA = hGPT->pGptHdr->PartitionEntryLBA;
    qwLBA += qwNSect;
    
    // Read it in
    iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
    if (iErr) GptReadEntryFail(3);
    
    // Copy the requested entry into the output buffer.
    *pGptEntryBuf = pSectEntry[iIndex];
    
read_return:
    free(pSect);
    
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptWriteEntry					      |
|									      |
|   Description:    Write a partition entry into a GPT			      |
|									      |
|   Parameters:     HANDLE hBlockDev	Drive to allocate space on	      |
|		    int iPartition	Index of the entry to read	      |
|		    void *pGptEntryBuf	Entry to be written		      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/08 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptWriteEntryFail(n)  { iErr = n; goto write_return; }

int GptWriteEntry(HGPT hGPT, int iPartition, EFI_PARTITION_ENTRY *pGptEntryBuf)
    {
    int iIndex;
    QWORD qwNSect;
    QWORD qwLBA;
    int iErr;
    EFI_PARTITION_ENTRY *pSectEntry;
    // To be cleaned up before return
    char *pSect = (char *)NULL;
    // End of clean-up zone
    
#ifdef _DEBUG
    if (iDebug) printf("GptWriteEntry(hBlockDev=%p, iPartition=%d)\n", hGPT->hBlockDev, iPartition);
#endif
    // Compute the number of partition entries per sector.
    int iEntryPerSect = hGPT->iSectorSize / sizeof(EFI_PARTITION_ENTRY);

    // Allocate a buffer to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptWriteEntryFail(2);
    pSectEntry = (EFI_PARTITION_ENTRY *)pSect;
    	
    // Is the request possible?
    if ((unsigned)iPartition >= hGPT->pGptHdr->NumberOfPartitionEntries) GptWriteEntryFail(1);
    
    // Locate the requested entry
    qwNSect = iPartition / iEntryPerSect;
    iIndex = iPartition % iEntryPerSect;
    qwLBA = hGPT->pGptHdr->PartitionEntryLBA;
    qwLBA += qwNSect;
    
    // Read it in
    iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
    if (iErr) GptWriteEntryFail(3);
    
    // Copy the entry into the output sector.
    pSectEntry[iIndex] = *pGptEntryBuf;
    
    // Write the updated sector
    iErr = GptBlockWrite(hGPT, qwLBA, 1, pSect);
    if (iErr) GptWriteEntryFail(3);
    
write_return:
    free(pSect);
    
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptAllocSectors					      |
|									      |
|   Description:    Allocate a contiguous block of sectors		      |
|									      |
|   Parameters:     HGPT hGPT		GPT to work on			      |
|		    EFI_PARTITION_ENTRY *pNewEntry	Where to store results|
|		        pNewEntry->StartingLBA	Start sector. 0 = 1st availab.|
|		        pNewEntry->EndingLBA	Number of sectors to allocate |
|		    							      |
|   Returns:	    >= 0 ==> The index of the allocated entry in the GPT      |
|                       In this case pNewEntry is filled with allocated data. |
|                       Exception: The type GUID field is left NULL.          |
|                   < 0 ==> Error.                                            |
|                       -1 = GPT full                                         |
|                       -2 = Not enough contiguous free space                 |
|                       -3 = Disk I/O error                                   |
|                       -4 = Not enough memory                                |
|                       -5 = Forced value out of GPT space or not free        |
|                                                                             |
|   Notes:          This routine should be protected in a critical section.   |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/05 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptAllocSectorsFail(n)  { iNewPartition = n; goto alloc_return; }

int GptAllocSectors(HGPT hGPT, EFI_PARTITION_ENTRY *pNewEntry)
    {
    int iPartition;
    int iNewPartition = -1;
    QWORD qwFirst = qwZero;
    QWORD qwLast = qwZero;
    // To be cleaned up before return
    QWORD *pqwFirst = (QWORD *)NULL;
    QWORD *pqwLast = (QWORD *)NULL;
    char *pGptEntryBuf = (char *)NULL;
    // End of clean-up zone
    EFI_PARTITION_ENTRY *pPartEntry;
    DWORD dwGptSect;
    int iIndex;
    int iErr;
    int j;
    DWORD dwNewSector;
    int iNewIndex;
    int nBlocks = 1;
#ifdef _DEBUG
    char szBuf1[20];
    char szBuf2[20];
#endif
    QWORD qwForceSect = pNewEntry->StartingLBA;
    QWORD qwNSect = pNewEntry->EndingLBA;
    
#ifdef _DEBUG
    if (iDebug) printf("GptAllocSectors(hBlockDev=%p, nSect=%s, forceSect=%s)\n",
                       hGPT->hBlockDev, qwtox(qwNSect, szBuf1), qwtox(qwForceSect, szBuf2));
#endif

    // Compute the number of partition entries per sector.
    int iEntryPerSect = hGPT->iSectorSize / sizeof(EFI_PARTITION_ENTRY);
        
    // Allocate buffers to access drive data.
    pGptEntryBuf = (char *)malloc(hGPT->iSectorSize);
    if (!pGptEntryBuf) GptAllocSectorsFail(-4);
    pPartEntry = (EFI_PARTITION_ENTRY *)pGptEntryBuf;
    	
    // Search the smallest contiguous block of free sectors large enough.
    // For that, build a list of the free disk areas.
    pqwFirst = (QWORD *)malloc(sizeof(QWORD) * (size_t)hGPT->pGptHdr->NumberOfPartitionEntries);
    pqwLast =  (QWORD *)malloc(sizeof(QWORD) * (size_t)hGPT->pGptHdr->NumberOfPartitionEntries);
    if (!pqwFirst || !pqwLast) GptAllocSectorsFail(-4);

    // Initially, assume all the GPT-managed area is free.
    pqwFirst[0] = hGPT->pGptHdr->FirstUsableLBA;
    pqwLast[0] = hGPT->pGptHdr->LastUsableLBA;

    // Then progress negatively, removing chunks of allocated space.
    for (iPartition = 0, iIndex = 0, dwGptSect=(DWORD)(hGPT->pGptHdr->PartitionEntryLBA);
    	 iPartition < (int)(hGPT->pGptHdr->NumberOfPartitionEntries);
    	 iPartition++, iIndex = (int)(iPartition % iEntryPerSect), dwGptSect += iIndex ? 0 : 1
    	)
    	{
    	if (iIndex == 0) // Read-in a new sector when passing a boundary.
	    {
	    QWORD qw;
	    iErr = GptBlockRead(hGPT, qw = dwGptSect, 1, pGptEntryBuf);
	    if (iErr) GptAllocSectorsFail(-3);
    	    }
    	// Skip unused entries, but keep mark of the first one.
    	if (IsNullUuid((uuid_t*)&(pPartEntry[iIndex].PartitionTypeGUID))) 
    	    {
	    // If we already have found an unused spot, just loop.
	    if (iNewPartition != -1) continue;
		    
	    // OK. Let's use this one for the allocation below.
	    iNewPartition = iPartition;
	    dwNewSector = dwGptSect;
	    iNewIndex = iIndex;
    	    continue;
    	    }
	    	
    	// This entry is in use. Record what sector area it uses.
	// printfx("Removing block %s", pPartEntry[iIndex].StartingLBA);
	// printfx("-%s\n", pPartEntry[iIndex].EndingLBA);
    	for (j=0; j<nBlocks; j++)
    	    {
    	    if (pPartEntry[iIndex].StartingLBA <= pqwLast[j])	// Then insert a hole in this segment
    	        {
    	        int k;
	    	        
    	        if (   (pqwFirst[j] < pPartEntry[iIndex].StartingLBA)
    	            && (pPartEntry[iIndex].EndingLBA < pqwLast[j]) )
    	            {	// If both ends of the partition strictly inside the free area, split it.
    	            // printf("Splitting a free block.\n");
		    for (k=nBlocks; k>j; k--)	// Move the rest of the list right by 1 slot
			{
			pqwFirst[k] = pqwFirst[k-1];
			pqwLast[k] = pqwLast[k-1];
			}
		    nBlocks += 1;
		    pqwLast[j] = pPartEntry[iIndex].StartingLBA - qwOne;
		    pqwFirst[j+1] = pPartEntry[iIndex].EndingLBA + qwOne;
		    }
		else if (   (pqwFirst[j] == pPartEntry[iIndex].StartingLBA)
    	                 && (pPartEntry[iIndex].EndingLBA == pqwLast[j]) )
    	            {	// If both ends of the partition exactly match the free area, remove it.
    	            // printf("Removing one free block.\n");
    	            nBlocks -= 1;
		    for (k=j; k<nBlocks; k++)	// Move the rest of the list left by 1 slot
			{
			pqwFirst[k] = pqwFirst[k+1];
			pqwLast[k] = pqwLast[k+1];
			}
    	            }
    	        else if (pqwFirst[j] < pPartEntry[iIndex].StartingLBA)
    	            {	// If the ends match, mark the new end
    	            // printf("Shrinking a free block end.\n");
		    pqwLast[j] = pPartEntry[iIndex].StartingLBA - qwOne;
                    }
                else
                    {	// The beginnings match. Mark the new beginning
    	            // printf("Shrinking a free block start.\n");
		    pqwFirst[j] = pPartEntry[iIndex].EndingLBA + qwOne;
                    }
		/*
		printf("Free blocks: Sectors");
		for (k=0; k<nBlocks; k++) { printfx(" %s", pqwFirst[k]); printfx("-%s;", pqwLast[k]); }
		printf("\n");
	        */
		break;	// Operation done
    	        }
    	    }
    	}
	    	
    if (iNewPartition == -1)
        {
    	GptAllocSectorsFail(-1);
    	}

    // Allocate a block of sectors for our partition data.
    if (qwForceSect != qwZero)
        {
        qwFirst = qwForceSect;
	qwLast = qwFirst + qwNSect - qwOne;
        // Scan the free block list to make sure the whole requested range is free
        for (j=0; j<nBlocks; j++)
            {
            if ((pqwFirst[j] <= qwFirst) && (pqwLast[j] >= qwLast)) break;
            }
        if (j >= nBlocks) GptAllocSectorsFail(-5);
        }
    else    // Scan the free block list to find the smallest area large enough for our data.
        {
        QWORD qwMaxWidth = _QWORD(0xFFFFFFFF, 0xFFFFFFFF);
#ifdef _DEBUG
        if (iVerbose) printf("Free blocks: Sectors");
#endif
        for (j=0; j<nBlocks; j++)
            {
#ifdef _DEBUG
            if (iVerbose)  { printfx(" %s", pqwFirst[j]); printfx("-%s;", pqwLast[j]); }
#endif
            QWORD qwWidth = qwOne + pqwLast[j] - pqwFirst[j];
            if (   (qwWidth >= qwNSect)
                && (qwWidth < qwMaxWidth) 
               )
                {
                if (   (qwFirst == qwZero)
                    || (pqwFirst[j] != hGPT->pGptHdr->LastUsableLBA) 
                   )
                    {  // Allocate that block, except if it's the last one.
                    qwMaxWidth = qwWidth;
                    qwFirst = pqwFirst[j];
                    }
                }
            }
#ifdef _DEBUG
        if (iVerbose) printf("\n");
#endif
        if (!qwFirst) GptAllocSectorsFail(-2);
        }

    qwLast = qwFirst + qwNSect - qwOne;
#ifdef _DEBUG
    if (iVerbose) { printfx("Allocated sectors 0x%s", qwFirst); printfx(" to 0x%s.\n", qwLast); }
#endif
	    
    // Fill in the new partition characteristics
    memset(pNewEntry, 0, sizeof(EFI_PARTITION_ENTRY));
    pNewEntry->PartitionTypeGUID = guidHardDiskImage;		// Anything but 0 will do at this stage.
    uuid_create((uuid_t *)&(pNewEntry->UniquePartitionGUID)); 	// Generate a new GUID
    pNewEntry->StartingLBA = qwFirst;
    pNewEntry->EndingLBA = qwLast;

alloc_return:
    free(pqwFirst);
    free(pqwLast);
    free(pGptEntryBuf);

    return iNewPartition;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptAllocFileCopy					      |
|									      |
|   Description:    Allocate a partition and copy it the given file contents  |
|									      |
|   Parameters:     HGPT hGPT		GPT to work on			      |
|		    char *pszFileName	File name			      |
|		    EFI_PARTITION_ENTRY *pNewEntry	Where to store results|
|		        pNewEntry->StartingLBA	Start sector. 0 = 1st availab.|
|		    							      |
|   Returns:	    >= 0 ==> The index of the allocated entry in the GPT      |
|                       In this case pNewEntry is filled with allocated data. |
|                   < 0 ==> Error.                                            |
|                       -1 = GPT full                                         |
|                       -2 = Not enough contiguous free space                 |
|                       -3 = Disk I/O error                                   |
|                       -4 = Not enough memory                                |
|                       -5 = Forced value out of GPT space or not free        |
|                       -10 = File not found                                  |
|                                                                             |
|   Notes:          This routine should be protected in a critical section.   |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/08 JFL Created this routine.				      |
|    2001/12/19 JFL Get the partition type GUID specified in the file, if any.|
*									      *
\*---------------------------------------------------------------------------*/

#ifndef _BIOS /* Impossible and useless for the BIOS version */

#include <stdio.h>

#define GptAllocFileCopyFail(n)  { iPartition=n; goto copy_return; }

int GptAllocFileCopy(HGPT hGPT, char *pszFileName, EFI_PARTITION_ENTRY *pNewEntry)
    {
    int i;
    int iPartition;
    QWORD qwLBA;
    int iErr;
    // To be cleaned up before return
    char *pSect = (char *)NULL;
    // End of clean-up zone
    int nSect;
    FILE *hf = NULL;
    
    // Compute the number of partition entries per sector.
    // int iEntryPerSect = hGPT->iSectorSize / sizeof(EFI_PARTITION_ENTRY);

    // Allocate a buffer to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptAllocFileCopyFail(-4);
    	
    // Open the partition image file
    hf = fopen(pszFileName, "rb");
    if (!hf) GptAllocFileCopyFail(-10);
    // Compute the number of sectors necessary.
    nSect = (int)((_filelength(_fileno(hf)) + hGPT->iSectorSize - 1) / hGPT->iSectorSize);

    // Search the smallest contiguous block of free sectors large enough.
    pNewEntry->EndingLBA = nSect;
    iPartition = GptAllocSectors(hGPT, pNewEntry);  // Side effect: pNewEntry->PartitionTypeGUID = 0;
    if (iPartition < 0) GptAllocFileCopyFail(iPartition);

#ifdef _DEBUG
    if (iVerbose)
        {
        printf("Allocating partition entry #%d for copying from file %s\n", iPartition, pszFileName);
        }
#endif
	    
    // Write the partition contents
    for (i=0, qwLBA=pNewEntry->StartingLBA; i<nSect; i++, qwLBA++)
        {
        size_t stRead;
	        
        stRead = fread(pSect, 1, hGPT->iSectorSize, hf);
        if (stRead < (size_t)(hGPT->iSectorSize)) memset(pSect+stRead, 0, hGPT->iSectorSize-stRead);
	        
	// ~~jfl 2001/12/19 Use the partition type GUID specified in the file, if any.
	if (i==0)
	    {
	    EFI_BOOT_PROGRAM_HEADER *pHdr = (EFI_BOOT_PROGRAM_HEADER *)pSect;
	    if (CheckCrc(hGPT->iSectorSize, &(pHdr->Header)))
		{
		if (!memcmp(((char *)&(pHdr->Header.Signature))+3, "MBR32", 5))
		    {
		    pNewEntry->PartitionTypeGUID = guidMbrBackup;   // If it's inside the GPT, then it's the backup!
		    }
		else	// Assume it's a relay or a boot program (Including the boot menu).
		    {	// Both have a partition entry behind the header.
		    pNewEntry->PartitionTypeGUID = pHdr->Partition.PartitionTypeGUID;
		    }
		}
	    else if (   (!memcmp(((char *)&(pHdr->Header.Signature))+3, "Relay", 5))
		     && (!uuidcmp(&(pHdr->Partition.PartitionTypeGUID), &guidRelay))
		    )	// It's a relay, but with the header incomplete (CRC not set yet).
		{
		pNewEntry->PartitionTypeGUID = guidRelay;
		// ~~jfl 2002/01/03 Update the partition header while we have it.
		uuid_create((uuid_t *)&(pHdr->Partition.UniquePartitionGUID));	// Generate a new GUID
		SetCrc(&(pHdr->Header));
		}
	    else if (IsMBR(pSect))	// Master Boot Record. Assume it's a hard disk.
		{
		pNewEntry->PartitionTypeGUID = guidHardDiskImage;
		}
	    if (IsNullUuid((uuid_t*)&(pNewEntry->PartitionTypeGUID)))	// Assume anything else is a floppy.
		{
		pNewEntry->PartitionTypeGUID = guidFloppyImage;
		}
	    }

	iErr = GptBlockWrite(hGPT, qwLBA, 1, pSect);
	if (iErr) GptAllocFileCopyFail(-3);
        }

copy_return:
    // Cleanup
    fclose(hf);
    free(pSect);
    
    return iPartition;    
    }
    
#endif /* !defined(_BIOS) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptWriteHeader					      |
|									      |
|   Description:    Write the GPT partition header and its backup	      |
|									      |
|   Parameters:     HANDLE hBlockDev	Drive to allocate space on	      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/08 JFL Created this routine.				      |
|    2002/06/28 JFL The backup header has the MyLBA and AlternateLBA fields   |
|		     reversed.						      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptWriteHeaderFail(n)  { iErr = n; goto header_return; }

int GptWriteHeader(HGPT hGPT)
    {
    int i;
    int iErr;
    char far *lpArray = (char far *)NULL;
    char far *lpc;
    int iEntryArraySize;
    int nSect;
    QWORD qwLBA;
    
#ifdef _DEBUG
    if (iDebug) printf("GptWriteHeader(hBlockDev=%p)\n", hGPT->hBlockDev);
#endif

    // Compute the entire entry array CRC, and update the header.
    iEntryArraySize = (int)(hGPT->pGptHdr->NumberOfPartitionEntries) * sizeof(EFI_PARTITION_ENTRY);
    nSect = (iEntryArraySize + hGPT->iSectorSize - 1) / hGPT->iSectorSize;
    lpArray = (char far *)_fmalloc(nSect * hGPT->iSectorSize);
    if (!lpArray) GptWriteHeaderFail(1);
    
    for (lpc=lpArray, i=0, qwLBA=hGPT->pGptHdr->PartitionEntryLBA; i<nSect; i++, lpc+=hGPT->iSectorSize, qwLBA++)
        {
	iErr = GptBlockRead(hGPT, qwLBA, 1, lpc);
	if (iErr) GptWriteHeaderFail(2);
        }

    hGPT->pGptHdr->PartitionEntryArrayCRC32 = crc32(lpArray, iEntryArraySize);
    
    // Write the backup GPT header
    // Exchange the main and alternate LBA addresses.
    qwLBA = hGPT->pGptHdr->AlternateLBA;
    hGPT->pGptHdr->AlternateLBA = hGPT->pGptHdr->MyLBA;
    hGPT->pGptHdr->MyLBA = qwLBA;
    // Update the header CRC
    SetCrc(&(hGPT->pGptHdr->Header));
    // Write the backup GPT header
    iErr = GptBlockWrite(hGPT, hGPT->pGptHdr->MyLBA, 1, hGPT->pGptHdr);
    if (iErr)
    	{
#ifdef _DEBUG
    	if (iVerbose) printf("Error %d writing the GPT backup header.\n", iErr);
#endif
    	GptWriteHeaderFail(3);
    	}

    // Write the main GPT header
    // Exchange the main and alternate LBA addresses.
    qwLBA = hGPT->pGptHdr->AlternateLBA;
    hGPT->pGptHdr->AlternateLBA = hGPT->pGptHdr->MyLBA;
    hGPT->pGptHdr->MyLBA = qwLBA;
    // Update the header CRC
    SetCrc(&(hGPT->pGptHdr->Header));
#ifdef _DEBUG
    if (iVerbose) DumpBuf(hGPT->pGptHdr, 0, (WORD)hGPT->pGptHdr->Header.HeaderSize);
#endif
    // Write the main GPT header
    iErr = GptBlockWrite(hGPT, hGPT->pGptHdr->MyLBA, 1, hGPT->pGptHdr);
    if (iErr)
    	{
#ifdef _DEBUG
    	if (iVerbose) printf("Error %d writing the GPT header.\n", iErr);
#endif
    	GptWriteHeaderFail(4);
    	}
    	
header_return:
    _ffree(lpArray);
    
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptBootProgramAdd					      |
|									      |
|   Description:    Add a boot program to the Boot Program chain	      |
|									      |
|   Parameters:     HGPT		GPT to work on			      |
|		    char *pNewSect	First sector the boot program	      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                   1 = Not enough memory                                     |
|                   2 = I/O error                                             |
|                   3 = Invalid boot program chain                            |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/15 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptBootProgramAddFail(n)  { iErr = n; goto bpadd_return; }

int GptBootProgramAdd(HGPT hGPT, char *pNewSect)
    {
    int iErr;
    // Buffer for the 1st sector of the new boot program
    EFI_BOOT_PROGRAM_HEADER *pNewProgram = (EFI_BOOT_PROGRAM_HEADER *)pNewSect;
    // Buffer for the 1st sector of other programs in the chain
    char *pSect = (char *)NULL;
    EFI_MBR_HEADER *pMbr;
    EFI_BOOT_PROGRAM_HEADER *pOldProgram;
    //
    QWORD qwLBA, qwLastLBA;
#ifdef _DEBUG
    char szQwordBuf[20];
#endif
    QWORD qwNewLBA = pNewProgram->Partition.StartingLBA;
    EFI_GUID guidNew = pNewProgram->Partition.PartitionTypeGUID;
    
#ifdef _DEBUG
    if (iDebug) printf("GptBootProgramAdd(hBlockDev=%p, lba=%s)\n", hGPT->hBlockDev, qwtox(qwNewLBA, szQwordBuf));
#endif

    // Allocate buffers to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptBootProgramAddFail(1);
    pMbr = (EFI_MBR_HEADER *)pSect;
    pOldProgram = (EFI_BOOT_PROGRAM_HEADER *)pSect;

    // Update the boot program chain
    iErr = GptBlockRead(hGPT, qwZero, 1, pSect);
    if (iErr) GptBootProgramAddFail(2);

    if (   (memcmp(((char *)&(pMbr->Header.Signature))+3, "MBR32", 5))	// Unknown signature
        || (!CheckCrc(128, &(pMbr->Header)))				// invalid CRC
       ) GptBootProgramAddFail(3);              // Invalid boot program chain
                
    qwLBA = pMbr->NextBootProg.StartingLBA;
    qwLastLBA = 0;
    if (!qwLBA)					// The chain is empty so far
        {
        pMbr->NextBootProg.StartingLBA = qwNewLBA;	// Start the chain with this boot program.
        }
    else if (GUIDCMP(guidNew, guidBootMenu))	// It's not empty, but Anything else than boot menus is inserted ahead of the chain.
        {
        pMbr->NextBootProg.StartingLBA = qwNewLBA;	// Start the chain with this new boot program.

        pNewProgram->NextBootProg.StartingLBA = qwLBA;	// And link this new boot program to the previous chain.
        }
    else for (;;)				// Else scan the chain until the current boot menu or the end
        {
        iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
        if (iErr) GptBootProgramAddFail(2);
                
        if (!CheckCrc(hGPT->iSectorSize, &(pOldProgram->Header))) GptBootProgramAddFail(3);
                
	if (!GUIDCMP(pOldProgram->Partition.PartitionTypeGUID, guidBootMenu))
            {  // There's already one boot menu in the chain!
            // Back-up to the previous boot program.
            qwLBA = qwLastLBA;
            iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
	    if (iErr) GptBootProgramAddFail(2);
            if (!qwLBA)
                pMbr->NextBootProg.StartingLBA = qwNewLBA;	// Start the chain with this boot menu.
            else
                pOldProgram->NextBootProg.StartingLBA = qwNewLBA;// Append this boot menu to the chain
            break;
            }
                    
        qwLastLBA = qwLBA;
        
        if (!pOldProgram->NextBootProg.StartingLBA)	// Reached the end of the chain!
            {
            pOldProgram->NextBootProg.StartingLBA = qwNewLBA;	// Append this boot menu to the chain
            break;
            }
                
        qwLBA = pOldProgram->NextBootProg.StartingLBA;
        }

    SetCrc(&pNewProgram->Header);			// Update the CRC.
    iErr = GptBlockWrite(hGPT, qwNewLBA, 1, pNewSect);
    if (iErr) GptBootProgramAddFail(2);

    SetCrc(&pOldProgram->Header);			// Update the CRC.
    iErr = GptBlockWrite(hGPT, qwLastLBA, 1, pSect);
    if (iErr) GptBootProgramAddFail(2);

bpadd_return:
    free(pSect);
    
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptBootProgramList					      |
|									      |
|   Description:    Display the Boot Program chain			      |
|									      |
|   Parameters:     HGPT		GPT to work on			      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                   1 = Not enough memory                                     |
|                   2 = I/O error                                             |
|                   3 = Invalid boot program head                             |
|                   4 = Invalid boot program chain                            |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/15 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptBootProgramListFail(n)  { iErr = n; goto bplist_return; }

int GptBootProgramList(HGPT hGPT)
    {
    int iErr;
    // Buffer for the 1st sector of programs in the chain
    char *pSect = (char *)NULL;
    EFI_MBR_HEADER *pMbr;
    EFI_BOOT_PROGRAM_HEADER *pOldProgram;
    //
    QWORD qwLBA;
    
    // Allocate buffers to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptBootProgramListFail(1);
    pMbr = (EFI_MBR_HEADER *)pSect;
    pOldProgram = (EFI_BOOT_PROGRAM_HEADER *)pSect;
    
    // Scan the chain
    printf("Boot program chain: ");
    
    iErr = GptBlockRead(hGPT, qwZero, 1, pSect);
    if (iErr) GptBootProgramListFail(2);

    if (   (memcmp(((char *)&(pMbr->Header.Signature))+3, "MBR32", 5))	// Unknown signature
        || (!CheckCrc(128, &(pMbr->Header)))				// invalid CRC
       ) GptBootProgramListFail(3);              // Invalid boot program chain
                
    qwLBA = pMbr->NextBootProg.StartingLBA;
    if (!qwLBA)					// The chain is empty so far
        {
        printf("Empty.");
        }
    else for (;;)				// Else scan the chain until the current boot menu or the end
        {
        printfx("LBA=0x%s ", qwLBA);
        
        iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
        if (iErr) GptBootProgramListFail(2);
                
        if (!CheckCrc(hGPT->iSectorSize, &(pOldProgram->Header))) GptBootProgramListFail(4);
        
        if (!pOldProgram->NextBootProg.StartingLBA)	// Reached the end of the chain!
            {
            printf("\n");
            break;
            }
                
        qwLBA = pOldProgram->NextBootProg.StartingLBA;
        }

bplist_return:
    free(pSect);
    
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GptBootProgramDelete				      |
|									      |
|   Description:    Delete a boot program from the Boot Program chain	      |
|									      |
|   Parameters:     HGPT		GPT to work on			      |
|		    QWORD qwFirst	First sector the boot program	      |
|		    							      |
|   Returns:	    0 = Success. Else error code.			      |
|                   1 = Not enough memory                                     |
|                   2 = I/O error                                             |
|                   3 = Invalid boot program chain                            |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/16 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define GptBootProgramDeleteFail(n)  { iErr = n; goto bpdel_return; }

int GptBootProgramDelete(HGPT hGPT, QWORD qwFirst)
    {
    int iErr;
    // Buffer for the 1st sector of other programs in the chain
    char *pSect = (char *)NULL;
    EFI_MBR_HEADER *pMbr;
    EFI_BOOT_PROGRAM_HEADER *pBootProgram;
    // Buffer for the 2nd sector of other programs in the chain
    char *pSect2 = (char *)NULL;
    EFI_BOOT_PROGRAM_HEADER *pDeletedProgram;
    //
    QWORD qwLBA, qwLastLBA;
#ifdef _DEBUG
    char szQwordBuf[20];
#endif
    
#ifdef _DEBUG
    if (iDebug) printf("GptBootProgramDelete(hBlockDev=%p, lba=%s)\n", hGPT->hBlockDev, qwtox(qwFirst, szQwordBuf));
#endif
    if (!qwFirst) return 0;	// Prevent problems

    // Allocate buffers to access drive data.
    pSect = (char *)malloc(hGPT->iSectorSize);
    if (!pSect) GptBootProgramDeleteFail(1);
    pMbr = (EFI_MBR_HEADER *)pSect;
    pBootProgram = (EFI_BOOT_PROGRAM_HEADER *)pSect;

    pSect2 = (char *)malloc(hGPT->iSectorSize);
    if (!pSect2) GptBootProgramDeleteFail(1);
    pDeletedProgram = (EFI_BOOT_PROGRAM_HEADER *)pSect2;

    // Update the boot program chain
    iErr = GptBlockRead(hGPT, qwZero, 1, pSect);
    if (iErr) GptBootProgramDeleteFail(2);

    if (   (memcmp(((char *)&(pMbr->Header.Signature))+3, "MBR32", 5))	// Unknown signature
        || (!CheckCrc(128, &(pMbr->Header)))				// invalid CRC
       ) GptBootProgramDeleteFail(3);              // Invalid boot program chain
                
    qwLBA = pMbr->NextBootProg.StartingLBA;
    qwLastLBA = 0;
    if (!qwLBA)					// The chain is empty so far
        {
        goto bpdel_return;				// Requested boot program NOT in the chain!
        }
    else while (qwLBA != qwFirst)			// Else scan the chain until the end
        {
        iErr = GptBlockRead(hGPT, qwLBA, 1, pSect);
        if (iErr) GptBootProgramDeleteFail(2);
                
        if (!CheckCrc(hGPT->iSectorSize, &(pBootProgram->Header))) GptBootProgramDeleteFail(3);
                
        qwLastLBA = qwLBA;
        
        if (!pBootProgram->NextBootProg.StartingLBA)	// Reached the end of the chain!
            {
            goto bpdel_return;				// Requested boot program NOT in the chain!Done.
            }
                
        qwLBA = pBootProgram->NextBootProg.StartingLBA;
        }

    iErr = GptBlockRead(hGPT, qwFirst, 1, pSect2);
    if (iErr) GptBootProgramDeleteFail(2);

    if (!CheckCrc(hGPT->iSectorSize, &(pDeletedProgram->Header))) GptBootProgramDeleteFail(3);
                
    qwLBA = pDeletedProgram->NextBootProg.StartingLBA;	// The boot program that was to follow de deleted one
    if (!qwLastLBA)
        pMbr->NextBootProg.StartingLBA = qwLBA;
    else
        pBootProgram->NextBootProg.StartingLBA = qwLBA;	// And link this new boot program to the previous chain.

    SetCrc(&pBootProgram->Header);			// Update the CRC.
    iErr = GptBlockWrite(hGPT, qwLastLBA, 1, pSect);
    if (iErr) GptBootProgramDeleteFail(2);

bpdel_return:
    free(pSect);
    free(pSect2);
    
    return iErr;
    }

