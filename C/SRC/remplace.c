/*****************************************************************************\
*		    							      *
*   Filename	    remplace.c						      *
*		    							      *
*   Description	    A program that replaces a string by another one	      *
*		    							      *
*   Notes	    Adapted from detab.c.				      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug	# Debug ver.  *
*		    							      *
*		    Character encodings in Windows:			      *
*		    - File encoding is ANSI or UTF-8 or UTF-16, based on BOM. *
*		    - stdin and stdout pipe encoding is the console code page.*
*			(unless redirected to a file!)			      *
*		    - stdin and stdout console encoding can/should be UTF-16. *
*		    - Command line arguments are encoded in the console code  *
*		    	page, then converted to UTF-8 by MsvLibX.	      *
*		    - Remplace is not an encoding conversion tool. It MUST    *
*		      preserve the input encoding, and any \n or \r\n markers.*
*		    => 							      *
*		    - Use file binary mode for input and output.	      *
*		    - Exception: For debug output to the console, use Unicode.*
*		    - Convert arguments to the input stream encoding.	      *
*		    							      *
*   History								      *
*    1986-11-05 JFL jf.larvoire@hp.com created this program, based on detab.c.*
*    1992-03-11 JFL Adapted to Microsoft C.				      *
*		    Modified to use binary mode for input and output.	      *
*		    Version 1.1 					      *
*    1992-10-24 JFL Cleaned up warnings.				      *
*		    Changed default to non pipe mode.			      *
*		    Added ability to read and write to the same file.	      *
*		    Improved usage help.				      *
*		    Recompiled in dual mode DOS+OS/2.			      *
*		    Version 1.2 					      *
*    1995-03-06 JFL Clarified the help message. 			      *
*		    Version 1.21					      *
*    1996-05-06 JFL Added support for wild cards.			      *
*		    Version 1.3 					      *
*    1996-06-19 JFL Updated help screen to describe how to manage dble quotes.*
*		    Version 1.31					      *
*    1996-06-24 JFL Added /= option to remove mime =XX encodings.	      *
*		    Version 1.4 					      *
*    1997-04-14 JFL Removed MSVC 1.x specific references to _osfile[].	      *
*		    Version 1.41					      *
*    1999-08-03 JFL Don't output a space when merging MIME lines.             *
*		    Version 1.43					      *
*    2001-11-06 JFL Added options -q, -nb, -same. Count the number of changes.*
*		    Version 1.5 					      *
*    2002-11-28 JFL Fixed a bug in wildcard mode: Incorrect data was output   *
*		    in case of a partial match.				      *
*		    Fixed another bug in case of a partial match: Some actual *
*		    matching strings were then missed.			      *
*		    Version 1.51					      *
*    2003-09-08 JFL Reversed the order of strings and file names.             *
*		    Removed the option -pipe. Now use stdin and stdout by     *
*		    default if no name is specified.			      *
*		    Version 1.6						      *
*    2003-09-23 JFL Fixed bug with -=. Added support for Unix-style files.    *
*		    Version 1.61					      *
*    2004-01-13 JFL Fixed bug with -same. 				      *
*		    Version 1.62					      *
*    2004-02-16 JFL Updated the usage message about " and \ parsing.          *
*    2004-04-01 JFL Rewrote the argument parsing to process \ with standard C *
*		    conventions. Added [] sets and ?+* regular expressions.   *
*    2004-04-02 JFL Added the support of NUL characters. 		      *
*                   Added escaping of special characters '.' and '['.         *
*                   Version 2.0   					      *
*    2004-05-28 JFL Removed debug code forgotten in the -= option. v2.01.     *
*    2004-09-27 JFL Return 0=Success; 1=No change done; 2=Error. v2.02.       *
*    2005-06-10 JFL Fixed a longstanding bug, with backtracking that did not  *
*                    work when inputing from stdin. Version 2.03.             *
*    2005-06-16 JFL Added a Unix version. Updated help. Version 2.04.         *
*    2005-09-22 JFL Fixed a bug when a pattern ends with .*                   *
*                   Added -i option to get the input string on the cmd line.  *
*		    Version 2.1.					      *
*    2005-09-22 JFL Added -% option to decode %NN URL codes.                  *
*		    Updated option -i to default input to the nul device.     *
*		    Use - to specify stdin or stdout instead of a file name.  *
*		    Added options -- and -. .				      *
*		    Version 2.2.					      *
*    2008-12-02 JFL Added -# option to stop parsing arguments.                *
*		    Version 2.21.					      *
*    2008-12-10 JFL Fixed a bug introduced on 12/02.                          *
*		    Updated the debug code to display \x sequences if needed. *
*		    Version 2.22.					      *
*    2010-12-19 JFL Flush every line of output on pipes and consoles.	      *
*		    Useful to see output in real time in long complex cmds.   *
*		    Version 2.23.					      *
*    2011-01-10 JFL Added the -t option to copy the input file time.          *
*		    Fixed the handling of - for the output to stdout.	      *
*		    Removed routine stcd_i, using atoi instead.		      *
*		    Removed the obsolete -w option, and added the -f option.  *
*		    Version 2.3.					      *
*    2011-09-27 JFL Display the number of changes in verbose only.            *
*		    Version 2.3.1.					      *
*    2012-02-29 JFL Added the ability to output the input match where \0 is   *
*		    in the new pattern.					      *
*		    Use my new common debugging macros.			      *
*		    Fixed a bug that caused the DOS version to crash.	      *
*		    Version 2.4.					      *
*    2012-03-12 JFL Added support for negative sets, like [^abc].             *
*		    Version 2.4.1.					      *
*    2012-03-17 JFL Moved utime.h to the standard Unix location.              *
*		    Version 2.4.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.4.3.                 *
*    2013-12-16 JFL Added max definition for MinGW.                           *
*    2014-06-24 JFL Corrected the help for the \0 regex replacement.          *
*		    Version 2.4.4.					      *
*    2014-12-03 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 2.4.5.					      *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 2.4.6.  					      *
*    2016-06-21 JFL Added regular expression ranges, like [a-z].              *
*		    Version 2.5.  					      *
*    2016-07-04 JFL Fixed an error in MSDOS help.			      *
*		    Version 2.5.1.  					      *
*    2016-09-02 JFL Minor change to the "Replacing xxx with yyy" message.     *
*		    Version 2.5.2.  					      *
*    2016-09-09 JFL Bug fix: An error while replacing a file in place caused  *
*		    it to be truncated to size 0.			      *
*                   Added options -same, -bak, -st. Version 2.5.3.            *
*    2016-09-13 JFL Added new routine IsSameFile to detect equiv. pathnames.  *
*                   Minor tweaks to fix compilation in Linux.                 *
*    2016-09-14 JFL Make sure the debug stream is always in text mode.        *
*		    Version 2.5.4.  					      *
*    2016-09-20 JFL Bug fix: The Win32 version did not process empty args.    *
*		    Version 2.5.5.  					      *
*    2017-05-11 JFL Bug fix: Do not convert the output encoding in any case.  *
*		    Match the old & new strings to the input stream encoding. *
*		    Display MsvcLibX library version in DOS & Windows.        *
*		    Prefix all verbose and debug comments with a "//".        *
*		    Version 2.6.    					      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.6"
#define PROGRAM_DATE    "2017-05-11"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#define _POSIX_SOURCE /* Force Linux to define fileno in stdio.h */
#define _XOPEN_SOURCE /* Force Linux to define tempnam in stdio.h */
#define _BSD_SOURCE   /* Force Linux to define S_IFREG in sys/stat.h */
#define _LARGEFILE_SOURCE64 1   /* Force using 64-bits file sizes if possible */
#define _GNU_SOURCE		/* Replaces nicely all the above */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>
#include <unistd.h>

#define SZ 80               /* Strings size */

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strieq(string1, string2) (stricmp(string1, string2) == 0)

/* Use MsvcLibX Library's debugging macros */
#include "debugm.h"
DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#include <direct.h>
#include <io.h>
#include "iconv.h"		/* MsvcLibX character encoding conversions */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define DEVNUL "NUL"

#define SAMENAME strieq		/* File name comparison routine */

#define stricmp	_stricmp	/* This one is not standard */

/* Do not use MsvcLibX encoding conversion for the output */
#if 0
#undef fputc
#undef fwrite
#endif

#endif /* defined(_WIN32) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#define TARGET_MSDOS
#define OS_NAME "DOS"

#include <direct.h>
#include <io.h>

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define DEVNUL "NUL"

#define SAMENAME strieq		/* File name comparison routine */

/* Workaround for a linker bug in DOS, which is case independant.
   So for DOS, FGetC is the same as fgetc. */
#define FGetC FGetC1
#define FPutC FOutC1
#define FSeek FSeek1
#define FWrite FWrite1

#endif /* defined(_MSDOS) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif

#define getch getchar
#define stricmp strcasecmp

#include <unistd.h>

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define DEVNUL "/dev/nul"

#define SAMENAME streq		/* File name comparison routine */

#endif /* defined(__unix__) */

/************************* MinGW-specific definitions ************************/

#ifdef __MINGW32__     /* Both MinGW32 and MinGW64 */

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

#endif

/********************** End of OS-specific definitions ***********************/

#ifdef _MSC_VER
#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */
#endif

void fail(char *pszFormat, ...) {
  va_list vl;
  int n = 0;

  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);    /*  Not thread-safe on WIN32 ?!? */
  va_end(vl);
  fprintf(stderr, "\n");

  exit(2);
}
#define FAIL(msg) fail("%s", msg);

/* Global variables */

int iVerbose = FALSE;
FILE *mf;			    /* Message output file */

/* Forward references */

char *version(void);		    /* Build the version string */
void usage(int);
int IsSwitch(char *pszArg);
int is_redirected(FILE *f);	    /* Check if a file handle is the console */
int GetEscChar(char *pszIn, char *pc); /* Get one escaped character */
int GetRxCharSet(char *pszOld, char cSet[256], int *piSetSize, char *pcRepeat);
int GetEscChars(char *pBuf, char *pszFrom, size_t iSize);
int InitBackBuf(char *psz0);
int FGetC(FILE *f);
int FSeek(FILE *f, long lOffset, int iOrigin);
int FPutC(int c, FILE *f);
size_t FWrite(const void *buf, size_t size, size_t count, FILE *f);
char *EscapeChar(char *pBuf, char c);
int PrintEscapeChar(FILE *f, char c);
int PrintEscapeString(FILE *f, char *pc);
void MakeRoom(char **ppOut, int *piSize, int iNeeded);
int MergeMatches(char *new, int iNewSize, char *match, int nMatch, char **ppOut);
int IsSameFile(char *pszPathname1, char *pszPathname2);

/*****************************************************************************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Main program routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int c;		    /* Current character */
  char old[SZ] = "";	    /* old string, to be replaced by the new string */
  char new[SZ] = "";	    /* New string, to replace the old string */
  char *maybe=malloc(SZ);   /* Possible matching input */
  int iMaybeSize = SZ;
  int oldDone = FALSE;
  int newDone = FALSE;
  int iNewSize = 0;	    /* length of the new string */
  int ixOld;		    /* Index in the old string */
  int ixMaybe = 0;	    /* Index in the maybe string */
  FILE *sf = NULL;	    /* Source file handle */
  FILE *df = NULL;	    /* Destination file handle */
  int i;
  char *pszInName = NULL;
  char *pszOutName = NULL;
  char szBakName[FILENAME_MAX+1];
  int iSameFile = FALSE;    /*  Backup the input file, and modify it in place. */
  int iCopyTime = FALSE;    /*  If true, set the out file time = in file time. */
  struct stat sInTime = {0};
  int demime = FALSE;
  long lnChanges = 0;	    /*  Number of changes done */
  int iQuiet = FALSE;
  int iBackup = FALSE;
  char cSet[256];	    /*  Character set to match */
  int iSetSize;		    /*  Number of valid characters in the set. */
  char cRepeat = '\0';	    /*  Repeat character. Either '?', '+', '*', or NUL. */
  int iOptionI = FALSE;	    /*  TRUE = -i option specified */
  int iEOS = FALSE;	    /*  TRUE = End Of Switches */
  char *pszPathCopy = NULL;
  char *pszDirName = NULL;  /*  Output file directory */
  char *pszOld8 = old;
  char *pszNew8 = new;

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
    if ((!iEOS) && IsSwitch(pszArg)) {          /* Process switches first */
      char *pszOpt = pszArg+1;
      if (   strieq(pszOpt, "?")
	  || strieq(pszOpt, "h")
	  || strieq(pszOpt, "-help")) {
	usage(0);
      }
      if (streq(pszOpt, "-")) {		/* End of switches */
	iEOS = TRUE;
	continue;
      }
      if (streq(pszOpt, ".")) {		/* No operation */
	oldDone = TRUE;
	newDone = TRUE;
	continue;
      }
      if (streq(pszOpt, "=")) {		/* Decode Mime =XX codes */
	demime = '=';
	oldDone = TRUE;
	newDone = TRUE;
	continue;
      }
      if (streq(pszOpt, "%")) {		/* Decode URL %XX codes */
	demime = '%';
	oldDone = TRUE;
	newDone = TRUE;
	continue;
      }
      if (streq(pszOpt, "#")) {		/* End of command line */
	/* Useful for adding comments in a Windows pipe */
	break;
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
      if (strieq(pszOpt, "f")) {		/* Fixed string <==> no regexp */
	cRepeat = '\xFF';
	continue;
      }
      if (strieq(pszOpt, "i")) {
	int iErr = InitBackBuf(argv[++i]);
	if (iErr) {
fail_no_mem:
	  FAIL("Not enough memory");
	}
	iOptionI = TRUE;
	continue;
      }
      if (strieq(pszOpt, "nb")) {
	iBackup = FALSE;
	continue;
      }
      if (strieq(pszOpt, "pipe")) {	/* Now the default. Left for compatibility with early version. */
	sf = stdin;
	df = stdout;
	continue;
      }
      if (strieq(pszOpt, "q")) {
	iQuiet = TRUE;
	continue;
      }
      if (strieq(pszOpt, "same")) {
	iSameFile = TRUE;
	continue;
      }
      if (strieq(pszOpt, "st")) {
	iCopyTime = TRUE;
	continue;
      }
      if (streq(pszOpt, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(pszOpt, "V")) {
	printf("%s\n", version());
	exit(0);
      }
      /* Default: Assume it's not a switch, but a string to replace */
    }
    if (!oldDone) {
      strncpy(old, pszArg, sizeof(old));
      oldDone = TRUE;
      continue;
    }
    if (!newDone) {
      iNewSize = GetEscChars(new, pszArg, sizeof(new));
      newDone = TRUE;
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
    usage(2);		    /* Error: Too many arguments */
  }

  if (!oldDone && !demime) usage(2);

  /* Report what the message stream is */
  DEBUG_CODE(
    if (mf == stderr) {	/* If stdout is redirected to a file or a pipe */
      DEBUG_FPRINTF((mf, "// Debug output sent to stderr.\n"));
    } else {
      DEBUG_FPRINTF((mf, "// Debug output sent to file #%d.\n", fileno(mf)));
    }
  )

  /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
  _setmode( _fileno( stdin ), _O_BINARY );
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  if (((!pszInName) || streq(pszInName, "-")) && iOptionI) {
    pszInName = DEVNUL;
    iSameFile = FALSE;	/*  Meaningless in this case. Avoid issues below. */
  }
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

  /* Identify the input encoding, and change the arguments encoding to match it */
#ifdef _WIN32
  {
    UINT inputCP = consoleCodePage;
    int err;
    DWORD dwBOM = 0;
    struct stat buf;			/* Use MSC 6.0 compatible names */
    int h = fileno(sf);			/* Get the file handle */
    int nc = 0;
    err = fstat(h, &buf);		/* Get information on that handle */
    if (err) fail("Can't stat the input file.\n");
    if (buf.st_mode & S_IFREG) {	/* It's a regular file */
      for (nc=0; nc<3; nc++) {
        int c = FGetC(sf);
        if (c == EOF) break;
        ((BYTE *)&dwBOM)[nc] = (BYTE)c;
      }
      if (dwBOM == 0xBFBBEF) {		/* If this is an UTF-8 BOM */
      	inputCP = CP_UTF8;
      } else {
	if ((dwBOM & 0xFFFF) == 0xFEFF) { /* If this is an UTF-16 BOM */
	  fail("UTF-16 files are not supported.\n");
      	}
      	inputCP = CP_ACP;
      }
    }
    FSeek(sf, -nc, SEEK_CUR); /* Return to the beginning of the input file */
    DEBUG_FPRINTF((mf, "// The input encoding is #%d\n", inputCP));
    /* Now we need to convert the old and new strings to the input encoding */
    pszOld8 = strdup(old);
    pszNew8 = strdup(new);
    ConvertString(old, sizeof(old), CP_UTF8, inputCP, NULL);
    ConvertString(new, sizeof(new), CP_UTF8, inputCP, NULL);
    iNewSize = (int)strlen(new);
  }
#endif

  if (iVerbose) {
    if (demime && !iQuiet) fprintf(mf, "// Replacing Mime %cXX codes.\n", demime);
    if (old[0] && !iQuiet) {
      fprintf(mf, "// Replacing \"%s\" (\"", pszOld8); /* Use the UTF-8 version in Windows */
      PrintEscapeString(mf, old);
      fprintf(mf, "\") with \"%s\" (\"", pszNew8); /* Use the UTF-8 version in Windows */
      PrintEscapeString(mf, new);
      fprintf(mf, "\").\n");
    }
  }

  ixOld = 0;
  ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
  ixMaybe = 0;

  while ((c = FGetC(sf)) != EOF) {	 /* Read chars until End of file */
    if (demime && (c == demime)) {
      char c0, c1, sz[3];
      int ic;

      if ((c0 = (char)FGetC(sf)) == EOF) {
	FPutC(c, df);
	break;
      }
      /* At the end of a line, it signals a broken line. Merge halves. */
      if (c0 == 0x0A) {
	/*  ~~jfl 2003-09-23 Added support for Unix-style files. Don't output anything. */
	lnChanges += 1;
	continue;
      }
      if ((c1 = (char)FGetC(sf)) == EOF) {
	FPutC(c, df);
	FPutC((char)c0, df);
	break;
      }
      /* At the end of a line, it signals a broken line. Merge halves. */
      if ((c0 == 0x0D) && (c1 == 0x0A)) {
	/*  ~~jfl 1999-08-02 Don't output anything. */
	lnChanges += 1;
	continue;
      }
      /* Else it's an ASCII code */
      sz[0] = (char)c0;
      sz[1] = (char)c1;
      sz[2] = '\0';
      DEBUG_FPRINTF((mf, "// Found code %s: ", sz));
      if (sscanf(sz, "%X", &ic)) {
	DEBUG_FPRINTF((mf, "Changed to char %c.\n", ic));
	FPutC((char)ic, df);
	lnChanges += 1;
      } else {
	DEBUG_FPRINTF((mf, "Not a valid code.\n"));
	FPutC('=', df);
	FWrite(sz, 2, 1, df);
      }
      continue;
    }

try_next_set:
    DEBUG_CODE_IF_ON(
      char cBuf[8];
      fprintf(mf, "// Trying to match '%s' in set [", EscapeChar(cBuf, (char)c));
      for (i=0; i<iSetSize; i++) PrintEscapeChar(mf, cSet[i]);
      fprintf(mf, "]%s ... ", cRepeat ? EscapeChar(cBuf, cRepeat) : "");
    );

    if (memchr(cSet, c, iSetSize)) {	/* If c belongs to the old string */
      DEBUG_CODE_IF_ON(
	char cBuf[8];
	fprintf(mf, "Match! Next is old[%d]='%s'\n", ixOld, EscapeChar(cBuf, old[ixOld]));
      )

      MakeRoom(&maybe, &iMaybeSize, ixMaybe+1); /*  If needed, extend the buffer */
      maybe[ixMaybe++] = (char)c;		/* Save the matching character */
      if (cRepeat == '?') cRepeat = '\0'; /* We've found it. No more expected. */
      if (cRepeat == '+') cRepeat = '*';  /* We've found it. More possible. */
      if (cRepeat == '*') continue;
      if (!old[ixOld]) {                  /* and if it is the last char. of the string to replace */
	char *new2;
	int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
	FWrite(new2, iNewSize2, 1, df);       /* then write new string */
	free(new2);
	lnChanges += 1;
	ixOld = 0;                      /* and start over again. */
	ixMaybe = 0;
      }
    } else {              /* Else it is an unexpected char. */
      DEBUG_CODE_IF_ON(
	char cBuf[8];
	fprintf(mf, "No match. Next is old[%d]='%s'\n", ixOld, EscapeChar(cBuf, old[ixOld]));
      )

      if ((cRepeat == '?') || (cRepeat == '*')) {
	if (old[ixOld]) {
	  ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
	  goto try_next_set;
	} else {		    /* The set was complete. Write the new string */
	  char *new2;
	  int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
	  FWrite(new2, iNewSize2, 1, df);       /* then write new string */
	  free(new2);
	  lnChanges += 1;
	}
	ixMaybe = 0;
      }
      if (ixMaybe) {                  /* If there were pending characters */
	/*  ~~jfl 2002-11-28 In case of a partial match, output 1 character only, and look for another match starting on the next. */
	FSeek(sf, -ixMaybe, SEEK_CUR); /*  Backtrack to the 2nd input character */
	MakeRoom(&maybe, &iMaybeSize, ixMaybe+1); /*  If needed, extend the buffer */
	maybe[ixMaybe++] = (char)c;
	c = maybe[0];
      }
      ixOld = 0;
      ixMaybe = 0;
      FPutC(c, df);	    /* Then output the given character. */
    }
    ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
  }
  DEBUG_FPRINTF((mf, "// End of file. Flushing remainders.\n"));
  if (((cRepeat == '?') || (cRepeat == '*')) && !old[ixOld]) {
    /* The set was complete. Write the new string */
    char *new2;
    int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
    FWrite(new2, iNewSize2, 1, df);       /* then write new string */
    free(new2);
    lnChanges += 1;
  } else if (ixOld) {
    FWrite(maybe, ixMaybe, 1, df); /* Flush an uncompleted old string */
  }

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);

  if (iSameFile) {
    if (iBackup) {	/* Create an *.bak file in the same directory */
      unlink(szBakName); 		/* Remove the .bak if already there */
      DEBUG_FPRINTF((mf, "// Rename \"%s\" as \"%s\"\n", pszInName, szBakName));
      rename(pszInName, szBakName);	/* Rename the source as .bak */
    } else {		/* Don't keep a backup of the input file */
      DEBUG_FPRINTF((mf, "// Remove \"%s\"\n", pszInName));
      unlink(pszInName); 		/* Remove the original file */
    }
    DEBUG_FPRINTF((mf, "// Rename \"%s\" as \"%s\"\n", pszOutName, pszInName));
    rename(pszOutName, pszInName);	/* Rename the destination as the source */
    pszOutName = pszInName;
  }

  if ((sf != stdin) && (df != stdout) && iCopyTime) {
    struct utimbuf sOutTime = {0};
    sOutTime.actime = sInTime.st_atime;
    sOutTime.modtime = sInTime.st_mtime;
    utime(pszOutName, &sOutTime);
  }

  if (iVerbose) fprintf(mf, "// Remplace: %ld changes done.\n", lnChanges);

  return ((lnChanges>0) ? 0 : 1);              /* and exit */
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
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help screen, and exit.		      |
|									      |
|   Parameters:     int err		    The exit code to pass to the OS.  |
|									      |
|   Returns:	    N/A 						      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
#if defined(_MSDOS) || defined(_WIN32)
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
	  );
}

void usage(int err)
    {
    FILE *f;

    f = stdout; 		/* Assume output on stdout will be visible */
    if (err && is_redirected(f))	/* If error and stdout redirected... */
	f = stderr;			/* ... then use stderr */

    /* Note: The help is too long, and needs to be split into several sub strings */
    /*       Also be careful of the % character that appears in some options */
    fprintf(f, "\
\n\
remplace version %s - Replace substrings in a stream\n\
\n\
Usage: remplace [SWITCHES] OPERATIONS [FILES_SPEC]\n\
\n\
files_spec: [INFILE [OUTFILE|-same]]\n\
  INFILE  Input file pathname. Default or \"-\": stdin\n\
  OUTFILE Output file pathname. Default or \"-\": stdout\n", version());
    fprintf(f, "%s", "\
\n\
operation: {old_string new_string}|-=|-%|-.\n\
  -=      Decode Mime =XX codes.\n\
  -%      Decode URL %XX codes.\n\
  -.      No change.\n\
\n\
Note that the input is byte-oriented, not line oriented. So both the old\n\
string and new string can span multiple lines.\n\
\n\
switches:\n\
  -#      Ignore all further arguments.\n\
  -?      Display this brief help screen.\n\
  --      End of switches.\n\
  -bak    When used with -same, create a backup file of the input file\n"
#ifdef _DEBUG
"  -d      Output debug information\n"
#endif
"\
  -f      Fixed old string = Disable the regular expression subset supported.\n\
  -i TEXT Input text to use before input file, if any. (Use - for force stdin)\n\
  -q      Quiet mode. No status message.\n\
  -same   Modify the input file in place. (Default: Automatically detected)\n\
  -st     Set the output file time to the same time as the input file.\n\
  -v      Verbose mode.\n\
  -V      Display this program version\n\
\n\
Examples:\n"
);
    fprintf(f,
#if defined _MSDOS || defined _WIN32
"  remplace \\n \\r\\n <unixfile >dosfile\n\
  remplace -t \\n \\r\\n unixfile -same\n\
  remplace -= unreadable_mime_file\n\
  remplace \\CHICAGO \\WIN95 config.sys -same -nb\n\
\n\
Note that the MSVC command line parser interprets quotes itself this way:\n\
Characters surrounded by \"s are parsed as a single argument. \"s are removed.\n\
Use \\\" to enter a \". \\ series are used literally, unless followed by a \".\n\
In that case, the \\s and \" are treated as by a C compiler.\n\
Special characters: Use \\r for CR, \\n for LF, \\x3C for <, \\x3E for >.\n\
Use the verbose mode to see how quotes and backslashes went through.\n"
#endif
#if defined __unix__
"  remplace \\\\n \\\\r\\\\n <unixfile >dosfile\n\
  remplace -t \\\\n \\\\r\\\\n unixfile -same\n\
  remplace -= unreadable_mime_file\n\
  remplace New-York \"Big apple\" catalog -same -nb\n\
\n\
Note that the Unix shells interpret quotes and backslashes themselves. Hence\n\
the double backslashes in the examples.\n\
Special characters: Use \\\\r for CR, \\\\n for LF, \\\\x3C for <, \\\\x3E for >.\n\
Use the verbose mode to see how quotes and backslashes went through.\n"
#endif
"\n\
Regular expressions subset for the old_string:\n\
  .     Matches any character.\n\
  c?    Matches 0 or 1 occurence of c.\n\
  c*    Matches 0 or plus occurences of c.\n\
  c+    Matches 1 or plus occurences of c.\n\
  [abc] Matches any of the enumerated characters. Use [[] to match one [.\n\
  [a-z] Matches any character in the specified range.\n\
  [^ab] Matches all but the enumerated characters.\n"
#if defined _MSDOS || defined _WIN32
"        Warning: ^ is cmd prompt escape character. Double it if needed.\n"
#endif
"\n\
The new string may contain the following special sequences\n\
  \\\\    Replaced by a single \\\n\
  \\\\0   Replaced by the current matching input\n\
\n\
Return code: 0=Success; 1=No change done; 2=Error.\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);

    exit(err);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetEscChar						      |
|									      |
|   Description:    Get one escaped character from a string.		      |
|									      |
|   Parameters:     char *pszIn		The string to parse	 	      |
|		    char *pc		The characters found                  |
|									      |
|   Returns:	    The number of characters read from pszIn.                 |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    2004-04-02 JFL Created this routine.                                     |
*									      *
\*---------------------------------------------------------------------------*/

int GetEscChar(char *pszIn, char *pc)
    {
    char *psz0 = pszIn;
    char c = *(pszIn++);
    int i;

    if (c == '\\') switch(c = *(pszIn++))
        {
        case '\0':			/*  End of string */
            *pc = '\\';
            pszIn -= 1;
            break;
        case '\\':			/*  Litteral \ . */
            *pc = c;
            break;
        case '0':			/*  NUL */
            *pc = '\0';
            break;
        case 'a':			/*  Alert (Bell) */
            *pc = '\a';
            break;
        case 'b':			/*  Backspace */
            *pc = '\b';
            break;
        case 'e':			/*  Escape */
            *pc = '\x1B';
            break;
        case 'f':			/*  Form feed */
            *pc = '\f';
            break;
        case 'n':			/*  New Line */
            *pc = '\n';
            break;
        case 'r':			/*  Return */
            *pc = '\r';
            break;
        case 't':			/*  Tabulation */
            *pc = '\t';
            break;
        case 'v':			/*  Vertical tabulation */
            *pc = '\v';
            break;
        case 'x':			/*  Hexadecimal character */
            sscanf(pszIn, "%2X", &i);	/* convert next 2 chars */
            pszIn += 2;			/* Count them */
            *pc = (char)i;
            break;
        default:			/*  Anything else: Preserve the characters. */
            *pc = c;
            break;
        }
    else
        {
	*pc = c;
	}

    return (int)(pszIn - psz0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetRxCharSet					      |
|									      |
|   Description:    Get a character set from the old string.		      |
|									      |
|   Parameters:     char *pszOld	Old string		 	      |
|		    char cSet[256]	Where to store the output characters. |
|                   int iSetBufSize     Size of the cSet buffer.              |
|                   int *piSetSize      Size of the output set.               |
|                   char *pcRepeat	The optional ?+* repetition character.|
|					or \xFF to disable regexp mechanism.  |
|									      |
|   Returns:	    The number of characters read from pszOld.                |
|									      |
|   Notes:	    This is a subset of the regular expressions syntax.	      |
|									      |
|   History:								      |
|    2004-04-01 JFL Created this routine.                                     |
|    2011-01-10 JFL Renamed from GetEscCharSet to GetRxCharSet.		      |
|		    Added the option to disable this regexp mechanism.        |
|    2012-03-12 JFL Added support for excluded character sets ([^xyz]).       |
*		    							      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int GetRxCharSet(char *pszOld, char cSet[256], int *piSetSize, char *pcRepeat)
    {
    char c;
    int n = 0;
    char *psz0 = pszOld;
    char negative;
    int i;
    int hasNul;

    if (*pcRepeat != '\xFF')
	{
	switch (c = *pszOld)
	    {
	    case '[':	/*  '[' is the beginning of a set of characters */
		pszOld += 1;
		memset(cSet, 0, 256);
		negative = 0; /*  1=This is a negative set, like [^abc] */
		if (*pszOld == '^') { negative = 1; pszOld += 1; }
		if (*pszOld == ']') { cSet[(unsigned char)']'] = 1; pszOld += 1; }
		while ((c = *pszOld)) {
		  pszOld += GetEscChar(pszOld, &c);
		  if (c == ']') break;
		  cSet[(unsigned char)c] = 1;
		  if (*pszOld == '-') { /*  If this is the beginning of a range */
		    char cLast = c; /*  Avoid problem if there's no end. */
		    pszOld += 1; /*  Skip the - */
		    pszOld += GetEscChar(pszOld, &cLast);
		    if (cLast < c) cLast = c; /*  Avoid problem if ends are reversed. */
		    for (c++ ; c <= cLast; c++) {
		      cSet[(unsigned char)c] = 1;
		    }
		  }
		}
		if (negative) for (i=0; i<256; i++) cSet[i] = (char)!cSet[i];
		hasNul = cSet[0];
		for (i=1, n=0; i<256; i++) if (cSet[i]) cSet[n++] = (char)i;
		if (hasNul) cSet[n++] = '\0';  /*  Ensure the NUL is the last character, so that the debug output below does not break. */
		break;
	    case '.':	/*  '.' matches any character */
		pszOld += 1;
		for (n=0; n<256; n++)
		    {
		    cSet[n] = (char)(n+1); /*  Ensure the NUL is the 256th character, so that the debug output below does not break. */
		    }
		break;
	    default:	/*  Normal character */
		pszOld += GetEscChar(pszOld, cSet);
		n = 1;
		break;
	    }
	*piSetSize = n;
	if (n<256) cSet[n] = '\0';	/*  Convenience to make it look like a string in most cases.     */
    
	/*  Now look for the optional ?+* repetition character behind. */
	c = *pszOld;
	switch (c)
	    {
	    case '?':
	    case '+':
	    case '*':
		*pcRepeat = c;
		pszOld += 1;
		break;
	    case '\0':
	    default:
		*pcRepeat = '\0';
		break;
	    }
	}
    else /* The regexp mechanism is disabled */
	{
	c = *(pszOld++);
	cSet[0] = c;
	*piSetSize = 1;
	}

#if 0
    DEBUG_CODE_IF_ON(
	fprintf(mf, "// Extracted set \"");
	PrintEscapeString(mf, cSet);
	fprintf(mf, "\" %c, return %d\n", *pcRepeat, pszOld - psz0);
    );
#endif

    return (int)(pszOld - psz0);
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetEscChars         				      |
|									      |
|   Description:    Convert an argument string, processing \ escape sequences.|
|									      |
|   Parameters:     char *pBuf		    Where to store the output string  |
|                   char *pszFrom	    argument to process 	      |
|		    int iBufSize            Output buffer size                |
|									      |
|   Returns:	    The number of characters output			      |
|									      |
|   Notes:	    The output is not a string. It's a set of bytes.	      |
|									      |
|   History:								      |
|    2004-04-01 JFL Rewrote the argument parsing to process \ with the        |
|		    standard C conventions.				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetEscChars(char *pBuf, char *pszFrom, size_t iSize)
    {
    char *pBuf0 = pBuf;

    while (*pszFrom)
	{
	if (!iSize) break;
	pszFrom += GetEscChar(pszFrom, pBuf++);
	iSize -= 1;
        }
    if (iSize) *pBuf = '\0';	/*  Convenience to make it look like a string in most cases. */

    return (int)(pBuf - pBuf0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    EscapeChar	         				      |
|									      |
|   Description:    Convert a character into an \ escape sequence if needed.  |
|									      |
|   Parameters:     char *pBuf		    Where to store the output string  |
|                   char c		    character to convert	      |
|									      |
|   Returns:	    pBuf                            			      |
|									      |
|   Notes:	    The output buffer must be at least 5 characters long.     |
|									      |
|   History:								      |
|    2008-12-10 JFL Created this routine.                                     |
|    2016-09-14 JFL Added a case for the NUL character = '\0'.                |
*									      *
\*---------------------------------------------------------------------------*/

char *EscapeChar(char *pBuf, char c)
    {
    if (!c)
	strcpy(pBuf, "\\0");
    else if ((c < ' ') || ((unsigned char)c > (unsigned char)'\x7F'))
	sprintf(pBuf, "\\x%02X", (unsigned char)c);
    else if (c == '\\') 
	strcpy(pBuf, "\\\\");
    else
	sprintf(pBuf, "%c", c);

    return pBuf;
    }

int PrintEscapeChar(FILE *f, char c)
    {
    char buf[8];

    return fprintf(f, "%s", EscapeChar(buf, c));
    }

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int PrintEscapeString(FILE *f, char *pc)
    {
    int n = 0;
    char c;

    while ((c = *(pc++))) n += PrintEscapeChar(f, c);

    return n;
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FGetC	         				      |
|									      |
|   Description:    Get a character from a file stream.			      |
|									      |
|   Parameters:     FILE *f		    The stream handle		      |
|									      |
|   Returns:	    The character read, or EOF.				      |
|									      |
|   Notes:	    This is a front end to the standard C library fgetc().    |
|		    The goal is to allow limited backtracking even on the     |
|		     standard input stream.				      |
|									      |
|   History:								      |
|    2005-06-10 JFL Created this routine.				      |
|    2008-12-02 JFL Fixed a bug in FSeek, which would not detect seek errors. |
|    2008-12-10 JFL Updated the fix, which was buggy.                         |
*									      *
\*---------------------------------------------------------------------------*/


char *pszBackBuf = NULL;/* Address of a circular back buffer */
char *pszBBEnd;		/* Back buffer end = First address beyond the last character */
char *pszBBHead;	/* Where to store the next chatacter that will be read */
char *pszBBTail;	/* Oldest character in the back buffer */
size_t lBB = 0;		/* Length of back buffer */
size_t nBB;		/* Number of characters stored in the back buffer */
size_t ixBB;		/* Current position relative to (backwards from) the head */

char *BBWrap(char *p) {	/* Wrap the pointer, to make sure it remains in the circular buffer */
  while (p < pszBackBuf) p += lBB;
  while (p >= pszBBEnd) p -= lBB;
  return p;
}

int InitBackBuf(char *psz0) { /* Returns 0=Success, else error */
  ixBB = nBB = strlen(psz0);
  lBB = max(nBB, 1024);
  if (!pszBackBuf) {
    pszBackBuf = malloc(lBB);
    if (!pszBackBuf) return 1;
  }
  pszBBEnd = pszBackBuf + lBB;
  strncpy(pszBackBuf, psz0, nBB);
  pszBBTail = pszBackBuf;
  pszBBHead = BBWrap(pszBackBuf + nBB);
  return 0;
}

int FGetC(FILE *f) {
  if (!pszBackBuf) InitBackBuf("");
  if (!pszBackBuf) return EOF;
  if (!ixBB) {
    int iRet = fgetc(f);
    if (iRet == EOF) return iRet;
    if (nBB == lBB) pszBBTail = BBWrap(pszBBTail + 1); /*  Drop one from the tail. */
    *pszBBHead = (char)iRet;
    pszBBHead = BBWrap(pszBBHead + 1);
    if (nBB < lBB) nBB += 1;
    return iRet;
  } else {
    return *BBWrap(pszBBHead-(ixBB--));
  }
}

int FSeek(FILE *f, long lOffset, int iOrigin) {
  if (iOrigin == SEEK_CUR) {
    if (lOffset >= 0) {
      long l;
      for (l=0; l<lOffset; l++) FGetC(f);
      return fseek(f, 0, SEEK_CUR);
    }
    ixBB += (size_t)-lOffset;
    if (ixBB <= nBB) return 0; /* Success */
    /* Else error, not enough data recorded for this move. */
  }
  InitBackBuf(""); /* Clear the whole back buffer */
  return 1; /* Error, unsupported */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FPutC	         				      |
|									      |
|   Description:    Output a character to a file stream.		      |
|									      |
|   Parameters:     char c		    The character to output	      |
|		    FILE *f		    The output stream handle	      |
|									      |
|   Returns:	    The character written, or EOF.			      |
|									      |
|   Notes:	    This is a front end to the standard C library fputc().    |
|		    The difference is that this one flushes the output at     |
|		    the end of every line for the console and pipes.	      |
|		    Useful to see output in real time in long complex cmds.   |
|		    Output to stdout is assumed to be to a console or pipe.   |
|									      |
|   History:								      |
|    2010-12-19 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int FPutC(int c, FILE *f) {
  int ret = fputc(c, f);
  if ((f == stdout) && (ret != EOF) && (c == '\n')) {
    fflush(f);
  }
  return ret;
}

size_t FWrite(const void *buf, size_t size, size_t count, FILE *f) {
  size_t ret = fwrite(buf, size, count, f);
  size *= ret; /* The total size actually written */
  if ((f == stdout) && size && memchr(buf, '\n', size)) {
    fflush(f);
  }
  return ret;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MakeRoom	         				      |
|									      |
|   Description:    Make sure a buffer is larger than a minimal size	      |
|									      |
|   Parameters:     							      |
|									      |
|   Returns:	    Nothing						      |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2012-02-29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

void MakeRoom(char **ppOut, int *piSize, int iNeeded) {
  if (iNeeded > *piSize) {
    *piSize = iNeeded+80;
    DEBUG_FPRINTF((mf, "// Reallocing buffer 0x%p to size %u\n", *ppOut, *piSize));
    *ppOut = realloc(*ppOut, *piSize);
    if (!*ppOut) {
      fprintf(stderr, "Error: not enough memory.\n");
      exit(1);
    }
  }
  return;
}
  
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MergeMatches         				      |
|									      |
|   Description:    Build the output string based on regex and matches	      |
|									      |
|   Parameters:     							      |
|									      |
|   Returns:	    The size of the output string			      |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2012-02-29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int MergeMatches(char *new, int iNewSize, char *match, int nMatch, char **ppOut) {
  char *pOut = malloc(SZ);
  int nOutSize = SZ;
  char *pc;
  int ixOut = 0;
  int n;
  
  DEBUG_ENTER(("MergeMatches(\"%s\", %d, \"%.*s\", %d, 0x%p);\n", new, iNewSize, nMatch, match, nMatch, ppOut));

  while (iNewSize) {
    pc = memchr(new, '\\', iNewSize);
    if (pc) {
      n = (int)(pc - new);
    } else {
      n = iNewSize;
    }
    if (n) {
      MakeRoom(&pOut, &nOutSize, ixOut+n);
      memcpy(pOut + ixOut, new, n);
      ixOut += n;
      new += n;
      iNewSize -= n;
    }
    if (pc) {
      MakeRoom(&pOut, &nOutSize, ixOut+2);
      switch (*(pc+1)) {
      case '0': { /* Replace \0 by the full matching string */
	memcpy(pOut + ixOut, match, nMatch);
	ixOut += nMatch;
	new = pc+2;
	iNewSize -= 2;
	break;
      }
      case '\\': /* Replace \\ by a single \ */
      	pOut[ixOut++] = '\\';
      	new += 2;
	iNewSize -= 2;
      	break;
      default: /* Let any other \? sequence fall through unchanged */
      	pOut[ixOut++] = *(new++);
      	iNewSize -= 1;
      	if (iNewSize) {
	  pOut[ixOut++] = *(new++);
	  iNewSize -= 1;
	}
	break;
      }
    }
  }
  *ppOut = pOut;

  DEBUG_LEAVE(("return %d; // \"%.*s\"\n", ixOut, ixOut, pOut));
  return ixOut;
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

