﻿/*****************************************************************************\
*		    							      *
*   Filename:	    dirsize.c						      *
*		    							      *
*   Description:    Display the amount of space used by a directory	      *
*		    							      *
*   Notes:	    Uses MsvcLibX library debugm.h debugging macros.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug	# Debug ver.  *
*		    							      *
*		    To build in DOS/Windows with MSVC tools, first build the  *
*		    MsvcLibX library.					      *
*		    Then run configure.bat, then (make.bat dirsize.exe).      *
*		    							      *
*   History:								      *
*    1986-09-03 JFL jf.larvoire@hp.com created this program.		      *
*    1989-10-12 JFL Added the recursion algorithm.			      *
*    1995-06-13 JFL Rewritten based on DIRC.C to support OS/2 and Win32.      *
*    1995-12-22 JFL Fixed a problem with long file names. Version 2.01.       *
*    2000-03-24 JFL Support files up to 4 GB, and disks up to 1000 GB.	      *
*		    Version 2.10.					      *
*    2001-04-10 JFL Fixed a bug when displaying sizes with intermediates 0s.  *
*                   Fixed and documented option -b.                           *
*		    Version 2.11.					      *
*    2012-01-09 JFL Skip inaccessible subdirectories.                         *
*		    Version 2.12.					      *
*    2012-01-12 JFL Restructured to use std C library routines from dirent.h. *
*                   Removed options -p. Added options -d, -v, -V.	      *
*                   Rewrote the debugging, using the new macros in debug.h.   *
*		    Added support for Unix.			              *
*    2012-01-19 JFL Fixed access to directories with non-ASCII names in WIN32.*
*		    Version 3.0.					      *
*    2012-01-23 JFL Fixed the canonic directory names display.                *
*		    Added options -H, -k, m, -g to specify dir sizes formats. *
*		    Version 3.0.1.					      *
*    2012-02-04 JFL Changed version format to: VERSION DATE OS [DEBUG]	      *
*		    Added version strings for more OS.                        *
*		    Version 3.0.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 3.0.3.                 *
*    2013-03-24 JFL Rebuilt with MsvcLibX.lib with support for UTF-8 names.   *
*		    Version 3.1.					      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 3.1.1.					      *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 3.1.2.  					      *
*    2016-09-23 JFL Use macro CDECL to allow building a DOS .com version.     *
*		    Version 3.1.3.  					      *
*    2017-10-02 JFL Fixed a conditional compilation bug in MSDOS.	      *
*		    Version 3.1.4.    					      *
*    2017-10-30 JFL Minor changes to the debugging output.		      *
*    2017-10-31 JFL Added options -i and -I. Changed the default back to      *
*		    iContinue = FALSE, negating the 2012-01-09 decision above.*
*		    Version 3.2.					      *
*    2018-04-29 JFL Make sure WIN32 pathname buffers are larger than 260 B.   *
*		    Changed option -d to -D, and -debug to -d.		      *
*    2018-04-30 JFL Check errors for all calls to chdir() and getcwd().	      *
*		    Rewrote finis() so that it displays errors internally.    *
*		    Version 3.3.					      *
*    2018-05-31 JFL Changed #if DIRENT2STAT_DEFINED to _DIRENT2STAT_DEFINED.  *
*		    Version 3.3.1.					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.3.3.2.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 3.3.3.      *
*    2020-03-16 JFL Fixed issue with Unix readdir() not always setting d_type.*
*                   Version 3.3.4.					      *
*    2020-04-20 JFL Added support for MacOS. Version 3.4.                     *
*    2021-02-27 JFL Fixed another issue with Unix readdir() and d_type.       *
*                   Version 3.4.1.					      *
*    2021-09-06 JFL Fixed "out of directory handles" error with the -i option.*
*                   Report the # of inaccessible dirs if any err. was ignored.*
*                   Continue by default for all recursive operations.	      *
*                   Version 3.5.					      *
*    2022-01-12 JFL Added option -f to follow links to directories. Ver. 3.6. *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 3.6.1.		      *
*    2023-11-16 JFL Bugfix: In case of error, pList may be used after realloc.*
*                   Version 3.6.2.					      *
*    2024-06-21 JFL Restructured to use SysLib's WalkDirTree(), to fix errors *
*		    when reading huge directories with > 200.000 files.       *
*    2024-07-19 JFL Renamed structures to make the code easier to understand. *
*    2024-10-17 JFL Use the UNUSED_ARG() macro in config.h.                   *
*    2024-10-22 JFL Abort the walk and cleanup properly after a Ctrl-C.       *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Display the total size used by a directory"
#define PROGRAM_NAME    "dirsize"
#define PROGRAM_VERSION "3.99"
#define PROGRAM_DATE    "2024-10-22"

#include <config.h>	/* OS and compiler-specific definitions */

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <inttypes.h>		/* Compiler-specific integer format types */
#include <sys/types.h>		/* OS-specific types (Dependant on _FILE_OFFSET_BITS) */
#include <sys/stat.h>		/* We use the stat function and structure */
#include <time.h>		/* We use the time_t structure */
#include <dirent.h>		/* We use the DIR type and the dirent structure */
#include <fnmatch.h>		/* We use wild card file name matching */
#include <unistd.h>		/* For chdir() */
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "dirx.h"	/* SysLib Directory access functions eXtensions */
#include "pathnames.h"	/* SysLib pathname management functions */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#ifndef UINTMAX_MAX /* For example Tru64 doesn't define it */
typedef unsigned long uintmax_t;
#endif

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

/* CDECL is defined in SysLib.h, which is included indirectly by SysLib's *.h */

#include <conio.h>	/* For _kbhit() */

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

/* CDECL is defined in WinDef.h, which is already included by MsvcLibX */

#endif /* defined(_WIN32) */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* Automatically defined when targeting an OS/2 application? */

#define CDECL __cdecl

#endif /* defined(_OS2) */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define CDECL				/* No such thing needed for Linux builds */

#endif /* defined(__unix__) */

/*********************************** Other ***********************************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(EXE_OS_NAME))
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

/* Flag OSs that have links (For some OSs which don't, macros are defined, but S_ISLNK always returns 0) */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK)
  #define OS_HAS_LINKS 1
#else
  #define OS_HAS_LINKS 0
#endif

/* Local definitions */

#define PATHNAME_SIZE PATH_MAX		/* Buffer size for holding pathnames, including NUL */
#define NODENAME_SIZE (NAME_MAX+1)	/* Buffer size for holding file names, including NUL */

#if PATHNAME_SIZE < 1000 /* Simplified implementation for MSDOS */
  #define PATHNAME_BUFS_IN_HEAP 0	/* Alloc them on the stack */
  #define NEW_PATHNAME_BUF(var) char var[PATHNAME_SIZE]
#else	/* Avoid stack overflows by storing too large variables on stack */
  #define PATHNAME_BUFS_IN_HEAP 1	/* Alloc them in the memory heap */
  #define NEW_PATHNAME_BUF(var) char *var = malloc(PATHNAME_SIZE)
#endif
#if !PATHNAME_BUFS_IN_HEAP
  #define TRIM_PATHNAME_BUF(var) do {} while (0)
  #define FREE_PATHNAME_BUF(var) do {} while (0)
#else
  #define TRIM_PATHNAME_BUF(var) do {char *p = realloc(var, strlen(var)+1); if (p) var = p;} while (0)
  #define FREE_PATHNAME_BUF(var) free(var);
#endif

/* Define a type for the total size. Must be at least 48bits for large disk sizes. */
/* Note: constant _INTEGRAL_MAX_BITS >= 64 is not defined on some Unix systems. */
#ifdef INT64_MAX
#define total_t uint64_t		/* Large enough for even very large disks */
#define TOTAL_T_IS_INT 1
#define TOTAL_FMT PRIu64
#else
#define total_t double			/* DOS does not support 64-bits int */
#define TOTAL_T_IS_INT 0
#define TOTAL_FMT ".0f"
#include <math.h>			/* We use floor() and fmod() */
#endif

#define MISMATCH (-32767)

#define RETCODE_SUCCESS 0               /* Return codes processed by finis() */
#define RETCODE_NO_MEMORY 1
#define RETCODE_INACCESSIBLE 2
#define RETCODE_CTRL_C 3

#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

typedef struct _selectOpts {	/* Options for selecting files */
  char *pattern;		    /* Wildcards pattern. NULL = select all */
  time_t datemin;		    /* Minimum timestamp. 0 = no minimum */
  time_t datemax;		    /* Maximum timestamp. 0 = no maximum */
} selectOpts;

typedef struct _scanOpts {	/* Options for scanning the directory tree */
  int recur;			    /* If TRUE, list subdirectories recursively */
  int total;			    /* If TRUE, totalize size of all subdirs */
  int subdirs;			    /* If TRUE, start scanning in each subdirectory */
  int depth;			    /* Current depth in the scan tree */
  int nErrors;			    /* Number of errors that were ignored */
#if OS_HAS_LINKS
  int follow;			    /* If TRUE, follow links to subdirectories */
#endif /* OS_HAS_LINKS */
} scanOpts;

/* Global variables */

char init_dir[PATHNAME_SIZE];       /* Initial directory */
char init_drive;                    /* Initial drive */
long csz=0;			    /* Cluster size. 0=Unknown. */
int band = FALSE;		    /* If TRUE, skip a line every 5 lines */
int iContinue = -1;                 /* If TRUE, continue after directory access errors */
int iQuiet = FALSE;		    /* If TRUE, only display major errors */
int iVerbose = FALSE;		    /* If TRUE, display additional information */
int iHuman = TRUE;		    /* If TRUE, display human-friendly values with a comma every 3 digits */
char *pszUnit = "B";		    /* "B"=bytes; "KB"=Kilo-Bytes; "MB"; GB" */
volatile int iCtrlC = FALSE;	    /* If TRUE, a Ctrl-C has been detected */

/* Function prototypes */

void usage(void);                   /* Display a brief help and exit */
void finis(int retcode, ...);       /* Return to the initial drive & exit */
int parse_date(char *token, time_t *pdate); /* Convert the argument to a time_t */
int Size2String(char *pBuf, total_t ll); /* Convert size to a decimal, with a comma every 3 digits */
int Size2StringWithUnit(char *pBuf, total_t llSize); /* Idem, appending the user-specified unit */

total_t ScanFiles(scanOpts *pScanOpts, selectOpts *pSelectOpts); /* Scan the current dir */
total_t ScanDirs(scanOpts *pScanOpts, selectOpts *pSelectOpts);  /* Scan every subdir */
void affiche(char *path, total_t size);/* Display sorted list */
int scandirX(const char *pszName,
	     struct dirent ***resultList,
	     int (*cbSelect) (const struct dirent *, void *pRef),
	     int (CDECL *cbCompare) (const struct dirent **, const struct dirent **),
	     void *pRef);	    /* scandir() extension, passing a pRef to cbSelect() */

long GetClusterSize(char drive);    /* Get cluster size */
void OnControlC(int iSignal);	    /* Ctrl-C signal handler */

/******************************************************************************
*                                                                             *
*                               Main routine                                  *
*                                                                             *
******************************************************************************/

int main(int argc, char *argv[]) {
  char *from = NULL;		/* What directory to list */
  int i;
  selectOpts sSelectOpts = {0};/* Constraints. None by default */
  scanOpts sScanOpts = {0};		/* Scanning options. All disabled by default */
  char *dateminarg = NULL;	/* Minimum date argument */
  char *datemaxarg = NULL;	/* Maximum date argument */
  int iUseCsz = FALSE;		/* If TRUE, use the cluster size */
  int err;
  char *pc;
  total_t size;			/* Total size */

  /* Parse command line arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) { /* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "b")) {
	band = TRUE;
	continue;
      }
      if (streq(opt, "c")) {
	iUseCsz = TRUE; /* Use the cluster size for size calculations */
	if (   ((i+1) < argc)
	    && sscanf(argv[i+1], "%ld", &csz)
	    && (csz != 0)) {
	  i += 1;		/* Skip the size in next argument */
	}
      continue;
      }
      if (streq(opt, "D")) {
	sScanOpts.subdirs = TRUE;
	continue;
      }
#ifdef _DEBUG
      if (streq(opt, "d")) {
	DEBUG_ON();
	continue;
      }
#endif
#if OS_HAS_LINKS
      if (streq(opt, "f")) {
	sScanOpts.follow = TRUE;
	continue;
      }
#endif
      if (streq(opt, "from")) {
	dateminarg = argv[++i];
	if (!parse_date(dateminarg, &sSelectOpts.datemin)) {
	  fprintf(stderr, "Error: Invalid date format: -from %s\n", dateminarg);
	  dateminarg = NULL;
	}
	continue;
      }
      if (   streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "g")) {
	pszUnit = "GB";
	continue;
      }
      if (streq(opt, "H")) {
	iHuman = FALSE;
	continue;
      }
      if (streq(opt, "i")) {
	iContinue = TRUE;
	continue;
      }
      if (streq(opt, "I")) {
	iContinue = FALSE;
	continue;
      }
      if (streq(opt, "k")) {
	pszUnit = "KB";
	continue;
      }
      if (streq(opt, "m")) {
	pszUnit = "MB";
	continue;
      }
      if (streq(opt, "nologo")) continue; /* Old option retired */
      if (streq(opt, "q")) {
	iQuiet = TRUE;
	continue;
      }
      if (   streq(opt, "r")
      	  || streq(opt, "s")) {
	sScanOpts.recur = TRUE;
	continue;
      }
      if (streq(opt, "t")) {
	sScanOpts.total = TRUE;
	continue;
      }
      if (streq(opt, "T")) {
	sScanOpts.total = FALSE;
	continue;
      }
      if (streq(opt, "to")) {
	datemaxarg = argv[++i];
	if (!parse_date(datemaxarg, &sSelectOpts.datemax)) {
	  fprintf(stderr, "Error: Invalid date format: -to %s\n", datemaxarg);
	  datemaxarg = NULL;
	}
	continue;
      }
      if (streq(opt, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {
	puts(DETAILED_VERSION);
	exit (0);
      }
      fprintf(stderr, "Warning: Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */

    /* If it's an argument */
    if (!from) {
      from = arg;
      continue;
    }
    fprintf(stderr, "Warning: Unexpected argument \"%s\" ignored.", arg);
  }

  /* If not explicitely defined, set iContinue based on context */
  if ((iContinue == -1) && (sScanOpts.total || sScanOpts.recur)) iContinue = TRUE; /* For all recursive operations, default to TRUE */
  if (iContinue == -1) iContinue = FALSE; /* Fon non-recusive operations, default to FALSE */

  /* Extract the search pattern if provided as part of the target pathname */
  if (from) {
    struct stat st;
    err = stat(from, &st);
    if (err || ((st.st_mode & S_IFMT) != S_IFDIR)) { /* If this is not a dir */
      pc = strrchr(from, DIRSEPARATOR_CHAR);
      if (!pc) {
	sSelectOpts.pattern = from;
	from = NULL;	/* Contrary to our initial guess, there's no dir name */
      } else {
	*pc = '\0';	/* Cut the wildcards pattern off the directory name */
	sSelectOpts.pattern = pc+1;
      }
    }
  }

#if defined(_OS2) 
/* Make sure to include os2.h at the beginning of this file, and before that
  to define the INCL_DOSMISC constant to enable the necessary section */
  DEBUG_IF_IS_ON(
    uint8_t mode;

    DosGetMachineMode(&mode);
    switch (mode) {
      case MODE_REAL:
	printf("Running in DOS mode.\n");
	break;
      case MODE_PROTECTED:
	printf("Running in OS/2 mode.\n");
	break;
      default:
	printf("Running neither in DOS nor in OS/2 mode!?!\n");
	break;
    }
  )
#endif

#if defined(_DEBUG)
  if (DEBUG_IS_ON()) {
    DEBUG_PRINT_MACRO(_LFS_LARGEFILE);
#if TOTAL_T_IS_INT
    printf("// total_t is uintmax_t\n");
#else
    printf("// total_t is double\n");
#endif
    printf("// uintmax_t is %d bits\n", (int)(8 * sizeof(uintmax_t)));
    printf("// stat.st_size is %d bits\n", (int)(8 * sizeof(((struct stat *)NULL)->st_size)));
    printf("// time_t is %d bits\n", (int)(8 * sizeof(time_t)));
    printf("// stat.st_mtime is %d bits\n", (int)(8 * sizeof(((struct stat *)NULL)->st_mtime)));
  }
#endif

  /* Get the initial drive, and change to the target drive if provided */
#if HAS_DRIVES
  init_drive = (char)getdrive();
  if (from && (from[1] == ':')) {
    if (from[0] >= 'a') from[0] -= 'a' - 'A';
    DEBUG_PRINTF(("chdrv(%d); // Drive %c:'\n", from[0] - '@', from[0]));
    err = chdrive(from[0] - '@');
    if (err) {
      finis(RETCODE_INACCESSIBLE, "Cannot access drive %c", from[0]);
    }
    from += 2;
  }
#endif

  /* Current directory on the target drive */
  pc = getcwd(init_dir, PATHNAME_SIZE);
  if (!pc) {
    finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");
  }

  /* Make sure to restore the initial drive/directory in case of a Ctrl-C */
  signal(SIGINT, OnControlC);

  /* Go to the target directory */
  if (from && from[0]) {
#if !HAS_MSVCLIBX
    DEBUG_PRINTF(("chdir(\"%s\");\n", from));
#endif
    err = chdir(from);
    if (err) {
      finis(RETCODE_INACCESSIBLE, "Cannot access directory %s", from);
    }
  }

  /* Check the cluster size on the target drive (and directory for Linux) */
  if (iUseCsz) {
    if (!csz) {
      csz = GetClusterSize(0);     /* Cluster size of current drive */
    }
    if (iVerbose) printf("The cluster size is %ld bytes.\n\n", csz);
  }

  /* Compute the files sizes */
  if (!sScanOpts.subdirs) {
    size = ScanFiles(&sScanOpts, &sSelectOpts);
    if ((!sScanOpts.recur) && (!iCtrlC)) { /* If Ctrl-C pressed, the computed size is incomplete */
      char szBuf[40];
      Size2StringWithUnit(szBuf, size);
      printf("%s\n", szBuf);
    }
  } else {
    size = ScanDirs(&sScanOpts, &sSelectOpts);
  }

  /* Report if some errors were ignored */
  if (sScanOpts.nErrors) {
    finis(RETCODE_INACCESSIBLE, "Incomplete results: Missing data for %d directories", sScanOpts.nErrors);
  }

  /* Restores the initial drive and directory and exit */
  finis(RETCODE_SUCCESS);
  return 0;
}

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: dirsize [SWITCHES] [TARGET]\n\
\n\
Switches:\n\
  -?|-h       Display this help message and exit.\n\
  -b          Skip a line every 5 lines, to improve readability.\n\
  -c          Use the actual cluster size to compute the total size.\n\
  -c size     Use the specified cluster size to compute the total size.\n\
  -D          Measure every subdirectory of the target directory.\n"
#ifdef _DEBUG
"\
  -d          Output debug information.\n"
#endif
#if OS_HAS_LINKS
"\
  -f          Follow links to directories.\n"
#endif
"\
  -from Y-M-D List only files starting from that date.\n\
  -g          Display sizes in Giga bytes.\n\
  -H          Display sizes without the human-friendly commas.\n\
  -i          Report only the number of access errors (Dflt for recursive ops.)\n\
  -I          Stop in case of directory access error (Default for other ops.)\n\
  -k          Display sizes in Kilo bytes.\n\
  -m          Display sizes in Mega bytes.\n\
  -q          Quiet mode: Do not display minor errors.\n\
  -r|-s       Display the sizes of all subdirectories too.\n\
  -t          Count the total size of all files plus that of all subdirs.\n\
  -T          Do not count the size of subdirs. (Default)\n\
  -to Y-M-D   List only files up to that date.\n\
  -v          Display verbose information.\n\
  -V          Display this program version and exit.\n\
\n\
Target:       PATHNAME|PATTERN|PATHNAME" DIRSEPARATOR_STRING "PATTERN\n\
Pathname:     Target directory pathname. Default: current directory\n\
Pattern:      Wildcards pattern. Default: " PATTERN_ALL "\n\
"
#include "footnote.h"
);

  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    finis						      |
|									      |
|   Description:    Display opt. msgs; Restore current path and drv, and exit.|
|									      |
|   Parameters:     retcode	The exit code				      |
|		    pszFormat	Optional format string to pass to printf      |
|		    ...		Optional printf arguments		      |
|		    							      |
|   Notes:	    If retcode is not 0, but no error message is needed, it   |
|		    is necessary to pass a NULL for pszFormat.		      |
|		    							      |
|   Returns:	    Does not return					      |
|									      |
|   History:								      |
|    2018-04-30 JFL Redesigned with optional error messages to display.       |
*									      *
\*---------------------------------------------------------------------------*/

void finis(int retcode, ...) {
  if (iCtrlC && (retcode != RETCODE_CTRL_C)) OnControlC(0); /* Report a soft Ctrl-C stop */
    
  if (retcode) { /* There might be an error message to report */
    va_list vl;
    char *pszFormat;

    va_start(vl, retcode);
    pszFormat = va_arg(vl, char *);
    if (pszFormat) { /* There is an error message to report */
      fputs("dirsize: ", stderr);
      if (retcode != RETCODE_CTRL_C) fputs("Error: ", stderr);
      vfprintf(stderr, pszFormat, vl);
      fputs(".\n", stderr);
    }
    va_end(vl);
  }

  DEBUG_PRINTF(("finis(): Returning to the initial directory.\n"));
  chdir(init_dir);	/* Don't test errors, as we're likely to be here due to another error */
#if HAS_DRIVES
  chdrive(init_drive);
#endif

  exit(retcode);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    OnControlC						      |
|									      |
|   Description:    Restore the initial directory before aborting on a Ctrl-C |
|									      |
|   Parameters:     							      |
|		    							      |
|   Notes:	    Warning: Called asynchronously from the foreground thread,|
|		    so error messages output in this signal thread might be   |
|		    mixed with the foreground thread output.		      |
|		    							      |
|   Returns:	    Nothing						      |
|									      |
|   History:								      |
|    2024-10-22 JFL Added this routine.                                       |
*									      *
\*---------------------------------------------------------------------------*/

void OnControlC(int iSignal) {
  UNUSED_ARG(iSignal);
  if (!iCtrlC) {	/* The first time, try aborting the walk softly */
    iCtrlC = TRUE;
  } else {		/* The following times, do a hard stop now */
    fflush(stdout); /* Unsuccessful attempt at preventing the mixing of the foreground output */
    finis(RETCODE_CTRL_C, "Ctrl-C detected, aborting");
  }
}

/******************************************************************************
*                                                                             *
*       Function:       ScanFiles                                             *
*                                                                             *
*       Description:    Scan the current directory, and add-up file sizes     *
*                                                                             *
*       Arguments:                                                            *
*         void *pConstraints	File selection constraints                    *
*                                                                             *
*       Return value:   Total size of all files                               *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       History:                                                              *
*                                                                             *
******************************************************************************/

typedef struct {
  selectOpts *pSelectOpts;
  total_t size;
  total_t nFiles;
} selectFiles;

int SelectFilesCB(const char *pszPathname, const struct dirent *pDE, void *p) {
  selectFiles *pSelectFiles = p;
  selectOpts *pSelectOpts = pSelectFiles->pSelectOpts;
  struct stat sStat;                         
  uintmax_t fsize;
  int iErr;

  UNUSED_ARG(pszPathname);

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
  _kbhit();		/* Side effect: Forces DOS to check for Ctrl-C */
#endif
  if (iCtrlC) return TRUE;	/* Abort scan */

  /* WalkDirTree() uses readdirx(), so d_type always valid, even under Unix */
  if (pDE->d_type != DT_REG) return FALSE;	/* We want only files */

#if _DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
  iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
  iErr = lstat(pDE->d_name, &sStat);
#endif
  if (iErr) return FALSE;	/* Ignore suspect entries */

  /* Skip files outside date range */
  if (pSelectOpts->datemin && (sStat.st_mtime < pSelectOpts->datemin)) return FALSE;
  if (pSelectOpts->datemax && (sStat.st_mtime > pSelectOpts->datemax)) return FALSE;

  /* Skip files which don't match the wildcard pattern */
  if (pSelectOpts->pattern) {
    if (fnmatch(pSelectOpts->pattern, pDE->d_name, FNM_CASEFOLD) == FNM_NOMATCH) {
      return FALSE;
    }
  }

  /* OK, all criteria pass. */
  memset(&sStat, 0, sizeof(sStat));

#if _DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
  iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
  iErr = lstat(pDE->d_name, &sStat);
#endif
  if (iErr) return -1;
  DEBUG_PRINTF(("// Counting %10"PRIuMAX" bytes for %-32s\n", (uintmax_t)(sStat.st_size), pDE->d_name));
  fsize = sStat.st_size; /* Get the actual file size */
  if (csz) {	/* If the cluster size is provided */
	      /* Round it to the next cluster multiple */
    fsize += csz-1;
    fsize -= fsize % csz;
  }
  pSelectFiles->nFiles += 1;    /* Count files */
  pSelectFiles->size += fsize;  /* Totalize sizes */

  return FALSE; /* Continue scanning */
}

total_t ScanFiles(scanOpts *pScanOpts, selectOpts *pSelectOpts) {
  total_t size = 0;
  total_t dSize;
  NEW_PATHNAME_BUF(szCurDir);
  int iErr;
  wdt_opts wdtOpts = {0};
  selectFiles sSelectFiles = {0};
  int iResult;

  DEBUG_ENTER(("ScanFiles(%p);\n", pSelectOpts));

#if PATHNAME_BUFS_IN_HEAP
  if (!szCurDir) {
    finis(RETCODE_NO_MEMORY, "Out of memory");
  }                                                        
#endif

  /* Scan all files */
  wdtOpts.iFlags = WDT_CONTINUE	| WDT_QUIET | WDT_NORECURSE;
  sSelectFiles.pSelectOpts = pSelectOpts;
  iResult = WalkDirTree(".", &wdtOpts, SelectFilesCB, &sSelectFiles);
  if (iResult < 0) { /* An error occurred */
    iErr = errno;
    if (iVerbose || !iContinue) {
      char *pszSeverity = iContinue ? "Warning" : "Error";
      char *pcd = getcwd(szCurDir, PATHNAME_SIZE); /* Canonic name of the target directory */
      if (!pcd) {
	finis(RETCODE_INACCESSIBLE, "Cannot get the current directory. %s", strerror(errno));
      }
      fprintf(stderr, "%s: Failed to scan files in %s. %s\n", pszSeverity, szCurDir, strerror(iErr));
    }
    if ((iErr == EACCES) && iContinue) {
      pScanOpts->nErrors += 1;
      return 0;
    }
    iErr = (iErr == EACCES) ? RETCODE_INACCESSIBLE : RETCODE_NO_MEMORY;
    finis(iErr, NULL); /* The error message has already been displayed */
  }
  size += sSelectFiles.size;
  /* printf("// Processed %ld files\n", (long)sSelectFiles.nFiles); */

  /* Optionally scan all subdirectories, unless aborting with iResult = 1 */
  if ((pScanOpts->recur || pScanOpts->total) && (iResult < 1)) {
    pScanOpts->depth += 1;
    dSize = ScanDirs(pScanOpts, pSelectOpts);
    pScanOpts->depth -= 1;
    if (pScanOpts->total) size += dSize;  /* Totalize sizes */
    if (pScanOpts->recur) {
      char *pcd = getcwd(szCurDir, PATHNAME_SIZE); /* Canonic name of the target directory */
      if (!pcd) {
	finis(RETCODE_INACCESSIBLE, "Cannot get the current directory. %s", strerror(errno));
      }
      TRIM_PATHNAME_BUF(szCurDir);
      affiche(szCurDir, size);
    }
  }

  FREE_PATHNAME_BUF(szCurDir);
  DEBUG_LEAVE(("return %" TOTAL_FMT ";\n", size));
  return size;
}

/******************************************************************************
*                                                                             *
*       Function:       ScanDirs                                              *
*                                                                             *
*       Description:    Scan subdirectories, and add-up file sizes            *
*                                                                             *
*       Arguments:                                                            *
*         void *pConstraints	File selection constraints                    *
*                                                                             *
*       Return value:   Total size of all files                               *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       History:                                                              *
*                                                                             *
******************************************************************************/

typedef struct {
  scanOpts *pScanOpts;
  selectOpts *pSelectOpts;
  total_t size;
  total_t nFiles;
} selectDirs;

int SelectDirsCB(const char *pszPathname, const struct dirent *pDE, void *p) {
  selectDirs *pSelectDirs = p;
  scanOpts *pScanOpts = pSelectDirs->pScanOpts;
  selectOpts *pSelectOpts = pSelectDirs->pSelectOpts;
  int isDir = FALSE;
  total_t dSize = 0;
  struct stat sStat;
  int iErr;
  NEW_PATHNAME_BUF(szCurDir);
  int iRet;

  UNUSED_ARG(pszPathname);

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
  _kbhit();		/* Side effect: Forces DOS to check for Ctrl-C */
#endif
  if (iCtrlC) {
    iRet = TRUE;
    goto exit_SelectDirsCB;	/* Abort scan */
  }

  do {
    /* WalkDirTree() uses readdirx(), so d_type always valid, even under Unix */
    if (   (pDE->d_type == DT_DIR)	/* We want only directories */
	&& (!streq(pDE->d_name, "."))	/* Except . */
	&& (!streq(pDE->d_name, ".."))) { /* and .. */
      isDir = TRUE;
#if OS_HAS_LINKS
    } else if (pDE->d_type == DT_LNK) {
      if (pScanOpts->follow) {
	struct stat s;
	iErr = stat(pDE->d_name, &s);
	if (iErr) {
	  char *pszSeverity = iContinue ? "Warning" : "dirsize: Error";
	  if (iVerbose || !iContinue) {
	    fprintf(stderr, "%s: Invalid link \"%s" DIRSEPARATOR_STRING "%s\". %s\n", pszSeverity, pszPathname, pDE->d_name, strerror(errno));
	  }
	  if (!iContinue) finis(RETCODE_INACCESSIBLE, NULL); /* The error message has already been displayed */
	  pScanOpts->nErrors += 1;
	  isDir = FALSE;
	}
	isDir = S_ISDIR(s.st_mode);
      } else {
	isDir = FALSE;
      }
#endif /* OS_HAS_LINKS */
    } else {
      isDir = FALSE;
    }
  } while (0);

  if (isDir && !iCtrlC) {
#if _DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = lstat(pDE->d_name, &sStat);
#endif

#if !HAS_MSVCLIBX
    DEBUG_PRINTF(("chdir(\"%s\");\n", pDE->d_name));
#endif
    iErr = chdir(pDE->d_name);
    if (iErr) {
      char *pszSeverity = iContinue ? "Warning" : "dirsize: Error";
      if (iVerbose || !iContinue) {
	char *pcd = getcwd(szCurDir, PATHNAME_SIZE); /* Canonic name of the target directory */
	if (!pcd) {
	  finis(RETCODE_INACCESSIBLE, "Cannot get the current directory, nor access %s. %s", pDE->d_name, strerror(errno));
	}
      	fprintf(stderr, "%s: Cannot access directory %s" DIRSEPARATOR_STRING "%s. %s\n", pszSeverity, szCurDir, pDE->d_name, strerror(errno));
      }
      if (!iContinue) finis(RETCODE_INACCESSIBLE, NULL); /* The error message has already been displayed */
      pScanOpts->nErrors += 1;
    } else {
      dSize = ScanFiles(pScanOpts, pSelectOpts);
      if (!pScanOpts->depth) {
	char *pcd = getcwd(szCurDir, PATHNAME_SIZE); /* Canonic name of the target directory */
	if (!pcd) {
	  finis(RETCODE_INACCESSIBLE, "Cannot get the current directory. %s", strerror(errno));
	}
	/* Don't trim the pathname here, as we're in a loop, and subsequent paths might be longer */
	affiche(szCurDir, dSize);
      }
#if !HAS_MSVCLIBX
      DEBUG_PRINTF(("chdir(\"..\");\n"));
#endif
      iErr = chdir("..");
      if (iErr) {
	finis(RETCODE_INACCESSIBLE, "Cannot return to \"%s\" parent directory. %s", szCurDir, strerror(errno));
      }

      pSelectDirs->size += dSize;  /* Totalize sizes */
    }
  }

  iRet = FALSE; /* Continue scanning */
exit_SelectDirsCB:
  FREE_PATHNAME_BUF(szCurDir);
  return iRet;
}

/* Scan all subdirectories */
total_t ScanDirs(scanOpts *pScanOpts, selectOpts *pSelectOpts) {
  total_t size = 0;
  int iErr;
  wdt_opts wdtOpts = {0};
  selectDirs sSelectDirs = {0};
  int iResult;
  NEW_PATHNAME_BUF(szCurDir);

  DEBUG_ENTER(("ScanDirs(%p);\n", pSelectOpts));

#if PATHNAME_BUFS_IN_HEAP
  if (!szCurDir) {
    finis(RETCODE_NO_MEMORY, "Out of memory");
  }
#endif

  /* Get all subdirectories */
  /* But for now, manage recursion locally */
  wdtOpts.iFlags = WDT_CONTINUE	| WDT_QUIET | WDT_NORECURSE;
  sSelectDirs.pScanOpts = pScanOpts;
  sSelectDirs.pSelectOpts = pSelectOpts;
  iResult = WalkDirTree(".", &wdtOpts, SelectDirsCB, &sSelectDirs);
  if (iResult < 0) { /* An error occurred */
    iErr = errno;
    if (iVerbose || !iContinue) {
      char *pszSeverity = iContinue ? "Warning" : "Error";
      char *pcd = getcwd(szCurDir, PATHNAME_SIZE); /* Canonic name of the target directory */
      if (!pcd) {
	finis(RETCODE_INACCESSIBLE, "Cannot get the current directory. %s", strerror(errno));
      }
      fprintf(stderr, "%s: Failed to scan directories in %s. %s\n", pszSeverity, szCurDir, strerror(iErr));
    }
    if ((iErr == EACCES) && iContinue) {
      pScanOpts->nErrors += 1;
      goto exit_ScanDirs;
    }
    iErr = (iErr == EACCES) ? RETCODE_INACCESSIBLE : RETCODE_NO_MEMORY;
    finis(iErr, NULL); /* The error message has already been displayed */
  }
  size = sSelectDirs.size;

exit_ScanDirs:
  FREE_PATHNAME_BUF(szCurDir);
  DEBUG_LEAVE(("return %" TOTAL_FMT ";\n", size));
  return size;
}

/******************************************************************************
*                                                                             *
*   Function:       affiche                                                   *
*                                                                             *
*   Description:	Display the directory size found		      *
*                                                                             *
*   Arguments:                                                                *
*                                                                             *
*      char *path	Name of the directory				      *
*      total_t size	Size found					      *
*                                                                             *
*   Return value:   0=Success; !0=Failure                                     *
*                                                                             *
*   Notes:              Using a dedicated type allows to avoid using the      *
*                       floating point libraries if we can.                   *
*                                                                             *
*   History:                                                                  *
*    2001-04-10 JFL Fixed a bug when displaying sizes with intermediates 0s.  *
*    2012-01-17 JFL Made the size argument type a macro depending on the      *
*                   compiler capabilities.                                    *
*                                                                             *
******************************************************************************/

/* Display a huge integer with optional commas every three digits. */
int Size2String(char *pBuf, total_t llSize) {
  int n = 0;
  char *pszFormat = "%" TOTAL_FMT;

#if TOTAL_T_IS_INT
  if (llSize >= 1000) {
    n = Size2String(pBuf, llSize/1000);
    llSize = llSize % 1000;
    if (iHuman) pBuf[n++] = ',';
    pszFormat = "%03" TOTAL_FMT; /*  Display leading 0s for the following groups */
  }
#else /* type is double */
  if (llSize >= 1000.) {
    n = Size2String(pBuf, floor(llSize/1000.));
    llSize = floor(fmod(llSize, 1000.) + 0.5);	/* llSize %= 1000. */
    if (iHuman) pBuf[n++] = ',';
    pszFormat = "%03" TOTAL_FMT; /*  Display leading 0s for the following groups */
  }
#endif
  n += sprintf(pBuf+n, pszFormat, llSize);
  return n;
}

/* Convert Bytes to KBytes */
total_t b2k(total_t llSize) {
  llSize = (llSize + 1023) / 1024;
#if !TOTAL_T_IS_INT
  llSize = floor(llSize);
#endif
  return llSize;
}

/* Idem, using the user-specified unit. */
int Size2StringWithUnit(char *pBuf, total_t llSize) {
  int n;
  int iShowUnit = 1;

  switch (*pszUnit) {
  case 'B': iShowUnit = 0; break;
  case 'K': llSize = b2k(llSize); break;
  case 'M': llSize = b2k(b2k(llSize)); break;
  case 'G': llSize = b2k(b2k(b2k(llSize))); break;
  }

  n = Size2String(pBuf, llSize);

  if (iShowUnit) {
    n += sprintf(pBuf+n, " %s", pszUnit);
  }

  return n;
}

void affiche(char *path, total_t llSize) {
  static int group=0;
  char szSize[40];

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
  _kbhit();		/* Side effect: Forces DOS to check for Ctrl-C */
#endif
  if (iCtrlC) return;	/* Scan aborted, so the size is invalid */

  /* Display the size and path name */
  Size2StringWithUnit(szSize, llSize);
  printf("%15s  %s\n", szSize, path);

  if (band && (++group == 5)) {
    group = 0;
    printf("\n");
  }

  return;
}

/******************************************************************************
*                                                                             *
*       Function:       parse_date                                            *
*                                                                             *
*       Description:    Parse a date on a command line switch                 *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *token   Date token from the command line                      *
*         time_t *pDate Pointer to the date field to update                   *
*                                                                             *
*       Return value:   TRUE if done, FALSE if invalid date found             *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       History:                                                              *
*                                                                             *
******************************************************************************/

int parse_date(char *token, time_t *pDate) {
  unsigned int year, month, day, hour, minute, second;
  unsigned int olympiads; /* 4-year periods */
  unsigned long t = 0;

  if (sscanf(token, "%d-%d-%d", &year, &month, &day) < 3) return FALSE;
  hour = 0;
  minute = 0;
  second = 0 ;

  /*  Compute the year relative to 1980 */
  if (year < 1970) return FALSE;

  if (month < 1) return FALSE;
  if (month > 12) return FALSE;

  if (day < 1) return FALSE;
  if (day > 31) return FALSE;

  /* Count days */
  year -= 1970; /* Start of Unix time_t epoch */
  olympiads = year / 4;
  year = year % 4;
  t = olympiads * (365 + 365 + 366 + 365);
  switch (year) {
    case 3: t += 366;
    case 2: t += 365;
    case 1: t += 365;
  }
  switch (month) {
    case 12: t += 30;
    case 11: t += 31;
    case 10: t += 30;
    case 9: t += 31;
    case 8: t += 31;
    case 7: t += 30;
    case 6: t += 31;
    case 5: t += 30;
    case 4: t += 31;
    case 3: t += (year == 2) ? 29 : 28;
    case 2: t += 31;
  }
  t += day-1;

  /* Count seconds */
  t *= 24;
  t += hour;
  t *= 60;
  t += minute;
  t *= 60;
  t += second;

  *pDate = (time_t)t;
  return TRUE;
}

/******************************************************************************
*                                                                             *
*	Function:	time2sec					      *
*                                                                             *
*	Description:	Convert a DOS date/time to a number of seconds	      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	  uint16_t date	Y/M/D in 7/4/5 bits format			      *
*	  uint16_t time	H/M/S in 5/6/5 bits format			      *
*                                                                             *
*	Return value:	Number of seconds since january 1st, 1980 at 00:00:00 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       History:                                                              *
*                                                                             *
******************************************************************************/

/* Number of days from new year to beginning of the month. */

uint16_t normal_year[12] =
    {
    /* JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC */
    /*  31  28  31  30  31  30  31  31  30  31  30  31 */
     0, 31, 59, 90,120,151,181,212,243,273,304,334 /*,365 */
    };

uint16_t leap_year[12] =
    {
    /* JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC */
    /*  31  29  31  30  31  30  31  31  30  31  30  31 */
     0, 31, 60, 91,121,152,182,213,244,274,305,335 /*,366 */
    };

long time2sec(uint16_t date, uint16_t time)
    {
    uint16_t y, m;
    long l;

    y = (date & 0xFE00) >> 9;       /* Years since 1980 */
    y += 3;                             /* Years since 1977 */
    l = (y / 4) * ((3*365)+366);
    l += (y % 4) * 365;                 /* Convert years to Days since 1977 */
    l -= 3*365;                         /* Convert years to Days since 1980 */
    y -= 3;                             /* Years since 1980 */

    m = (date & 0x1E0) >> 5;        /* Month (1 to 12) */
    m -= 1;                             /* Month (0 to 11) */
    if (y % 4)
        l += normal_year[m];
    else
        l += leap_year[m];

    l += (date & 0x1F) - 1;             /* Days since 1980/01/01 */

    l *= 24;
    l += (time & 0xF800) >> 11;     /* Hours since 1980/01/01 at 00:00:00 */
    l *= 60;
    l += (time & 0x7E0) >> 5;       /* Minutes since 1980/01/01 at 00:00:00 */
    l *= 60;
    l += 2 * (time & 0x1F);             /* Seconds since 1980/01/01 at 00:00:00 */

    return l;
    }

/******************************************************************************
*                                                                             *
*	Function:	GetClusterSize					      *
*                                                                             *
*	Description:	Get the size of the cluster on a drive		      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	  char disk	Number of the target disk			      *
*                                                                             *
*	Return value:	Per the function definition			      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       History:                                                              *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*                               OS/2 Version                                  *
*                                                                             *
******************************************************************************/

#ifdef _OS2

/* Make sure to include os2.h at the beginning of this file, and before that
    to define the INCL_VIO constant to enable the necessary section */

long GetClusterSize(char drive)	       /* Get cluster size */
    {
    FSALLOCATE fsinf;
    uint16_t err;

    err = DosQFSInfo(drive, FSIL_ALLOC, (PBYTE)&fsinf, sizeof(FSALLOCATE));
    if (err) return 0;

    return fsinf.cSectorUnit * fsinf.cbSector;
    }

#endif

/******************************************************************************
*                                                                             *
*                               WIN32 Version                                 *
*                                                                             *
******************************************************************************/

#ifdef _WIN32

/* Make sure to include windows.h at the beginning of this file, and especially
    the kernel section */

long GetClusterSize(char drive) {	       /* Get cluster size */
  char *root = "@:\\";
  DWORD SectorsPerCluster;
  DWORD BytesPerSector;
  DWORD FreeClusters;
  DWORD Clusters;
  int ok;

  if (!drive)
    root = NULL;
  else {
    /* Workaround for invalid warning in old versions of Visual Studio */
    #if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER <= 1400) /* For Visual C++ versions up to Visual Studio 2005 */
    #pragma warning(disable:4244) /* Ignore the "'+=' : conversion from 'int' to 'char', possible loss of data" warning */
    #endif /* (_MSC_VER <= 1400) */
    root[0] += drive;
    #if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER <= 1400) /* For Visual C++ versions up to Visual Studio 2005 */
    #pragma warning(default:4244) /* Restore the "'+=' : conversion from 'int' to 'char', possible loss of data" warning */
    #endif /* (_MSC_VER <= 1400) */
  }

  ok = GetDiskFreeSpace(root, &SectorsPerCluster, &BytesPerSector,
			  &FreeClusters, &Clusters);
  if (!ok) return 0;

  return (long)(SectorsPerCluster * BytesPerSector);
}

#endif

/******************************************************************************
*                                                                             *
*                               MS_DOS Version                                *
*                                                                             *
******************************************************************************/

#ifdef _MSDOS

long GetClusterSize(char drive)	       /* Get cluster size */
    {
    union REGS inreg;
    union REGS outreg;

    inreg.h.ah = 0x36;
    inreg.h.dl = drive;

    intdos(&inreg,&outreg);

    if (outreg.x.ax == -1)
        return(0);  /* Error */
    else /* (Bytes/sector) * (sectors/cluster) */
        return((long)outreg.x.cx * (long)outreg.x.ax);
    }

#endif

/******************************************************************************
*                                                                             *
*                                Linux Version                                *
*                                                                             *
******************************************************************************/

#ifdef _UNIX

long GetClusterSize(char drive)	{       /* Get cluster size */
  NEW_PATHNAME_BUF(path);
  struct stat st;
  int iErr;

#if PATHNAME_BUFS_IN_HEAP
  if (!path) {
    finis(RETCODE_NO_MEMORY, "Out of memory");
  }
#endif

  if (drive != 0) return 0;		/* No such thing under Unix */
  if (!getcwd(path, PATHNAME_SIZE)) return 0;
  if (strcmp(path, "/")) strcpy(path, ".");
  iErr = stat(path, &st);
  if (iErr) st.st_blksize = 0;
  FREE_PATHNAME_BUF(path);
  return st.st_blksize;
}

#endif

/******************************************************************************
*                                                                             *
*                         End of OS-specific routines                         *
*                                                                             *
******************************************************************************/

