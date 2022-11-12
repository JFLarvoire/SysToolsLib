/*****************************************************************************\
*                                                                             *
*   File name	    cpuid.c						      *
*									      *
*   Description	    Identify the CPU ID and speed.			      *
*									      *
*   Notes	    References:						      *
*		    Intel Application Note 485 - The CPUID Instruction	      *
*		    Intel 64 and IA-32 Architectures Software Developer's Manual
*		    Wikipedia: https://en.wikipedia.org/wiki/CPUID	      *
*		    							      *
*		    TO DO: Add support for VMX capabilities detection, like   *
*		    Intel's EPT (Extended Page Tables) (= AMD's SLAT (Second  *
*		    Level Address Translation))                               *
*		    Unfortunately, this is not defined in CPUID, but in MSRs. *
*		    See Intel's IA32 Software Development Manual volume 3     *
*		    chapter 28.2 (EPT) and appendix A (VMX Capability         *
*		    Reporting Facility                                        *
*                                                                             *
*                   Microsoft's amd64 C compiler does not support inline      *
*                   assembly. Instead, use the intrinsic functions that       *
*                   allow emitting cpuid or readmsr, etc, instructions.       *
*                   Reference:                                                *
*                   https://learn.microsoft.com/en-us/cpp/intrinsics/compiler-intrinsics
*                   https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex
*                   https://learn.microsoft.com/en-us/cpp/intrinsics/readmsr  *
*                                                                             *
*   History								      *
*    1997-06-13 JFL Created this file.					      *
*    1997-09-03 JFL Updated comments.					      *
*    1998-03-18 JFL Added -i option to display processor ID information.      *
*    1999-02-01 JFL Updated processors IDs and capability flags from the      *
*		     latest specs from Intel.				      *
*    2000-03-22 JFL Added Pentium III and some Willamette support.	      *
*    2003-01-22 JFL Added Pentium IV support.				      *
*    2005-04-28 JFL Added Extended Feature Flags support.		      *
*    2005-09-30 JFL Override NODOSLIB's putchar, to output on stdout.         *
*    2009-08-29 JFL Read the extended model number too.                       *
*		    Added numerous processor models to the list.              *
*		    Added numerous new feature flags definitions.             *
*    2009-08-31 JFL Added the definition of several AMD extended features.    *
*    2009-09-01 JFL Added numerous processor code names from wikipedia.       *
*		    Added a version time stamp, displayed by -?.              *
*    2009-10-06 JFL Adapted to WIN32.                                         *
*    2012-10-18 JFL Added my name in the help.                                *
*    2013-01-31 JFL Display the number of cores.                              *
*    2013-05-30 JFL Some CPUs pretend to support function 0xB, but do not.    *
*    2016-04-12 JFL Removed a duplicate prototype, now defined in pmode.h.    *
*    2017-05-31 JFL Fixed WIN32 warnings. No functional code change.	      *
*    2017-12-18 JFL Fixed DOS warnings. No functional code change.	      *
*    2019-04-19 JFL Use the version strings from the new stversion.h.         *
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition.		      *
*    2019-09-27 JFL Fixed a minor formating error in a debug message.	      *
*    2020-02-23 JFL Decode cpuid(7, 0) output.		          	      *
*    2020-02-24 JFL Added option -m to experiment with reading MSRs.	      *
*    2020-02-26 JFL Added option -w to experiment with reading WMI props.     *
*		    Output a few WMI props, including SLAT, in Windows.	      *
*		    Skip head spaces in brand string, if any.		      *
*    2020-03-02 JFL Display the CPUID index for every set of feature flags.   *
*		    Corrected typos and errors about MTRR registers.	      *
*    2022-11-09 JFL Added option -c to manually test one CPUID call.          *
*		    Added support for the WIN64 operating system.             *
*    2022-11-10 JFL Rewrite support for cpuid(0x0B), replaced by cpuid(0x1F). *
*		    Fixed the extended family calculation and display.	      *
*    2022-11-11 JFL Added many new feature bits definitions.                  *
*		    Added the short alias for each feature bit.		      *
*		    Don't display a computed name if we have the brand string.*
*		    Use debug and experimental features in debug builds only. *
*		    Restructured main() to use action flags and subroutines.  *
*		    Added options -a, -f, -n, -t to invoke individual actions.*
*		    Added option -q to query if a given feature is available. *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Identify the processor and its features"
#define PROGRAM_NAME    "cpuid"
#define PROGRAM_VERSION "2022-11-11"

/* Definitions */

#pragma warning(disable:4001)       /* Ignore the // C++ comment warning */

/************************ Win32-specific definitions *************************/

#if defined(_WIN64)

/* As the Microsoft amd64 compiler does not support inline assembly, the only
   way to invoke cpuid and rdmsr instructions is to use compiler intrinsics */

#include <intrin.h>

#elif defined(_WIN32)	/* Automatically defined when targeting a Win32 applic. */

/* These intrinsics could also be used for the x86 compiler, but as this was
   already implemented using inline assembly, leaving it this way for now */

   // Definitions, from MS-DOS' pmode.h, for data emission within the code stream.
#define DB(x) __asm _emit x			// BYTE
#define DW(x) DB((x) & 0xFF) DB((x) >> 8U)	// WORD
#define DD(x) DW((x) & 0xFFFF) DW((x) >> 16U)	// DWORD

#endif

#ifdef _WIN32		/* Automatically defined when targeting both WIN32 and WIN64 apps. */

#define SUPPORTED_OS 1

#define _CRT_SECURE_NO_WARNINGS // Don't warn about old unsecure functions.

#define _WIN32_DCOM

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Manipulate bytes, words and dwords. Can be used both as rvalue and as lvalue.
#define DWORD0(qw) (((DWORD *)(&(qw)))[0])
#define DWORD1(qw) (((DWORD *)(&(qw)))[1])

#define WORD0(qw) (((WORD *)(&(qw)))[0])
#define WORD1(qw) (((WORD *)(&(qw)))[1])
#define WORD2(qw) (((WORD *)(&(qw)))[2])
#define WORD3(qw) (((WORD *)(&(qw)))[3])

#define BYTE0(qw) (((BYTE *)(&(qw)))[0])
#define BYTE1(qw) (((BYTE *)(&(qw)))[1])
#define BYTE2(qw) (((BYTE *)(&(qw)))[2])
#define BYTE3(qw) (((BYTE *)(&(qw)))[3])
#define BYTE4(qw) (((BYTE *)(&(qw)))[4])
#define BYTE5(qw) (((BYTE *)(&(qw)))[5])
#define BYTE6(qw) (((BYTE *)(&(qw)))[6])
#define BYTE7(qw) (((BYTE *)(&(qw)))[7])

int identify_processor(void); // 0=8086, 1=80186, 2=80286, etc...

#if _DEBUG
#ifdef _MSVCLIBX_H_
extern int iDebug;
#else
int iDebug = FALSE;
#endif
#endif

#pragma warning(disable:4996)	// Ignore the sscanf function or variable may be unsafe.... warning

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#define SUPPORTED_OS 1

#pragma warning(disable:4209)	// Ignore the benign typedef redefinition warning
				// as several of the following redefine CHAR, WORD, DWORD
#include "clibdef.h"
#include "utildef.h"
#include "lodos.h"
#include "pmode.h"

#define vcpi2prot vm2prot

#if _DEBUG
int iDebug = FALSE;
#endif

#endif /* _MSDOS */

/******************************* Any other OS ********************************/

#ifndef SUPPORTED_OS
#error "Unsupported OS"
#endif

/********************** End of OS-specific definitions ***********************/

/* SysToolsLib include files */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define streq(s1, s2) (!strcmp(s1, s2))     /* Test if strings are equal */

/* Intel processors list */

typedef struct
    {
    int iFamily;
    int iModel;
    char *pszCodeName;
    char *pszName;
    } INTEL_PROC;

INTEL_PROC IntelProcList[] =
    { // See http://en.wikipedia.org/wiki/List_of_Intel_microprocessors
//  Family Model CodeName Name  Brand name
    { 4,     0,  "",		"486 DX" },
    { 4,     1,  "",		"486 DX" },
    { 4,     2,  "",		"486 SX" },
    { 4,     3,  "",		"486 DX2" },
    { 4,     4,  "",		"486 SL" },
    { 4,     5,  "",		"486 SX2" },
    { 4,     7,  "",		"486 DX2 enhanced" },
    { 4,     8,  "",		"486 DX4" },

    { 5,     1,  "P5",		"Pentium" },	    // Initial Pentium (60, 66)
    { 5,     2,  "P54C",	"Pentium" },	    // Pentium (75-200)
    { 5,     3,  "",		"Pentium Overdrive for 486 systems" },
    { 5,     4,  "P55C/Tillamook", "Pentium MMX" }, // Pentium MMX (166-233)

    { 6,     1,  "P6",		"Pentium Pro" },    // Pentium Pro
    { 6,     3,  "Klamath",	"Pentium II" },     // Pentium II (233-450)
    { 6,     5,  "DesChutes",	"Pentium II" },     // Portable P. II
    { 6,     6,  "Mendocino",	"Celeron" },
    { 6,     7,  "Katmai",	"Pentium III" },
    { 6,     8,  "CopperMine",	"Pentium III" },
    { 6,     9,  "Banias",	"Pentium M model 9 130nm" },
    { 6,    10,  "",		"Pentium III Xeon A" },
    { 6,    11,  "Tualatin",	"Pentium III model B" },
    { 6,    13,  "Dothan",	"Pentium M model D 90nm" },
    { 6,    14,  "Yonah",	"Core model E 65nm" },
    { 6,    15,  "Conroe",	"Core 2 model F 65nm" },
    { 6,    21,  "Tolapai",	"EP80579 Integrated Processor" },
    { 6,    22,  "",		"Celeron model 16h" },
    { 6,    23,  "Wolfdale",	"Core 2 Extreme 45nm" },
    { 6,    26,  "Bloomfield",	"Core i7 45nm" },
    { 6,    28,  "",		"Atom 45nm" },
    { 6,    29,  "",		"Xeon MP 45nm" },

    { 7,     0,  "Merced",	"Itanium" },

    { 15,    0,  "Willamette",	"Pentium 4 model 0 180nm" },
    { 15,    1,  "Willamette",	"Pentium 4 model 1 180nm" },
    { 15,    2,  "Northwood",	"Pentium 4 model 2 130nm" },
    { 15,    3,  "Prescott",	"Pentium 4 model 3 90nm" },
    { 15,    4,  "Prescott-2M",	"Pentium 4 model 4 90nm" }, // 64 bits
    { 15,    6,  "Cedar Mill",	"Pentium 4 model 6 65nm" }, // 64 bits

    { 16,    0,  "McKinley",	"Itanium 2 180nm" },
    { 16,    1,  "Madison",	"Itanium 2 130nm" },
    { 16,    2,  "Madison 9M",	"Itanium 2 130nm" },
    /* The Itanium processor list in Wikipedia...
       https://en.wikipedia.org/wiki/List_of_Intel_Itanium_processors
       ... has inconsistent family numbers for subsequent itanium processors.
       It's unlikely that any one of them ever ran DOS or Windows anyway, so
       no need to extend the list here. */
    };
#define N_INTEL_PROCS (sizeof(IntelProcList) / sizeof(INTEL_PROC))

/* Action flags */

#define SHOW_NAME	0x0001
#define SHOW_FEATURES	0x0002
#define SHOW_FREQUENCY	0x0004

/* Global variables */

int iVerbose = FALSE;

/* Forward references */

void usage(void);
DWORD _rdtsc(void);
long getms(void);
int GetProcessorName(int iFamily, char *pBuf, int iBufSize);
int MeasureProcSpeed(void);
int DisplayProcInfo(char *pszQuery);
DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx);
#if _DEBUG
void _rdmsr(DWORD dwECX, DWORD pdwMSR[2]);
#endif
#ifdef _WIN32
int GetWmiProcInfo(char *lpszPropName, void *lpBuf, size_t lBuf);
void DisplayProcWmiInfo(void);
#endif // defined(_WIN32)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description	    EXE program main initialization routine		      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the BIOS, if run from ROM.     |
|									      |
|   History								      |
|    1996/11/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[]) {
  int i;
  int iFamily;
  int iFrequency;
  char *pszQuery = NULL;
  int iAction = 0;
  int iFirst = TRUE;

#ifdef _MSDOS
  int iErr;

  _dos_setblock(0x1000, (WORD)_psp, (WORD *)&iErr); /* 3rd arg = max § size */
#endif // defined(_MSDOS)

  /* Process arguments */

  for (i=1 ; i<argc ; i++) {
    char *arg = argv[i];
    if ((arg[0] == '-') || (arg[0] == '/')) {  /* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "?")) {		/* -?: Help */
	usage();                            /* Display help */
      }
      if (streq(opt, "a")) {		/* -f: Show all */
	iAction = ~0;
	continue;
      }
      if (streq(opt, "c")) {		/* -c: Call the CPUID instruction */
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	dwEAX = dwEBX = dwECX = dwEDX = 0;
	if (((i+1) < argc) && sscanf(argv[i+1], "%X", &dwEAX)) {   /* Leaf number */
	  i += 1;
	  if (((i+1) < argc) && sscanf(argv[i+1], "%X", &dwECX)) { /* Sub-leaf number */
	    i += 1;
	    printf("CPUID(0x%lX, %0xlX)\n", (unsigned long)dwEAX, (unsigned long)dwECX);
	  } else {
	    printf("CPUID(0x%lX)\n", (unsigned long)dwEAX);
	  }
          _cpuid(dwEAX, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	  printf("EAX = 0x%08lX\n", (unsigned long)dwEAX);
	  printf("EBX = 0x%08lX\n", (unsigned long)dwEBX);
	  printf("ECX = 0x%08lX\n", (unsigned long)dwECX);
	  printf("EDX = 0x%08lX\n", (unsigned long)dwEDX);
	  exit(0);
	} else {
	  fprintf(stderr, "Missing or invalid CPUID leaf number\n");
	  exit(1);
	}
      }
#if _DEBUG
      if (streq(opt, "d")) {		/* -d: Debug information */
        iDebug = TRUE;
        iVerbose = TRUE;
	  continue;
      }
#endif
      if (streq(opt, "f")) {		/* -f: Show features */
	iAction |= SHOW_FEATURES;
	continue;
      }
#if _DEBUG
      if (streq(opt, "m")) {		/* -m: Read MSR (Experimental)*/
	int iMSR;
	if (((i+1) < argc) && sscanf(argv[i+1], "%X", &iMSR)) {
	  DWORD pdwMSR[2];
	  i += 1;
	  printf("Reading MSR(0x%X)\n", iMSR);
	  fflush(stdout);
	  _rdmsr(iMSR, pdwMSR);
	  printf("MSR(0x%X) = 0x%08ulX:%08ulX\n", iMSR, pdwMSR[1], pdwMSR[0]);
	  exit(0);
	} else {
	  fprintf(stderr, "Missing or invalid MSR number\n");
	  exit(1);
	}
      }
#endif /* _DEBUG */
      if (streq(opt, "n")) {		/* -n: Show name */
	iAction |= SHOW_NAME;
	continue;
      }
      if (streq(opt, "q")) {		/* -q: Query if a feature is supported */
	if ((i+1) < argc) {
	  pszQuery = argv[++i];
	} else {
	  fprintf(stderr, "Missing feature name\n");
	  exit(1);
	}
	iAction |= SHOW_FEATURES;
	continue;
      }
      if (streq(opt, "t")) {		/* -t: Measure the frequency using the TSC */
	iAction |= SHOW_FREQUENCY;
	continue;
      }
      if (streq(opt, "v")) {		/* -v: Verbose information */
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {		/* -V: Display version information */
	puts(DETAILED_VERSION);
	return 0;
      }
#if defined(_WIN32)
      if (streq(opt, "w")) {		/* -w: Get WMI processor information */
      	char buf[1024];
      	char *pszPropName = argv[++i];
	int iResult = GetWmiProcInfo(pszPropName, buf, sizeof(buf));
	switch (iResult) {
	case -1: {
	  HRESULT hr = *(HRESULT *)buf;
	  fprintf(stderr, "Failed to get WMI Win32_Processor property %s. HRESULT 0x%X", pszPropName, hr);
	  break; }
	case VT_BSTR:
	  printf("%s = %s\n", pszPropName, buf);
	  break;
	default:
	  printf("%s = %d\n", pszPropName, *(int *)buf);
	  break;
	}
	return 0;
      }
#endif // defined(_WIN32)
    }
  }

  if (!iAction) iAction = SHOW_NAME;

  iFamily = identify_processor(); /* Actually the Family + Extended Family */

  /* Display the processor name */

  if (iAction & SHOW_NAME) {
    char szName[64] = "";

    if (GetProcessorName(iFamily, szName, sizeof(szName))) {
      /* if (!iFirst) printf("\n"); */ iFirst = FALSE;
      if (iVerbose) printf("The processor is an ");
      printf("%s\n", szName);
    }
  }

  /* The following actions can only be done on a Pentium or better */

  if (iFamily >= 5) {
    /* On Pentium or better, display the processor feature flags */
    if (iAction & SHOW_FEATURES) {
      if (!iFirst) printf("\n"); iFirst = FALSE;
      DisplayProcInfo(pszQuery);
    }

#ifdef _WIN32
    if ((iAction & SHOW_FEATURES) && !pszQuery) {
      if (!iFirst) printf("\n"); iFirst = FALSE;
      DisplayProcWmiInfo();
    }
#endif // defined(_WIN32)

    /* On Pentium or better, compute the processor frequency using the TSC */
    /* Note: This algorithm is compatible with Windows 95 & NT */
    if (iAction & SHOW_FREQUENCY) {
      if (!iFirst) printf("\n"); iFirst = FALSE;
      iFrequency = MeasureProcSpeed();
      printf("Measured frequency: %ld MHz\n", iFrequency);
    }
  }

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    usage						      |
|									      |
|   Description	    Display a brief help for this program		      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    N/A 						      |
|                                                                             |
|   History								      |
|    1996/10/10 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: cpuid [SWITCHES]\n\
\n\
Optional switches:\n\
\n\
  -a        Display all we know about the processor\n\
  -c EAX [ECX]  Get one given CPUID leaf and optional sub-leaf\n"
#if _DEBUG
"\
  -d        Output debug information\n"
#endif
"\
  -f        Display detailed processor features\n"
#if _DEBUG
"\
  -m MSR    Read a Model Specific Register\n\
  -n        Display the processor name (Default)\n"
#endif
"\
  -q FEAT   Query if the given feature is available (1)\n\
  -t        Measure the CPU clock frequency using the Time Stamp Counter\n\
  -v        Verbose mode\n\
  -V        Display this program version and exit\n"
#if defined(_WIN32)
"\
  -w PROP   Get a WMI Win32_Processor property\n"
#endif
"\
\n\
(1) FEAT = A short feature name, as defined in Wikipedia page\n\
    https://en.wikipedia.org/wiki/CPUID\n\
    Ex: \"fpu\" or \"pae\"\n\
    Option -f shows the short feature name ahead of each description.\n\
\n\
Author: Jean-Francois Larvoire - jf.larvoire@free.fr\n\
");
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    _rdtsc						      |
|									      |
|   Description	    Read the low DWORD of the time stamp counter	      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    DX:AX = Cycle count 				      |
|                                                                             |
|   Notes	    Warning: The RDTSC instruction generates a GPF when run   |
|		    under Windows in virtual 86 mode.			      |
|		    This is why it is recommended to only call this routine   |
|		    while in 16-bits protected mode.			      |
|                                                                             |
|   History								      |
|    1996/11/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

#define rdtsc DW(0x310F)

#ifdef _MSDOS
DWORD _rdtsc(void) {
   _asm {
	rdtsc
	DATASIZE
	mov	dx, ax		// mov edx, eax
	mov	cl, 16
	DATASIZE
	shr	dx, cl		// shr edx, 16
    }
#pragma warning(disable:4035)	// Ignore the no return value warning
}
#pragma warning(default:4035)
#endif

#if defined(_WIN64)

DWORD _rdtsc(void) {
  return (DWORD)__rdtsc();
}

#elif defined(_WIN32)

DWORD _rdtsc(void) {
   _asm {
	rdtsc
	mov	edx, eax
	mov	cl, 16
	shr	edx, cl
    }
#pragma warning(disable:4035)	// Ignore the no return value warning
}
#pragma warning(default:4035)
#endif

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    _dos_get100th					      |
|									      |
|   Description	    Get the MS-DOS 1/100th of a second relative to 0h00.      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    DX:AX = Current 1/100th of a second relative to 0h00.     |
|                                                                             |
|   History								      |
|    1996/11/20 JFL Created this routine				      |
|    2003/01/21 JFL Rewrote using Microsoft-compatible _dos_getime().	      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
long _dos_get100th(void) {
  long l;
  _dostime_t dostime;

  _dos_gettime(&dostime);
  l = dostime.hour;
  l *= 60;
  l += dostime.minute;
  l *= 60;
  l += dostime.second;
  l *= 100;
  l += dostime.hsecond;

  return l;
}

long getms(void) {
  return _dos_get100th() * 10;
}
#endif // defined(_MSDOS)

#ifdef _WIN32
long getms(void) {
  long l;
  SYSTEMTIME wintime;

  GetSystemTime(&wintime);

  l = wintime.wHour;
  l *= 60;
  l += wintime.wMinute;
  l *= 60;
  l += wintime.wSecond;
  l *= 1000;
  l += wintime.wMilliseconds;

  return l;
}
#endif // defined(_WIN32)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProcessorName					      |
|									      |
|   Description	    Get or build the processor name                           |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    							      |
|                                                                             |
|   History								      |
|    2022-11-11 JFL Extracted this routine from cpuid.c main routine.	      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProcessorName(int iFamily, char *pBuf, int iBufSize) {
  int i;

  if (iBufSize < 49) return 0; /* We need that much for the brand string */
  if (iFamily < 5) {
    sprintf(pBuf, "80%d", (iFamily * 100) + 86);
    /* TODO: Distinguish siblings, like 8086/8088 or 386DX/386SX, etc */
  } else { // if (iFamily >= 5)
    int iModel;
    DWORD dwModel;

    dwModel = _cpuid(1, NULL, NULL, NULL, NULL);

    /* Compute the extended model number */
    iModel = BYTE0(dwModel) >> 4;
    if ((iFamily == 6) || (iFamily == 15)) {
      int iExtModel = BYTE2(dwModel) & 0x0F;
      iModel |= (iExtModel << 4);
    }

    /* On Pentium or better, get the processor brand name from CPUID output */
    if (_cpuid(0x80000000, NULL, NULL, NULL, NULL) >= 0x80000004) {
      DWORD *pdwBrand = (DWORD *)pBuf;
      char *pszName;
      char *pc;

      // CPUID(0x80000002 - 0x80000004) : Get brand string.
      _cpuid(0x80000002, pdwBrand+0, pdwBrand+1, pdwBrand+2, pdwBrand+3);
      _cpuid(0x80000003, pdwBrand+4, pdwBrand+5, pdwBrand+6, pdwBrand+7);
      _cpuid(0x80000004, pdwBrand+8, pdwBrand+9, pdwBrand+10, pdwBrand+11);
      // Skip leading spaces
      for (pszName = pBuf; *pszName == ' '; pszName++) ;
      // Compress multiple spaces into a single space
      for (pc=pszName; *pc; pc++) {
	while ((pc[0]==' ') && (pc[1]==' ')) {
	  char *pc2;
	  for (pc2=pc; (pc2[0]=pc2[1]) != '\0'; pc2++) ;
	}
      }
    } else {
      /* Else compute the processor name from the CPUID family and model numbers */
      for (i=0; i<N_INTEL_PROCS; i++) {
	if (   (IntelProcList[i].iFamily == iFamily)
	    && (IntelProcList[i].iModel == iModel)) {
	  break;
	}
      }
      if (i < N_INTEL_PROCS) {
	strcpy(pBuf, IntelProcList[i].pszName);
      } else {
	char *pszFamily;
	char szFamily[16];
	switch (iFamily) {
	  /* Families < 5 are handled separately above */
	  case 5:
	    pszFamily = "Pentium";
	    break;
	  case 6:
	    pszFamily = "P6";
	    break;
	  case 7:
	    pszFamily = "Itanium";
	    break;
	  case 15:
	    pszFamily = "Pentium 4";
	    break;
	  case 16:
	  case 17:
	    pszFamily = "Itanium 2";
	    break;
	  default:
	    sprintf(szFamily, "Family %d", iFamily);
	    pszFamily = szFamily;
	    break;
	}
	sprintf(pBuf, "%s model %d", pszFamily, iModel);
      }
    }
  }
  return (int)strlen(pBuf);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    MeasureProcSpeed					      |
|									      |
|   Description	    Measure the processor speed                               |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    The speed in MHz					      |
|                                                                             |
|   History								      |
|    2022-11-11 JFL Extracted this routine from cpuid.c main routine.	      |
*									      *
\*---------------------------------------------------------------------------*/

int MeasureProcSpeed() {
  long lt0, lt1;
  DWORD dwt0, dwt1;
#ifdef _MSDOS
  int iErr;
#endif // defined(_MSDOS)

#ifdef _MSDOS
  // Switch to protected mode since the time-stamp counter is NOT
  // accessible in virtual 86 mode.
  if (dpmi_detect() == 0) {
#if _DEBUG
    if (iDebug) printf("Switching to 16-bits PM using DPMI.\n");
#endif
    iErr = dpmi2prot();
    if (iErr) {
      fprintf(stderr, "DPMI error %d switching to protected mode.\n", iErr);
      exit(1);
    }
  } else if (vcpi_detect() == 0) {
#if _DEBUG
    if (iDebug) printf("Switching to 16-bits PM using VCPI.\n");
#endif
    iErr = vcpi2prot();
    if (iErr) {
      fprintf(stderr, "VCPI error %d switching to protected mode.\n", iErr);
      exit(1);
    }
  } else { // No virtual machine control program detected.
#if _DEBUG
    if (iDebug) printf("Staying in real mode.\n");
#endif
  }
#endif // defined(_MSDOS)

#ifdef _MSDOS
  BeginCriticalSection();
#endif // defined(_MSDOS)
  // Wait for the end of the current tick
  lt0 = getms();
  while (lt0 == (lt1 = getms())) ;
  lt0 = lt1;
  dwt0 = _rdtsc();
  // Wait for 1 second
  while ((lt1 = getms()) < (lt0 + 1000)) {}
  dwt1 = _rdtsc();
#ifdef _MSDOS
  EndCriticalSection();
#endif // defined(_MSDOS)
  // Compute frequency
  dwt1 -= dwt0;			// Number of cycles
  lt1 -= lt0;			// Number of 1/1000th of a second
#if _DEBUG
  if (iDebug) printf("Counted %lu cycles in %ld ms\n", dwt1, lt1);
#endif
  if (lt1 < 0) lt1 += 86400000;	// Possible wrap-around at midnight
  lt1 *= 1000;			// Convert to microseconds
  dwt1 += lt1/2;			// Round quotient to the nearest value
  dwt1 /= lt1;			// First frequency evaluation
#if _DEBUG
  if (iDebug) printf("Raw frequency measure: %lu MHz\n", dwt1);
#endif
  // Round to the nearest multiple of 16.66666 = (100/6)
  if (dwt1 > 95) {	// Rule applies only for processors above 100 MHz
    dwt1 *= 6;
    dwt1 += 50;
    dwt1 /= 100;
    dwt1 *= 100;
    dwt1 /= 6;
  }
  return (int)dwt1;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DisplayProcInfo					      |
|									      |
|   Description	    Display detailed processor information, from CPUID output.|
|									      |
|   Parameters	    char *pszQuery	Name of a feature to check	      |
|									      |
|   Returns	    TRUE if the feature was found (Whether enabled or not)    |
|                                                                             |
|   History								      |
|    1998-03-18 JFL Created this routine				      |
|    2009-08-31 JFL Restructured to display both enabled and disabled features|
|		    Added the definitions of numerous AMD extended features.  |
|    2009-09-01 JFL Renamed from DisplayProcId to DisplayProcInfo.            |
|    2022-11-11 JFL Restructured to allow searching for features.             |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

/* CAUTION: Make sure the strings below do not contain any TAB character! */

char *ppszFeatures[32] = { /* Intel Features Flags - EAX=1 -> EDX */
  /* 0x00000001  0 */ "fpu - Integrated FPU",
  /* 0x00000002  1 */ "vme - Enhanced V86 mode",
  /* 0x00000004  2 */ "de - I/O breakpoints",
  /* 0x00000008  3 */ "pse - 4 MB pages",
  /* 0x00000010  4 */ "tsc - Time stamp counter",
  /* 0x00000020  5 */ "msr - Model-specific registers",
  /* 0x00000040  6 */ "pae - Physical address extensions",
  /* 0x00000080  7 */ "mce - Machine-check exception",
  /* 0x00000100  8 */ "cx8 - CMPXCHG8B instruction",
  /* 0x00000200  9 */ "apic - Integrated APIC",
  /* 0x00000400 10 */ "(EDX bit 10 reserved)",
  /* 0x00000800 11 */ "sep - SYSENTER/SYSEXIT instructions",
  /* 0x00001000 12 */ "mttr - MTRR registers, and the MTRR_CAP register",
  /* 0x00002000 13 */ "pge - Page Global Enable bit in CR4",
  /* 0x00004000 14 */ "mca - Machine check architecture",
  /* 0x00008000 15 */ "cmov - CMOV instructions",
  /* 0x00010000 16 */ "pat - Page Attribute table in MTRRs",
  /* 0x00020000 17 */ "pse-36 - 36-bit page size extensions",
  /* 0x00040000 18 */ "psn - Processor Serial Number in CPUID#3",
  /* 0x00080000 19 */ "clfsh - CLFLUSH instruction",
  /* 0x00100000 20 */ "(EDX bit 20 reserved)",
  /* 0x00200000 21 */ "ds - Debug Trace Store & Event Mon.",
  /* 0x00400000 22 */ "acpi - ACPI thermal and clock control registers",
  /* 0x00800000 23 */ "mmx - MMX instructions",
  /* 0x01000000 24 */ "fxsr - FXSAVE and FXRSTOR Instructions",
  /* 0x02000000 25 */ "sse - SSE (Streaming SIMD Extensions)",
  /* 0x04000000 26 */ "sse2 - SSE 2 (Streaming SIMD Extensions v2)",
  /* 0x08000000 27 */ "ss - Self-Snoop memory and caches",
  /* 0x10000000 28 */ "htt - Hyper-threading capable",
  /* 0x20000000 29 */ "tm - Thermal monitoring circuit",
  /* 0x40000000 30 */ "ia64 - IA64 capable",
  /* 0x80000000 31 */ "pbe - Pending Break Enable (PBE# pin) wakeup capability",
};

char *ppszFeatures2[32] = { /* Intel Features Flags - EAX=1 -> ECX */
  /* 0x00000001  0 */ "sse3 - SSE 3 (Streaming SIMD Extensions v3)",
  /* 0x00000002  1 */ "pclmulqdq - PCLMULDQ instruction",
  /* 0x00000004  2 */ "dtes64 - 64-Bit Debug Store",
  /* 0x00000008  3 */ "monitor - MONITOR and MWAIT instructions",
  /* 0x00000010  4 */ "ds-cpl - CPL Qualified Debug Store",
  /* 0x00000020  5 */ "vmx - VMX (Virtual Machine Extensions)",
  /* 0x00000040  6 */ "smx - Safer Mode Extensions (Trusted Execution)",
  /* 0x00000080  7 */ "est - Enhanced SpeedStep Technology",
  /* 0x00000100  8 */ "tm2 - Thermal Monitor 2 Control Circuit",
  /* 0x00000200  9 */ "ssse3 - SSSE 3 (Suplemental Streaming SIMD Extensions v3)",
  /* 0x00000400 10 */ "cnxt-id - L1 data cache Context ID",
  /* 0x00000800 11 */ "sdbg - SDBG (Silicon Debug interface)",
  /* 0x00001000 12 */ "fma - Fused Multiply Add extensions",
  /* 0x00002000 13 */ "cx16 - CMPXCHG16B instruction",
  /* 0x00004000 14 */ "xtpr - Send Task Priority Messages update control",
  /* 0x00008000 15 */ "pdcm - Perfmon and Debug Capability",
  /* 0x00010000 16 */ "(ECX bit 16 reserved)",
  /* 0x00020000 17 */ "pcid - Process Context Identifiers (CR4 bit 17)",
  /* 0x00040000 18 */ "dca - Direct Cache Access for DMA writes",
  /* 0x00080000 19 */ "sse4.1 - SSE 4.1 (Streaming SIMD Extensions 4.1)",
  /* 0x00100000 20 */ "sse4.2 - SSE 4.2 (Streaming SIMD Extensions 4.2)",
  /* 0x00200000 21 */ "x2apic - Extended xAPIC Support",
  /* 0x00400000 22 */ "movbe - MOVBE Instruction",
  /* 0x00800000 23 */ "popcnt - POPCNT Instruction",
  /* 0x01000000 24 */ "tsc-deadline - Timestamp Counter Deadline",
  /* 0x02000000 25 */ "aes - AES instruction",
  /* 0x04000000 26 */ "xsave - XSAVE/XRESTOR instructions",
  /* 0x08000000 27 */ "osxsave - OS-Enabled SXAVE/XRESTOR Management",
  /* 0x10000000 28 */ "Aavx - VX (Advanced Vector eXtensions)",
  /* 0x20000000 29 */ "f16c - 16-bit Floating Point Conversion instructions",
  /* 0x40000000 30 */ "rdrnd - RDRAND instruction",
  /* 0x80000000 31 */ "hypervisor - Hypervisor present (always zero on physical CPUs)",
};

char *ppszFeatures70b[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> EBX */
  /* 0x00000001  0 */ "fsgsbase - FSGSBASE instructions (RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE)",
  /* 0x00000002  1 */ "IA32_TSC_ADJUST MSR is supported",
  /* 0x00000004  2 */ "sgx - SGX (Software Guard Extensions)",
  /* 0x00000008  3 */ "bmi1 - BMI1 (Bit Manipulation Instruction Set 1)",
  /* 0x00000010  4 */ "hle - HLE (Hardware Lock Elision)",
  /* 0x00000020  5 */ "avx2 - AVX2 (Advanced Vector Extensions 2)",
  /* 0x00000040  6 */ "x87 FPU Data Pointer updated only on x87 exceptions",
  /* 0x00000080  7 */ "smep - SMEP (Supervisor-Mode Execution Prevention)",
  /* 0x00000100  8 */ "bmi2 - BMI2 (Bit Manipulation Instruction Set 2)",
  /* 0x00000200  9 */ "erms - Enhanced REP MOVSB/STOSB",
  /* 0x00000400 10 */ "invpcid - INVPCID instruction",
  /* 0x00000800 11 */ "rtm - RTM (Restricted Transactional Memory) instructions",
  /* 0x00001000 12 */ "rdt-m - RDT-M (Resource Director Technology Monitoring)",
  /* 0x00002000 13 */ "FPU CS and DS values deprecated",
  /* 0x00004000 14 */ "mpx - MPX (Memory Protection Extensions)",
  /* 0x00008000 15 */ "rdt-a - RDT-A (Resource Director Technology Allocation)",
  /* 0x00010000 16 */ "avx512-f - AVX-512 Foundation Instructions",
  /* 0x00020000 17 */ "avx512-dq - AVX-512 Doubleword and Quadword Instructions",
  /* 0x00040000 18 */ "rdseed - RDSEED instruction",
  /* 0x00080000 19 */ "adx - ADX (Multi-Precision Add-Carry Instruction Extensions)",
  /* 0x00100000 20 */ "smap - SMAP (Supervisor-Mode Access Prevention) instructions",
  /* 0x00200000 21 */ "avx512-ifma - AVX-512 Integer Fused Multiply-Add Instructions",
  /* 0x00400000 22 */ "pcommit - PCOMMIT (Persistent Memory Commit) instruction", // Deprecated: https://software.intel.com/en-us/blogs/2016/09/12/deprecate-pcommit-instruction
  /* 0x00800000 23 */ "clflushopt - CLFLUSHOPT Instruction",
  /* 0x01000000 24 */ "clwb - CLWB (Cache Line Write Back) instruction",
  /* 0x02000000 25 */ "pt - Intel Processor Trace",
  /* 0x04000000 26 */ "avx512-pf - AVX-512 Prefetch Instructions",
  /* 0x08000000 27 */ "avx512-er - AVX-512 Exponential and Reciprocal Instructions",
  /* 0x10000000 28 */ "avx512-cd - AVX-512 Conflict Detection Instructions",
  /* 0x20000000 29 */ "sha - SHA (Secure Hash Algorithm Extensions)",
  /* 0x40000000 30 */ "avx512-bw - AVX-512 Byte and Word Instructions",
  /* 0x80000000 31 */ "avx512-vl - AVX-512 Vector Length Extensions",
};

char *ppszFeatures70c[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> ECX */
  /* 0x00000001  0 */ "prefetchwt1 - PREFETCHWT1 instruction",
  /* 0x00000002  1 */ "avx512-vbmi - AVX-512 Vector Bit Manipulation Instructions",
  /* 0x00000004  2 */ "umip - User-mode Instruction Prevention",
  /* 0x00000008  3 */ "pku - PKU (Memory Protection Keys for User-mode pages)",
  /* 0x00000010  4 */ "ospke - PKU enabled by OS",
  /* 0x00000020  5 */ "waitpkg - WAITPKG (UMWAIT instruction)",
  /* 0x00000040  6 */ "avx512-vbmi2 - AVX-512 Vector Bit Manipulation Instructions 2",
  /* 0x00000080  7 */ "cet_ss - Control flow enforcement (CET) shadow stack instructions",
  /* 0x00000100  8 */ "gnfi - GFNI (Galois Field instructions)",
  /* 0x00000200  9 */ "vaes - VAES (Vector AES instruction set (VEX-256/EVEX))",
  /* 0x00000400 10 */ "vpclmulqdq - CLMUL instruction set (VEX-256/EVEX)",
  /* 0x00000800 11 */ "avx512-vnni - AVX-512 Vector Neural Network Instructions",
  /* 0x00001000 12 */ "avx512-bitalg - AVX-512 BITALG instructions",
  /* 0x00002000 13 */ "tme - IA32_TME related MSRs",
  /* 0x00004000 14 */ "avx512-vpopcntdq - AVX-512 Vector Population Count Double and Quad-word",
  /* 0x00008000 15 */ "(ECX bit 15 reserved)",
  /* 0x00010000 16 */ "la57 - 5-level paging",
  /* 0x00020000 17 */ "mawau - MPX Address-Width Adjust bit 0",
  /* 0x00040000 18 */ "mawau - MPX Address-Width Adjust bit 1",
  /* 0x00080000 19 */ "mawau - MPX Address-Width Adjust bit 2",
  /* 0x00100000 20 */ "mawau - MPX Address-Width Adjust bit 3",
  /* 0x00200000 21 */ "mawau - MPX Address-Width Adjust bit 4",
  /* 0x00400000 22 */ "rdpid - RDPID (Read Processor ID) instruction",
  /* 0x00800000 23 */ "kl - Key Locker",
  /* 0x01000000 24 */ "BUS_LOCK_DETECT",
  /* 0x02000000 25 */ "cldemote - CLDEMOTE (Cache Line Demote) instruction",
  /* 0x04000000 26 */ "(ECX bit 26 reserved)",
  /* 0x08000000 27 */ "movdiri - MOVDIR (Direct Store) instructions",
  /* 0x10000000 28 */ "movdir64b - MOVDIR64B (Direct Store) instructions",
  /* 0x20000000 29 */ "enqcmd - Enqueue Stores",
  /* 0x40000000 30 */ "sgx-lc - SGX Launch Configuration instructions",
  /* 0x80000000 31 */ "pks - Protection keys for supervisor-mode pages",
};

char *ppszFeatures70d[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> EDX */
  /* 0x00000001  0 */ "(EDX bit 0 reserved)",
  /* 0x00000002  1 */ "(EDX bit 1 reserved)",
  /* 0x00000004  2 */ "avx512-4vnniw - AVX-512 4-register Neural Network instructions",
  /* 0x00000008  3 */ "avx512-4fmaps - AVX-512 4-register Multiply Accumulation Single precision",
  /* 0x00000010  4 */ "fsrm - FSRM (Fast Short REP MOVSB)",
  /* 0x00000020  5 */ "uintr - User Inter-processor Interrupts",
  /* 0x00000040  6 */ "(EDX bit 6 reserved)",
  /* 0x00000080  7 */ "(EDX bit 7 reserved)",
  /* 0x00000100  8 */ "avx512-vp2intersect - AVX-512 VP2INTERSECT Doubleword and Quadword Instructions",
  /* 0x00000200  9 */ "srdbs-ctrl - Special Register Buffer Data Sampling Mitigations",
  /* 0x00000400 10 */ "mc-clear - VERW instruction clears CPU buffers",
  /* 0x00000800 11 */ "rtm-always-abort - All TSX transactions are aborted",
  /* 0x00001000 12 */ "(EDX bit 12 reserved)",
  /* 0x00002000 13 */ "TSX_FORCE_ABORT MSR is available",
  /* 0x00004000 14 */ "serialize - SERIALIZE instruction",
  /* 0x00008000 15 */ "hybrid - Mixture of CPU types in processor topology",
  /* 0x00010000 16 */ "tsxldtrk - TSXLDTRK instruction",
  /* 0x00020000 17 */ "(EDX bit 17 reserved)",
  /* 0x00040000 18 */ "pconfig - PCONFIG Platform Configuration (Memory Encryption)",
  /* 0x00080000 19 */ "lbr - Architectural Last Branch Records",
  /* 0x00100000 20 */ "cet-ibt - Control flow enforcement (CET) indirect branch tracking",
  /* 0x00200000 21 */ "(EDX bit 21 reserved)",
  /* 0x00400000 22 */ "amx-bf16 - Tile computation on bfloat16 numbers",
  /* 0x00800000 23 */ "avx512-fp16 - AVX512-FP16 half-precision floating-point instructions",
  /* 0x01000000 24 */ "amx-tile - Tile architecture",
  /* 0x02000000 25 */ "amx-int8 - Tile computation on 8-bit integers",
  /* 0x04000000 26 */ "spec_ctrl - IBRS_IBPB (Indirect Branch Restricted Speculation)",
  /* 0x08000000 27 */ "stibp - STIBP (Single Thread Indirect Branch Predictor)",
  /* 0x10000000 28 */ "l1d_flush - IA32_FLUSH_CMD MSR",
  /* 0x20000000 29 */ "IA32_ARCH_CAPABILITIES Speculative Side Channel Mitigations",
  /* 0x40000000 30 */ "IA32_CORE_CAPABILITIES MSR (lists model-specific core capabilities)",
  /* 0x80000000 31 */ "ssbd - SSBD (Speculative Store Bypass Disable)",
};

char *ppszFeatures71a[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=1 -> EAX */
  /* 0x00000001  0 */ "",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "rao-int - RAO-INT instructions",
  /* 0x00000010  4 */ "avx-vnni - AVX Vector Neural Network Instructions",
  /* 0x00000020  5 */ "avx512-bf16 - AVX-512 BFLOAT16 instructions",
  /* 0x00000040  6 */ "",
  /* 0x00000080  7 */ "cmpccxadd - CMPccXADD instructions",
  /* 0x00000100  8 */ "archperfmonext - Architectural Performance Monitoring Extended Leaf (EAX=23h)",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "fast_zero_rep_movsb - Fast zero-length MOVSB",
  /* 0x00000800 11 */ "fast_short_rep_stosb - Fast zero-length STOSB",
  /* 0x00001000 12 */ "fast_short_rep_cmpsb_scasb - Fast zero-length CMPSB and SCASB",
  /* 0x00002000 13 */ "",
  /* 0x00004000 14 */ "",
  /* 0x00008000 15 */ "",
  /* 0x00010000 16 */ "",
  /* 0x00020000 17 */ "fred - Flexible Return and Event Delivery",
  /* 0x00040000 18 */ "lkgs - LKGS Instruction",
  /* 0x00080000 19 */ "wrmsrns - WRMSRNS instruction",
  /* 0x00100000 20 */ "",
  /* 0x00200000 21 */ "amx-fp16 - AMX instructions for FP16 numbers",
  /* 0x00400000 22 */ "hreset - HRESET instruction and management system",
  /* 0x00800000 23 */ "avx-ifma - AVX IFMA instructions",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "lam - Linear Address Masking",
  /* 0x08000000 27 */ "msrlist - RDMSRLIST and WRMSRLIST instructions and msr",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "",
};

char *ppszFeatures71b[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=1 -> EBX */
  /* 0x00000001  0 */ "IA32_PPIN and IA32_PPIN_CTL MSRs",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "",
  /* 0x00000020  5 */ "",
  /* 0x00000040  6 */ "",
  /* 0x00000080  7 */ "",
  /* 0x00000100  8 */ "",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "",
  /* 0x00000800 11 */ "",
  /* 0x00001000 12 */ "",
  /* 0x00002000 13 */ "",
  /* 0x00004000 14 */ "",
  /* 0x00008000 15 */ "",
  /* 0x00010000 16 */ "",
  /* 0x00020000 17 */ "",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "",
  /* 0x00100000 20 */ "",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "",
  /* 0x00800000 23 */ "",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "",
  /* 0x08000000 27 */ "",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "",
};

char *ppszFeatures71c[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=1 -> ECX */
  /* 0x00000001  0 */ "",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "",
  /* 0x00000020  5 */ "",
  /* 0x00000040  6 */ "",
  /* 0x00000080  7 */ "",
  /* 0x00000100  8 */ "",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "",
  /* 0x00000800 11 */ "",
  /* 0x00001000 12 */ "",
  /* 0x00002000 13 */ "",
  /* 0x00004000 14 */ "",
  /* 0x00008000 15 */ "",
  /* 0x00010000 16 */ "",
  /* 0x00020000 17 */ "",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "",
  /* 0x00100000 20 */ "",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "",
  /* 0x00800000 23 */ "",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "",
  /* 0x08000000 27 */ "",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "",
};

char *ppszFeatures71d[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=1 -> EDX */
  /* 0x00000001  0 */ "",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "avx-vnn-int8 - AVX VNNI INT8 instructions",
  /* 0x00000020  5 */ "avx-ne-convert - AVX NE CONVERT instructions",
  /* 0x00000040  6 */ "",
  /* 0x00000080  7 */ "",
  /* 0x00000100  8 */ "",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "",
  /* 0x00000800 11 */ "",
  /* 0x00001000 12 */ "",
  /* 0x00002000 13 */ "",
  /* 0x00004000 14 */ "prefetchiti - PREFETCHIT0 and PREFETCHIT1 instructions",
  /* 0x00008000 15 */ "",
  /* 0x00010000 16 */ "",
  /* 0x00020000 17 */ "",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "",
  /* 0x00100000 20 */ "",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "",
  /* 0x00800000 23 */ "",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "",
  /* 0x08000000 27 */ "",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "",
};

char *ppszExtFeatures[32] = { /* AMD Extended Features Flags - EDX */
  /* Unknown bits, with a "" definition, will not be displayed. */
  /* Flags that are just a copy of the corresponding Intel Features Flag are commented-out */
  /* 0x00000001  0 */ "", // "fpu - Integrated FPU",
  /* 0x00000002  1 */ "", // "vme - Enhanced V86 mode",
  /* 0x00000004  2 */ "", // "de - I/O breakpoints",
  /* 0x00000008  3 */ "", // "pse - 4 MB pages",
  /* 0x00000010  4 */ "", // "tsc - Time stamp counter",
  /* 0x00000020  5 */ "", // "msr - Model-specific registers",
  /* 0x00000040  6 */ "", // "pae - Physical address extensions",
  /* 0x00000080  7 */ "", // "mce - Machine-check exception",
  /* 0x00000100  8 */ "", // "cx8 - CMPXCHG8B instruction",
  /* 0x00000200  9 */ "", // "apic - Integrated APIC",
  /* 0x00000400 10 */ "", // "(EDX bit 10 reserved)",
  /* 0x00000800 11 */ "syscall - SYSCALL and SYSRET instructions",
  /* 0x00001000 12 */ "", // "mttr - MTRR registers, and the MTRR_CAP register",
  /* 0x00002000 13 */ "", // "pge - Page Global Enable bit in CR4",
  /* 0x00004000 14 */ "", // "mca - Machine check architecture",
  /* 0x00008000 15 */ "", // "cmov - CMOV instructions",
  /* 0x00010000 16 */ "", // "pat - Page Attribute table in MTRRs",
  /* 0x00020000 17 */ "", // "pse-36 - 36-bit page size extensions",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "mp - Multiprocessor Capable",
  /* 0x00100000 20 */ "nx - Execution disable bit",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "mmxext - AMD extensions to MMX",
  /* 0x00800000 23 */ "", // "mmx - MMX instructions",
  /* 0x01000000 24 */ "", // "fxsr - FXSAVE and FXRSTOR Instructions",
  /* 0x02000000 25 */ "fxsr_opt - FXSAVE/FXRSTOR optimizations",
  /* 0x04000000 26 */ "pdpe1gb - Gigabyte pages",
  /* 0x08000000 27 */ "rdtscp - RDTSCP instruction",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "lm - 64 bit instructions (=long mode/EM64T/x86_64)",
  /* 0x40000000 30 */ "3dnowext - AMD extensions to 3DNow!",
  /* 0x80000000 31 */ "3dnow - 3DNow! instructions",
};

char *ppszExtFeatures2[32] = { /* AMD Extended Features Flags - ECX */
  /* 0x00000001  0 */ "lahf_lm - LAHF and SAHF in 64-bits mode",
  /* 0x00000002  1 */ "cmp_legacy - Hyperthreading not valid",
  /* 0x00000004  2 */ "svm - Secure Virtual Machine instructions",
  /* 0x00000008  3 */ "extapic - Extended APIC space",
  /* 0x00000010  4 */ "cr8_legacy - Use of LOCK prefix to read CR8 in 32-bit mode",
  /* 0x00000020  5 */ "abm - Advanced bit manipulation (lzcnt and popcnt instructions)",
  /* 0x00000040  6 */ "sse4a - SSE4A Instructions",
  /* 0x00000080  7 */ "misalignsse - Misaligned SSE mode",
  /* 0x00000100  8 */ "3dnowprefetch - 3DNow! PREFETCH/PREFETCHW instructions",
  /* 0x00000200  9 */ "osvw - OS Visible Workaround",
  /* 0x00000400 10 */ "ibs - Instruction Based Sampling",
  /* 0x00000800 11 */ "xop - XOP instruction set",
  /* 0x00001000 12 */ "skinit - SKINIT/STGI instructions",
  /* 0x00002000 13 */ "wdt - Watchdog timer",
  /* 0x00004000 14 */ "",
  /* 0x00008000 15 */ "lwp - Light Weight Profiling",
  /* 0x00010000 16 */ "fma4 - 4 operands fused multiply-add",
  /* 0x00020000 17 */ "tce - Translation Cache Extension",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "nodeid_msr - NodeID MSR",
  /* 0x00100000 20 */ "",
  /* 0x00200000 21 */ "tbm - Trailing Bit Manipulation",
  /* 0x00400000 22 */ "topoext - Topology Extensions",
  /* 0x00800000 23 */ "perfctr_core - Core performance counter extensions",
  /* 0x01000000 24 */ "perfctr_nb - NB performance counter extensions",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "dbx - Data breakpoint extensions",
  /* 0x08000000 27 */ "perftsc - Performance TSC",
  /* 0x10000000 28 */ "pcx_l2i - L2I perf counter extensions",
  /* 0x20000000 29 */ "monitorx - MONITORX and MWAITX instructions",
  /* 0x40000000 30 */ "addr_mask_ext - ?",
  /* 0x80000000 31 */ "",
};

char *YesNo(unsigned long n) {
  if (n != 0)
    return "Yes";
  else
    return "No";
}

int ReportFeatures(char *pszRegName, DWORD dwValue, char *ppszFeatureNames[32], char *pszQuery) {
  int i;
  size_t nQueryLen = 0;

  if (!(dwValue || pszQuery)) return FALSE; /* Speed things up is no bit is set */

  if (pszQuery) nQueryLen = strlen(pszQuery);

  for (i = 0; i < 32; i++) {
    char *pszName = ppszFeatureNames[i];
    DWORD dwMask = (DWORD)1 << i;
    if (pszQuery) {
      char *pszSpace = strchr(pszName, ' ');
      size_t nLen = (int)(pszSpace ? (pszSpace - pszName) : strlen(pszName));
      if ((nLen == nQueryLen) && !strncmp(pszQuery, pszName, nLen)) {
	printf("%-3s %s\n", YesNo(dwValue & dwMask), pszName);
        return TRUE;
      } else {
      	pszName = "";
      }
    }
    if (pszName[0]) {
      printf(" %s %2d %-3s %s\n", pszRegName, i, YesNo(dwValue & dwMask), pszName);
    }
  }
  if (!pszQuery) printf("\n");

  return FALSE;
}

int DisplayProcInfo(char *pszQuery) {
  DWORD dwMaxValue;
  DWORD dwMaxValueX;
  DWORD dwModel;
  DWORD dwModel2;
  DWORD dwFeatures;
  DWORD dwFeatures2;
  DWORD dwFeatures3;
  DWORD dwFeatures4;
  char szName[14];
  int iFamily;
  int iModel;
  int i;
  int nCores;

  // CPUID(0) :
  _cpuid(0, &dwMaxValue, (DWORD *)(szName+0), (DWORD *)(szName+8), (DWORD *)(szName+4));
  szName[12] = '\0';
  if (!pszQuery) printf("%s", szName);

  if (dwMaxValue >= 1) {
    // CPUID(1) : Request the Family/Model/Step
    _cpuid(1, &dwModel, &dwModel2, &dwFeatures2, &dwFeatures);
    if (!pszQuery) {
      iFamily = BYTE1(dwModel) & 0x0F;
      if (iFamily == 0x0F) {
	int iExtFamily = (WORD1(dwModel) >> 4) & 0xFF;
	iFamily += iExtFamily;
      }
      printf(" Family %d", iFamily);
      iModel = BYTE0(dwModel) >> 4;
      if ((iFamily == 6) || (iFamily == 15)) {
	int iExtModel = BYTE2(dwModel) & 0x0F;
	iModel |= (iExtModel << 4);
      }
      printf(" Model %d", iModel);
      printf(" Stepping %d", BYTE0(dwModel) & 0x0F);
      for (i=0; i<N_INTEL_PROCS; i++) {
	if (   (IntelProcList[i].iFamily == iFamily)
	    && (IntelProcList[i].iModel == iModel)) {
	  printf(": %s \"%s\"", IntelProcList[i].pszName,
				IntelProcList[i].pszCodeName);
	  break;
	}
      }
      printf("\n\n");
    }

    // CPUID(0x80000000) : Get max extended function supported.
    if (!pszQuery) printf("Max base function: 0x%08lX\n", dwMaxValue);
    dwMaxValueX = _cpuid(0x80000000, NULL, NULL, NULL, NULL);
    if (!pszQuery) {
      if (dwMaxValueX >= 0x80000000)
	printf("Max extended function: 0x%08lX\n", dwMaxValueX);
      else
	printf("No extended CPUID functions.\n");
      printf("\n");
    }

    /* Intel Feature Flags */
    if (!pszQuery) printf("CPUID(1): Intel Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures, dwFeatures2);
    if (ReportFeatures("EDX", dwFeatures, ppszFeatures, pszQuery)) return TRUE;
    if (ReportFeatures("ECX", dwFeatures2, ppszFeatures2, pszQuery)) return TRUE;

    /* AMD Extended Features Flags */
    if (dwMaxValueX >= 0x80000001) {
      /* Only display those that are documented in recent Intel's manuals */
      // CPUID(0x80000001) : Get extended feature flags.
      _cpuid(0x80000001, NULL, NULL, &dwFeatures4, &dwFeatures3);
      if (!pszQuery) printf("CPUID(0x80000001): AMD Extended Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures3, dwFeatures4);
      if (ReportFeatures("EDX", dwFeatures3, ppszExtFeatures, pszQuery)) return TRUE;
      if (ReportFeatures("ECX", dwFeatures4, ppszExtFeatures2, pszQuery)) return TRUE;
    }

    /* Structured Extended Feature Flags */
    if (dwMaxValue >= 7) {
      DWORD dwEAX, dwEBX, dwECX, dwEDX;
      int nSubLeaves;

      dwECX = 0;
      _cpuid(7, &dwEAX, &dwEBX, &dwECX, &dwEDX);
      if (!pszQuery) printf("CPUID(7, 0): Extended Features Flags: EAX=0x%08lX EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX\n", dwEAX, dwEBX, dwECX, dwEDX);
      nSubLeaves = (int)dwEAX + 1;
      if (!pszQuery) printf(" EAX        Max sub-leave = %ld\n\n", dwEAX);
      if (ReportFeatures("EBX", dwEBX, ppszFeatures70b, pszQuery)) return TRUE;
      if (ReportFeatures("ECX", dwECX, ppszFeatures70c, pszQuery)) return TRUE;
      if (ReportFeatures("EDX", dwEDX, ppszFeatures70d, pszQuery)) return TRUE;

      if (nSubLeaves > 1) {
	dwECX = 1;
	_cpuid(7, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	if (!pszQuery) printf("CPUID(7, 1): Extended Features Flags: EAX=0x%08lX EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX\n", dwEAX, dwEBX, dwECX, dwEDX);
	if (ReportFeatures("EAX", dwEAX, ppszFeatures71a, pszQuery)) return TRUE;
	if (ReportFeatures("EBX", dwEBX, ppszFeatures71b, pszQuery)) return TRUE;
	if (ReportFeatures("ECX", dwECX, ppszFeatures71c, pszQuery)) return TRUE;
	if (ReportFeatures("EDX", dwEDX, ppszFeatures71d, pszQuery)) return TRUE;
      }

      if (nSubLeaves > 2) {
	fprintf(stderr, "Warning: There are %d sub-leaves, so there are more registers to decode\n\n", nSubLeaves);
      }
    }

    if (pszQuery) {
      fprintf(stderr, "Unknown feature: %s\n", pszQuery);
      return(FALSE);
    }

    /* Brand string */
    if (dwMaxValueX >= 0x80000004) {
      char szBrand[48];
      DWORD *pdwBrand = (DWORD *)szBrand;
      char *pszBrand = szBrand;

      // CPUID(0x80000002 - 0x80000004) : Get brand string.
      _cpuid(0x80000002, pdwBrand+0, pdwBrand+1, pdwBrand+2, pdwBrand+3);
      _cpuid(0x80000003, pdwBrand+4, pdwBrand+5, pdwBrand+6, pdwBrand+7);
      _cpuid(0x80000004, pdwBrand+8, pdwBrand+9, pdwBrand+10, pdwBrand+11);
      while (*pszBrand == ' ') pszBrand++; // Skip head spaces, if any
      printf("Brand string: \"%s\"\n", pszBrand);
      printf("\n");
    }

    /* Virtual and Physical address Sizes */
    if (dwMaxValueX >= 0x80000008) {
      DWORD dwInfo;

      _cpuid(0x80000008, &dwInfo, NULL, NULL, NULL);
      printf("Physical Address Size: %d bits\n", BYTE0(dwInfo));
      printf("Virtual Address Size: %d bits\n", BYTE1(dwInfo));
      printf("\n");
    }

    /* Number of cores and threads */
    printf("Cores and threads\n");
    nCores = 1;
    if (dwFeatures & (1L << 28)) nCores = (int)BYTE2(dwModel2);
    printf(" CPUID(1):  Silicon supports %d logical processors\n", nCores);
    if (dwMaxValue >= 4) {
      DWORD dwEAX, dwEBX, dwECX, dwEDX;
      int nMaxCores, nMaxThreads;

      dwECX = 0;
      _cpuid(4, &dwEAX, &dwEBX, &dwECX, &dwEDX);
      nMaxCores = (int)((dwEAX >> 26) & 0x3F) + 1;
      nMaxThreads = (int)((dwEAX >> 14) & 0xFFF) + 1;
      printf(" CPUID(4):  Silicon supports %d cores and %d threads/core\n", nMaxCores, nMaxThreads);
    }
    if (dwMaxValue >= 0x0B) {
      int iFunction = (dwMaxValue >= 0x1F) ? 0x1F : 0x0B; /* Function 0x1F is the preferred replacement, if available */
      int iLevel = 0;
      DWORD dwEAX, dwEBX, dwECX, dwEDX;
      int nLogicalProcs;
      int iType;
      char *pszType;

      for (iLevel = 0; ; iLevel++) { /* Loop untested with any level > 1 */
	dwECX = iLevel;
	_cpuid(iFunction, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	iType = (int)BYTE1(dwECX);
	if (!iType) break; /* Enumeration complete */
	nLogicalProcs = WORD0(dwEBX);
	if (!nLogicalProcs) break; /* Some CPU models set dwMaxValue >= 11, yet return 0 here, and generate an exception if any further call is made.
				      (But we were not testing iType at that time.) */
	switch (iType) {
	  case 1: pszType = "SMT (Simultaneous MultiThreading)"; break;
	  case 2: pszType = "Core"; break;
	  case 3: pszType = "Module"; break;
	  case 4: pszType = "Tile"; break;
	  case 5: pszType = "Die"; break;
	  default: pszType = "Unknown type"; break;
	}
	printf(" CPUID(0x%X, %d): %d logical processors at %s level\n",
	       iFunction, iLevel, nLogicalProcs, pszType);
	/* The APIC ID returned in EDX varies from one execution to the next,
	   as the process is executed on a random logical processor. */
	/* We _might_ have to loop using instead: iLevel = dwEDX >> (dwEAX & 0x1F) */
      }
    }
  }

  return(FALSE);
}

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    _cpuid						      |
|									      |
|   Description	    Execute a CPUID instruction, and collect results	      |
|									      |
|   Parameters	    DWORD dwId	    ID index (input)			      |
|		    DWORD *pEax     Where to store EAX. Discard EAX if NULL.  |
|		    DWORD *pEbx     Where to store EBX. Discard EBX if NULL.  |
|		    DWORD *pEcx     Where to store ECX. Discard ECX if NULL.  |
|		    DWORD *pEdx     Where to store EDX. Discard EDX if NULL.  |
|									      |
|   Returns	    DX:AX = EAX 					      |
|                                                                             |
|   Notes								      |
|                                                                             |
|   History								      |
|    2000-03-23 JFL Created this routine				      |
|    2005-04-28 JFL Changed the 1st argument type from int to DWORD.          |
|    2013-01-31 JFL Added the ability to pass ECX as an input.                |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

#ifdef _MSDOS
#define cpuid DW(0xA20F)

DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx) {
   DWORD dwRet;

   _asm {
	; Some advanced CPUID functions use ecx as an input
	mov	bx, pEcx
	test	bx, bx
	jz	no_ecx_in
	DATASIZE
	mov	cx, [bx]
no_ecx_in:

	DATASIZE
	mov	ax, word ptr dwId

	cpuid

	DATASIZE
	mov	word ptr dwRet, eax

	; Return EBX
	DATASIZE
	mov	ax, bx
	mov	bx, pEbx
	test	bx, bx
	jz	no_ebx
	DATASIZE
	mov	[bx], eax
no_ebx:

	; Return EAX
	DATASIZE
	mov	eax, word ptr dwRet
	mov	bx, pEax
	test	bx, bx
	jz	no_eax
	DATASIZE
	mov	[bx], eax
no_eax:

	; Return ECX
	mov	bx, pEcx
	test	bx, bx
	jz	no_ecx
	DATASIZE
	mov	[bx], ecx
no_ecx:

	; Return EDX
	mov	bx, pEdx
	test	bx, bx
	jz	no_edx
	DATASIZE
	mov	[bx], edx
no_edx:
    }

    return dwRet;
}
#endif // defined(_MSDOS)

#if defined(_WIN64)

DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx) {
  int cpuInfo[4];
  cpuInfo[0] = pEax ? (int)*pEax : 0;
  cpuInfo[1] = pEbx ? (int)*pEbx : 0;
  cpuInfo[2] = pEcx ? (int)*pEcx : 0;
  cpuInfo[3] = pEdx ? (int)*pEdx : 0;
  __cpuidex(cpuInfo, (int)dwId, (int)cpuInfo[2]);
  if (pEax) *pEax = (DWORD)(cpuInfo[0]);
  if (pEbx) *pEbx = (DWORD)(cpuInfo[1]);
  if (pEcx) *pEcx = (DWORD)(cpuInfo[2]);
  if (pEdx) *pEdx = (DWORD)(cpuInfo[3]);
  return (DWORD)(cpuInfo[0]);
}

#elif defined(_WIN32)

DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx) {
   DWORD dwRet;

   _asm {
	; Some advanced CPUID functions use ecx as an input
	mov	ebx, dword ptr pEcx
	test	ebx, ebx
	jz	no_ecx_in
	mov	ecx, [ebx]
no_ecx_in:

	mov	eax, dword ptr dwId

	cpuid

	mov	dwRet, eax

	; Return EBX
	mov	eax, ebx
	mov	ebx, pEbx
	test	ebx, ebx
	jz	no_ebx
	mov	[ebx], eax
no_ebx:

	; Return EAX
	mov	eax, dwRet
	mov	ebx, pEax
	test	ebx, ebx
	jz	no_eax
	mov	[ebx], eax
no_eax:

	; Return ECX
	mov	ebx, pEcx
	test	ebx, ebx
	jz	no_ecx
	mov	[ebx], ecx
no_ecx:

	; Return EDX
	mov	ebx, pEdx
	test	ebx, ebx
	jz	no_edx
	mov	[ebx], edx
no_edx:
    }

    return dwRet;
}
#endif // defined(_WIN32)

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

#if _DEBUG

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    _rdmsr						      |
|									      |
|   Description	    Execute a RDMSR instruction, and collect results	      |
|									      |
|   Parameters	    DWORD dwECX		MSR index (input)		      |
|		    DWORD pdwMSR[2]	Where to store EDX:EAX		      |
|									      |
|   Returns	    Nothing	 					      |
|                                                                             |
|   Notes								      |
|                                                                             |
|   History								      |
|    2020-02-24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

#ifdef _MSDOS
#define rdmsr DW(0x320F)

void _rdmsr(DWORD dwECX, DWORD pdwMSR[2]) {
   _asm {
	DATASIZE
	mov	cx, word ptr dwECX
	rdmsr
	mov	bx, pdwMSR
	DATASIZE
	mov	[bx], ax
	DATASIZE
	mov	[bx+4], dx
    }
    return;
}
#endif // defined(_MSDOS)

#if defined(_WIN64)

void _rdmsr(DWORD dwECX, DWORD pdwMSR[2]) {
  *(__int64 *)pdwMSR = __readmsr((int)dwECX);
}

#elif defined(_WIN32)

void _rdmsr(DWORD dwECX, DWORD pdwMSR[2]) {
   _asm {
	mov	ecx, dwECX
	rdmsr			; 0F 32
	mov	ebx, pdwMSR
	mov	[ebx], eax
	mov	[ebx+4], edx
    }
    return;
}
#endif // defined(_WIN32)

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

#endif /* _DEBUG */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    identify_processor					      |
|									      |
|   Description	    Tell which generation of processor we're running on	      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    Processor generation:			              |
|                        0 = 8086					      |
|                        1 = 80186					      |
|                        2 = 80286					      |
|                        3 = 80386					      |
|                        4 = 80486					      |
|                        5 = Pentium					      |
|                        6 = Pentium Pro, P2, P3, and all later Core procs.   |
|                        7 = Itanium					      |
|                        15 = Pentium 4					      |
|                        16 = Itanium 2 early steppings			      |
|                        17 = Itanium 2	later steppings			      |
|                                                                             |
|   Notes	    Assume that under Win32, the processor is a Pentium	      |
|                   or better, so no need to test for earlier versions.       |
|                                                                             |
|   History								      |
|    2010-09-06 JFL Created this Win32 version.				      |
|    2022-11-10 JFL Fixed the extended family calculation.	              |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32
int identify_processor(void) {
  DWORD dwModel;
  int iFamily;

  dwModel = _cpuid(1, NULL, NULL, NULL, NULL);

  /* Compute the extended family number */
  iFamily = BYTE1(dwModel) & 0x0F;
  if (iFamily == 0x0F) {
    int iExtFamily = (WORD1(dwModel) >> 4) & 0xFF;
    iFamily += iExtFamily;
  }

  return iFamily;
}
#endif // defined(_WIN32)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetWmiProcInfo					      |
|									      |
|   Description	    Get processor information from WMI			      |
|									      |
|   Parameters	    							      |
|									      |
|   Returns	    -1: Error; Else if >= 0: VARTYPE of the result	      |
|									      |
|   Notes	    Based on samples in					      |
|                   https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
|                   https://stackoverflow.com/questions/1431103/how-to-obtain-data-from-wmi-using-a-c-application
|                                                                             |
|   History	    							      |
|    2020-02-25 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

// Note: #define _WIN32_DCOM before including windows.h above

#include <wbemidl.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

int GetWmiProcInfo(char *lpszPropName, void *lpBuf, size_t lBuf) {
  HRESULT hr = 0;
  int iResult = -1;
  IWbemLocator         *pLoc  = NULL;
  IWbemServices        *pSvc = NULL;
  IEnumWbemClassObject *pEnum = NULL;
  size_t lPropName = lstrlen(lpszPropName);
  WCHAR *lpwszPropName = NULL;
  WCHAR *lpwszBuf = NULL;
  // BSTR strings we'll use (http://msdn.microsoft.com/en-us/library/ms221069.aspx)
  BSTR resource = NULL;
  BSTR language = NULL;
  BSTR query    = NULL;

  resource = SysAllocString(L"ROOT\\CIMV2");
  language = SysAllocString(L"WQL");
  lpwszPropName = (LPWSTR)malloc(sizeof(WCHAR)*(lPropName + 1));
  if (lpwszPropName) {
    MultiByteToWideChar(CP_ACP, 0, lpszPropName, (int)lPropName + 1, lpwszPropName, (int)lPropName + 1);
  }
  lpwszBuf = (LPWSTR)malloc(sizeof(WCHAR)*(lPropName + 30));
  if (lpwszBuf) {
    wsprintfW(lpwszBuf, L"SELECT %s FROM Win32_Processor", lpwszPropName);
    query = SysAllocString(lpwszBuf);
  }

  // 1. Initialize COM
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr)) goto cleanup_and_exit;

  // 2. Set general COM security levels
  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
  if (FAILED(hr)) goto cleanup_and_exit;

  // 3. Obtain the initial locator to WMI
  hr = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &pLoc);
  if (FAILED(hr)) goto cleanup_and_exit;

  // 4. Connect to WMI
  hr = pLoc->lpVtbl->ConnectServer(pLoc, resource, NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
  if (FAILED(hr)) goto cleanup_and_exit;

  // 5. Set security levels on the proxy
  // Apparently not needed here

  // 6. Issue the WMI query
  hr = pSvc->lpVtbl->ExecQuery(pSvc, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &pEnum);
  if (FAILED(hr)) goto cleanup_and_exit;

  // 7. Get the data from the query
  if (pEnum) {
    IWbemClassObject *pObj = NULL;
    ULONG nReturn = 0;

    // Enumerate the retrieved objects
    // https://docs.microsoft.com/en-us/windows/win32/api/oaidl/ns-oaidl-variant
    // https://docs.microsoft.com/en-us/windows/win32/api/wtypes/ne-wtypes-varenum
    while((hr = pEnum->lpVtbl->Next(pEnum, WBEM_INFINITE, 1, &pObj, &nReturn)) == S_OK) {
      VARIANT vtProp;

      hr = pObj->lpVtbl->Get(pObj, lpwszPropName, 0, &vtProp, 0, 0);
      if (FAILED(hr)) goto cleanup_and_exit;

      // Copy the result to the caller's buffer
      iResult = vtProp.vt;
      memset(lpBuf, 0, lBuf);
      switch (vtProp.vt) {
      case VT_I1:
      case VT_UI1:
      	if (lBuf >= 1) *(CHAR *)lpBuf = vtProp.cVal;
      	break;
      case VT_I2:
      case VT_UI2:
      case VT_BOOL:
      	if (lBuf >= 2) *(USHORT *)lpBuf = vtProp.uiVal;
      	break;
      case VT_I4:
      case VT_UI4:
      case VT_INT:
      case VT_UINT:
      	if (lBuf >= 4) *(ULONG *)lpBuf = vtProp.ulVal;
      	break;
      case VT_BSTR:
      	WideCharToMultiByte(CP_ACP, 0, vtProp.bstrVal, -1, lpBuf, (int)lBuf, "?", NULL);
      	break;
      default:
	iResult = -1;
      	fprintf(stderr, "Unsupported VARIANT type: %d\n", vtProp.vt);
      }

      // Release the current result object
      pObj->lpVtbl->Release(pObj);

      break; // Ignore the value for the other processors, assuming they'll all be the same.
    }
  }

cleanup_and_exit:
  // Release WMI COM interfaces
  if (pEnum) pEnum->lpVtbl->Release(pEnum);
  if (pSvc) pSvc->lpVtbl->Release(pSvc);
  if (pLoc) pLoc->lpVtbl->Release(pLoc);
  CoUninitialize();

  // Free everything else we've allocated
  free(lpwszBuf);
  free(lpwszPropName);
  SysFreeString(query);
  SysFreeString(language);
  SysFreeString(resource);

  if ((iResult < 0) && (lBuf >= sizeof(HRESULT))) *(HRESULT *)lpBuf = hr;
  return iResult;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DisplayProcWmiInfo					      |
|									      |
|   Description	    Display a few WMI properties			      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    None						      |
|                                                                             |
|   Notes	    Displays SLAT (Second Level Address Translation),	      |
|		    which would otherwise require reading MSRs,               |
|		    which cannot be done in Ring 3.			      |
|                                                                             |
|   History								      |
|    2020-02-26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void DisplayProcWmiInfo(void) {
  char *pszProps[] = {
    "L2CacheSize",
    "L3CacheSize",
    "SecondLevelAddressTranslationExtensions"
  };
  int i;

  printf("WMI Win32_Processor information\n");
  for (i=0; i<(sizeof(pszProps)/sizeof(char *)); i++) {
    char *pszPropName;
    int iRet;
    char buf[256];
    pszPropName = pszProps[i];
    iRet = GetWmiProcInfo(pszPropName, buf, sizeof(buf));
    switch (iRet) {
    case -1: {
      HRESULT hr = *(HRESULT *)buf;
      printf(" %s = (WMI Error. HRESULT = 0x%X)\n", pszPropName, hr);
      break; }
    case VT_BOOL:
      printf(" %s = %s\n", pszPropName, *(BOOL *)buf ? "True" : "False");
      break;
    case VT_BSTR:
      printf(" %s = %s\n", pszPropName, buf);
      break;
    default:
      printf(" %s = %d\n", pszPropName, *(int *)buf);
      break;
    }
  }
}

#endif // defined(_WIN32)
