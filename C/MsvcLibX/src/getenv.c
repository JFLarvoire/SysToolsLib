/*****************************************************************************\
*                                                                             *
*   Filename	    getenv.c						      *
*									      *
*   Description     Redefinitions of standard C library's getenv()	      *
*                                                                             *
*   Notes	    							      *
*                                                                             *
*   History								      *
*    2025-08-07 JFL Created this module.				      *
*                                                                             *
*		  © Copyright 2025 Jean-François Larvoire		      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define  _CRT_SECURE_NO_WARNINGS /* Prevent "This function or variable may be unsafe" warnings */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */

#if defined(_WIN32)

#include "iconv.h"	/* MsvcLibX encoding conversion routines */
#include "dict.h"	/* SysToolsLib Manage a dictionary */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    getenvM						      |
|									      |
|   Description	    Define a code-page dependant getenv() function            |
|									      |
|   Parameters	    char *pszName	Name of the variable to get	      |
|		    UINT uCP		The selected code page		      |
|		    							      |
|   Returns	    The string converted to the given code page, or NULL      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2025-08-07 JFL Created this routine                             	      |
|    2025-08-10 JFL Use a dictionary to avoid a memory leak.         	      |
*									      *
\*---------------------------------------------------------------------------*/

static dict_t *dict = NULL;

char *getenvM(const char *pszName, UINT uCP) {
  WCHAR *pwszName = NULL;
  WCHAR *pwszValue;
  char *pszTemp = NULL;
  char *pszValue = NULL;
  char *pszDefValue;

  DEBUG_ENTER(("getenvM(\"%s\", %u)\n", pszName, uCP));

  /* First check if it's already in the dictionnary */
  if (!dict) {	/* Create an empty dictionary */
    dict = NewDict();
  } else {	/* Search in the existing dictionary */
    pszValue = DictValue(dict, pszName);
    if (pszValue) RETURN_STRING(pszValue);
  }

  /* Convert the name to a unicode string */
  pwszName = MultiByteToNewWideString(uCP, pszName);
  if (!pwszName) RETURN_CONST(NULL);

  /* Get the Unicode value */
  pwszValue = _wgetenv(pwszName);
  if (!pwszValue) goto cleanup_and_exit;

  /* Convert the value to a new multibyte string */
  pszTemp = WideToNewMultiByteString(uCP, pwszValue);
  if (!pszTemp) goto cleanup_and_exit;

  /* Compare it to the default value */
#undef getenv /* Now on getenv() refers to Microsoft C Library's getenv() */
  pszDefValue = getenv(pszName);
  if (pszDefValue && !strcmp(pszDefValue, pszTemp)) {
    /* ASCII and Multibyte are the same. Use the default environment string */
    pszValue = pszDefValue;
  } else {
    /* Use the multibyte string we generated, which _is_ non ASCII */
    DEBUG_PRINTF(("# This is a non-ASCII string! Storing it in the dictionary.\n"));
    NewDictValue(dict, pszName, pszTemp);
    pszValue = pszTemp;
    pszTemp = NULL; /* Do not free the memory below */
  }

cleanup_and_exit:
  free(pwszName);
  free(pszTemp);
  RETURN_STRING(pszValue);
}

#endif /* defined(_WIN32) */
