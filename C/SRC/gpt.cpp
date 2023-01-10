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
*    2016-12-05 JFL Reformated the source to modern coding standards.         *
*		    Improved the partition size formating readability.	      *
*		    Use the same units for disk and partition sizes.	      *
*		    Added options -H and -I to control the disk size SI unit. *
*		    Use the oprintf() routine for object formatting.	      *
*		    Version 1.1.					      *
*    2016-12-09 JFL Added option -x for symmetry with option -t added on 12/5.*
*		    Version 1.1.1.					      *
*    2017-04-15 JFL When listing drives, tolerate missing indexes, as one     *
*		    drive may have been recently unplugged. Version 1.1.2.    * 
*    2017-06-28 JFL Fixed warnings. No functional code change. Version 1.1.3. * 
*    2017-08-03 JFL Display MsvcLibX & SysLib library versions in DOS & Win.  *
*		    Fixed and improved the FormatSize() routine. Version 1.1.4.
*    2017-08-15 JFL Fixed warnings in Visual Studio 2015. Version 1.1.5.      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.1.6.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.1.7.      *
*    2021-10-20 JFL Display the correct partition size when it's 0xFFFFFFFF.  *
*		    Correctly display 64-bit LBAs.			      *
*                   Skip drives with no media inside. Report it in verb. mode.*
*		    Version 1.2.                                              *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Dump GUID Partition Tables"
#define PROGRAM_NAME    "gpt"
#define PROGRAM_VERSION "1.2"
#define PROGRAM_DATE    "2021-10-20"

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
#include "oprintf.h"

/* SysToolsLib include files */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define streq(s1, s2) (!strcmp(s1, s2)) /* For the main test routine only */

#define fail(message) { printf message; exit(1); }

/* Global variables */

extern "C" {
/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */
int iVerbose = FALSE;
int iBase = 16;		// Base to use for formatted input/output (10 or 16)
char cBase = 'X';	// Format character to use for formatted output (d, u, X)
int iKB = 1000;		// Base for hard disk sizes KB, MB, etc. 1000 or 1024
}

/* Prototypes */

void usage(void);
void DumpBuf(void FAR *fpBuf, WORD wStart, WORD wStop);
int dump_part(MASTERBOOTSECTOR *pMbs, QWORD qwDiskSectors);
int FormatSize(QWORD &qwSize, char *pBuf, size_t nBufSize, int iKB);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     Main program routine				      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS.			      |
|									      |
|   History								      |
|									      |
|    1998-10-05 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[]) {
  int iErr;
  int i;
  int iHDisk = 0;
  HANDLE hDrive;
  HDGEOMETRY sHdGeometry;
  int iList = FALSE;		// If TRUE, dump the MBR and GPT partition tables
  int iCreate = FALSE;	// If TRUE, Create a new GPT on every disk

  /* Get the command line arguments */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if ((arg[0] == '-') || (arg[0] == '/')) { /* It's a switch */
      char *opt = arg+1;
      if (   streq(opt, "help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "c")) {
	iCreate = TRUE;
	continue;
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_ON();
	  printf("Debug mode.\n");
	  iVerbose = TRUE;
	  setvbuf(stdout, NULL, _IONBF, 0); /* Disable output buffering, to get all output in case of crash */
	  continue;
	}
      )
      if (streq(opt, "H")) {
	iKB = 1000;
	continue;
      }
      if (streq(opt, "I")) {
	iKB = 1024;
	continue;
      }
      if (streq(opt, "l")) {
	iList = TRUE;
	continue;
      }
      if (streq(opt, "t")) {
	iBase = 10;
	cBase = 'u';
	continue;
      }
      if (streq(opt, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      if (streq(opt, "x")) {
	iBase = 16;
	cBase = 'X';
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
  if (iList) {
    int nMissing = 0; /* Number of missing disk indexes so far */
    for (iHDisk=0; nMissing < 32; iHDisk++) {
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

      hDrive = HardDiskOpen(iHDisk, READONLY);
      if (!hDrive) {	/* If there is no disk with that index */
      	nMissing += 1;		/* Then count the missing drive */
      	continue;		/* And keep searching, as one drive may have been recently unplugged */
      }

      iErr = HardDiskGetGeometry(hDrive, &sHdGeometry);
      if (iErr) {
      	if (iVerbose) {
	  printf("\n");
	  if (iHDisk) {
	    printf("-------------------------------------------------------------------------------\n\n");
	  }
	  printf("Hard Disk #%d: No media in the drive\n", iHDisk);
	}
	continue;
      }
      printf("\n");
      if (iHDisk) {
	printf("-------------------------------------------------------------------------------\n\n");
      }
  
      qwSize = sHdGeometry.qwSectors * (DWORD)(sHdGeometry.wSectorSize);
      FormatSize(qwSize, szSize, sizeof(szSize), iKB);
      oprintf("Hard disk #{%d}: {%s} in {%I64{%c}} sectors", iHDisk, szSize, cBase, sHdGeometry.qwSectors);
      oprintf(" ({%l{%c}}/{%l{%c}}/{%l{%c}})\n",
	      cBase, sHdGeometry.dwXlatCyls, cBase, sHdGeometry.dwXlatHeads, cBase, sHdGeometry.dwXlatSects);
  
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
      dump_part((MASTERBOOTSECTOR *)pBuf, sHdGeometry.qwSectors);
  
      // Dump the GPT, if any
      sprintf(szHdName, "hd%d:", iHDisk);
      HardDiskClose(hDrive);
      hGptDrive = BlockOpen(szHdName, "r"); // GptXxx() procs use Block handles, not HardDisk handles. 
      if (!hGptDrive) continue; // Should not happen as HardDiskOpen succeeded above.
      hGpt = GptOpen(hGptDrive);
      if (hGpt) {
	UINT32 n;
  
	printf("\nGPT:\n");
  
	// iErr = HardDiskRead(hDrive, _QWORD(1), 1, pBuf);
	// if (iErr) fail(("Error %d reading the GPT header.\n", iErr));
	pGptHdr = hGpt->pGptHdr;
	if (iVerbose) DumpBuf(pGptHdr, 0, sizeof(EFI_PARTITION_TABLE_HEADER));
  
	oprintf("Main GPT LBA = {%I64{%c}}\n", cBase, pGptHdr->MyLBA);
	oprintf("Alt. GPT LBA = {%I64{%c}}\n", cBase, pGptHdr->AlternateLBA);
	oprintf("First LBA = {%I64{%c}}\n", cBase, pGptHdr->FirstUsableLBA);
	oprintf("Last LBA = {%I64{%c}}\n", cBase, pGptHdr->LastUsableLBA);
	oprintf("Disk GUID = ");
	PrintUuid((uuid_t *)&(pGptHdr->DiskGUID));
	printf("\n");
	oprintf("Part. LBA = {%I64{%c}}\n", cBase, pGptHdr->PartitionEntryLBA);
	oprintf("# of entries = {%{%c}}\n", cBase, pGptHdr->NumberOfPartitionEntries);
	oprintf("Entry size = {%{%c}}\n", cBase, pGptHdr->SizeOfPartitionEntry);
  
	printf("\n  #     Size         Start LBA           End LBA  Name\n");
	for (n=0; n<pGptHdr->NumberOfPartitionEntries; n++) {
	  iErr = GptReadEntry(hGpt, (int)n, pPartEntry);
	  if (iErr) continue;
	  if (IsNullUuid((uuid_t*)&pPartEntry->PartitionTypeGUID)) continue; // Unused entry
	  if (iVerbose) DumpBuf(pPartEntry, 0, sizeof(EFI_PARTITION_ENTRY));
	  qwSize = pPartEntry->EndingLBA + 1 - pPartEntry->StartingLBA;
	  qwSize *= wSectorSize;
	  FormatSize(qwSize, szSize, sizeof(szSize), iKB);
	  printf("%3u ", n);
	  printf("%8s  ", szSize);
	  oprintf("{%16I64{%c}}  ", cBase, pPartEntry->StartingLBA);
	  oprintf("{%16I64{%c}}  ", cBase, pPartEntry->EndingLBA);
	  for (i=0; i<36; i++) { // Convert the Unicode string to ASCII
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
  }

#if 0	// 2016-07-07 JFL Removed this code for now, as this is too dangerous

  // Create a GPT if needed
  if (iCreate) {
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
    for (DWORD dw=(DWORD)pGptHdr->PartitionEntryLBA; dw < (DWORD)pGptHdr->FirstUsableLBA; dw++) {
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
*                                                                             *
|  Function	    usage						      |
|									      |
|  Description      Display a brief help for this program		      |
|									      |
|  Parameters       None						      |
|									      |
|  Returns	    N/A 						      |
|									      |
|  Notes								      |
|									      |
|  History								      |
|    1998-10-14 JFL Initial implementation.				      |
|    2017-07-28 JFL Display the MsvcLibX and SysLib libraries versions.       |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: gpt [switches]\n\
\n\
Switches:\n\
"
#ifdef _DEBUG
"\
  -d    Debug mode.\n\
"
#endif
"\
  -t    Use base 10 for input and output.\n\
  -v    Display verbose information.\n\
  -V    Display this program version and exit.\n\
  -x    Use base 16 for input and output. (default)\n\
"
#include "footnote.h"
);
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description     Test if a command line argument is a switch.	      |
|									      |
|   Parameters      char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
#if defined(_WIN32) || defined(_MSDOS)
    case '/':
#endif
      return (*(short*)pszArg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
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
|    1999-05-26 JFL Extracted this code from older programs.		      |
*									      *
\*---------------------------------------------------------------------------*/

#define PERLINE 0x10			// Number of bytes dumped per line

void DumpBuf(void FAR *fpBuf, WORD wStart, WORD wStop) {
  WORD w, wLine, wColumn;
  unsigned char c;
  char FAR *fpc = (char FAR *)fpBuf;

  for (wLine = wStart - (wStart % PERLINE); wLine < wStop; wLine += PERLINE) {
    printf("%04X  ", wLine);

    // On the left hand dump the hexadecimal data
    for (wColumn = 0; wColumn < PERLINE; wColumn++) {
      if (!(wColumn & 3)) printf(" ");
      w = wLine + wColumn;
      if ((w >= wStart) && (w < wStop)) {
	printf("%02X ", (unsigned char)(fpc[w]));
      } else {
	printf("%3s", "");
      }
    }
    // On the right hand side, display the ASCII characters
    printf(" ");
    for (wColumn = 0; wColumn < PERLINE; wColumn++) {
      if (!(wColumn & 3)) printf(" ");
      w = wLine + wColumn;
      c = fpc[w];
      if (c < ' ') c = '.';   // Control character would display garbage
      if ((w >= wStart) && (w < wStop)) {
      	printf("%c", c);
      } else {
	printf(" ");
      }
    }
    printf("\n");
  }

  return;
}

/*---------------------------------------------------------------------------*\
|									      *
|  Function	    FormatSize						      |
|									      |
|  Description	    Format a disk size in a human-readable way		      |
|									      |
|  Arguments	    iKB = 1000 => Output a size in XB (Powers of 1000)	      |
|		    iKB = 1024 => Output a size in XiB (Powers of 1024)	      |
|		    iKB = 1440 => Output a size in floppy XB (1024 * 1000^N)  |
|		    							      |
|  Notes	    Makes sure the decimal number has at most 3 characters.   |
|		    Ex: "12 MB" or "123 MB" or "1.2 GB"			      |
|		    (Except for floppys. Ex: "1.44 MB")			      |
|		    							      |
|		    Do not use floating point numbers, because we want to     |
|		    avoid dragging in the floating point libraries in MS-DOS  |
|		    programs if possible. This makes them much smaller.	      |
|		    							      |
|  History								      |
|    2016-07-07 JFL Initial implementation.				      |
|    2016-12-05 JFL Output 2 or 3 significant digits, possibly with a period. |
|		    (Improves readbility in the same output space.)	      |
|		    Added argument iKB to select the SI base. (KB or KiB)     |
|    2017-07-08 JFL Do not display a period if there's no digit afterwards.   |
|    2017-07-28 JFL Added the floppy-specific algorithm, if iKB is 1440.      |
|		    Avoid having two spaces for sizes in B (ie. < 1 KB).      |
|		    Fixed the output for DOS.				      |
|    2017-07-30 JFL Added a workaround for an MSVC 1.52c bug.		      |
|    2017-08-03 JFL Cosmetic change: Align byte sizes with other XB sizes.    |
*		    							      *
\*---------------------------------------------------------------------------*/

char szUnits[] = " KMGTPE"; // Unit prefixes for 2^0, 2^10, 2^20, 2^30, etc

#pragma warning(disable:4459) /* Ignore the "declaration of 'iKB' hides global declaration" warning */

int FormatSize(QWORD &qwSize, char *pBuf, size_t nBufSize, int iKB) {
  int i;
  char szFraction[10] = ".";
  int iFloppy = FALSE;
  char szUnit[3] = " B";
  char *pszUnit = szUnit;
  WORD wKB = (WORD)iKB;
  if (iKB == 1440) iFloppy = TRUE;	// Use the floppy-specific algorithm
  if (iKB != 1000) wKB = (WORD)1024;	// The only 2 valid values are 1000 and 1024
  for (i=0; i<6; i++) {
    if ((qwSize >> 14) == qwZero) break; // If (qwSize < 16K)
    qwSize /= wKB;	// Change to the next higher scale
    if (iFloppy) wKB = 1000;		// Floppys divide once by 1024, then by 1000
  }
  DWORD dwSize = (DWORD)qwSize;
  if (dwSize >= (10UL*wKB)) { // We'll have two significant digits. Switch to the next scale.
    dwSize /= wKB;
    i++;
  } else if (dwSize >= wKB) { // We'll have 1 significant digits, + 1 after the period.
    DWORD dwFraction = (dwSize % wKB) * 1000UL;
#if _MSDOS // Work around for an MSVC 1.52c DOS compiler bug. Do not remove, else the second sprintf prints "000".
    // Forces MSVC 1.52c to actually compute dwFraction, instead of (mistakenly) simplifying the formula in the second sprintf.
    sprintf(szFraction+1, "%ld", dwFraction);
#endif // _MSDOS
    sprintf(szFraction+1, "%03ld", dwFraction / wKB);
    dwSize /= wKB;
    i++;
  } else {
    szFraction[0] = '\0';	// No fractional part needed.
  }
  if (iFloppy) {	// Floppy disk size formatting. Ex: "360 KB" or "1.2 MB" or "1.44 MB"
    szFraction[3] = '\0';	// Limit the fractional part to 2 digits at most.
    if (szFraction[2] == '0') szFraction[2] = '\0';	// But keep only 1 if the 2nd is 0
  } else {		// Hard disk size formatting. Ex: "32 GB" or "320 GB" or "3.2 TB"
    szFraction[2] = '\0';	// Limit the fractional part to 1 digit at most.
  }
  if (!szFraction[1]) szFraction[0] = '\0'; // If there's nothing behind the dot, then remove the dot.
  szUnit[0] = szUnits[i];
  if (szUnit[0] == ' ') pszUnit = "B "; // If there's no prefix, use just "B"
  return _snprintf(pBuf, nBufSize, "%lu%s %s", dwSize, szFraction, pszUnit);
}

#pragma warning(default:4459) /* Restore the "declaration of 'iKB' hides global declaration" warning */

/*---------------------------------------------------------------------------*\
|									      *
|   Function	    dump_part						      |
|									      |
|   Description	    Dump a partition table on screen			      |
|									      |
|   Parameters	    pb      Pointer to a copy of the disk sector 0	      |
|		    qwTotal Optional # of sectors on the disk. 0=ignore.      |
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
|     2021-10-18 JFL Changed type 0x27 from MS Service to MS Recovery.        |
|		     In verbose mode, also display free space between parts.  |
*									      *
\*---------------------------------------------------------------------------*/

/* Partition type names */

typedef struct {
  int iType;
  char *pszName;
} TYPENAME;

/* See https://en.wikipedia.org/wiki/Partition_type for details */
TYPENAME type_name[] = {       /* "type" field in partition structure */
  // Note: For more, see Ralph Brown interrupt list, at int 19h definition.
  { 0x00, "None" },		    /* Entry not used */
  { 0x01, "FAT12" },		    /* DOS primary partition, 12 bits FAT */
  { 0x02, "Xenix Root" },	    /* Xenix */
  { 0x03, "Xenix /usr" },	    /* Xenix */
  { 0x04, "FAT16 <32M" },	    /* DOS primary partition, 16 bits FAT */
  { 0x05, "Extended CHS <8G" },	    /* DOS 3.3 extended partition */
  { 0x06, "FAT16B CHS <8G" },	    /* DOS 4 > 32 MB partition */
  { 0x07, "NTFS/HPFS/QNX..." },     /* Installable File System. See inside part. */
  { 0x08, "AIXboot, OS/2 1.x" },    /* AIX boot partition; OS/2 1.x; etc. */
  { 0x09, "AIXdata" },		    /* AIX data partition */
  { 0x0A, "OS/2 boot mgr" },	    /* OS/2 boot manager; etc. */
  { 0x0B, "FAT32 CHS" },	    /* DOS 7.1 FAT32 partition */
  { 0x0C, "FAT32X LBA" }, 	    /* DOS 7.1 FAT32 partition, Using LBA */
  { 0x0E, "FAT16X LBA" }, 	    /* DOS 4 > 32 MB partition, using LBA */
  { 0x0F, "Extented LBA" },	    /* DOS 3.3 extended partition, using LBA */
  { 0x10, "OPUS" },		    /* AIM alliance & Taligent (?) */
  { 0x11, "Hidden FAT12" },	    /* DOS primary partition, 12 bits FAT */
  { 0x12, "OEM Service" },	    /* Compaq diagnostics */
  { 0x14, "Hidden FAT16<32M" },     /* DOS primary partition, 16 bits FAT */
  { 0x15, "Hidden Extd <8G" },      /* DOS 3.3 extended partition */
  { 0x16, "Hidden FAT16B" },	    /* DOS 4 > 32 MB partition */
  { 0x17, "Hidden NTFS" },	    /* Installable File System. See inside part. */
  { 0x18, "AST Hibernate" },	    /* ? */
  { 0x1B, "Hidden FAT32" },	    /* DOS 7.1 FAT32 partition */
  { 0x1C, "Hidden FAT32X" },	    /* DOS 7.1 FAT32 partition, Using LBA */
  { 0x1E, "Hidden FAT16X" },	    /* DOS 4 > 32 MB partition, using LBA */
  { 0x1F, "Hidden Extd LBA" },      /* DOS 3.3 extended partition, using LBA */
  { 0x20, "OSF1" },		    // Willowsoft Overture File System (OFS1)
  { 0x21, "FSo2" },		    // FSo2
  { 0x23, "Reserved" },		    // officially listed as reserved
  { 0x24, "NEC MS-DOS 3.x" },	    // NEC MS-DOS 3.x
  { 0x26, "Reserved" },		    // officially listed as reserved
  { 0x27, "MS Recovery" },	    // Hidden NTFS partition with Windows Recovery WIM image
  { 0x31, "Reserved" },		    // officially listed as reserved
  { 0x33, "Reserved" },		    // officially listed as reserved
  { 0x34, "Reserved" },		    // officially listed as reserved
  { 0x35, "OS/2 JFS" },		    // OS/2 Journaling File system
  { 0x36, "Reserved" },		    // officially listed as reserved
  { 0x38, "Theos" },		    // Theos
  { 0x39, "Plan 9" },		    // Bell Lab's Plan 9
  { 0x3C, "PQMagic recovery" },     // PowerQuest PartitionMagic recovery partition
  { 0x3C, "PQMagic NetWare" },      // PowerQuest PartitionMagic hidden NetWare
  { 0x40, "VENIX 80286" },	    // VENIX 80286
  { 0x41, "PowerPC boot" },	    // PowerPC boot partition
  { 0x42, "MS Dyn Extd" },	    // Dynamic Extended partition, aka. Logical Disk Manager (LDM) partition.
  { 0x45, "EUMEL/Elan" },	    // EUMEL/Elan
  { 0x46, "EUMEL/Elan" },	    // EUMEL/Elan
  { 0x47, "EUMEL/Elan" },	    // EUMEL/Elan
  { 0x48, "EUMEL/Elan" },	    // EUMEL/Elan
  { 0x4C, "ETH Oberon" },	    // ETH Zürich Aos FS
  { 0x4D, "QNX Primary" },	    // QNX
  { 0x4E, "QNX Secondary" },	    // QNX
  { 0x4F, "QNX Tertiary" },	    // QNX
  { 0x4F, "ETH Oberon" }, 	    // Oberon boot/data partition
  { 0x50, "OnTrack R/O" },	    // OnTrack Disk Manager, read-only partition
  { 0x51, "NOVELL" }, 		    // Novell
  { 0x51, "OnTrack R/W" },	    // OnTrack Disk Manager, read/write partition
  { 0x52, "CP/M" },		    // CP/M
  { 0x52, "V/386" },		    // Microport System V/386
  { 0x53, "OnTrack R/O" },	    // OnTrack Disk Manager, write-only partition???
  { 0x54, "OnTrack DDO" },	    // OnTrack Disk Manager (DDO)
  { 0x55, "EZ-Drive" },		    // EZ-Drive (see also INT 13/AH=FFh"EZ-Drive")
  { 0x56, "VFeature" },		    // GoldenBow VFeature
  { 0x5C, "Priam EDISK" },	    // Priam EDISK
  { 0x61, "SpeedStor" },	    // SpeedStor
  { 0x63, "Unix" },		    // Unix SysV/386, 386/ix, Mach, MtXinu BSD 4.3 on Mach
  { 0x64, "NetWare 286" },	    // Novell NetWare 286
  { 0x65, "NetWare (3.11)" },	    // Novell NetWare (3.11)
  { 0x67, "Novell" },	 	    // Novell
  { 0x68, "Novell" },	 	    // Novell
  { 0x69, "Novell" },	 	    // Novell
  { 0x70, "DiskSecure" },	    // DiskSecure Multi-Boot
  { 0x71, "Reserved" },		    // officially listed as reserved
  { 0x73, "Reserved" },		    // officially listed as reserved
  { 0x74, "Reserved" },		    // officially listed as reserved
  { 0x75, "PC/IX" },		    // PC/IX
  { 0x76, "Reserved" },		    // officially listed as reserved
  { 0x7E, "F.I.X." },	 	    // F.I.X.
  { 0x80, "Minix < v1.4a" },	    // Minix v1.1 - 1.4a, or MS NT Fault Tolerant partition
  { 0x81, "Minix 1.4b+" },	    // Minix Boot partition
  { 0x82, "Minix Swap" },	    // Minix Swap partition
  { 0x82, "Solaris" },		    // Solaris (Unix)
  { 0x83, "Linux ext2" },	    // Linux native file system (ext2fs/xiafs)
  { 0x84, "Hibernation" },	    // MS APM suspend 2 disk, and Intel Rapid Start 
  { 0x85, "Linux EXT" },	    // Linux EXT
  { 0x86, "FAT16 stripe set" },	    // FAT16 volume/stripe set (Windows NT)
  { 0x87, "NTFS stripe set" },	    // NTFS volume/stripe set
  { 0x88, "Linux Plain Text" },	    // Linux EXT
  { 0x8B, "FAT32 stripe set" },	    // FAT32 volume/stripe set (Windows NT)
  { 0x8C, "FAT32 stripe LBA" },	    // FAT32 volume/stripe set with LBA (Windows NT)
  { 0x8D, "FreeDOS Hid. F12" },	    // Hidden FAT12
  { 0x8E, "Linux LVM" },	    // Linux LVM
  { 0x90, "FreeDOS Hid. F16" },	    // Hidden FAT16
  { 0x91, "FreeDOS Hid. Ext" },	    // Hidden Extended
  { 0x92, "FreeDOS Hid. F16B" },    // Hidden FAT16B
  { 0x93, "Linux Hid. ext2" },	    // Linux hidded 0x83
  { 0x94, "Amoeba bb table" },      // Amoeba bad block table
  { 0x96, "ISO-9660" },		    // CHRP ISO-9660 file system
  { 0x97, "FreeDOS Hid. F32" },	    // Hidden FAT32
  { 0x98, "FreeDOS Hid. F32X" },    // Hidden FAT32X
  { 0x99, "Mylex EISA SCSI" },	    // Mylex EISA SCSI
  { 0x9A, "FreeDOS Hid. F16X" },    // Hidden FAT32X
  { 0x9B, "FreeDOS Hid. ExtX" },    // Hidden FAT32X
  { 0x9F, "BSD/OS 3.0+, BSDI" },    // BSd
  { 0xA0, "Hibernation" },	    // Phoenix NoteBIOS Power Management "Save-to-Disk" partition
  { 0xA1, "Reserved" },		    // officially listed as reserved
  { 0xA3, "Reserved" },		    // officially listed as reserved
  { 0xA4, "Reserved" },		    // officially listed as reserved
  { 0xA5, "FreeBSD, BSD/386" },	    // FreeBSD, BSD/386
  { 0xA6, "OpenBSD" },		    // OpenBSD
  { 0xA7, "NeXT" },		    // NeXTSTEP
  { 0xA8, "MAC OS X UFS" },	    // Apple Darwin
  { 0xA9, "NetBSD slice" }, 	    // NetBSD (http://www.netbsd.org/)
  { 0xAB, "MAC OS X Boot" },	    // Apple Darwin
  { 0xAF, "MAC OS X HFS" },	    // Apple
  { 0xB1, "QNX Neutrino" },	    // officially listed as reserved
  { 0xB2, "QNX Neutrino" },	    // QNX
  { 0xB3, "QNX Neutrino" },	    // officially listed as reserved
  { 0xB4, "Reserved" },		    // officially listed as reserved
  { 0xB6, "Reserved" },		    // officially listed as reserved
  { 0xB7, "BSDI filesys" },	    // BSDI file system (secondarily swap)
  { 0xB8, "BSDI swap" },	    // BSDI swap partition (secondarily file system)
  { 0xBE, "Solaris boot" },	    // Solaris boot partition
  { 0xBF, "Solaris" },		    // Solaris
  { 0xC0, "DR-DOS secure" },	    // DR-DOS/Novell DOS secured partition, or MS Valid NT Fault Tolerant partition
  { 0xC1, "DR-DOS secure 12" },	    // DR DOS 6.0 LOGIN.EXE-secured 12-bit FAT partition
  { 0xC4, "DR-DOS secure 16" },	    // DR DOS 6.0 LOGIN.EXE-secured 16-bit FAT partition
  { 0xC5, "DR-DOS secure Ex" },	    // DR DOS 6.0 LOGIN.EXE-secured 16-bit FAT partition
  { 0xC6, "DR-DOS secure Hg" },	    // DR DOS 6.0 LOGIN.EXE-secured Huge partition
  { 0xC7, "Syrinx Boot" },	    // Syrinx Boot
  { 0xCB, "DR-DOS secure 32" },	    // Reserved for DR-DOS secured FAT32
  { 0xCC, "DR-DOS secure32X" },	    // Reserved for DR-DOS secured FAT32 (LBA)
  { 0xCE, "DR-DOS secure16X" },	    // Reserved for DR-DOS secured FAT16 (LBA)
  { 0xD0, "MU-DOS secure" },	    // Multiuser DOS secured FAT12
  { 0xD1, "MU-DOS secure 12" },	    // Old Multiuser DOS secured FAT12
  { 0xD4, "MU-DOS secure 16" },	    // Old Multiuser DOS secured FAT16 (<= 32M)
  { 0xD5, "MU-DOS secure Ex" },	    // Old Multiuser DOS secured extended partition
  { 0xD6, "MU-DOS secure Hg" },	    // Old Multiuser DOS secured FAT16 (> 32M)
  { 0xD8, "CP/M-86" },		    // CP/M-86
  { 0xDB, "CP/M" },		    // CP/M, Concurrent CP/M, Concurrent DOS
  { 0xDB, "CTOS" },		    // CTOS (Convergent Technologies OS)
  { 0xDF, "TeraByte Bootit" },	    // TeraByte Unlimited Bootit
  { 0xE0, "ST AVFS" },		    // ST Microelectronics
  { 0xE1, "SpeedStor FAT12" },	    // SpeedStor 12-bit FAT extended partition
  { 0xE3, "Storage Dims" },	    // Storage Dimensions
  { 0xE4, "SpeedStor FAT16" },	    // SpeedStor 16-bit FAT extended partition
  { 0xE5, "Tandy MS-DOS" },	    // officially listed as reserved
  { 0xE6, "Reserved" },		    // officially listed as reserved
  { 0xE8, "Linux LUKS" },	    // Linux Unified Key Setup
  { 0xEB, "BeOS" },		    // BeOS BFS (BFS1)
  { 0xED, "GPT hybrid MBR" },	    // Robert Elliott, Hewlett Packard
  { 0xEE, "GPT Protection" },	    // EFI GPT Protection Partition
  { 0xEF, "EFI System Part." },	    // EFI System Partition
  { 0xF0, "PA-RISC Linux LDR" },    // must reside in first physical 2 GB
  { 0xF1, "Storage Dims" },	    // Storage Dimensions
  { 0xF2, "DR-DOS secondary" },	    // DOS 3.3+ secondary partition
  { 0xF3, "Reserved" },		    // officially listed as reserved
  { 0xF4, "SpeedStor" },	    // SpeedStor
  { 0xF4, "Storage Dims" },	    // Storage Dimensions
  { 0xF5, "Prologue" },		    // Prologue
  { 0xF6, "Reserved" },		    // officially listed as reserved
  { 0xFB, "VMware VMFS" },	    // VMware VMFS
  { 0xFC, "VMware VMKCORE" },	    // VMware VMKCORE
  { 0xFD, "Linux RAID" },	    // Linux RAID superblock with auto-detect
  { 0xFE, "LANstep" },		    // LANstep
  { 0xFE, "IBM PS/2 IML" },	    // IBM PS/2 IML (Initial Microcode Load) partition
  { 0xFF, "Xenix BBT" },	    // Xenix bad block table
};

#define KNOWN_TYPES (sizeof(type_name)/sizeof(TYPENAME))

int dump_part(MASTERBOOTSECTOR *pb, QWORD qwDiskSectors) {
  PARTITION *pp;
  int i, j;
  WORD bcyl, ecyl;
  WORD type;
  char *pszPartitionName;
  char *pszFormat;
  char *pszFormat2;
  char szSize[8];
  QWORD qwSize;
  QWORD qwLast = 1;

  printf("\nBoot sector ID marker %04X (%s).\n", pb->mbsSignature,
	  (pb->mbsSignature == 0xAA55) ? "Correct" : "Should be AA55");
  printf("\
Partitions             | Beginning  |    End     |       Sectors      |   Size\n");
  printf("\
Type              Boot | Cyl  Hd Se | Cyl  Hd Se |    First    Number |  Bytes\n");

  /* Free spaces format */
  pszFormat2 = "    {%-16s}   |            |            |{%9I64{%c}} {%9I64{%c}} |{%7s}\n";

  pp = &(pb->mbsPart[0]);
  for (i = 0; i < 4; i++, pp++) {
    if (iVerbose && ((QWORD)pp->first_sector > qwLast)) { /* Display free space between partitions */
      /* Assumes that the partitions are ordered sequentially */
      QWORD qwNSect = (QWORD)pp->first_sector - qwLast;
      qwSize = qwNSect << 9;
      FormatSize(qwSize, szSize, sizeof(szSize), iKB);
      oprintf(pszFormat2, "Free Space", cBase, qwLast, cBase, qwNSect, szSize);
    }

    bcyl = pp->beg_lcyl + ((WORD)(pp->beg_hcyl) << 8);
    ecyl = pp->end_lcyl + ((WORD)(pp->end_hcyl) << 8);
    type = pp->type;
    pszPartitionName = "Other";
    for (j=0; j < KNOWN_TYPES; j++) {
      if ((int)type == type_name[j].iType) {
	pszPartitionName = type_name[j].pszName;
	break;
      }
    }

    qwSize = pp->n_sectors;
    if ((pp->n_sectors == 0xFFFFFFFF) && (qwDiskSectors != (QWORD)0)) {
      qwSize = qwDiskSectors - pp->first_sector;
    }
    qwSize <<= 9; /* multiply by 512 */
    FormatSize(qwSize, szSize, sizeof(szSize), iKB);
    /* Partition entries format */
    if (cBase == 'u') {
      pszFormat = "%3u %-16s %c |%4u %3u%3u |%4u %3u%3u |%9lu %9lu |%7s\n";
      if (pp->n_sectors == 0xFFFFFFFF) // Force displaying -1 here v
      pszFormat = "%3u %-16s %c |%4u %3u%3u |%4u %3u%3u |%9lu %9ld |%7s\n";
    } else { // Assume iBase == 16
      pszFormat = " %02X %-16s %c |%4X %3X%3X |%4X %3X%3X |%9lX %9lX |%7s\n";
    }

    printf(pszFormat,
	    type, pszPartitionName, pp->boot ? 'Y' : 'N',
	    bcyl, pp->beg_head, pp->beg_sect,
	    ecyl, pp->end_head, pp->end_sect,
	    pp->first_sector, pp->n_sectors,
	    /* pp->n_sectors / 1953 */ szSize);

    if (type) { /* If this is an actual partition */
      if (pp->n_sectors != 0xFFFFFFFF) {
	qwLast = (QWORD)(pp->first_sector) + pp->n_sectors;
      } else {
	qwLast = qwDiskSectors; /* It goes to the end of the drive */
      }
    }
  }

  if (iVerbose && (qwDiskSectors > qwLast)) { /* Display free space in the end */
    /* Assumes that the partitions are ordered sequentially */
    QWORD qwNSect = qwDiskSectors - qwLast;
    qwSize = qwNSect << 9;
    FormatSize(qwSize, szSize, sizeof(szSize), iKB);
    oprintf(pszFormat2, "Free Space", cBase, qwLast, cBase, qwNSect, szSize);
  }

  return 0;
}

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
