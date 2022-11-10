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
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Identify the processor and its features"
#define PROGRAM_NAME    "cpuid"
#define PROGRAM_VERSION "2022-11-10"

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

    { 15,    0,  "Willamette",	"Pentium 4 model 0 180nm" },
    { 15,    1,  "Willamette",	"Pentium 4 model 1 180nm" },
    { 15,    2,  "Northwood",	"Pentium 4 model 2 130nm" },
    { 15,    3,  "Prescott",	"Pentium 4 model 3 90nm" },
    { 15,    4,  "Prescott-2M",	"Pentium 4 model 4 90nm" }, // 64 bits
    { 15,    6,  "Cedar Mill",	"Pentium 4 model 6 65nm" }, // 64 bits
    };
#define N_INTEL_PROCS (sizeof(IntelProcList) / sizeof(INTEL_PROC))

/* Forward references */

void usage(void);
DWORD _rdtsc(void);
long getms(void);
void DisplayProcInfo(void);
DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx);
void _rdmsr(DWORD dwECX, DWORD pdwMSR[2]);
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
  long lt0, lt1;
  DWORD dwt0, dwt1;
  int iVerbose = FALSE;
  int iDebug = FALSE;

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
      if (streq(opt, "d")) {		/* -d: Debug information */
	iDebug = TRUE;
	iVerbose = TRUE;
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

  iFamily = identify_processor();

  if (iFamily < 5) {
    printf("\nThe processor is a 80%d\n", (iFamily * 100) + 86);
  } else { // if (iFamily >= 5)
    int iModel;
    int iExtModel;
    int iExtFamily;
    DWORD dwModel;
    char szBrand[48] = "";

    dwModel = _cpuid(1, NULL, NULL, NULL, NULL);

    /* Compute the extended model number */
    iModel = BYTE0(dwModel) >> 4;
    iExtModel = BYTE2(dwModel) & 0x0F;
    iModel |= (iExtModel << 4);

    /* Compute the extended model number */
    /* iFamily = BYTE1(dwModel) & 0x0F; */
    iExtFamily = (WORD1(dwModel) >> 4) & 0xFF;
    iFamily |= (iExtFamily << 4);

    /* On Pentium or better, get the processor brand name from CPUID output */

    /* Use the brand string if available */
    if (_cpuid(0x80000000, NULL, NULL, NULL, NULL) >= 0x80000004) {
      DWORD *pdwBrand = (DWORD *)szBrand;
      char *pszBrand;
      char *pc;

      // CPUID(0x80000002 - 0x80000004) : Get brand string.
      _cpuid(0x80000002, pdwBrand+0, pdwBrand+1, pdwBrand+2, pdwBrand+3);
      _cpuid(0x80000003, pdwBrand+4, pdwBrand+5, pdwBrand+6, pdwBrand+7);
      _cpuid(0x80000004, pdwBrand+8, pdwBrand+9, pdwBrand+10, pdwBrand+11);
      // Skip leading spaces
      for (pszBrand = szBrand; *pszBrand == ' '; pszBrand++) ;
      // Compress multiple spaces into a single space
      for (pc=pszBrand; *pc; pc++) {
	while ((pc[0]==' ') && (pc[1]==' ')) {
	  char *pc2;
	  for (pc2=pc; (pc2[0]=pc2[1]) != '\0'; pc2++) ;
	}
      }
      // Display the readable brand string.
      printf("\nThe processor is an %s\n", pszBrand);
    }

    /* Else compute the processor name from the CPUID family and model numbers */
    if ((!szBrand[0]) || iVerbose) {
      for (i=0; i<N_INTEL_PROCS; i++) {
	if (   (IntelProcList[i].iFamily == iFamily)
	    && (IntelProcList[i].iModel == iModel)) {
	  break;
	}
      }
      if (i < N_INTEL_PROCS) {
	printf("\nThe processor is a %s\n", IntelProcList[i].pszName);
      } else {
	printf("\nThe processor is a 80%d model %d\n",
		    (iFamily * 100) + 86, iModel);
      }
    }

    /* On Pentium or better, compute the processor frequency using the TSC */
    /* Note: This algorithm is compatible with Windows 95 & NT */

    if (iVerbose) {
      DisplayProcInfo();

#ifdef _WIN32
      DisplayProcWmiInfo();
#endif // defined(_WIN32)
    }

#ifdef _MSDOS
    // Switch to protected mode since the time-stamp counter is NOT
    // accessible in virtual 86 mode.
    if (dpmi_detect() == 0) {
      if (iDebug) printf("Switching to 16-bits PM using DPMI.\n");
      iErr = dpmi2prot();
      if (iErr) {
	fprintf(stderr, "DPMI error %d switching to protected mode.\n", iErr);
	exit(1);
      }
    } else if (vcpi_detect() == 0) {
      if (iDebug) printf("Switching to 16-bits PM using VCPI.\n");
      iErr = vcpi2prot();
      if (iErr) {
	fprintf(stderr, "VCPI error %d switching to protected mode.\n", iErr);
	exit(1);
      }
    } else { // No virtual machine control program detected.
      if (iDebug) printf("Staying in real mode.\n");
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
    if (iDebug) printf("Counted %lu cycles in %ld ms\n", dwt1, lt1);
    if (lt1 < 0) lt1 += 86400000;	// Possible wrap-around at midnight
    lt1 *= 1000;			// Convert to microseconds
    dwt1 += lt1/2;			// Round quotient to the nearest value
    dwt1 /= lt1;			// First frequency evaluation
    if (iDebug) printf("Raw frequency measure: %lu MHz\n", dwt1);
    // Round to the nearest multiple of 16.66666 = (100/6)
    if (dwt1 > 95) {	// Rule applies only for processors above 100 MHz
      dwt1 *= 6;
      dwt1 += 50;
      dwt1 /= 100;
      dwt1 *= 100;
      dwt1 /= 6;
    }
    printf("Frequency: %ld MHz\n", dwt1);
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
Usage: CPUID [switches]\n\
\n\
Optional switches:\n\
\n\
  -c EAX [ECX]  Get one given CPUID leaf and optional sub-leaf\n\
  -v    Display detailed processor capabilities information\n\
  -V    Display this program version and exit\n\
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
|   Function	    DisplayProcInfo					      |
|									      |
|   Description	    Display detailed processor information, from CPUID output.|
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    None						      |
|                                                                             |
|   History								      |
|    1998/03/18 JFL Created this routine				      |
|    2009/08/31 JFL Restructured to display both enabled and disabled features|
|		    Added the definitions of numerous AMD extended features.  |
|    2009/09/01 JFL Renamed from DisplayProcId to DisplayProcInfo.            |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

char *ppszFeatures[32] = { /* Intel Features Flags - EAX=1 -> EDX */
  /* 0x00000001  0 */ "Integrated FPU",
  /* 0x00000002  1 */ "Enhanced V86 mode",
  /* 0x00000004  2 */ "I/O breakpoints",
  /* 0x00000008  3 */ "4 MB pages",
  /* 0x00000010  4 */ "Time stamp counter",
  /* 0x00000020  5 */ "Model-specific registers",
  /* 0x00000040  6 */ "Physical address extensions",
  /* 0x00000080  7 */ "Machine-check exception",
  /* 0x00000100  8 */ "CMPXCHG8B instruction",
  /* 0x00000200  9 */ "Integrated APIC",
  /* 0x00000400 10 */ "(EDX bit 10 reserved)",
  /* 0x00000800 11 */ "SYSENTER/SYSEXIT instructions",
  /* 0x00001000 12 */ "MTRR registers, and the MTRR_CAP register",
  /* 0x00002000 13 */ "Page Global Enable bit in CR4",
  /* 0x00004000 14 */ "Machine check architecture",
  /* 0x00008000 15 */ "CMOV instructions",
  /* 0x00010000 16 */ "Page Attribute table in MTRRs",
  /* 0x00020000 17 */ "36-bit page size extensions",
  /* 0x00040000 18 */ "Processor Serial Number in CPUID#3",
  /* 0x00080000 19 */ "CLFLUSH instruction",
  /* 0x00100000 20 */ "(EDX bit 20 reserved)",
  /* 0x00200000 21 */ "Debug Trace Store & Event Mon.",
  /* 0x00400000 22 */ "ACPI thermal and clock control registers",
  /* 0x00800000 23 */ "MMX instructions",
  /* 0x01000000 24 */ "FXSAVE and FXRSTOR Instructions",
  /* 0x02000000 25 */ "SSE (Streaming SIMD Extensions)",
  /* 0x04000000 26 */ "SSE2 (Streaming SIMD Extensions v2)",
  /* 0x08000000 27 */ "Self-Snoop memory and caches",
  /* 0x10000000 28 */ "Hyper-threading capable",
  /* 0x20000000 29 */ "Thermal monitoring circuit",
  /* 0x40000000 30 */ "IA64 capable",
  /* 0x80000000 31 */ "Pending Break Enable (PBE# pin) wakeup capability",
};

char *ppszFeatures2[32] = { /* Intel Features Flags - EAX=1 -> ECX */
  /* 0x00000001  0 */ "SSE3 (Streaming SIMD Extensions v3)",
  /* 0x00000002  1 */ "PCLMULDQ instruction",
  /* 0x00000004  2 */ "64-Bit Debug Store",
  /* 0x00000008  3 */ "MONITOR and MWAIT instructions",
  /* 0x00000010  4 */ "CPL Qualified Debug Store",
  /* 0x00000020  5 */ "VMX (Virtual Machine Extensions)",
  /* 0x00000040  6 */ "Safer Mode Extensions (Trusted Execution)",
  /* 0x00000080  7 */ "Enhanced SpeedStep Technology",
  /* 0x00000100  8 */ "Thermal Monitor 2 Control Circuit",
  /* 0x00000200  9 */ "SSSE3 (Suplemental Streaming SIMD Extensions v3)",
  /* 0x00000400 10 */ "L1 data cache Context ID",
  /* 0x00000800 11 */ "SDBG (Silicon Debug interface)",
  /* 0x00001000 12 */ "Fused Multiply Add extensions",
  /* 0x00002000 13 */ "CMPXCHG16B instruction",
  /* 0x00004000 14 */ "Send Task Priority Messages update control",
  /* 0x00008000 15 */ "Perfmon and Debug Capability",
  /* 0x00010000 16 */ "(ECX bit 16 reserved)",
  /* 0x00020000 17 */ "Process Context Identifiers (CR4 bit 17)",
  /* 0x00040000 18 */ "Direct Cache Access for DMA writes",
  /* 0x00080000 19 */ "SSE4.1 (Streaming SIMD Extensions 4.1)",
  /* 0x00100000 20 */ "SSE4.2 (Streaming SIMD Extensions 4.2)",
  /* 0x00200000 21 */ "Extended xAPIC Support",
  /* 0x00400000 22 */ "MOVBE Instruction",
  /* 0x00800000 23 */ "POPCNT Instruction",
  /* 0x01000000 24 */ "Timestamp Counter Deadline",
  /* 0x02000000 25 */ "AES instruction",
  /* 0x04000000 26 */ "XSAVE/XRESTOR instructions",
  /* 0x08000000 27 */ "OS-Enabled SXAVE/XRESTOR Management",
  /* 0x10000000 28 */ "AVX (Advanced Vector eXtensions)",
  /* 0x20000000 29 */ "16-bit Floating Point Conversion instructions",
  /* 0x40000000 30 */ "RDRAND instruction",
  /* 0x80000000 31 */ "Hypervisor present (always zero on physical CPUs)",
};

char *ppszFeatures70b[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> EBX */
  /* 0x00000001  0 */ "FSGSBASE instructions (RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE)",
  /* 0x00000002  1 */ "IA32_TSC_ADJUST MSR is supported",
  /* 0x00000004  2 */ "SGX (Software Guard Extensions)",
  /* 0x00000008  3 */ "BMI1 (Bit Manipulation Instruction Set 1)",
  /* 0x00000010  4 */ "HLE (Hardware Lock Elision)",
  /* 0x00000020  5 */ "AVX2 (Advanced Vector Extensions 2)",
  /* 0x00000040  6 */ "x87 FPU Data Pointer updated only on x87 exceptions",
  /* 0x00000080  7 */ "SMEP (Supervisor-Mode Execution Prevention)",
  /* 0x00000100  8 */ "BMI2 (Bit Manipulation Instruction Set 2)",
  /* 0x00000200  9 */ "Enhanced REP MOVSB/STOSB",
  /* 0x00000400 10 */ "INVPCID instruction",
  /* 0x00000800 11 */ "RTM (Restricted Transactional Memory) instructions",
  /* 0x00001000 12 */ "RDT-M (Resource Director Technology Monitoring)",
  /* 0x00002000 13 */ "FPU CS and DS values deprecated",
  /* 0x00004000 14 */ "MPX (Memory Protection Extensions)",
  /* 0x00008000 15 */ "RDT-A (Resource Director Technology Allocation)",
  /* 0x00010000 16 */ "AVX512F",
  /* 0x00020000 17 */ "AVX512DQ",
  /* 0x00040000 18 */ "RDSEED instruction",
  /* 0x00080000 19 */ "ADX (Multi-Precision Add-Carry Instruction Extensions)",
  /* 0x00100000 20 */ "SMAP (Supervisor-Mode Access Prevention) instructions",
  /* 0x00200000 21 */ "AVX512_IFMA",
  /* 0x00400000 22 */ "PCOMMIT (Persistent Memory Commit) instruction",	// Deprecated: https://software.intel.com/en-us/blogs/2016/09/12/deprecate-pcommit-instruction
  /* 0x00800000 23 */ "CLFLUSHOPT Instruction",
  /* 0x01000000 24 */ "CLWB (Cache Line Write Back) instruction",
  /* 0x02000000 25 */ "Intel Processor Trace",
  /* 0x04000000 26 */ "AVX512PF",
  /* 0x08000000 27 */ "AVX512ER",
  /* 0x10000000 28 */ "AVX512CD",
  /* 0x20000000 29 */ "SHA (Secure Hash Algorithm Extensions)",
  /* 0x40000000 30 */ "AVX512BW",
  /* 0x80000000 31 */ "AVX512VL",
};

char *ppszFeatures70c[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> ECX */
  /* 0x00000001  0 */ "PREFETCHWT1 instruction",
  /* 0x00000002  1 */ "AVX-512 Vector Bit Manipulation Instructions",
  /* 0x00000004  2 */ "User-mode Instruction Prevention",
  /* 0x00000008  3 */ "PKU (Memory Protection Keys for User-mode pages)",
  /* 0x00000010  4 */ "PKU enabled by OS",
  /* 0x00000020  5 */ "WAITPKG (UMWAIT instruction)",
  /* 0x00000040  6 */ "AVX-512 Vector Bit Manipulation Instructions 2",
  /* 0x00000080  7 */ "SHSTK (Shadow Stack instructions)",
  /* 0x00000100  8 */ "GFNI (Galois Field instructions)",
  /* 0x00000200  9 */ "VAES (Vector AES instruction set (VEX-256/EVEX))",
  /* 0x00000400 10 */ "CLMUL instruction set (VEX-256/EVEX)",
  /* 0x00000800 11 */ "AVX-512 Vector Neural Network Instructions",
  /* 0x00001000 12 */ "AVX-512 BITALG instructions",
  /* 0x00002000 13 */ "(ECX bit 13 reserved)",
  /* 0x00004000 14 */ "AVX-512 Vector Population Count Double and Quad-word",
  /* 0x00008000 15 */ "(ECX bit 15 reserved)",
  /* 0x00010000 16 */ "5-level paging",
  /* 0x00020000 17 */ "MPX Address-Width Adjust bit 0",
  /* 0x00040000 18 */ "MPX Address-Width Adjust bit 1",
  /* 0x00080000 19 */ "MPX Address-Width Adjust bit 2",
  /* 0x00100000 20 */ "MPX Address-Width Adjust bit 3",
  /* 0x00200000 21 */ "MPX Address-Width Adjust bit 4",
  /* 0x00400000 22 */ "RDPID (Read Processor ID) instruction",
  /* 0x00800000 23 */ "(ECX bit 23 reserved)",
  /* 0x01000000 24 */ "(ECX bit 24 reserved)",
  /* 0x02000000 25 */ "CLDEMOTE (Cache Line Demote) instruction",
  /* 0x04000000 26 */ "(ECX bit 26 reserved)",
  /* 0x08000000 27 */ "MOVDIR (Direct Store) instructions",
  /* 0x10000000 28 */ "MOVDIR64B (Direct Store) instructions",
  /* 0x20000000 29 */ "(ECX bit 29 reserved)",
  /* 0x40000000 30 */ "SGX (Software Guard Extensions) instructions",
  /* 0x80000000 31 */ "(ECX bit 31 reserved)",
};

char *ppszFeatures70d[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=0 -> EDX */
  /* 0x00000001  0 */ "",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "AVX-512 4-register Neural Network instructions",
  /* 0x00000008  3 */ "AVX-512 4-register Multiply Accumulation Single precision",
  /* 0x00000010  4 */ "FSRM (Fast Short REP MOVSB)",
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
  /* 0x00040000 18 */ "PCONFIG Platform Configuration (Memory Encryption)",
  /* 0x00080000 19 */ "",
  /* 0x00100000 20 */ "IBT (Indirect-Branch Tracking)",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "",
  /* 0x00800000 23 */ "",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "IBRS_IBPB (Indirect Branch Restricted Speculation)",
  /* 0x08000000 27 */ "STIBP (Single Thread Indirect Branch Predictor)",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "Speculative Side Channel Mitigations",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "SSBD (Speculative Store Bypass Disable)",
};

char *ppszFeatures71a[32] = { /* Structured Extended Feature Flags - EAX=7, ECX=1 -> EAX */
  /* 0x00000001  0 */ "",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "",
  /* 0x00000020  5 */ "AVX-512 BFLOAT16 instructions",
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

char *ppszExtFeatures[32] = { /* AMD Extended Features Flags - EDX */
  /* Unknown bits, with a "" definition, will not be displayed. */
  /* Flags that are just a copy of the corresponding Intel Features Flag are commented-out */
  /* 0x00000001  0 */ "", // "Integrated FPU",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "", // "Time stamp counter",
  /* 0x00000020  5 */ "",
  /* 0x00000040  6 */ "",
  /* 0x00000080  7 */ "",
  /* 0x00000100  8 */ "", // "CMPXCHG8B instruction",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "",
  /* 0x00000800 11 */ "SYSCALL and SYSRET instructions",
  /* 0x00001000 12 */ "",
  /* 0x00002000 13 */ "",
  /* 0x00004000 14 */ "",
  /* 0x00008000 15 */ "", // "CMOV instructions",
  /* 0x00010000 16 */ "",
  /* 0x00020000 17 */ "",
  /* 0x00040000 18 */ "",
  /* 0x00080000 19 */ "",
  /* 0x00100000 20 */ "Execution disable bit",
  /* 0x00200000 21 */ "",
  /* 0x00400000 22 */ "AMD extensions to MMX",
  /* 0x00800000 23 */ "", // "MMX instructions",
  /* 0x01000000 24 */ "", // "FXSAVE and FXRSTOR Instructions",
  /* 0x02000000 25 */ "", // "SSE instructions",
  /* 0x04000000 26 */ "",
  /* 0x08000000 27 */ "RDTSCP instruction",
  /* 0x10000000 28 */ "PDPE1GB Gibibyte pages",
  /* 0x20000000 29 */ "64 bit instructions (=long mode/EM64T/x86_64)",
  /* 0x40000000 30 */ "AMD extensions to 3DNow!",
  /* 0x80000000 31 */ "3DNow! instructions",
};

char *ppszExtFeatures2[32] = { /* AMD Extended Features Flags - ECX */
  /* 0x00000001  0 */ "LAHF and SAHF in 64-bits mode",
  /* 0x00000002  1 */ "",
  /* 0x00000004  2 */ "Secure Virtual Machine instructions",
  /* 0x00000008  3 */ "",
  /* 0x00000010  4 */ "Use of LOCK prefix to read CR8",
  /* 0x00000020  5 */ "LZCNT instruction (Count leading Zeros)",
  /* 0x00000040  6 */ "SSE4A Instructions",
  /* 0x00000080  7 */ "",
  /* 0x00000100  8 */ "3DNow! PREFETCH/PREFETCHW instructions",
  /* 0x00000200  9 */ "",
  /* 0x00000400 10 */ "",
  /* 0x00000800 11 */ "",
  /* 0x00001000 12 */ "DEV support",
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
  /* 0x00800000 23 */ "POPCNT instruction",
  /* 0x01000000 24 */ "",
  /* 0x02000000 25 */ "",
  /* 0x04000000 26 */ "",
  /* 0x08000000 27 */ "",
  /* 0x10000000 28 */ "",
  /* 0x20000000 29 */ "",
  /* 0x40000000 30 */ "",
  /* 0x80000000 31 */ "",
};

char *YesNo(unsigned long n) {
  if (n != 0) 
    return "Yes";
  else
    return "No";
}

void DisplayProcInfo(void) {
  DWORD dwMaxValue;
  DWORD dwMaxValueX;
  DWORD dwModel;
  DWORD dwModel2;
  DWORD dwFeatures;
  DWORD dwFeatures2;
  DWORD dwFeatures3;
  DWORD dwFeatures4;
  DWORD dwMask;
  char szName[14];
  int iFamily;
  int iModel;
  int i;
  int nCores;

  // CPUID(0) : 
  _cpuid(0, &dwMaxValue, (DWORD *)(szName+0), (DWORD *)(szName+8), (DWORD *)(szName+4));
  szName[12] = '\0';
  printf("%s", szName);

  if (dwMaxValue >= 1) {
    // CPUID(1) : Request the Family/Model/Step
    _cpuid(1, &dwModel, &dwModel2, &dwFeatures2, &dwFeatures);
    iFamily = BYTE1(dwModel) & 0x0F;
    printf(" Family %d", iFamily);
    iModel = BYTE0(dwModel) >> 4;
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

    // CPUID(0x80000000) : Get max extended function supported.
    printf("Max base function: 0x%08lX\n", dwMaxValue);
    dwMaxValueX = _cpuid(0x80000000, NULL, NULL, NULL, NULL);
    if (dwMaxValueX >= 0x80000000)
      printf("Max extended function: 0x%08lX\n", dwMaxValueX);
    else
      printf("No extended CPUID functions.\n");
    printf("\n");

    /* Intel Feature Flags */
    printf("CPUID(1): Intel Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures, dwFeatures2);
    for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
      printf(" EDX %2d %-3s %s\n", i, YesNo(dwFeatures & dwMask), ppszFeatures[i]);
    }
    printf("\n");
    for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
      printf(" ECX %2d %-3s %s\n", i, YesNo(dwFeatures2 & dwMask), ppszFeatures2[i]);
    }
    printf("\n");

    /* AMD Extended Features Flags */
    if (dwMaxValueX >= 0x80000001) {
      /* Only display those that are documented in recent Intel's manuals */
      // CPUID(0x80000001) : Get extended feature flags.
      _cpuid(0x80000001, NULL, NULL, &dwFeatures4, &dwFeatures3);
      printf("CPUID(0x80000001): AMD Extended Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures3, dwFeatures4);
      for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
	char *pszName = ppszExtFeatures[i];
	if (pszName[0])
	  printf(" EDX %2d %-3s %s\n", i, YesNo(dwFeatures3 & dwMask), pszName);
      }
      printf("\n");
      for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
	char *pszName = ppszExtFeatures2[i];
	if (pszName[0])
	    printf(" ECX %2d %-3s %s\n", i, YesNo(dwFeatures4 & dwMask), pszName);
      }
      printf("\n");
    }

    /* Structured Extended Feature Flags */
    if (dwMaxValue >= 7) {
      DWORD dwEAX, dwEBX, dwECX, dwEDX;
      int nLeaves;

      dwECX = 0;
      _cpuid(7, &dwEAX, &dwEBX, &dwECX, &dwEDX);
      nLeaves = (int)dwEAX; /* Max ECX input value for cpuid(7) */
      printf("CPUID(7): Extended Features Flags: EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX\n", dwEBX, dwECX, dwEDX);
      for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
	char *pszName = ppszFeatures70b[i];
	printf(" EBX %2d %-3s %s\n", i, YesNo(dwEBX & dwMask), pszName);
      }
      printf("\n");
      for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
	char *pszName = ppszFeatures70c[i];
	printf(" ECX %2d %-3s %s\n", i, YesNo(dwECX & dwMask), pszName);
      }
      printf("\n");
      for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1) {
	char *pszName = ppszFeatures70d[i];
	if (pszName[0])
	  printf(" EDX %2d %-3s %s\n", i, YesNo(dwEDX & dwMask), pszName);
      }
      printf("\n");
      if (nLeaves > 0) {
	printf("ECX=%d => There are more Extended Feature Flags leaves\n\n", nLeaves); 
      }
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
  printf("\n");
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
|                        Etc...					              |
|                                                                             |
|   Notes	    Assume that under Win32, the processor is a Pentium	      |
|                   or better.                                                |
|                                                                             |
|   History								      |
|    2010-09-06 JFL Created this Win32 version.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32
int identify_processor(void) {
  DWORD dwModel;
  int iFamily;
  int iExtFamily;

  dwModel = _cpuid(1, NULL, NULL, NULL, NULL);

  /* Compute the extended family number */
  iFamily = BYTE1(dwModel) & 0x0F;
  iExtFamily = (WORD1(dwModel) >> 4) & 0xFF;
  iFamily |= (iExtFamily << 4);

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
  printf("\n");
}

#endif // defined(_WIN32)
