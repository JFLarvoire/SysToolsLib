/*****************************************************************************\
*                                                                             *
*   File name:	    cpuid.c						      *
*									      *
*   Description:    Identify the CPU ID and speed.			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*									      *
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
*    2017-05-31 JFL Fixed warnings. No functional code change.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DATE    "2017-05-31"

/* Definitions */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define OS_NAME "WIN32"

#define _CRT_SECURE_NO_WARNINGS // Don't warn about old unsecure functions.

#include <stdio.h>
#include <stdlib.h>

// if defined, the following flags inhibit the definitions in windows.h
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES   // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS       // MF_*
#define NOICONS       // IDI_*
#define NOKEYSTATES   // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS   // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
// #define OEMRESOURCE       // OEM Resource values
#define NOATOM        // Atom Manager routines
#define NOCLIPBOARD   // Clipboard routines
#define NOCOLOR       // Screen colors
#define NOCTLMGR      // Control and Dialog routines
#define NODRAWTEXT    // DrawText() and DT_*
#define NOGDI         // All GDI defines and routines
#define NOKERNEL      // All KERNEL defines and routines
#define NOUSER        // All USER defines and routines
#define NONLS         // All NLS defines and routines
#define NOMB          // MB_* and MessageBox()
#define NOMEMMGR      // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE    // typedef METAFILEPICT
#define NOMINMAX      // Macros min(a,b) and max(a,b)
#define NOMSG         // typedef MSG and associated routines
#define NOOPENFILE    // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL      // SB_* and scrolling routines
#define NOSERVICE     // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND       // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH          // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM        // COMM driver routines
#define NOKANJI       // Kanji support stuff.
#define NOHELP        // Help engine interface.
#define NOPROFILER    // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX         // Modem Configuration Extensions
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

// Definitions, from MS-DOS' pmode.h, for data emission within the code stream.
#define DB(x) __asm _emit x			// BYTE
#define DW(x) DB((x) & 0xFF) DB((x) >> 8U)	// WORD
#define DD(x) DW((x) & 0xFFFF) DW((x) >> 16U)	// DWORD

int identify_processor(void); // 0=8086, 1=80186, 2=80286, etc...

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#define OS_NAME "DOS"

#include "clibdef.h"
#include "utildef.h"
#include "lodos.h"
#include "pmode.h"

#define vcpi2prot vm2prot

#endif /* _MSDOS */

/********************** End of OS-specific definitions ***********************/

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
    { 15,    1,  "",		"Pentium 4 model 1 180nm" },
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    EXE program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the BIOS, if run from ROM.     |
|									      |
|   History:								      |
|									      |
|    1996/11/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[])
    {
    int i;
    int iFamily;
    long lt0, lt1;
    DWORD dwt0, dwt1;
    int iVerbose = FALSE;
    int iDebug = FALSE;

#ifdef _MSDOS
    int iErr;

    _dos_setblock(0x1000, _psp, &iErr);
#endif // defined(_MSDOS)

    /* Process arguments */

    for (i=1 ; i<argc ; i++)
        {
        _strlwr(argv[i]);
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))    /* It's a switch */
            {
	    if (streq(argv[i]+1, "?"))		/* -?: Help */
                {
                usage();                            /* Display help */
		}
	    if (streq(argv[i]+1, "d"))		/* -d: Debug information */
                {
		iDebug = TRUE;
		iVerbose = TRUE;
		continue;
                }
	    if (streq(argv[i]+1, "v"))		/* -v: Verbose information */
                {
		iVerbose = TRUE;
		continue;
                }
	    }
	}

    iFamily = identify_processor();

    if (iFamily < 5)
	{
	printf("\nThe processor is a 80%d\n", (iFamily * 100) + 86);
	}
    else // if (iFamily >= 5)
	{
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
	if (_cpuid(0x80000000, NULL, NULL, NULL, NULL) >= 0x80000004)
	    {
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
	    for (pc=pszBrand; *pc; pc++)
		{
		while ((pc[0]==' ') && (pc[1]==' '))
		    {
		    char *pc2;
		    for (pc2=pc; (pc2[0]=pc2[1]) != '\0'; pc2++) ;
		    }
		}
	    // Display the readable brand string.
	    printf("\nThe processor is an %s\n", pszBrand);
	    }

	/* Else compute the processor name from the CPUID family and model numbers */
	if ((!szBrand[0]) || iVerbose)
	    {
	    for (i=0; i<N_INTEL_PROCS; i++)
		{
		if (   (IntelProcList[i].iFamily == iFamily)
		    && (IntelProcList[i].iModel == iModel))
		    {
		    break;
		    }
		}
	    if (i < N_INTEL_PROCS)
		{
		printf("\nThe processor is a %s\n", IntelProcList[i].pszName);
		}
	    else
		{
		printf("\nThe processor is a 80%d model %d\n",
			    (iFamily * 100) + 86, iModel);
		}
	    }

	/* On Pentium or better, compute the processor frequency using the TSC */
	/* Note: This algorithm is compatible with Windows 95 & NT */

	if (iVerbose) DisplayProcInfo();

#ifdef _MSDOS
	// Switch to protected mode since the time-stamp counter is NOT
	// accessible in virtual 86 mode.
	if (dpmi_detect() == 0)
	    {
	    if (iDebug) printf("Switching to 16-bits PM using DPMI.\n");
	    iErr = dpmi2prot();
	    if (iErr)
		{
		printf("DPMI error %d switching to protected mode.\n", iErr);
		exit(0);
		}
	    }
	else if (vcpi_detect() == 0)
	    {
	    if (iDebug) printf("Switching to 16-bits PM using VCPI.\n");
	    iErr = vcpi2prot();
	    if (iErr)
		{
		printf("VCPI error %d switching to protected mode.\n", iErr);
		exit(0);
		}
	    }
	else // No virtual machine control program detected.
	    {
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
	if (iDebug) printf("Counted %uld cycles in %ld ms\n", dwt1, lt1);
	if (lt1 < 0) lt1 += 86400000;	// Possible wrap-around at midnight
	lt1 *= 1000;			// Convert to microseconds
	dwt1 += lt1/2;			// Round quotient to the nearest value
	dwt1 /= lt1;			// First frequency evaluation
	if (iDebug) printf("Raw frequency measure: %ld MHz\n", dwt1);
	// Round to the nearest multiple of 16.66666 = (100/6)
	if (dwt1 > 95)	// Rule applies only for processors above 100 MHz
	    {
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
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help for this program		      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    N/A 						      |
|                                                                             |
|   History:								      |
|									      |
|    1996/10/10 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void)
    {
    puts("\
\n\
Processor identifier, version " PROGRAM_DATE " for " OS_NAME "\n\
\n\
Usage:\n\
\n\
  CPUID [switches]\n\
\n\
Optional switches:\n\
\n\
  -v    Display detailed processor capabilities information.\n\
\n\
Author: Jean-Francois Larvoire\n\
");
    exit(0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _rdtsc						      |
|									      |
|   Description:    Read the low DWORD of the time stamp counter	      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    DX:AX = Cycle count 				      |
|                                                                             |
|   Notes:	    Warning: The RDTSC instruction generates a GPF when run   |
|		    under Windows in virtual 86 mode.			      |
|		    This is why it is recommended to only call this routine   |
|		    while in 16-bits protected mode.			      |
|                                                                             |
|   History:								      |
|									      |
|    1996/11/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

#define rdtsc DW(0x310F)

#ifdef _MSDOS
DWORD _rdtsc(void)
    {
    _asm
	{
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

#ifdef _WIN32
DWORD _rdtsc(void)
    {
    _asm
	{
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
|   Function:	    _dos_get100th					      |
|									      |
|   Description:    Get the MS-DOS 1/100th of a second relative to 0h00.      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    DX:AX = Current 1/100th of a second relative to 0h00.     |
|                                                                             |
|   History:								      |
|									      |
|    1996/11/20 JFL Created this routine				      |
|    2003/01/21 JFL Rewrote using Microsoft-compatible _dos_getime().	      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
long _dos_get100th(void)
    {
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
    
long getms(void)
    {
    return _dos_get100th() * 10;
    }
#endif // defined(_MSDOS)

#ifdef _WIN32
long getms(void)
    {
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
|   Function:	    DisplayProcInfo					      |
|									      |
|   Description:    Display detailed processor information, from CPUID output.|
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    None						      |
|                                                                             |
|   History:								      |
|									      |
|    1998/03/18 JFL Created this routine				      |
|    2009/08/31 JFL Restructured to display both enabled and disabled features|
|		    Added the definitions of numerous AMD extended features.  |
|    2009/09/01 JFL Renamed from DisplayProcId to DisplayProcInfo.            |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

char *ppszFeatures[32] = { /* Intel Features Flags - EDX */
    /* 0x00000001 */ "Integrated FPU",				//  0
    /* 0x00000002 */ "Enhanced V86 mode",
    /* 0x00000004 */ "I/O breakpoints",
    /* 0x00000008 */ "4 MB pages",
    /* 0x00000010 */ "Time stamp counter",			//  4
    /* 0x00000020 */ "Model-specific registers",
    /* 0x00000040 */ "Physical address extensions",
    /* 0x00000080 */ "Machine-check exception",
    /* 0x00000100 */ "CMPXCHG8B instruction",			//  8
    /* 0x00000200 */ "Integrated APIC",
    /* 0x00000400 */ "MTTR registers",
    /* 0x00000800 */ "SYSENTER/SYSEXIT instructions",
    /* 0x00001000 */ "MTTR_CAP register",			// 12
    /* 0x00002000 */ "Page Global Enable",
    /* 0x00004000 */ "Machine check architecture",
    /* 0x00008000 */ "CMOV instructions",
    /* 0x00010000 */ "Page Attribute table in MTRRs",		// 16
    /* 0x00020000 */ "36-bit page size extensions",
    /* 0x00040000 */ "Serial number in CPUID#3",
    /* 0x00080000 */ "CLFLUSH instruction",
    /* 0x00100000 */ "(EDX bit 20 reserved)",			// 20
    /* 0x00200000 */ "Debug Trace Store & Event Mon.",
    /* 0x00400000 */ "ACPI thermal and clock control registers",
    /* 0x00800000 */ "MMX instructions",
    /* 0x01000000 */ "FXSAVE and FXRSTOR Instructions",		// 24
    /* 0x02000000 */ "Streaming SIMD Extensions (SSE)",
    /* 0x04000000 */ "Streaming SIMD Extensions v2 (SSE2)",
    /* 0x08000000 */ "Self-Snoop memory and caches",
    /* 0x10000000 */ "Hyper-threading capable",			// 28
    /* 0x20000000 */ "Thermal monitoring circuit",
    /* 0x40000000 */ "IA64 capable",
    /* 0x80000000 */ "Signal Break on FERR",
};

char *ppszFeatures2[32] = { /* Intel Features Flags - ECX */
    /* 0x00000001 */ "Streaming SIMD Extensions v3 (SSE3)",	//  0
    /* 0x00000002 */ "PCLMULDQ instruction",
    /* 0x00000004 */ "64-Bit Debug Store",
    /* 0x00000008 */ "MONITOR and MWAIT instructions",
    /* 0x00000010 */ "CPL Qualified Debug Store",		//  4
    /* 0x00000020 */ "Virtual Machine Extensions",
    /* 0x00000040 */ "Safer Mode Extensions (Trusted Execution)",
    /* 0x00000080 */ "Enhanced SpeedStep Technology",
    /* 0x00000100 */ "Thermal Monitor 2 Control Circuit",	//  8
    /* 0x00000200 */ "Supplemental Streaming SIMD Extensions v3 (SSSE3)",
    /* 0x00000400 */ "L1 data cache Context ID",
    /* 0x00000800 */ "(ECX bit 11 reserved)",
    /* 0x00001000 */ "Fused Multiply Add extensions",		// 12
    /* 0x00002000 */ "CMPXCHG16B instruction",
    /* 0x00004000 */ "Send Task Priority Messages update control",
    /* 0x00008000 */ "Perfmon and Debug Capability",
    /* 0x00010000 */ "(ECX bit 16 reserved)",			// 16
    /* 0x00020000 */ "Process Context Identifiers",
    /* 0x00040000 */ "Direct Cache Access",
    /* 0x00080000 */ "Streaming SIMD Extensions 4.1 (SSE4.1)",
    /* 0x00100000 */ "Streaming SIMD Extensions 4.2 (SSE4.2)",	// 20
    /* 0x00200000 */ "Extended xAPIC Support",
    /* 0x00400000 */ "MOVBE Instruction",
    /* 0x00800000 */ "POPCNT Instruction",
    /* 0x01000000 */ "Timestamp Counter Deadline",		// 24
    /* 0x02000000 */ "AES instruction",
    /* 0x04000000 */ "XSAVE/XSTOR States",
    /* 0x08000000 */ "OS-Enabled Extended State Management",
    /* 0x10000000 */ "Advanced Vector eXtensions",		// 28
    /* 0x20000000 */ "16-bit Floating Point Conversion instructions",
    /* 0x40000000 */ "RDRAND instruction",
    /* 0x80000000 */ "(ECX bit 31 reserved)",
};

char *ppszExtFeatures[32] = { /* AMD Extended Features Flags - EDX */
    /* Unknown bits, with a "" definition, will not be displayed. */
    /* Flags that are just a copy of the corresponding Intel Features Flag are commented-out */
    /* 0x00000001 */ "", // "Integrated FPU",                   //  0
    /* 0x00000002 */ "",
    /* 0x00000004 */ "",
    /* 0x00000008 */ "",
    /* 0x00000010 */ "", // "Time stamp counter",               //  4
    /* 0x00000020 */ "",
    /* 0x00000040 */ "",
    /* 0x00000080 */ "",
    /* 0x00000100 */ "", // "CMPXCHG8B instruction",            //  8
    /* 0x00000200 */ "",
    /* 0x00000400 */ "",
    /* 0x00000800 */ "SYSCALL and SYSRET instructions",
    /* 0x00001000 */ "",                                        // 12
    /* 0x00002000 */ "",
    /* 0x00004000 */ "",
    /* 0x00008000 */ "", // "CMOV instructions",
    /* 0x00010000 */ "",                                        // 16
    /* 0x00020000 */ "",
    /* 0x00040000 */ "",
    /* 0x00080000 */ "",
    /* 0x00100000 */ "Execution disable bit",                   // 20
    /* 0x00200000 */ "",
    /* 0x00400000 */ "AMD extensions to MMX",
    /* 0x00800000 */ "", // "MMX instructions",
    /* 0x01000000 */ "", // "FXSAVE and FXRSTOR Instructions",  // 24
    /* 0x02000000 */ "", // "SSE instructions",
    /* 0x04000000 */ "",
    /* 0x08000000 */ "RDTSCP instruction",
    /* 0x10000000 */ "",                                        // 28
    /* 0x20000000 */ "64 bit instructions (=long mode/EM64T/x86_64)",
    /* 0x40000000 */ "AMD extensions to 3DNow!",
    /* 0x80000000 */ "3DNow! instructions",
};

char *ppszExtFeatures2[32] = { /* AMD Extended Features Flags - ECX */
    /* 0x00000001 */ "LAHF and SAHF in 64-bits mode",           //  0
    /* 0x00000002 */ "",
    /* 0x00000004 */ "Secure Virtual Machine instructions",
    /* 0x00000008 */ "",
    /* 0x00000010 */ "Use of LOCK prefix to read CR8",          //  4
    /* 0x00000020 */ "LZCNT instruction (Count leading Zeros)",
    /* 0x00000040 */ "SSE4A Instructions",
    /* 0x00000080 */ "",
    /* 0x00000100 */ "3DNow! PREFETCH/PREFETCHW instructions",  //  8
    /* 0x00000200 */ "",
    /* 0x00000400 */ "",
    /* 0x00000800 */ "",
    /* 0x00001000 */ "DEV support",                             // 12
    /* 0x00002000 */ "",
    /* 0x00004000 */ "",
    /* 0x00008000 */ "",
    /* 0x00010000 */ "",                                        // 16
    /* 0x00020000 */ "",
    /* 0x00040000 */ "",
    /* 0x00080000 */ "",
    /* 0x00100000 */ "",                                        // 20
    /* 0x00200000 */ "",
    /* 0x00400000 */ "",
    /* 0x00800000 */ "POPCNT instruction",
    /* 0x01000000 */ "",                                        // 24
    /* 0x02000000 */ "",
    /* 0x04000000 */ "",
    /* 0x08000000 */ "",
    /* 0x10000000 */ "",                                        // 28
    /* 0x20000000 */ "",
    /* 0x40000000 */ "",
    /* 0x80000000 */ "",
};

char *YesNo(unsigned long n)
    {
    if (n != 0) 
        return "Yes";
    else
        return "No";
    }

void DisplayProcInfo(void)
    {
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

    if (dwMaxValue >= 1)
	{
	// CPUID(1) : Request the Family/Model/Step
	_cpuid(1, &dwModel, &dwModel2, &dwFeatures2, &dwFeatures);
	iFamily = BYTE1(dwModel) & 0x0F;
	printf(" Family %d", iFamily);
	iModel = BYTE0(dwModel) >> 4;
	printf(" Model %d", iModel);
	printf(" Stepping %d", BYTE0(dwModel) & 0x0F);
	for (i=0; i<N_INTEL_PROCS; i++)
	    {
	    if (   (IntelProcList[i].iFamily == iFamily)
		&& (IntelProcList[i].iModel == iModel))
		{
		printf(": %s \"%s\"", IntelProcList[i].pszName,
				      IntelProcList[i].pszCodeName);
		break;
		}
	    }
	printf("\n");
	printf("\n");

	// CPUID(0x80000000) : Get max extended function supported.
	printf("Max base function: 0x%08lX\n", dwMaxValue);
	dwMaxValueX = _cpuid(0x80000000, NULL, NULL, NULL, NULL);
	if (dwMaxValueX >= 0x80000000)
	    printf("Max extended function: 0x%08lX\n", dwMaxValueX);
	else
	    printf("No extended CPUID functions.\n");
	printf("\n");

	/* Intel Feature Flags */
	printf("Intel Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures, dwFeatures2);
	for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1)
	    {
	    printf(" EDX %-2d %-3s %s\n", i, YesNo(dwFeatures & dwMask), ppszFeatures[i]);
	    }
	for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1)
	    {
	    printf(" ECX %-2d %-3s %s\n", i, YesNo(dwFeatures2 & dwMask), ppszFeatures2[i]);
	    }
	printf("\n");

	/* AMD Extended Features Flags */
	if (dwMaxValueX >= 0x80000001)
	    { /* Only display those that are documented in recent Intel's manuals */
	    // CPUID(0x80000001) : Get extended feature flags.
	    _cpuid(0x80000001, NULL, NULL, &dwFeatures4, &dwFeatures3);
	    printf("AMD Extended Features Flags: EDX=0x%08lX ECX=0x%08lX\n", dwFeatures3, dwFeatures4);
	    for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1)
		{
		char *pszName = ppszExtFeatures[i];
		if (pszName[0])
		    printf(" EDX %-2d %-3s %s\n", i, YesNo(dwFeatures3 & dwMask), pszName);
		}
	    for (i = 0, dwMask = 1; dwMask; i++, dwMask <<= 1)
		{
		char *pszName = ppszExtFeatures2[i];
		if (pszName[0])
		    printf(" ECX %-2d %-3s %s\n", i, YesNo(dwFeatures4 & dwMask), pszName);
		}
	    printf("\n");
	    }

	/* Brand string */
	if (dwMaxValueX >= 0x80000004)
	    {
	    char szBrand[48];
	    DWORD *pdwBrand = (DWORD *)szBrand;

	    // CPUID(0x80000002 - 0x80000004) : Get brand string.
	    _cpuid(0x80000002, pdwBrand+0, pdwBrand+1, pdwBrand+2, pdwBrand+3);
	    _cpuid(0x80000003, pdwBrand+4, pdwBrand+5, pdwBrand+6, pdwBrand+7);
	    _cpuid(0x80000004, pdwBrand+8, pdwBrand+9, pdwBrand+10, pdwBrand+11);
	    printf("Brand string: \"%s\"\n", szBrand);
	    printf("\n");
	    }

	/* Virtual and Physical address Sizes */
	if (dwMaxValueX >= 0x80000008)
	    {
	    DWORD dwInfo;

	    _cpuid(0x80000008, &dwInfo, NULL, NULL, NULL);
	    printf("Physical Address Size: %d bits\n", BYTE0(dwInfo));
	    printf("Virtual Address Size: %d bits\n", BYTE1(dwInfo));
	    printf("\n");
	    }

	/* Number of cores and threads */
	printf("Cores and threads\n");
	nCores = 1;
	if (dwFeatures & (1 << 28)) nCores = (int)BYTE2(dwModel2);
	printf(" CPUID(1):  Silicon supports %d logical processors\n", nCores);
	if (dwMaxValue >= 4)
	    {
	    DWORD dwEAX, dwEBX, dwECX, dwEDX;
	    int nMaxCores, nMaxThreads;

	    dwECX = 0;
	    _cpuid(4, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	    nMaxCores = ((dwEAX >> 26) & 0x3F) + 1;
	    nMaxThreads = ((dwEAX >> 14) & 0xFFF) + 1;
	    printf(" CPUID(4):  Silicon supports %d cores and %d threads/core\n", nMaxCores, nMaxThreads);
	    }
	if (dwMaxValue >= 11)
	    {
	    DWORD dwEAX, dwEBX, dwECX, dwEDX;
	    int nMaxCores, nMaxThreads, nMaxLast, nMaxNext;
	    int nLevel;

	    dwECX = 0;
	    _cpuid(11, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	    nMaxThreads = WORD0(dwEBX);
	    if (nMaxThreads) { /* Some CPU models set dwMaxValue >= 11, yet return 0 here, and generate an exception if any further call is made */
		dwECX = 1;
		_cpuid(11, &dwEAX, &dwEBX, &dwECX, &dwEDX);
		nMaxLast = WORD0(dwEBX);
		nMaxCores = WORD0(dwEBX) / nMaxThreads;
		printf(" CPUID(11): Factory enabled %d cores and %d threads/core\n", nMaxCores, nMaxThreads);
		/* The CPUID spec mentions future procs will have more levels. Loop untested as of January 2013, as no available CPU has any. */
		if (nMaxLast) for (nLevel = 2; ; nLevel++)
		    {
		    dwECX = nLevel;
		    _cpuid(11, &dwEAX, &dwEBX, &dwECX, &dwEDX);
		    nMaxNext = WORD0(dwEBX) / nMaxLast;
		    nMaxLast = WORD0(dwEBX);
		    if (!nMaxLast) break; /* No more data. We've reached the last processor set level. */
		    printf(" CPUID(11): Factory enabled %d level %d core sets\n", nMaxLast, nLevel);
		    }
		}
	    }
	printf("\n");
	}
    else
	{
	printf("\n");
	}
    }

#pragma warning(default:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _cpuid						      |
|									      |
|   Description:    Execute a CPUID instruction, and collect results	      |
|									      |
|   Parameters:     DWORD dwId	    ID index (input)			      |
|		    DWORD *pEax     Where to store EAX. Discard EAX if NULL.  |
|		    DWORD *pEbx     Where to store EBX. Discard EBX if NULL.  |
|		    DWORD *pEcx     Where to store ECX. Discard ECX if NULL.  |
|		    DWORD *pEdx     Where to store EDX. Discard EDX if NULL.  |
|									      |
|   Returns:	    DX:AX = EAX 					      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|									      |
|    2000-03-23 JFL Created this routine				      |
|    2005-04-28 JFL Changed the 1st argument type from int to DWORD.          |
|    2013-01-31 JFL Added the ability to pass ECX as an input.                |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

#ifdef _MSDOS
#define cpuid DW(0xA20F)

DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx)
    {
    DWORD dwRet;

    _asm
	{
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

#ifdef _WIN32
DWORD _cpuid(DWORD dwId, DWORD *pEax, DWORD *pEbx, DWORD *pEcx, DWORD *pEdx)
    {
    DWORD dwRet;

    _asm
	{
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
|   Function:	    identify_processor					      |
|									      |
|   Description:    Tell which generation of processor we're running on	      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    Processor generation:			              |
|                        0 = 8086					      |
|                        1 = 80186					      |
|                        2 = 80286					      |
|                        Etc...					              |
|                                                                             |
|   Notes:	    Assume that under Win32, the processor is a Pentium	      |
|                   or better.                                                |
|                                                                             |
|   History:								      |
|    2010-09-06 JFL Created this Win32 version.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32
int identify_processor(void)
    {
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

