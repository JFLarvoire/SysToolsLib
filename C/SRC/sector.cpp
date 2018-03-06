/*****************************************************************************\
*                                                                             *
*   File name:	    sector.cpp						      *
*                                                                             *
*   Description:    Copy disk sectors					      *
*                                                                             *
*   Notes:          The MS-DOS version must be compiled with the options:     *
*                   /G2 /link /stack:32768                                    *
*                                                                             *
*                   TO DO:                                                    *
*                   - Change the BlockXxxx API to allow 32-bits sizes.        *
*                   - Change the File functions to support UTF-8 names,       *
*                     then change this program to be UTF-8 compatible.        *
*                                                                             *
*                   When writing to a physical disk, Windows quickly detects  *
*		    new partition tables, mounts the corresponding volumes,   *
*		    which locks the corresponding sectors. Then writing fails.*
*                   To avoid that, clear partition tables in the MBR; then    *
*                   write everything starting at sector 1; Then write the MBR.*
*                   TO DO:                                                    *
*                   Automate this, or write backwards from the end.           *
*                   Alternatives: Try using volume locking IOCTLs?            *
*                   File System Management:                                   *
*    https://msdn.microsoft.com/en-us/library/windows/desktop/aa364407.aspx   *
*                   Disk Management Control Codes:                            *
*    https://msdn.microsoft.com/en-us/library/windows/desktop/aa363979.aspx   *
*                   Volume Management Control Codes:                          *
*    https://msdn.microsoft.com/en-us/library/windows/desktop/aa365729.aspx   *
*                                                                             *
*   History:								      *
*    1994-09-09 JFL Created this program.				      *
*    1994-12-15 JFL Dump the partition table if this is the boot sector.      *
*    1996-06-25 JFL Report BIOS errors, if any. 			      *
*    1999-02-17 JFL Added recognition of FAT32 partition, hidden ones, etc.   *
*    2000-02-17 JFL Added option to dump a MBS from a file.		      *
*                   Decode partition types beyond 0x20. Display size in MB.   *
*                   Version 1.3.                                              *
*    2000-06-06 JFL Adapted from bootsect.c. Version 2.0                      *
*    2000-07-25 JFL Added -d, -g, -sdp, -spt options.                         *
*                   Fixed bug with multi-track transfers. Version 2.1.        *
*    2001-03-08 JFL Restructured to use common block I/O routines.            *
*    2001-03-21 JFL Use oprintf routine for object formatting.                *
*    2001-09-14 JFL Works finally for DOS/Win9X/WinNT. Version 3.0.           *
*    2001-12-21 JFL Updated to use the new MultiOS.lib. Version 3.02.         *
*    2002-02-07 JFL Added ability to read/write logical volumes. (DOS & NT)   *
*		    Version 3.1.					      *
*    2002-04-05 JFL Switch -spt did not manage input base. Version 3.11.      *
*    2002-04-08 JFL Scale the initial location based on sector size if specif.*
*                   Do not destroy target files if they exist. Version 3.12.  *
*    2002-04-15 JFL Open the target block device in read-only mode if the -ro *
*                   option is set. This is necessary because the iReadOnly    *
*		    flag works only in _DEBUG mode. Version 3.13.	      *
*    2002-10-29 JFL Added message giving maximum possible size to copy.	      *
*    2002-11-05 JFL Pressing ESC allows to cancel long operations.	      *
*		    Added option -z. Version 3.14.                            *
*    2003-07-22 JFL Added options -sb, -sw, -sdw, -sqw.	Version 3.20.         *
*    2004-03-02 JFL Added option -fbs. Version 3.30.                          *
*    2008-03-20 JFL Use the selected base for -ld output.		      *
*                   Changed option -d to mean debug mode.                     *
*		    Automatically use bigger buffers to improve speed for     *
*		    large transfers. Version 3.31.                            *
*    2008-04-21 JFL Added support for files > 4GB. Version 3.32.	      *
*    2008-04-22 JFL Moved all file support to new MultiOS library modules.    *
*		    Version 3.4.					      *
*    2010-10-08 JFL Added tweaks for WIN64 support. 		              *
*		    Added several partition types, including EFI system part. *
*		    Version 3.41.					      *
*    2012-10-04 JFL Minor output tweaks to better read huge GB disk sizes.    *
*		    Use JFL debugging framework.                              *
*		    Version 3.42.					      *
*    2015-08-24 JFL Use the standard - argument to output to stdout.          *
*		    Added a percentage progress report for large copies.      *
*		    Added option -q to be fully silent.                       *
*		    Added a Ctrl-C handler to stop gracefully like with ESC.  *
*		    Version 3.5.					      *
*    2016-04-13 JFL Include WIN95 support only if the 98DDK is available.     *
*		    Allow specifying hard disks as hd0, hd1, etc.             *
*		    Version 3.6.					      *
*    2016-07-07 JFL Moved iReadOnly to the SYSLIB.			      *
*		    Version 3.6.1.					      *
*    2016-07-08 JFL Display human-readable size, with variable units.	      *
*		    Version 3.6.2.					      *
*    2016-07-12 JFL We have no dependancy on msdos.h anymore.		      *
*		    Version 3.6.3.					      *
*    2016-10-12 JFL For WIN95 builds, do not fail if the 98DDK is missing.    *
*		    Version 3.6.4.					      *
*    2016-12-05 JFL Reformated the source to modern coding standards.         *
*		    Improved the partition size formating readability.	      *
*		    Use the same units for disk and partition sizes.	      *
*		    Added options -H and -I to control the disk size SI unit. *
*		    Version 3.7.					      *
*    2017-02-11 JFL Added a dirty workaround for Windows' auto-mount feature. *
*		    Version 3.8.					      *
*    2017-04-15 JFL When listing drives, tolerate missing indexes, as one     *
*		    drive may have been recently unplugged. Version 3.8.1.    *
*    2017-07-07 JFL Compile-out Win95 extensions until we fix ringo.c.	      *
*    2017-07-08 JFL Do not display a period if there's no digit afterwards.   *
*		    Version 3.8.2.					      *
*    2017-07-15 JFL Removed BLOCK_TYPE_* constants. Use BT_* enums instead.   *
*    2017-07-28 JFL Added support for floppy disk drives.		      *
*		    Display MsvcLibX & SysLib library versions in DOS & Win.  *
*		    Use the 2017-02-11 workaround only when writing to a disk.*
*    2017-08-03 JFL Cosmetic change: Align byte sizes with other XB sizes.    *
*		    Version 4.0.					      *
*		                                                              *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "4.0"
#define PROGRAM_DATE    "2017-08-03"

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#include <malloc.h>
#include <conio.h>
#include <signal.h>

#define streq(s1, s2) (!strcmp(s1, s2))
#define ivprintf if (iVerbose) printf
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define FALSE 0
#define TRUE 1

typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned long int DWORD;
typedef int BOOL;

// The following comes from the SysLib library
#include "qword.h"
#include "harddisk.h"
#include "floppydisk.h"
#include "block.h"
#include "oprintf.h"
#include "IsMBR.h"

#if defined(_WIN95) // If building for WIN95
#if   HAS_98DDK && FIXED_RING0_C
#include "ring0.h"
#include "r0ios.h"
#else
#if FIXED_RING0_C
#pragma message("Note: Additional features are available in sector.exe when the Windows 98 DDK is configured.")
#endif
#endif // HAS_98DDK
#endif // defined(_WIN95)

#define ISECT0 1

#ifdef _MSDOS
#define OS_NAME "MS-DOS"
#endif
#if defined(_WIN32) && !defined(_WIN64)
#define OS_NAME "WIN32"
#endif
#if defined(_WIN64)
#define OS_NAME "WIN64"
#endif

typedef struct _PATCH
    {
    WORD wLength;	    // 1=byte; 2=word; 4=dword; 8=qword
    WORD wOffset;	    // Where to patch
    QWORD qwValue;	    // Value to patch
    struct _PATCH *pNext;   // Next patch
    } PATCH;

#define NEW(t) (t *)malloc(sizeof(t))

/* ---------------------------- Global variables --------------------------- */

extern "C" {
/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */
int iQuiet = FALSE;	// If TRUE, avoid displaying anything
int iVerbose = FALSE;	// If TRUE, display detailed progress information
int iProgress = FALSE;	// If TRUE, display a progress report
int iBase = 16;		// Base to use for formatted input/output (10 or 16)
char cBase = 'X';	// Format character to use for formatted output (d, u, X)
int iKB = 1000;		// Base for hard disk sizes KB, MB, etc. 1000 or 1024
volatile int interrupted = FALSE;	// If TRUE, abort the ongoing copy
void (*previousHandler)(int);		// Previous Control-C handler
void CtrlCHandler(int signal) {		// New Control-C handler
    if (signal == SIGINT) interrupted = TRUE; // Tell the main thread to abort
}
}

// QWORD qwMax = _QWORD(-1L, -1L);	// Initialization prevents building a .COM
QWORD qwMax;
#define QWMAX qwMax

PATCH *pFirstPatch = NULL;		// Head of patch linked list.
PATCH *pLastPatch = NULL;		// Tail of patch linked list.

/* -------------------------- FUNCTION prototypes -------------------------- */

char *version(int iVerbose);
void usage(void);
int IsSwitch(char *pszArg);
void DumpBuf(void FAR *fpBuf, WORD wStart, WORD wStop);
int dump_part(MASTERBOOTSECTOR *pMbs);
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
|   Notes								      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[]) {
  qwMax = _QWORD(0xFFFFFFFF, 0xFFFFFFFF); // Static init prevents building a .COM
				// Must be done before any reference to QWMAX.
  int i;
  QWORD qw;
  int iErr = 0;
  int iDump = FALSE;		// If TRUE, dump the sector
  int iPart = FALSE;		// If TRUE, dump a partition table
  int iPipe = FALSE;
  int iSPT = FALSE;		// If TRUE, update partition table
  long lPTParms[11];		// PartN, Type, Cyl0, Hd0, Sec0, Cyl1, Hd1, Sec1
  int iGeometry = FALSE;	// If TRUE, display the drive geometry
  int iBPB = FALSE;		// If TRUE, dump the Bios Parameter Block
  int iSH = FALSE;		// If TRUE, set Bios Parameter Block "Hidden" field
  HANDLE hFrom = NULL;
  HANDLE hTo = NULL;
  char *pszFrom = NULL;
  char *pszTo = NULL;
  int iAll = FALSE;
  int iSSize = -1;		// Sector size
  int iBSize = -1;		// Buffer size
  int nFrom = 0;		// Equivalent number of source blocks
  int nTo = 0;			// Equivalent number of destination blocks
  QWORD qwNSect = QWMAX;	// Number of blocks to transfer
  QWORD qwFromSect = QWMAX;	// Index of 1st block on source
  QWORD qwToSect = QWMAX;	// Index of 1st block on destination
  QWORD qwSect0;		// Initial value of qwFromSect
  QWORD qwNBytes;		// Number of bytes to transfer
  char *pBuf = NULL;
  char *pszHDName = NULL;	// Hard disk name
  HANDLE hDisk = NULL;		// Hard disk handle
#ifndef _MSDOS
#define ZINIT = {0}		// Clear the structure
#else
#define ZINIT      		// This triggers a compilation error in MSVC 1.5.
#endif
  HDGEOMETRY sHdGeometry ZINIT;	// Hard disk geometry
  int nHeads = 0;		// Hard disk geometry: Number of heads
  int nSectPerTrack = 0;	// Hard disk geometry: Number of sectors/track
  int iListDrives = FALSE;	// If TRUE, display a list of available drives.
  int iAppendZeros = FALSE;	// If TRUE, append zeros as needed beyond the end of the source data copied.
  int iFindBS = FALSE;	        // If TRUE, scan the disk looking for boot sectors.
  DWORD dwKB, dwKB0;		// Used for computing the progress report
  DWORD dwMB, dwMB0;		// Used for computing the progress report
  FDGEOMETRY sFdGeometry ZINIT;	// Hard disk geometry

  /* Get the command line arguments */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (streq(arg, "-")) {	// - is the standard place-holder for outputing data to stdout
      iPipe = TRUE;
      continue;
    }
    if (IsSwitch(argv[i])) {	/* It's a switch */
      char *opt = arg+1;
      if (   streq(opt, "help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "a")) {
	iAll = TRUE;
	continue;
      }
      if (streq(opt, "bpb")) {
	iBPB = TRUE;
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
      if (streq(opt, "fbs")) {
	iFindBS = TRUE;
	iAll = TRUE;
	continue;
      }
      if (streq(opt, "g")) {
	iGeometry = TRUE;
	continue;
      }
      if (streq(opt, "H")) {
	iKB = 1000;
	continue;
      }
      if (streq(opt, "I")) {
	iKB = 1024;
	continue;
      }
      if (streq(opt, "l") || streq(opt, "ld")) {
	iListDrives = TRUE;
	continue;
      }
      if (streq(opt, "p")) {
	iPart = TRUE;
	continue;
      }
      if (streq(opt, "q")) {
	iQuiet = TRUE;
	iVerbose = FALSE;
	continue;
      }
      if (streq(opt, "ro")) {
	iReadOnly = TRUE;
	continue;
      }
      if (streq(opt, "s")) {
	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    iSSize = (int)strtol(argv[++i], NULL, iBase);
	continue;
      }
      if (streq(opt, "sb")) {
	PATCH *pPatch = NEW(PATCH);

	pPatch->wLength = 1;
	if (!pFirstPatch) pFirstPatch = pPatch;
	if (pLastPatch) pLastPatch->pNext = pPatch;

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->wOffset = (WORD)strtoul(argv[++i], NULL, iBase);

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->qwValue = (DWORD)(BYTE)strtoul(argv[++i], NULL, iBase);

	pPatch->pNext = NULL;
	pLastPatch = pPatch;
	continue;
      }
      if (streq(opt, "sdw")) {
	PATCH *pPatch = NEW(PATCH);

	pPatch->wLength = 4;
	if (!pFirstPatch) pFirstPatch = pPatch;
	if (pLastPatch) pLastPatch->pNext = pPatch;

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->wOffset = (WORD)strtoul(argv[++i], NULL, iBase);

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->qwValue = (DWORD)strtoul(argv[++i], NULL, iBase);

	pPatch->pNext = NULL;
	pLastPatch = pPatch;
	continue;
      }
      if (streq(opt, "sh")) {
	iSH = TRUE;
	continue;
      }
      if (streq(opt, "spt")) {
	int j;

	iSPT = TRUE;
	for (j=0; j<(sizeof(lPTParms)/sizeof(long)); j++) {
	  if (   ((i+1) < argc)
	      && (!IsSwitch(argv[i+1]))) {
	    lPTParms[j] = strtol(argv[++i], NULL, iBase);
	  } else {
	    lPTParms[j] = 0;
	  }
	}
	continue;
      }
      if (streq(opt, "sqw")) {
	PATCH *pPatch = NEW(PATCH);

	pPatch->wLength = 8;
	if (!pFirstPatch) pFirstPatch = pPatch;
	if (pLastPatch) pLastPatch->pNext = pPatch;

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->wOffset = (WORD)strtoul(argv[++i], NULL, iBase);

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    strtoqw(argv[++i], pPatch->qwValue, iBase);

	pPatch->pNext = NULL;
	pLastPatch = pPatch;
	continue;
      }
      if (streq(opt, "sw")) {
	PATCH *pPatch = NEW(PATCH);

	pPatch->wLength = 2;
	if (!pFirstPatch) pFirstPatch = pPatch;
	if (pLastPatch) pLastPatch->pNext = pPatch;

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->wOffset = (WORD)strtoul(argv[++i], NULL, iBase);

	if (   ((i+1) < argc)
	    && (!IsSwitch(argv[i+1])))
	    pPatch->qwValue = (WORD)strtoul(argv[++i], NULL, iBase);

	pPatch->pNext = NULL;
	pLastPatch = pPatch;
	continue;
      }
      if (streq(opt, "t")) {
	iBase = 10;
	cBase = 'u';
	continue;
      }
      if (streq(opt, "tf")) {	/* Test function FormatSize() */
	char szBuf[20];
	iBase = 10;
	cBase = 'u';
	strtoqw(argv[i+i], qw, iBase);
	FormatSize(qw, szBuf, sizeof(szBuf), atoi(argv[i+2]));
	printf("That is '%s'\n", szBuf);
	exit(0);
      }
      if (streq(opt, "v")) {
	iQuiet = FALSE;
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	printf("%s\n", version(TRUE));
	exit(0);
      }
      if (streq(opt, "x")) {
	iBase = 16;
	cBase = 'X';
	continue;
      }
      if (streq(opt, "z")) {
	iAppendZeros = TRUE;
	continue;
      }
      printf("Unrecognized switch %s. Ignored.\n", argv[i]);
      continue;
    }
    if (!pszFrom) {
      pszFrom = argv[i];
      continue;
    }
    if (streq(argv[i], ":")) {	// This was the old place-holder for outputing data to stdout
      iPipe = TRUE;
      continue;
    }
    if ((!pszTo) && (!iPipe)) {	// This must be after the comparison with ":" or "-"
      pszTo = argv[i];
      continue;
    }
    if (qwNSect == QWMAX) {
      strtoqw(argv[i], qwNSect, iBase);
      continue;
    }
    if (qwFromSect == QWMAX) {
      strtoqw(argv[i], qwFromSect, iBase);
      continue;
    }
    if (qwToSect == QWMAX) {
      strtoqw(argv[i], qwToSect, iBase);
      continue;
    }
    printf("Unexpected argument: %s\nIgnored.\n", argv[i]);
    break;	/* Ignore other arguments */
  }

  if (iListDrives) {
    int nMissing = 0; /* Number of missing disk indexes so far */
    for (i=0; nMissing < 32; i++) {	/* Tolerate up to 32 missing entries before giving up hope */
      char szSize[8];
      hDisk = HardDiskOpen(i, READONLY);
      if (!hDisk) {	/* If there is no disk with that index */
      	nMissing += 1;		/* Then count the missing drive */
      	continue;		/* And keep searching, as one drive may have been recently unplugged */
      }
      // Get the drive characteristics
      iErr = HardDiskGetGeometry(hDisk, &sHdGeometry);
      QWORD qwSize = sHdGeometry.qwSectors * (DWORD)(sHdGeometry.wSectorSize);
      FormatSize(qwSize, szSize, sizeof(szSize), iKB); /* Compute the size in MiB or GiB ... */
      oprintf("Hard Disk hd{%d}: #Sect={%I64{%c}} ({%s})", i, cBase, sHdGeometry.qwSectors, szSize);
      oprintf("  Phys({%l{%c}}/{%l{%c}}/{%l{%c}})",
	     cBase, (long)sHdGeometry.dwCyls, cBase, (long)sHdGeometry.dwHeads, cBase, (long)sHdGeometry.dwSects);
      oprintf(" / Xlat({%l{%c}}/{%l{%c}}/{%l{%c}})\n",
	     cBase, (long)sHdGeometry.dwXlatCyls, cBase, (long)sHdGeometry.dwXlatHeads, cBase, (long)sHdGeometry.dwXlatSects);
      HardDiskClose(hDisk);
      nMissing = 0; // Reset the missing drive counter
    }

#if defined(_WIN95) && HAS_98DDK && FIXED_RING0_C
    if (iVerbose && (GetVersion() > 0x80000000)) {	// If iVerbose on Win95/98/ME
      ISP_GET_FRST_NXT_DCB ispGfnDCB;

      ispGfnDCB.ISP_gfnd_hdr.ISP_func = ISP_GET_FIRST_NEXT_DCB;   // Standard header / Function number
      ispGfnDCB.ISP_gfnd_dcb_type = 0xFF;			    // device type or 0ffh / List all types

      for (ispGfnDCB.ISP_gfnd_dcb_offset = 0;
	   !R0IosService((PISP)&ispGfnDCB);
	   ispGfnDCB.ISP_gfnd_dcb_offset = ispGfnDCB.ISP_gfnd_found_dcb) {
	WORD *pw;
	DWORD dwCyl, dwHead, dwSect;
	QWORD qwSects;

	PIOSDCB pDCB = (PIOSDCB)(ispGfnDCB.ISP_gfnd_found_dcb);
	if (pDCB->DCB_cmn.DCB_physical_dcb != (DWORD)pDCB) continue; // This is not a physical device

	dwCyl  = pDCB->DCB_bdd.DCB_apparent_cyl_cnt;
	dwHead = pDCB->DCB_bdd.DCB_apparent_head_cnt;
	dwSect = pDCB->DCB_bdd.DCB_apparent_spt;
	qwSects = *(QWORD *)pDCB->DCB_bdd.DCB_apparent_sector_cnt;

	printf("DCB @ %p: ", pDCB);
	switch (pDCB->DCB_bus_type) {
	  case DCB_BUS_ESDI:
	    printf("ESDI  ");
	    // Now decode the IDE ID sector contents.
	    // To do: Add support for 48-bit sector addressing (ATA 4 or ATA 5 I think)
	    pw = (WORD *)(pDCB->DCB_cmn.DCB_pEid);
	    if (pw && (pw[53] & 1)) {
	      dwCyl  = pw[54];
	      dwHead = pw[55];
	      dwSect = pw[56];
	      qwSects = *(DWORD *)(pw+60);
	    }
	    break;
	  case DCB_BUS_SCSI:
	    printf("SCSI  ");
	    break;
	  case DCB_BUS_NEC:
	    printf("NEC   ");
	    break;
	  case DCB_BUS_SMART:
	    printf("SMART ");
	    break;
	  case DCB_BUS_ABIOS:
	    printf("ABIOS ");
	    break;
	  default:
	    printf("Bus?? ");
	    break;
	}
	switch (pDCB->DCB_cmn.DCB_device_type) {
	  case DCB_type_disk:		printf("hard disk "); break;	/* All Direct Access Devices -- non-removable     */
	  case DCB_type_tape:		printf("tape "); break;		/* Sequencial Access Devices        */
	  case DCB_type_printer:	printf("printer "); break;	/* Printer Device                   */
	  case DCB_type_processor:	printf("processor "); break;	/* Processor type device            */
	  case DCB_type_worm:		printf("worm "); break;		/* Write Once Read Many Device      */
	  case DCB_type_cdrom:	printf("cdrom "); break;	/* CD ROM Device                    */
	  case DCB_type_scanner:	printf("scanner "); break;	/* Scanner Device                   */
	  case DCB_type_optical_memory: printf("optical disk "); break; /* some Optical disk                */
	  case DCB_type_changer:	printf("juke box "); break;	/* Changer device e.g. juke box     */
	  case DCB_type_comm:		printf("comm device "); break;	/* Communication devices            */
	  case DCB_type_floppy:	printf("floppy "); break;	/* devices like floppy */
	  default: printf("device type %d ", pDCB->DCB_cmn.DCB_device_type); break;
	}
	printf("#%X ", pDCB->DCB_cmn.DCB_unit_number);
	switch (pDCB->DCB_cmn.DCB_device_type) {
	  case DCB_type_disk:			/* All Direct Access Devices -- non-removable     */
	  case DCB_type_floppy:		/* devices like floppy */
	      printf(": %ld MB  (%ld/%ld/%ld)",
		     (long)((Qword2Double(qwSects) * pDCB->DCB_bdd.DCB_apparent_blk_size) / 1000000),
		     dwCyl, dwHead, dwSect);
	      break;
	  default:
	      break;
	}
	printf("\n");
      }
    }
#endif // defined(_WIN95) && HAS_98DDK

    /* Repeat scan for floppys */
    nMissing = 0; /* Number of missing disk indexes so far */
    for (i=0; nMissing < 4; i++) {	/* Tolerate up to 4 missing entries before giving up hope */
      char szSize[8];
      hDisk = FloppyDiskOpen(i, READONLY);
      if (!hDisk) {	/* If there is no disk with that index */
      	nMissing += 1;		/* Then count the missing drive */
      	continue;		/* And keep searching, as one drive may have been recently unplugged */
      }
      // Get the drive characteristics
      iErr = FloppyDiskGetGeometry(hDisk, &sFdGeometry);
      if (!iErr) { // A floppy is present in the drive
	QWORD qwSize = sFdGeometry.dwSectors * sFdGeometry.wSectorSize;
	FormatSize(qwSize, szSize, sizeof(szSize), 1440); /* Compute the size in MiB or GiB, using the floppy-specific algorithm */
	oprintf("Floppy Disk fd{%d}: #Sect={%l{%c}} ({%s})", i, cBase, sFdGeometry.dwSectors, szSize);
	oprintf("  Phys({%l{%c}}/{%l{%c}}/{%l{%c}})\n",
	       cBase, (long)sFdGeometry.wCyls, cBase, (long)sFdGeometry.wHeads, cBase, (long)sFdGeometry.wSects);
      } else { // No floppy in the drive
	oprintf("Floppy Disk fd{%d}: No floppy in the drive\n", i);
      }
      FloppyDiskClose(hDisk);
      nMissing = 0; // Reset the missing drive counter
    }

    exit(0);
  }

  /* Open the files */
  if (!pszFrom) usage();
  if (pszFrom) {
    hFrom = BlockOpen(pszFrom, "rb");
    if (!hFrom) {
      printf("Error: Can't open %s\n", pszFrom);
      exit(1);
    }
    if ((iSSize != -1) && (qwFromSect != QWMAX)) {
      qwFromSect *= iSSize / BlockSize(hFrom);
    }
  }
  if (pszTo) {
    hTo = BlockOpen(pszTo, iReadOnly ? "rb" : "r+b");
    if (!hTo) {
      printf("Error: Can't open %s\n", pszTo);
      BlockClose(hFrom);
      exit(1);
    }
    if ((iSSize != -1) && (qwToSect != QWMAX)) {
      qwToSect *= iSSize / BlockSize(hTo);
    }
  } else {
    iDump = TRUE;	// Default: Dump if no target is specified
  }

  DEBUG_CODE_IF_ON(
    oprintf("Source block size: {%{%c}} bytes\n", cBase, BlockSize(hFrom));
    if (hTo) oprintf("Destination block size: {%{%c}} bytes\n", cBase, BlockSize(hTo));
    qw = BlockCount(hFrom);
    qw *= (DWORD)BlockSize(hFrom);
    oprintf("Source size: {%I64{%c}} bytes\n", cBase, qw);
    if (hTo) {
      qw = BlockCount(hTo);
      qw *= (DWORD)BlockSize(hTo);
      oprintf("Destination size: {%I64{%c}} bytes\n", cBase, qw);
    }
  )
  /* Get the hard disk geometry, if any */

  if (BlockType(hFrom) == BT_HARDDISK) {
    pszHDName = pszFrom;
    hDisk = BlockPtr(hFrom)->h;
  } else if (hTo && (BlockType(hTo) == BT_HARDDISK)) {
    pszHDName = pszTo;
    hDisk = BlockPtr(hTo)->h;
  }

  if (hDisk) {
    iErr = HardDiskGetGeometry(hDisk, &sHdGeometry);
    if (iErr) {
geometry_failure:
      printf("Cannot get disk geometry.\n");
      BlockClose(hFrom);
      BlockClose(hTo);
      exit(1);
    }
    nHeads = (int)sHdGeometry.dwXlatHeads;
    nSectPerTrack = (int)sHdGeometry.dwXlatSects;
  }

  if (iGeometry) {
    char *pszUnit = "MB";
    long lSize;
    if (!hDisk) goto geometry_failure;

    printf("Drive %s parameters:\n", pszHDName);
    // if (hdp.wInfo & 2) // ~~jfl 2000-06-07 Test fails in many valid cases
    {
      oprintf("{%l{%c}} Cylinders, ", cBase, sHdGeometry.dwXlatCyls);
      oprintf("{%l{%c}} Heads, ", cBase, sHdGeometry.dwXlatHeads);
      oprintf("{%l{%c}} Sectors/track, ", cBase, sHdGeometry.dwXlatSects);
      oprintf("{%{%c}} Bytes/Sector\n", cBase, sHdGeometry.wSectorSize);
    }
    oprintf("Total: {%I64{%c}} Sectors", cBase, sHdGeometry.qwSectors);
    lSize = (long)(sHdGeometry.qwSectors / (DWORD)(1000000 / sHdGeometry.wSectorSize));
    if (lSize > 10000) {
      lSize /= 1000;
      pszUnit = "GB";
    }
    printf(" = %ld %s\n", lSize, pszUnit);
  }

  /* Set defaults */

  if (iSSize == -1) {
    iSSize = BlockSize(hFrom);
    if (hTo) iSSize = max(iSSize, BlockSize(hTo));
  }
  if (iVerbose) oprintf("Block size used: {%{%c}}\n", cBase, iSSize);
  // Make sure the requested block size is a multiple of both devices block sizes.
  if (   (iSSize % BlockSize(hFrom))
      || ((hTo) && (iSSize % BlockSize(hTo)))
     ) {
    printf("Remainder from = %d; Remainder to = %d.\n", iSSize % BlockSize(hFrom), iSSize % BlockSize(hTo));
    printf("Requested block size is not a multiple of source or destination sector size.\n");
    BlockClose(hFrom);
    BlockClose(hTo);
    exit(1);
  }

  // Compute the number of bytes to transfer
  if (iAll) {				// If -all, Copy the whole source.
    qwNBytes = BlockCount(hFrom);
    qwNBytes *= (DWORD)BlockSize(hFrom);
  } else if (qwNSect == QWMAX) {	// Else if unspecified choose reasonable default
    switch (BlockType(hFrom)) {
      case BT_FILE:
	qwNSect = BlockCount(hFrom);
	break;
      case BT_HARDDISK:
      default:
	qwNSect = 1;
    }
    qwNBytes = qwNSect;
    qwNBytes *= (DWORD)BlockSize(hFrom);
  } else {			// Use the specified number of blocks.
    qwNBytes = qwNSect;
    qwNBytes *= iSSize;
  }
  QWORD qwMB = qwNBytes;
  qwMB /= (DWORD)0x100000;
  if (iVerbose) oprintf("There are {%I64{%c}} bytes to transfer ({%I64d}MB).\n", cBase, qwNBytes, qwMB);

  if (qwFromSect == QWMAX) {
    qwFromSect = 0;
  }

  if (qwToSect == QWMAX) {
    qwToSect = 0;
  }

  if (   iDump
      && (BlockType(hFrom) == BT_HARDDISK)
      && (qwFromSect == (DWORD)0)
     ) {
    iPart = TRUE;	// Dump partition table for master boot record
  }

  // Make sure the request is compatible with the source block size
  if (qwNBytes % (DWORD)BlockSize(hFrom)) {
    printf("Total size is not a multiple of source's %d block size.\n", BlockSize(hFrom));
    BlockClose(hFrom);
    BlockClose(hTo);
    exit(1);
  }

  // Make sure the request is compatible with the target block size
  if (   hTo
      && (qwNBytes % (DWORD)BlockSize(hTo))
     ) {
    printf("Total size is not a multiple of destination's %d block size.\n", BlockSize(hTo));
    BlockClose(hFrom);
    BlockClose(hTo);
    exit(1);
  }

  // Make sure the request is compatible with the source device size
  qw = qwNBytes;			// Total number of bytes to copy.
  qw /= BlockSize(hFrom);		// Number of source sectors to copy
  qw += qwFromSect;			// Index of limit sector
  if ((!iAppendZeros) && (qw > BlockCount(hFrom))) {
    printf("Source is too small for requested transfer.\n");
    // ~~jfl 2002-10-29 Display the maximum possible size.
    qw = BlockCount(hFrom);
    qw -= qwFromSect;		// Max number of device sectors to transfer
    qw *= BlockSize(hFrom);		// Max number of bytes to transfer
    qw /= (DWORD)iSSize;		// Max number of requested sectors to transfer
    oprintf("Maximum allowed: {%I64{%c}}\n", cBase, qw);
    BlockClose(hFrom);
    BlockClose(hTo);
    exit(1);
  }

  // Make sure the request is compatible with the target device size
  if (   hTo
      && (BlockType(hTo) != BT_FILE)	// Files can be extended to any length
     ) {
    qw = qwNBytes;			// Total number of bytes to copy.
    qw /= BlockSize(hTo);		// Number of source sectors to copy
    qw += qwToSect;			// Index of limit sector
    if (qw > BlockCount(hTo)) {
      printf("Destination is too small for requested transfer.\n");
      // ~~jfl 2002-11-05 Display the maximum possible size.
      qw = BlockCount(hTo);
      qw -= qwToSect;		// Max number of device sectors to transfer
      qw *= BlockSize(hTo);	// Max number of bytes to transfer
      qw /= (DWORD)iSSize;	// Max number of requested sectors to transfer
      oprintf("Maximum allowed: {%I64{%c}}\n", cBase, qw);
      BlockClose(hFrom);
      BlockClose(hTo);
      exit(1);
    }
  }

  // Allocate a buffer
  iBSize = iSSize;
  if (iBSize < 512) iBSize = 512; // Make sure it's at least 512
  pBuf = (char *)malloc(iBSize);
  if (!pBuf) {
    printf("Not enough memory for transfer buffer.\n");
    BlockClose(hFrom);
    BlockClose(hTo);
    exit(1);
  }
  // Try allocating a bigger buffer for better performance on large transfers.
  for (i=0; i<4; i++) { // Try 4KB, 32KB, 256KB, 2MB
    int iBSize1;
    char *pBuf2;

    iBSize1 = iBSize << 3; // iBSize * 8
    if ((unsigned)iBSize1 < (unsigned)iBSize) break; // Under DOS, this can wrap around!
    if (qwNBytes <= (DWORD)iBSize1) break; // Big enough already.

    pBuf2 = (char *)realloc(pBuf, iBSize1);
    if (!pBuf2) break; // Not enough memory for a bigger buffer

    // OK, use that bigger buffer.
    pBuf = pBuf2;
    iBSize = iBSize1;
  }
  if (iBSize > iSSize) {
    if (iVerbose) oprintf("Buffer size used: {%{%c}}\n", cBase, iBSize);
  }

  // Go for it!!!
  qwSect0 = qwFromSect;
  nFrom = iBSize / BlockSize(hFrom);
  if (hTo) nTo = iBSize / BlockSize(hTo);
  dwKB0 = (DWORD)(qwNBytes >> 10); // Only used for floppys. Floppy sizes < 50000KB. Won't overflow.
  if (!dwKB0) dwKB0 = 1; // Avoid dividing by 0 in progress report
  dwMB0 = (DWORD)(qwNBytes >> 20); // Will overflow for disk sizes > 4PB. Largest is 16TB in 2017.
  if (!dwMB0) dwMB0 = 1; // Avoid dividing by 0 in progress report
  if ((qwNBytes > (DWORD)iBSize) && !iQuiet) {
    previousHandler = signal(SIGINT, CtrlCHandler);
    printf("Press ESC to abort the copy.\n");
    iProgress = TRUE;
  }
  if (iProgress) {
    if (           (BlockType(hFrom) == BT_FLOPPYDISK)
        || (hTo && (BlockType(hTo) == BT_FLOPPYDISK))) {
      iProgress = 1; /* Floppys are small and very slow. Use KB to show progress */
    } else {
      iProgress = 2; /* All other block devices are fast. Use MB to show progress */
    }
  }
  /* Dirty workaround for Windows' auto-mount feature, that blocks access to the partitions while we write them */
#ifdef _WIN32
  int iWriteMbrLast = FALSE;
  int nPhases = 1;
  int iFromSectPerSect = iSSize / BlockSize(hFrom);
  int iToSectPerSect = 0;
  QWORD qwFromSect0 = qwFromSect;
  if (hTo) iToSectPerSect = iSSize / BlockSize(hTo);
  if (hTo && (BlockSize(hTo) > 1) && (qwMB >= 1) && (qwToSect == 0)) { /* If copying a whole disk image to a disk */
    iWriteMbrLast = TRUE;
    nPhases = 2;
  }
  for (int iPhase = 0; iPhase < nPhases; iPhase++) {
    if (iWriteMbrLast) {
      if (iPhase == 0) { /* Copy everything but the MBR */
      	qwNBytes -= iSSize;
      	qwFromSect += iFromSectPerSect;
      	qwToSect += iToSectPerSect;
      } else { /* Phase 1: Copy the MBR */
      	qwNBytes = iSSize;
      	qwFromSect = qwFromSect0;
      	qwToSect = 0;
      }
    }
#endif /* defined(_WIN32) */
  for ( ;
       qwNBytes != (DWORD)0;
       qwNBytes -= (DWORD)iBSize, qwFromSect += nFrom, qwToSect += nTo
      ) {
    // Special case for the last buffer
    if (qwNBytes < (DWORD)iBSize) {
      iBSize = (int)qwNBytes;
      nFrom = iBSize / BlockSize(hFrom);
      if (hTo) nTo = iBSize / BlockSize(hTo);
    }
    // ~~jfl 2002-11-05 Added possibility to cancel long operations.
    if (_kbhit() && (_getch() == '\x1B')) interrupted = TRUE;
    if (interrupted) {
      printf("\nOPERATION INTERRUPTED! Please be patient while write buffers get flushed.\n");
      signal(SIGINT, SIG_DFL); // Restore the default handler, to allow forcing an immediate abort if things stall for too long. 
      iErr = 3;
      break;
    }

    if (iVerbose || iProgress) putchar('\r');
    if (iVerbose) oprintf("Copying {%s} {%I64{%c}} ", BlockIndexName(hFrom), cBase, qwFromSect);
    if (iVerbose && iProgress) putchar('(');
    if (iProgress == 1) {
      dwKB = dwKB0 - (DWORD)(qwNBytes >> 10); // Only used for floppys. Floppy sizes < 50000KB. Won't overflow.
      printf("%ldKB / %d%%", dwKB, ((100 * dwKB) / dwKB0));
    } else if (iProgress == 2) {
      dwMB = dwMB0 - (DWORD)(qwNBytes >> 20); // Will overflow for disk sizes > 4PB. Largest is 16TB in 2017.
      printf("%ldMB / %d%%", dwMB, ((100 * dwMB) / dwMB0));
    }
    if (iVerbose && iProgress) putchar(')');
    if (iProgress) putchar(' ');
    // iErr = BlockRead(hFrom, qwFromSect, nFrom, pBuf);
    {
      // Problem: The Block API is limited to 0xFFFF blocks at a time.
      // This is fine for disks, but not for files ==> Split in small I/Os
      unsigned int uDone = 0;     // Number of blocks already read
      unsigned int uLeft = nFrom; // Number of blocks left to read
      WORD wFrom = 0x8000;        // Read at most 32K at a time
      iErr = 0;
      for ( ; uLeft; uLeft-=wFrom, uDone+=wFrom) {
	if (uLeft < wFrom) wFrom = (WORD)uLeft; // Last batch
	if ((iErr = BlockRead(hFrom, qwFromSect+uDone, wFrom, pBuf+(uDone*BlockSize(hFrom)))) != 0) break;
      }
    }

    if (iFindBS) {
      oprintf("\r{%s} {%I64{%c}}    ", BlockIndexName(hFrom), cBase, qwFromSect);
      if (IsBS(pBuf, iSSize) || IsMBR(pBuf)) {
	printf("\n");
	// Dump its contents
	DumpBuf(pBuf, 0, (WORD)iSSize);
	// For the boot sector, decode the partition table
	if (IsMBR(pBuf)) dump_part((MASTERBOOTSECTOR *)pBuf);
	// Stop to let the user review the dump.
	printf("\nPress any key to seach further, or ESC to stop.\n");
	while (_kbhit()) _getch(); // Flush the input buffer.
	if (_getch() == '\x1B') break;
      }
      continue;
    }

    if (iErr) {
      if (!iAppendZeros) oprintf("\nError 0x{%02X} reading {%s} {%I64{%c}}.\n", iErr, BlockIndexName(hFrom), cBase, qwFromSect);
      for (i=0; i<iBSize; i++) pBuf[i] = '\0';
      if (!iAppendZeros) continue;
    }

    if (iSH && (qwFromSect == qwSect0))	{	// Set Bios Parameter Block Hidden sectors (1st sector only)
      BOOTSECTOR *pbs = (BOOTSECTOR *)pBuf;

      pbs->bsMedia = 0xF8;			// Hard disk
      pbs->bsSecPerTrack = (WORD)nSectPerTrack;
      pbs->bsHeads = (WORD)nHeads;
      pbs->bsHiddenSecs = (DWORD)qwToSect;
    }

    if (iSPT && (qwFromSect == qwSect0)) {	// Set partition table entry (1st sector only)
      PARTITION *pp;

      /* Fill in the missing data, if possible */
      if ((!lPTParms[9]) && lPTParms[5]) {	// If 32-bits sector base missing, but CHS base present
	// Compute 32-bits base from CHS.
	lPTParms[9] = (((lPTParms[3] * (long)nHeads) + lPTParms[4]) * nSectPerTrack) + lPTParms[5] - ISECT0;
      }

      if ((!lPTParms[5]) && lPTParms[9]) {	// If CHS base missing, but 32-bits sector base present
	  					// Compute CHS from 32-bits sector base.
	DWORD dw = lPTParms[9];
	lPTParms[5] = (int)(dw % nSectPerTrack) + ISECT0;
	dw /= nSectPerTrack;
	lPTParms[4] = (int)(dw % nHeads);
	dw /= nHeads;
	lPTParms[3] = (int)dw;
	if (dw > 1023) {	// Must fit on 10 bits!
	  // If not, saturate.
	  lPTParms[3] = 1023;
	  lPTParms[4] = nHeads - 1;
	  lPTParms[5] = nSectPerTrack + ISECT0 - 1;
	}
      }

      if ((!lPTParms[10]) && lPTParms[8]) {	// If 32-bits sector count missing, but CHS end present
	// Compute 32-bits count from CHS.
	lPTParms[10] = ((((lPTParms[6] * (long)nHeads) + lPTParms[7]) * nSectPerTrack) + lPTParms[8] - ISECT0) + 1 - lPTParms[9];
      }

      if ((!lPTParms[8]) && lPTParms[10]) {	// If CHS end missing, but 32-bits sector count present
	// Compute CHS from 32-bits count
	DWORD dw = lPTParms[9] + lPTParms[10] - 1;	// Number of last sector
	lPTParms[8] = (int)(dw % nSectPerTrack) + ISECT0;
	dw /= nSectPerTrack;
	lPTParms[7] = (int)(dw % nHeads);
	dw /= nHeads;
	lPTParms[6] = (int)dw;
	if (dw > 1023) {	// Must fit on 10 bits!
	  // If not, saturate.
	  lPTParms[6] = 1023;
	  lPTParms[7] = nHeads - 1;
	  lPTParms[8] = nSectPerTrack + ISECT0 - 1;
	}
      }

      pp = &(((MASTERBOOTSECTOR *)pBuf)->mbsPart[lPTParms[0] & 3]);	// Select partition 0 to 3

      pp->type = (BYTE)(lPTParms[1]);
      pp->boot = (BYTE)(lPTParms[2]);

      pp->beg_lcyl = (BYTE)(lPTParms[3]);
      pp->beg_hcyl = (WORD)(lPTParms[3] >> 8);
      pp->beg_head = (BYTE)(lPTParms[4]);
      pp->beg_sect = (BYTE)(lPTParms[5]);

      pp->end_lcyl = (BYTE)(lPTParms[6]);
      pp->end_hcyl = (WORD)(lPTParms[6] >> 8);
      pp->end_head = (BYTE)(lPTParms[7]);
      pp->end_sect = (BYTE)(lPTParms[8]);

      pp->first_sector = lPTParms[9];
      pp->n_sectors = lPTParms[10];

// 	    if (hTo == hFrom) fseek(hf, -SECTORSIZE, SEEK_CUR);	// Move back 1 sector
    }

    if (pFirstPatch) {
      PATCH *pPatch;
      for (pPatch = pFirstPatch; pPatch; pPatch = pPatch->pNext) {
	DEBUG_PRINTF(("\nPatching %d bytes at offset %u.\n", pPatch->wLength, pPatch->wOffset));
	switch (pPatch->wLength) {
	  case 1: *(BYTE *)(pBuf+pPatch->wOffset) = (BYTE)(DWORD)pPatch->qwValue; break;
	  case 2: *(WORD *)(pBuf+pPatch->wOffset) = (WORD)pPatch->qwValue; break;
	  case 4: *(DWORD *)(pBuf+pPatch->wOffset) = (DWORD)pPatch->qwValue; break;
	  case 8: *(QWORD *)(pBuf+pPatch->wOffset) = (QWORD)pPatch->qwValue; break;
	  default: break;
	}
      }
      pFirstPatch = NULL; // Don't do it twice.
    }

    if (iDump) {
      int j;
      for (i=j=0; i<iBSize; i+=iSSize, j++) {
	oprintf("\n{%s} {%I64{%c}}", BlockIndexName(hFrom), cBase, qwFromSect+j);
//      if (iSect >= 0)
//        printf(" (Cyl %d, Hd %d, Sct %d)", iCyl, iHead, iSect + 1 - ISECT0);
	printf("\n");
	// Dump its contents
	DumpBuf(pBuf+i, 0, (WORD)iSSize);
	// For the boot sector, decode the partition table
	if (iPart) {
	  dump_part((MASTERBOOTSECTOR *)pBuf+i);
	  iPart = FALSE;	// Don't dump it for subsequent sectors
	}
	printf("\n");
      }
    }

    if (iBPB && (qwFromSect == qwSect0)) {	// Get Bios Parameter Block (1st sector only)
      BOOTSECTOR *pbs = (BOOTSECTOR *)pBuf;

      printf("BIOS Parameter Block:\n");
      printf("Jump = %02.2X %02.2X %02.2X\n", pbs->bsJump[0], pbs->bsJump[1], pbs->bsJump[2]);
      printf("OemName = \"%.8s\"\n", pbs->bsOemName);
      printf("Media descriptor = %02.2X\n", pbs->bsMedia);
      oprintf("Bytes/Sector = {%{%c}}\n", cBase, pbs->bsBytesPerSec);
      oprintf("Sectors/Track = {%{%c}}\n", cBase, pbs->bsSecPerTrack);
      oprintf("Heads = {%{%c}}\n", cBase, pbs->bsHeads);
      oprintf("Sectors total = {%l{%c}}\n", cBase, pbs->bsSectors ? (long)(pbs->bsSectors) : pbs->bsHugeSectors);
      oprintf("Hidden sectors = {%l{%c}}\n", cBase, pbs->bsHiddenSecs);
      oprintf("Reserved sectors = {%{%c}}\n", cBase, pbs->bsResSectors);
      oprintf("FATs = {%{%c}}\n", cBase, pbs->bsFATs);
      oprintf("Sectors/FAT = {%{%c}}\n", cBase, pbs->bsFATsecs);
      oprintf("Sectors/Cluster = {%{%c}}\n", cBase, pbs->bsSecPerClust);
      oprintf("Root Dir entries = {%{%c}}\n", cBase, pbs->bsRootDirEnts);
      if (pbs->bsBootSignature == 0x29) {
	printf("Drive # = %02.2X\n", pbs->bsDriveNumber);
	printf("Volume ID = %08.8lX\n", pbs->bsVolumeID);
	printf("Volume Label = \"%.11s\"\n", pbs->bsVolumeLabel);
	printf("File system = \"%.8s\"\n", pbs->bsFileSysType);
      }
    }

    if (hTo) {
      // iErr = BlockWrite(hTo, qwToSect+uSect, nTo, pBuf);
      // Problem: The Block API is limited to 0xFFFF blocks at a time.
      // This is fine for disks, but not for files ==> Split in small I/Os
      unsigned int uDone = 0;   // Number of blocks already written
      unsigned int uLeft = nTo; // Number of blocks left to write
      WORD wTo = 0x8000;        // Write at most 32K blocks at a time
      iErr = 0;
      for ( ; uLeft; uLeft-=wTo, uDone+=wTo) {
	if (uLeft < wTo) wTo = (WORD)uLeft; // Last batch
	if ((iErr = BlockWrite(hTo, qwToSect+uDone, wTo, pBuf+(uDone*BlockSize(hTo)))) != 0) {
	  oprintf("\nError: Can't write {%s} {%s} {%I64{%c}}\n", pszTo, BlockIndexName(hTo), cBase, qwToSect+uDone);
	  break;
	}
      }
      if (iErr) break;
    }
  }
  /* Dirty workaround for Windows' auto-mount feature, that blocks access to the partitions while we write them */
#ifdef _WIN32
  } /* End of the 2-phase copy */
#endif /* defined(_WIN32) */

  BlockClose(hTo);
  BlockClose(hFrom);

  if ((!iErr) && (!iQuiet)) printf("\r%40s\rDone.\n", "");

  return (iErr ? 1 : 0);
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
|                                                                             |
|  Notes								      |
|									      |
|  History								      |
|    1994-09-09 JFL Created this routine				      |
|    2017-07-28 JFL Display the MsvcLibX and SysLib libraries versions.       |
*									      *
\*---------------------------------------------------------------------------*/

/* Get the program version string, optionally with libraries versions */
char *version(int iLibsVer) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME DEBUG_VERSION;
  char *pszVer = NULL;
  if (iLibsVer) {
    char *pszLibVer = ""
#if defined(_MSVCLIBX_H_)	/* If used MsvcLibX */
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
#if defined(__SYSLIB_H__)	/* If used SysLib */
#include "syslib_version.h"
	  " ; SysLib " SYSLIB_VERSION
#endif
    ;
    pszVer = (char *)malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  printf("\nDisk sector manager version %s\n\n", version(FALSE));

  printf("\
Usage:\n\
\n\
  sector [switches] {source} [destination [number [origin [origin]]]]\n\
\n\
With...\n\
\n\
{source} = Where to read from. Either C: , NN: , or filename.\n\
  C: = Logical drive letter. Example: \"A:\"\n\
  NN: = BIOS physical drive number. Example: \"80:\" for HD 0 or \"0:\" for FD 0\n\
  fdN: = Floppy Disk number N. Example: \"fd0:\" for FD 0\n\
  hdN: = Hard Disk number N. Example: \"hd0:\" for HD 0\n\
  filename = Any valid file pathname.\n\
[destination] = Where to write to. Same format as {source}.\n\
  Default or \"-\": Dump the data read on to the standard output.\n\
[number] = The number of sectors to read, Default: 1\n\
[origin] = Linear Block Address of the first sector. Default: Sector 0.\n\
           Note: For disks, the unit is 512 bytes; For files, it's 1 byte.\n\
\n");
  printf("\
Switches:\n\
\n\
  -a    Copy all sectors from source.\n\
  -bpb  Dump a boot sector Bios Parameter Block.\n"
#ifdef _DEBUG
"\
  -d    Debug mode.\n\
"
#endif
"\
  -fbs  Find boot sectors.\n\
  -g    Display the source or destination drives geometry.\n\
  -H    Display disk sizes in MB, GB, etc. (default)\n\
  -I    Display disk sizes in MiB, GiB, etc.\n\
  -ld   List available disks. (alias -l)\n\
  -n N  Number of sectors to copy. Default: 1 for disks, all for files.\n\
  -p    Force dumping the partition table.\n\
  -ro   Read-only mode. Simulate commands execution without any write.\n\
  -s N  Set sector size. Default: Biggest of the two. 1/files. ~512/disks.\n\
  -sb OFFSET VALUE  Set byte. Idem with -sw, -sdw, -sqw for word, dword, qword.\n\
  -sh   Set the \"Hidden\" field in a boot sector BPB equal to the BS LBA.\n\
  -spt N [Parameters]     Set partition table N (0 to 3) entry. Details below.\n\
  -t    Use base 10 for input and output.\n\
  -v    Display verbose information.\n\
  -V    Display this program version and exit.\n\
  -x    Use base 16 for input and output. (default)\n\
  -z    Append zeros to the end of input data if needed.\n\
\n");
    printf("\
To update an entry in a partition table (PT): Use the -spt option.\n\
Can update directly a PT on a drive: Ex: {source} = 80: ; No destination.\n\
Or updates a PT within a file. Must specify the eventual target drive.\n\
Parameters = List of PT parameters, in the order displayed by this program:\n\
{Type} {Boot} {BegCyl BegHead BegSect} {EndCyl EndHead EndSect} {First Length}\n\
All are in decimal. All are optional. Default is 0. All 0 = Erase PT entry.\n\
 Type   OS type. Example: 1=FAT12; 5=FAT16; etc.\n\
 Boot   0=Non-bootable; 128=Bootable.\n\
 BegCyl BegHead BegSect   First sector coords. If 0 0 0, computed from {First}.\n\
 EndCyl EndHead EndSect   Last sector coords. If 0 0 0, computed from {Length}.\n\
 First  32-bits index of 1st sector. If 0, computed from BegCyl/BegHead/BegSect\n\
 Length 32-bits number of sectors. If 0, computed from EndCyl/EndHead/EndSect.\n\
\n\
Author: Jean-Francois Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");
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
  { 0x27, "MS Service" },	    // Hidden NTFS partition with Windows kernel
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

int dump_part(MASTERBOOTSECTOR *pb) {
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
  for (i = 0; i < 4; i++, pp++) {
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

    qwSize = (QWORD)(pp->n_sectors) << 9; /* multiply by 512 */
    FormatSize(qwSize, szSize, sizeof(szSize), iKB);
    if (cBase == 'u') {
      pszFormat = "%3u %-16s %c |%4u %3u%3u | %4u %3u%3u |%8lu %10lu |%7s\n";
      if (pp->n_sectors == 0xFFFFFFFF) // Force displaying -1 here v
      pszFormat = "%3u %-16s %c |%4u %3u%3u | %4u %3u%3u |%8lu %10ld |%7s\n";
    } else { // Assume iBase == 16
      pszFormat = " %02X %-16s %c |%4X %3X%3X | %4X %3X%3X |%8lX %10lX |%7s\n";
    }

    printf(pszFormat,
	    type, pszPartitionName, pp->boot ? 'Y' : 'N',
	    bcyl, pp->beg_head, pp->beg_sect,
	    ecyl, pp->end_head, pp->end_sect,
	    pp->first_sector, pp->n_sectors,
	    /* pp->n_sectors / 1953 */ szSize);
  }

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetBiosErrorString					      |
|									      |
|   Description     Convert an int 13 BIOS error code into a text string.     |
|									      |
|   Parameters	    int iErr		    The BIOS error code 	      |
|									      |
|   Returns	    The string describing the error.  			      |
|									      |
|   Notes								      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

char *GetBiosErrorString(int iErr) {
  switch (iErr) {
    case 0x01: return "Invalid function in AH or invalid parameter";
    case 0x02: return "Address mark not found";
    case 0x03: return "Disk write-protected";
    case 0x04: return "Sector not found/read error";
    case 0x05: return "Reset failed";
    case 0x06: return "disk changed";
    case 0x07: return "drive parameter activity failed";
    case 0x08: return "DMA overrun";
    case 0x09: return "Data boundary error (DMA across 64K boundary or >80h sectors)";
    case 0x0A: return "Bad sector detected";
    case 0x0B: return "Bad track detected";
    case 0x0C: return "Unsupported track or invalid media";
    case 0x0D: return "Invalid number of sectors on format";
    case 0x0E: return "Control data address mark detected";
    case 0x0F: return "DMA arbitration level out of range";
    case 0x10: return "Uncorrectable CRC or ECC error on read";
    case 0x11: return "Data ECC corrected";
    case 0x20: return "Controller failure";
    case 0x31: return "No media in drive";
    case 0x32: return "Incorrect drive type stored in CMOS";
    case 0x40: return "Seek failed";
    case 0x80: return "Time-out-";
    case 0xAA: return "Drive not ready";
    case 0xB0: return "Volume not locked in drive";
    case 0xB1: return "Volume locked in drive";
    case 0xB2: return "Volume not removable";
    case 0xB3: return "Volume in use";
    case 0xB4: return "Lock count exceeded";
    case 0xB5: return "Valid eject request failed";
    case 0xBB: return "Undefined error";
    case 0xCC: return "Write fault";
    case 0xE0: return "Status register error";
    case 0xFF: return "Sense operation failed";

    default: return "Unknown BIOS error";
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetDosErrorString 					      |
|									      |
|   Description:    Convert an int 21H error code into an English string      |
|									      |
|   Parameters:     int iErr	    The int 21H error code		      |
|									      |
|   Returns:	    An English string describing the error		      |
|                                                                             |
|   History:								      |
|									      |
|    1998-05-11 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

char *GetDosErrorString(int iErr) {
  switch (iErr) {
    // Errors for int 24H/25H
    case 0x00: return "Write protect violation";
    case 0x01: return "Unknown unit";
    case 0x02: return "Drive not ready";
    case 0x04: return "Data CRC error";
    case 0x06: return "Seek error";
    case 0x07: return "Unknown media";
    case 0x08: return "Sector not found";
    case 0x0A: return "Write fault";
    case 0x0B: return "Read fault";
    case 0x0C: return "General failure";
    case 0x0F: return "Invalid media change";
#if NEEDED
    // Errors for int 21H (partial list)
    case 0x01: return "Invalid function";
    case 0x02: return "File not found";
    case 0x03: return "Path not found";
    case 0x04: return "Too many open files";
    case 0x05: return "Access denied";
    case 0x06: return "Invalid handle";
    case 0x07: return "Memory arena trashed";
    case 0x08: return "Not enough memory";
    case 0x09: return "Invalid memory block";
    case 0x0A: return "Bad environment";
    case 0x0B: return "Bad file format";
    case 0x0C: return "Invalid access";
    case 0x0D: return "Invalid data";
    case 0x0F: return "Invalid drive";
    case 0x10: return "Current directory";
    case 0x11: return "Not same device";
    case 0x12: return "No more files";
    case 0x13: return "Write protect";
    case 0x14: return "Bad unit";
    case 0x15: return "Not ready";
#endif
    default: return "Unknown error";
  }
}

