/*****************************************************************************\
*									      *
*   File name:	    fgetc.c						      *
*									      *
*   Description:    Get one character from a file			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1998/05/24 JFL Created this file					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    fgetc						      |
|									      |
|   Description:    Get a character from a file 			      |
|									      |
|   Parameters:     FILE *hf		File handle			      |
|									      |
|   Returns:	    The character read, cast as an int. 		      |
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

int fgetc(FILE *hf)
    {
    int i = 0;

    if (!fread(&i, 1, 1, hf)) return EOF;
    return i;
    }
