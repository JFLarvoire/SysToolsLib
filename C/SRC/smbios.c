/*****************************************************************************\
*									      *
*   File name:	    smbios.c						      *
*									      *
*   Description:    DMI / SMBIOS tables management routines		      *
*									      *
*   Notes:	    Specification up to version 2.0 were called DMI BIOS.     *
*		    Specification version 2.1 and above were called SMBIOS.   *
*		    DMI stands for Desktop Management Interface.	      *
*		    SMBIOS stands for System Management BIOS.		      *
*									      *
*   History:								      *
*    1996-05-20 JFL Initial implementation.				      *
*    1997-06-13 JFL Added DMI BIOS 2.0 support. 			      *
*    1998-10-30 JFL Fixed bug in SMBIOS 2.0 table read. 		      *
*    1998-11-03 JFL Added detection of SMBIOS 2.1 table-based support.	      *
*    1998-11-04 JFL Added detection of HP extensions to DMI 2.0 table access. *
*    1999-07-08 JFL Restructured to make the code more easily reusable.       *
*		    Created routines DmiInit() and DmiGetStructByHandle().    *
*    1999-07-12 JFL Split this file into dmi.c and dmidump.c.		      *
*		    Added switches -d, -20 and -32.			      *
*    2002-07-15 JFL Added decoding of common structures. Version 1.4.	      *
*    2005-05-30 JFL Added some SMBIOS 2.4 information. Version 1.5.	      *
*    2010-06-14 JFL Adapted to WIN32. Removed the -p option. Version 2.0.     *
*    2012-01-04 JFL Added decoding of type 17 (memory module).		      *
*		    Output the structure type in decimal instead of hexa.     *
*    2012-01-05 JFL Major improvements to allow using the output of this      *
*		    program in management scripts.			      *
*                   Added structure names up to SMBIOS 2.7, and some contents.*
*		    Restructured with standard fields definitions in dmistd.c *
*                   and HP-specific OEM extensions in dmihp.c.                *
*		    Removed option: -d					      *
*		    Added options: -a -b -dw -i -l -n -q -qw -s -t -V -w -x   *
*                   Version 2.1.					      *
*    2012-07-17 JFL Added support for a Win64 version. Version 2.1.1.         *
*    2012-07-17 JFL Renamed this program as smbios.exe. Version 2.1.2.        *
*    2012-10-18 JFL Added my name in the help. Version 2.1.3.                 *
*    2016-03-29 JFL Fixed support for WIN64. Decode several more structures.  *
*		    Report the table length, plus the extra strings length.   *
*		    In verbose mode, dump the table and its trailing strings. *
*                   Version 2.2.					      *
*    2016-04-27 JFL Use SYSLIB's SMBIOS and UUID management routines.	      *
*		    This fixes issues with cleared UUID fields in Windows.    *
*		    Added option -m to Specify an SMBIOS access method.	      *
*		    Added option -d to enable the debug mode.          	      *
*                   Version 2.2.1.					      *
*    2016-07-04 JFL Restructured to make the OEM tables decoding optional.    *
*		    Use the -m option for DOS too, instead of -20 and -32.    *
*                   Version 2.3.					      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.2.3.1.*
*    2019-04-28 JFL Update PROGRAM_VERSION if including HPE-specific tables.  *
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.3.2.      *
*    2022-02-01 JFL Improved the interface to the optional HPE tables decoder.*
*                   Version 2.4.					      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Display SMBIOS tables contents"
#define PROGRAM_NAME      "smbios"
#define PROGRAM_VERSION_0 "2.4"
#define PROGRAM_DATE      "2022-02-01"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if HAS_SYSLIB
#include "smbios.h"	    /* SMBIOS access definitions and routines */
#include "uuid.h"	    /* UUID management routines */
#endif

#ifndef HAS_SYSLIB
#pragma message("Including smbios_lib.c")
#include "smbios_lib.c"		/* SMBIOS access definitions and routines */
#endif

#include "smbios_defs.c"	/* Standard tables definitions */
										/* Include lines in $(O)\smbios.ver.h */
#if HAS_SMBIOS_HP								/* #define PROGRAM_VERSION */
#define PROGRAM_VERSION_HPE "/HPE"                                              /* #define PROGRAM_VERSION */
#else                                                                           /* #define PROGRAM_VERSION */
#define PROGRAM_VERSION_HPE ""                                                  /* #define PROGRAM_VERSION */
#endif                                                                          /* #define PROGRAM_VERSION */

#define PROGRAM_VERSION PROGRAM_VERSION_0 PROGRAM_VERSION_HPE

/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

/* A convenient macro */
#define streq(s1, s2) (!strcmp(s1, s2))     /* Test if strings are equal */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define SUPPORTED_OS 1

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#define SUPPORTED_OS 1

#endif /* _MSDOS */

/******************************* Any other OS ********************************/

#ifndef SUPPORTED_OS
#error "Unsupported OS"
#endif

/********************** End of OS-specific definitions ***********************/

#define FALSE 0
#define TRUE 1

/* Global variables */

int iVerbose = 0;

/* -------------------------- FUNCTION prototypes -------------------------- */
/* ------------------------------------------------------------------------- */

void usage(int retcode);
int IsSwitch(char *pszArg);
int PrintStringIfDefined(PSMBIOS21HEADER pDmi21Hdr, BYTE *pStruct, int iString, char *pszDescript);

/* ------------- Optional extensions using the above prototypes ------------ */

/* -------------------------- HPE BIOS extensions -------------------------- */

#if HAS_SMBIOS_HP
#pragma message("Including smbios_hp.c")
#include "smbios_hp.c"		/* HP OEM-specific tables definitions */
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    C program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|    1996/05/20 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[]) {
  int i, j;
  char *sz;		/* Temp string */
  DWORD dw;		/* Temp double word */
  SMBIOS21HEADER dmi21Hdr;
  int hStruct;
  int hNext;
  BYTE *pStruct;
  int iDump = FALSE;
  int iType = -1;
  DWORD dwMethod = 0; 	/* Default: Let the program choose the method. */
  int iCol;
  int iSmbiosVersion;
  BYTE bSize;
  int iSize;
  DWORD dwFeatures;
  char *szDmi = "DMI";	/* DMI or SMBIOS */
  int iList = FALSE;	/* If TRUE, list structures, but don't dump them */
  int iNum = -1;	/* If >= 0, list only that structure */
  int iDumpString = 0;	/* If != 0, dump only the string referenced at this offset */
  int iDumpByte = 0;	/* If != 0, dump only the byte at this offset */
  int iDumpWord = 0;	/* If != 0, dump only the word at this offset */
  int iDumpDWord = 0;	/* If != 0, dump only the dword at this offset */
#ifdef QWORD_DEFINED
  int iDumpQWord = 0;	/* If != 0, dump only the qword at this offset */
#endif
  int iHeader = FALSE;	/* If TRUE, display DMI/SMBIOS table header infos. */
  int iHexa = FALSE;	/* If TRUE, output byte/word/dword in hexadecimal */
  char *szByteFormat = "%d\n";		/* Format to use to output bytes */
  char *szWordFormat = "%d\n";		/* Format to use to output words */
  char *szDWordFormat = "%ld\n";	/* Format to use to output dwords */
#ifdef QWORD_DEFINED
  char *szQWordFormat = "%I64d\n";	/* Format to use to output dwords */
#endif
  int iQuiet = FALSE;	/* If TRUE, display only the bare minimal infos */

  /* Process arguments */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(argv[i])) {	    /* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "?")) {		/* -?: Help */
	usage(0);			     /* Display help & exit */
      }
      if (streq(opt, "a")) {		/* -a: Dump all tables */
	iDump = TRUE;
	iType = -1;
	iHeader = TRUE;
	continue;
      }
      if (streq(opt, "b")) {		/* -b: Dump a specific byte */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  iDumpByte = atoi(argv[++i]);
	}
	continue;
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_ON();
	  printf("Debug mode.\n");
	  continue;
	}
      )
      if (streq(opt, "dw")) {		/* -dw: Dump a specific dword */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  iDumpDWord = atoi(argv[++i]);
	}
	continue;
      }
      if (streq(opt, "i")) {		/* -i: Display DMI/SMBIOS header information */
	iHeader = TRUE;
	continue;
      }
      if (streq(opt, "l")) {		/* -l: List tables */
	iList = TRUE;
	continue;
      }
      if (streq(opt, "m")) {		/* -m: Specify an SMBIOS access method */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  dwMethod = atoi(argv[++i]);
	}
#ifdef _MSDOS
	switch (dwMethod) {
	  case 1: dwMethod = 0x5F4D535F; break;	/* '_SM_' Use SMBIOS 2.1+ 32-bits tables in ROM */
	  case 2: dwMethod = 0x494D445F; break;	/* '_DMI' Use HP DMI 2.0  32-bits tables in ROM */
	  case 3: dwMethod = 0x506E5024; break;	/* '$PnP' Use SMBIOS 2.0+ 16-bits tables PnP API */
	  case 4: dwMethod = 0x494D4424; break;	/* '$DMI' Use DMI    1.0+ 16-bits tables in ROM */
	}
#endif
	continue;
      }
      if (streq(opt, "n")) {		/* -n: Dump a specific table */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  if (!iList) iDump = TRUE;
	  iNum = atoi(argv[++i]);
	}
	continue;
      }
      if (streq(opt, "q")) {		/* -l: Quiet mode */
	iQuiet = TRUE;
	continue;
      }
#ifdef QWORD_DEFINED
      if (streq(opt, "qw")) {		/* -qw: Dump a specific qword */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  iDumpQWord = atoi(argv[++i]);
	}
	continue;
      }
#endif
      if (streq(opt, "s")) {		/* -s: Dump a specific string */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  iDumpString = atoi(argv[++i]);
	}
	continue;
      }
      if (streq(opt, "t")) {		/* -t: Dump only tables of type N */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  if (!iList) iDump = TRUE;
	  iType = atoi(argv[++i]);
	}
	continue;
      }
      if (streq(opt, "v")) {		/* -v: Verbose */
	iVerbose += 1;
	iHeader = TRUE;
	continue;
      }
      if (streq(opt, "V")) {		/* -V: Display the version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      if (streq(opt, "w")) {		/* -w: Dump a specific word */
	if ( ((i+1) < argc) && !IsSwitch(argv[i+1]) ) {
	  iDumpWord = atoi(argv[++i]);
	}
	continue;
      }
      if (streq(opt, "x")) {		/* -x: Hexadecimal output */
	iHexa = TRUE;
	szByteFormat = "%02X\n";	/* Format to use to output bytes */
	szWordFormat = "%04X\n";	/* Format to use to output words */
	szDWordFormat = "%08lX\n";	/* Format to use to output dwords */
#ifdef QWORD_DEFINED
	szQWordFormat = "%016I64X\n";	/* Format to use to output dwords */
#endif
	continue;
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    printf("Unexpected argument: %s. Ignored.\n", arg);
    break;  /* Ignore other arguments */
  }

#ifdef _WIN32 /* Windows provides SMBIOS data with this header */
  DEBUG_CODE(
    /* Sanity check: SmBiosClose() assumes this */
    i = sizeof(RAWSMBIOSDATA);
    if (i != 8) {
      printf("Error: RAWSMBIOSDATA is %d bytes. Expected 8 bytes.\n", i);
      exit(1);
    }
  )
#endif

  /* The default action is to dump just the DMI/SMBIOS header */
  if ((!iDump) && (!iList)) iHeader = TRUE;

  iSmbiosVersion = SmBiosInit(&dmi21Hdr, dwMethod);
  if (!iSmbiosVersion)
    {
    if (!dwMethod)
      fprintf(stderr, "Error: This BIOS does not support DMI/SMBIOS.\n");
    else
      fprintf(stderr, "Error: This BIOS does not support this DMI/SMBIOS access method.\n");
    exit(1);
    }

  if (iHeader) printf("SMBIOS version %d.%d\n", iSmbiosVersion>>8, iSmbiosVersion&0x0F);

  switch (dmi21Hdr.dwSignature)
    {
    case $DMI:
      szDmi = "DMI";
      if (iHeader) printf("Reading from 16-bits DMI 1.x tables in ROM.\n");
      if (iVerbose) printf("DMI header at %lp\n",
			   *(LPSMBIOSHEADER *)(dmi21Hdr.bFormatted) );
      break;

#ifdef _MSDOS
    case $PnP:
      szDmi = "DMI";
      if (iHeader) printf("Calling 16-bits PnP-BIOS API.\n");
      if (iVerbose)
	{
	LPPNPHEADER lpPnpHdr;
	LPPNPBIOS lpPnpBios;

	lpPnpHdr = *(LPPNPHEADER *)(dmi21Hdr.bFormatted);
	lpPnpBios = lpPnpHdr->lpRmEntry;

	printf("PnP BIOS entry at %lp\n", lpPnpBios);
	}
      break;
#endif /* defined(_MSDOS) */

    case _SM_:
      szDmi = "SMBIOS";
#ifdef _MSDOS
      if (iHeader) printf("Reading from 32-bits SMBIOS 2.1+ tables in ROM.\n");
#endif
#ifdef _WIN32
      if (iHeader) {
      	switch (dmi21Hdr.bFormatted[0]) {
	  case 1: printf("Reading SMBIOS tables using the XP SP2+ WIN32 API.\n"); break;
	  case 2: printf("Reading from SMBIOS tables copy in the registry.\n"); break;
      	}
      }
      if (iVerbose) printf("(in HKLM\\SYSTEM\\CurrentControlSet\\services\\mssmbios\\Data\\SMBiosData)\n");
#endif
      break;

    case _DMI:
      szDmi = "SMBIOS";
      if (iHeader) printf("Reading from 32-bits HP-proprietary DMI 2.0 tables in ROM.\n");
      if (iVerbose) printf("DMI header at %lp\n",
			   *(LPSMBIOS20HPHEADER *)(dmi21Hdr.bFormatted) );
      break;

    default:
      fprintf(stderr, "Error: Tables access method unsupported by this program yet.\n");
      exit(1);
    }

  if (iHeader) printf("%d structures <= %u bytes.\n", dmi21Hdr.wNumStructures,
						      dmi21Hdr.wMaxStructSize);
  if (iVerbose) {
#ifndef _WIN64 /* DOS or WIN32 cases: 32-bits address */
               printf("Base at %08lX length %04X.\n", dmi21Hdr.dwPhysicalAddress,
						      dmi21Hdr.wTotalSize);
#else	       /* Win64 case: 64-bits address */
               printf("Base at %08lX%08lX length %04X.\n", DWORD_AT(dmi21Hdr.bFormatted,1),
						      dmi21Hdr.dwPhysicalAddress,
						      dmi21Hdr.wTotalSize);
#endif
  }

  /* Scan all tables and dump them */

  if (iDump || iList) {
    int iXSize;
    int iSizeToDump;
    int isHP = SmBiosIsHpPc(&dmi21Hdr);	/* TRUE if HP BIOS detected. */

    DEBUG_PRINTF(("isHP = %d\n", isHP));

    pStruct = (BYTE *)SmBiosAllocStruct(&dmi21Hdr);
    if (!pStruct) {
      fprintf(stderr, "Error: Not enough memory for %s structure copy.\n", szDmi);
      exit(1);
    }

    if (iHeader) printf("\n");

    for (hStruct = 0; hStruct != -1; hStruct = hNext) {
      BYTE bType;

      hNext = SmBiosGetStructByHandle(&dmi21Hdr, hStruct, pStruct);
      if (!hNext) {
	if (hStruct == 0)
	  printf("Access method not supported by this program yet.\n");
	break; /* Unexpected error reading the structure. */
      }
      bType = pStruct[0];

      /* Display only the structure specified, if any. */
      if ((iNum != -1) && (iNum != hStruct)) continue;
      /* Display only the specified structure type, if any. */
      if ((iType != -1) && ((BYTE)iType != bType)) continue;

      /* Get the structure size */
      bSize = pStruct[1];
      iSize = (int)bSize;
      iXSize = SmBiosGetFullStructureSize(pStruct);

      /* Dump a specific string */
      if (iDumpString) {
      	i = 0;
      	if (iDumpString < iSize) i = pStruct[iDumpString];
      	if (i) printf("%s\n", SmBiosGetString(&dmi21Hdr, pStruct, i));
      	continue;
      }
      /* Dump a specific byte */
      if (iDumpByte) {
      	if (iDumpByte < iSize) printf(szByteFormat, pStruct[iDumpByte]);
      	continue;
      }
      /* Dump a specific word */
      if (iDumpWord) {
      	if (iDumpWord < iSize) printf(szWordFormat, WORD_AT(pStruct,iDumpWord));
      	continue;
      }
      /* Dump a specific dword */
      if (iDumpDWord) {
      	if (iDumpDWord < iSize) printf(szDWordFormat, (unsigned long)DWORD_AT(pStruct,iDumpDWord));
      	continue;
      }
#ifdef QWORD_DEFINED
      /* Dump a specific qword */
      if (iDumpQWord) {
      	if (iDumpQWord < iSize) printf(szQWordFormat, QWORD_AT(pStruct,iDumpQWord));
      	continue;
      }
#endif

      /* Display a one-line structure presentation */
      if (iQuiet) {
	printf("%d %d\n", hStruct, bType);
	continue;
      }
      printf("%s Structure # %d type %d", szDmi, hStruct, bType);
      if (iList) {
      	if (bType < NDMI2TABLETYPES) {
      	  printf(": %s\n", szDmi2TableTypes[bType]);
      	} else if (bType == 126) {
      	  printf(": Inactive Structure\n");
      	} else if (bType == 127) {
      	  printf(": End-of-Table\n");
#if HAS_SMBIOS_HP
      	} else if (isHP && (bType >= 192) && (bType < (192 + NHPTABLETYPES))) {
      	  printf(": %s\n", szHpTableTypes[bType - 192]);
#endif
      	} else {
      	  printf(": Unknown type\n");
      	}
      	continue;
      }
      i = iXSize-iSize;		/* Extra space for trailing strings */
      if (--i == 1) i = 0;	/* If there's no extra string, report 0 */
      printf(" length %d + %d\n", iSize, i);

      /* Dump the structure contents */
      iSizeToDump = iSize;
      if (iVerbose) iSizeToDump = SmBiosGetFullStructureSize(pStruct);
      for (i=0; i<iSizeToDump; i++) {
	if (i && !(i&0x1F)) printf("\n");   /* New line every 32 bytes. */
	if (!(i&3)) printf(" ");	    /* Space every 4 bytes. */
	printf("%02X", pStruct[i]);
      }
      printf("\n");

      /* Decode known structures contents */
      if (iSmbiosVersion < 0x200) {	    /* If DMI 1.x */
	WORD wDmiSeg = WORD1(dmi21Hdr.dwPhysicalAddress) << 12;	/* The two valid values are E000 and F000. */
	switch (bType) {
	  case 1:
	    printf("BIOS information:\n");
	    printf("Vendor = %Fs; ", *(DWORD *)(pStruct+2));
	    printf("Version = %Fs; ", *(DWORD *)(pStruct+6));
	    printf("Release date = %Fs\n", *(DWORD *)(pStruct+0x0C));
	    printf("ROM size = %d KB; ", 64 << pStruct[0x14]);
	    printf("Base segment = %04X; ", *(WORD *)(pStruct+0x0A));
	    printf("Features = %08lX (TBD)\n", *(DWORD *)(pStruct+0x10)); /* Needs decoding feature DWORD at offset 0x10; */
	    break;
	  case 2:
	    printf("PC information:\n");
	    printf("Manufacturer = %Fs; ", *(WORD *)(pStruct+2), wDmiSeg);
	    printf("Product = %Fs\n", *(WORD *)(pStruct+4), wDmiSeg);
	    printf("Version = %Fs; ", *(WORD *)(pStruct+6), wDmiSeg);
	    printf("Serial # = %Fs\n", *(WORD *)(pStruct+8), wDmiSeg);
	    break;
	  case 3:
	    printf("Processor information:\n");
	    printf("Manufacturer = %Fs; ", *(WORD *)(pStruct+4), wDmiSeg);
	    printf("Name = %Fs; ", *(WORD *)(pStruct+6), wDmiSeg);
	    printf("Family = %d; ", pStruct[3]);
	    printf("Model = %d\n", pStruct[2]);
	    printf("Max speed = %d MHz; ", *(WORD *)(pStruct+8));
	    i = (int)pStruct[0x0A];
	    if (i >= NPROCSOCKETTYPES) i = 0;
	    printf("Proc socket = %s\n", szProcSocketType[i]);
	    break;
	  case 4:
	    printf("Memory per SIMM socket:\n");
	    printf("Socket = %Fs; ", *(WORD *)(pStruct+2), wDmiSeg);
	    
	    bSize = pStruct[5];
	    iSize = 1 << (int)(bSize & 0x7F);
	    printf("SIMM size = %d MB; ", iSize);
	
	    bSize = pStruct[6];
	    iSize = 1 << (int)(bSize & 0x7F);
	    printf("Max size = %d MB; ", iSize);
	
	    printf("Speed = %d ns\n", pStruct[7]);
	    break;
	  case 5:
	    printf("Cache information:\n");
	    printf("Level %d; ", pStruct[2]);
	    
	    bSize = pStruct[3];
	    iSize = (int)(bSize & 0x7F);
	    if (bSize & 0x80) iSize *= 64;
	    printf("Size = %d; ", iSize);
	
	    bSize = pStruct[4];
	    iSize = (int)(bSize & 0x7F);
	    if (bSize & 0x80) iSize *= 64;
	    printf("Max = %d\n", iSize);
	    break;
	  case 6:
	    printf("Port connector information:\n");
	    printf("Ref = %Fs; ", *(WORD *)(pStruct+4), wDmiSeg);
	    
	    i = (int)pStruct[3];
	    if (i >= NPORTTYPES) i = 0;
	    printf("Type = %s; ", szPortType[i]);
	    
	    i = (int)pStruct[2];
	    if (i >= NCONNTYPES) i = 0;
	    printf("Connector = %s\n", szConnType[i]);
	    break;
	  case 7:
	    printf("System slots:\n");
	    printf("Ref = %Fs; ", *(WORD *)(pStruct+2), wDmiSeg);
	    
	    i = (int)pStruct[4];
	    if (isHP) { /* HP has a non-standard implementation, with the Nth bit set, instead of value N. */
	      if (i&1)
		printf("Full length; ");
	      else
		printf("Half length; ");

	      j = i;
	      for (i = 1; i < NSLOTTYPES; i++) if (j & (1 << i)) {
		printf("Type = %s; ", szSlotType[i]);
	      }
	    } else {	    /* Standard implementation */
	      if (i >= NSLOTTYPES) i = 0;
	      printf("Type = %s; ", szSlotType[i]);
	    }

	    printf("Width = %d bits; ", 1 << pStruct[5]);
	    
	    i = (int)pStruct[6];
	    if (i >= NSLOTUSES) i = 0;
	    printf("Use = %s\n", szSlotUse[i]);
	    break;
	  case 8:
	    printf("OEM strings:\n");
	    break;
	  default: {
	    int iDecoded = FALSE;
#if HAS_SMBIOS_HP
      	    if (isHP && (bType >= 0x80)) {
      	      iDecoded = DecodeHPDmiTable(pStruct);
      	    }
#endif
	    if (!iDecoded) {
	      printf("Unknown structure.\n");
	    }
	    break;
	  }
	}
      } else {			/* Else SMBIOS 2.x or better */
	switch (bType) {
	  case 0:
	    printf(" BIOS information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Vendor");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 5, "Version");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 8, "Release date");
	    printf("  ROM size = %d KB\n", 64 * (pStruct[9] + 1));
	    printf("  Base segment = %04X\n", *(WORD *)(pStruct+6));
	    dwFeatures = *(DWORD *)(pStruct+0x0A);
	    printf("  Features = %08lX\n", dwFeatures);
	    for (j=0; j<NBIOSCHARACTERISTICS; j++) { /* Decode BIOS feature DWORD at offset 0x10; */
	      if (dwFeatures & (1UL << j)) printf("\t%s\n", szBiosCharacteristics[j]);
	    }
	    printf("  BIOS Vendor Features = %04X\n", WORD_AT(pStruct,0x0E));
	    printf("  System Vendor Features = %04X\n", WORD_AT(pStruct,0x10));
	    if (iSize > 0x12) {
	      i = BYTE_AT(pStruct,0x12);
	      if (i) {
	        printf("  Features Extension 1 = %02X\n", i);
		for (j=0; j<8; j++) { /* Decode BIOS feature DWORD at offset 0x10; */
		  if (i & (1 << j)) printf("\t%s\n", szBiosFeaturesExt[j]);
		}
	      }
	    }
	    if (iSize > 0x13) {
	      i = BYTE_AT(pStruct,0x13);
	      if (i) {
	        printf("  Features Extension 2 = %02X\n", i);
		for (j=0; j<8; j++) { /* Decode BIOS feature DWORD at offset 0x10; */
		  if (i & (1 << j)) printf("\t%s\n", szBiosFeaturesExt[j+8]);
		}
	      }
	    }
	    if (iSize > 0x14) { i = BYTE_AT(pStruct,0x14); if (i < 0xFF) printf("  Major release = %d\n", i); }
	    if (iSize > 0x15) { i = BYTE_AT(pStruct,0x15); if (i < 0xFF) printf("  Minor release = %d\n", i); }
	    if (iSize > 0x16) { i = BYTE_AT(pStruct,0x16); if (i < 0xFF) printf("  Embedded Controller Major release = %d\n", i); }
	    if (iSize > 0x17) { i = BYTE_AT(pStruct,0x17); if (i < 0xFF) printf("  Embedded Controller Minor release = %d\n", i); }
	    break;

	  case 1:
	    printf(" System information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Manufacturer");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 5, "Product");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 6, "Version");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 7, "Serial #");
	    if (iSize > 0x17) {
	      printf("  UUID = ");
	      PrintUuid((uuid_t *)(pStruct+8));
	      printf("\n");
	    }
	    if (iSize > 0x18) {
	      i = pStruct[0x18];
	      if (i) {
	        if (i >= NWAKEUPTYPES) i=2;
	        printf("  Wake Up Type = %s\n", szWakeUpType[i]);
	      }
	    }
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x19, "SKU Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x1A, "Family");
	    break;

	  case 2: {
	    printf(" Motherboard information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Manufacturer");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 5, "Product");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 6, "Version");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 7, "Serial Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 8, "Asset Tag");

	    i = BYTE_AT(pStruct, 9);
	    printf("  Feature Flags = %02X\n", i);
	    for (j=0; j<NBASEBOARDFLAGS; j++) if (i & (1<<j)) printf("   %s\n", szBaseBoardFlags[j]);

	    i = BYTE_AT(pStruct, 0x0D);
	    if (i >= NBASEBOARDTYPES) i=0;
	    printf("  Board Type = %s\n", szBaseBoardType[i]);

	    break;
	  }

	  case 3: {
	    int iType3;
	    int iLock;
	    printf(" Enclosure or chassis information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Manufacturer");
	    iType3 = BYTE_AT(pStruct,5);
	    iLock = iType3 & 0x80;
	    iType3 &= 0x7F;
	    if (iType3 >= NCHASSISTYPES) iType3=0;
	    printf("  Chassis Type = %s\n", szChassisType[iType3]);
	    if (iLock) {
	      printf("  Has a lock\n");
	    } else {
	      printf("  Does not have a lock (or unknown)\n");
	    }
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 6, "Version");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 7, "Serial Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 8, "Asset Tag");
	    /* TO DO: Decode bytes 0x09 to 0x12 */
	    if (iSize > 0x14) {
	      int nElem;
	      int lElem;
	      nElem = BYTE_AT(pStruct,0x13);
	      lElem = BYTE_AT(pStruct,0x14);
	      i = 0x15 + (nElem * lElem);
	      PrintStringIfDefined(&dmi21Hdr, pStruct, i, "SKU");
	    }
	    break;
	  }

	  case 4:
	    printf(" Processor information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x07, "Manufacturer");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x10, "Name");
	    printf("  Family = %d\n", pStruct[6]);
	    printf("  Model = %d\n", pStruct[5]);
	    printf("  Max speed = %d MHz\n", *(WORD *)(pStruct+0x14));
	    printf("  Cur speed = %d MHz\n", *(WORD *)(pStruct+0x16));
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x04, "Socket name");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x20, "Serial #");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x21, "Asset tag");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x22, "Part #");
	    break;

	  case 5:
	    printf(" Memory controller information:\n");
	    
	    printf("  Max module size = %d MB; ", 1 << pStruct[8]);

	    printf("  Supported speeds = ");
	    i = *(int *)(pStruct+9);
	    for (j=0; j<NMEMSPEEDS; j++) if (i & (1<<j)) printf("%s; ", szMemSpeed[j]);
	    if (i & ~((1<<NMEMSPEEDS)-1)) printf("Other; ");

	    printf("\n  Supported types = ");
	    i = *(int *)(pStruct+0x0B);
	    for (j=0; j<NMEMTYPES; j++) if (i & (1<<j)) printf("%s; ", szMemType[j]);
	    if (i & ~((1<<NMEMTYPES)-1)) printf("Other; ");

	    printf("\n  Supported voltage = ");
	    i = (int)pStruct[0x0D];
	    for (j=0; j<NMEMVOLTS; j++) if (i & (1<<j)) printf("%s; ", szMemVolt[j]);
	    if (i & ~((1<<NMEMVOLTS)-1)) printf("Other; ");

	    printf("\n");
	    break;

	  case 6:
	    printf(" Memory module information:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Socket");

	    printf("Type = ");
	    i = *(int *)(pStruct+7);
	    for (j=0; j<NMEMTYPES; j++) if (i & (1<<j)) printf("%s; ", szMemType[j]);
	    
	    i = (int)pStruct[9] & 0x7F;
	    printf("Size = ");
	    switch (i)
	      {
	      case 0x7D: printf("Not determinable; "); break;
	      case 0x7E: printf("Module is installed, but not enabled; "); break;
	      case 0x7F: printf("Not installed; "); break;
	      default: printf("%d MB; ", 1 << i); break;
	      }

	    printf("Speed = %d ns\n", pStruct[6]);
	    break;

	  case 16:
	    printf(" Memory array:\n");

	    i = BYTE_AT(pStruct, 4);
	    if (i<NMEMORYARRAYLOCATIONS) printf("  Location = %s\n", szMemoryArrayLocations[i]); else printf("%d\n", i); 
	    i = BYTE_AT(pStruct, 5);
	    if (i<NMEMORYARRAYUSES) printf("  Use = %s\n", szMemoryArrayUses[i]); else printf("%d\n", i); 
	    i = BYTE_AT(pStruct, 4);
	    if (i<NMEMORYARRAYECCTYPES) printf("  ECC Method = %s\n", szMemoryArrayEccTypes[i]); else printf("%d\n", i); 

	    dw = DWORD_AT(pStruct, 0x07);
	    sz = "KB";
	    if (!(dw & 0x3FF)) {
	      dw /= 1024;
	      sz = "MB";
	      if (!(dw & 0x3FF)) {
		dw /= 1024;
		sz = "GB";
	      }
	    }
	    printf("  Max Size = %lu %s\n", (unsigned long)dw, sz);

	    i = WORD_AT(pStruct, 0x0B);
	    if ((i|1)!=0xFFFF) printf("  Memory Error Information Handle = %d\n", i); 
	    i = WORD_AT(pStruct, 0x0D);
	    if (i) printf("  Number of sockets = %d\n", i); 
	    /* To do: Decode the extended size at  offset 0x0F. */

	    break;

	  case 17:
	    printf(" Memory device:\n");

	    i = WORD_AT(pStruct, 0x0C);
	    sz = "MB";
	    if (i & 0x8000) {
	      sz = "KB";
	      i &= 0x7FFF;
	    }
	    printf("  Size = %d %s\n", i, sz);

	    if (*(WORD *)(pStruct+8)) printf("  Total width = %d bits\n", *(WORD *)(pStruct+8));
	    if (*(WORD *)(pStruct+0x0A)) printf("  Data width = %d bits\n", *(WORD *)(pStruct+0x0A));

	    if (pStruct[0x0E]) printf("  Form Factor = %d\n", *(BYTE *)(pStruct+0x0E));
	    if (pStruct[0x0F]) printf("  Device Set = %d\n", *(BYTE *)(pStruct+0x0F));
	    printf("  Form Factor = ");
	    i = (int)*(BYTE *)(pStruct+0x0E);
	    if (i<NMEMORYDEVICEFORMFACTORS) printf("%s\n", szMemoryDeviceFormFactors[i]); else printf("%d\n", i); 
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x10, "Socket Name");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x11, "Bank Name");
	    i = (int)*(BYTE *)(pStruct+0x12);
	    if (i) {
	      printf("  Memory Type = ");
	      if (i<NMEMORYDEVICETYPES) printf("%s\n", szMemoryDeviceTypes[i]); else printf("%d\n", i); 
	    }
	    i = (int)*(WORD *)(pStruct+0x13);
	    if (i) {
	      printf("  Type Details = ");
	      for (j=0; j<16; j++) if (i & (1<<j)) printf("%s; ", szMemoryTypeDetails[j]);
	      printf("\n");
	    }
	    if (iSize > 0x15) if (*(WORD *)(pStruct+0x15)) printf("  Max Speed = %d MHz\n", *(WORD *)(pStruct+0x15));
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x17, "Manufacturer");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x18, "Serial Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x19, "Asset Tag");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x1A, "Part Number");
	    break;

	  case 22:
	    printf(" Portable Battery:\n");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 4, "Location");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 5, "Manufacturer");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 6, "Manufacture Date");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 7, "Serial Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 8, "Device Name");

	    i = BYTE_AT(pStruct, 9);
	    if (i >= NBATTERYCHEMISTRIES) i = 0;
	    printf("  Chemistry = %s\n", szBatteryChemistry[i]);

	    i = WORD_AT(pStruct, 0x0A);
	    if (i) printf("  Design Capacity = %u milli-watt-hours\n", i);

	    i = WORD_AT(pStruct, 0x0C);
	    if (i) printf("  Design Voltage = %u milli-volts\n", i);

	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x0E, "SBDS Version Number");
	    PrintStringIfDefined(&dmi21Hdr, pStruct, 0x14, "SBDS Device Chemistry");
	    break;

	  default: {
	    int iDecoded = 1;
	    /* Display the table type, if known. */
	    if ((int)bType < NDMI2TABLETYPES) { /* Known table, but decoding code not implemented yet */
	      printf(" %s:\n", szDmi2TableTypes[bType]);
	    } else if (bType == 126) { /* Don't move above as we want to dump the table below */
	      printf(" Inactive Structure:\n");
	    } else if (bType == 127) { /* There should be nothing to dump, but just in case, leave it here */
	      printf(" End-of-Table.\n");
#if HAS_SMBIOS_HP
      	    } else if (isHP && (bType >= 192)) {
      	      iDecoded = DecodeHPSmbiosTable(&dmi21Hdr, pStruct);
#endif
	    } else {
	      iDecoded = 0;
	    }
	    if (iDecoded == 2) break; /* It was fully decoded, including strings */
	    if (!iDecoded) {
	      printf("Unknown table:\n");
	    }
	    /* Display the following strings, if any. */
	    /* Protect ourselves against buggy tables that may lack the final extra NUL. */
	    iCol = 0;
	    if (*(WORD FAR *)(pStruct+pStruct[1])) {
	      for (i = pStruct[1]; (WORD)i < dmi21Hdr.wMaxStructSize; i++) {
		if (pStruct[i]) {
		  if (!iCol) { printf("  "); iCol = 2; }
		  printf("%c", pStruct[i]);
		  iCol += 1;
		} else {
		  printf("\n");
		  iCol = 0;
		  if (!pStruct[i+1]) break; /* 2 Consecutive NULs = end of strings */
		}
	      }
	    }
	    break;
	  } /* End Default case */
	} /* End switch */
      } /* End else DMI 2.x */
      /* Separate structures with one blank line. */
      printf("\n");
    }
  }

  exit(0);
#pragma warning(disable:4035)	/* Ignore the no return value warning */
}
#pragma warning(default:4035)	/* Ignore the no return value warning */


/*---------------------------------------------------------------------------*\
|									      *
|  FUNCTION NAME:	usage						      |
|									      |
|  DESCRIPTION: 	Display a brief help for this program.		      |
|									      |
|  INPUT PARAMETERS:							      |
|	int retcode	    Exit code testable as an ERRORLEVEL in batch files|
|									      |
|  RETURN VALUE:							      |
|	None								      |
|									      |
|  NOTES:								      |
|									      |
|  HISTORY:								      |
|   1993-05-17 JFL Initial implementation within devmain().		      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(int retcode) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: smbios [OPTIONS]\n\
\n\
Options:\n\
    -?	    Display this help screen and exit\n\
    -a      Dump all tables\n\
    -b N    Dump only the byte at offset N\n\
"
DEBUG_CODE(
"\
    -d      Enable the debug mode\n\
"
)
"\
    -dw N   Dump only the dword at offset N\n\
    -i	    Display information about the DMI/SMBIOS header\n\
    -l	    List tables, but don't dump their contents\n\
    -m N    Specify an SMBIOS access method. Default: 0=Any\n\
    -n N    Dump table number (= handle) N.\n\
    -q      Quiet mode. Display only the bare minimum information\n\
"
#ifdef QWORD_DEFINED
"\
    -qw N   Dump only the qword at offset N\n\
"
#endif /* defined(QWORD_DEFINED) */
"\
    -t N    Dump all tables of type N\n\
    -s N    Dump only the string referenced at offset N\n\
    -v	    Display additional verbose information\n\
    -V	    Display this program version and exit\n\
    -w N    Dump only the word at offset N\n\
    -x      Dump the byte/word/dword in hexadecimal. Default: decimal.\n\
\n\
Note: Currently, only the most common structure types are decoded.\n\
      Still, all types are dumped, and individual string/byte/word/dword fields\n\
      in all structure types (known or unknown) can be displayed individually.\n\
      In verbose mode, the strings area following the structure is dumped too.\n\
\n\
"
#ifdef _MSDOS
"\
SMBIOS Access Methods:\n\
    0       Default: Try the following methods in sequence\n\
    1       Use SMBIOS 2.1+ 32-bits tables in ROM\n\
    2       Use HP DMI 2.0  32-bits tables in ROM\n\
    3       Use SMBIOS 2.0+ 16-bits tables PnP API\n\
    4       Use DMI    1.0+ 16-bits tables in ROM\n\
"
#endif /* defined(_MSDOS) */
#ifdef _WIN32
"\
SMBIOS Access Methods:\n\
    0       Default: Try the following methods in sequence\n\
    1       Use WIN32 function GetSystemFirmwareTable (XP SP2 and later)\n\
    2       Use the mssmbios.sys copy of the SMBIOS table in the registry\n\
\n\
Note: All UUIDs and serial numbers are cleared in the copy of the tables\n\
      in the registry. In Windows XP SP1 or older, use the MS-DOS version of.\n\
      this program to get them if needed.\n\
"
#endif /* defined(_WIN32) */
"\
\n\
Author: Jean-Francois Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
"
);

  exit(retcode);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|									      |
|   Description:    Test if a command line argument is a switch.	      |
|									      |
|   Parameters:     char *pszArg					      |
|									      |
|   Returns:	    TRUE or FALSE					      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1997-03-04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
    case '/':
      return TRUE;
    default:
      return FALSE;
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PrintStringIfDefined				      |
|									      |
|   Description:    Print an SMBIOS structure string iff it is defined        |
|									      |
|   Parameters:     PSMBIOS21HEADER pDmi21Hdr	DMI header		      |
|		    BYTE *pStruct		Structure address	      |
|		    int iString			String definition index	      |
|		    char *pszDescript		String description	      |
|		    							      |
|   Returns:	    The number of bytes written				      |
|									      |
|   Notes:	    							      |
|		    							      |
|   History:								      |
|    2016-03-29 JFL Factored out this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int PrintStringIfDefined(PSMBIOS21HEADER pDmi21Hdr, BYTE *pStruct, int iString, char *pszDescript) {
  int iSize = BYTE_AT(pStruct, 1);
  char *pszString;
  char *pc;
  if (iString >= iSize) return 0; /* The string is not present in this version of the structure */
  iString = BYTE_AT(pStruct, iString);
  if (!iString) return 0; /* The string is not defined in this structure */
  pszString = SmBiosGetString(pDmi21Hdr, pStruct, iString);
  if ((!pszString) || (!*pszString)) return 0; /* The string is not defined in this structure */
  for (pc=pszString; *pc; pc++) if (*pc != ' ') break;
  if (!*pc) return 0; /* The string is defined, but only contains spaces */
  return printf("  %s = %s\n", pszDescript, pszString);
}

