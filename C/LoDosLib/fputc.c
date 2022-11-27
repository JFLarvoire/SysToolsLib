/*****************************************************************************\
*									      *
*   File name	    fputc.c						      *
*									      *
*   Description     Put one character into a file			      *
*									      *
*   Notes								      *
*									      *
*   History								      *
*    2022-11-24 JFL Created this file					      *
*									      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    fputc						      |
|									      |
|   Description     Put a character into a file				      |
|									      |
|   Parameters      int c		The character to write		      |
|		    FILE *hf		File handle			      |
|									      |
|   Returns	    The character written, or EOF in case of error.	      |
|									      |
|   Notes	    Standard C library routine. 			      |
|									      |
|   History								      |
|    2022-11-24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int fputc(int c, FILE *hf) {
  if (c == '\n') fwrite("\r", 1, 1, hf);
  return fwrite((char *)&c, 1, 1, hf) ? c : EOF;
}
