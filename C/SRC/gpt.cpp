/*****************************************************************************\
*                                                                             *
*   Filename	    gpt.cpp						      *
*									      *
*   Description     Manage GUID Partition Tables			      *
*                                                                             *
*   Notes	    TO DO: Make this a UTF-8 app, to display Unicode names    *
*			   correctly.					      *
*									      *
*   History								      *
*    2000-09-11 JFL Created this file.					      *
*    2016-07-07 JFL Removed built-in routines, and use SYSLIB's instead.      *
*		    Display human-readable size, with variable units.	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.0"
#define PROGRAM_DATE    "2016-07-08"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************ Win32-specific definitions *************************/

#ifdef _WIN32           // Automatically defined when targeting a Win32 applic.

#define OS_NAME "WIN32"

#include <windows.h>

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		// Automatically defined by MSVC 1.5

#define OS_NAME "DOS"

typedef void *HANDLE;

#endif

/********************** End of OS-specific definitions ***********************/

#define FALSE 0
#define TRUE 1

#include "qword.h"
#include "harddisk.h"
#include "uuid.h"
#include "gpt.h"

#define streq(s1, s2) (!strcmp(s1, s2)) /* For the main test routine only */

#define fail(message) { printf message; exit(1); }

/* Global variables */

extern "C" {
/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */
int iVerbose = FALSE;
}

/* Prototypes */

void usage(void);
void DumpBuf(void far *fpBuf, WORD wStart, WORD wStop);
int FormatSize(QWORD &qwSize, char *pBuf, size_t nBufSize);
int dump_part(MASTERBOOTSECTOR *pb);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Program main routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|									      |
|    1998/10/05 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[])
    {
    int iErr;
    int i;
    int iHDisk = 0;
    HANDLE hDrive;
    HDGEOMETRY sHdGeometry;
    int iList = FALSE;		// If TRUE, dump the MBR and GPT partition tables
    int iCreate = FALSE;	// If TRUE, Create a new GPT on every disk

    /* Get the command line arguments */

    for (i=1; i<argc; i++)
	{
	char *arg = argv[i];
	if ((arg[0] == '-') || (arg[0] == '/')) /* It's a switch */
	    {
	    char *opt = arg+1;
	    if (   streq(opt, "help")
		|| streq(opt, "h")
		|| streq(opt, "?"))
		{
		usage();
		}
	    if (streq(opt, "c"))
		{
		iCreate = TRUE;
		continue;
		}
            DEBUG_CODE(
	      if (streq(opt, "d"))
		  {
		  DEBUG_ON();
		  printf("Debug mode.\n");
		  iVerbose = TRUE;
		  setvbuf(stdout, NULL, _IONBF, 0); /* Disable output buffering, to get all output in case of crash */
		  continue;
		  }
	    )
	    if (streq(opt, "l"))
		{
		iList = TRUE;
		continue;
		}
	    if (streq(opt, "v"))
		{
		iVerbose = TRUE;
		continue;
		}
	    printf("Unrecognized switch %s. Ignored.\n", arg);
	    continue;
	    }
	printf("Unexpected argument: %s\nIgnored.\n", arg);
	break;	/* Ignore other arguments */
	}

    /* Set default to iList if nothing requested */
    if ((!iList) && (!iCreate)) iList = TRUE;

    /* For every physical drives in the system, dump the MBR and GPT partition tables */
    if (iList) for (iHDisk=0; (hDrive = HardDiskOpen(iHDisk, 0)) != 0; iHDisk++)
	{
	char *pBuf;
	EFI_PARTITION_TABLE_HEADER *pGptHdr;
	char *pBuf2;
	EFI_PARTITION_ENTRY *pPartEntry;
	char szHdName[8] = "hd0:";	// Hard disk name
	HANDLE hGptDrive;
	HGPT hGpt;
	WORD wSectorSize;
	QWORD qwSize;
	char szSize[8];

	printf("\n");
	if (iHDisk) {
	  printf("-------------------------------------------------------------------------------\n\n");
	}

	HardDiskGetGeometry(hDrive, &sHdGeometry);
	qwSize = sHdGeometry.qwSectors * (DWORD)(sHdGeometry.wSectorSize);
	FormatSize(qwSize, szSize, sizeof(szSize));
	printf("Hard disk #%d: %sB  (%ld/%ld/%ld)\n", iHDisk,
		/* (long)((Qword2Double(sHdGeometry.qwSectors) * sHdGeometry.wSectorSize) / 1000000) */ szSize,
		sHdGeometry.dwXlatCyls, sHdGeometry.dwXlatHeads, sHdGeometry.dwXlatSects);

	// Dump the legacy partition table.
	wSectorSize = sHdGeometry.wSectorSize;
	pBuf = (char *)malloc(wSectorSize);
	pBuf2 = (char *)malloc(wSectorSize);
	if ((!pBuf) || (!pBuf2)) fail(("Not enough memory (1)"));
	pGptHdr = (EFI_PARTITION_TABLE_HEADER *)pBuf;
	pPartEntry = (EFI_PARTITION_ENTRY *)pBuf2;
	memset(pBuf2, 0, sHdGeometry.wSectorSize);

	iErr = HardDiskRead(hDrive, _QWORD(0), 1, pBuf);
	if (iErr) fail(("Error %d reading the MBR.\n", iErr));

	if (iVerbose) DumpBuf(pBuf, 0x1BE, 0x200);
	dump_part((MASTERBOOTSECTOR *)pBuf);

	// Dump the GPT, if any
	sprintf(szHdName, "hd%d:", iHDisk);
	HardDiskClose(hDrive);
	hGptDrive = BlockOpen(szHdName, "r"); // GptXxx() procs use Block handles, not HardDisk handles. 
	if (!hGptDrive) continue; // Should not happen as HardDiskOpen succeeded above.
	hGpt = GptOpen(hGptDrive);
	if (hGpt)
	    {
	    UINT32 n;

	    printf("\nGPT:\n");

	    // iErr = HardDiskRead(hDrive, _QWORD(1), 1, pBuf);
	    // if (iErr) fail(("Error %d reading the GPT header.\n", iErr));
	    pGptHdr = hGpt->pGptHdr;
	    if (iVerbose) DumpBuf(pGptHdr, 0, sizeof(EFI_PARTITION_TABLE_HEADER));

	    char szQwBuf[20];
	    printf("Main GPT LBA = 0x%s\n", qwtox(pGptHdr->MyLBA, szQwBuf));
	    printf("Alt. GPT LBA = 0x%s\n", qwtox(pGptHdr->AlternateLBA, szQwBuf));
	    printf("First LBA = 0x%s\n", qwtox(pGptHdr->FirstUsableLBA, szQwBuf));
	    printf("Last LBA = 0x%s\n", qwtox(pGptHdr->LastUsableLBA, szQwBuf));
	    printf("Disk GUID = ");
	    PrintUuid((uuid_t *)&(pGptHdr->DiskGUID));
	    printf("\n");
	    printf("Part. LBA = 0x%s\n", qwtox(pGptHdr->PartitionEntryLBA, szQwBuf));
	    printf("# of entries = 0x%X\n", pGptHdr->NumberOfPartitionEntries);
	    printf("Entry size = 0x%X\n", pGptHdr->SizeOfPartitionEntry);

	    printf("\n  #     Size         Start LBA           End LBA  Name\n");
	    for (n=0; n<pGptHdr->NumberOfPartitionEntries; n++)
	        {
	        iErr = GptReadEntry(hGpt, (int)n, pPartEntry);
	        if (iErr) continue;
	        if (IsNullUuid((uuid_t*)&pPartEntry->PartitionTypeGUID)) continue; // Unused entry
		if (iVerbose) DumpBuf(pPartEntry, 0, sizeof(EFI_PARTITION_ENTRY));
	        qwSize = pPartEntry->EndingLBA + 1 - pPartEntry->StartingLBA;
	        qwSize *= wSectorSize;
	        FormatSize(qwSize, szSize, sizeof(szSize));
	        printf("%3u ", n);
	        printf("%7sB  ", szSize);
	        printf("%16s  ", qwtox(pPartEntry->StartingLBA, szQwBuf));
	        printf("%16s  ", qwtox(pPartEntry->EndingLBA, szQwBuf));
	        for (i=0; i<36; i++) // Convert the Unicode string to ASCII
	            {
	            CHAR16 uc;
	            uc = pPartEntry->PartitionName[i];
	            if (!uc) break; // End of string
	            if ((WORD)uc >= 0x80) uc = '?'; // No equivalent ASCII character
	            printf("%c", uc);
	            }
		printf("\n");
	        }
	    GptClose(hGpt);
	    }
	BlockClose(hGptDrive);

	free(pBuf);
	free(pBuf2);
	}

#if 0	// 2016-07-07 JFL Removed this code for now, as this is too dangerous

    // Create a GPT if needed
    if (iCreate)
	{
	DWORD dwTrackSize;
	DWORD dwSectorSize;
	DWORD dwPartitions;
	DWORD dwTableLength;

	hDrive = HardDiskOpen(iHDisk, 0);
	HardDiskGetGeometry(hDrive, &sHdGeometry);
	/*
	   Sectors in 1st track:
	      1 Master Boot sector;
	      1 GPT Header sector
	      1 GPT Header backup sector
	      NPT Partition tables sectors
	      NPS Partition data sectors
	   Sizes
	      SS Size of a sector
	      SP Size of a partition entry in the partition table
	   Equations:
	      Track size: TS = 3 + NPT + NPS
	      Partitions: NPS = NPT * (SS/SP)
	   Resolution:
	      NPT = (SP * (TS-3)) / (SS + SP)
	      NPS = (SS * (TS-3)) / (SS + SP)

	*/
	wSectorSize = sHdGeometry.wSectorSize;
	pBuf = (char *)malloc(wSectorSize);
	pBuf2 = (char *)malloc(wSectorSize);

	dwTrackSize = sHdGeometry.dwXlatSects;
	dwSectorSize = sHdGeometry.wSectorSize;
	dwPartitions =   (dwSectorSize * (dwTrackSize-3))
		       / (dwSectorSize + sizeof(EFI_PARTITION_ENTRY));
	dwTableLength = (dwTrackSize-3) - dwPartitions;

	uuid_init();		// Initialize UUID generation seeds.

	pGptHdr->Header.Signature = *(QWORD *)(char *)EFI_PTAB_HEADER_ID;
	pGptHdr->Header.Revision = 0x00010000;
	pGptHdr->Header.HeaderSize = sizeof(EFI_PARTITION_TABLE_HEADER);
	pGptHdr->Header.CRC32 = 0;				// TO DO: Compute CRCs!
	pGptHdr->Header.Reserved = 0;

	pGptHdr->MyLBA = _QWORD(1);				// The LBA of the GPT
	pGptHdr->AlternateLBA = _QWORD(dwTrackSize - 2);	// Backup GPT in the next to last sector in 1st track
	pGptHdr->FirstUsableLBA = _QWORD(dwTableLength + 2);// Allows to define 1 partition/sector following GPT
	pGptHdr->LastUsableLBA = _QWORD(dwTrackSize - 1);	// Last sector in 1st track
	uuid_create((uuid_t *)&(pGptHdr->DiskGUID)); 	// Generate a new GUID
	pGptHdr->PartitionEntryLBA = _QWORD(2);		// Partition entries begin in the next sector
	pGptHdr->NumberOfPartitionEntries = (dwTableLength * dwSectorSize) / sizeof(EFI_PARTITION_ENTRY);
	pGptHdr->SizeOfPartitionEntry = sizeof(EFI_PARTITION_ENTRY);
	pGptHdr->PartitionEntryArrayCRC32 = 0;		// TO DO: Compute CRCs!

	// Set the GPT header
	if (iVerbose) DumpBuf(pBuf, 0, 80);

	iErr = HardDiskWrite(hDrive, pGptHdr->MyLBA, 1, pBuf);
	if (iErr) fail(("Error %d writing the GPT header.\n", iErr));
	// Set the backup GPT header
	iErr = HardDiskWrite(hDrive, pGptHdr->AlternateLBA, 1, pBuf);
	if (iErr) fail(("Error %d writing the GPT backup header.\n", iErr));

	// Set all entries as unused
	for (DWORD dw=(DWORD)pGptHdr->PartitionEntryLBA; dw < (DWORD)pGptHdr->FirstUsableLBA; dw++)
	    {
	    iErr = HardDiskWrite(hDrive, _QWORD(dw), 1, pBuf2);
	    if (iErr) fail(("Error %d writing the GPT entries.\n", iErr));
	    }

	free(pBuf);
	free(pBuf2);
	}
#endif

    return 0;
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help for this program.		      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    N/A 						      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|     1998/10/14 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage()
    {
    printf("\
\n\
GPT Manager Version 0.1 for " OS_NAME "\n\
\n\
Usage:\n\
\n\
GPT [switches]\n\
\n\
Switches:\n\
\n\
  -v    Display verbose information.\n"
#ifdef _DEBUG
"  -d    Debug mode.\n"
#endif
);
    exit(0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DumpBuf						      |
|									      |
|   Description     Display the contents of a memory buffer		      |
|									      |
|   Parameters	    void far *fpBuf Buffer address			      |
|		    WORD wStart     Index of the first byte to display	      |
|		    WORD wStop	    Index of the first byte NOT to display    |
|									      |
|   Notes	    Aligns the output on offsets multiple of 0x10.	      |
|									      |
|   Returns	    None						      |
|									      |
|   History								      |
|    1999/05/26 JFL Extracted this code from older programs.		      |
*									      *
\*---------------------------------------------------------------------------*/

#define PERLINE 0x10			// Number of bytes dumped per line

void DumpBuf(void far *fpBuf, WORD wStart, WORD wStop)
    {
    WORD w, wLine, wColumn;
    unsigned char c;
    char far *fpc = (char far *)fpBuf;

    for (wLine = wStart - (wStart % PERLINE); wLine < wStop; wLine += PERLINE)
	{
	printf("%04X  ", wLine);

	// On the left hand dump the hexadecimal data
	for (wColumn = 0; wColumn < PERLINE; wColumn++)
	    {
	    if (!(wColumn & 3)) printf(" ");
	    w = wLine + wColumn;
	    if ((w >= wStart) && (w < wStop))
		printf("%02X ", (unsigned char)(fpc[w]));
	    else
		printf("%3s", "");
	    }
	// On the right hand side, display the ASCII characters
	printf(" ");
	for (wColumn = 0; wColumn < PERLINE; wColumn++)
	    {
	    if (!(wColumn & 3)) printf(" ");
	    w = wLine + wColumn;
	    c = fpc[w];
	    if (c < ' ') c = '.';   // Control character would display garbage
	    if ((w >= wStart) && (w < wStop))
		printf("%c", c);
	    else
		printf(" ");
	    }
	printf("\n");
	}

    return;
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function	    FormatSize						      |
|									      |
|   Description	    Format a disk size in a human-readable way		      |
|									      |
|   Notes	    Make sure there are at most 4 significant digits	      |
|		    							      |
|   History								      |
|     2016-07-07 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

char szUnits[] = " KMGTPE"; // Unit prefixes for 1, 10^3, 10^6, 10^9, etc

int FormatSize(QWORD &qwSize, char *pBuf, size_t nBufSize) {
  int i;
  for (i=0; i<6; i++) {
    if ((qwSize >> 13) == qwZero) break; // If (qwSize < 8K)
    qwSize >>= 10;	// Change to the next higher scale
  }
  return _snprintf(pBuf, nBufSize, "%u %c", (DWORD)qwSize, szUnits[i]);
}

/*---------------------------------------------------------------------------*\
|									      *
|   Function	    dump_part						      |
|									      |
|   Description	    Dump a partition table on screen			      |
|									      |
|   Parameters	    pb      Pointer to a copy of the disk sector 0	      |
|									      |
|   Returns	    TRUE/FALSE	   TRUE if the partition table is valid	      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|     1991-05-30 JFL Initial implementation.				      |
|     1994-12-15 JFL Changed RomSetup-specific types to standard Windows types|
|     1999-02-17 JFL Added recognition of FAT32 partitions, hidden ones, etc. |
|     2016-07-07 JFL Use new routine FormatSize to display a friendly size.   |
*									      *
\*---------------------------------------------------------------------------*/

/* Partition type names */

typedef struct
    {
    int iType;
    char *pszName;
    } TYPENAME;

TYPENAME type_name[] =	       /* "type" field in partition structure */
    {
    // Note: For more, see Ralph Brown interrupt list, at int 19h definition.
    { 0x00, "None" },		    /* Entry not used */
    { 0x01, "FAT12" },		    /* DOS primary partition, 12 bits FAT */
    { 0x02, "Xenix Root" },	    /* Xenix */
    { 0x03, "Xenix /usr" },	    /* Xenix */
    { 0x04, "FAT16 < 32 MB" },	    /* DOS primary partition, 16 bits FAT */
    { 0x05, "Extended DOS" },	    /* DOS 3.3 extended partition */
    { 0x06, "FAT16" },		    /* DOS 4 > 32 MB partition */
    { 0x07, "QNX/HPFS/NTFS..." },   /* Installable File System. See inside part. */
    { 0x08, "AIXboot, OS/2 1.x" },  /* AIX boot partition; OS/2 1.x; etc. */
    { 0x09, "AIXdata" },	    /* AIX data partition */
    { 0x0A, "OS/2 boot mgr" },	    /* OS/2 boot manager; etc. */
    { 0x0B, "FAT32" },		    /* DOS 7.1 FAT32 partition */
    { 0x0C, "FAT32X" }, 	    /* DOS 7.1 FAT32 partition, Using LBA */
    { 0x0E, "FAT16X" }, 	    /* DOS 4 > 32 MB partition, using LBA */
    { 0x0F, "Extended-X" },	    /* DOS 3.3 extended partition, using LBA */
    { 0x10, "OPUS" },		    /* AIM alliance & Taligent (?) */
    { 0x11, "Hidden FAT12" },	    /* DOS primary partition, 12 bits FAT */
    { 0x12, "Compaq diags" },	    /* Compaq diagnostics */
    { 0x14, "Hidden FAT16<32M" },   /* DOS primary partition, 16 bits FAT */
    { 0x15, "Hidden Extended" },    /* DOS 3.3 extended partition */
    { 0x16, "Hidden FAT16" },	    /* DOS 4 > 32 MB partition */
    { 0x17, "Hidden NTFS" },	    /* Installable File System. See inside part. */
    { 0x18, "AST Hibernate" },	    /* ? */
    { 0x1B, "Hidden FAT32" },	    /* DOS 7.1 FAT32 partition */
    { 0x1C, "Hidden FAT32X" },	    /* DOS 7.1 FAT32 partition, Using LBA */
    { 0x1E, "Hidden FAT16X" },	    /* DOS 4 > 32 MB partition, using LBA */
    { 0x1F, "Hidden ExtendedX" },   /* DOS 3.3 extended partition, using LBA */
    { 0x20, "OSF1" },		    // Willowsoft Overture File System (OFS1)
    { 0x21, "FSo2" },		    // FSo2
    { 0x23, "Reserved" },	    // officially listed as reserved
    { 0x24, "NEC MS-DOS 3.x" },     // NEC MS-DOS 3.x
    { 0x26, "Reserved" },	    // officially listed as reserved
    { 0x31, "Reserved" },	    // officially listed as reserved
    { 0x33, "Reserved" },	    // officially listed as reserved
    { 0x34, "Reserved" },	    // officially listed as reserved
    { 0x36, "Reserved" },	    // officially listed as reserved
    { 0x38, "Theos" },		    // Theos
    { 0x3C, "PQMagic recovery" },   // PowerQuest PartitionMagic recovery partition
    { 0x40, "VENIX 80286" },	    // VENIX 80286
    { 0x41, "PowerPC boot" },	    // PowerPC boot partition
    { 0x42, "SFS" },		    // Secure File System by Peter Gutmann
    { 0x45, "EUMEL/Elan" },	    // EUMEL/Elan
    { 0x46, "EUMEL/Elan" },	    // EUMEL/Elan
    { 0x47, "EUMEL/Elan" },	    // EUMEL/Elan
    { 0x48, "EUMEL/Elan" },	    // EUMEL/Elan
    { 0x4D, "QNX" },		    // QNX
    { 0x4E, "QNX Secondary" },	    // QNX
    { 0x4F, "QNX Secondary" },	    // QNX
    { 0x4F, "Oberon" }, 	    // Oberon boot/data partition
    { 0x50, "OnTrack R/O" },	    // OnTrack Disk Manager, read-only partition
    { 0x51, "NOVELL" }, 	    // Novell
    { 0x51, "OnTrack R/W" },	    // OnTrack Disk Manager, read/write partition
    { 0x52, "CP/M" },		    // CP/M
    { 0x52, "V/386" },		    // Microport System V/386
    { 0x53, "OnTrack R/O" },	    // OnTrack Disk Manager, write-only partition???
    { 0x54, "OnTrack DDO" },	    // OnTrack Disk Manager (DDO)
    { 0x55, "EZ-Drive" },	    // EZ-Drive (see also INT 13/AH=FFh"EZ-Drive")
    { 0x56, "VFeature" },	    // GoldenBow VFeature
    { 0x5C, "Priam EDISK" },	    // Priam EDISK
    { 0x61, "SpeedStor" },	    // SpeedStor
    { 0x63, "Unix" },		    // Unix SysV/386, 386/ix, Mach, MtXinu BSD 4.3 on Mach
    { 0x64, "NetWare 286" },	    // Novell NetWare 286
    { 0x65, "NetWare (3.11)" },     // Novell NetWare (3.11)
    { 0x67, "Novell" }, 	    // Novell
    { 0x68, "Novell" }, 	    // Novell
    { 0x69, "Novell" }, 	    // Novell
    { 0x70, "DiskSecure" },	    // DiskSecure Multi-Boot
    { 0x71, "Reserved" },	    // officially listed as reserved
    { 0x73, "Reserved" },	    // officially listed as reserved
    { 0x74, "Reserved" },	    // officially listed as reserved
    { 0x75, "PC/IX" },		    // PC/IX
    { 0x76, "Reserved" },	    // officially listed as reserved
    { 0x7E, "F.I.X." }, 	    // F.I.X.
    { 0x80, "Minix < v1.4a" },	    // Minix v1.1 - 1.4a
    { 0x81, "Linux Boot" },	    // Linux
    { 0x82, "Linux Swap" },	    // Linux Swap partition
    { 0x82, "Solaris" },	    // Solaris (Unix)
    { 0x83, "Linux ext2" },	    // Linux native file system (ext2fs/xiafs)
    { 0x84, "Hidden FAT16" },	    // OS/2-renumbered type 04h partition (related to hiding DOS C: drive)
    { 0x85, "Linux EXT" },	    // Linux EXT
    { 0x86, "FAT16 stripe set" },   // FAT16 volume/stripe set (Windows NT)
    { 0x87, "NTFS stripe set" },    // NTFS volume/stripe set
    { 0x8B, "FAT32 stripe set" },   // FAT32 volume/stripe set (Windows NT)
    { 0x8C, "FAT32 stripe LBA" },   // FAT32 volume/stripe set with LBA (Windows NT)
    { 0x93, "Amoeba filesys" },     // Amoeba file system
    { 0x94, "Amoeba bb table" },    // Amoeba bad block table
    { 0x99, "Mylex EISA SCSI" },    // Mylex EISA SCSI
    { 0xA0, "Hibernation" },	    // Phoenix NoteBIOS Power Management "Save-to-Disk" partition
    { 0xA1, "Reserved" },	    // officially listed as reserved
    { 0xA3, "Reserved" },	    // officially listed as reserved
    { 0xA4, "Reserved" },	    // officially listed as reserved
    { 0xA5, "FreeBSD, BSD/386" },   // FreeBSD, BSD/386
    { 0xA6, "OpenBSD" },	    // OpenBSD
    { 0xA9, "NetBSD" }, 	    // NetBSD (http://www.netbsd.org/)
    { 0xB1, "Reserved" },	    // officially listed as reserved
    { 0xB3, "Reserved" },	    // officially listed as reserved
    { 0xB4, "Reserved" },	    // officially listed as reserved
    { 0xB6, "Reserved" },	    // officially listed as reserved
    { 0xB7, "BSDI filesys" },	    // BSDI file system (secondarily swap)
    { 0xB8, "BSDI swap" },	    // BSDI swap partition (secondarily file system)
    { 0xBE, "Solaris boot" },	    // Solaris boot partition
    { 0xC0, "DR-DOS secure" },	    // DR-DOS/Novell DOS secured partition
    { 0xC0, "CTOS" },		    // CTOS
    { 0xC1, "DR-DOS secure 12" },   // DR DOS 6.0 LOGIN.EXE-secured 12-bit FAT partition
    { 0xC4, "DR-DOS secure 16" },   // DR DOS 6.0 LOGIN.EXE-secured 16-bit FAT partition
    { 0xC6, "DR-DOS secure Hg" },   // DR DOS 6.0 LOGIN.EXE-secured Huge partition
    { 0xC7, "Syrinx Boot" },	    // Syrinx Boot
    { 0xCB, "DR-DOS secure 32" },   // Reserved for DR-DOS secured FAT32
    { 0xCC, "DR-DOS secure32X" },   // Reserved for DR-DOS secured FAT32 (LBA)
    { 0xCE, "DR-DOS secure16X" },   // Reserved for DR-DOS secured FAT16 (LBA)
    { 0xD0, "MU-DOS secure 12" },   // Multiuser DOS secured FAT12
    { 0xD1, "MU-DOS secure 12" },   // Old Multiuser DOS secured FAT12
    { 0xD4, "MU-DOS secure 12" },   // Old Multiuser DOS secured FAT16 (<= 32M)
    { 0xD5, "MU-DOS secure 12" },   // Old Multiuser DOS secured extended partition
    { 0xD6, "MU-DOS secure 12" },   // Old Multiuser DOS secured FAT16 (> 32M)
    { 0xD8, "CP/M-86" },	    // CP/M-86
    { 0xDB, "CP/M" },		    // CP/M, Concurrent CP/M, Concurrent DOS
    { 0xDB, "CTOS" },		    // CTOS (Convergent Technologies OS)
    { 0xDF, "TeraByte Bootit" },    // TeraByte Unlimited Bootit
    { 0xE1, "SpeedStor FAT12" },    // SpeedStor 12-bit FAT extended partition
    { 0xE3, "Storage Dims" },	    // Storage Dimensions
    { 0xE4, "SpeedStor FAT16" },    // SpeedStor 16-bit FAT extended partition
    { 0xE5, "Reserved" },	    // officially listed as reserved
    { 0xE6, "Reserved" },	    // officially listed as reserved
    { 0xEB, "BeOS" },		    // BeOS BFS (BFS1)
    { 0xEE, "GPT Protection" },     // EFI GPT Protection Partition
    { 0xEF, "EFI System Part." },   // EFI System Partition
    { 0xF1, "Storage Dims" },	    // Storage Dimensions
    { 0xF2, "DOS secondary" },	    // DOS 3.3+ secondary partition
    { 0xF3, "Reserved" },	    // officially listed as reserved
    { 0xF4, "SpeedStor" },	    // SpeedStor
    { 0xF4, "Storage Dims" },	    // Storage Dimensions
    { 0xF5, "Prologue" },	    // Prologue
    { 0xF6, "Reserved" },	    // officially listed as reserved
    { 0xFB, "VMware VMFS" },	    // VMware VMFS
    { 0xFB, "VMware VMKCORE" },	    // VMware VMKCORE
    { 0xFE, "LANstep" },	    // LANstep
    { 0xFE, "IBM PS/2 IML" },	    // IBM PS/2 IML (Initial Microcode Load) partition
    { 0xFF, "Xenix BBT" },	    // Xenix bad block table
    };

#define KNOWN_TYPES (sizeof(type_name)/sizeof(TYPENAME))

int dump_part(MASTERBOOTSECTOR *pb)
    {
    PARTITION *pp;
    int i, j;
    WORD bcyl, ecyl;
    WORD type;
    char *pszPartitionName;
    char *pszFormat;
    char szSize[8];
    QWORD qwSize;

    printf("\nBoot sector ID marker %04X (%s).\n", pb->mbsSignature,
            (pb->mbsSignature == 0xAA55) ? "Correct" : "Should be AA55");
    printf("\
Partitions             | Beginning  |     End     |       Sectors      |   Size\n");
    printf("\
Type              Boot | Cyl  Hd Se | Cyl.  Hd Se |   First     Number |  Bytes\n");

    pp = &(pb->mbsPart[0]);
    for (i = 0; i < 4; i++, pp++)
        {
        bcyl = pp->beg_lcyl + ((WORD)(pp->beg_hcyl) << 8);
        ecyl = pp->end_lcyl + ((WORD)(pp->end_hcyl) << 8);
        type = pp->type;
	pszPartitionName = "Other";
	for (j=0; j < KNOWN_TYPES; j++)
	    {
	    if ((int)type == type_name[j].iType)
		{
		pszPartitionName = type_name[j].pszName;
		break;
		}
	    }

	qwSize = (QWORD)(pp->n_sectors) << 9; /* multiply by 512 */
	FormatSize(qwSize, szSize, sizeof(szSize));
	pszFormat = "%3u %-16s %c |%4u %3u%3u | %4u %3u%3u |%8lu %10lu |%7s\n";
	if (pp->n_sectors == 0xFFFFFFFF) // Force displaying -1 here v
	pszFormat = "%3u %-16s %c |%4u %3u%3u | %4u %3u%3u |%8lu %10ld |%7s\n";
	printf(pszFormat,
		type, pszPartitionName, pp->boot ? 'Y' : 'N',
                bcyl, pp->beg_head, pp->beg_sect,
                ecyl, pp->end_head, pp->end_sect,
                pp->first_sector, pp->n_sectors,
                /* pp->n_sectors / 1953 */ szSize);
        }

    return 0;
    }

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
