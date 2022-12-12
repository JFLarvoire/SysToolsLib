/*****************************************************************************\
*                                                                             *
*   Filename	    errno.h						      *
*                                                                             *
*   Description     MsvcLibX extensions to errno.h.			      *
*                                                                             *
*   Notes	    							      *
*                                                                             *
*   History								      *
*    2021-12-12 JFL Created this file.					      *
*    2022-12-12 JFL Added a definition for EOVERFLOW.			      *
*									      *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef	_MSVCLIBX_errno_H
#define	_MSVCLIBX_errno_H	1

#include "msvclibx.h"

#ifndef _INC_ERRNO	/* #include guard macro defined by MSVC errno.h */

#ifdef errno	/* Also defined in stddef.h, in which case the UCRT_INCLUDE_FILE() macro fails */
#undef errno		  /* It'll be redefined identically in errno.h anyway */
#undef _CRT_ERRNO_DEFINED /* _CRT_ERRNO_DEFINED would prevent that redefinition */
#endif		/* And now we're sure that the UCRT_INCLUDE_FILE() macro will work properly */

#include UCRT_INCLUDE_FILE(errno.h) /* Include MSVC's own <errno.h> file */

#endif /* defined(_INC_ERRNO) */

#ifdef __cplusplus
extern "C" {
#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

/*
// MS Visual C++ 1.52 for DOS is standard up to errno 34, then diverges up to errno 36.
//  Many errnos within the list are actually unused, and for them _sys_errlist[] = "".
//  List of apparently unused errnos: 1 3-6 10-11 14-16 19-21 23 25-27 29-32 35
*/

#ifndef ENAMETOOLONG /* Not defined in MSVC 1.5 errno.h, but defined in winsock.h */
#define ENAMETOOLONG 38
#endif

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

/* Library-specific routine used internally by many standard routines */
extern int Win32ErrorToErrno(); /* Converts the last WIN32 error to a Posix error code */

#ifndef ELOOP
/*
// Unix defines ELOOP as errno 40.
// MS Visual C++ 1.52 for DOS is standard up to errno 34, then diverges up to errno 36.
//  Many errnos within the list are actually unused, and for them _sys_errlist[] = "".
// MS Visual C++ 9 for Windows is standard up to errno 34, then diverges up to errno 43.
//  Also MSVC9 does not define errno:
//   15      // The posix standard ENOTBLK "Block device required"
//   26      // The posix standard ETXTBSY "Text file busy"
//   35      // Positioned between standard ERANGE and EDEADLK
//   37      // Positioned between standard EDEADLK and ENAMETOOLONG
//   43      // Positioned last, after standard ENOTEMPTY
// The _sys_errlist[] pointer for all the above points to a single string "Unknown error".
// MS Visual C++ 10 and later define ELOOP as 114.
*/
#define ELOOP  35  /* Using the first available slot */  /* Update _sys_errlist[ELOOP] accordingly in any routine that generates ELOOP! */
#endif /* !defined(ELOOP) */

#endif /* defined(_WIN32) */

/********************** End of OS-specific definitions ***********************/

#ifndef EOVERFLOW
#define EOVERFLOW ERANGE /* ERANGE is "Result too large". TODO: Find a unique value & adapt strerror() */
#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(_MSVCLIBX_errno_H)  */

