/*****************************************************************************\
*                                                                             *
*   File name	    pferror.c						      *
*                                                                             *
*   Description	    Print a formatted error message			      *
*                                                                             *
*   Notes	    A generalized version of perror().                        *
*                                                                             *
*   History                                                                   *
*    2021-12-15 JFL Created this file.					      *
*    2025-12-20 JFL Added functions getprogname() for DOS and Windows.        *
*                   Automatically prepend the program name to the error msg.  *
*                   Added functions that automatically append the LibC error. *
*                   Added functions for displaying warnings.	              *
*    2026-01-28 JFL Allow not passing a message, and show just on the C error.*
*                                                                             *
\*****************************************************************************/

/* Make sure the include files define the extensions we need */
#define _GNU_SOURCE		/* Define GNU extensions */
/* #define _BSD_SOURCE   	   BSD extensions are defined by default, and GNU actually complains about them */
#define _DARWIN_C_SOURCE    	/* Define MacOS extensions */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "mainutil.h"	/* This module definitions */
#include "progname.h"	/* Define the getprogname() function or macro */

/* Generate a compound error message, and print it on stderr. Msg structure: */
/* PROGNAME: MSG_TYPE: CONTEXT_DESCRIPTION: ROOT_CAUSE */
/* Common subroutine used by all the following error and warning routines */
int pGenError(char *pszType, char *pszFormat, va_list vl, char *pszCause) {
  int n = 0;
  const char *pszName = getprogname();
  if (pszName && pszName[0]) n += fprintf(stderr, "%s: ", pszName);
  if (pszType) n += fprintf(stderr, "%s: ", pszType); /* Error, Warning, Info, etc... */
  if (pszFormat) n += vfprintf(stderr, pszFormat, vl);	/* The main part of the error message */
  if (pszFormat && pszCause) n += fprintf(stderr, ": ");
  if (pszCause) n += fprintf(stderr, "%s", pszCause); /* Optional C or OS error message */
  if (fputs("\n", stderr) >= 0) n += 1;
  return n;
}

int CDECL pferror(char *pszFormat, ...) {
  va_list vl;
  int n;
  va_start(vl, pszFormat);
  n = pGenError("Error", pszFormat, vl, NULL);
  va_end(vl);
  return n;
}

int CDECL pfcerror(char *pszFormat, ...) {
  va_list vl;
  int n;
  va_start(vl, pszFormat);
  n = pGenError("Error", pszFormat, vl, strerror(errno));
  va_end(vl);
  return n;
}

int CDECL pfwarning(char *pszFormat, ...) {
  va_list vl;
  int n;
  va_start(vl, pszFormat);
  n = pGenError("Warning", pszFormat, vl, NULL);
  va_end(vl);
  return n;
}

int CDECL pfcwarning(char *pszFormat, ...) {
  va_list vl;
  int n;
  va_start(vl, pszFormat);
  n = pGenError("Warning", pszFormat, vl, strerror(errno));
  va_end(vl);
  return n;
}

int CDECL pfnotice(char *pszType, char *pszFormat, ...) {
  va_list vl;
  int n;
  va_start(vl, pszFormat);
  n = pGenError(pszType, pszFormat, vl, NULL);
  va_end(vl);
  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    getprogname						      |
|									      |
|   Description     Get the program invocation name 			      |
|									      |
|   Parameters      None						      |
|									      |
|   Returns	    A pointer to the program invocation name		      |
|		    							      |
|   Notes	    DOS & Windows ports of the BSD and MacOS eponym.	      |
|		    							      |
|   History								      |
|    2025-12-20 JFL Created these routines.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
#include <stdlib.h>	/* For _pgmptr */
static char *_progname = NULL;
const char *getprogname(void) {
  if (!_progname) {
    char far *lpc = _fstrrchr(_pgmptr, '\\');
    size_t l;
    lpc = lpc ? (lpc+1) : _pgmptr;
    l = _fstrlen(lpc);
    _progname = malloc(l + 1);
    if (_progname) {
      char *pc;
      _fstrcpy(_progname, lpc);
      pc = strrchr(_progname, '.');
      if (pc) *pc = '\0';	/* Remove the extension */
      strlwr(_progname);	/* Make it look less agressive */
    } else {
      _progname = "";
    }
  }
  return _progname;
}
#endif

#ifdef _WIN32
static char *_progname = NULL;
_CRTIMP extern char **__argv;
const char *getprogname(void) {
  if (!_progname) {
    char *pc = strrchr(__argv[0], '\\');
    _progname = pc ? (pc+1) : __argv[0];
    pc = strrchr(_progname, '.');
    if (pc) *pc = '\0';		/* Remove the extension */
  }
  return _progname;
}
#endif

