/*****************************************************************************\
*                                                                             *
*   Filename	    asprintf.c						      *
*									      *
*   Description     Reimplementation of GNU library's asprintf()	      *
*                                                                             *
*   Notes	    Uses _vsnprintf(), which is available in all MSVC versions.
*		    							      *
*   History								      *
*    2020-06-26 JFL Created this module, based on debugm.h macros.	      *
*                                                                             *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS /* Disable safety warning for _vsnprintf() */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _MSDOS	/* The MSVC 1.52 compiler for MS-DOS does not support inline for C */
#define INLINE static	/* So hide it as a static function instead */
#else		/* The VC++ compilers for Windows do support inline as _inline */
#define INLINE _inline
#endif

/* Work around the theoric (but unlikely) possibility that shrinking a buffer can fail */
INLINE char *try_shrink_buf(void *pBuf, int iSize) {
  char *p2 = realloc(pBuf, iSize);
  return p2 ? p2 : pBuf; /* If this failed, return the initial buffer */
}

/* Lower level version using a va_list */
int vasprintf(char **ppszBuf, const char *pszFormat, va_list vl) {
  char *pBuf, *pBuf2;
  int n, nBufSize = 64;
  va_list vl0, vl2;
  /* First try it once with the original va_list (When nBufSize == 128) */
  /* This consumes the vl arguments, which needs to be done once */
  va_copy(vl0, vl);	/* Save a copy of the caller's va_list */
  for (pBuf = NULL; (pBuf2 = (char *)realloc(pBuf, nBufSize *= 2)) != NULL; ) {
    va_copy(vl2, vl0);
    n = _vsnprintf(pBuf = pBuf2, nBufSize, pszFormat, (nBufSize == 128) ? vl : vl2);
    va_end(vl2);
    if ((n >= 0) && (n < nBufSize)) { /* Success at last, now we know the necessary size */
      *ppszBuf = try_shrink_buf(pBuf, n+1); /* Free the unused space in the end */
      va_end(vl0);
      return n;
    } /* Else if n == nBufSize, actually not success, as there's no NUL in the end */
  }
  va_end(vl0);
  return -1;
}

/* Upper level version using ellipsis */
int asprintf(char **ppszBuf, const char *pszFormat, ...) {
  int iRet;
  va_list vl;
  va_start(vl, pszFormat);
  iRet = vasprintf(ppszBuf, pszFormat, vl);
  va_end(vl);
  return iRet;
}

