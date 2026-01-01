/************************ :encoding=UTF-8:tabSize=8: *************************\
*                                                                             *
*   Filename:	    dirx.h						      *
*									      *
*   Description:    Directory access functions eXtensions		      *
*                                                                             *
*   Notes:	    Work around the problem of readdir() not always setting   *
*		    d_type for some file systems in Unix		      *
*									      *
*   History:								      *
*    2020-03-11 JFL Created this file.					      *
*    2020-03-19 JFL Enforce that we only supports 64-bits file sizes.	      *
*    2026-01-01 JFL Added macros for accessing non-standard parts of the      *
*		    dirent structure.					      *
*									      *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_DIRX_H_
#define _SYSLIB_DIRX_H_

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#if defined(_MSVCLIBX_H_)	/* MsvcLibX always sets d_type. No need for wrapper functions */

/* Don't bother including "SysLib.h", since here we're actually NOT using anything from SysLib */

#define opendirx(pName) opendir(pName)
#define readdirx(pDir) readdir(pDir)
#define closedirx(pDir) closedir(pDir)

#else				/* Define a set of wrapper functions that do */

#include "SysLib.h"		/* SysLib Library core definitions */
#include <dirent.h>		/* Unix directory access functions definitions */
#include <stddef.h>		/* For the offsetof() macro */

/* Detect unsupported cases */
#if defined(_FILE_OFFSET_BITS)
  #if _FILE_OFFSET_BITS < 64
    #error "SysLib's dirx.c only supports 64-bits file sizes. Please use #define _FILE_OFFSET_BITS 64"
  #endif
#else
  #if defined(__GNUC__) && !defined(_DIRENT_MATCHES_DIRENT64)
    #error "SysLib's dirx.c only supports 64-bits file sizes. Please use #define _FILE_OFFSET_BITS 64"
  #else
    #warning "SysLib's dirx.c only supports 64-bits file sizes. Please use #define _FILE_OFFSET_BITS 64"
  #endif
#endif

DIR *opendirx(const char *pName);
struct dirent *readdirx(DIR *pDir);	/* Some Unix FS set d_type = UNKNOWN */
int closedirx(DIR *);

#endif /* not defined(_MSVCLIBX_H_) */

/*---------------------------------------------------------------------------*/
/* Access to non-standard parts of the dirent structure */

/* The Posix spec only requires the d_ino and d_name members in struct dirent.
   GNU LibC defines constants _DIRENT_HAVE_D_TYPE, _DIRENT_HAVE_D_NAMLEN, and
   _DIRENT_HAVE_D_RECLEN for OSs that have dirent structures with respectively
   d_type, d_namlen, and d_reclen fields.
   Do define these constants for the other build environments we support. */

#if HAS_MSVCLIBX		/* DOS & Windows, using MSVC and our extensions */ 
#define _DIRENT_HAVE_D_TYPE
#endif

#ifdef __DARWIN_MAXPATHLEN	/* MacOS */
#define _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_TYPE
#endif

#if __BSD_VISIBLE		/* FreeBSD */
#define _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_TYPE
#endif

/* Get the length of the file name */
#ifdef _DIRENT_HAVE_D_NAMLEN
#define DirentNamLen(pDE) (int)((pDE)->d_namlen)
#else
#define DirentNamLen(pDE) (int)strlen((pDE)->d_name)
#endif

/* Get the length of the dirent structure record. Usually aligned on an int or long boundary. */
#ifdef _DIRENT_HAVE_D_RECLEN
#define DirentRecLen(pDE) (int)((pDE)->d_reclen)
#else		/* Two constants cancel-out in the addition below: (+1 for the name's NUL) (-1 for (sizeof(int) - 1)) */ 
#define DirentRecLen(pDE) (((int)(offsetof(struct dirent, d_name)) + DirentNamLen(pDE) + sizeof(int)) & ~(sizeof(int) - 1))
#endif

/* Get a pointer to what's following the dirent structure */
#define AfterDirent(pDE) (void *)(((char *)(pDE)) + DirentRecLen(pDE));

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_DIRX_H_ */
