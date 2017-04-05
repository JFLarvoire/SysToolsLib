/*****************************************************************************\
*									      *
*   File name:	    fgets.c						      *
*									      *
*   Description:    Get one line from a file				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1998/05/24 JFL Created this file					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    fgets						      |
|									      |
|   Description:    Get a line from a file				      |
|									      |
|   Parameters:     char *pszLine	Where to store the line read	      |
|		    int iSize		Size of the above buffer	      |
|		    FILE *hf		File handle			      |
|									      |
|   Returns:	    pszLine if success, or NULL if failure.		      |
|									      |
|   Notes:	    Standard C library routine. 			      |
|									      |
|		    Uses lodoslib's FILE * alias for the MS-DOS file handle.  |
|		    This simplifies a lot the implementation of the functions |
|		    we support. 					      |
|									      |
|   History:								      |
|									      |
|    1998/05/24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

char *fgets(char *pszLine, int iSize, FILE *hf)
    {
    int i;
    int ic;

    for (i=0; i<(iSize-1); )
	{
	ic = fgetc(hf);
	if (ic == EOF)
	    {
	    pszLine[i] = '\0';
	    return i ? pszLine : NULL;
	    }
	pszLine[i++] = (char)ic;
	if ((char)ic == '\n') break;
	}
    pszLine[i] = '\0';
    return pszLine;
    }
