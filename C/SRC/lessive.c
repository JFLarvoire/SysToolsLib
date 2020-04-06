/*****************************************************************************\
*                                                                             *
*   File name:	    lessive.c				                      *
*                                                                             *
*   Description:    Remove blanks at the end of lines in a text file.	      *
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
*    2017-05-29 JFL Added error message for failures to backup or rename the  *
*		    output files.					      *
*                   Display MsvcLibX library version in DOS & Windows.        *
*                   Version 1.4.1.                                            *
*    2017-08-25 JFL Use strerror() for portability to Unix. Version 1.4.2.    *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.1.4.3.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.4.4.      *
*    2020-04-06 JFL Use mkstemp() instead of tempnam() to avoid sec. warnings.*
*                   Fixed a bug when the backup file does not exist initially.*
*                   Copy the input file mode flags to the output file.        *
*                   Generalized -bak to backup any existing output file.      *
*                   Added options -b|--bak as synonyms for -bak.              *
*                   Added options -=|--same as synonyms for -same.            *
*                   Version 1.5.                                              *
*		                                                              *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Remove blanks at the end of lines"
#define PROGRAM_NAME    "lessive"
#define PROGRAM_VERSION "1.5"
#define PROGRAM_DATE    "2020-04-06"

#include "predefine.h" /* Define optional features we need in the C libraries */

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
#include <errno.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

#define TRUE 1
#define FALSE 0

#define LINESIZE 16384	/* Can't allocate more than 64K for stack in some OS. */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Defined for Win32 applications */

#include <dos.h>
#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

/* Avoid warnings for convenient Microsoft-specific routines */
#define strlwr  _strlwr
#define stricmp _stricmp
#define chmod	_chmod

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Defined for MS-DOS applications */

#include <dos.h>
#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Defined for Unix applications */

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
  int n = fprintf(stderr, "Error: ");

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
int file_exists(const char *); 	/* Does this file exist? (TRUE/FALSE) */

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
  char *pszTmpName = NULL;	/* Temporary file name */
  int nChanged = 0;		/* Number of lines changed */
  char line[LINESIZE];
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
      if (strieq(pszOpt, "help") || streq(pszOpt, "h") || streq(pszOpt, "?")) {
	usage();
      }
      if (   strieq(pszOpt, "b")
	  || strieq(pszOpt, "bak")
	  || strieq(pszOpt, "-bak")) {
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
      if (   streq(pszOpt, "=")
	  || strieq(pszOpt, "same")
	  || strieq(pszOpt, "-same")) {
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
      if (streq(pszOpt, "V")) {
	puts(DETAILED_VERSION);
	exit(0);
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
    if (pszOutName) iSameFile = FALSE;	/* If pszOutName is "-", iSameFile is meaningless */
    if (!iSameFile) {
      df = stdout;
    } else {
      pszOutName = pszInName;
    }
  } else { /*  Ignore the -iSameFile argument. Instead, verify if they're actually the same. */
    iSameFile = IsSameFile(pszInName, pszOutName);
    if (iBackup && !file_exists(pszOutName)) iBackup = FALSE; /* There's nothing to backup */
  }
  if (iSameFile || iBackup) { /* Then write to a temporary file */
    int iFile;
    /* But do as if we were writing directly to the target file.
       Test the write rights before wasting time on the conversion */
    df = fopen(pszOutName, "r+");
    if (!df) goto open_df_failed;
    fclose(df);
    df = NULL;
    /* OK, we have write rights, so go ahead with the conversion */
    DEBUG_FPRINTF((mf, "// %s. Writing to a temp file.\n", iSameFile ? "In and out files are the same" : "Backup requested"));
    pszPathCopy = strdup(pszOutName);
    if (!pszPathCopy) goto fail_no_mem;
    pszDirName = dirname(pszPathCopy);
    pszTmpName = strdup(pszDirName);
    if (pszTmpName) pszTmpName = realloc(pszTmpName, strlen(pszTmpName)+10);
    if (!pszTmpName) goto fail_no_mem;
    strcat(pszTmpName, DIRSEPARATOR_STRING "dtXXXXXX");
    iFile = mkstemp(pszTmpName);
    if (iFile == -1) fail("Can't create temporary file %s. %s\n", pszTmpName, strerror(errno));
    df = fdopen(iFile, "wb+");
    if (!df) goto open_df_failed;
    if (iBackup) { /* Create the name of an *.bak file in the same directory */
      char *pszNameCopy = strdup(pszOutName);
      char *pszBaseName = basename(pszNameCopy);
      char *pc;
      if (!pszNameCopy) goto fail_no_mem;
      strcpy(szBakName, pszDirName);
      strcat(szBakName, DIRSEPARATOR_STRING);
      pc = strrchr(pszBaseName, '.');
      if (pc) {
	if (SAMENAME(pc, ".bak")) {
	  fail("Can't backup file %s\n", pszOutName);
	}
	*pc = '\0';			/* Remove the extension */
      }
      strcat(szBakName, pszBaseName);	/* Copy the base name without the extension */
      strcat(szBakName, ".bak");	/* Set extension to .bak */
      free(pszNameCopy);		/* We don't need that copy anymore */
    }
  } else {
    DEBUG_FPRINTF((mf, "// Writing directly to the out file.\n"));
  }
  if (!df) {
    df = fopen(pszOutName, "wb");
    if (!df) {
open_df_failed:
      if (sf != stdout) fclose(sf);
      fail("Can't write to file %s. %s\n", pszOutName, strerror(errno));
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
  DEBUG_FPRINTF((mf, "// Writing done\n"));

  if (iSameFile || iBackup) {
    if (iBackup) {	/* Create an *.bak file in the same directory */
#if !defined(_MSVCLIBX_H_)
      DEBUG_FPRINTF((mf, "unlink(\"%s\");\n", szBakName));
#endif
      iErr = unlink(szBakName); 	/* Remove the .bak if already there */
      if ((iErr == -1) && (errno != ENOENT)) {
	fail("Can't delete file %s. %s\n", szBakName, strerror(errno));
      }
      DEBUG_FPRINTF((mf, "rename(\"%s\", \"%s\");\n", pszOutName, szBakName));
      iErr = rename(pszOutName, szBakName);	/* Rename the source as .bak */
      if (iErr == -1) {
	fail("Can't backup %s. %s\n", pszOutName, strerror(errno));
      }
    } else {		/* iSameFile==TRUE && iBackup==FALSE. Don't keep a backup of the input file */
#if !defined(_MSVCLIBX_H_)
      DEBUG_FPRINTF((mf, "unlink(\"%s\");\n", pszInName));
#endif
      iErr = unlink(pszInName); 	/* Remove the original file */
      if (iErr == -1) {
	fail("Can't delete file %s. %s\n", pszInName, strerror(errno));
      }
    }
    DEBUG_FPRINTF((mf, "rename(\"%s\", \"%s\");\n", pszTmpName, pszOutName));
    iErr = rename(pszTmpName, pszOutName); /* Rename the temporary file as the destination */
    if (iErr == -1) {
      fail("Can't create %s. %s\n", pszOutName, strerror(errno));
    }
  }

  /* Copy the file mode flags */
  if (df != stdout) {
    int iMode = sInTime.st_mode;
    DEBUG_PRINTF(("chmod(\"%s\", 0x%X);\n", pszOutName, iMode));
    iErr = chmod(pszOutName, iMode); /* Try making the target file writable */
    DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
  }

  /* Optionally copy the timestamp */
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
    printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: lessive [SWITCHES] [INFILE [OUTFILE|-same]]\n\
\n\
Switches:\n\
  -b|-bak  Create an *.bak backup file of existing output files\n"
#ifdef _DEBUG
"\
  -d       Output debug information\n"
#endif
"\
  -=|-same Modify the input file in place. (Default: Automatically detected)\n\
  -st      Set the output file time to the same time as the input file.\n\
\n\
Arguments:\n\
  INFILE   Input file pathname. Default or \"-\": stdin\n\
  OUTFILE  Output file pathname. Default or \"-\": stdout\n\
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    file_exists						      |
|									      |
|   Description     Check if a pathname refers to an existing file	      |
|									      |
|   Parameters:     char *pszPathname	    The pathname to check	      |
|                   							      |
|   Returns	    1 = It's a file; 0 = It does not exist, or it's not a file|
|									      |
|   Notes	    Uses stat, to check what's really behind links	      |
|		    							      |
|   History								      |
|    2020-04-04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int file_exists(const char *pszName) {	/* Does this file exist? (TRUE/FALSE) */
  struct stat st;
  int iErr;

  DEBUG_ENTER(("file_exists(\"%s\");\n", pszName));

  iErr = stat(pszName, &st);		/* Get the status of that file */
  if (iErr) RETURN_CONST(FALSE);	/* It does not exist, or is inaccessible anyway */

  RETURN_INT(S_ISREG(st.st_mode));
}

