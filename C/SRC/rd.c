/*****************************************************************************\
*		    							      *
*   Filename:	    rd.c						      *
*		    							      *
*   Description:    Test the rmdir() function				      *
*		    							      *
*   Notes:	    Uses our custom debugging macros in debugm.h.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc rd.c -o rd		    # Release mode version    *
*		    gcc -D_DEBUG rd.c -o rd.debug   # Debug version	      *
*		    							      *
*   History:								      *
*    2017-10-05 JFL Created this program, as a test of MsvcLibX's rmdir().    *
*    2017-10-09 JFL Bug fix: The help screen was displayed twice.             *
*    2018-03-23 JFL Fixed several problems with error messages.		      *
*		    Added routine GetProgramNames(); Use global variables     *
*		    program and progcmd for help, and all tagged messages.    *
*    2018-05-31 JFL Use the new zapFile() and zapDir() from zap.c.            *
*                   Use routine printError() for all error messages.          *
*		    Version 1.1.					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.1.1.1.*
*		    							      *
\*****************************************************************************/

#define PROGRAM_NAME    "rd"
#define PROGRAM_VERSION "1.1.1"
#define PROGRAM_DATE    "2019-04-18"

#define _GNU_SOURCE	/* Use GNU extensions. And also MsvcLibX support for UTF-8 I/O */

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>	/* For mkdir() */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#endif /* defined(_WIN32) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#endif /* defined(_MSDOS) */

/*********************************** Other ***********************************/

#ifndef DIRSEPARATOR_CHAR
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

#define exists(pathname) (access(pathname, F_OK) != -1) /* Check if a pathname exists */

/********************** End of OS-specific definitions ***********************/

/* Global variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */
int printError(char *pszFormat, ...);	/* Print errors in a consistent format */

/* Forward declarations */
void usage(void);
int IsSwitch(char *pszArg);
/* zap functions options */
typedef struct zapOpts {
  int iFlags;
  char *pszPrefix;
} zapOpts;
/* zapOpts iFlags */
#define FLAG_VERBOSE	0x0001		/* Display the pathname operated on */
#define FLAG_NOEXEC	0x0002		/* Do not actually execute */
#define FLAG_RECURSE	0x0004		/* Recursive operation */
#define FLAG_NOCASE	0x0008		/* Ignore case */
#define FLAG_FORCE	0x0010		/* Force operation on read-only files */
int zapFile(const char *path, zapOpts *pzo); /* Delete a file */
int zapFileM(const char *path, int iMode, zapOpts *pzo); /* Faster */
int zapDir(const char *path, zapOpts *pzo);  /* Delete a directory */
int zapDirM(const char *path, int iMode, zapOpts *pzo); /* Faster */

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
  int i;
  char *pszPath = NULL;
  int iForce = FALSE;		/* TRUE = Delete all files and subdirectories */
  int iVerbose = FALSE;
  int iNoExec = FALSE;
  int iTest = FALSE;		/* TRUE = raw test mode */
  int iErr;
  int nErr = 0;
  int iRet = 0;

  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  /* Process arguments */
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
      if (streq(opt, "f")) {	/* Force deleting all files and subdirectories */
	iForce = TRUE;
	continue;
      }
      if (streq(opt, "s")) {	/* Force deleting all files and subdirectories */
	iForce = TRUE;
	continue;
      }
      if (streq(opt, "t")) {	/* Raw rmdir() test mode */
	iTest = TRUE;
	continue;
      }
      if (streq(opt, "X")) {	/* NoExec mode: Display what files will be deleted */
	iNoExec = TRUE;
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "v")) {	/* Verbose infos */
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
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

  iErr = 0;
  if (iTest) {		/* Call the raw rmdir() function */
    iErr = rmdir(pszPath);
  } else if (iForce) {	/* Recursively delete everything */
    zapOpts zo = {FLAG_RECURSE | FLAG_NOCASE, ""};
    if (iVerbose) zo.iFlags |= FLAG_VERBOSE;
    if (iNoExec) zo.iFlags |= FLAG_NOEXEC;
    nErr = zapDir(pszPath, &zo);
  } else {		/* Invoke the rmdir() function, if needed */
    if (exists(pszPath)) {
      if (iVerbose) {
	char *pszSep = DIRSEPARATOR_STRING;
	if (pszPath[strlen(pszPath) - 1] == DIRSEPARATOR_CHAR) pszSep = ""; /* There's already a trailing separator */
      	printf("%s%s\n", pszPath, pszSep);
      }
      if (!iNoExec) iErr = rmdir(pszPath);
    }
  }
  if (iErr) {
    DEBUG_PRINTF(("errno = %d\n", errno));
    printError("Failed to delete \"%s\": %s", pszPath, strerror(errno));
    nErr = 1;
  }
  if (nErr) {
    if (nErr > 1) printError("%d files or directories could not be deleted", nErr);
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

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - Remove a directory\n\
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
  -f          Force deleting all files and subdirectories\n\
  -s          Force deleting all files and subdirectories\n\
  -t          Test mode: Just call the raw rmdir() function\n\
  -v          Output verbose information\n\
  -V          Display this program version and exit\n\
  -X          NoExec mode: Display what would be deleted, but don't do it\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
,
#ifdef __unix__
  progcmd
#else
  "\"rd.exe\""
#endif
);
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProgramNames					      |
|									      |
|   Description     Extract the program names from argv[0]		      |
|									      |
|   Parameters      char *argv[0]					      |
|									      |
|   Returns	    0							      |
|									      |
|   Notes	    Sets global variables program and progcmd.		      |
|		    Designed to work independantly of MsvcLibX.		      |
|		    							      |
|   History								      |
|    2018-03-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    printError						      |
|									      |
|   Description     Print error messages with a consistent format	      |
|									      |
|   Parameters      char *pszFormat					      |
|		    ...							      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    Uses global variables program and progcmd,		      |
|		    set by GetProgramNames().				      |
|		    							      |
|   History								      |
|    2018-05-31 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int printError(char *pszFormat, ...) {
  va_list vl;
  int n;

  n = fprintf(stderr, "%s: ", program);
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, ".\n");
  va_end(vl);

  return n;
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
|   Function	    NewPathName						      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
*									      *
\*---------------------------------------------------------------------------*/

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  return buf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zapDir						      |
|									      |
|   Description     Remove a directory, and all its files and subdirectories. |
|									      |
|   Parameters      const char *path		The directory pathname	      |
|		    int iFlags			Verbose & NoExec flags	      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2018-05-31 JFL Changed the iFlags argument to zapOpts *pzo.	      |
|		    The FLAG_FORCE flag now deletes read-only files.	      |
|		    Split zapFile() off of zapDir().			      |
|		    Added zapXxxM routines, with an additional iMode argument,|
|		     to avoid unnecessary slow calls to lstat() in Windows.   |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int zapFileM(const char *path, int iMode, zapOpts *pzo) {
  int iFlags = pzo->iFlags;
  char *pszSuffix = "";
  int iErr = 0;

  DEBUG_ENTER(("zapFileM(\"%s\", 0x%04X);\n", path, iMode));

  if (S_ISDIR(iMode)) {
    errno = EISDIR;
    RETURN_INT(1);
  }
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(iMode)) {
    pszSuffix = ">";
  }
#endif

  if (iFlags & FLAG_VERBOSE) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  if (iFlags & FLAG_NOEXEC) RETURN_INT(0);
  if (iFlags & FLAG_FORCE) {
    if (!(iMode & S_IWRITE)) {
      iMode |= S_IWRITE;
      DEBUG_PRINTF(("chmod(%p, 0x%X);\n", path, iMode));
      iErr = -chmod(path, iMode); /* Try making the target file writable */
      DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
    }
    if (iErr) RETURN_INT(iErr);
  }
  iErr = -unlink(path); /* If error, iErr = 1 = # of errors */

  RETURN_INT(iErr);
}

int zapFile(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapFile(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) RETURN_INT(1);
  
  iErr = zapFileM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

int zapDirM(const char *path, int iMode, zapOpts *pzo) {
  char *pPath;
  int iErr;
  struct stat sStat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFlags = pzo->iFlags;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix;

  DEBUG_ENTER(("zapDirM(\"%s\", 0x%04X);\n", path, iMode));

  if (!S_ISDIR(iMode)) {
    errno = ENOTDIR;
    RETURN_INT(1);
  }

  pDir = opendir(path);
  if (!pDir) RETURN_INT(1);
  while ((pDE = readdir(pDir))) {
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    pPath = NewPathName(path, pDE->d_name);
    pszSuffix = "";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = -lstat(pPath, &sStat); /* If error, iErr = 1 = # of errors */
#endif
    if (!iErr) switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	iErr = zapDirM(pPath, sStat.st_mode, pzo);
      	pszSuffix = DIRSEPARATOR_STRING;
      	break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      case DT_LNK:
      	pszSuffix = ">";
      	/* Fall through into the DT_REG case */
#endif
      case DT_REG:
	iErr = zapFileM(pPath, sStat.st_mode, pzo);
      	break;
      default:
      	iErr = 1;		/* We don't support deleting there */
#if defined(ENOSYS)
      	errno = ENOSYS;		/* Function not supported */
#else
      	errno = EPERM;		/* Operation not permitted */
#endif
      	pszSuffix = "?";
      	break;
    }
    if (iErr) {
      if (pDE->d_type != DT_DIR) printError("Error deleting \"%s%s\": %s", pPath, pszSuffix, strerror(errno));
      nErr += iErr;
      /* Continue the directory scan, looking for other files to delete */
    }
    free(pPath);
  }
  closedir(pDir);

  iErr = 0;
  pszSuffix = DIRSEPARATOR_STRING;
  if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
  if (iVerbose) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  if (!iNoExec) iErr = rmdir(path);
  if (iErr) {
    printError("Error deleting \"%s%s\": %s", path, pszSuffix, strerror(errno));
    nErr += 1;
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

int zapDir(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapDir(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) {
    printError("Error: Can't stat \"%s\": %s", path, strerror(errno));
    RETURN_INT(1);
  }
  
  iErr = zapDirM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

