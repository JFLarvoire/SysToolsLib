/*****************************************************************************\
*									      *
*   File name:	    getenv.c						      *
*									      *
*   Description:    Get an environment string				      *
*									      *
*   Notes:	    Warning: This creates a local copy of the string 	      *
*		    on every invokation. POTENTIAL MEMORY LEAK!!!	      *
*		    Use _fgetenv() instead.				      *
*									      *
*   History:								      *
*    2001/06/11 JFL Created this file					      *
*									      *
*      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"		    // For string functions
#include "lodos.h"		    // For _dos_exec prototype

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    getenv						      |
|									      |
|   Description:    Get and environment string 				      |
|									      |
|   Parameters:     char *pszName					      |
|									      |
|   Returns:	    A local copy of the string, or NULL is not found.	      |
|									      |
|   History:								      |
|    2001/04/13 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

char *getenv(char *pszName)
    {
    char far *lpValue;
    char *pValue;
    int iLen;

    lpValue = _fgetenv(pszName);
    if (!lpValue) return NULL;

    for (iLen=0; lpValue[iLen]; iLen++) ;	// Get the string length
    iLen += 1;					// Buffer size
    pValue = malloc(iLen);
    if (!pValue) return NULL;
    _fmemcpy(pValue, lpValue, iLen);		// Make a local copy

    return pValue;
    }

