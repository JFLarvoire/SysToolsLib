/*****************************************************************************\
*                                                                             *
*   File name	    predefine.h				                      *
*                                                                             *
*   Description     Define optional features we need in the C libraries	      *
*                                                                             *
*   Notes           Include this file before the first C library include file.*
*                                                                             *
*                   This file defines several old obsolete constants.         *
*                   Please leave them in, as they allow to still build        *
*                   this system tools library on old systems.                 *
*                                                                             *
*   History								      *
*    2020-03-19 JFL jf.larvoire@hpe.com created this file.                    *
*    2020-04-19 JFL Added support for MacOS.                                  *
*		                                                              *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _CLIBX_PREDEFINE_H_
#define _CLIBX_PREDEFINE_H_

#define _CRT_SECURE_NO_WARNINGS 1 /* Disable Microsoft Visual C++ security warnings for old routines */

#define _POSIX_SOURCE		/* Define POSIX extensions. Ex: function fileno in stdio.h */
#define _XOPEN_SOURCE		/* Define XOPEN extensions. Ex: function tempnam in stdio.h */
#define _BSD_SOURCE    		/* Define BSD extensions. Ex: S_IFREG in sys/stat.h */
#define _DEFAULT_SOURCE		/* glibc >= 2.19 will complain about _BSD_SOURCE if it doesn't see this */
#define _LARGEFILE_SOURCE	/* Define LFS extensions. Ex: type off_t, and functions fseeko and ftello */
#define _LARGEFILE64_SOURCE	/* Define extra routines supporting 64-bit file sizes */
#define _GNU_SOURCE		/* Implies all the above. And also MsvcLibX support for UTF-8 I/O */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes by default, if the OS supports it */
#define _LARGEFILE_SOURCE64	/* Old obsolete alias for the same? */

#define _ISOC99_SOURCE		/* Tell the GNU library that we want C99 extensions */
#define __STDC_LIMIT_MACROS	/* Make sure C99 macros are defined in C++ */
#define __STDC_CONSTANT_MACROS

#define _DARWIN_C_SOURCE    	/* On a Mac, define MacOS extensions */

#endif  /* _CLIBX_PREDEFINE_H_ */
