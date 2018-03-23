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
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.0.2"
#define PROGRAM_DATE    "2018-03-23"

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

#define exists(pathname) (access(pathname, F_OK) != -1) /* Check if a pathname exists */

/********************** End of OS-specific definitions ***********************/

/* Global variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */

/* Forward declarations */
char *version(int iVerbose);
void usage(void);
int IsSwitch(char *pszArg);
int rmdirRF(const char *path, int iFlags); /* Remove a directory, and all its files and subdirectories. */

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

#define FLAG_VERBOSE	0x0001
#define FLAG_NOEXEC	0x0002

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

  iErr = 0;
  if (iTest) {		/* Call the raw rmdir() function */
    iErr = rmdir(pszPath);
  } else if (iForce) {	/* Recursively delete everything */
    int iFlags = 0;
    if (iVerbose) iFlags |= FLAG_VERBOSE;
    if (iNoExec) iFlags |= FLAG_NOEXEC;
    nErr = rmdirRF(pszPath, iFlags);
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
    fprintf(stderr, "%s \"%s\": Error: %s!\n", program, pszPath, strerror(errno));
    nErr = 1;
  }
  if (nErr) {
    if (nErr > 1) fprintf(stderr, "%s: %d files or directories could not be deleted!\n", program, nErr);
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
%s version %s\n\
\n\
Remove a directory\n\
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
  -X          NoExec mode: Display what will be deleted, but don't do it\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, program, version(FALSE), progcmd);
#ifdef __unix__
  printf("\n");
#endif
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
|   Function	    rmdirRF						      |
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
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int rmdirRF(const char *path, int iFlags) {
  char *pPath;
  int iErr;
  struct stat sstat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix;

  DEBUG_ENTER(("rmdirRF(\"%s\");\n", path));

  iErr = lstat(path, &sstat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT) && !iVerbose) return 0; /* Already deleted. Not an error. */
  if (iErr) return 1;
  if (!S_ISDIR(sstat.st_mode)) {
    errno = ENOTDIR;
    return 1;
  }

  pDir = opendir(path);
  if (!pDir) return 1;
  while ((pDE = readdir(pDir))) {
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    iErr = 0;
    pPath = NewPathName(path, pDE->d_name);
    pszSuffix = "";
    switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	iErr = rmdirRF(pPath, iFlags);
      	pszSuffix = DIRSEPARATOR_STRING;
      	break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      case DT_LNK:
      	pszSuffix = ">";
      	/* Fall through into the DT_REG case */
#endif
      case DT_REG:
      	if (iVerbose) printf("%s%s\n", pPath, pszSuffix);
      	if (!iNoExec) iErr = (unlink(pPath) != 0);
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
      if (pDE->d_type != DT_DIR) fprintf(stderr, "%s: Error deleting \"%s%s\": %s\n", program, pPath, pszSuffix, strerror(errno));
      nErr += iErr;
      /* Continue the directory scan, looking for other files to delete */
    }
    free(pPath);
  }
  closedir(pDir);

  iErr = 0;
  pszSuffix = DIRSEPARATOR_STRING;
  if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
  if (iVerbose) printf("%s%s\n", path, pszSuffix);
  if (!iNoExec) iErr = rmdir(path);
  if (iErr) {
    fprintf(stderr, "%s: Error deleting \"%s%s\": %s\n", program, path, pszSuffix, strerror(errno));
    nErr += 1;
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

