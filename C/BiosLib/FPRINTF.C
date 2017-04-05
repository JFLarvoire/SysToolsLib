/*****************************************************************************\
*                                                                             *
*   Filename	    fprintf.c						      *
*									      *
*   Description:    A limited fprintf() for writing to the BIOS console	      *
*                                                                             *
*   Notes:	    							      *
*		    							      *
*   History:								      *
*    2015-12-10 JFL Created this module.				      *
*    2016-04-11 JFL Renamed putstr() as cputs().			      *
*                                                                             *
*      (c) Copyright 2015-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        fprintf	                                              |
|                                                                             |
|   Description     Redefinition of a C library routine			      |
|                                                                             |
|   Parameters      See a C library reference for the list of arguments       |
|                                                                             |
|   Returns         See a C library reference for the return value            |
|                                                                             |
|   Notes           See SPRINTF.C for the limited list of supported formats.  |
|                   The output must not be longer than 1024 bytes. Else crash!|
|                                                                             |
|   History 								      |
|    2015-12-10 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl fprintf(FILE *f, const char *format, ...) {
  char uneligne[1024];
  int n;

  n = sprintf1(uneligne, &format);
  cputs(uneligne);
  return n;
}
