/*****************************************************************************\
*									      *
*   File name:	    atoi.c						      *
*									      *
*   Description:    Convert a decimal string to an integer		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/09/07 JFL Created this module.				      *
*		    Extracted stcd_i() from stc.c.			      *
*		    Added Standard C library's atoi().                        *
*									      *
*      (c) Copyright 1988-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    stcd_i						      |
|									      |
|   Description:    Convert a decimal string into an integer		      |
|									      |
|   Parameters:     char *string	Pointer to the string to convert      |
|		    int *pi		Where to store the result	      |
|									      |
|   Returns:	    Number of input characters processed		      |
|									      |
|   Notes:	    This routine was part of the Lattice C library.	      |
|		    It had to be rewritten from scratch when printf was       |
|		     ported from the Lattice to the Microsoft C compiler.     |
|		    The Lattice name and syntax have been retained for	      |
|		     compatibility, even though they are not very friendly!   |
|									      |
|   History:								      |
|    1988/02/04 JFL Created this routine				      |
|    1999/09/07 JFL Accept leading spaces in the input string.		      |
|    2001/03/27 JFL Use Standard C library's strtol().			      |
*									      *
\*---------------------------------------------------------------------------*/

int stcd_i(const char *pszString, int *pi)
    {
    char *pc;

    *pi = (int)strtol(pszString, &pc, 10);

    return pc-pszString;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    atoi						      |
|									      |
|   Description:    Convert a decimal string into an integer		      |
|									      |
|   Parameters:     char *pszString	    The decimal string to convert     |
|									      |
|   Returns:	    The corresponding integer				      |
|									      |
|   Notes:	    Standard C library routine				      |
|									      |
|   History:								      |
|									      |
|    1999/09/06 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int atoi(const char *pszString)
    {
    return (int)strtol(pszString, NULL, 10);
    }
