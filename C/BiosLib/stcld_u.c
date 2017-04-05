/*****************************************************************************\
*									      *
*   File name:	    stcld_u.c						      *
*									      *
*   Description:    Convert a string to an unsigned long integer	      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    2000/07/25 JFL Adapted from stclh_u.c.				      *
*    2001/03/27 JFL Use Standard C library's strtoul().			      *
*									      *
*      (c) Copyright 2000-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*									      *
|   Function:	    stcld_u						      |
|									      |
|   Description:    Convert a decimal string to an unsigned long integer      |
|									      |
|   Parameters:     char *string	String to convert		      |
|		    unsigned long *pdw	Where to store the result	      |
|									      |
|   Returns:	    Number of input characters processed		      |
|									      |
|   Notes:	    Supports negative numbers.				      |
|									      |
|   History:								      |
|									      |
|    2000/07/24 JFL Created this routine, adapted from stclh_u() and stcd_i().|
*									      *
\*---------------------------------------------------------------------------*/

int stcld_u(const char *string, unsigned long *pul)
    {
    char *pc;

    *pul = strtoul(string, &pc, 10);

    return pc-string;
    }

