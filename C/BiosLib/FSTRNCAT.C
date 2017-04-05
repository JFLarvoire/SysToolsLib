/*****************************************************************************\
*									      *
*   File name:	    FSTRNCAT.C						      *
*									      *
*   Description:    C library routines for use in an environment where the    *
*		     standard libraries can't be used.  		      *
*									      *
*   Notes:	        						      *
*									      *
*   History:								      *
*    1993/10/06 JFL Separated this file from CLIBC.C.			      *
*									      *
*      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */

//+--------------------------------------------------------------------------
//+ Function   : _fstrncat
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : 
//+
//+ Creation   : 03-Oct-1991 by SLR
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 02-Mar-1992  LTQ     Fixed bug: Use to say ';j<n)'
//+ 06-Oct-1993  JFL     Made sure all arguments and returns are far.
//+ 10-Mar-1994  JFL     Changed i,j types from int to size_t.
//+
//+--------------------------------------------------------------------------

char far * far _fstrncat(char far *d, const char far *s, size_t n)
{
   size_t i,j;

   /* find the end of the destination string */

   for (j=0; d[j] != '\0'; j++) ;

   /* Copy at most N characters, up to the NUL */
   for (i = 0 ; i < n; i++,j++) /* LTQ 03/02/92 fixed bug, use to be j<n */
   {
      if (!(d[j] = s[i]) ) break;
   }
    /* Fill the remaining space with NULs if less than N characters copied */
   for (     ; j < n; j++)
   {
      d[j] = '\0';
   }

   return d;
}
