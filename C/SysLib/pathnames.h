/************************ :encoding=UTF-8:tabSize=8: *************************\
*                                                                             *
*   Filename	    pathnames.h						      *
*									      *
*   Description     Constants and routines for managing pathnames	      *
*                                                                             *
*   Notes	    							      *
*		    							      *
*   History:								      *
*    2021-12-15 JFL Created this file.					      *
*									      *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_PATHNAMES_H_
#define _SYSLIB_PATHNAMES_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#include <dirent.h>
#include <sys/types.h>

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

#ifdef _WIN32			/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE
#define IGNORECASE TRUE

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

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

char *NewJoinedPath(const char *pszPart1, const char *pszPart2);	/* Join 2 paths, and return the new string */
char *NewCompactJoinedPath(const char *pszPart1, const char *pszPart2);	/* Idem, removing all useless ./ parts */

/* WalkDirTree definitions */

/* WalkDirTree option flags */
#define WDT_CONTINUE	0x0001		/* Handle recoverable errors as warnings, and continue */
#define WDT_QUIET	0x0002		/* Do not display warnings & infos */
#define WDT_NORECURSE	0x0004		/* Do not recurse into subdirectories */
#define WDT_FOLLOW	0x0008		/* Recurse into junctions & symlinkds */
#define WDT_ONCE	0x0010		/* Scan multi-linked directories only once */

typedef struct {		/* WalkDirTree options. Must be cleared before use. */
  int iFlags;			/* [IN] Options */
  ino_t nDir;			/* [OUT] Number of directories scanned */
  ino_t nFile;			/* [OUT] Number of directory entries processed */
  int nErr;			/* [OUT] Number of errors */
  void *pOnce;			/* [RESERVED] Used internally to process WDT_ONCE */
} wdt_opts;

typedef int (*pWalkDirTreeCB_t)(const char *pszRelPath, const struct dirent *pDE, void *pRef);

extern int WalkDirTree(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_PATHNAMES_H_ */
