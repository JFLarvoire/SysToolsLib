/*****************************************************************************\
*									      *
*   File name:	    stc.c						      *
*									      *
*   Description:    Convert numbers to strings		 		      *
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
//+ Function   : stci_h
//+
//+ Purpose    : Convert an integer to an hexadecimal string
//+
//+ Parameters : Pointer to the buffer where to store the result string
//+              The integer to convert
//+
//+ Return     : Size of the output string
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
//+ 25-Aug-1997  JFL     Rewrote using the new routine stcli_h.
//+
//+--------------------------------------------------------------------------

int stci_h(char *string, int i)
    {
    return stcli_h(string, i & 0xFFFFL);
    }

//+--------------------------------------------------------------------------
//+ Function   : stci_d
//+
//+ Purpose    : Convert an integer to a decimal string
//+
//+ Parameters : Pointer to the buffer where to store the result string
//+       The integer to convert
//+
//+ Return     : Size of the output string
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
//+ 25-Aug-1997  JFL     Rewrote using the new routine stcli_d.
//+
//+--------------------------------------------------------------------------

int stci_d(char *string, int i)
    {
    return stcli_d(string, i);
    }

//+--------------------------------------------------------------------------
//+ Function   : stcu_d
//+
//+ Purpose    : Convert an unsigned integer to a decimal string
//+
//+ Parameters : Pointer to the buffer where to store the result string
//+              The integer to convert
//+
//+ Return     : Size of the output string
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
//+ 25-Aug-1997  JFL     Rewrote using the new routine stcli_d.
//+
//+--------------------------------------------------------------------------

int stcu_d(char *string, unsigned int u)
    {
    return stcli_d(string, u);
    }

/*---------------------------------------------------------------------------*\
*									      *
|   Function:	    stcli_h						      |
|									      |
|   Description:    Convert a long integer into an hexadecimal string         |
|									      |
|   Parameters:     char *string	Where to store the result	      |
|		    unsigned long l	The long integer to convert	      |
|									      |
|   Returns:	    The size of the output string			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/08/25 JFL Created this routine, adapted from stci_h().	      |
*									      *
\*---------------------------------------------------------------------------*/

static char hex[] = "0123456789ABCDEF";

int stcli_h(char *string, unsigned long l)
    {
    register int n = 0;
    int i;
    unsigned long u;
    char s[8];

    u = l;
    do
	{
	s[n++] = hex[u % 16];
	u /= 16;
	}
	while (u);

    i = n;
    do *(string++) = s[--i]; while (i);
    *string = '\0';

    return n;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    stcli_d						      |
|									      |
|   Description:    Convert a long integer into a string		      |
|									      |
|   Parameters:     char *string	Where to store the result	      |
|		    long l		The long integer to convert	      |
|									      |
|   Returns:	   The size of the output string			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/08/25 JFL Created this routine, adapted from stci_d().	      |
*									      *
\*---------------------------------------------------------------------------*/

int stcli_d(char *string, long l)
    {
    register int n = 0;
    register int j = 0;
    char s[12];

    if (l < 0)
	{
	*(string++) = '-';
	l = -l;
	n = 1;	      /* Count the sign */
	}

    do
	{
	s[j++] = hex[l % 10];
	l /= 10;
	}
    while (l);
    n += j;     /* Total number of characters */

    do *(string++) = s[--j]; while (j); /* Reverse and copy the string */
    *string = '\0';

    return n;
    }
