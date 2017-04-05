/*****************************************************************************\
*                                                                             *
*   Filename:	    CMAIN.C						      *
*									      *
*   Description:    Preprocess the command line arguments and call main       *
*                                                                             *
*   Notes:	    Defines the subset of the standard C library routines     *
*									      *
*   History:								      *
*    1995-04-07 JFL Created this file.					      *
*    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
*		    							      *
*      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
				     definition there */
#include "utildef.h"

#define MAXARGS 20	// Maximum number of arguments supported

extern _cdecl main(int argc, char *argv[]);

//+--------------------------------------------------------------------------
//+ Function   : cmain
//+
//+ Purpose    : Preprocess the command line arguments and call main
//+
//+ Parameters : None
//+
//+ Return     : Program return code for BIOS use
//+
//+ Notes:     :
//+
//+ Creation   : 1990 by Jean-François LARVOIRE, in RomSetup
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

int _cdecl cmain(void)
{
   int argc;
   char *argv[MAXARGS];

   // Do not attempt to recover the program name from the environment
   // into Argument 0, because this is complex, and is seldom needed.
   argv[0] = "C";   /* Store a dummy program name in Argument 0 */

   // Break the rest of the argument line
   argc = BreakArgLine(MAKEFP(_psp, ArgLine), argv+1, MAXARGS-1) + 1;

   return main(argc, argv);
}
