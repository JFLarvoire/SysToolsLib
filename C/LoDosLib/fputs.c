/*****************************************************************************\
*									      *
*   File name:	    fputs.c						      *
*									      *
*   Description:    Put one line into a file				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1998/05/24 JFL Created this file					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"		    // For string functions
#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    fputs						      |
|									      |
|   Description:    Put a line into a file				      |
|									      |
|   Parameters:     char *pszLine	The line to write		      |
|		    FILE *hf		File handle			      |
|									      |
|   Returns:	    The number of characters written.			      |
|									      |
|   Notes:	    Standard C library routine. 			      |
|									      |
|   History:								      |
|									      |
|    1998/05/24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int fputs(char *pszLine, FILE *hf)
    {
    int iLen;

    iLen = strlen(pszLine);
    return fwrite(pszLine, 1, iLen, hf);
    }
