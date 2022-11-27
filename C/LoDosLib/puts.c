/*****************************************************************************\
*                                                                             *
*   File name	    puts.c						      *
*									      *
*   Description     Standard C library routine puts			      *
*									      *
*   Notes	    LoDosLib version, writing to DOS file handle 1	      *
*		    							      *
*   History								      *
*    2022-11-24 JFL Created this module.				      *
*                                                                             *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"            /* Make sure our implementation matches the
                                     definition there */
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    puts						      |
|									      |
|   Description     Standard C library routine puts			      |
|									      |
|   Parameters      See puts arguments in a C library reference		      |
|									      |
|   Returns	    See puts return value in a C library reference	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2022-11-24 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int puts(const char *pszLine) {
  int iRet = fputs(pszLine, stdout);
  if (iRet < 0) return iRet;
  return fputs("\r\n", stdout);
}
