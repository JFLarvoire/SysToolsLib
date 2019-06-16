/*****************************************************************************\
*                                                                             *
*   Filename:	    stversion.h						      *
*                                                                             *
*   Description:    Get SysToolslib components versions			      *
*                                                                             *
*   Notes:	    Include this file in the main module of a program,        *
*                   to define the routine char *version(int idetailed).       *
*                                                                             *
*   History:								      *
*    2019-04-16 JFL jf.larvoire@hp.com created this file.                     *
*		    Moved here the OS identification strings from debugm.h.   *
*		    Added the definitions for the minimum OS version, and     *
*		    the processor name.					      *
*		    							      *
*        (C) Copyright 2016 Hewlett Packard Enterprise Development LP         *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef	_STVERSION_H
#define	_STVERSION_H	1
                                               

#define SYSTOOLSLIB_VERSION         2019,6,15,0
#define SYSTOOLSLIB_VERSION_STR     "2019-06-15"
#define SYSTOOLSLIB_NAME            "System Tools Library"


/******************** OS identification string definition ********************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */
#  ifndef _BIOS
#    define EXE_OS_NAME "DOS"
#  else
#    define EXE_OS_NAME "BIOS"
#  endif
#  ifndef _M_I86TM
#    define EXE_SUFFIX ".exe"
#  else
#    define EXE_SUFFIX ".com"
#  endif
#endif /* _MSDOS */

#ifdef _OS2		/* To be defined on the command line for the OS/2 version */
#  define EXE_OS_NAME "OS/2"
#  define EXE_SUFFIX ".exe"
#endif /* _OS2 */

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#  if WINVER < 0x0400
#    define MIN_OS_NAME "NT3"
#  elif WINVER == 0x0400
#    define MIN_OS_NAME "Win95/NT4"
#  elif WINVER == 0x0401
#    define MIN_OS_NAME "Win98"
#  elif WINVER == 0x0500
#    define MIN_OS_NAME "Win2000"
#  elif WINVER == 0x0501
#    define MIN_OS_NAME "XP"
#  elif WINVER == 0x0502
#    define MIN_OS_NAME "Server2003"
#  elif WINVER == 0x0600
#    define MIN_OS_NAME "Vista"
#  elif WINVER == 0x0601
#    define MIN_OS_NAME "Win7"
#  elif WINVER == 0x0602
#    define MIN_OS_NAME "Win8"
#  elif WINVER == 0x0603
#    define MIN_OS_NAME "Win8.1"
#  elif WINVER == 0x0A00
#    define MIN_OS_NAME "Win10"
#  else
#    undef MIN_OS_NAME
#  endif

#  if defined(_M_ARM64)
#    define EXE_PROC_NAME "arm64"
#  elif defined(_M_ARM)
#    define EXE_PROC_NAME "arm"
#  elif defined(_M_AMD64) || defined(_M_X64)
#    define EXE_PROC_NAME "amd64"
#  elif defined(_M_IX86)
#    define EXE_PROC_NAME "x86"
#  elif defined(_M_IA64)
#    define EXE_PROC_NAME "ia64"
#  elif defined(RC_INVOKED) && defined(MACHINE)
#    define EXE_PROC_NAME MACHINE
#  else
#    undef EXE_PROC_NAME
#  endif

#  if defined(__MINGW64__)
#    define EXE_OS_NAME "MinGW64"
#  elif defined(__MINGW32__)
#    define EXE_OS_NAME "MinGW32"
#  elif defined(_WIN64)
#    define EXE_OS_NAME "Win64"
#  else
#    if defined(WITH_DOS_STUB)
#      define EXE_OS_NAME "DOS+Win32"
#    else
#      define EXE_OS_NAME "Win32"
#    endif
#  endif

#  define EXE_SUFFIX ".exe"

#endif /* _WIN32 */

#ifdef __unix__		/* Automatically defined when targeting a Unix app. */
#  if defined(__CYGWIN64__)
#    define EXE_OS_NAME "Cygwin64"
#  elif defined(__CYGWIN32__)
#    define EXE_OS_NAME "Cygwin"
#  elif defined(_TRU64)
#    define EXE_OS_NAME "Tru64"	/* 64-bits Alpha Tru64 */
#  elif defined(__linux__)
#    define EXE_OS_NAME "Linux"
#  else
#    define EXE_OS_NAME "Unix"
#  endif
#  define EXE_SUFFIX ""
#endif /* __unix__ */

/**************** End of OS identification string definition *****************/

/* Define the SysToolsLib standard program version strings */

#if defined(_DEBUG)
#  define AND_DEBUG_VERSION " debug version"
#else
#  define AND_DEBUG_VERSION ""
#endif

/* Name and short version. Used in help screens */
#define PROGRAM_NAME_AND_VERSION					\
	"\n" PROGRAM_NAME EXE_SUFFIX					\
	/* " version " PROGRAM_VERSION */				\
        /* DEBUG_VERSION        If used SysToolsLib's debugm.h */	\
        AND_DEBUG_VERSION						\

/* Parts of the detailed version defined further down. */
#if defined(PROGRAM_VERSION)	/* We must have at least one of PROGRAM_VERSION and AND_PROGRAM_DATE */
#  define VERSION_SPACE " "	/* If we have both, they must be separated by a space */
#else
#  define PROGRAM_VERSION ""
#  define VERSION_SPACE ""
#endif
#if defined(PROGRAM_DATE)	/* Date of last change */
#  define AND_PROGRAM_DATE VERSION_SPACE PROGRAM_DATE
#else
#  define AND_PROGRAM_DATE ""
#endif
#if defined(EXE_OS_NAME)	/* MsvcLibX target OS name (DOS|WIN32|WIN64|...) */
#  define AND_EXE_OS_NAME " " EXE_OS_NAME
#else
#  define AND_EXE_OS_NAME ""
#endif
#if defined(MIN_OS_NAME)	/* Minimum OS version */
#  define AND_MIN_OS_NAME " >= " MIN_OS_NAME
#else
#  define AND_MIN_OS_NAME ""
#endif
#if defined(EXE_PROC_NAME)	/* Processor name */
#  define AND_EXE_PROC_NAME " " EXE_PROC_NAME
#else
#  define AND_EXE_PROC_NAME ""
#endif
#undef AND_DEBUG_VERSION
#if defined(_DEBUG)
#  define AND_DEBUG_VERSION " Debug"
#else
#  define AND_DEBUG_VERSION ""
#endif
#if defined(BIOS_LIB)		/* If used BiosLib */
#include "bioslib_version.h"
#  define AND_BIOSLIB_VERSION " ; BiosLib " BIOSLIB_VERSION
#else
#  define AND_BIOSLIB_VERSION ""
#endif
#if defined(_LODOS_LIB)		/* If used LoDosLib */
#include "lodoslib_version.h"
#  define AND_LODOSLIB_VERSION " ; LoDosLib " LODOSLIB_VERSION
#else
#  define AND_LODOSLIB_VERSION ""
#endif
#if defined(_PMODE_LIB)		/* If used PModeLib */
#include "pmodelib_version.h"
#  define AND_PMODELIB_VERSION " ; PModeLib " PMODELIB_VERSION
#else
#  define AND_PMODELIB_VERSION ""
#endif
#if defined(_MSVCLIBX_H_)	/* If used MsvcLibX */
#include "msvclibx_version.h"
#  define AND_MSVCLIBX_VERSION " ; MsvcLibX " MSVCLIBX_VERSION
#else
#  define AND_MSVCLIBX_VERSION ""
#endif
#if defined(__SYSLIB_H__)	/* If used SysLib */
#include "syslib_version.h"
#  define AND_SYSLIB_VERSION " ; SysLib " SYSLIB_VERSION
#else
#  define AND_SYSLIB_VERSION ""
#endif

/* Detailed version. Used for the -V option. */
#define DETAILED_VERSION							     \
	PROGRAM_VERSION								     \
	AND_PROGRAM_DATE     /* Date of last change */                       	     \
	AND_EXE_OS_NAME      /* MsvcLibX target OS name (DOS|WIN32|WIN64|...) */     \
	AND_MIN_OS_NAME      /* Minimum OS version */                                \
	AND_EXE_PROC_NAME    /* Processor name */                                    \
        AND_DEBUG_VERSION    /* If defined(_DEBUG) */				     \
	AND_BIOSLIB_VERSION  /* If used BiosLib */                                   \
	AND_LODOSLIB_VERSION /* If used LoDosLib */                                  \
	AND_PMODELIB_VERSION /* If used PModeLib */                                  \
	AND_MSVCLIBX_VERSION /* If used MsvcLibX */                                  \
	AND_SYSLIB_VERSION   /* If used SysLib */                                    \

#endif /* !defined(_STVERSION_H) */

