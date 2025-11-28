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
*    2025-11-27 JFL Rewrote TrimDotParts() as a more general NormalizePath(). *
*                                                                             *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS	/* Prevent MSVC warnings about unsecure C library functions */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */

/* SysLib include files */
#include "pathnames.h"	/* Public definitions for this module */

#define FALSE 0
#define TRUE 1

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    NormalizePath					      |
|									      |
|   Description     Remove unnecessary "/", "." and ".." parts in the path    |
|									      |
|   Parameters      char path		The path name to operate on           |
|		    							      |
|   Returns	    0=Success, else error in errno			      |
|		    							      |
|   Notes	    The pathname is changed in place.			      |
|		    In case of error, it might be left in a truncated state.  |
|		    							      |
|		    In DOS/Windows, also changes / to \.		      |
|		    							      |
|   History								      |
|    2021-12-15 JFL Split TrimDotParts() off of NewTrimJoinedPath().	      |
|    2025-11-15 JFL Fixed a bug when the os has drives.			      |
|    2025-11-27 JFL Rewrote TrimDotParts() as a more general NormalizePath(). |
*									      *
\*---------------------------------------------------------------------------*/

typedef struct _pathNode {
  char *pszName;
  int iLen;
} pathNode;

#ifndef streq
#define streq(s1, s2) (!strcmp(s1, s2))
#endif

#if 0 /* Change to 1 to generate debug output in the debug version */
#define DPRINTF DEBUG_PRINTF
#else
#define DPRINTF(args)
#endif

int NormalizePath(char *path) { /* Remove //, . and .. parts in the path */
  char *pIn;
  char *pOut;
  int isAbsolute;	/* TRUE for absolute paths */
  int iPrevSep, iSep;	/* TRUE if the previous or current char is a path separator */
  pathNode *pList = NULL;	/* Build-up a list of pathNode structures */
  int iListSize = 0;		/* Grow the list dynamically */
  int nNodes = 0;		/* Number of nodes used in the list */
  char *pszName = NULL;		/* The beginning of the current node name */
  int i, j;
#if HAS_DRIVES
  char hasDrive = FALSE;
#endif

  if (!path) return (errno = EINVAL);
#if HAS_DRIVES /* If part1 begins with a drive letter, preserve it */
  if (path[0] && (path[1] == ':')) {
    hasDrive = TRUE;
    path += 2;
  }
#endif
  if (!path[0]) return 0;	/* Empty string, nothing to do */

  isAbsolute = (path[0] == DIRSEPARATOR_CHAR);

  /* Parse all nodes, skipping the . nodes */
  for (pIn = path, iPrevSep = TRUE; ; pIn++, iPrevSep = iSep) {
    char c = *pIn;
    iSep = (   (c == DIRSEPARATOR_CHAR)
#if DIRSEPARATOR_CHAR != '/'
	    || (c == '/') /* In DOS/Windows, / is not normal, but acceptable on input */
#endif
            || (c == '\0'));
    if (iSep != iPrevSep) {
      if (!iSep) {	/* We're beginning a node */
      	DPRINTF(("[%d] Begin node\n", (int)(pIn-path)));
	pszName = pIn;
      } else {		/* We're ending a node */
	*pIn = '\0';
      	DPRINTF(("[%d] End node %s\n", (int)(pIn-path), pszName));
	if (!streq(pszName, ".")) { /* Ignore all . components */
	  if (nNodes == iListSize) {
	    pList = realloc(pList, (iListSize += 10) * (sizeof(pathNode)));
	    if (!pList) return errno;
	  }
	  pList[nNodes].pszName = pszName;
	  pList[nNodes].iLen = (int)(pIn - pszName);
	  nNodes += 1;
	}
      }
    }
    if (!c) break;
  }

  /* Remove all NAME/.. sequences */
  for (i=1; i<nNodes; i++) {
    if (streq(pList[i].pszName, "..") && !streq(pList[i-1].pszName, "..")) {
      DPRINTF(("Removing pair %s/%s\n", pList[i-1].pszName, pList[i].pszName));
      for (j=i+1; j<nNodes; j++) pList[j-2] = pList[j]; /* Shift the end of the list */
      nNodes -= 2;
      i -= 1; /* Repeat at the same offset in the list */
    }
  }

  /* For absolute paths, remove all .. nodes ahead of the list */
  if (isAbsolute) {
    for (i=0; i<nNodes; i++) if (!streq(pList[i].pszName, "..")) break;
    if (i) {
      DPRINTF(("Removing %d .. nodes ahead of the list\n", i));
      for (j=i; j<nNodes; j++) pList[j-i] = pList[j];
      nNodes -= i;
    }
  }

  /* Rebuild the normalized path */
  pOut = path;
  if (isAbsolute) *(pOut++) = DIRSEPARATOR_CHAR;
  for (i=0; i<nNodes; i++) {
    if (i) *(pOut++) = DIRSEPARATOR_CHAR;
    pszName = pList[i].pszName;
    if (pszName > pOut) strcpy(pOut, pszName); /* Move it only if needed */
    pOut += pList[i].iLen;
  }
  if ((!isAbsolute) && !nNodes) {
#if HAS_DRIVES
    if (!hasDrive) /* The . is not necessary if the drive letter is present */
#endif
    *(pOut++) = '.'; /* An empty relative path actually refers to . */
  }
  *pOut = '\0';

  free(pList);
  return 0;
}

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
|    2025-11-18 JFL Allow the second part to be NULL.			      |
|		    Allow the second part to contain a sub-path.	      |
*									      *
\*---------------------------------------------------------------------------*/

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
  if (pszPath) {
    int iErr = NormalizePath(pszPath);
    if (iErr) {
      free(pszPath);
      pszPath = NULL;
    }
  }
  return pszPath;
}
