/*****************************************************************************\
*                                                                             *
*   Filename	    wchar.h						      *
*                                                                             *
*   Description     Add missing definitions in MSVC's sys/types.h.	      *
*                                                                             *
*   Notes	    							      *
*                                                                             *
*   History								      *
*    2022-01-05 JFL Created this file.                                        *
*									      *
*         © Copyright 2022 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef	_MSVCLIBX_WCHAR_H
#define	_MSVCLIBX_WCHAR_H 1

#include "msvclibx.h"

#if _USE_EXTENDED_STAT_STRUCT

/* Redefine the _dev_t and _ino_t types. Must be the same as in sys/types.h */

/* Define an actual device ID type. Must be an integer type. */
#if defined(_MSDOS)
typedef short _dev_t;			/* Use the drive letter */
#elif defined(_WIN32)
typedef __int64 _dev_t;			/* Use the device Serial Number */
#endif
#define _DEV_T_DEFINED	 /* Prevent MSVC's own <sys/types.h> file from redefining it */

/* Define an actual inode number type. Must be an unsigned integer type. */
#if defined(_MSDOS)
typedef unsigned long _ino_t;		/* Use the cluster number */
#elif defined(_WIN32)
typedef unsigned __int64 _ino_t;	/* Use the file ID Number */
#endif
#define _INO_T_DEFINED	 /* Prevent MSVC's own <sys/types.h> file from redefining it */

#endif /* _USE_EXTENDED_STAT_STRUCT */

/*****************************************************************************/

#include UCRT_INCLUDE_FILE(wchar.h) /* Include MSVC's own <sys/types.h> file */

#endif /* !defined(_MSVCLIBX_WCHAR_H) */
