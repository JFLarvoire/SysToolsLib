/*****************************************************************************\
*									      *
*   File name:	    fopen.c						      *
*									      *
*   Description:    Open a file 					      *
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
|   Function:	    fopen						      |
|									      |
|   Description:    Open a file 					      |
|									      |
|   Parameters:     char *pszName	File name			      |
|		    char *pszMode	Access mode			      |
|									      |
|   Returns:	    FILE *hFile 	Handle of open file, or 0 if error.   |
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

FILE *fopen(char *pszName, char *pszMode)
    {
    FILE *hf;
    int iErr = 0;
    WORD wMode;
    WORD wAction;

    if (strchr(pszMode, 'r'))
	{
	wMode = 0;	    // Read
	wAction = 0x01;     // Don't create ; open
	iErr += 1;
	}

    if (strchr(pszMode, 'w'))
	{
	wMode = 1;	    // Write
	wAction = 0x12;     // Create ; Truncate
	iErr += 1;
	}

    if (strchr(pszMode, 'a'))
	{
	wMode = 1;	    // Write
	wAction = 0x11;     // Create ; Open
	iErr += 1;
	}

    if (iErr != 1) return 0;	    // One of the above 3 must be specified

    if (strchr(pszMode, '+'))	// Mode modifier
	{
	wMode = 2;	    // Read/Write
	}

    iErr = ExtendedOpen(pszName, wMode, 0, wAction, &(WORD)hf);
    if (iErr) return 0;

    return hf;
    }
