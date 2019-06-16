/*****************************************************************************\
*                                                                             *
*   Filename	    detab.c                                                   *
*                                                                             *
*   Description	    Convert tabs to spaces                                    *
*                                                                             *
*   Notes	    Adapted from Software Tools, by Kernighan and Plauger     *
*                                                                             *
*   History                                                                   *
*    1984-01-14 MB  Written by Michael Burton. Last Update: 14 Jan 1984       *
*    1984-07    JW  Converted to Microsoft C, debugged and improved by        *
*		    Jack Wright  7/84	                                      *
*    1986-12-15 JFL Default spacing of 4 for .C files                         *
*    1987-01-26 JFL Possibility to append to the dest. file                   *
*    2010-11-22 JFL Changed back to 8 spaces per tab for all files.           *
*		    Updated for modern C compilers.                           *
*		    Default input: stdin; Default output: stdout.	      *
*    2012-10-18 JFL Added the option to use - to specify stdin or stdout.     *
*		    Added authors names in the help.                          *
*                   Display the OS name.                                      *
*                   Version 2.1.                                              *
*    2012-10-18 JFL Added build for Unix.                                     *
*                   Version 2.1.1.                                            *
*    2016-09-09 JFL Restructured to fix a serious issue: Data was lost        *
*                   when the output file was the same as the input file.      *
*                   Added options -same, -bak, -st.                           *
*                   Now a UTF-8 app, that can process any Unicode pathname.   *
*                   Nothing remains of the original authors' code.            *
*    2016-09-13 JFL Added new routine IsSameFile to detect equiv. pathnames.  *
*    2016-09-14 JFL Make sure the debug stream is always in text mode.	      *
*                   Version 3.0.                                              *
*    2017-05-29 JFL Added error message for failures to backup or rename the  *
*		    output files.					      *
*                   Display MsvcLibX library version in DOS & Windows.        *
*                   Version 3.0.1.                                            *
*    2017-08-25 JFL Use strerror() for portability to Unix. Version 3.0.2.    *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.3.0.3.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 3.0.4.      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Convert tabs to spaces"
#define PROGRAM_NAME    "detab"
#define PROGRAM_VERSION "3.0.4"
#define PROGRAM_DATE    "2019-06-12"

#define _CRT_SECURE_NO_WARNINGS /* Prevent security warnings for old routines */

#define _POSIX_SOURCE /* Force Linux to define fileno in stdio.h */
#define _XOPEN_SOURCE /* Force Linux to define tempnam in stdio.h */
#define _BSD_SOURCE /* Force Linux to define S_IFREG in sys/stat.h */
#define _LARGEFILE_SOURCE64 1 /* Force using 64-bits file sizes if possible */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS		/* Define global variables used by our debugging macros */

/************************** OS-specific definitions **************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

#endif /* defined(_MSDOS) */

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

#include <direct.h>
#include <io.h>
#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

/*  Avoid deprecation warnings */
#define stricmp	_stricmp	/* This one is not standard */

#endif /* defined(_WIN32) */

#ifdef __unix__     /* Unix */

#define stricmp strcasecmp

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define SAMENAME streq		/* File name comparison routine */

#endif /* defined(__unix__) */

/********************** End of OS-specific definitions ***********************/

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strieq(string1, string2) (stricmp(string1, string2) == 0)

void fail(char *pszFormat, ...) {
  va_list vl;
  int n = fprintf(stderr, "Error: ");

  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);    /* Not thread-safe on WIN32 ?!? */
  va_end(vl);
  fprintf(stderr, "\n");

  exit(2);
}
#define FAIL(msg) fail("%s", msg);

/* Forward definitions */
int IsSwitch(char *pszArg);
int is_redirected(FILE *f);
int IsSameFile(char *pszPathname1, char *pszPathname2);

/* Global variables */
int iVerbose = FALSE;
FILE *mf;			    /* Message output file */
char usage[] = 
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: detab [OPTIONS] [INFILE [OUTFILE|-same [N]]]\n\
\n\
Options:\n\
  -a      Append a form feed and the output to the destination file.\n\
  -bak    When used with -same, create a backup file of the input file\n"
#ifdef _DEBUG
"  -d      Output debug information\n"
#endif
"\
  -same   Modify the input file in place. (Default: Automatically detected)\n\
  -st     Set the output file time to the same time as the input file.\n\
  -t N    Number of columns between tab stops. Default: 8\n\
\n\
Arguments:\n\
  INFILE  Input file pathname. Default or \"-\": stdin\n\
  OUTFILE Output file pathname. Default or \"-\": stdout\n\
  N       Number of columns between tab stops. Default: 8\n\
\n\
Authors: Michael Burton, Jack Wright, Jean-François Larvoire\n"
#ifdef __unix__
"\n"
#endif
;

int main(int argc, char *argv[])
  {
  int col=1, n=8;
  int c;
  char *mode = "w";		/* Destination file access mode */
  FILE *sf = NULL;		/* Source file handle */
  FILE *df = NULL;		/* Destination file handle */
  int i;
  char *pszInName = NULL;
  char *pszOutName = NULL;
  char szBakName[FILENAME_MAX+1];
  int iBackup = FALSE;
  int iSameFile = FALSE;	/* Backup the input file, and modify it in place. */
  int iCopyTime = FALSE;	/* If true, set the out file time = in file time. */
  struct stat sInTime = {0};
  char *pszPathCopy = NULL;
  char *pszDirName = NULL;	/* Output file directory */
  int iErr;

  /* Open a new message file stream for debug and verbose messages */
  if (is_redirected(stdout)) {	/* If stdout is redirected to a file or a pipe */
    /* Then use stderr to make sure they're visible. */
    /* Drawback: Some scripting shells (Ex: tclsh) will think our program has failed. */
    mf = stderr;
  } else {
    /* Else use stdout to avoid the above drawback. */
    /* This requires duplicating the handle, to make sure it remains in text mode,
       as stdout may be switched to binary mode further down */
    mf = fdopen(dup(fileno(stdout)), "wt");
    /* Disable buffering in both files, else the output may not appear in the programmed order */
    setvbuf(stdout, NULL, _IONBF, 0); /* Disable buffering for stdio */
    setvbuf(mf, NULL, _IONBF, 0); /* Disable buffering for dup of stdio */
  }

  /* Process arguments */

  for (i=1; i<argc; i++) {
    char *pszArg = argv[i];
    if (IsSwitch(pszArg)) {		/* It's a switch */
      char *pszOpt = pszArg+1;
      if (   strieq(pszOpt, "?")
	  || strieq(pszOpt, "h")
	  || strieq(pszOpt, "-help")) {
	printf("%s", usage);
	return 0;
      }
      if (strieq(pszOpt, "a")) {	/* Use append mode */
	mode = "a";
	continue;
      }
      if (strieq(pszOpt, "bak")) {
	iBackup = TRUE;
	continue;
      }
#ifdef _DEBUG
      if (streq(pszOpt, "d")) {
	DEBUG_ON();
	iVerbose = TRUE;
	continue;
      }
#endif
      if (strieq(pszOpt, "same")) {
	iSameFile = TRUE;
	continue;
      }
      if (strieq(pszOpt, "st")) {	/* Same Time */
	iCopyTime = TRUE;
	continue;
      }
      if (strieq(pszOpt, "t")) {	/* Set Tab width */
      	if ((i+1) < argc) n = atoi(argv[++i]);
	continue;
      }
      if (streq(pszOpt, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(pszOpt, "V")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
      fprintf(stderr, "Invalid switch %s\x07\n", pszArg);
      continue;
    }
    /* It's not a switch, it's an argument */
    if (!pszInName) {
      pszInName = pszArg;
      continue;
    }
    if (!pszOutName) {
      pszOutName = pszArg;
      continue;
    }
    n = atoi(pszArg);
  }

  if (n < 1 || n > 32) {
    fputs("Tabs < 1 or > 32\x07\n", stderr);
    return 1;
  }

  /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
  _setmode( _fileno( stdin ), _O_BINARY );
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  if ((!pszInName) || streq(pszInName, "-")) {
    sf = stdin;
    iSameFile = FALSE;	/*  Meaningless in this case. Avoid issues below. */
  } else {
    sf = fopen(pszInName, "rb");
    if (!sf) fail("Can't open file %s\n", pszInName);
    stat(pszInName, &sInTime);
  }
  if ((!pszOutName) || streq(pszOutName, "-")) {
    if (!iSameFile) df = stdout;
  } else { /*  Ignore the -iSameFile argument. Instead, verify if they're actually the same. */
    iSameFile = IsSameFile(pszInName, pszOutName);
  }
  if (iSameFile) {
    DEBUG_FPRINTF((mf, "// In and out files are the same. Writing to a temp file.\n"));
    pszPathCopy = strdup(pszInName);
    if (!pszPathCopy) goto fail_no_mem;
    pszDirName = dirname(pszPathCopy);
    pszOutName = tempnam(pszDirName, "conv.");
    DEBUG_FPRINTF((mf, "tempnam(\"%s\", \"conv.\"); // \"%s\"\n", pszDirName, pszOutName));
    if (iBackup) {	/* Create an *.bak file in the same directory */
      char *pszNameCopy = strdup(pszInName);
      char *pszBaseName = basename(pszNameCopy);
      char *pc;
      if (!pszNameCopy) goto fail_no_mem;
      strcpy(szBakName, pszDirName);
      strcat(szBakName, DIRSEPARATOR_STRING);
      pc = strrchr(pszBaseName, '.');
      if (pc) {
	if (SAMENAME(pc, ".bak")) {
	  fail("Can't backup file %s\n", pszInName);
	}
	*pc = '\0';			/* Remove the extension */
      }
      strcat(szBakName, pszBaseName);	/* Copy the base name without the extension */
      strcat(szBakName, ".bak");	/* Set extension to .bak */
      free(pszNameCopy);		/* We don't need that copy anymore */
    }
  } else {
    DEBUG_FPRINTF((mf, "// In and out files are distinct. Writing directly to the out file.\n"));
  }
  if (!df) {
    df = fopen(pszOutName, "wb");
    if (!df) {
      if (sf != stdout) fclose(sf);
      fail("Can't open file %s\n", pszOutName);

    }
  }

  if (mode[0] == 'a') fputs("\x0C", df); /* In append mode, add a form feed */

  while ((c = fgetc(sf)) != EOF) {  	/*  c MUST be int!  */
    switch (c) {
    case '\t':
      do {
	fputc(' ', df);
	col++;
      } while (((col-1) % n) != 0);
      /* Must subtr. 1 since col++ is done BEFORE this check.  */
      break;
    case '\n':
      fputc('\n', df);
      col = 1;
      break;
    default:
      fputc(c, df);
      col++;
    }
  }

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);

  if (iSameFile) {
    if (iBackup) {	/* Create an *.bak file in the same directory */
      iErr = unlink(szBakName); 	/* Remove the .bak if already there */
      if (iErr == -1) {
	fail("Can't delete file %s. %s\n", szBakName, strerror(errno));
      }
      DEBUG_FPRINTF((mf, "// Rename \"%s\" as \"%s\"\n", pszInName, szBakName));
      iErr = rename(pszInName, szBakName);	/* Rename the source as .bak */
      if (iErr == -1) {
	fail("Can't backup %s. %s\n", pszInName, strerror(errno));
      }
    } else {		/* Don't keep a backup of the input file */
      DEBUG_FPRINTF((mf, "// Remove \"%s\"\n", pszInName));
      iErr = unlink(pszInName); 	/* Remove the original file */
      if (iErr == -1) {
	fail("Can't delete file %s. %s\n", pszInName, strerror(errno));
      }
    }
    DEBUG_FPRINTF((mf, "// Rename \"%s\" as \"%s\"\n", pszOutName, pszInName));
    iErr = rename(pszOutName, pszInName);	/* Rename the destination as the source */
    if (iErr == -1) {
      fail("Can't create %s. %s\n", pszInName, strerror(errno));
    }
    pszOutName = pszInName;
  }

  if ((sf != stdin) && (df != stdout) && iCopyTime) {
    struct utimbuf sOutTime = {0};
    sOutTime.actime = sInTime.st_atime;
    sOutTime.modtime = sInTime.st_mtime;
    utime(pszOutName, &sOutTime);
  }

  return 0;

fail_no_mem:
  fail("Out of memory");
  return 1;
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
|   Function:	    is_redirected					      |
|									      |
|   Description:    Check if a FILE is a device or a disk file. 	      |
|									      |
|   Parameters:     FILE *f		    The file to test		      |
|									      |
|   Returns:	    TRUE if the FILE is a disk file			      |
|									      |
|   Notes:	    Designed for use with the stdin and stdout FILEs. If this |
|		    routine returns TRUE, then they've been redirected.       |
|									      |
|   History:								      |
|    2004-04-05 JFL Added a test of the S_IFIFO flag, for pipes under Windows.|
*									      *
\*---------------------------------------------------------------------------*/

#ifndef S_IFIFO
#define S_IFIFO         0010000         /* pipe */
#endif

int is_redirected(FILE *f)
    {
    int err;
    struct stat buf;			/* Use MSC 6.0 compatible names */
    int h;

    h = fileno(f);			/* Get the file handle */
    err = fstat(h, &buf);		/* Get information on that handle */
    if (err) return FALSE;		/* Cannot tell more if error */
    return (   (buf.st_mode & S_IFREG)	/* Tell if device is a regular file */
            || (buf.st_mode & S_IFIFO)	/* or it's a FiFo */
	   );
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSameFile						      |
|									      |
|   Description     Check if two pathnames refer to the same file	      |
|									      |
|   Parameters:     char *pszPathname1	    The first pathname to check	      |
|                   char *pszPathname2	    The second pathname to check      |
|                   							      |
|   Returns	    1 = Same file; 0 = Different files			      |
|									      |
|   Notes	    Constraints:					      |
|		    - Do not change the files.				      |
|		    - Fast => Avoid resolving links when not necessary.	      |
|		    - Works even if the files do not exist yet.		      |
|		    							      |
|		    Must define a SAMENAME constant, that refers to a file    |
|		    name comparison routine. This routine is OS-dependant,    |
|		    as comparisons are case-dependant in Unix, but not in     |
|		    Windows.						      |
|		    							      |
|   History								      |
|    2016-09-12 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSameFile(char *pszPathname1, char *pszPathname2) {
  int iSameFile;
  char *pszBuf1 = NULL;
  char *pszBuf2 = NULL;
#if defined _WIN32
  WIN32_FILE_ATTRIBUTE_DATA attr1;
  WIN32_FILE_ATTRIBUTE_DATA attr2;
#else
  struct stat attr1;
  struct stat attr2;
#endif /* defined _WIN32 */
  int bDone1;
  int bDone2;
  DEBUG_CODE(
  char *pszReason;
  )

  DEBUG_ENTER(("IsSameFile(\"%s\", \"%s\");\n", pszPathname1, pszPathname2));

  /* First try the obvious: Compare the input arguments */
  if (streq(pszPathname1, pszPathname2)) {
    DEBUG_CODE(pszReason = "Exact same pathnames";)
    iSameFile = TRUE;
IsSameFile_done:
    free(pszBuf1);
    free(pszBuf2);
    RETURN_INT_COMMENT(iSameFile, ("%s\n", pszReason));
  }

  /* Then try a simple attributes comparison, to quickly detect different files */
#if defined _WIN32
  bDone1 = (int)GetFileAttributesEx(pszPathname1, GetFileExInfoStandard, &attr1);
  bDone2 = (int)GetFileAttributesEx(pszPathname2, GetFileExInfoStandard, &attr2);
#else
  bDone1 = stat(pszPathname1, &attr1) + 1;
  bDone2 = stat(pszPathname2, &attr2) + 1;
#endif /* defined _WIN32 */
  if (bDone1 != bDone2) {
    DEBUG_CODE(pszReason = "One exists and the other does not";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  if ((!bDone1) && SAMENAME(pszPathname1, pszPathname2)) {
    DEBUG_CODE(pszReason = "They will be the same";)
    iSameFile = TRUE;
    goto IsSameFile_done;
  }
  if ((bDone1) && memcmp(&attr1, &attr2, sizeof(attr1))) {
    DEBUG_CODE(pszReason = "They're different sizes, times, etc";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  /* They look very similar now: Names differ, but same size, same dates, same attributes */

  /* Get the canonic names, with links resolved, to see if they're actually the same or not */
  pszBuf1 = realpath(pszPathname1, NULL);
  pszBuf2 = realpath(pszPathname2, NULL);
  if ((!pszBuf1) || (!pszBuf2)) {
    DEBUG_CODE(pszReason = "Not enough memory for temp buffers";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  iSameFile = SAMENAME(pszBuf1, pszBuf2);
  DEBUG_LEAVE(("return %d; // \"%s\" %c= \"%s\";\n", iSameFile, pszBuf1, iSameFile ? '=' : '!', pszBuf2));
  free(pszBuf1);
  free(pszBuf2);
  return iSameFile; 
}

