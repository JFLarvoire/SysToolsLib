/*****************************************************************************\
*                                                                             *
*   Filename	    DISPLFLD.C						      *
*									      *
*   Description     Display a field anywhere on screen			      *
*                                                                             *
*   Notes								      *
*									      *
*   History								      *
*    1990-11-15 JFL Created routine display_field().   			      *
*    1994-03-10 JFL Separated this file from CLIBC.C.			      *
*    2016-04-11 JFL Renamed putstr_color() as cputs_color().		      *
*		    							      *
*      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"    // Make sure our implementation matches the
                        //  definition there.

//+--------------------------------------------------------------------------
//+ Function   : display_field
//+
//+ Purpose    : Display a field anywhere on screen
//+
//+ Parameters : See the function definition below
//+
//+ Return     : None
//+
//+ Notes:     :
//+
//+ Creation   : 15-Nov-1990 by Jean-François LARVOIRE, in CUTIL.C.
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

void display_field(
short col,           // Column of the first character (Normally 0 to 79)
short row,           // Row of the first character (Normally 0 to 24)
short len,           // Field length. Maximum: 80 characters.
const char *string,  // String to display
short color)         // Color attribute to use
{
   short i;
   char buffer[82];

   /* Left justify, length as specified */
   for (i = 0; (i < len) && (string[i]); i++) buffer[i] = string[i];
   for ( ; i < len; i++) buffer[i] = ' ';
   buffer[len] = '\0';

   gotoXY(col, row);
   cputs_color(color, buffer);

   gotoXY(0, 24);
   return;
}
