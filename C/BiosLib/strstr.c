/*****************************************************************************\
*									      *
*   File name:	    strstr.c						      *
*									      *
*   Description:    Find a substring in a string			      *
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
|   Function:	    strstr						      |
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

char *strstr(const char *string1, const char *string2)
    {
    const char *pc;
    int n;

    n = strlen(string2);
    for (pc = string1; *pc; pc++)   /* Scan the first string */
	{
	if (!strncmp(pc, string2, n)) return (char *)pc;
	}

    return NULL;
    }
