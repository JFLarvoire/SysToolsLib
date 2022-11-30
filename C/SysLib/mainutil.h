/*****************************************************************************\
*                                                                             *
*   File name	    mainutil.h						      *
*                                                                             *
*   Description	    Main C program utilility definitions and routines	      *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-12-15 JFL Created this file.					      *
*    2022-10-19 JFL Added definitions for IsSwitch() and for streqi().	      *
*    2022-11-29 JFL Moved the CDECL declaration to SysLib.h.                  *
*                                                                             *
\*****************************************************************************/

#ifndef _SYSLIB_MAINUTIL_H_
#define _SYSLIB_MAINUTIL_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Convenient string comparison macros */
#if defined(_MSDOS)
#define streq(s1, s2) (!strcmp(s1, s2))		/* Test if strings are equal */
#define strieq(s1, s2) (!_stricmp(s1, s2))	/* Idem, not case sensitive */
#define strnieq(s1, s2, n) (!_strnicmp(s1, s2, n))
#elif defined(_WIN32)
#include "Windows.h"
#define streq(s1, s2) (!lstrcmp(s1, s2))	/* Test if strings are equal */
#define strieq(s1, s2) (!lstrcmpi(s1, s2))	/* Idem, not case sensitive */
#define strnieq(s1, s2, n) (!_strnicmp(s1, s2, n))
#else
#define streq(s1, s2) (!strcmp(s1, s2))		/* Test if strings are equal */
#define strieq(s1, s2) (!strcasecmp(s1, s2))	/* Idem, not case sensitive */
#define strnieq(s1, s2, n) (!strncasecmp(s1, s2, n))
#endif

/* Main C modules utility routines */
int CDECL IsSwitch(char *pszArg);	 /* Test if a command-line argument is a switch */
int CDECL pferror(char *pszFormat, ...); /* Print error messages on stderr, in a standardized format */

#endif /* _SYSLIB_MAINUTIL_H_ */
