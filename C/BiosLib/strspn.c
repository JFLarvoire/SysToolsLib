/*****************************************************************************\
*									      *
*   File name:	    strspn.c						      *
*									      *
*   Description:    Skip in a string all characters from a set of characters  *
*									      *
*   Notes:	    Standard C library routine. 			      *
*									      *
*   History:								      *
*    1998/05/25 JFL Created this file					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strspn						      |
|									      |
|   Purpose:	    Redefinition of a Standard C Library routine	      |
|									      |
|   Parameters:     Standard						      |
|									      |
|   Return:	    Standard						      |
|									      |
|   History:								      |
|    1995/10/25 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

size_t strspn(const char *string1, const char *string2)
    {
    const char *pc1;
    const char *pc2;
    char c1, c2;

    for (pc1 = string1; c1 = *pc1; pc1++)   /* Scan the first string */
	{
	int found;

	found = FALSE;
	for (pc2 = string2; c2 = *pc2; pc2++)	/* Scan the second string */
	    {
	    if (c1 == c2)
		{
		found = TRUE;
		break;
		}
	    }
	if (!found) break;	/* If c1 not in string2, we're done */
	}

    return pc1-string1;
    }
