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
*    2025-11-15 JFL Generalized NewJoinedPath() to allow joining two paths, as*
*		    if two successive CDs were made, each part being optional.*
*                   Fixed a bug in TrimDotParts() for OSs that have drives.   *
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
|   Description     Join two pathname parts into a new combined pathname      |
|									      |
|   Parameters      const char *part1		The directory name, or NULL   |
|		    const char *part2		The sub-path name, or NULL    |
|		    							      |
|   Returns	    Pointer to the new pathname, or NULL if allocation failed.|
|		    							      |
|   Notes	    Typically used to join a directory pathname and file name.|
|		    When the second name part contains a path, the two paths  |
|		    are joined as if two consecutive chdir() calls were made. |
|		    So if the second path is an absolute path, the first path |
|		    is discarded.					      |
|		    In DOS/Windows, the first part may begin with an X: drive |
|		    letter, but the second part may not.		      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created the NewPathName routine			      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
|    2019-02-06 JFL Added call to new routine TrimDot, removing all ./ parts. |
|    2021-12-15 JFL Moved it to SysLib, and renamed it as NewTrimJoinedPath.  |
|		    Split NewJoinedPath off of it.			      |
|    2025-11-15 JFL Fixed a bug in TrimDotParts() when the os has drives.     |
|    2025-11-15 JFL Allow the second part to be NULL.			      |
|		    Allow the second part to contain a sub-path.	      |
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
#if HAS_DRIVES
	     || ((pIn == (path+2)) && (c == ':'))
#endif
            );
  }
  *(pOut++) = c;
}

char *NewJoinedPath(const char *pszPart1, const char *pszPart2) {
  size_t lPart1 = pszPart1 ? strlen(pszPart1) : 0;
  size_t lPart2 = pszPart2 ? strlen(pszPart2) : 0;
  char *buf = malloc(lPart1 + lPart2 + 2); /* +2 for the middle / and final NUL */
  if (!buf) return NULL;		/* Out of memory */
  if (!(lPart1 || lPart2)) return NULL;	/* At least one part must be defined */
  if (!lPart1) { /* Implies lPart2 > 0 => pszPart2 is not NULL */
    strcpy(buf, pszPart2);
    return buf;
  }
  /* lPart1 > 0 => pszPart1 is not NULL, and pszPart1[0] is not NUL */
  strcpy(buf, pszPart1);
  if (!lPart2) return buf;
  /* lPart2 > 0 => pszPart2 is not NULL, and pszPart2[0] is not NUL */
  if (pszPart2[0] == DIRSEPARATOR_CHAR) { /* Part 2 is an absolute pathname */
    char *pRoot = buf;
#if HAS_DRIVES /* If part1 begins with a drive letter, preserve it */
    if (pszPart1[1] == ':') pRoot += 2;
#endif
    strcpy(pRoot, pszPart2);
    return buf;
  }
  /* Part 2 is a relative pathname. Append it, after a / or \ if needed */
  if ((buf[lPart1-1] != DIRSEPARATOR_CHAR)) buf [lPart1++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPart1, pszPart2);
  return buf;
}

char *NewCompactJoinedPath(const char *pszPart1, const char *pszPart2) {
  char *pszPath = NewJoinedPath(pszPart1, pszPart2);
  if (pszPath) TrimDotParts(pszPath);
  return pszPath;
}
