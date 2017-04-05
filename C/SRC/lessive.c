/*****************************************************************************\
*                                                                             *
*   File name:	    lessive.c				                      *
*                                                                             *
*   Description:    Remove blanks at the end lines in a text file.	      *
*                                                                             *
*   History:								      *
*    1989-11-28 JFL jf.larvoire@hp.com created this program.                  *
*    1991-07-16 JFL Fixed not to remove blank lines. Version 1.01.	      *
*    1992-05-20 JFL Support for output file equal to the input file.          *
*		    Support for OS/2. Version 1.1.			      *
*    2006-09-11 JFL Support for Win32. Version 1.2.                           *
*    2010-02-12 JFL Fixed bug causing a crash with lines > 256 characters.    *
*                   Version 1.2.1.                                            *
*    2011-02-24 JFL Only display the program banner with the help screen.     *
*                   The -v option now displays the number of lines changed.   *
*                   Try renaming the temp file, and copy it only if needed.   *
*                   Version 1.3.                                              *
*    2012-10-18 JFL Added my name in the help. Version 1.3.1.                 *
*    2014-12-04 JFL Corrected several warnings. Version 1.3.2.		      *
*    2016-01-08 JFL Fixed all warnings in Linux. Version 1.3.3. 	      *
*    2016-09-13 JFL Restructured to fix a serious issue: Data was lost        *
*                   when the output file was the same as the input file.      *
*                   Added options -same, -bak, -st.                           *
*                   Now a UTF-8 app, that can process any Unicode pathname.   *
*                   Nothing remains of the original authors' code.            *
*                   Added new routine IsSameFile to detect equiv. pathnames.  *
*    2016-09-14 JFL Make sure the debug stream is always in text mode.	      *
*    2016-09-15 JFL Bug fix: Now preserve the \r characters at end of lines.  *
*                   Version 1.4.                                              *
*    2016-10-05 JFL Removed obsolete TARGET_xxx definitions, not used anymore.*
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.4"
#define PROGRAM_DATE    "2016-09-15"

#define _CRT_SECURE_NO_WARNINGS /* Avoid MSVC security warnings */

#define _POSIX_SOURCE /* Force Linux to define fileno in stdio.h */
#define _XOPEN_SOURCE /* Force Linux to define tempnam in stdio.h */
#define _BSD_SOURCE /* Force Linux to define S_IFREG in sys/stat.h */
#define _LARGEFILE_SOURCE64 1 /* Force using 64-bits file sizes if possible */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>
#include <unistd.h>

/* Use MsvcLibX Library's debugging macros */
#include "debugm.h"
DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

#define TRUE 1
#define FALSE 0

#define LINESIZE 16384	/* Can't allocate more than 64K for stack in some OS. */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Defined for Win32 applications */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#include <dos.h>
#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

/* Avoid warnings for convenient Microsoft-specific routines */
#define strlwr _strlwr
#define stricmp _stricmp

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Defined for MS-DOS applications */

#define OS_NAME "DOS"

#include <dos.h>
#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Defined for Unix applications */

#define OS_NAME "UNIX"

#include <unistd.h>
#include <ctype.h>
#include <dirent.h>

char *strlwr(char *psz)    /* Convenient Microsoft library routine not available on Unix. */
   {
   char c;
   char *psz0 = psz;
   while ((c = *psz)) *(psz++) = (char)tolower(c);
   return psz0;
   }

#define stricmp strcasecmp

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define SAMENAME streq		/* File name comparison routine */

#endif

/********************** End of OS-specific definitions ***********************/

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strieq(string1, string2) (stricmp(string1, string2) == 0)

void fail(char *pszFormat, ...) {
  va_list vl;
  int n = 0;

  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);    /* Not thread-safe on WIN32 ?!? */
  va_end(vl);
  fprintf(stderr, "\n");

  exit(2);
}
#define FAIL(msg) fail("%s", msg);

/* Global variables */

int iVerbose = 0;
FILE *mf;			    /* Message output file */
#define verbose(args) if (iVerbose) printf args

/* Function prototypes */

void usage(void);                   /* Display a brief help and exit */
int IsSwitch(char *pszArg);
int is_redirected(FILE *f);
int IsSameFile(char *pszPathname1, char *pszPathname2);

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
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int main(int argc, char *argv[]) {
  int i;
  char *pszInName = NULL;	/* Source file name */
  FILE *sf = NULL;		/* Source file pointer */
  char *pszOutName = NULL;	/* Destination file name */
  FILE *df = NULL;		/* Destination file pointer */
  int nChanged = 0;		/* Number of lines changed */
  char line[LINESIZE];
  char szBakName[FILENAME_MAX+1];
  int iBackup = FALSE;
  int iSameFile = FALSE;	/* Backup the input file, and modify it in place. */
  int iCopyTime = FALSE;	/* If true, set the out file time = in file time. */
  struct stat sInTime = {0};
  char *pszPathCopy = NULL;
  char *pszDirName = NULL;	/* Output file directory */

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
      if (strieq(pszOpt, "help") || streq(pszOpt, "h") || streq(pszOpt, "?")) {
	usage();
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
      if (streq(pszOpt, "v") || strieq(pszOpt, "verbose")) {
	iVerbose = 1;
	continue;
      }
      printf("Unrecognized switch %s. Ignored.\n", argv[i]);
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
    printf("Unexpected argument: %s\nIgnored.\n", argv[i]);
    break;  /* Ignore other arguments */
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

  while(fgets(line, LINESIZE, sf)) {
    int l;
    int hasCR, hasLF;
    i = l = (int)strlen(line);
    /* Is there an \n at the end of the line? */
    hasLF = ((i) && (line[i-1] == '\n'));
    if (hasLF) i -= 1;
    /* Is there an \r at the end of the line? */
    hasCR = ((i) && (line[i-1] == '\r'));
    if (hasCR) i -= 1;
    /* Remove all trailing spaces, tabs, \r and \n */
    while (i) {
      i -= 1;             /* Offset of last character in line */
      if (line[i] == '\r') continue;          /* Return */
      if (line[i] == '\n') continue;          /* Newline */
      if (line[i] == ' ') continue;           /* Space */
      if (line[i] == '\x09') continue;        /* Tab */
      i += 1;             /* Length of string to preserve */
      break;
    }
    /* Restore the final \r, if initially present */
    if (hasCR) line[i++] = '\r';
    /* Restore the final \n, if initially present */
    if (hasLF) line[i++] = '\n';
    /* Make sure there's a final NUL */
    line[i] = '\0';
    /* Count lines changed */
    if (i != l) nChanged += 1;

    fputs(line, df);
  }

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);

  if (iSameFile) {
    if (iBackup) {	/* Create an *.bak file in the same directory */
      unlink(szBakName); 		/* Remove the .bak if already there */
      DEBUG_FPRINTF((mf, "Rename \"%s\" as \"%s\"\n", pszInName, szBakName));
      rename(pszInName, szBakName);	/* Rename the source as .bak */
    } else {		/* Don't keep a backup of the input file */
      DEBUG_FPRINTF((mf, "Remove \"%s\"\n", pszInName));
      unlink(pszInName); 		/* Remove the original file */
    }
    DEBUG_FPRINTF((mf, "Rename \"%s\" as \"%s\"\n", pszOutName, pszInName));
    rename(pszOutName, pszInName);	/* Rename the destination as the source */
    pszOutName = pszInName;
  }

  if ((sf != stdin) && (df != stdout) && iCopyTime) {
    struct utimbuf sOutTime = {0};
    sOutTime.actime = sInTime.st_atime;
    sOutTime.modtime = sInTime.st_mtime;
    utime(pszOutName, &sOutTime);
  }

  if (iVerbose) fprintf(mf, "%d lines trimmed\n", nChanged);

  return 0;

fail_no_mem:
  fail("Out of memory");
  return 1;
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help screen 			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    Does not return					      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
    printf("\n\
Lessive Version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME " - Wipe out trailing blanks\n\
\n\
Usage: lessive [SWITCHES] [INFILE [OUTFILE|-same]]\n\
\n\
Switches:\n\
  -bak    When used with -same, create a backup file of the input file\n"
#ifdef _DEBUG
"  -d      Output debug information\n"
#endif
"\
  -same   Modify the input file in place. (Default: Automatically detected)\n\
  -st     Set the output file time to the same time as the input file.\n\
\n\
Arguments:\n\
  INFILE  Input file pathname. Default or \"-\": stdin\n\
  OUTFILE Output file pathname. Default or \"-\": stdout\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
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

int IsSwitch(char *pszArg)
    {
    switch (*pszArg)
	{
	case '-':
#if defined(_WIN32) || defined(_MSDOS)
	case '/':
#endif
	    return (pszArg[1] != '\0');
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

