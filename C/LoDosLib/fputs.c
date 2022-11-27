/*****************************************************************************\
*									      *
*   File name	    fputs.c						      *
*									      *
*   Description     Put one line into a file				      *
*									      *
*   Notes								      *
*									      *
*   History								      *
*    1998-05-24 JFL Created this file					      *
*    2022-11-24 JFL Rewrote using fputc(), to get \n -> \r\n translation.     *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"
#include "clibdef.h"		    // For string functions

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        fputs	                                              |
|									      |
|   Description     Write a string into a file				      |
|									      |
|   Parameters      const char *pszLine		String to write		      |
|		    FILE *hf			File handle		      |
|									      |
|   Returns	    >=0 if success, else EOF and set errno.		      |
|									      |
|   Notes	    Standard C library routine. 			      |
|									      |
|   History								      |
|    1998-05-24 JFL Created this routine.				      |
|    2022-11-24 JFL Rewrote using fputc(), to get \n -> \r\n translation.     |
*									      *
\*---------------------------------------------------------------------------*/

int fputs(const char *pszLine, FILE *hf) {
  char c;

  while (c = *pszLine++) fputc(c, hf); /* fputc() translates \n to \r\n */
  return 0;
}
