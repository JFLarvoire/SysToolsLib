/*****************************************************************************\
*		    							      *
*   Filename:	    md.c						      *
*		    							      *
*   Description:    Test the mkdir() function				      *
*		    							      *
*   Notes:	    Uses our custom debugging macros in debugm.h.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc md.c -o md		    # Release mode version    *
*		    gcc -D_DEBUG md.c -o md.debug   # Debug version	      *
*		    							      *
*   History:								      *
*    2017-10-04 JFL Created this program, as a test of MsvcLibX's mkdir().    *
*    2017-10-11 JFL Updated the help screen for Windows. Version 1.0.1.       *
*    2018-05-31 JFL Bug fix: mkdirp() worked, but returned an error, if the   *
*		     path contained a trailing [back]slash. Version 1.0.2.    *
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.0.2"
#define PROGRAM_DATE    "2018-05-31"

#define _GNU_SOURCE	/* Use GNU extensions. And also MsvcLibX support for UTF-8 I/O */

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>	/* For mkdir() */

/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#ifndef S_IRWXUGO
#define S_IRWXUGO  (S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#endif /* defined(_WIN32) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#endif /* defined(_MSDOS) */

/*********************************** Other ***********************************/

#ifndef EXE_OS_NAME
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

/* Forward declarations */
char *version(int iVerbose);
void usage(void);
int IsSwitch(char *pszArg);
int isdir(const char *pszPath); /* Is this an existing directory */
int mkdir1(const char *path, mode_t mode, int iVerbose); /* Call mkdir() */
int mkdirp(const char *path, mode_t mode, int iVerbose); /* Same as mkdir -p */

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
|    2014-02-05 JFL Created this routine                                      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int iErr;
  int i;
  char *pszPath = NULL;
  int iMode = S_IRWXUGO;
  int iParent = TRUE;		/* TRUE = Create all parent directories */
  int iVerbose = FALSE;
  int iTest = FALSE;		/* TRUE = raw test mode */
  int iRet = 0;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      char *opt = arg+1;
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_MORE();
	  continue;
	}
      )
      if (   streq(opt, "help")
	  || streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "p")) {	/* Create all intermediate directories */
	iParent = TRUE;
	continue;
      }
      if (streq(opt, "P")) {	/* Do not create all intermediate directories */
	iParent = FALSE;
	continue;
      }
      if (streq(opt, "t")) {	/* Raw mkdir() test mode */
	iTest = TRUE;
	continue;
      }
      if (streq(opt, "v")) {	/* Verbose infos */
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	printf("%s\n", version(iVerbose));
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    /* If it's an argument */
    if (!pszPath) {
      pszPath = arg;
      continue;
    }
    printf("Unexpected argument %s. Ignored.\n", arg);
    continue;
  }

  if (!pszPath) usage();

  if (iTest) {		/* Call the raw mkdir() function */
    iErr = mkdir(pszPath, iMode);
  } else if (iParent) {	/* Create all parent directories */
    iErr = mkdirp(pszPath, iMode, iVerbose);
  } else {		/* Create just the one directory, if needed */
    if (isdir(pszPath)) { /* Check if it's necessary */
      DEBUG_PRINTF(("// The directory already exists\n"));
      return 0;
    }
    iErr = mkdir1(pszPath, iMode, iVerbose);
  }
  if (iErr) {
    DEBUG_PRINTF(("errno = %d\n", errno));
    fprintf(stderr, "md \"%s\": Error: %s!\n", pszPath, strerror(errno));
    iRet = 1;
  } else {
    iRet = 0;
  }
#ifdef __unix__
  printf("\n");
#endif

  return iRet;
}

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

char *version(int iVerbose) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " EXE_OS_NAME DEBUG_VERSION;
  char *pszLibVer = ""
#if defined(_MSDOS) || defined(_WIN32)
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
    ;
  char *pszVer = NULL;
  if (iVerbose) {
    pszVer = malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  printf("\n\
md version %s\n\
\n\
Create a directory\n\
\n\
Usage:\n\
  %s [SWITCHES] DIRNAME\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n"
#ifdef _DEBUG
"\
  -d          Output debug information\n"
#endif
"\
  -p          Create all intermediate directories if needed (Default)\n\
  -P          Do not create all intermediate directories if needed\n\
  -t          Test mode: Just call the raw mkdir() function\n\
  -v          Output verbose information\n\
  -V          Display this program version and exit\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, version(FALSE), 
#ifdef __unix__
  "md"
#else
  "\"md.exe\""
#endif

);
#ifdef __unix__
  printf("\n");
#endif

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
|   Function	    mkdirp						      |
|									      |
|   Description     Create a directory, and all parent directories as needed. |
|									      |
|   Parameters      Same as mkdir					      |
|									      |
|   Returns	    Same as mkdir					      |
|									      |
|   Notes	    Same as mkdir -p					      |
|		    							      |
|   History								      |
|    1990s      JFL Created this routine in update.c			      |
|    2017-10-04 JFL Improved the error handling, stopping at the first error. |
|		    Avoid testing access repeatedly if we know it'll fail.    |
|    2017-10-06 JFL Added the iVerbose arguement.			      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif

int isdir(const char *pszPath) {
  struct stat sstat;
  int iErr = lstat(pszPath, &sstat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr) return 0;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(sstat.st_mode)) {
    char *pszReal = realpath(pszPath, NULL);
    int iRet = 0; /* If realpath failed, this is a dangling link, so not a directory */
    if (pszReal) iRet = isdir(pszReal);
    return iRet;
  }
#endif
  return S_ISDIR(sstat.st_mode);
}

/* Create one directory */
int mkdir1(const char *pszPath, mode_t pszMode, int iVerbose) {
  if (iVerbose) {
    char *pszSuffix = DIRSEPARATOR_STRING;
    size_t l = strlen(pszPath);
    if (l && (pszPath[l-1] == DIRSEPARATOR_CHAR)) pszSuffix = ""; /* There's already a trailing separator */
    printf("%s%s\n", pszPath, pszSuffix);
  }
#ifndef HAS_MSVCLIBX
  DEBUG_PRINTF(("mkdir(\"%s\", 0x%X);\n", pszPath, pszMode));
#endif
  return mkdir(pszPath, pszMode);
}

/* Create all parent directories */
int mkdirp(const char *pszPath0, mode_t pszMode, int iVerbose) {
  char *pszPath = strdup(pszPath0);
  int iErr = 0; /* Assume success */
  int iSkipTest = FALSE;
  DEBUG_ENTER(("mkdirp(\"%s\", 0x%X, %d);\n", pszPath, pszMode, iVerbose));
  if (pszPath) {
    char c;
    char *pc = pszPath;
    if (pc[0] && (pc[1] == ':') && (pc[2] == DIRSEPARATOR_CHAR)) pc += 2; /* Skip the drive if absolute path */
    for (c = pc[0]; c; ) { /* Repeat for all components in the path */
      while (*pc == DIRSEPARATOR_CHAR) pc++; ; /* Skip leading slashes if absolute path */
      while (*pc && (*pc != DIRSEPARATOR_CHAR)) pc++; /* Skip the file or dir name */
      c = *pc; /* Either NUL or / or \ */
      *pc = '\0'; /* Trim pszPath */
      if (iSkipTest || !isdir(pszPath)) { /* If the intermediate path does not exist */
	iErr = mkdir1(pszPath, pszMode, iVerbose); /* Then create it. */
	if (iErr) break; /* No need to go further if this failed */
	iSkipTest = TRUE; /* We know future existence tests will fail */
      }
      *pc = c; /* Restore pszPath */
      if (c && !pc[1]) break; /* This was the trailing [back]slash */
    }
  }
  free(pszPath);
  RETURN_INT_COMMENT(iErr, (iErr ? "Failed. errno=%d - %s\n" : "Success\n", errno, strerror(errno)));
}

#ifdef _MSDOS
#pragma warning(default:4100)
#endif

