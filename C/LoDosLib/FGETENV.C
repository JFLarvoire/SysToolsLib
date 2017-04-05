/*****************************************************************************\
*									      *
*   File name:	    fgetenv.c						      *
*									      *
*   Description:    Get an environment string				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1996/09/26 JFL Created this file					      *
*									      *
*      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"		    // For string functions
#include "utildef.h"		    // For _psp
#include "lodos.h"		    // For _dos_exec prototype

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fgetenv						      |
|									      |
|   Description:    Get a string in the environment.			      |
|									      |
|   Parameters:     char *pszName	Environment string name 	      |
|									      |
|   Returns:	    DX:AX		Environment string value address      |
|									      |
|   Notes:	    Non-standard derivative of the standard getenv.	      |
|		    This is necessary because our startup module does not     |
|		    make a near copy of the environment segment.	      |
|									      |
|   History:								      |
|									      |
|    1996/10/02 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

char far *_fgetenv(char *pszName)
    {
    WORD far *lpw;
    char far *lpEnv;	// Address of the environment
    char far *lpc;	// Used to scan the environment
    char *pcn;		// Used to scan the string name


    WORD0(lpw) = 0x2C;
    WORD1(lpw) = _psp;

    WORD0(lpEnv) = 0;
    WORD1(lpEnv) = *lpw;

    while (*lpEnv)
	{
	for (lpc = lpEnv, pcn = pszName; *pcn; lpc++, pcn++)
	    {
	    if (*lpc != *pcn) break;
	    }

	if ((!*pcn) && (*lpc == '=')) return ++lpc; // Found!

	while (*lpEnv) lpEnv++;     // Skip that environment string
	lpEnv++;		    // Skip the trailing NUL
	}

    return NULL;
    }
