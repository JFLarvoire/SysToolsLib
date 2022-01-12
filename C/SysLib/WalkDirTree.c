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

/* Flag OSs that have links (For some OSs which don't, macros are defined, but S_ISLNK always returns 0) */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK)
  #define OS_HAS_LINKS 1
#else
  #define OS_HAS_LINKS 0
#endif

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
|		    wdt_opts *pOpts	Options. Must be cleared before use.  |
|		    pWalkDirTreeCB	Callback called for every dir entry   |
|		    void *pRef		Passed to the callback		      |
|		    							      |
|   Returns	    0=Walk complete; 1=Callback said to stop; -1=Error found  |
|									      |
|   Notes	    Avoid recursing into looping links.			      |
|		    Report looping links exactly once.			      |
|		    							      |
|		    Error = It's not possible to do what was requested	      |
|		    Warning = Something wrong, but not blocking, was detected |
|		    Notice = Something was done that might need explaining    |
|		    							      |
|		    pOpts->WDT_CONTINUE = Treat recoverable errors as warnings|
|		    pOpts->WDT_QUIET = Don't report warnings & infos	      |
|		    							      |
|   History								      |
|    2021-12-14 JFL Created this routine.				      |
|    2021-12-22 JFL Detect link loops, and avoid entering them.		      |
|    2021-12-26 JFL Detect dangling links.				      |
|    2022-01-08 JFL Detect duplicate pathnames to folders visited before.     |
|    2022-01-10 JFL Optionally detect alias names for folders visited before. |
|    2022-01-11 JFL More consistent error handling & better statistics.       |
*									      *
\*---------------------------------------------------------------------------*/

/* Record all previously visited directories. Useful to avoid reporting files twice, when links point to directories */
/* Note: Initially implemented as a linked list, but too slow when used on a whole hard disk. (O(N²))
         With ~3 million files in ~300.000 directories, the linked list version took 16 minutes,
         whereas the tree version took 3 minutes, and the dictionary version now takes 2.5 minutes. (Both O(N.log(N)) */
/* TODO: It would be even better and faster to use a hash table, but I don't have one yet in my C library */
#if OS_HAS_LINKS
#include "dict.h"
DICT_DEFINE_PROCS();
#endif /* OS_HAS_LINKS */

/* Linked list of parent directories. Useful to detect back links */
typedef struct _NAMELIST {
  struct _NAMELIST *prev;
  const char *path;
} NAMELIST;

/* Internal subroutine, used to avoid infinite loops on link back loops */
static int WalkDirTree1(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef, NAMELIST *prev, int iDepth) {
  char *pPath;
  char *pPathname = NULL;
  int iRet = 0;
  DIR *pDir = NULL;
  struct dirent *pDE;
#if OS_HAS_LINKS
  char *pRootBuf = NULL;
  char *pTrueName = NULL;
  dict_t *dict = NULL;
  BOOL bCreatedDict = FALSE;
#endif /* OS_HAS_LINKS */
  NAMELIST root = {0};
  NAMELIST list = {0};

  DEBUG_ENTER(("WalkDirTree(\"%s\", ...);\n", path));

  if ((!path) || !strlen(path)) RETURN_INT_COMMENT(-1, ("path is empty\n"));

  pDir = opendirx(path);
  if (!pDir) {
    if (errno == EACCES) goto access_denied;
    goto fail_entry;
  }
  
  pOpts->nDir += 1;	/* One more directory scanned */

  if (!prev) { /* Record the true name of the directory tree root to search from */
#if OS_HAS_LINKS
    pRootBuf = malloc(PATH_MAX);
    if (!pRootBuf) goto out_of_memory;

    iRet = MlxResolveLinks(path, pRootBuf, PATH_MAX);
    if (iRet) {
      if ((errno == EBADF) || (errno == EINVAL)) { /* Unsupported link type, ex: Windows Container Isolation filter */
      	iRet = 0; /* opendir() succeeded, so as far as we're concerned, this is a directory */
      	strcpy(pRootBuf, path); /* opendir() succeeded, so the name fits in PATHMAX bytes */
      } else {
	goto fail_entry;
      }
    }
    pRootBuf = SafeRealloc(pRootBuf, lstrlen(pRootBuf)+1); /* Free the unused space */
    root.path = pRootBuf;
    
    if (pOpts->iFlags & WDT_ONCE) { /* Check if an alias has been visited before */
      pOpts->pOnce = dict = NewDict();
      if (!dict) goto out_of_memory;
      bCreatedDict = TRUE;
    }
#else /* !OS_HAS_LINKS */
    root.path = path;
#endif /* OS_HAS_LINKS */
    prev = &root;
  }

  pPath = path;
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */

  list.prev = prev;

  while ((pDE = readdirx(pDir)) != NULL) { /* readdirx() ensures d_type is set */
#if OS_HAS_LINKS
    int bIsDir;		 /* TRUE if this is a link pointing to a directory */
    char *pszBadLinkMsg; /* Flag bad links, pointing at a description of the problem */
#endif /* OS_HAS_LINKS */

    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (streq(pDE->d_name, ".")) continue;	/* Skip the . directory */
    if (streq(pDE->d_name, "..")) continue;	/* Skip the .. directory */

    pOpts->nFile += 1;	/* One more file scanned */

    pPathname = NewCompactJoinedPath(pPath, pDE->d_name);
    if (!pPathname) goto out_of_memory;
    list.path = pPathname;

#if OS_HAS_LINKS
    bIsDir = FALSE;
    pszBadLinkMsg = NULL;
    pTrueName = NULL;
    if (   ((pDE->d_type == DT_DIR) || (pDE->d_type == DT_LNK))
        && ((pOpts->iFlags & WDT_FOLLOW) || (pOpts->iFlags & WDT_ONCE))) {
      pTrueName = malloc(PATH_MAX);
      if (!pTrueName) goto out_of_memory;
      errno = 0;
      bIsDir = isEffectiveDir(pPathname);
      if (bIsDir && (MlxResolveLinks(pPathname, pTrueName, PATH_MAX) == 0)) {
	/* Resolution succeeded */
	pTrueName = SafeRealloc(pTrueName, lstrlen(pTrueName)+1); /* Free the unused space */
	if (pOpts->iFlags & WDT_FOLLOW) {
	  NAMELIST *pList;
	  list.path = pTrueName; /* Record this path for next time */
	  /* Check if we've seen this path before in the parent folders */
	  for (pList = prev; pList; pList = pList->prev) {
	    if (!lstrcmp(pTrueName, pList->path)) {
	      pszBadLinkMsg = "Link loops back";
	      break;
	    }
	  }
	}
      } else { /* pPathname is a symlink pointing to a file, or a link looping to itself */
      	if (errno) switch (errno) {
	case ELOOP:	/* There's a link looping to itself */
	  pszBadLinkMsg = "Link loops to itself"; break;
	case ENOENT:	/* There's a dangling link */
	  pszBadLinkMsg = "Dangling link"; break;
	case EBADF:	/* Unsupported link type, ex: Linux symlink */
	case EINVAL:	/* Unsupported reparse point type */
	  pszBadLinkMsg = "Unsupported link type"; break;
	default: /* There's a real error we can't handle here */
	  pferror("Can't resolve \"%s\": %s", pPathname, strerror(errno));
	  goto silent_fail; /* Abort the search */
	}
      }
    }
#endif /* OS_HAS_LINKS */

    /* Report the valid directory entry to the callback */
    iRet = pWalkDirTreeCB(pPathname, pDE, pRef);
    if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */

    switch (pDE->d_type) {
#if OS_HAS_LINKS
      case DT_LNK:
	if (pszBadLinkMsg) {
	  if (pOpts->iFlags & WDT_FOLLOW) { /* When following links, it's an error */
	    if (!((pOpts->iFlags & WDT_CONTINUE) && (pOpts->iFlags & WDT_QUIET))) {
	      pferror("%s: \"%s\"", pszBadLinkMsg, pPathname);
	    }
	    if (!(pOpts->iFlags & WDT_CONTINUE)) goto silent_fail;
	    pOpts->nErr += 1; /* Else count the error, and keep searching */
	  } else { /* When not following links, it's just a warning */
	    if (!(pOpts->iFlags & WDT_QUIET)) {
	      fprintf(stderr, "Warning: %s: \"%s\"\n", pszBadLinkMsg, pPathname);
	    }
	  }
	  break; /* Don't follow the bad or looping link, and keep searching */
	}
	if (!bIsDir) break; /* This is not a link to a subdirectory */
	if (!(pOpts->iFlags & WDT_FOLLOW)) break;
	/* Fallthrough into the directory case */
#endif /* OS_HAS_LINKS */
      case DT_DIR:
#if OS_HAS_LINKS
	/* Check if we've seen this path before anywhere else */
	if (pOpts->iFlags & WDT_ONCE) { /* Check if an alias has been visited before */
	  char *pszPrevious;
	  dict = pOpts->pOnce;
	  pszPrevious = DictValue(dict, pTrueName);
	  if (pszPrevious) { /* The same directory has been visited before under another alias name */
	    if (!(pOpts->iFlags & WDT_QUIET)) {
	      fprintf(stderr, "Notice: Already visited \"%s\" as \"%s\"\n", pPathname, pszPrevious);
	    }
	    break;
	  } else { /* OK, we've not visited this directory before. Record its name in the dictionary */
	    char *pszDup = strdup(pPathname);
	    if (!pszDup) goto out_of_memory;
	    NewDictValue(dict, pTrueName, pszDup);
	  }
	}
#endif /* OS_HAS_LINKS */
      	if (!list.path) list.path = pPathname;
      	if (!(pOpts->iFlags & WDT_NORECURSE)) {
	  if (!list.path) list.path = pPathname;
      	  iRet = WalkDirTree1(pPathname, pOpts, pWalkDirTreeCB, pRef, &list, iDepth+1);
      	}
      	break;
      default:
      	break;
    }
    if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */
    /* Free the buffers that will be reallocated during the next loop */
    free(pPathname);
    pPathname = NULL;
#if OS_HAS_LINKS
    free(pTrueName);
    pTrueName = NULL;
#endif /* OS_HAS_LINKS */
  }
  if (iRet) goto cleanup_and_return;;	/* -1 = Error, abort; 1 = Success, stop */
  /* Else it's readdir() that failed. Check errno to see the reason */
  if (errno == ENOENT) goto cleanup_and_return;	/* There are no more files */
  if (errno == EACCES) {
access_denied:
    if (!((pOpts->iFlags & WDT_CONTINUE) && (pOpts->iFlags & WDT_QUIET))) {
      pferror("Can't enter \"%s\": %s", path, strerror(errno));
    }
    if (pOpts->iFlags & WDT_CONTINUE) goto count_err_and_return;
    goto silent_fail;
  }
  goto fail_entry; /* Anything else is an unexpected error we can't handle */

out_of_memory:
  pferror("Out of memory");
  goto silent_fail;

fail_entry:
  pferror("Can't enter \"%s\": %s", path, strerror(errno));
silent_fail:		/* The error message has already been displayed */
  iRet = -1;
count_err_and_return:	/* Count an error, cleanup and return */
  pOpts->nErr += 1;
cleanup_and_return:
  if (pDir) closedirx(pDir);
  free(pPathname);
#if OS_HAS_LINKS
  free(pRootBuf);
  free(pTrueName);
  if (bCreatedDict) { /* We're the first folder that created the visited dictionary. Delete it before returning. */
    dictnode *pNode;
    while ((pNode = FirstDictValue(dict)) != NULL) {
      DeleteDictValue(dict, pNode->pszKey, free);
    }
    free(dict);
    pOpts->pOnce = NULL;
  }
#endif /* OS_HAS_LINKS */
  RETURN_INT_COMMENT(iRet, ((iRet == -1) ? "Error, stop walk\n" : (iRet ? "Success, stop Walk\n" : "Success, continue walk\n")));
}

/* Public routine. Do not instrument with debug macros, to avoid call depth alignment issues. */
int WalkDirTree(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef) {
  return WalkDirTree1(path, pOpts, pWalkDirTreeCB, pRef, NULL, 0);
}
