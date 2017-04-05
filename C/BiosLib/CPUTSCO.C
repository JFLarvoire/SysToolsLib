/*****************************************************************************\
*                                                                             *
*   Filename	    CPUTSCO.C						      *
*									      *
*   Description     Output a coloured string to the console		      *
*                                                                             *
*   Notes								      *
*									      *
*   History								      *
*    1990-11-11 JFL Created routine putstr_color().   			      *
*    1994-03-10 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Renamed putstr_color() as cputs_color().		      *
*		    							      *
*      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"    // Make sure our implementation matches the
                        //  definition there.

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    cputs_color						      |
|									      |
|   Description     Output a string on screen with a given color attribute    |
|									      |
|   Parameters      short color			Color attribute		      |
|		    const char *string)		String to display	      |
|									      |
|   Returns	    Nothing						      |
|									      |
|   Notes	    Contrary to puts(), cputs() does not append a new line.   |
|		    							      |
|   History								      |
|    1990-11-11 JFL Created routine putstr_color().    			      |
|    2016-04-11 JFL Renamed putstr_color() as cputs_color().		      |
*									      *
\*---------------------------------------------------------------------------*/

void cputs_color(
  short color,		// Color attribute
  const char *string	// String to display
) {
  while (*string) {
    if (*string == '\n') putch_color(color, '\r'); /* Convert LF to CRLF */
    putch_color(color, *string);
    string += 1;
  }
  return;
}
