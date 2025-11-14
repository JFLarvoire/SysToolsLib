/*****************************************************************************\
*                                                                             *
*   File name	    WalkDirTree.c					      *
*                                                                             *
*   Description	    Call a function for every dir entry in a directory tree   *
*                                                                             *
*   Notes	    TODO: Implement Unix & MacOS version of WalkDirTree().    *
*                                                                             *
*   History                                                                   *
*    2021-11-27 JFL Created this file.					      *
*    2022-10-16 JFL Avoid errors in MacOS.				      *
*    2024-06-21 JFL Added support for detecting already visited paths in Unix.*
*    2025-08-15 JFL Declare the dict. data destructor when creating the dict. *
*    2025-11-10 JFL Added the ability to limit the recursion depth.           *
*                                                                             *
\*****************************************************************************/

#define _UTF8_LIB_SOURCE
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#define _CRT_SECURE_NO_WARNINGS /* Prevent MSVC warnings about unsecure C library functions */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

/* SysToolsLib include files */
#include "debugm.h"		/* SysToolsLib debugging macros */

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
|    2020-03-16 JFL Use stat instead of lstat, it's faster and simpler!       |
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
|   Function	    GetRealName						      |
|									      |
|   Description     Generate a name that uniquely identifies the real file    |
|									      |
|   Parameters      const char *path		A file pathname		      |
|		    							      |
|   Returns	    NULL if a buffer cannot be allocated.		      |
|		    "" if an error occurred while resolving links. See errno. |
|		    The true pathname (Windows) or a unique string (Unix).    |
|		    							      |
|   Notes	    Resolves links to see what they point to		      |
|		    							      |
|   History								      |
|    2024-06-21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

/* Check the list of supported OS/Compiler pairs */
#if defined(_MSDOS) || defined(_WIN32)

#if defined(_MSC_VER)
#if !HAS_MSVCLIBX
#error "This code requires Standard C library routines missing from Microsoft's C library. Install MsvcLibX to get them."
#endif /* !HAS_MSVCLIBX */
#else /* !defined(_MSC_VER) */
#error "Links resolution only implemented for this OS using MSVC + MsvcLibX library"
#endif /* defined(_MSC_VER) */

#elif defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX

#define KEY_SIZE (2*(sizeof(dev_t) + sizeof(ino_t)))
#define HAS_DEV_AND_FILE_ID 1 /* If the OS doesn't have them, there will be an error when using KEY_SIZE */

#else /* None of MS-DOS, Windows, Unix, Mach */

#if OS_HAS_LINKS
#error "Links resolution not implemented for this OS"
#endif /* OS_HAS_LINKS */

#endif /* defined(_MSDOS) || defined(_WIN32) */

#if OS_HAS_LINKS

#if HAS_MSVCLIBX
char *GetRealName(char *pathname) {
  int iRet;
  char *pTrueName = malloc(PATH_MAX);
  if (!pTrueName) return NULL;
  pTrueName[0] = '\0';
  iRet = MlxResolveLinks(pathname, pTrueName, PATH_MAX); /* MsvcLibX resolves all links */
  if (iRet) return pTrueName; /* if GetRealName()=="", check errno */
  pTrueName = ShrinkBuf(pTrueName, lstrlen(pTrueName)+1); /* Free the unused space */
  return pTrueName;
}
#elif HAS_DEV_AND_FILE_ID
/* Convert a devID/fileID pair into a new hexadecimal string */
char *MakeDevIdKey(char *pszKey, dev_t devID, ino_t fileID) {
  int i, o=0;
  if (!pszKey) pszKey = malloc(KEY_SIZE + 1); /* If not provided, allocate a buffer */
  if (!pszKey) return NULL;
  for (i=sizeof(dev_t)-1; i>=0; i--) {
    unsigned char c = ((unsigned char *)&devID)[i];
    char hex[] = "0123456789ABCDEF";
    pszKey[o++] = hex[c>>4];
    pszKey[o++] = hex[c&0x0F];
  }
  for (i=sizeof(ino_t)-1; i>=0; i--) {
    unsigned char c = ((unsigned char *)&fileID)[i];
    char hex[] = "0123456789ABCDEF";
    pszKey[o++] = hex[c>>4];
    pszKey[o++] = hex[c&0x0F];
  }
  pszKey[o] = '\0';
  return pszKey;
}
char *GetRealName(char *pathname) {
  struct stat st;
  int iErr;
  char *pszKey = malloc(KEY_SIZE + 1);
  if (!pszKey) return NULL;
  pszKey[0] = '\0';
  iErr = stat(pathname, &st); /* Let the OS resolve all links */
  if (iErr) return pszKey; /* if GetRealName()=="", check errno */
  MakeDevIdKey(pszKey, st.st_dev, st.st_ino);
  return pszKey;
}
#endif /* HAS_DEV_AND_FILE_ID */

#endif /* OS_HAS_LINKS */

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
|    2024-06-21 JFL Added support for detecting already visited paths in Unix.|
*									      *
\*---------------------------------------------------------------------------*/

/* Record all previously visited directories. Useful to avoid reporting files twice, when links point to directories */
/* Note: Initially implemented as a linked list, but too slow when used on a whole hard disk. (O(N²))
         With ~3 million files in ~300.000 directories, the linked list version took 16 minutes,
         whereas the tree version took 3 minutes, and the dictionary version now takes 2.5 minutes. (Both O(N.log(N)) */
/* TODO: It would be even better and faster to use a hash table, but I don't have one yet in my C library */
#if OS_HAS_LINKS
#include "dict.h"
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
  int bCreatedDict = FALSE;
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
    pRootBuf = GetRealName(path);
    if (!pRootBuf) goto out_of_memory;
    if (!pRootBuf[0]) {
#if HAS_MSVCLIBX
      if ((errno == EBADF) || (errno == EINVAL)) { /* Unsupported link type, ex: Windows Container Isolation filter */
      	iRet = 0; /* opendir() succeeded, so as far as we're concerned, this is a directory */
      	strcpy(pRootBuf, path); /* opendir() succeeded, so the name fits in PATHMAX bytes */
      } else {
	goto fail_entry;
      }
#elif HAS_DEV_AND_FILE_ID
      goto fail_entry; /* Unlikely to happen, since opendir() did work */
#endif
    }
    root.path = pRootBuf;

    if (pOpts->iFlags & WDT_ONCE) { /* Check if an alias has been visited before */
      pOpts->pOnce = dict = NewDict(free); /* Values must be freed when nodes are deleted */
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
      errno = 0;
      bIsDir = isEffectiveDir(pPathname); /* In Windows+MsvcLibX, this may fail, despite d_type == DT_DIR above */
      if (bIsDir && ((pTrueName = GetRealName(pPathname)) != NULL) && pTrueName[0]) {
	if (pOpts->iFlags & WDT_FOLLOW) {
	  NAMELIST *pList;
	  list.path = pTrueName; /* Record this path for next time */
	  /* Check if we've seen this path before in the parent folders */
	  for (pList = prev; pList; pList = pList->prev) {
	    if (!strcmp(pTrueName, pList->path)) {
	      pszBadLinkMsg = "Link loops back";
	      break;
	    }
	  }
	}
      } else { /* pPathname is a symlink pointing to a file, or a link looping to itself */
	if (bIsDir && !pTrueName) goto out_of_memory;
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
	if (!(pOpts->iFlags & WDT_NORECURSE)) {
	  if ((!pOpts->iMaxDepth) || (iDepth < pOpts->iMaxDepth)) {
	    if (!list.path) list.path = pPathname;
	    iRet = WalkDirTree1(pPathname, pOpts, pWalkDirTreeCB, pRef, &list, iDepth+1);
	  }
	}
	DEBUG_PRINTF(("// End of processing the dir \"%s\"\n", pPathname));
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
  if (errno == 0) goto cleanup_and_return;	/* There are no more files (Unix) */
  /* Else it's readdir() that failed. Check errno to see the reason */
  if (errno == ENOENT) goto cleanup_and_return;	/* There are no more files (MsvcLibX) */
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
      DeleteDictValue(dict, pNode->pszKey);
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
