/*****************************************************************************\
*                                                                             *
*   Filename	    fullpath.c						      *
*									      *
*   Description     Get the absolute pathname for a relative UTF-8 path       *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History								      *
*    2016-09-13 JFL Created this module with routine from truename.c.         *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ security warnings */

#define _UTF8_SOURCE /* Generate the UTF-8 version of routines */

#include <stdlib.h>
#include <limits.h>
#include <malloc.h>

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

#include <windows.h>		/* Also includes MsvcLibX' WIN32 UTF-8 extensions */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fullpathU						      |
|                                                                             |
|   Description:    Get the absolute pathname for a relative UTF-8 path       |
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
  char *absPath0 = absPath;
  DWORD n;
  if (!absPath) {
    absPath = malloc(UTF8_PATH_MAX); /* Worst case for UTF-8 is 4 bytes/Unicode character */
    if (!absPath) return NULL;
  }
  n = GetFullPathNameU(relPath, (DWORD)maxLength, absPath, NULL);
  if (!n) {
    errno = Win32ErrorToErrno();
    if (!absPath0) free(absPath);
    return NULL;
  }
  if (!absPath0) absPath = realloc(absPath, strlen(absPath) + 1);
  return absPath;
}

#endif /* defined(_WIN32) */

