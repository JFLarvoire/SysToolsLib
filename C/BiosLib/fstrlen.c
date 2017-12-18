/*****************************************************************************\
*									      *
*   File name:	    fstrlen.c						      *
*									      *
*   Description:    Get the length of a far string.			      *
*									      *
*   Notes:	    Microsoft C library routine. 			      *
*									      *
*   History:								      *
*    2002/07/15 JFL Created this file					      *
*									      *
*      (c) Copyright 2002-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _fstrlen						      |
|									      |
|   Purpose:	    Redefinition of a Microsoft C Library routine	      |
|									      |
|   Parameters:     Standard						      |
|									      |
|   Return:	    Standard						      |
|									      |
|   History:								      |
|    2002/07/15 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _fstrlen(char far *lpsz)
    {
    int i;
    
    for (i=0; lpsz[i]; i++) ;
    return i;
    }
    
