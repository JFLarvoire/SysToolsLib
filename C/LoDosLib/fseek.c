/*****************************************************************************\
*									      *
*   File name:	    fseek.c						      *
*									      *
*   Description:    Move the file pointer in the file			      *
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
|   Function:	    fseek						      |
|									      |
|   Description:    Move the file pointer in the file			      |
|									      |
|   Parameters:     FILE *hf		File handle			      |
|		    long lOffset	Relative displacement		      |
|		    int iOrigin 	Origin. 0=Begin, 1=Current, 2=End.    |
|									      |
|   Returns:	    0 = Success, else OS error. 			      |
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

int fseek(FILE *hf, long lOffset, int iOrigin)
    {
    long l;

    l = lseek(fileno(hf), lOffset, iOrigin);
    if (l < 0) return (int)(-l);    // MS-DOS error
    return 0;
    }
