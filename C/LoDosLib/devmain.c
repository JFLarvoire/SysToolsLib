/*****************************************************************************\
*                                                                             *
*   File name:	    devmain						      *
*									      *
*   Description:    Mini device driver initialization main routine.	      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1998/05/19 JFL Created this file					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "clibdef.h"
#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    devmain						      |
|									      |
|   Description:    Device driver main initialization routine		      |
|									      |
|   Parameters:     char far *fpParms	    Pointer to the argument line      |
|									      |
|   Returns:	    0 = Success. Stay resident. 			      |
|		    1 = Failure. Unload from memory.			      |
|									      |
|   History:								      |
|									      |
|    1995/08/22 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#define MAXARG 10

int _cdecl devmain(char far *fpParms)
    {
    int argc;
    char *argv[MAXARG];

    argc = BreakArgLine(fpParms, argv, MAXARG);

    return main(argc, argv);
    }
