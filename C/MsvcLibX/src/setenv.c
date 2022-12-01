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
*                                                                             *
*		  © Copyright 2022 Jean-François Larvoire		      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <stdlib.h>
#include <string.h>

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
|   Notes	    							      |
|		    							      |
|   History								      |
|    2022-11-29 JFL Created this routine                               	      |
*									      *
\*---------------------------------------------------------------------------*/

/* Emulate the standard C library setenv(), based on Microsoft's _putenv() */
int setenv(const char *pszName, const char *pszValue, int iOverwrite) {
  int iErr = 0;
  size_t l = strlen(pszName);
  char *pszBuf = malloc(l+1+strlen(pszValue)+1);
  if (!pszBuf) return -1;
  strcpy(pszBuf, pszName);
  pszBuf[l] = '=';
  strcpy(pszBuf+l+1, pszValue);
  if (iOverwrite || !getenv(pszName)) iErr = _putenv(pszBuf);
  free(pszBuf);
  return iErr;
}

