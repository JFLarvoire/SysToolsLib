/*****************************************************************************\
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
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "3.2"
#define PROGRAM_DATE    "2017-10-31"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */
/* #define __USE_BSD	    */	/* Use BSD extensions (DT_xxx types in dirent.h) */
/* #define _POSIX_SOURCE 2  */	/* Use Posix 2 extensions (FNM_xxx flags in fnmatch.h) */
#define _GNU_SOURCE		/* Replaces nicely the above two */
#define _LARGEFILE_SOURCE 1	/* Force using 64-bits file sizes if possible */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */
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

#ifndef UINTMAX_MAX /* For example Tru64 doesn't define it */
typedef unsigned long uintmax_t;
#endif

#define FALSE 0
#define TRUE 1

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

#define DIRSEPARATOR_CHAR '\\'		/* Directory separator character */
#define DIRSEPARATOR_STRING "\\"
#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE 13                /* 8.3 name length = 8+1+3+1 = 13 */
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define IGNORECASE TRUE

#define CDECL __cdecl

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

#define DIRSEPARATOR_CHAR '\\'		/* Directory separator character */
#define DIRSEPARATOR_STRING "\\"
#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE FILENAME_MAX
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define IGNORECASE TRUE

/* CDECL is defined in WinDef.h, which is already included by MsvcLibX */

#endif /* defined(_WIN32) */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* Automatically defined when targeting an OS/2 application? */

#define DIRSEPARATOR_CHAR '\\'		/* Directory separator character */
#define DIRSEPARATOR_STRING "\\"
#define PATHNAME_SIZE CCHMAXPATH	/* FILENAME_MAX incorrect in stdio.h */
#define NODENAME_SIZE CCHMAXPATHCOMP
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define IGNORECASE TRUE

#define CDECL __cdecl

#endif /* defined(_OS2) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__	/* Automatically defined when targeting a Unix application */

#define DIRSEPARATOR_CHAR '/'		/* Directory separator character */
#define DIRSEPARATOR_STRING "/"
#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE FILENAME_MAX
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define IGNORECASE FALSE

#define CDECL				/* No such thing needed for Linux builds */

#endif /* defined(__unix__) */

/********************** End of OS-specific definitions ***********************/

/* Local definitions */

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

#define streq(string1, string2) (strcmp(string1, string2) == 0)
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
} scanOpts;

/* Global variables */

char init_dir[PATHNAME_SIZE];       /* Initial directory */
char init_drive;                    /* Initial drive */
long csz=0;			    /* Cluster size. 0=Unknown. */
int band = FALSE;		    /* If TRUE, skip a line every 5 lines */
int iContinue = FALSE;              /* If TRUE, continue after having an error */
int iQuiet = FALSE;		    /* If TRUE, only display major errors */
int iVerbose = FALSE;		    /* If TRUE, display additional information */
int iHuman = TRUE;		    /* If TRUE, display human-friendly values with a comma every 3 digits */
char *pszUnit = "B";		    /* "B"=bytes; "KB"=Kilo-Bytes; "MB"; GB" */

/* Function prototypes */

char *version(void);		    /* Build a string with the program versions */
void usage(void);                   /* Display a brief help and exit */
void finis(int retcode);            /* Return to the initial drive & exit */
int parse_date(char *token, time_t *pdate); /* Convert the argument to a time_t */
int Size2String(char *pBuf, total_t ll); /* Convert size to a decimal, with a comma every 3 digits */
int Size2StringWithUnit(char *pBuf, total_t llSize); /* Idem, appending the user-specified unit */

total_t ScanFiles(scanOpts *pOpts, void *pConstraints); /* Scan the current dir */
total_t ScanDirs(scanOpts *pOpts, void *pConstraints);  /* Scan every subdir */
void affiche(char *path, total_t size);/* Display sorted list */
int scandirX(const char *pszName,
	     struct dirent ***resultList,
	     int (*cbSelect) (const struct dirent *, void *pRef),
	     int (CDECL *cbCompare) (const struct dirent **, const struct dirent **),
	     void *pRef);	    /* scandir() extension, passing a pRef to cbSelect() */

long GetClusterSize(char drive);    /* Get cluster size */

/******************************************************************************
*                                                                             *
*                               Main routine                                  *
*                                                                             *
******************************************************************************/

int main(int argc, char *argv[]) {
  char *from = NULL;		/* What directory to list */
  int i;
  selectOpts fConstraints = {
    0,				/* pattern: Wildcards pattern. NULL=undefined */
    0,				/* datemin: Minimum date stamp. 0=undefined */
    0,				/* datemax: Maximum date stamp. 0=undefined */
  };
  scanOpts sOpts = {
    FALSE,			/* recur: If TRUE, list subdirectories recursively */
    FALSE,			/* total: If TRUE, totalize size of all subdirs */
    FALSE,			/* subdirs: If TRUE, start scanning in each subdirectory */
    0,				/* Initial depth in the search tree */
  };
  char *dateminarg = NULL;	/* Minimum date argument */
  char *datemaxarg = NULL;	/* Maximum date argument */
  int iUseCsz = FALSE;		/* If TRUE, use the cluster size */
  int err;
  char *pc;
  total_t size;			/* Total size */

  /* Parse command line arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (   (arg[0] == '-')
#ifndef __unix__
        || (arg[0] == '/')
#endif
       ) { /* It's a switch */
      if (streq(arg+1, "b")) {
	band = TRUE;
	continue;
      }
      if (streq(arg+1, "c")) {
	iUseCsz = TRUE; /* Use the cluster size for size calculations */
	if (   ((i+1) < argc)
	    && sscanf(argv[i+1], "%ld", &csz)
	    && (csz != 0)) {
	  i += 1;		/* Skip the size in next argument */
	}
      continue;
      }
      if (streq(arg+1, "d")) {
	sOpts.subdirs = TRUE;
	continue;
      }
#ifdef _DEBUG
      if (streq(arg+1, "debug")) {
	DEBUG_ON();
	continue;
      }
#endif
      if (streq(arg+1, "from")) {
	dateminarg = argv[++i];
	if (!parse_date(dateminarg, &fConstraints.datemin)) {
	  fprintf(stderr, "Error: Invalid date format: -from %s\n", dateminarg);
	  dateminarg = NULL;
	}
	continue;
      }
      if (   streq(arg+1, "help")
	  || streq(arg+1, "h")
	  || streq(arg+1, "?")) {
	usage();
      }
      if (streq(arg+1, "g")) {
	pszUnit = "GB";
	continue;
      }
      if (streq(arg+1, "H")) {
	iHuman = FALSE;
	continue;
      }
      if (streq(arg+1, "i")) {
	iContinue = TRUE;
	continue;
      }
      if (streq(arg+1, "I")) {
	iContinue = FALSE;
	continue;
      }
      if (streq(arg+1, "k")) {
	pszUnit = "KB";
	continue;
      }
      if (streq(arg+1, "m")) {
	pszUnit = "MB";
	continue;
      }
      if (streq(arg+1, "nologo")) continue; /* Old option retired */
      if (streq(arg+1, "q")) {
	iQuiet = TRUE;
	continue;
      }
      if (   streq(arg+1, "r")
      	  || streq(arg+1, "s")) {
	sOpts.recur = TRUE;
	continue;
      }
      if (streq(arg+1, "t")) {
	sOpts.total = TRUE;
	continue;
      }
      if (streq(arg+1, "to")) {
	datemaxarg = argv[++i];
	if (!parse_date(datemaxarg, &fConstraints.datemax)) {
	  fprintf(stderr, "Error: Invalid date format: -to %s\n", datemaxarg);
	  datemaxarg = NULL;
	}
	continue;
      }
      if (streq(arg+1, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(arg+1, "V")) {
	printf("%s\n", version());
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

  /* Extract the search pattern if provided as part of the target pathname */
  if (from) {
    struct stat st;
    err = stat(from, &st);
    if (err || ((st.st_mode & S_IFMT) != S_IFDIR)) { /* If this is not a dir */
      pc = strrchr(from, DIRSEPARATOR_CHAR);
      if (!pc) {
	fConstraints.pattern = from;
	from = NULL;	/* Contrary to our initial guess, there's no dir name */
      } else {
	*pc = '\0';	/* Cut the wildcards pattern off the directory name */
	fConstraints.pattern = pc+1;
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
#ifndef __unix__
  init_drive = (char)getdrive();
  if (from && (from[1] == ':')) {
    if (from[0] >= 'a') from[0] -= 'a' - 'A';
    DEBUG_PRINTF(("chdrv(%d); // Drive %c:'\n", from[0] - '@', from[0]));
    err = chdrive(from[0] - '@');
    if (err) {
      fprintf(stderr, "Error: Cannot access drive %c:\n", from[0]);
      finis(RETCODE_INACCESSIBLE);
    }
    from += 2;
  }
#endif

  /* Current directory on the target drive */
  getcwd(init_dir, PATHNAME_SIZE);

  /* Go to the target directory */
  if (from && from[0]) {
    DEBUG_PRINTF(("chdir(\"%s\");\n", from));
    err = chdir(from);
    if (err) {
      fprintf(stderr, "Error: Cannot access directory %s.\n", from);
      finis(RETCODE_INACCESSIBLE);
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
  if (!sOpts.subdirs) {
    size = ScanFiles(&sOpts, &fConstraints);
    if (!sOpts.recur) {
      char szBuf[40];
      Size2StringWithUnit(szBuf, size);
      printf("%s\n", szBuf);
    }
  } else {
    size = ScanDirs(&sOpts, &fConstraints);
  }

  /* Restores the initial drive and directory and exit */
  finis(RETCODE_SUCCESS);
  return 0;
}

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " EXE_OS_NAME
	  DEBUG_VERSION
	  );
}

void usage(void) {
  printf("\n\
DirSize version %s\n\
\n\
Usage: dirsize [SWITCHES] [TARGET]\n\
\n\
Display the total size used by the target directory.\n\
\n\
Switches:\n\
  -?          Display this help message and exit.\n\
  -b	      Skip a line every 5 lines, to improve readability.\n\
  -c	      Use the actual cluster size to compute the total size.\n\
  -c size     Use the specified cluster size to compute the total size.\n\
  -d	      Compare every subdirectory of the target directory.\n"
#ifdef _DEBUG
"\
  -debug      Output debug information.\n"
#endif
"\
  -from Y-M-D List only files starting from that date.\n\
  -g	      Display sizes in Giga bytes.\n\
  -H	      Display sizes without the human-friendly commas.\n\
  -i	      Ignore directory access errors.\n\
  -I	      Stop in case of directory access error. (Default)\n\
  -k	      Display sizes in Kilo bytes.\n\
  -m	      Display sizes in Mega bytes.\n\
  -q          Quiet mode: Do not display minor errors.\n\
  -r|-s	      Display the sizes of all subdirectories too.\n\
  -t	      Count the total size of all files plus that of all subdirs.\n\
  -to Y-M-D   List only files up to that date.\n\
  -v          Display verbose information.\n\
  -V          Display this program version and exit.\n\
\n\
Target:       PATHNAME|PATTERN|PATHNAME" DIRSEPARATOR_STRING "PATTERN\n\
Pathname:     Target directory pathname. Default: current directory\n\
Pattern:      Wildcards pattern. Default: " PATTERN_ALL "\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
, version());

  exit(0);
}

void finis(int retcode) {
  switch (retcode) {
    case RETCODE_SUCCESS:
      break;
    case RETCODE_NO_MEMORY:
      printf("Not enough memory.\n");
      break;
    default:
      break;
  }

  chdir(init_dir);
#ifndef __unix__
  chdrive(init_drive);
#endif

  exit(retcode);
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

int SelectFilesCB(const struct dirent *pDE, void *p) {
  selectOpts *pC = p;
  struct stat sStat;
  int iErr;

  if (pDE->d_type != DT_REG) return FALSE;	/* We want only files */

#ifdef DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
  iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
  iErr = lstat(pDE->d_name, &sStat);
#endif
  if (iErr) return FALSE;	/* Ignore suspect entries */

  /* Skip files outside date range */
  if (pC->datemin && (sStat.st_mtime < pC->datemin)) return FALSE;
  if (pC->datemax && (sStat.st_mtime > pC->datemax)) return FALSE;

  /* Skip files which don't match the wildcard pattern */
  if (pC->pattern) {
    if (fnmatch(pC->pattern, pDE->d_name, FNM_CASEFOLD) == FNM_NOMATCH) {
      return FALSE;
    }
  }

  /* OK, all criteria pass. */
  return TRUE;
}

total_t ScanFiles(scanOpts *pOpts, void *pConstraints) {
  total_t size = 0;
  total_t dSize;
  char szCurDir[PATHNAME_SIZE];
  struct dirent *pDE;
  struct dirent **ppDE;
  struct dirent **pDElist;
  int nDE;
  uintmax_t fsize;
  struct stat sStat;
  int iErr;

  DEBUG_ENTER(("ScanFiles(%p);\n", pConstraints));

  /* Scan all files */
  nDE = scandirX(".", &pDElist, SelectFilesCB, NULL, pConstraints);
  if (nDE < 0) {
    fprintf(stderr, "Error: Out of directory handles.\n");
    finis(RETCODE_NO_MEMORY);
  }
  for (ppDE = pDElist; nDE--; ppDE++) {
    pDE = *ppDE;
    memset(&sStat, 0, sizeof(sStat));

#ifdef DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = lstat(pDE->d_name, &sStat);
#endif
    if (iErr) continue;
    DEBUG_PRINTF(("// Counting %10"PRIuMAX" bytes for %-32s\n", (uintmax_t)(sStat.st_size), pDE->d_name));
    fsize = sStat.st_size; /* Get the actual file size */
    if (csz) {	/* If the cluster size is provided */
		/* Round it to the next cluster multiple */
      fsize += csz-1;
      fsize -= fsize % csz;
    }
    size += fsize;  /* Totalize sizes */

    free(pDE);
  }
  free(pDElist);

  /* Optionally scan all subdirectories */
  if (pOpts->recur || pOpts->total) {
    pOpts->depth += 1;
    dSize = ScanDirs(pOpts, pConstraints);
    pOpts->depth -= 1;
    if (pOpts->total) size += dSize;  /* Totalize sizes */
    if (pOpts->recur) {
      getcwd(szCurDir, sizeof(szCurDir)); /* Canonic name of the target directory */
      affiche(szCurDir, size);
    }
  }

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

int SelectDirsCB(const struct dirent *pDE) {
  if (   (pDE->d_type == DT_DIR)	/* We want only directories */
      && (!streq(pDE->d_name, "."))	/* Except . */
      && (!streq(pDE->d_name, "..")))	/* and .. */
    return TRUE;
  else
    return FALSE;
}

/* Scan all subdirectories */
total_t ScanDirs(scanOpts *pOpts, void *pConstraints) {
  total_t size = 0;
  total_t dSize;
  struct dirent *pDE;
  struct dirent **ppDE;
  struct dirent **pDElist;
  int nDE;
  int iErr;
  struct stat sStat;
  char szCurDir[PATHNAME_SIZE];

  DEBUG_ENTER(("ScanDirs(%p);\n", pConstraints));

  /* Get all subdirectories */
  nDE = scandir(".", &pDElist, SelectDirsCB, alphasort);
  if (nDE < 0) {
    fprintf(stderr, "Error: Out of directory handles.\n");
    finis(RETCODE_NO_MEMORY);
  }
  for (ppDE = pDElist; nDE--; ppDE++) {
    pDE = *ppDE;

#ifdef DIRENT2STAT_DEFINED /* DOS/Windows return stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = lstat(pDE->d_name, &sStat);
#endif

#if !HAS_MSVCLIBX
    DEBUG_PRINTF(("chdir(\"%s\");\n", pDE->d_name));
#endif
    iErr = chdir(pDE->d_name);
    if (iErr) {
      char *pszSeverity = iContinue ? "Warning" : "Error";
      if (iVerbose || !iContinue) {
	getcwd(szCurDir, sizeof(szCurDir));   /* Canonic name of the target directory */
      	fprintf(stderr, "%s: Cannot access directory %s" DIRSEPARATOR_STRING "%s. %s\n", pszSeverity, szCurDir, pDE->d_name, strerror(errno));
      }
      if (!iContinue) finis(RETCODE_INACCESSIBLE);
    } else {
      dSize = ScanFiles(pOpts, pConstraints);
      if (!pOpts->depth) {
	getcwd(szCurDir, sizeof(szCurDir));   /* Canonic name of the target directory */
	affiche(szCurDir, dSize);
      }
#if !HAS_MSVCLIBX
      DEBUG_PRINTF(("chdir(\"..\");\n"));
#endif
      iErr = chdir("..");
  
      size += dSize;  /* Totalize sizes */
    }

    free(pDE);
  }
  free(pDElist);

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

#ifdef __unix__

long GetClusterSize(char drive)	{       /* Get cluster size */
  char path[PATHNAME_SIZE];
  struct stat st;
  int iErr;

  if (drive != 0) return 0;		/* No such thing under Unix */
  if (!getcwd(path, PATHNAME_SIZE)) return 0;
  if (strcmp(path, "/")) strcpy(path, ".");
  iErr = stat(path, &st);
  if (iErr) st.st_blksize = 0;
  return st.st_blksize;
}

#endif

/******************************************************************************
*                                                                             *
*                         End of OS-specific routines                         *
*                                                                             *
******************************************************************************/


/*****************************************************************************\
*                                                                             *
*   Function:	    scandirX		 				      *
*									      *
*   Description:    Select entries in a directory			      *
*									      *
*   Arguments:	    const char *name	Directory name            	      *
*		    dirent ***namelist  where to store the result array       *
*		    int (*cbSelect)()   Selection callback function           *
*		    int (CDECL *cbCompare)()  Comparison function for sorting *
*		    void *pRef		Reference data to pass to cbSelect    *
*									      *
*   Return value:   # of entries in the array, or -1 if error.		      *
*									      *
*   Notes:	    Extension of the standard scandir routine, allowing to    *
*		    pass arguments to the selection routine.
*		    							      *
*   History:								      *
*    2012-01-11 JFL Initial implementation				      *
*                                                                             *
\*****************************************************************************/

typedef int (CDECL *pCompareProc)(const void *item1, const void *item2);

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int scandirX(const char *pszName,
	     struct dirent ***resultList,
	     int (*cbSelect) (const struct dirent *, void *pRef),
	     int (CDECL *cbCompare) (const struct dirent **, const struct dirent **),
	     void *pRef) {
  int n = 0;
  DIR *pDir;
  struct dirent *pDirent;
  struct dirent *pDirent2;
  struct dirent **pList = NULL;
  struct dirent **pList2;

  DEBUG_ENTER(("scandirX(\"%s\", %p, %p, %p, %p);\n", pszName, resultList, cbSelect, cbCompare, pRef));

  pDir = opendir(pszName);
  if (!pDir) {
    DEBUG_LEAVE(("return -1; // errno=%d\n", errno));
    return -1;
  }

  while ((pDirent = readdir(pDir))) {
    if (!pDirent->d_type) { /* Some filesystems don't set this field */
      struct stat st;
      char szName[PATHNAME_SIZE];
      sprintf(szName, "%s/%s", pszName, pDirent->d_name);
      lstat(szName, &st);
      if (S_ISREG(st.st_mode)) pDirent->d_type = DT_REG;
      if (S_ISDIR(st.st_mode)) pDirent->d_type = DT_DIR;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      if (S_ISLNK(st.st_mode)) pDirent->d_type = DT_LNK;
#endif
#if defined(S_ISBLK) && S_ISBLK(S_IFBLK) /* In DOS it's defined, but always returns 0 */
      if (S_ISBLK(st.st_mode)) pDirent->d_type = DT_BLK;
#endif
#if defined(S_ISCHR) && S_ISCHR(S_IFCHR) /* In DOS it's defined, but always returns 0 */
      if (S_ISCHR(st.st_mode)) pDirent->d_type = DT_CHR;
#endif
#if defined(S_ISFIFO) && S_ISFIFO(S_IFIFO) /* In DOS it's defined, but always returns 0 */
      if (S_ISFIFO(st.st_mode)) pDirent->d_type = DT_FIFO;
#endif
#if defined(S_ISSOCK) && S_ISSOCK(S_IFSOCK) /* In DOS it's defined, but always returns 0 */
      if (S_ISSOCK(st.st_mode)) pDirent->d_type = DT_SOCK;
#endif
    }
    if (cbSelect && !cbSelect(pDirent, pRef)) continue; /* We don't want this one. Continue search. */
    /* OK, we've selected this one. So append a copy of this dirent to the list. */
    n += 1;
    pList2 = (struct dirent **)realloc(pList, n * sizeof(struct dirent *));
    pDirent2 = malloc(sizeof(struct dirent));
    if (!pList2 || !pDirent2) {
      if (pDirent2) free(pDirent2);
      for (n-=1; n>0; ) free(pList[--n]);
      /* errno = ENOMEM; */ /* Out of memory. Should already be set by malloc failure */
      DEBUG_LEAVE(("return -1; // errno=%d\n", errno));
      return -1;
    }
    *pDirent2 = *pDirent;
    pList = pList2;
    pList[n-1] = pDirent2;
  }

  closedir(pDir);

  if (cbCompare) qsort(pList, n, sizeof(struct dirent *), (pCompareProc)cbCompare);
  *resultList = pList;
  DEBUG_LEAVE(("return %d;\n", n));
  return n;
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif
