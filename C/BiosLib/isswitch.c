/*****************************************************************************\
*									      *
*   File name:	    isswitch.c						      *
*									      *
*   Description:    Test if a command line argument is a switch.	      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1998-05-25 JFL Created this file					      *
*									      *
*      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|									      |
|   Description:    Test if a command line argument is a switch.	      |
|									      |
|   Parameters:     char *pszArg					      |
|									      |
|   Returns:	    TRUE or FALSE					      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1997-03-04 JFL Created this routine				      |
|    2022-11-30 JFL Changed to the _cdecl calling convention, to avoid a      |
|		    conflict with the eponym routine in SysLib.		      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl IsSwitch(char *pszArg)
    {
    switch (*pszArg)
	{
	case '-':
	case '/':
	    return TRUE;
	default:
	    return FALSE;
	}
    }
