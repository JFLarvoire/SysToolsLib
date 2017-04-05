/*****************************************************************************\
*									      *
*   File name:	    BREAKARG.C  					      *
*									      *
*   Description:    Extract C arguments from the DOS command line	      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/04/07 JFL Created this file.  				      *
*									      *
*      (c) Copyright 1993-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"		/* Make sure our implementation matches the
				     definition there */

/* These two arrays should be allocated in the BSS segment,
    and not take any room in the executable program */

char ArgLineCopy[ARGLINESIZE];	/* Unmodified copy of the arguments line */
char Arguments[ARGLINESIZE];	/* Exploded NUL-terminated arguments */

/* ============================================================================
=                                                                             =
=  FUNCTION NAME:       BreakArgLine                                          =
=                                                                             =
=  DESCRIPTION:         Break the DOS command line into standard C arguments  =
=                                                                             =
=  INPUT PARAMETERS:                                                          =
=       char far *fpParms   Far pointer to the DOS parameter line             =
=	char *pszArg[]	    Address of an array of arguments pointers	      =
=	int iMaxArgs	    Number of elements of the pszArg array	      =
=                                                                             =
=  RETURN VALUE:                                                              =
=       int argc            Number of arguments found                         =
=                                                                             =
=  NOTES:   Makes a local copy of the command line string.		      =
=	    This is useful for EXE files and device drivers where the	      =
=	    argument line is not in the DGROUP segment. 		      =
=									      =
=	    Device drivers argument lines begin with the driver names.	      =
=	    COM and EXE programs argument lines don't. The program name is    =
=	     in the environment segment.				      =
=	    It is the responsability of the routine calling BreakArgLine() to =
=	     make sure the first actual argument goes to argv[1].	      =
=									      =
=  MODIFICATION HISTORY NOTES:						      =
=                                                                             =
=   JFL 93/10/05 Initial implementation within devmain().                     =
=   JFL 94/04/14 Extracted from devmain, and created this routine.            =
=   JFL 95/04/07 Extracted from llkinit.c.				      =
=                                                                             =
============================================================================ */

int BreakArgLine(char far *fpParms, char *pszArg[], int iMaxArgs)
    {
    int i;
    int argc;
    char c, c0;

    /* Get a local copy of the command line string */

    for (i=0; i<(ARGLINESIZE-1); i++)
        {
	ArgLineCopy[i] = c = *(fpParms++);
        if ((c == '\r') || (c == '\n'))     /* DOS ends line with a CR or LF */
            break;
        }
    ArgLineCopy[i] = '\0';		    /* C ends strings with a NUL */

    /* Break down the command line into standard C arguments */

    for (i=0, argc=0, c0='\0'; argc<iMaxArgs; i++)
        {
	c = ArgLineCopy[i];
        if ((c == ' ') || (c == '\t')) c = '\0';
	Arguments[i] = c;
	if (!ArgLineCopy[i]) break;
        if (c && !c0)
            {
	    pszArg[argc] = Arguments+i;
            argc += 1;
            }
        c0 = c;
        }

    return argc;
    }
