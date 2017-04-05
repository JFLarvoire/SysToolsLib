/*****************************************************************************\
*                                                                             *
*   Filename:	    FCPUTS.C						      *
*									      *
*   Description:    Output a far string to the console			      *
*                                                                             *
*   Notes:	    Microsoft C library extension for console I/O.            *
*									      *
*   History:								      *
*    1993-10-06 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Renamed _fputstr() as _fcputs().			      *
*		    							      *
*      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fcputs						      |
|									      |
|   Description:    Output a far string to the console			      |
|									      |
|   Parameters:     The string to output				      |
|									      |
|   Returns:	    0 if succesfull.					      |
|									      |
|   Notes:	    Contrary to puts(), cputs() does not append a new line.   |
|		    							      |
|   History:								      |
|    1991-10-03 SLR Created routine fputstr().         			      |
|    1993-10-06 JFL Made sure all arguments and returns are far.	      |
|    2016-04-11 JFL Renamed _fputstr() as _fcputs().			      |
|		    Removed far from return type, as this is meaningless.     |
*									      *
\*---------------------------------------------------------------------------*/

int _fcputs(const char far *s) {
  char c;

  while (c = *s++) {
    if (c == '\n') putchar('\r');
    putchar(c);
  }

  return 0;
}
