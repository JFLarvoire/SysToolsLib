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
*		    							      *
*   History:								      *
*    2014-02-07 JFL Created this program.				      *
*    2014-03-25 JFL Fix the final name case.				      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 1.0.1.					      *
*    2015-12-18 JFL Convert the short names to long names.		      *
*                   Renamed some options, and added code page opts -A, -O, -U.*
*		    Version 1.1.					      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.1"
#define PROGRAM_DATE    "2015-12-18"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _GNU_SOURCE	/* Include extra BSD-specific functions. Implied by _GNU_SOURCE. */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>	// For the symlink and readlink functions

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

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

DWORD GetFullPathNameU(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart);
char *_fullpathU(char *absPath, const char *relPath, size_t maxLength);
int FixNameCase(char *pszPathname);
DWORD GetLongPathNameU(LPCTSTR lpShortName, LPTSTR lpBuf, DWORD nBufferLength);

#define cp codePage			/* Initial console code page in iconv.c */

#endif

/*********************************** Other ***********************************/

#ifndef OS_NAME
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/* Forward declarations */
char *version(void);
void usage(void);
int IsSwitch(char *pszArg);

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
  char buf[FILENAME_MAX] = "";
  char absName[FILENAME_MAX];
  int bGetAbsName = TRUE;
  int bResolveLinks = TRUE;
  int bResolveShortNames = TRUE;

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
	  DEBUG_ON();
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
	printf("%s\n", version());
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

  if (bGetAbsName) {
    char *lpPart;
    n = GetFullPathNameU(pszPath, sizeof(absName), absName, &lpPart);
    if (!n) {
      DEBUG_CODE(
      DWORD dwError = GetLastError();
      )
      DEBUG_PRINTF(("Win32Error = %d (0x%X)\n", dwError, dwError));
      fprintf(stderr, "truename: failed to get the absolute name of \"%s\": %s!\n", pszPath);
      return 1;
    }
    DEBUG_PRINTF(("absName = \"%s\"\n", absName));
    pszPath = absName;
    strcpy(buf, absName); // In case we use option -L to NOT resolve links
  }

  DEBUG_PRINTF(("Size of WCHAR = %d bits\n", 8*sizeof(WCHAR)));
  DEBUG_PRINTF(("Size of wchar_t = %d bits\n", 8*sizeof(wchar_t)));

  if (bResolveLinks) n = ResolveLinks(pszPath, buf, sizeof(buf));

  if (n >= 0) {
    if (bResolveShortNames) {
      char longName[sizeof(buf)];
      n = GetLongPathNameU(buf, longName, sizeof(buf)); /* This will NOT correct long names case */
      if (!n) goto report_err;
      strcpy(buf, longName);
    }
    /* Correct the pathname case.
       This is necessary because both command line arguments, and link targets,
       may not match the actual targets names case */
    FixNameCase(buf); 
    printf("%s\n", buf);
  } else {
report_err:
    DEBUG_PRINTF(("errno = %d\n", errno));
    fprintf(stderr, "truename: Error processing pathname \"%s\": %s!\n", pszPath, strerror(errno));
    return 1;
  }
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

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
	  );
}

void usage(void) {
  printf("\n\
truename version %s\n\
\n\
Get the canonic name for a pathname, resolving all links in the path.\n\
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|                                                                             |
|   Description:    Test if an argument is a command-line switch.             |
|                                                                             |
|   Parameters:     char *pszArg	    Would-be argument		      |
|                                                                             |
|   Return value:   TRUE or FALSE					      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  return (   (*pszArg == '-')
#ifndef __unix__
	  || (*pszArg == '/')
#endif
         ); /* It's a switch */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetFullPathNameU					      |
|                                                                             |
|   Description:    Get the absolute pathname for a relative UTF-8 path.      |
|                                                                             |
|   Parameters:     See WIN32's GetFullPathName()			      |
|                                                                             |
|   Return value:   The length of the full pathname, or 0 if error	      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2014-02-07 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

DWORD GetFullPathNameU(LPCTSTR lpName, DWORD nBufferLength, LPTSTR lpBuf, LPTSTR *lpFilePart) {
  WCHAR wszName[MAX_PATH];
  WCHAR wszBuf[MAX_PATH];
  char szName[MAX_PATH*4]; /* Worst case for UTF-8 is 4 bytes/Unicode character */
  int n;
  DWORD dwResult;
  WCHAR *wlpFilePart;

  DEBUG_ENTER(("GetFullPathNameU(\"%s\", %d, %p, %p);\n", lpName, nBufferLength, lpBuf, lpFilePart));

  n = MultiByteToWideChar(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpName,		/* lpMultiByteStr, */
			  lstrlen(lpName)+1,	/* cbMultiByte, */
			  wszName,		/* lpWideCharStr, */
			  MAX_PATH		/* cchWideChar, */
			  );
  if (!n) RETURN_INT_COMMENT(0, ("Failed to convert the name to Unicode\n"));

  dwResult = GetFullPathNameW(wszName, MAX_PATH, wszBuf, &wlpFilePart);
  if (!dwResult) RETURN_INT_COMMENT(0, ("GetFullPathNameW() failed\n"));

  /* nRead = UnicodeToBytes(pwStr, len, buf, bufsize); */
  n = WideCharToMultiByte(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  wszBuf,		/* lpWideCharStr, */
			  (int)dwResult + 1,	/* cchWideChar, */
			  lpBuf,		/* lpMultiByteStr, */
			  (int)nBufferLength,	/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  if (!n) RETURN_INT_COMMENT(0, ("Failed to convert the full name from Unicode\n"));

  if (lpFilePart) { /* Convert the file part, and get the length of the converted string */
    int m;	/* Length of the converted string */
    m = WideCharToMultiByte(CP_UTF8,			/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			    0,				/* dwFlags, */
			    wlpFilePart,		/* lpWideCharStr, */
			    lstrlenW(wlpFilePart),	/* cchWideChar, */
			    szName,			/* lpMultiByteStr, */
			    sizeof(szName),		/* cbMultiByte, */
			    NULL,			/* lpDefaultChar, */
			    NULL			/* lpUsedDefaultChar */
			    );
    /* (n-1) is the length of the full UTF-8 pathname */
    /* So ((n-1) - m) is the offset of the file part in the full UTF-8 pathname */
    *lpFilePart = lpBuf + (n - 1) - m;
  }

  RETURN_INT_COMMENT(n-1, ("\"%s\" \"%s\"\n", lpBuf, lpFilePart?*lpFilePart:"(NULL)"));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fullpathU						      |
|                                                                             |
|   Description:    Get the absolute pathname for a relative UTF-8 path.      |
|                                                                             |
|   Parameters:     See MSVC's _fullpath() in stdlib.h			      |
|                                                                             |
|   Return value:   Pointer to the full pathname, or NULL if error	      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2014-03-25 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

char *_fullpathU(char *absPath, const char *relPath, size_t maxLength) {
  DWORD n;
  if (!absPath) {
    absPath = malloc(MAX_PATH*4); /* Worst case for UTF-8 is 4 bytes/Unicode character */
    if (!absPath) return NULL;
  }
  n = GetFullPathNameU(relPath, (DWORD)maxLength, absPath, NULL);
  return n ? absPath : NULL;
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
	pszPath = szRootDir; // Use the "C:\\" copy on the stack to make sure the routine is reentrant
	pszPath[0] = pszPathname[0];
      } else { /* Just a root directory name */
	pszPath = "\\";
      }
      pszName += 1;
    }
  } else { /* No path separator */
    pszName = pszPathname;
    if (lDrive) { /* A drive letter, then a file name */
      pszPath = szDriveCurDir; // Use the "C:." copy on the stack to make sure the routine is reentrant
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetLongPathNameU					      |
|                                                                             |
|   Description:    Get the long pathname for a short UTF-8 path.	      |
|                                                                             |
|   Parameters:     See WIN32's GetLongPathName()			      |
|                                                                             |
|   Return value:   The length of the full pathname, or 0 if error	      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2015-12-18 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

DWORD GetLongPathNameU(LPCTSTR lpShortName, LPTSTR lpBuf, DWORD nBufferLength) {
  WCHAR wszShortName[MAX_PATH];
  WCHAR wszBuf[MAX_PATH];
  int n;
  DWORD dwResult;

  DEBUG_ENTER(("GetLongPathNameU(\"%s\", %p, %d);\n", lpShortName, lpBuf, nBufferLength));

  n = MultiByteToWideChar(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpShortName,		/* lpMultiByteStr, */
			  lstrlen(lpShortName)+1,  /* cbMultiByte, */
			  wszShortName,		/* lpWideCharStr, */
			  MAX_PATH		/* cchWideChar, */
			  );
  if (!n) RETURN_INT_COMMENT(0, ("Failed to convert the short name to Unicode\n"));

  dwResult = GetLongPathNameW(wszShortName, wszBuf, MAX_PATH);
  if (!dwResult) RETURN_INT_COMMENT(0, ("GetLongPathNameW() failed\n"));

  /* nRead = UnicodeToBytes(pwStr, len, buf, bufsize); */
  n = WideCharToMultiByte(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  wszBuf,		/* lpWideCharStr, */
			  (int)dwResult + 1,	/* cchWideChar, */
			  lpBuf,		/* lpMultiByteStr, */
			  (int)nBufferLength,	/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  if (!n) RETURN_INT_COMMENT(0, ("Failed to convert the Long name from Unicode\n"));

  RETURN_INT_COMMENT(n-1, ("\"%s\"\n", lpBuf));
}

