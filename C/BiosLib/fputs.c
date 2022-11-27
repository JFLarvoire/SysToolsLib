/*****************************************************************************\
*                                                                             *
*   Filename	    fputs.c						      *
*									      *
*   Description     A limited fputs() for writing to the BIOS console	      *
*                                                                             *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2022-11-24 JFL Created this module.				      *
*                                                                             *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        fputs	                                              |
|                                                                             |
|   Description     Write a string into a file				      |
|                                                                             |
|   Parameters      const char *pszLine		String to write		      |
|		    FILE *hf			File handle		      |
|                                                                             |
|   Returns	    >=0 if success, else EOF and set errno.		      |
|                                                                             |
|   Notes	    Standard C library routine. 			      |
|                                                                             |
|   History 								      |
|    2022-11-24 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int fputs(const char *pszString, FILE *f) {
  return cputs(pszString); /* BiosLib's cputs() translates \n to \r\n */
}
