/************************ :encoding=UTF-8:tabSize=8: *************************\
*                                                                             *
*   Filename	    pathnames.h						      *
*									      *
*   Description     Constants and routines for managing pathnames	      *
*                                                                             *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2021-12-15 JFL Created this file.					      *
*    2025-11-10 JFL Added the ability to limit WalkDirTree() recursion depth. *
*    2025-11-11 JFL Added macros for managing file (path)names buffers.	      *
*    2025-11-15 JFL Added WalkDirTree flag WDT_CBINOUT.			      *
*    2025-11-17 JFL Added WalkDirTree flags WDT_DIRONLY and WDT_CD.	      *
*    2025-11-25 JFL WalkDirTree() now uses DT_ENTER & DT_LEAVE to report      *
*		    directory entry and exit.				      *
*    2025-11-27 JFL Added routine NormalizePath().			      *
*		    Fixed macro TRIM_NODENAME_BUF().			      *
*    2025-12-02 JFL Added cwd-pwd.c definitions.			      *
*    2025-12-17 JFL Added support for WDT_INONLY.                             *
*    2025-12-21 JFL Fixed TRIM_PATHNAME_BUF() and TRIM_NODENAME_BUF().        *
*    2025-12-30 JFL WalkDirTree() can now optionally sort directories.        *
*		    							      *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_PATHNAMES_H_
#define _SYSLIB_PATHNAMES_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debugm.h"		/* For ShrinkBuf() */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE
#define IGNORECASE TRUE

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2		/* To be defined on the command line for the OS/2 version */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE
#define IGNORECASE TRUE

#endif /* _OS2 */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE
#define IGNORECASE TRUE

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#ifdef _UNIX		/* Defined in NMaker versions.h for Unix flavors we support */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES FALSE
#define IGNORECASE FALSE

#endif /* __unix__ */

/*********************************** Other ***********************************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(PATTERN_ALL))
#error "Unsupported OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

/* Flag OSs that have links (For some OSs which don't, macros are defined, but S_ISLNK always returns 0) */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK)
  #define OS_HAS_LINKS 1
#else
  #define OS_HAS_LINKS 0
#endif

/*****************************************************************************/

/* Helper macros for managing temporary buffers for file pathnames */

#define PATHNAME_BUF_SIZE PATH_MAX	/* Buffer size for holding pathnames, including NUL */
#define NODENAME_BUF_SIZE (NAME_MAX+1)	/* Buffer size for holding file names, including NUL */

#define PATHNAME_DO_NOTHING() do {} while (0)
#define PATHNAME_DO(code) do {code} while (0)

#define NEW_PATHNAME_BUF() malloc(PATHNAME_BUF_SIZE)
#if PATHNAME_BUF_SIZE < 256 /* Simplified implementation for MSDOS */
  #define PATHNAME_BUFS_IN_HEAP 0	/* Alloc them on the stack */
  #define PATHNAME_BUF(var) char var[PATHNAME_BUF_SIZE]
  #define IF_PATHNAME_BUFS_IN_HEAP(code) PATHNAME_DO_NOTHING();
#else	/* Avoid stack overflows by storing large names in the heap */
  #define PATHNAME_BUFS_IN_HEAP 1	/* Alloc them in the memory heap */
  #define PATHNAME_BUF(var) char *var = NEW_PATHNAME_BUF()
  #define IF_PATHNAME_BUFS_IN_HEAP(code) PATHNAME_DO(code);
#endif
#define TRIM_PATHNAME_BUF(var) IF_PATHNAME_BUFS_IN_HEAP(var = ShrinkBuf(var, strlen(var)+1);)
#define FREE_PATHNAME_BUF(var) IF_PATHNAME_BUFS_IN_HEAP(free(var);)

#define NEW_NODENAME_BUF() malloc(NODENAME_BUF_SIZE)
#if NODENAME_BUF_SIZE < 16 /* Simplified implementation for MSDOS */
  #define NODENAME_BUFS_IN_HEAP 0	/* Alloc them on the stack */
  #define NODENAME_BUF(var) char var[NODENAME_BUF_SIZE]
  #define IF_NODENAME_BUF_IN_HEAP(code) PATHNAME_DO_NOTHING();
#else	/* Avoid stack overflows by storing large names in the heap */
  #define NODENAME_BUFS_IN_HEAP 1	/* Alloc them in the memory heap */
  #define NODENAME_BUF(var) char *var = NEW_NODENAME_BUF()
  #define IF_NODENAME_BUF_IN_HEAP(code) PATHNAME_DO(code);
#endif
#define TRIM_NODENAME_BUF(var) IF_NODENAME_BUF_IN_HEAP(var = ShrinkBuf(var, strlen(var)+1);)
#define FREE_NODENAME_BUF(var) IF_NODENAME_BUF_IN_HEAP(free(var);)

/* End of helper macros for managing temporary buffers for file pathnames */

/*****************************************************************************/

/* Public routines */

int NormalizePath(char *path);	/* Remove unnecessaty / . .. parts in a path. Return 0=success */
char *NewJoinedPath(const char *pszPart1, const char *pszPart2);	/* Join 2 paths, and return the new string */
char *NewCompactJoinedPath(const char *pszPart1, const char *pszPart2);	/* Idem, normalizing it after joining it */

/* ------------------------ WalkDirTree definitions ------------------------ */

/* WalkDirTree option flags */
#define WDT_CONTINUE	0x0001		/* Handle recoverable errors as warnings, and continue */
#define WDT_QUIET	0x0002		/* Do not display warnings & infos */
#define WDT_NORECURSE	0x0004		/* Do not recurse into subdirectories */
#define WDT_FOLLOW	0x0008		/* Recurse into junctions & symlinkds */
#define WDT_ONCE	0x0010		/* Scan multi-linked directories only once */
#define WDT_CBINOUT	0x0020		/* Callback when entering and leaving a directory */
#define WDT_DIRONLY	0x0040		/* Callback for effective directories (ie. links too if WDT_FOLLOW), but not for effective files */
#define WDT_CD		0x0080		/* Change current directory to the directories scanned */
#define WDT_INONLY	0x0100		/* Callback only when entering directories, but not for their content */
/* The following flag must be last, with the highest defined bit */
#define WDT_USER_FLAG   0x0200		/* Allow adding user-defined flags, for use in the callbacks */

/* Dummy dirent dir types, giving special infos to the callback.
   DT_XXX dir types defined in dirent.h typically are in the 0-15 range */
/* Known risk: These two constants might collide with existing types in exotic OSs.
   TO DO, at the cost of compatibility with older SysToolsLib C/SRC code:
   Rewrite WalkDirTree() to pass an additional callback argument: CB_DIRENT | CB_ENTRY | CB_LEAVE */
#define DT_ENTER	0xF0		/* Inform the callback that we're entering a directory */
#define DT_LEAVE	0xF1		/* Inform the callback that we're leaving a directory */

typedef void (*pSortDEListProc)(struct dirent **pDEList, int nDE);

typedef struct {		/* WalkDirTree options. Must be cleared before use. */
  int iFlags;			/* [IN] Options */
  int iMaxDepth;		/* [IN] Maximum recursion depth. 0=No limit */
  pSortDEListProc pSortProc;	/* [IN] Optional routine for sorting dir. entries (Most OSs return entries sorted already) */
  ino_t nDir;			/* [OUT] Number of directories scanned */
  ino_t nFile;			/* [OUT] Number of directory entries processed */
  int nErr;			/* [OUT] Number of errors */
  void *pOnce;			/* [RESERVED] Used internally to process WDT_ONCE */
} wdt_opts;

typedef int (*pWalkDirTreeCB_t)(const char *pszRelPath, const struct dirent *pDE, void *pRef);

extern int WalkDirTree(const char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef);

extern int *DirentExtraFlags(struct dirent *pDE); /* WalkDirTree() appends extra flags to the dirent structures to be sorted */
#define DEF_ISDIR	0x0001	/* If set, the entry is a directory, or a link to a dir. */
#define DEF_RECURSE	0x0002	/* If set, WalkDirTree() will recurse in this dir */

/* ------------------------- cwd-pwd.c definitions ------------------------- */

#ifdef _UNIX	/* In Unix, redefine chdir & getcwd as our extended routines */

char *GetCWD(char *buf, size_t len);	/* Get the logical current directory = PWD env var */
char *NewCWDString(void);		/* Get a new string with the logical current directory */
int ChDir(const char *pszPath);		/* Set the logical and physical current directory */

#undef chdir
#define chdir ChDir

#undef getcwd       
#define getcwd GetCWD

#define getcwd0 NewCWDString

#endif /* defined(_UNIX) */

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_PATHNAMES_H_ */
