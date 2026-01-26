/*****************************************************************************\
*                                                                             *
*   Filename	    TrueName.c						      *
*									      *
*   Description:    Resolve all links in a WIN32 pathname.                    *
*                                                                             *
*   Notes:	    TO DO: Move the WIN32-specific code to subroutines, and   *
*		    port the code to other operating systems.		      *
*		    Note that even though pathnames in Unix are generally     *
*		    case-sensitive, pathnames on Samba/CIFS mounts are not.   *
*		    so a truename program could be useful on Unix too.	      *
*		    (Actually Linux has the realpath command for that now.)   *
*		    							      *
*   History:								      *
*    2014-02-07 JFL Created this program.				      *
*    2014-03-25 JFL Fix the final name case.				      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 1.0.1.					      *
*    2015-12-18 JFL Convert the short names to long names.		      *
*                   Renamed some options, and added code page opts -A, -O, -U.*
*		    Version 1.1.					      *
*    2016-09-12 JFL Moved WIN32 UTF-8 routines to the MsvcLibX library.       *
*                   Minor tweaks to fix compilation in Linux.                 *
*		    Version 1.1.1.					      *
*    2018-04-24 JFL Use NAME_MAX from limits.h. Version 1.1.2.		      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.1.3.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.1.4.      *
*    2021-11-29 JFL Renamed ResolveLinks*() as MlxResolveLinks*().	      *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 1.1.5.		      *
*    2026-01-03 JFL Option -d can now be repeated. Version 1.1.6.	      *
*    2026-01-23 JFL Now using the rewritten MsvcLibX routines. Version 1.2.   *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Get the canonic name of a path, with all links resolved"
#define PROGRAM_NAME    "truename"
#define PROGRAM_VERSION "1.2"
#define PROGRAM_DATE    "2026-01-23"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>	/* For the symlink and readlink functions */
#include <limits.h>	/* Defines PATH_MAX and NAME_MAX */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#include <windows.h>		/* Also includes MsvcLibX' WIN32 UTF-8 extensions */
#include <iconv.h>              /* For the codePage variable */

#define cp codePage		/* Initial console code page in iconv.c */

/*********************************** Other ***********************************/

#else

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX
#endif /* defined(__unix__) */

#error "Unsupported OS. Please add support for it if needed."

#endif

/********************** End of OS-specific definitions ***********************/

/* Forward declarations */
void usage(void);

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
  ssize_t n = -1;
  char buf[PATH_MAX] = "";
  char absName[PATH_MAX];
  int bGetAbsName = TRUE;
  int bResolveLinks = TRUE;
  int bResolveShortNames = TRUE;
  int bUsedMlxResolveLinks = FALSE;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
#ifdef _WIN32
      if (streq(arg+1, "A")) {	/* Force encoding output with the ANSI code page */
	cp = CP_ACP;
	continue;
      }
#endif
      DEBUG_CODE(
	if (streq(arg+1, "d")) {
	  DEBUG_MORE();
	  continue;
	}
      )
      if (   streq(arg+1, "help")
	  || streq(arg+1, "-help")
	  || streq(arg+1, "h")
	  || streq(arg+1, "?")) {
	usage();
      }
      if (streq(arg+1, "l")) {	/* Resolve links */
	bResolveLinks = TRUE;
	continue;
      }
      if (streq(arg+1, "L")) {	/* Do not resolve links */
	bResolveLinks = FALSE;
	continue;
      }
#ifdef _WIN32
      if (streq(arg+1, "O")) {	/* Force encoding output with the OEM code page */
	cp = CP_OEMCP;
	continue;
      }
#endif
      if (streq(arg+1, "r")) {	/* Resolve relative paths = Get the absolute pathname */
	bGetAbsName = TRUE;
	continue;
      }
      if (streq(arg+1, "R")) {	/* Do not resolve relative paths = do not get the absolute pathname */
	bGetAbsName = FALSE;
	continue;
      }
      if (streq(arg+1, "s")) {	/* Resolve short names */
	bResolveShortNames = TRUE;
	continue;
      }
      if (streq(arg+1, "S")) {	/* Do not resolve short names */
	bResolveShortNames = FALSE;
	continue;
      }
#ifdef _WIN32
      if (streq(arg+1, "U")) {	/* Force encoding output with the UTF-8 code page */
	cp = CP_UTF8;
	continue;
      }
#endif
      if (streq(arg+1, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      DEBUG_CODE(
	if (streq(arg+1, "xd")) {
	  XDEBUG_ON();
	  continue;
	}
      )
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

  if (bGetAbsName) {
    char *lpPart;
    n = GetFullPathName(pszPath, sizeof(absName), absName, &lpPart);
    if (!n) {
      DEBUG_CODE(
      DWORD dwError = GetLastError();
      )
      DEBUG_PRINTF(("Win32Error = %d (0x%X)\n", dwError, dwError));
      errno = Win32ErrorToErrno();
      pfcerror("Failed to get the absolute name");
      return 1;
    }
    DEBUG_PRINTF(("absName = \"%s\"\n", absName));
    pszPath = absName;
    strcpy(buf, absName); /* In case we use option -L to NOT resolve links */
  }

  if (bResolveLinks) {
    /* Resolves links with the same algorithm as realpath().
       But don't use realpath() directly, because:
       - We want to optionally _not_ correct the name case or expand short names.
       - We need to know if it did the resolution using MlxGetFileName() or MlxResolveLinks().*/
    /* First try letting Windows do the resolution itself */
    int iErr = MlxGetFileName(pszPath, buf, sizeof(buf));
    if (iErr && (errno == ENOSYS)) {
      /* The OS does not support name resolution. Do it ourselves */
      char *pszPath2 = malloc(UTF8_PATH_MAX);
      if (!pszPath2) {
      	pfcerror(NULL); /* Will print something like "Out of memory" */
      	return 1;
      }
      bUsedMlxResolveLinks = TRUE;
      iErr = MlxResolveSubstDrives(pszPath, pszPath2, UTF8_PATH_MAX);
      if (!iErr) iErr = MlxResolveLinks(pszPath2, buf, sizeof(buf));
      free(pszPath2);
    }
    if (iErr) {
      pfcerror("Failed to resolve links");
      return 1;
    }
  }

  if (bResolveShortNames) {
    char longName[sizeof(buf)];
    n = GetLongPathName(buf, longName, sizeof(buf)); /* Also corrects the name case */
    if (!n) {
      errno = Win32ErrorToErrno();
      /* Detect a special corner case: Sometimes MlxGetFileName() processes a
         name successfully, but GetLongPathName() fails with access denied */
      if (bResolveLinks && (errno == EACCES)) {
      	if ((!strchr(buf, '~')) && (!bUsedMlxResolveLinks)) {
      	  goto print_result; /* It's not a short name, and MlxGetFileName() DID fix the name case */
      	}
      }
      pfcerror("Failed to get the long pathname");
      return 1;
    }
    strcpy(buf, longName);
  }

  /* No need to correct the pathname case anymore, as it's already done by GetLongPathName() */
print_result:
  printf("%s\n", buf);

  return 0;
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
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
  truename [SWITCHES] PATHNAME\n\
\n\
Switches:\n\
  -?          Display this help message and exit.\n"
#ifdef _WIN32
"\
  -A          Force encoding the output using the ANSI character set.\n"
#endif
#ifdef _DEBUG
"\
  -d          Output debug information.\n"
#endif
"\
  -l          Resolve symbolic links. (default)\n\
  -L          Do not resolve symbolic links.\n"
#ifdef _WIN32
"\
  -O          Force encoding the output using the OEM character set.\n"
#endif
"\
  -r          Resolve relative paths = Output absolute paths (default)\n\
  -R          Do not resolve relative paths.\n\
  -s          Resolve short names. (default)\n\
  -S          Do not resolve short names.\n"
#ifdef _WIN32
"\
  -U          Force encoding the output using the UTF-8 character encoding.\n"
#endif
"\
  -V          Display this program version and exit.\n\
"
#include "footnote.h"
);

  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FixNameCase						      |
|									      |
|   Description     Correct the file name case if needed		      |
|									      |
|   Notes	    Returns TRUE if something was changed in the name.        |
|									      |
|		    TO DO: Rewrite this routine to avoid spinning through     |
|			   all file names. Use Windows' Find* routines.	      |
|			   Also split this routine in two:		      |
|			   FixFilenameCase/FixPathnameCase		      |
|									      |
|   History								      |
|    2013-03-27 JFL Initial implementattion.				      |
|    2014-03-20 JFL Bug fix: Make sure the drive letter is upper case.        |
|		    Bug fix: Avoid an unnecessary search if the path is empty.|
*									      *
\*---------------------------------------------------------------------------*/

#if 0 /* We actually don't need this, because GetLongPathName() does it already */

int FixNameCase(char *pszPathname) {
  char *pszPath = pszPathname;
  char *pszName;
  DIR *pDir;
  struct dirent *pDE;
  int iModified = FALSE;
  int lDrive = 0;
  char szRootDir[] = "C:\\";
  char szDriveCurDir[] = "C:.";

  DEBUG_ENTER(("FixNameCase(\"%s\");\n", pszPathname));

  if (pszPathname[0] && (pszPathname[1] == ':')) { /* There's a drive letter */
    char c, C;
    lDrive = 2;
    c = pszPathname[0];
    C = (char)toupper(c);
    if (c != C) {
      pszPathname[0] = C;	/* Make sure the drive is upper case */
      iModified = TRUE;
    }
    if (!pszPathname[2]) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
  }

  pszName = strrchr(pszPathname, '\\');
  if (pszName) { /* There's a path separator */
    if (pszName != (pszPathname + lDrive)) { /* Possibly a drive letter, then a parent path name */
      *(pszName++) = '\0';
      iModified |= FixNameCase(pszPathname); /* Recursively fix the parent pathname */
    } else { /* Possibly a drive letter, then a root directory name */
      if (lDrive) { /* A drive letter, then a root directory name */
	pszPath = szRootDir; /* Use the "C:\\" copy on the stack to make sure the routine is reentrant */
	pszPath[0] = pszPathname[0];
      } else { /* Just a root directory name */
	pszPath = "\\";
      }
      pszName += 1;
    }
  } else { /* No path separator */
    pszName = pszPathname;
    if (lDrive) { /* A drive letter, then a file name */
      pszPath = szDriveCurDir; /* Use the "C:." copy on the stack to make sure the routine is reentrant */
      pszPath[0] = pszPathname[0];
      pszName += 2;  /* Skip the drive letter */
    } else {	  /* Just the file name */
      pszPath = ".";
    }
  }
  if (!*pszName) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));

  /* Scan all directory entries that match the requested name */
  pDir = opendir(pszPath);
  if (!pDir) {
    if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
    RETURN_BOOL_COMMENT(FALSE, ("Can't open directory \"%s\"\n", pszPath));
  }
  while ((pDE = readdir(pDir))) {
    if (_stricmp(pszName, pDE->d_name)) continue; /* Names differ */
    if (strcmp(pszName, pDE->d_name)) { /* If the names match, but the case differs */
      strcpy(pszName, pDE->d_name);	/* Correct the name */
      iModified = TRUE;
    }
    break;
  }
  closedir(pDir);

  if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
  RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
}

#endif
