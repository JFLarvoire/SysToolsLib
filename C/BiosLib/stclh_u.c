/*****************************************************************************\
*									      *
*   File name:	    stclh_u.c						      *
*									      *
*   Description:    Convert a string to an unsigned long integer	      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1997/10/06 JFL Extracted these routines from sprintf.c.		      *
*									      *
*      (c) Copyright 1988-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

//+--------------------------------------------------------------------------
//+ Function   : stch_i
//+
//+ Purpose    : Convert an hexadecimal string to an integer
//+
//+ Parameters : Pointer to the string to convert
//+              Pointer to the integer where to store the result
//+
//+ Return     : Number of input characters processed
//+
//+ Notes:     : This routine was part of the Lattice C library.
//+              It had to be rewritten from scratch when printf was ported
//+               from the Lattice to the Microsoft C compiler.
//+              The Lattice name and syntax have been retained for
//+               compatibility, even though they are not very friendly!
//+
//+              This code is tried and tested. It is not expected to change.
//+              This is why the variable names have not been updated to Pike
//+               coding standards.
//+
//+ Creation   : 04-Feb-1988 by Jean-François LARVOIRE
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 09-Mar-1993  JFL     Reformatted according to Pike coding standards
//+ 27-Mar-2001  JFL     Rewrote using Standard C library's strtoul().
//+			 Moved from stc.c to stclh_u.c.
//+
//+--------------------------------------------------------------------------

int stch_i(const char *string, int *pi)
    {
    char *pc;

    *pi = (int)strtoul(string, &pc, 16);

    return pc-string;
    }

/*---------------------------------------------------------------------------*\
*									      *
|   Function:	    stclh_u						      |
|									      |
|   Description:    Convert an hexadecimal string to an unsigned long integer |
|									      |
|   Parameters:     char *string	String to convert		      |
|		    unsigned long *pdw	Where to store the result	      |
|									      |
|   Returns:	    Number of input characters processed		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/03/04 JFL Created this routine, adapted from stch_i().	      |
|    2001/03/27 JFL Rewrote using Standard C library's strtoul().	      |
*									      *
\*---------------------------------------------------------------------------*/

int stclh_u(const char *string, unsigned long *pul)
    {
    char *pc;

    *pul = strtoul(string, &pc, 16);

    return pc-string;
    }
