/*****************************************************************************\
*                                                                             *
*   Filename:	    CPUTS.C						      *
*									      *
*   Description:    Output a string to the console			      *
*                                                                             *
*   Notes:	    Microsoft C library extension for console I/O.            *
*									      *
*   History:								      *
*    1987       JFL Created routine putstr().          			      *
*    1993-10-06 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Renamed putstr() as cputs().			      *
*		    							      *
*      (c) Copyright 1987-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    cputs						      |
|									      |
|   Description:    Output a string to the console			      |
|									      |
|   Parameters:     The string to output				      |
|									      |
|   Returns:	    0 if succesfull.					      |
|									      |
|   Notes:	    Contrary to puts(), cputs() does not append a new line.   |
|		    							      |
|   History:								      |
|    1987       JFL Created routine putstr().          			      |
*    2016-04-11 JFL Renamed putstr() as cputs().			      *
*									      *
\*---------------------------------------------------------------------------*/

int cputs(const char *s) {
  char c;

  while (c = *s++) {
    if (c == '\n') putchar('\r');
    putchar(c);
  }

  return 0;
}
