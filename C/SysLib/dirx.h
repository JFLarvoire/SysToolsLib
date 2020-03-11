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
*									      *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_DIRX_H_
#define _SYSLIB_DIRX_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#if defined(_MSVCLIBX_H_)	/* MsvcLibX always sets d_type. No need for wrapper functions */
#define opendirx(pName) opendir(pName)
#define readdirx(pDir) readdir(pDir)
#define closedirx(pDir) closedir(pDir)
#else				/* Define a set of wrapper functions that do */
#include <dirent.h>		/* Unix directory access functions definitions */
DIR *opendirx(const char *pName);
struct dirent *readdirx(DIR *pDir);	/* Some Unix FS set d_type = UNKNOWN */
int closedirx(DIR *);
#endif

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_DIRX_H_ */
