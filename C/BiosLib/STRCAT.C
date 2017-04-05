/*****************************************************************************\
*									      *
*   File name:	    STRCAT.C						      *
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
//+ Function   : strcat
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : 
//+
//+ Creation   : 17-Oct-1991 by LTQ
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

char  *strcat(char  *d, const char  *s)
{
   int i,j;

   i = 0;
   j = 0;

   /* find the end of the destination string */

   while (d[i] != '\0') i++;

   /* Copy up to the NUL */

   while (s[j] != '\0') d[i++] = s[j++];
   d[i] = '\0';

   return d;
}
