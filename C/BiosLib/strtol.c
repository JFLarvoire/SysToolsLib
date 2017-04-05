/*****************************************************************************\
*									      *
*   File name:	    strtol.c						      *
*									      *
*   Description:    Convert a string to a long integer			      *
*									      *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2001/03/23 JFL Created this file					      *
*									      *
*      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strtol						      |
|									      |
|   Description:    Convert a string to a long integer			      |
|									      |
|   Parameters:     const char *pszString	The string to convert	      |
|		    char **ppszEnd		Where the scan stopped	      |
|		    int iBase			The conversion base 8/10/16   |
|									      |
|   Returns:	    long converted value				      |
|									      |
|   Notes:	    Does not manage overflows, contrary to C standard lib's.  |
|									      |
|   History:								      |
|    2001/03/23 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

long strtol(const char *pszString, char **ppszEnd, int iBase)
    {
    int iSign = FALSE;
    long l;

    while (*pszString == ' ') pszString++;	// Skip heading spaces.

    if (*pszString == '-')
	{
	pszString++;
	iSign = TRUE;
	}

    l = (long)strtoul(pszString, ppszEnd, iBase);

    if (iSign) l = -l;

    return l;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strtoul						      |
|									      |
|   Description:    Convert a string to an unsigned long integer	      |
|									      |
|   Parameters:     const char *pszString	The string to convert	      |
|		    char **ppszEnd		Where the scan stopped	      |
|		    int iBase			The conversion base 8/10/16   |
|									      |
|   Returns:	    unsigned long converted value			      |
|									      |
|   Notes:	    Differences with standard C library:		      |
|		    - Does not manage overflows				      |
|		    - Does not detect base if iBase == 0		      |
|		    - Does not support bases > 16.			      |
|									      |
|   History:								      |
|    2001/03/23 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

static char hex[16] = "0123456789ABCDEF";

unsigned long strtoul(const char *pszString, char **ppszEnd, int iBase)
    {
    unsigned long ul;
    int iDigit;

    for (ul = 0; TRUE; pszString++)
        {
	char c = *pszString;
	if (c >= 'a') c -= ('a'-'A');	// Convert to upper case
	for (iDigit=0; iDigit<iBase; iDigit++) if (c==hex[iDigit]) break;
	if (iDigit >= iBase)
            {
            if (ppszEnd) *ppszEnd = (char *)pszString;
            return ul;
            }
        ul *= iBase;
	ul += iDigit;
        }
    }
