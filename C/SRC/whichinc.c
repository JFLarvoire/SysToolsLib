/*****************************************************************************\
*                                                                             *
*  Filename	    whichinc.c						      *
*									      *
*  Description      Find which include files are used by a given C source     *
*									      *
*  History								      *
*    1995-05-02 JFL Created.						      *
*    2000-03-07 JFL Added the /v option.				      *
*    2001-03-22 JFL Added support for absolute pathnames.		      *
*		    Fixed bug causing a protection fault in WIN32.	      *
*		    Only display the files containing the searched name,      *
*		     if any. Document that second argument in help.	      *
*		    Version 1.2.					      *
*    2001-05-04 JFL Fixed cosmetic problem when an include file is not found. *
*		    Version 1.21.					      *
*    2009-02-24 JFL Added support for #[spaces]include and #[spaces]define.   *
*                   Allow specifying a .h in the INCLUDE path directly.       *
*		    Added option -i for adding INCLUDE paths		      *
*		    Version 1.3.					      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2016-09-23 JFL Removed warnings. No functional code change.              *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.3.1.*
*    2019-06-13 JFL Added PROGRAM_DESCRIPTION definition. Version 1.3.2.      *
*    2022-10-20 JFL Use IsSwitch() for the arguments parsing. Version 1.3.3.  *
*    2022-11-29 JFL Search the next include file when finding an MsvcLibX     *
*		    MSVC_INCLUDE_FILE() or UCRT_INCLUDE_FILE() macro.	      *
*		    Added option -q to avoid displaying duplicates, etc.      *
*    2022-11-29 JFL Make sure all paths are displayed with \ in DOS/Windows.  *
*		    Display less information by default, and move the rest    *
*		    to the verbose and debug modes. Version 1.4.	      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Find C include files used in a source file"
#define PROGRAM_NAME    "whichinc"
#define PROGRAM_VERSION "1.4"
#define PROGRAM_DATE    "2022-12-01"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _UTF8_SOURCE		/* Forces MsvcLibX into UTF-8 mode */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* MsvcLibX include files */
#include <unistd.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define _MAX_PATH  FILENAME_MAX
#define _MAX_DRIVE 3
#define _MAX_DIR   FILENAME_MAX
#define _MAX_FNAME FILENAME_MAX
#define _MAX_EXT   FILENAME_MAX

#endif /* __unix__ */

/*********************** End of OS-specific definitions **********************/

#define LINESIZE 256

typedef struct tagStringList {
  char *string;
  struct tagStringList *next;
} StringList;

#define FLAG_VERBOSE	0x01
#define FLAG_QUIET	0x02

int iDeltaShift = 2; /* # of characters to indent output per depth level */

char szNewIncludeList[4*1024] = "INCLUDE="; /* All include paths */

/* Redefine obsolete names */ 
#define strlwr _strlwr

/* Forward references */
void usage(int iRetCode);
StringList *WhichInc(char *pszName, int iFlags, int iShift, StringList *psl, char *pszSearch);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     C program main initialization routine		      |
|									      |
|   Parameters      int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The exit code to return to the OS.			      |
|									      |
|   History								      |
|    1995-05-02 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int i;
  char *pszArgName = NULL;
  char szName[FILENAME_MAX];
  int iExtension = FALSE;
  char *pc;
  char *pszSearch = NULL;
  char *pszName = szName;
  char *pszInclude = getenv("INCLUDE");
  int iNewIncludes = FALSE;
  int iFlags = 0;

  if (pszInclude) strcpy(szNewIncludeList+8, pszInclude);

  /* Get the command line arguments */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) { /* It's a switch */
      char *opt = arg+1;
      if (   streq(opt, "help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage(0);
      }
      DEBUG_CODE(
      if (streq(arg+1, "d")) {			/* -d: Debug */
	DEBUG_ON();
	continue;
      }
      )
      if (   streq(opt, "i")
	  || streq(opt, "I")) {
	pszInclude = szNewIncludeList + strlen(szNewIncludeList);
	if (pszInclude > (szNewIncludeList+8)) *(pszInclude++) = ';';
	strcpy(pszInclude, argv[++i]);
	iNewIncludes = TRUE;
	continue;
      }
      if (streq(opt, "q")) {
	iFlags |= FLAG_QUIET;
	continue;
      }
      if (streq(opt, "s")) {
	pszSearch = argv[++i];
	continue;
      }
      if (streq(opt, "v")) {
	iFlags |= FLAG_VERBOSE;
	continue;
      }
      if (streq(opt, "V")) {	/* -V: Display version information */
	puts(DETAILED_VERSION);
	return 0;
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    if (!pszArgName) {
      pszArgName = arg;
      continue;
    }
    printf("Unexpected argument: %s\nIgnored.\n", arg);
    break;	/* Ignore other arguments */
  }

  if (!pszArgName) {
    usage(1);
  }

  /* If we added new includes, update our copy of the environment */

  if (iNewIncludes) putenv(szNewIncludeList);
  pszInclude = szNewIncludeList + 8;

  /* Add the default extension to the argument if needed */

  strcpy(szName, pszArgName);
  if ( ((pc = strrchr(szName, '.')) != NULL) && !(strchr(pc, '\\')) ) {
      /* If dot found and no backslash afterwards */
    iExtension = TRUE;
  }
  if (!iExtension) {	 /* Use the default extension */
    strcat(szName, ".c");
  }

  /* If it's an include file, search it in the INCLUDE path */
  if ((access(szName, 0) == -1) && !strchr(szName, '\\')) {
    char cBuf[FILENAME_MAX];
    if (iFlags & FLAG_VERBOSE) printf("\tSearching %s in %s.\n", szName, pszInclude);
    _searchenv(szName, "INCLUDE", cBuf);
    if (!cBuf[0]) {
      printf("File not found: %s\n", szName);
      exit(1);
    }
    pszName = cBuf;
  }

  /* Create the INCLUDE_NEXT variable,
     with the tail of the INCLUDE variable after the MsvcLibX include path */
  pc = strstr(getenv("INCLUDE"), "MsvcLibX\\include;");
  if (pc) setenv("INCLUDE_NEXT", pc+17, TRUE);

  /* Search include files */

  WhichInc(pszName, iFlags, 0, NULL, pszSearch);

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    usage						      |
|									      |
|   Description     Display a brief online help and exit		      |
|									      |
|   Parameters      int iRetCode      The program exit code	 	      |
|									      |
|   Returns	    Does not return					      |
|									      |
|   History								      |
|    1995-05-02 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(int iRetCode) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: whichinc [options] {filename}\n\
\n\
List the include files referenced by the C source {filename}.\n\
Uses the INCLUDE environment variable to find them.\n\
Default {filename} extension: .c\n\
\n\
Options:\n\
  -?            Display this help and exit.\n"
#ifdef _DEBUG
"\
  -D            Output debug information.\n"
#endif
"\
  -i {path}     Add path to the include list. (May be repeated)\n\
  -q            Quiet mode: Do not report minor issues.\n\
  -s {name}     Search where name is #defined, and display the definition.\n\
  -v            Display verbose information during search.\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@free.fr\n"
#ifdef _UNIX
"\n"
#endif
);

  exit(iRetCode);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    WhichInc						      |
|									      |
|   Description     Scan a file for #include definitions		      |
|									      |
|   Parameters      char *pszName     File name                               |
|		    int iShift        How much to shift the name displayed    |
|		    StringList *psl   List of previous include files	      |
|		    char *pszSearch   Optional constant definition to search  |
|									      |
|   Returns	    StringList *      Pointer to the new head of the list     |
|									      |
|   History								      |
|    1995-05-02 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

StringList *WhichInc(char *pszName, int iFlags, int iShift, StringList *psl, char *pszSearch) {
  FILE *sf;
  char cLine[LINESIZE];
  char *pc;
  StringList *pNewSL;
  StringList *ps;
  int iNamePrinted = FALSE;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iQuiet = iFlags & FLAG_QUIET;

  /* Prevent reopening (sometimes recursively) of a file already included */
  for (ps = psl; ps; ps = ps->next) {
    if (streq(pszName, ps->string)) {
      if (iVerbose && !pszSearch) printf("%*s%s (Already included.)\n", iShift, "", pszName);
      return psl;
    }
  }

  if (!pszSearch) printf("%*s%s", iShift, "", pszName);

  /* Open the file */
  sf = fopen(pszName, "rb");
  if (!sf) {
    if (pszSearch) printf("%*s%s", iShift, "", pszName);
    printf(" (Not found. Aborting.)\n");
    exit(1);
  }

  if (!pszSearch) printf("\n");

  /* Add a new node ahead of the linked list of file names */
  pc = malloc(sizeof(StringList) + strlen(pszName) + 1);
  if (!pc) {
    printf("Not enough memory.\n");
    exit(2);
  }
  pNewSL = (StringList *)pc;
  pc += sizeof(StringList);
  strcpy(pc, pszName);
  pNewSL->string = pc;
  pNewSL->next = psl;
  psl = pNewSL;

  /* Scan the file for inclusions */
  while (fgets(cLine, LINESIZE, sf)) {
    char cLine2[LINESIZE];
    char *pszToken;
    static char *pszBlanks = " \t\n\r";
    size_t l;

    memcpy(cLine2, cLine, LINESIZE);    /* Save the line read */
    l = strlen(cLine2);
    if ((l > 0) && (cLine2[l-1] == '\n'))
	cLine2[--l] = '\0'; /* Remove the trailing new line */
    if (   ((pszToken = strtok(cLine, pszBlanks)) != NULL)
	&& (   streq(pszToken, "#include")
	    || (   streq(pszToken, "#")
		&& ((pszToken = strtok(NULL, pszBlanks)) != NULL)
		&& streq(pszToken, "include")
	       )
	   )
       ) {
      char *pszIncName;		   /* The include file name. Ex: stdio.h */
      char cBuf[FILENAME_MAX];
      char *pszEnvVar = "INCLUDE"; /* The name of the environment variable to use */

      pszToken = strtok(NULL, pszBlanks);
      pszIncName = strtok(pszToken, "\"<>");
      DEBUG_PRINTF(("\tSearching %s in %s.\n", pszIncName, getenv("INCLUDE")));

      /* Detect MsvcLibX include_next macros */
      if (   !strncmp(pszIncName, "MSVC_INCLUDE_FILE(", 18)
	  || !strncmp(pszIncName, "UCRT_INCLUDE_FILE(", 18)
	  || !strncmp(pszIncName, "WINSDK_INCLUDE_FILE(", 20)) {
	pszEnvVar = "INCLUDE_NEXT";
	pszIncName = strchr(pszIncName, '(') + 1;
	pc = strchr(pszIncName, ')');
	if (pc) {
	  if (iVerbose && !pszSearch) printf("%*s%s (include_next macro)\n", iShift+iDeltaShift, "", pszIncName);
	  *pc = '\0';
	} else {
	  if (!pszSearch && !iQuiet) printf("%*s%s (Invalid macro)\n", iShift+iDeltaShift, "", pszIncName);
	  continue;
	}
      }

#if defined(_MSDOS) || defined(_WIN32)
      { /* Convert all / to \ characters */
      int i;
      for (i=0; pszIncName[i]; i++) if (pszIncName[i] == '/') pszIncName[i] = '\\';
      }
#endif

      if (pszIncName[0] != '\\') /* ~~JFL 2001/03/22 Added this test. */
	  _searchenv(pszIncName, pszEnvVar, cBuf);
      else
	  _fullpath(cBuf, pszIncName, sizeof(cBuf));
      if (!cBuf[0]) { /* if Pathname not found, search also in same directory as parent. */
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(pszName, drive, dir, fname, ext );
	sprintf(cBuf, "TEMP=%s%s", drive, dir);
	putenv(cBuf);
	_searchenv(pszIncName, "TEMP", cBuf);
      }
      if (cBuf[0]) { /* if Pathname found */
	/* open it now */
	psl = WhichInc(cBuf, iFlags, iShift+iDeltaShift, psl, pszSearch);
      } else { /* strlen(cBuf) == 0. Pathname not found. */
	if (!pszSearch && !iQuiet) printf("%*s%s (Not found, Ignored.)\n", iShift+iDeltaShift, "", pszIncName);
      }
    }
    memcpy(cLine, cLine2, LINESIZE);    /* Restore the line read */
    if (   pszSearch
	&& ((pszToken = strtok(cLine, pszBlanks)) != NULL)
	&& (    streq(pszToken, "#define")
	    || (   streq(pszToken, "#")
		&& ((pszToken = strtok(NULL, pszBlanks)) != NULL)
		&& streq(pszToken, "define")
	       )
	   )
        ) {
      pszToken = strtok(NULL, "( \t\n\r");
      if (streq(pszToken, pszSearch)) {
	if (!iNamePrinted) {
	  printf("%*s%s\n", iShift, "", pszName);
	  iNamePrinted = TRUE;
	}
	printf("%*s%s\n", iShift+iDeltaShift, "", cLine2);
      }
    }
  }

  fclose(sf);

  return psl;
}

