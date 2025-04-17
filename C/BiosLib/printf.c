/*****************************************************************************\
*                                                                             *
*   Filename	    PRINTF.C						      *
*									      *
*   Description     Redefinition of a standard C library routine	      *
*                                                                             *
*   Notes	    							      *
*		    							      *
*   History								      *
*    1993-10-06 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Renamed putstr() as cputs().			      *
*    2022-11-24 JFL Use fputs(), so that it can be overridden by LoDosLib's   *
*                   fputs() if desired.                                       *
*                                                                             *
*      (c) Copyright 1987-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */
                                         
//+--------------------------------------------------------------------------
//+ Function   : printf
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : See SPRINTF.C for the limited list of supported formats.
//+              The output must not be longer than 1024 bytes. Else crash!
//+
//+ Creation   : 1987 by Jean-Fran�ois LARVOIRE, in CLIBC.C.
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 15-Nov-1990  JFL     Added _cdecl keywords to printf & sprintf.
//+ 20-Dec-1990  JFL     Changed the printf buffer from 80 to 256 characters to
//+                       prevent problems with strings longer than one line.
//+ 09-Jul-1993  JFL     Changed the printf buffer to 1024 bytes. Same reason.
//+ 07-Oct-1993  JFL     Changed the limit from 4 to 6 arguments.
//+ 09-Mar-1994  JFL     Removed the limit on the # of arguments altogether.
//+ 2025-03-10   JFL     Rewritten to work with strings of any length.
//+
//+--------------------------------------------------------------------------

static void SprintChar(char *pszOutput, char c) {
  UNUSED_ARG(pszOutput);
  if (c == '\n') putchar('\r');
  if (c) putchar(c);
}

int _cdecl printf(const char *format, ...) {
  auto va_list vl;
  va_start(vl, format);
  return _vsnprintf1(SprintChar, NULL, 0, format, vl);
}

#if 0
int _cdecl printf(const char *format, ...) {
  char uneligne[128];
  char *pLigne;
  int n;

  n = sprintf1(NULL, &format);
  if (n < sizeof(uneligne)) {
    pLigne = uneLigne;
  } else {
    pLigne = malloc(n+1);
  }
  n = sprintf1(uneligne, &format);
  fputs(uneligne, stdout);
  if (pLigne != uneLigne) free(uneLigne);
  return n;
}
#endif
