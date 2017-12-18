/*****************************************************************************\
*                                                                             *
*   Filename:	    FSTRCMP.C						      *
*									      *
*   Description:    Compare two far strings				      *
*                                                                             *
*   Notes:	                                                              *
*									      *
*   History:								      *
*    1993-10-06 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Removed far from return type, as this is meaningless.     *
*		    							      *
*      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fstrcmp						      |
|									      |
|   Description:    Compare two far strings				      |
|									      |
|   Parameters:     See a C library reference for the list of arguments	      |
|									      |
|   Returns:	    See a C library reference for the return value	      |
|									      |
|   Notes:	    							      |
|		    							      |
|   History:								      |
|    1991-10-03 SLR Created routine fputstr().         			      |
|    1993-10-06 JFL Made sure all arguments and returns are far.	      |
|		    Rewritten to remove the 100 characters limit.	      |
|    2016-04-11 JFL Removed far from return type, as this is meaningless.     |
*									      *
\*---------------------------------------------------------------------------*/

int _fstrcmp(const char far *str1, const char far * str2) {
  signed char c1, c2, dif;

  do {
    c1 = (signed char)*(str1++);
    c2 = (signed char)*(str2++);
    dif = c1-c2;
  } while (c1 && c2 && !dif);

  return dif;
}
