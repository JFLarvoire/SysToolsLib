/*****************************************************************************\
*                                                                             *
*   Filename:	    sys/types.h						      *
*                                                                             *
*   Description:    Add missing definitions in MSVC's sys/types.h.	      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2014-05-26 JFL Created this file.                                        *
*    2014-06-06 JFL Moved mode_t & off*_t definitions here, from sys\stat.h.  *
*    2015-11-15 JFL Visual Studio 2015 moved this file to the Windows Kit UCRT.
*    2017-02-28 JFL Redefine pid_t as an int in all cases: It's more standard.*
*    2022-01-05 JFL Optionally redefine _dev_t and _ino_t.		      *
*    2022-02-25 JFL Use different definitions for dev_t,ino_t & _dev_t,_ino_t.*
*									      *
*         � Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef	_MSVCLIBX_SYS_TYPES_H
#define	_MSVCLIBX_SYS_TYPES_H 1

#include "msvclibx.h"

#if _USE_EXTENDED_STAT_STRUCT

/* Redefine the dev_t and ino_t types. Must be the same as in wchar.h */
#if !defined(_LIBX_INO_T_DEFINED)

#if defined(_DEV_T_DEFINED) || defined(_INO_T_DEFINED) /* Avoid using the wrong definition */
#error "_dev_t and/or _ino_t have already been defined by MSVC own include files"
#endif

/* Duplicate MSVC own _dev_t and _ino_t fake types definitions */
#if defined(_MSDOS)
typedef short _dev_t;
#elif defined(_WIN32)
typedef int _dev_t;
#endif
#define _DEV_T_DEFINED	 /* Prevent MSVC's own <sys/types.h> file from redefining it */

typedef unsigned short _ino_t;
#define _INO_T_DEFINED	 /* Prevent MSVC's own <sys/types.h> file from redefining it */

/* Define an actual device ID type for MsvcLibX. Must be an integer type. */
#if defined(_MSDOS)
typedef short dev_t;		/* Use the drive letter */
#elif defined(_WIN32)
typedef __int64 dev_t;		/* Use the device Serial Number */
#endif

/* Define an actual inode number type. Must be an unsigned integer type. */
#if defined(_MSDOS)
typedef unsigned long ino_t;	/* Use the cluster number */
#elif defined(_WIN32)
typedef unsigned __int64 ino_t;	/* Use the file ID Number */
#endif
#define _LIBX_INO_T_DEFINED	/* Prevent MsvcLibX wchar.h from redefining it */

#endif /* !defined(_LIBX_INO_T_DEFINED) */
#endif /* _USE_EXTENDED_STAT_STRUCT */

/*****************************************************************************/

#include UCRT_INCLUDE_FILE(sys\types.h) /* Include MSVC's own <sys/types.h> file */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

/* File offset type */
/* For now, use the MSVC 32-bits functions in all cases */
#define off_t _off_t

typedef int pid_t;  /* The pid is the PSP segment. MSVC's process.h defines it as int. */

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

/* File offset types */
#define off64_t __int64 /* Don't use a typedef because MSVC's _fseeki64 doesn't like it */

#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
  #define off_t off64_t
#else
  #define off_t _off_t
#endif

typedef unsigned long DWORD; /* Don't include <windows.h> to get DWORD definition, as this draws too much,
				including things we want to override in other parts of MsvcLibX */

typedef int pid_t;        /* Windows defines it as a DWORD, which is the same size as in int, in both WIN32 and WIN64 */

#endif /* defined(_WIN32) */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* Automatically defined when targeting an OS/2 application? */

/* File offset type */
/* For now, use the MSVC 32-bits functions in all cases */
#define off_t _off_t


#endif /* defined(_OS2) */

/********************** End of OS-specific definitions ***********************/

/* Signed size type */
#ifndef _SSIZE_T_DEFINED
#if defined(_MSDOS)
typedef int ssize_t;
#elif defined(_WIN32)
#include <stdint.h>
#ifdef  _WIN64
typedef __int64 ssize_t;
#else
typedef _W64 int ssize_t;
#endif /* defined(_WIN64) */
#endif /* defined(_WIN32) */
#define _SSIZE_T_DEFINED
#endif /* !_SSIZE_T_DEFINED */

/* MsvcLibX handles uid_t and gid_t in pwd.c and grp.c */
typedef int gid_t;
typedef int uid_t;

/* MsvcLibX uses mode_t in sys/stat.h */
typedef int mode_t;

/* File link counts type (not used by MsvcLibX so far) */
typedef int nlink_t; /* Is short in some Unix versions */

#endif /* !defined(_MSVCLIBX_SYS_TYPES_H) */

