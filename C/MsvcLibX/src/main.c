/*****************************************************************************\
*                                                                             *
*   Filename	    main.c						      *
*									      *
*   Description:    Main routine for WIN32 UTF-8 programs		      *
*                                                                             *
*   Notes:	    							      *
*		    							      *
*   History:								      *
*    2014-03-03 JFL Created this module.				      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _UTF8_SOURCE

#include <stdio.h>
#include "msvclibx.h"

#ifdef _WIN32

#include <windows.h>
#include <iconv.h>	/* For MsvcLibX' codePage global variable */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        BreakArgLine                                              |
|                                                                             |
|   Description     Break the Windows command line into standard C arguments  |
|                                                                             |
|   Parameters      LPSTR pszCmdLine    NUL-terminated argument line          |
|                   char *pszArg[]      Array of arguments pointers           |
|                                                                             |
|   Returns         int argc            Number of arguments found             |
|                                                                             |
|   Notes           MSVC library startup \" parsing rule is:                  |
|                   2N backslashes + " ==> N backslashes and begin/end quote  |
|                   2N+1 backslashes + " ==> N backslashes + literal "	      |
|                   N backslashes ==> N backslashes                           |
|                                                                             |
|   History 								      |
|    1993-10-05 JFL Initial implementation within devmain().		      |
|    1994-04-14 JFL Extracted from devmain, and created this routine.	      |
|    1995-04-07 JFL Extracted from llkinit.c.				      |
|    1996-09-26 JFL Adapted to Win32 programs.				      |
|    1996-12-11 JFL Use Windows string routines.			      |
|    2001-09-18 JFL Set argv[0] with actual module file name.		      |
|                   Manage quoted strings as a single argument.               |
|    2001-09-25 JFL Only process \x inside strings.                           |
|    2014-03-04 JFL Removed the C-style \-quoting of characters, which was    |
|                   convenient, but incompatible with MSVC argument parsing.  |
|                   Removed the limitation on the # of arguments.             |
|                   Made the code compatible with ANSI and UTF-8 encodings.   |
*                                                                             *
\*---------------------------------------------------------------------------*/

int BreakArgLine(LPSTR pszCmdLine, char ***pppszArg) {
  int i, j;
  int argc = 0;
  char c, c0;
  char *pszCopy;
  int iString = FALSE;
  int nBackslash = 0;
  char **ppszArg;

  ppszArg = (char **)malloc((argc+1)*sizeof(char *));

  /* Make a local copy of the argument line */
  /* Break down the local copy into standard C arguments */

  pszCopy = malloc(lstrlen(pszCmdLine) + 1);
  /* Copy the string, managing quoted characters */
  for (i=0, j=0, c0='\0'; ; i++) {
    c = pszCmdLine[i];
    if (!c) {		    /* End of argument line */
      for ( ; nBackslash; nBackslash--) pszCopy[j++] = '\\'; /* Output pending \s */
      pszCopy[j++] = c;
      break;
    }
    if (c == '\\') {	    /* Escaped character in string (maybe) */
      nBackslash += 1; 
      if (!c0) {  /* Beginning of a new argument */
	ppszArg[argc++] = pszCopy+j;
	ppszArg = (char **)realloc(ppszArg, (argc+1)*sizeof(char *));
	c0 = c;
      }
      continue;
    }
    if (c == '"') {
      if (nBackslash & 1) { /* Output N/2 \ and a literal " */
      	for (nBackslash >>= 1; nBackslash; nBackslash--) pszCopy[j++] = '\\';
	pszCopy[j++] = c0 = c;
	continue;
      }
      if (nBackslash) {	    /* Output N/2 \ and switch string mode */
      	for (nBackslash >>= 1; nBackslash; nBackslash--) pszCopy[j++] = '\\';
      }
      iString = !iString;
      continue;
    }
    for ( ; nBackslash; nBackslash--) pszCopy[j++] = '\\'; /* Output pending \s */
    if ((!iString) && ((c == ' ') || (c == '\t'))) c = '\0';
    if (c && !c0) {  /* Beginning of a new argument */
      ppszArg[argc++] = pszCopy+j;
      ppszArg = (char **)realloc(ppszArg, (argc+1)*sizeof(char *));
    }
    pszCopy[j++] = c0 = c;
  }

  ppszArg[argc] = NULL;
  *pppszArg = ppszArg;

  return argc;
}

#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */

int _mainU0(void) {
  LPWSTR lpwCommandLine;
  char *pszCommandLine;
  int n;
  char **argv;
  int argc;
  WCHAR wc;

  /* Get the command line */  
  lpwCommandLine = GetCommandLineW();
  /* Trim tail spaces */
  n = lstrlenW(lpwCommandLine);
  while (n && (wc = lpwCommandLine[n-1]) && ((wc == L' ') || (wc == L'\t'))) lpwCommandLine[--n] = L'\0';
  /* Allocate space for the UTF8 copy */
  n += 1;	/* Count the final NUL */
  pszCommandLine = malloc(4 * n); /* Worst case */ /* Assume this can't fail at this stage */
  /* Convert the Unicode command line to UTF-8 */
  n = WideCharToMultiByte(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpwCommandLine,	/* lpWideCharStr, */
			  n,			/* cchWideChar, */
			  pszCommandLine,	/* lpMultiByteStr, */
			  (4 * n),		/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  if (!n) {
#undef fprintf /* Use the real fprintf, to avoid further conversion errors! */
    fprintf(stderr, "Warning: Can't convert the argument line to UTF-8\n");
    pszCommandLine[0] = '\0';
  }
  realloc(pszCommandLine, n+1); /* Resize the memory block to fit the UTF-8 line */

  argc = BreakArgLine(pszCommandLine, &argv);

  /* Record the console code page, to allow converting the output accordingly */
  codePage = GetConsoleOutputCP();

  return _mainU(argc, argv);
}

#pragma warning(default:4706)

#endif /* defined(_WIN32) */

