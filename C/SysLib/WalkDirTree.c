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
*    2022-10-16 JFL Avoid errors in MacOS.				      *
*    2024-06-21 JFL Added support for detecting already visited paths in Unix.*
*    2025-08-15 JFL Declare the dict. data destructor when creating the dict. *
*    2025-11-10 JFL Added the ability to limit the recursion depth.           *
*    2025-11-15 JFL Optionally callback on directory entry and exit.	      *
*    2025-11-17 JFL Optionally callback on effective directories only.	      *
*                   Optionally change the current directory to the one scanned.
*    2025-11-19 JFL Fixed the ability to change drives.			      *
*    2025-11-25 JFL WalkDirTree() now uses DT_ENTER & DT_LEAVE to report      *
*		    directory entry and exit.				      *
*    2025-11-25 JFL Fixed a bug when using WDT_CD.			      *
*    2025-11-30 JFL Restructured the error management.			      *
*    2025-12-02 JFL Using SysLib's new chdir for Unix, with its own debug msgs.
*                                                                             *
\*****************************************************************************/

#define _UTF8_LIB_SOURCE
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#define _CRT_SECURE_NO_WARNINGS /* Prevent MSVC warnings about unsecure C library functions */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#ifdef _MSC_VER			/* DOS and Windows */
#include <direct.h>		/* For _getdrive() */
#endif

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
|   Function	    GetUniqueIdString					      |
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
|    2025-11-29 JFL Renamed from GetTrueName to GetUniqueIdString, because    |
|		    in Unix, the string returned is not a name at all.	      |
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
char *GetUniqueIdString(const char *pathname) {
  int iRet;
  char *pTrueName = malloc(PATH_MAX);
  if (!pTrueName) return NULL;
  pTrueName[0] = '\0';
  iRet = MlxResolveLinks(pathname, pTrueName, PATH_MAX); /* MsvcLibX resolves all links */
  if (iRet) return pTrueName; /* if GetUniqueIdString()=="", check errno */
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
char *GetUniqueIdString(const char *pathname) {
  struct stat st;
  int iErr;
  char *pszKey = malloc(KEY_SIZE + 1);
  if (!pszKey) return NULL;
  pszKey[0] = '\0';
  iErr = stat(pathname, &st); /* Let the OS resolve all links */
  if (iErr) return pszKey; /* if GetUniqueIdString()=="", check errno */
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

#if _DEBUG
char *DumpOpts(wdt_opts *po) {
  static char szBuf[16];
  int i = 0;
  szBuf[i++] = (char)(po->iFlags & WDT_CONTINUE ? 'C' : 'c');
  szBuf[i++] = (char)(po->iFlags & WDT_QUIET ? 'Q' : 'q');
  szBuf[i++] = (char)(po->iFlags & WDT_NORECURSE ? 'r' : 'R');
  i += sprintf(szBuf+i, "%d", po->iMaxDepth);	/* Maximum depth to search in the scan tree */
  szBuf[i++] = (char)(po->iFlags & WDT_FOLLOW ? 'F' : 'f');
  szBuf[i++] = (char)(po->iFlags & WDT_ONCE ? 'O' : 'o');
  szBuf[i++] = (char)(po->iFlags & WDT_CBINOUT ? 'X' : 'x');
  szBuf[i++] = (char)(po->iFlags & WDT_DIRONLY ? 'D' : 'd');
  szBuf[i++] = (char)(po->iFlags & WDT_CD ? 'V' : 'v');
  szBuf[i] = '\0';
  return szBuf;
}
#endif /* _DEBUG */


/* Internal subroutine, used to avoid infinite loops on link back loops */
static int WalkDirTree1(const char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef, NAMELIST *prev, int iDepth) {
  const char *path_to_read;	/* Same as the path argument, or . if the WDT_CD flag is used */
  char *pPathname = NULL;
  char *pPath0 = NULL;
#if HAS_DRIVES
  int iDrive0 = 0;
#endif
  int iErr;
  int iRet = 0;
  DIR *pDir = NULL;
  struct dirent *pDE;
#if OS_HAS_LINKS
  char *pRootBuf = NULL;
  char *pUniqueID = NULL;
  dict_t *dict = NULL;
  int bCreatedDict = FALSE;
#endif /* OS_HAS_LINKS */
  NAMELIST root = {0};
  NAMELIST list = {0};
  int iPrintErrors = !((pOpts->iFlags & WDT_CONTINUE) && (pOpts->iFlags & WDT_QUIET));
  char *pRelatName;
  struct dirent *pFakeInOutDE = NULL;
  char *pszFailingOpVerb;

  DEBUG_ENTER(("WalkDirTree(\"%s\", {%s}, ..., %d);\n", path, DumpOpts(pOpts), iDepth));

  if ((!path) || !path[0]) RETURN_INT_COMMENT(-1, ("path is empty\n"));

  if (pOpts->iFlags & WDT_CD) { /* Change CD to the directory to scan */
    const char *pNewCD;
    if (iDepth == 0) {		/* The first time only, record the return path */
      pPath0 = NEW_PATHNAME_BUF();
      if (!pPath0) goto out_of_memory;
#if HAS_DRIVES
      if (path[1] == ':') {
	int iDrive = path[0] - '@';
	if (path[0] >= 'a') iDrive -= 'a' - 'A';
	iDrive0 = _getdrive();
	iErr = _chdrive(iDrive);
	if (iErr) {
	  if (iPrintErrors) pferror("Can't change to drive %c: %s", iDrive + '@', strerror(errno));
	  goto unrecoverable_error; /* Unrecoverable because other branches of the same tree will be on the same inaccessible drive */
	}
      }
#endif
      if (!getcwd(pPath0, PATHNAME_BUF_SIZE)) {
	if (iPrintErrors) pferror("Can't get the current directory: %s", strerror(errno));
	goto unrecoverable_error;
      }
      TRIM_PATHNAME_BUF(pPath0);
      pNewCD = path;
    } else { /* We're recursing, descending a single level each time, at the end of the path */
      pNewCD = strrchr(path, DIRSEPARATOR_CHAR);
      pNewCD = pNewCD ? (pNewCD + 1) : path;
    }
    iErr = chdir(pNewCD);
    if (iErr) {
      pszFailingOpVerb = "enter";
      goto print_dir_op_error;
    }
    path_to_read = ".";
  } else {
    path_to_read = path;
  }

  pDir = opendirx(path_to_read);
  if (!pDir) {
    pszFailingOpVerb = "open";
    goto print_dir_op_error;
  }

  pOpts->nDir += 1;	/* One more directory scanned */

  if (!prev) { /* Record the true name of the directory tree root to search from */
#if OS_HAS_LINKS
    pRootBuf = GetUniqueIdString(path_to_read);
    if (!pRootBuf) goto out_of_memory;
    if (!pRootBuf[0]) {
      pszFailingOpVerb = "identify";
#if HAS_MSVCLIBX
      if ((errno == EBADF) || (errno == EINVAL)) { /* Unsupported link type, ex: Windows Container Isolation filter */
      	iRet = 0; /* opendir() succeeded, so as far as we're concerned, this is a directory */
      	free(pRootBuf);
      	pRootBuf = strdup(path);
      	if (!pRootBuf) goto out_of_memory;
      } else {
	goto print_dir_op_error;
      }
#elif HAS_DEV_AND_FILE_ID
      goto print_dir_op_error; /* Unlikely to happen, since opendir() did work */
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

  if (pOpts->iFlags & WDT_CBINOUT) {
    pFakeInOutDE = (struct dirent *)calloc(offsetof(struct dirent, d_name) + 2, 1);
    if (!pFakeInOutDE) goto out_of_memory;
    pFakeInOutDE->d_type = DT_ENTER;
    iRet = pWalkDirTreeCB(path, pFakeInOutDE, pRef); /* Notify the callback of the directory entry */
    if (iRet) goto cleanup_and_return;;	/* -1 = Error, abort; 1 = Success, stop */
  }

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

    pPathname = NewJoinedPath(path, pDE->d_name);
    if (!pPathname) goto out_of_memory;
    list.path = pPathname;
    pRelatName = (pOpts->iFlags & WDT_CD) ? pDE->d_name : pPathname;

#if OS_HAS_LINKS
    bIsDir = FALSE;
    pszBadLinkMsg = NULL;
    pUniqueID = NULL;
    if (   ((pDE->d_type == DT_DIR) || (pDE->d_type == DT_LNK))
        && ((pOpts->iFlags & WDT_FOLLOW) || (pOpts->iFlags & WDT_ONCE))) {
      errno = 0;
      bIsDir = isEffectiveDir(pRelatName); /* In Windows+MsvcLibX, this may fail, despite d_type == DT_DIR above */
      if (bIsDir && ((pUniqueID = GetUniqueIdString(pRelatName)) != NULL) && pUniqueID[0]) {
	if (pOpts->iFlags & WDT_FOLLOW) {
	  NAMELIST *pList;
	  list.path = pUniqueID; /* Record this path for next time */
	  /* Check if we've seen this path before in the parent folders */
	  for (pList = prev; pList; pList = pList->prev) {
	    if (!strcmp(pUniqueID, pList->path)) {
	      pszBadLinkMsg = "Link loops back";
	      break;
	    }
	  }
	}
      } else { /* pPathname is a symlink pointing to a file, or a link looping to itself */
	if (bIsDir && !pUniqueID) goto out_of_memory;
	if (errno) switch (errno) {
	case ELOOP:	/* There's a link looping to itself */
	  pszBadLinkMsg = "Link loops to itself"; break;
	case ENOENT:	/* There's a dangling link */
	  pszBadLinkMsg = "Dangling link"; break;
	case EBADF:	/* Unsupported link type, ex: Linux symlink */
	case EINVAL:	/* Unsupported reparse point type */
	  pszBadLinkMsg = "Unsupported link type"; break;
	default: /* There's a real error we don't know how to handle */
	  if (iPrintErrors) pferror("Can't resolve \"%s\": %s", pPathname, strerror(errno));
	  goto check_whether_to_continue; /* Don't report this link to the callback */
	}
      }
    }
#endif /* OS_HAS_LINKS */

    /* Report the valid directory entry to the callback */
    if (!(pOpts->iFlags & WDT_DIRONLY)) {
      iRet = pWalkDirTreeCB(pPathname, pDE, pRef);
      if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */
    }

    switch (pDE->d_type) {
#if OS_HAS_LINKS
      case DT_LNK:
	if (pszBadLinkMsg) {
	  if (pOpts->iFlags & WDT_FOLLOW) { /* When following links, it's an error */
	    if (iPrintErrors) pferror("%s: \"%s\"", pszBadLinkMsg, pPathname);
	    goto check_whether_to_continue;
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
	  pszPrevious = DictValue(dict, pUniqueID);
	  if (pszPrevious) { /* The same directory has been visited before under another alias name */
	    if (!(pOpts->iFlags & WDT_QUIET)) {
	      fprintf(stderr, "Notice: Already visited \"%s\" as \"%s\"\n", pPathname, pszPrevious);
	    }
	    break;
	  } else { /* OK, we've not visited this directory before. Record its name in the dictionary */
	    char *pszDup = strdup(pPathname);
	    if (!pszDup) goto out_of_memory;
	    NewDictValue(dict, pUniqueID, pszDup);
	  }
	}
#endif /* OS_HAS_LINKS */
	if (pOpts->iFlags & WDT_DIRONLY) {
	  iRet = pWalkDirTreeCB(pPathname, pDE, pRef);
	  if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */
	}
	if (!(pOpts->iFlags & WDT_NORECURSE)) {
	  if ((!pOpts->iMaxDepth) || (iDepth < pOpts->iMaxDepth)) {
	    if (!list.path) list.path = pPathname;
	    iRet = WalkDirTree1(pPathname, pOpts, pWalkDirTreeCB, pRef, &list, iDepth+1);
	    DEBUG_PRINTF(("// Back walking in \"%s\"\n", path));
	  }
	}
      	break;
      default:
      	break;
    }
    if (iRet) break;	/* -1 = Error, abort; 1 = Success, stop */
 
#if OS_HAS_LINKS /* Right now, we jump to this label only while treating links */
    if (0) { /* Skip this block when coming from immediately above */
check_whether_to_continue:
      if (!(pOpts->iFlags & WDT_CONTINUE)) goto recoverable_error;
      pOpts->nErr += 1; /* Else count the error, and keep searching */
    }
#endif

    /* Free the buffers that will be reallocated during the next loop */
    free(pPathname);
    pPathname = NULL;
#if OS_HAS_LINKS
    free(pUniqueID);
    pUniqueID = NULL;
#endif /* OS_HAS_LINKS */
  }
  if (iRet) goto cleanup_and_return;;	/* -1 = Error, abort; 1 = Success, stop */
  /* Else it's readdir() that failed. Check errno to see the reason */
  if (errno == 0) goto cleanup_and_return;	/* There are no more files (Unix) */
  if (errno == ENOENT) goto cleanup_and_return;	/* There are no more files (MsvcLibX) */
  pszFailingOpVerb = "read"; /* Anything else is an unexpected error we can't handle */
print_dir_op_error:
  if (iPrintErrors) pferror("Can't %s dir \"%s\": %s", pszFailingOpVerb, path, strerror(errno));
  if (iDepth > 0) goto recoverable_error; /* Maybe walking in a parallel branch will be possible */
  goto unrecoverable_error; /* The failure is at the base of the tree */

out_of_memory:
  if (iPrintErrors) pferror("Out of memory");
  goto unrecoverable_error;

recoverable_error:	/* Error preventing local operation (Ex: access denied), but not further operation elsewhere */
  if (pOpts->iFlags & WDT_CONTINUE) goto count_err_and_return;
unrecoverable_error:	/* Severe error preventing further operation. ex: out of memory */
  iRet = -1;
count_err_and_return:	/* Count an error, cleanup and return */
  pOpts->nErr += 1;
cleanup_and_return:
  if (pDir) closedirx(pDir);
  if (pFakeInOutDE) { /* Only notify the exit if the entry notification was sent */
    pFakeInOutDE->d_type = DT_LEAVE;
    iRet = pWalkDirTreeCB(path, pFakeInOutDE, pRef); /* Notify the callback of the directory exit */
  }
  free(pPathname);
#if OS_HAS_LINKS
  free(pRootBuf);
  free(pUniqueID);
  if (bCreatedDict) { /* We're the first folder that created the visited dictionary. Delete it before returning. */
    dictnode *pNode;
    while ((pNode = FirstDictValue(dict)) != NULL) {
      DeleteDictValue(dict, pNode->pszKey);
    }
    free(dict);
    pOpts->pOnce = NULL;
  }
#endif /* OS_HAS_LINKS */
  free(pFakeInOutDE);
  if (pOpts->iFlags & WDT_CD) {
    char *pPath1 = pPath0 ? pPath0 : "..";
    iErr = chdir(pPath1);
    if (iErr) {
      if (iPrintErrors) pferror("Can't return to \"%s\": %s", pPath1, strerror(errno));
      iRet = -1;
    }
    FREE_PATHNAME_BUF(pPath0);
#if HAS_DRIVES
    if (iDrive0) {
      iErr = _chdrive(iDrive0);
      if (iErr) {
      	if (iPrintErrors) pferror("Can't return to drive %c: %s", iDrive0 + '@', strerror(errno));
      	iRet = -1;
      }
    }
#endif
  }
  RETURN_INT_COMMENT(iRet, ((iRet == -1) ? "Error, stop walk\n" : (iRet ? "Success, stop Walk\n" : "Success, continue walk\n")));
}

/* Public routine. Do not instrument with debug macros, to avoid call depth alignment issues. */
int WalkDirTree(const char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef) {
  return WalkDirTree1(path, pOpts, pWalkDirTreeCB, pRef, NULL, 0);
}
