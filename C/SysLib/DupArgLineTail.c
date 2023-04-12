/*****************************************************************************\
*                                                                             *
*   Filename	    DupArgLineTail.c					      *
*									      *
*   Description     Recreate a command line from the tail of an argument list *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2023-04-12 JFL jf.larvoire@free.fr extracted this routine from in.c.     *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#if defined(_WIN32)
#include <windows.h>	/* for GetCommandLineW() */
#endif

#ifdef _MSDOS
#include "dos.h"	/* for _MK_FP() */
#endif

#include "CmdLine.h"	/* SysLib routines managing command-line arguments */

#include "debugm.h"	/* SysToolsLib debug macros */

#define TRUE 1
#define FALSE 0

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DupArgLineTail					      |
|									      |
|   Description     Recreate a command line from the tail of an argument list |
|									      |
|   Returns	    A new string, quoted and escaped as needed		      |
|									      |
|   Notes	    For DOS and Windows, uses the actual command line for the |
|		    current program, to make sure the quoting is done exactly |
|		    as the requestor intended it.			      |
|		    							      |
|   History								      |
|    2023-01-03 JFL Created this code in in.c.				      |
|    2023-03-27 JFL Added the actual command line processing for DOS & Windows.
|    2023-04-12 JFL Moved that code into this new routine.		      |
*									      *
\*---------------------------------------------------------------------------*/

char *DupArgLineTail(int argc, char **argv, int iArg0) {
  int i;
#if defined(_MSDOS) || defined(_WIN32)
  int i0;
#else
  int iLen;
  char *pszQuoted;
  char *pszCmd = argv[iArg0];
#endif
  int n;
  char *pszCmdLine = NULL;

  if (iArg0 >= argc) {
    errno = EINVAL; /* Invalid argument */
    return NULL;
  }

#if defined(_WIN32) /* In Windows, use the actual command line */
  {
  LPWSTR lpwCommandLine;
  WCHAR wc;

  /* Get the Unicode command line */
  lpwCommandLine = GetCommandLineW();
  /* Trim tail spaces */
  n = lstrlenW(lpwCommandLine);
  while (n && ((wc = lpwCommandLine[n-1]) != L'\0') && ((wc == L' ') || (wc == L'\t'))) lpwCommandLine[--n] = L'\0';
  /* Allocate space for the UTF8 copy */
  n += 1;	/* Count the final NUL */
  pszCmdLine = malloc(4 * n); /* Worst case */
  if (!pszCmdLine) return NULL; /* Memory allocation failed */
  /* Convert the Unicode command line to UTF-8 */
  n = WideCharToMultiByte(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpwCommandLine,	/* lpWideCharStr, */
			  n,			/* cchWideChar, */
			  pszCmdLine,		/* lpMultiByteStr, */
			  (4 * n),		/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  if (!n) { /* This should never happen, but just in case, test it anyway */ 
    DEBUG_PRINTF(("Can't convert the argument line to UTF-8\n"));
    free(pszCmdLine);
    errno = EFAULT; /* Bad address (For lack of a better errno!) */
    return NULL;
  }
  pszCmdLine = ShrinkBuf(pszCmdLine, n+1); /* Resize the memory block to fit the UTF-8 line */
  i0 = 0;	/* The Windows command line includes the argv[0] program name */
#endif /* defined(_WIN32) */
#if defined(_MSDOS)
  {
  char far *lpszCmdLine = _MK_FP(_psp, 0x81);	/* PSP:81 = Command Line */
  n = (int)(lpszCmdLine[-1]);			/* PSP:80 = Command Line Length */
  pszCmdLine = malloc(n+1);
  if (!pszCmdLine) return NULL;
  _fmemcpy(pszCmdLine, lpszCmdLine, n);		/* Make a local copy */
  pszCmdLine[n] = '\0';
  i0 = 1;	/* The DOS command line begins at argv[1], without the program name */
#endif /* defined(_MSDOS) */
#if defined(_MSDOS) || defined(_WIN32)
  /* Skip the current program arguments */
  for (i=i0; i<iArg0; i++) {
    char *pszArg = strstr(pszCmdLine, argv[i]);
    DEBUG_PRINTF(("Searching arg[%d] \"%s\" in {%s}\n", i, argv[i], pszCmdLine));
    if (!pszArg) {
      DEBUG_PRINTF(("Error: Can't find arg[%d] \"%s\" in {%s}\n", i, argv[i], pszCmdLine));
      free(pszCmdLine);
      errno = E2BIG; /* Argument list too long (For lack of a better errno!) */
      return NULL;
    }
    pszCmdLine = pszArg + strlen(argv[i]);
    while (*pszCmdLine && !isspace(*pszCmdLine)) pszCmdLine += 1;
  }
  while (*pszCmdLine && isspace(*pszCmdLine)) pszCmdLine += 1;
  DEBUG_PRINTF(("Command line: {%s}\n", pszCmdLine));
#else /* For all non-Microsoft OSs, like Unix */
  {
  /* Allocate a buffer large enough for rebuilding the command line */
  iLen = 1; /* 1 byte for the final NUL */
  for (i=iArg0; i<argc; i++) iLen += (2 * (int)strlen(argv[i])) + 2; /* Worst case for the cmd & every arg */
  pszCmdLine = malloc(iLen);
  if (!pszCmdLine) return NULL;
  /* Rebuild the command line */
  pszQuoted = CondQuoteShellArg(pszCmd);
  if (!pszQuoted) {
    free(pszCmdLine);
    return NULL;
  }
  n = sprintf(pszCmdLine, "%s", pszQuoted);
  free(pszQuoted);
  for (i=iArg0+1; i<argc; i++) {
    pszQuoted = CondQuoteShellArg(argv[i]);
    if (!pszQuoted) {
      free(pszCmdLine);
      return NULL;
    }
    n += sprintf(pszCmdLine+n, " %s", pszQuoted);
    free(pszQuoted);
  }
#endif /* All non-Microsoft OSs, like Unix */
  }

  return pszCmdLine;
}
