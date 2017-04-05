/*****************************************************************************\
*									      *
*   File name:	    STRUPR.C						      *
*									      *
*   Description:    C library routines for use in an environment where the    *
*		     standard libraries can't be used.  		      *
*									      *
*   Notes:	        						      *
*									      *
*   History:								      *
*    1993/10/06 JFL Separated this file from CLIBC.C.			      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

//+--------------------------------------------------------------------------
//+ Function   : strupr
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     :
//+
//+ Creation   : 14-dec-1995 by JFL
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

char *strupr(char *s)
{
   char c;
   char *s0;

   s0 = s;
   while (c = *s)
   {
      *s = toupper(c);
      s += 1;
   }
   return s0;
}
