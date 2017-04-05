/*****************************************************************************\
*									      *
*   File name:	    fread.c						      *
*									      *
*   Description:    Read from a file					      *
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
|   Function:	    fread						      |
|									      |
|   Description:    Read from a file					      |
|									      |
|   Parameters:     void *pBuf		Buffer where to store the data read   |
|		    int nBytes		Number of bytes per block	      |
|		    int nBlocks 	Number of blocks		      |
|		    FILE *hf		File handle			      |
|									      |
|   Returns:	    The number of blocks read.				      |
|									      |
|   Notes:	    Standard C library routine. 			      |
|									      |
|		    Uses lodoslib's FILE * alias for the MS-DOS file handle.  |
|		    This simplifies a lot the implementation of the functions |
|		    we support. 					      |
|									      |
|		    Files are always opened in binary mode.		      |
|									      |
|   History:								      |
|									      |
|    1998/05/24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

size_t fread(void *pBuf, size_t nBytes, size_t nCount, FILE *hf)
    {
    WORD wDone;
    WORD wSize;
    int iErr;

    wSize = (WORD)(nBytes * nCount);	// Incorrect result if >64K

    iErr = _dos_read(fileno(hf), pBuf, wSize, &wDone);
    if (iErr) return 0;

    return wDone / nBytes;
    }
