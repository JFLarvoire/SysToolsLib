/*****************************************************************************\
*                                                                             *
*   File name	    WalkDirTree.c					      *
*                                                                             *
*   Description	    Call a function for every dir entry in a directory tree   *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-11-27 JFL Created this file.					      *
*                                                                             *
\*****************************************************************************/

#define _UTF8_LIB_SOURCE

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debugging macros */

/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
#include "pathnames.h"		/* Pathname management definitions and functions */
#include "mainutil.h"		/* Print errors, streq, etc */

/* Safe realloc() */
void *SafeRealloc(void *old, size_t new_size) {
  void *new = realloc(old, new_size); /* This may fail, even for a smaller size */
  return new ? new : old;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    isEffectiveDir					      |
|									      |
|   Description     Check if pathname refers to an existing directory	      |
|									      |
|   Parameters      const char *path		The directory name	      |
|		    							      |
|   Returns	    TRUE or FALSE					      |
|		    							      |
|   Notes	    Resolves links to see what they point to		      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2020-03-16 JFL Use stat instead of lstat, it faster and simpler!         |
*									      *
\*---------------------------------------------------------------------------*/

int isEffectiveDir(const char *pszPath) {
  struct stat sStat;
  int iErr = stat(pszPath, &sStat);
  if (iErr) return 0;
  return S_ISDIR(sStat.st_mode);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    WalkDirTree						      |
|									      |
|   Description     Invoke a callback for every file in a directory tree      |
|									      |
|   Parameters      char *path		The directory pathname		      |
|		    							      |
|   Returns	    0=Walk complete; 1=Callback said to stop; -1=Error found  |
|									      |
|   Notes	    Avoid recursing into looping links.			      |
|		    Report looping links exactly once.			      |
|		    							      |
|   History								      |
|    2021-12-14 JFL Created this routine.				      |
|    2021-12-22 JFL Detect link loops, and avoid entering them.		      |
|    2021-12-26 JFL Detect dangling links.				      |
*									      *
\*---------------------------------------------------------------------------*/

/* Linked list of previous pathnames */
typedef struct _NAMELIST {
  struct _NAMELIST *prev;
  const char *path;
} NAMELIST;

int WalkDirTree1(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef, NAMELIST *prev, int iDepth) {
  char *pPath;
  int iRet = 0;
  DIR *pDir;
  struct dirent *pDE;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
  char *pRootBuf = NULL;
  char *pBuf= NULL;
#endif
  NAMELIST root = {0};
  NAMELIST list = {0};

  DEBUG_ENTER(("WalkDirTree(\"%s\", ...);\n", path));

  if ((!path) || !strlen(path)) RETURN_INT_COMMENT(-1, ("path is empty\n"));

  pDir = opendirx(path);
  if (!pDir) {
    if (!(pOpts->iFlags & WDT_IGNOREERR)) {
      if (!(pOpts->iFlags & WDT_QUIET)) {
      	pferror("Can't access \"%s\": %s", path, strerror(errno));
      }
    }
    goto fail;
  }

  if (!prev) { /* Record the true name of the directory tree root to search from */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
    pRootBuf = malloc(PATH_MAX);
    if (!pRootBuf) goto out_of_memory;

    iRet = MlxResolveLinks(path, pRootBuf, PATH_MAX);
    if (iRet) goto fail;
    pRootBuf = SafeRealloc(pRootBuf, lstrlen(pRootBuf)+1); /* Free the unused space */
    root.path = pRootBuf;
#else
    root.path = path;
#endif
    prev = &root;
  }

  pPath = path;
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */

  list.prev = prev;

  while ((pDE = readdirx(pDir)) != NULL) { /* readdirx() ensures d_type is set */
    char *pPathname;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
    int bIsDir;		/* TRUE if this is a link pointing to a directory */
    char *pszBadLink;	/* Flag bad links, pointing at a description of the problem */
#endif

    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (streq(pDE->d_name, ".")) continue;	/* Skip the . directory */
    if (streq(pDE->d_name, "..")) continue;	/* Skip the .. directory */

    pPathname = NewCompactJoinedPath(pPath, pDE->d_name);
    if (!pPathname) goto out_of_memory;
    list.path = pPathname;

#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
    bIsDir = FALSE;
    pszBadLink = NULL;
    if (((pDE->d_type == DT_DIR) || (pDE->d_type == DT_LNK)) && (pOpts->iFlags & WDT_FOLLOW)) {
      /* Make sure that there's no loop, that would have caused it to be reported already */
      NAMELIST *pList;
      pBuf = malloc(PATH_MAX);
      if (!pBuf) goto out_of_memory;
      errno = 0;
      bIsDir = isEffectiveDir(pPathname);
      if (bIsDir && (MlxResolveLinks(pPathname, pBuf, PATH_MAX) == 0)) {
	/* Resolution succeeded */
	pBuf = SafeRealloc(pBuf, lstrlen(pBuf)+1); /* Free the unused space */
	list.path = pBuf; /* Record this path for next time */
	/* Check if we've seen this path before */
	for (pList = prev; pList; pList = pList->prev) {
	  if (!lstrcmp(pBuf, pList->path)) {
	    pszBadLink = "Link loops back";
	    break;
	  }
	}
      } else { /* pPathname is a symlink pointing to a file, or a link looping to itself */
	if (errno == ELOOP) { /* There's a link looping to itself */
	  pszBadLink = "Link loops to itself";
	} else if (errno == ENOENT) { /* There's a dangling link */
	  pszBadLink = "Dangling link";
	} else if (errno == EBADF) { /* Unsupported link type, ex: Linux symlink */
	  pszBadLink = "Unsupported link";
	} else if (errno) { /* There's a real error we can't handle here */
	  free(pPathname);
	  iRet = -1; /* Abort the search */
	  break;
	}
      }
    }
#endif

    /* Report the valid directory entry to the callback */
    iRet = pWalkDirTreeCB(pPathname, pDE, pRef);
    if (pOpts->pNProcessed && (iRet >= 0)) *(pOpts->pNProcessed) += 1; /* Number of files successfully processed */
    if (iRet == -1) pOpts->nErr += 1;
    if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */

    switch (pDE->d_type) {
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
      case DT_LNK:
	if (pszBadLink) {
	  if (!(pOpts->iFlags & WDT_QUIET)) {
	    pferror("%s: \"%s\"", pszBadLink, pPathname);
	  }
	  break; /* Don't follow the bad or looping link, and keep searching */
	}
	if (!bIsDir) break; /* This is not a link to a subdirectory */
	/* Fallthrough into the directory case */
#endif
      case DT_DIR:
      	if (!list.path) list.path = pPathname;
      	if (!(pOpts->iFlags & WDT_NORECURSE)) {
	  if (!list.path) list.path = pPathname;
      	  iRet = WalkDirTree1(pPathname, pOpts, pWalkDirTreeCB, pRef, &list, iDepth+1);
      	}
      	break;
      default:
      	break;
    }
    free(pPathname);
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
    free(pBuf);
    pBuf = NULL;
#endif
    if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */
  }
  closedirx(pDir);
  goto cleanup_and_return;

out_of_memory:
  pferror("Out of memory");
fail:
  pOpts->nErr += 1;
  iRet = -1;
  goto cleanup_and_return;

cleanup_and_return:
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
  free(pRootBuf);
  free(pBuf);
#endif
  RETURN_INT_COMMENT(iRet, ((iRet == -1) ? "Error, stop walk\n" : (iRet ? "Success, stop Walk\n" : "Success, continue walk\n")));
}

int WalkDirTree(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef) {
  return WalkDirTree1(path, pOpts, pWalkDirTreeCB, pRef, NULL, 0);
}
