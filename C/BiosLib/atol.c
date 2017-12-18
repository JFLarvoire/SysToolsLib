/*****************************************************************************\
*									      *
*   File name:	    atol.c						      *
*									      *
*   Description:    Convert a decimal string to a long integer		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    2000/09/07 JFL Created this module.				      *
*		    Extracted stcd_i() from stc.c.			      *
*		    Added Standard C library's atoi().                        *
*									      *
*      (c) Copyright 1999-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    atol						      |
|									      |
|   Description:    Convert a decimal string into an integer		      |
|									      |
|   Parameters:     char *pszString	    The decimal string to convert     |
|									      |
|   Returns:	    The corresponding long integer			      |
|									      |
|   Notes:	    Standard C library routine				      |
|									      |
|   History:								      |
|									      |
|    1999/09/06 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

long atol(const char *pszString)
    {
    return strtol(pszString, NULL, 10);
    }

