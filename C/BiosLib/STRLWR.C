/*****************************************************************************\
*									      *
*   File name:	    STRLWR.C						      *
*									      *
*   Description:    C library routines for use in an environment where the    *
*		     standard libraries can't be used.  		      *
*									      *
*   Notes:	        						      *
*									      *
*   History:								      *
*    1993/10/06 JFL Separated this file from CLIBC.C.			      *
*									      *
*      (c) Copyright 1993-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

//+--------------------------------------------------------------------------
//+ Function   : strlwr
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+                                                 
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : 
//+
//+ Creation   : 30-Jul-1993 by JFL
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 06-Oct-1993  JFL     Updated to use the tolower macro
//+ 08-Mar-1994  JFL     Oops. Fixed a bug introduced by the above update.
//+
//+--------------------------------------------------------------------------

char *strlwr(char *s)
{
   char c;
   char *s0;

   s0 = s;
   while (c = *s)
   {
      *s = tolower(c);
      s += 1;
   }
   return s0;
}
