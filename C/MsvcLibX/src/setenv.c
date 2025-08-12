/*****************************************************************************\
*                                                                             *
*   Filename	    setenv.c						      *
*									      *
*   Description     Redefinitions of standard C library's setenv()	      *
*                                                                             *
*   Notes	    							      *
*                                                                             *
*   History								      *
*    2022-12-01 JFL Created this module.				      *
*    2025-08-10 JFL Added a Windows-specific implementation.		      *
*                                                                             *
*		  © Copyright 2022 Jean-François Larvoire		      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define  _CRT_SECURE_NO_WARNINGS /* Prevent "This function or variable may be unsafe" warnings */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debugm.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    setenv						      |
|									      |
|   Description	    Redefine the standard setenv() functions	              |
|									      |
|   Parameters	    char *pszName	Name of the variable to set	      |
|		    char *pszValue	Value to set			      |
|		    int iOverwrite	If TRUE, allow overwriting a previous |
|		    			value for that variable		      |
|		    							      |
|   Returns	    0 = Success, -1 = Failure				      |
|		    							      |
|   Notes	    The Windows-specific version updates the Unicode version  |
|		    of the environment, to make sure non-ASCII characters     |
|		    are preserved.					      |
|		    							      |
|   History								      |
|    2022-11-29 JFL Created this routine                             	      |
|    2025-08-10 JFL Added a Windows-specific implementation.		      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_MSDOS)

/* Emulate the standard C library setenv(), based on Microsoft's _putenv() */
int setenv(const char *pszName, const char *pszValue, int iOverwrite) {
  int iErr = 0;

  DEBUG_ENTER(("setenv(\"%s\", \"%s\", %d)\n", pszName, pszValue, iOverwrite));

  if ((!pszName) || (!pszName[0]) || strchr(pszName, '=')) {
    errno = EINVAL;
    RETURN_CONST(-1);
  }

  if (iOverwrite || !getenv(pszName)) {
    /* Build the "VAR=VALUE" string expected by putenv() */
    size_t l = strlen(pszName);
    char *pszBuf = malloc(l+1+strlen(pszValue)+1);
    if (!pszBuf) RETURN_CONST(-1);
    strcpy(pszBuf, pszName);
    pszBuf[l] = '=';
    strcpy(pszBuf+l+1, pszValue);
    /* Update the environment */
    iErr = _putenv(pszBuf);
    /* Cleanup */
    free(pszBuf);
  }
  RETURN_INT(iErr);
}

#endif /* defined(_MSDOS) */

#if defined(_WIN32)

#include "iconv.h"

#include <windows.h>

/* Emulate the standard C library setenv(), based on Microsoft's _putenv() */
int setenvM(const char *pszName, const char *pszValue, int iOverwrite, UINT uCP) {
  int iErr = -1;

  DEBUG_ENTER(("setenv(\"%s\", \"%s\", %d)\n", pszName, pszValue, iOverwrite));

  if ((!pszName) || (!pszName[0]) || strchr(pszName, '=')) {
    errno = EINVAL;
    RETURN_CONST(-1);
  }

  if (iOverwrite || !getenv(pszName)) {
    WCHAR *pwszName = NULL;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszBuf = NULL;
    size_t l;
    /* Convert the name to a unicode string */
    pwszName = MultiByteToNewWideString(uCP, pszName);
    if (!pwszName) goto cleanup;
    /* Convert the value to a unicode string */
    pwszValue = MultiByteToNewWideString(uCP, pszValue);
    if (!pwszValue) goto cleanup;
    /* Build the "VAR=VALUE" string expected by putenv() */
    l = lstrlenW(pwszName);
    pwszBuf = malloc(sizeof(WCHAR)*(l+1+lstrlenW(pwszValue)+1));
    if (!pwszBuf) goto cleanup;
    lstrcpyW(pwszBuf, pwszName);
    pwszBuf[l] = L'=';
    lstrcpyW(pwszBuf+l+1, pwszValue);
    /* Update the environment */
    iErr = _wputenv(pwszBuf);
    /* Cleanup */
cleanup:
    free(pwszName);
    free(pwszValue);
    free(pwszBuf);
  } else {
    iErr = 0;
  }
  RETURN_INT(iErr);
}

#endif /* defined(_WIN32) */

