/*****************************************************************************\
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

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_DIRX_H_ */
