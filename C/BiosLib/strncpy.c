/*****************************************************************************\
*									      *
*   File name:	    STRNCPY.C						      *
*									      *
*   Description:    C library routines for use in an environment where the    *
*		     standard libraries can't be used.  		      *
*									      *
*   Notes:	        						      *
*									      *
*   History:								      *
*    1993/10/06 JFL Separated this file from CLIBC.C.			      *
*									      *
*      (c) Copyright 1987-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

//+--------------------------------------------------------------------------
//+ Function   : strncpy
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : 
//+
//+ Creation   : 1987 by JFL
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 10-Mar-1994  JFL     Changed i type from int to size_t.
//+
//+--------------------------------------------------------------------------

char *strncpy(char *d, const char *s, size_t n)
{
   size_t i;

   /* Copy at most N characters, up to the NUL */
   for (i = 0; i < n; i++)
   {
      if (!(d[i] = s[i]) ) break;
   }
   /* Fill the remaining space with NULs if less than N characters copied */
   for (     ; i < n; i++)
   {
      d[i] = '\0';
   }

   return d;
}
