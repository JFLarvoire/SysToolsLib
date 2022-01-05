/*****************************************************************************\
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

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2		/* To be defined on the command line for the OS/2 version */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#endif /* _OS2 */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32			/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES FALSE

#endif /* __unix__ */

/*********************************** Other ***********************************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(PATTERN_ALL))
#error "Unsupported OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

char *NewJoinedPath(const char *pszPart1, const char *pszPart2);	/* Join 2 paths, and return to new string */
char *NewCompactJoinedPath(const char *pszPart1, const char *pszPart2);	/* Idem, removing all useless ./ parts */

/* WalkDirTree definitions */

/* WalkDirTree option flags */
#define WDT_IGNOREERR	0x0001		/* Ignore directory access errors */
#define WDT_NORECURSE	0x0002		/* Do not recurse into subdirectories */
#define WDT_FOLLOW	0x0004		/* Recurse into junctions & symlinkds */
#define WDT_QUIET	0x0008		/* Do not display minor errors */

typedef struct {	/* WalkDirTree options */
  int iFlags;			/* Options */
  int *pNProcessed;		/* Optional pointer to a number of files tested */
  int nErr;			/* Number of errors */
} wdt_opts;

typedef int (*pWalkDirTreeCB_t)(char *pszRelPath, struct dirent *pDE, void *pRef);

#ifdef _WIN32

/* File ID, unique locally on a server. */
typedef struct { /* Equivalent to the FILE_ID_INFO structure defined in winbase.h */
  /* https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info */
  DWORD dwIDVol0;		/* Volume ID (low DWORD for NTFS and ReFS) */
  DWORD dwIDVol1;		/* Volume ID (high DWORD for ReFS, 0 for NTFS) */
  DWORD dwIDFil0;		/* File ID (low DWORD for NTFS and ReFS) */
  DWORD dwIDFil1;		/* File ID (high DWORD for NTFS and ReFS) */
  DWORD dwIDFil2;		/* File ID (DWORD #3 for ReFS, 0 for NTFS) */
  DWORD dwIDFil3;		/* File ID (DWORD #4 for ReFS, 0 for NTFS) */
} FILE_ID;

extern BOOL GetFileID(const char *pszName, FILE_ID *pFID);

#endif /* _WIN32 */

extern int WalkDirTree(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_PATHNAMES_H_ */
