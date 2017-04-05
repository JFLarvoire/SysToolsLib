/*****************************************************************************\
*                                                                             *
*   File name	    puts.c						      *
*									      *
*   Description     Standard C library routine puts			      *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2015-10-27 JFL Created this module.				      *
*    2016-04-11 JFL Renamed putstr() as cputs().			      *
*                                                                             *
*      (c) Copyright 2015-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
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
|    2015-10-27 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int puts(const char *s) {
   cputs(s);		/* Use NoDosLib's cputs to output the string */
   putchar('\r');	/* Then output the CRLF */
   putchar('\n');

   return 0;
}
