/*****************************************************************************\
*									      *
*   File name:	    filelen.c						      *
*									      *
*   Description:    Write into a file					      *
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
|   Function:	    filelength						      |
|									      |
|   Description:    Get the length of a file				      |
|									      |
|   Parameters:     int hf		The MS-DOS file handle		      |
|									      |
|   Returns:	    DX:AX = the file size				      |
|									      |
|   Notes:	    Standard C library routine. 			      |
|									      |
|   History:								      |
|									      |
|    1998/05/24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

long filelength(int hf)
    {
    long l0;
    long l;

    l0 = lseek(hf, 0, 1);    // Get the current offset
    l = lseek(hf, 0, 2);     // Get the file end offset
    lseek(hf, l0, 0);	     // Move back to the current offset

    return l;
    }
