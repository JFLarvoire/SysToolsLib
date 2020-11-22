/*****************************************************************************\
*                                                                             *
*   Filename        copydate.c                                                *
*                                                                             *
*   Description     Copy the date/time stamp from one file to another         *
*                                                                             *
*   Notes           							      *
*                                                                             *
*   History                                                                   *
*    2020-11-05 JFL Factored-out the copydate routine from backnum, update,...*
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using fopen, etc */

#define _GNU_SOURCE		/* Include as many extensions as possible */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <sys/time.h>		/* For lutimes() */
#include <errno.h>

#include "debugm.h"		/* SysToolsLib debug macros */

#include "copyfile.h"		/* Public definitions for this file */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define LocalFileTime localtime

/* In MacOS, these struct stat fields have different names */
#if defined(__MACH__)
#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#endif

#endif /* __unix__ */

/*********************** End of OS-specific definitions **********************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function 	    copydate						      |
|									      |
|   Description     Copy the date/time stamp from one file to another	      |
|									      |
|   Parameters      const char *pszToFile	Destination file	      |
|		    const char *pszFromFile	Source file		      |
|									      |
|   Returns 	    0=success, else error and errno set			      |
|									      |
|   Notes 	    This operation is useless if the destination file is      |
|		    written to again. So it's necessary to flush it before    |
|		    calling this function.				      |
|									      |
|   History 								      |
|    1996-10-14 JFL Made a clean, application-independant, version.	      |
|    2011-05-12 JFL Rewrote in an OS-independant way.			      |
|    2015-01-08 JFL Fallback to using chmod and utimes if lchmod and lutimes  |
|                   are not implemented. This will cause minor problems if the|
|                   target is a link, but will work well in all other cases.  |
|    2020-11-04 JFL Restructured with support for ns resolution when possible.|
*									      *
\*---------------------------------------------------------------------------*/

#if 0 && defined(_UNIX) /* Useful to debug time functions availability issues */
  #ifdef _BSD_SOURCE
    #pragma message("#define _BSD_SOURCE // => lutimes() defined")
  #else
    #pragma message("#undef  _BSD_SOURCE // => lutimes() NOT defined")
  #endif
  
  #ifdef _POSIX_C_SOURCE
    #pragma message("#define _POSIX_C_SOURCE " VALUEIZE(_POSIX_C_SOURCE) " // => utimensat() defined if >= 200809L")
  #else
    #pragma message("#undef  _POSIX_C_SOURCE // => utimensat() probably NOT defined")
  #endif
  
  #ifdef _ATFILE_SOURCE
    #pragma message("#define _ATFILE_SOURCE // => utimensat() defined if glibc < 2.10")
  #else
    #pragma message("#undef  _ATFILE_SOURCE // => utimensat() probably NOT defined")
  #endif
  
  #ifdef _GNU_SOURCE
    #pragma message("#define _GNU_SOURCE")
  #else
    #pragma message("#undef  _GNU_SOURCE")
  #endif
#endif

/* Old Glibc versions define lchmod, but only implement a stub that always fails */
#ifdef __stub_lchmod
#if defined(_DEBUG)
#pragma message("The C Library implements a fake lchmod(). Using our own replacement.")
#endif
#ifdef _ATFILE_SOURCE
#define lchmod(path, mode) fchmodat(AT_FDCWD, path, mode, AT_SYMLINK_NOFOLLOW)
#else
#define lchmod lchmod1 /* Then use our own replacement for lchmod */
int lchmod1(const char *path, mode_t mode) {
  struct stat st = {0};
  int err;
  DEBUG_PRINTF(("lchmod1(\"%s\", %X);\n", path, mode));
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lchmod all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = chmod(path, mode);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif
#endif

/*---------------------------------------------------------------------------*\
|*               Preferred implementation with 1ns resolution                *|
\*---------------------------------------------------------------------------*/

#if defined(__USE_XOPEN2K8) || (defined(__POSIX_VISIBLE) && (__POSIX_VISIBLE >= 200809L))

#if defined(_DEBUG)
/* This is the normal case, so don't even display the message anymore
#pragma message("Using utimensat() to copy timestamps with 1ns resolution.")
*/
#endif
/* References:
   https://pubs.opengroup.org/onlinepubs/9699919799/functions/utimensat.html
   https://manpages.ubuntu.com/manpages/trusty/en/man2/utimensat.2.html
   https://www.freebsd.org/cgi/man.cgi?query=utimensat&sektion=2
*/

int copydate(const char *pszToFile, const char *pszFromFile) { /* Copy the file dates */
  struct stat stFrom = {0};
  struct timespec tsTo[2] = {{0}, {0}};
  int err;
  err = lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  tsTo[0] = stFrom.st_atim;
  tsTo[1] = stFrom.st_mtim;
  err = utimensat(AT_FDCWD, pszToFile, tsTo, AT_SYMLINK_NOFOLLOW);
  return err;                       /* Success */
}

/*---------------------------------------------------------------------------*\
|*              Second best implementation with 1us resolution               *|
\*---------------------------------------------------------------------------*/

#elif defined(_STRUCT_TIMEVAL)

#if defined(_DEBUG)
#pragma message("Using lutimes() to copy timestamps with 1us resolution.")
#endif
/* References:
   Not listed in pubs.opengroup.org
   https://manpages.ubuntu.com/manpages/trusty/en/man3/lutimes.3.html
   https://www.freebsd.org/cgi/man.cgi?query=lutimes&sektion=2
*/

/* Some Unix C libraries also have a non-functional stub for this one */
#ifdef __stub_lutimes
#pragma message("The C Library implements a fake lutimes(). Using our own replacement.")
#define lutimes lutimes1 /* Then use our own replacement for lutimes */
int lutimes1(const char *path, const struct timeval times[2]) {
  struct stat st = {0};
  int err;
  /* DEBUG_PRINTF(("lutimes1(\"%s\", %p);\n", path, &times)); // No need for this as VALUEIZE(lutimes) duplicates this below. */
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lutimes all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = utimes(path, times);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif

/* Micro-second file time resolution supported. Use lutimes(). */
int copydate(const char *pszToFile, const char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct timeval tvTo[2] = {{0}, {0}};
  int err;
  lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  TIMESPEC_TO_TIMEVAL(&tvTo[0], &stFrom.st_atim);
  TIMESPEC_TO_TIMEVAL(&tvTo[1], &stFrom.st_mtim);
  err = lutimes(pszToFile, tvTo);
#ifndef _MSVCLIBX_H_ /* Trace lutimes() call and return in Linux too */
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(stFrom.st_mtime)); /* Time of last data modification */
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d.%06ld",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec, (long)stFrom.st_mtim.tv_nsec / 1000);
    DEBUG_PRINTF((VALUEIZE(lutimes) "(\"%s\", %s) = %d\n", pszToFile, buf, err));
  });
#endif
  return err;                       /* Success */
}

/*---------------------------------------------------------------------------*\
|*               Worst implementation with just 1s resolution                *|
\*---------------------------------------------------------------------------*/

#else /* !defined(_STRUCT_TIMEVAL) */

#if defined(_DEBUG)
#pragma message("Using utime() to copy timestamps with 1s resolution.")
#endif
/* References:
   https://pubs.opengroup.org/onlinepubs/9699919799/functions/utime.html
   https://manpages.ubuntu.com/manpages/trusty/en/man2/utime.2.html
   https://www.freebsd.org/cgi/man.cgi?query=utime&sektion=3
*/

/* No support for micro-second or better file time resolution. Ex: in MS-DOS. */
int copydate(const char *pszToFile, const char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct utimbuf utbTo = {0};
  int err;
  DEBUG_PRINTF(("copydate(\"%s\", \"%s\")\n", pszToFile, pszFromFile));
  err = lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  utbTo.actime = stFrom.st_atime;
  utbTo.modtime = stFrom.st_mtime;
  err = utime(pszToFile, &utbTo);
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(utbTo.modtime)); /* Time of last data modification */
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
    DEBUG_PRINTF(("utime(\"%s\", %s) = %d %s\n", pszToFile, buf, err,
      		  err?strerror(errno):""));
  });
  return err;                       /* Success */
}

#endif /* !defined(_STRUCT_TIMEVAL) */

