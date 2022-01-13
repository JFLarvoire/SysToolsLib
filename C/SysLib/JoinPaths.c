/*****************************************************************************\
*                                                                             *
*   File name	    JoinPaths.c						      *
*                                                                             *
*   Description	    Join two paths together				      *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-12-15 JFL Created this file.					      *
*                                                                             *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS	/* Prevent MSVC warnings about unsecure C library functions */

#include <string.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

#include "pathnames.h"	/* Public definitions for this module */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    NewJoinedPath					      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    Pointer to the new pathname, or NULL if allocation failed.|
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created the NewPathName routine			      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
|    2019-02-06 JFL Added call to new routine TrimDot, removing all ./ parts. |
|    2021-12-15 JFL Moved it to SysLib, and renamed it as NewTrimJoinedPath.  |
|		    Split NewJoinedPath off of it.			      |
*									      *
\*---------------------------------------------------------------------------*/

void TrimDotParts(char *path) { /* Remote ./ parts in the path */
  char *pIn = path;
  char *pOut = path;
  char c;
  int first = TRUE;
  for ( ; (c = *(pIn++)) != '\0'; ) {
    if (first && (c == '.') && (*pIn == DIRSEPARATOR_CHAR)) {
      pIn += 1; /* Eat up the / and continue */
      continue;
    }
    *(pOut++) = c;
    first = (   (c == DIRSEPARATOR_CHAR)
#if OS_HAS_DRIVES
	     || ((pIn == (path+2)) && (c == ':'))
#endif
            );
  }
  *(pOut++) = c;
}

char *NewJoinedPath(const char *pszPart1, const char *pszPart2) {
  size_t lPath = pszPart1 ? strlen(pszPart1) : 0;
  size_t lName = strlen(pszPart2);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, pszPart1);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, pszPart2);
  return buf;
}

char *NewCompactJoinedPath(const char *pszPart1, const char *pszPart2) {
  char *pszPath = NewJoinedPath(pszPart1, pszPart2);
  if (pszPath) TrimDotParts(pszPath);
  return pszPath;
}
