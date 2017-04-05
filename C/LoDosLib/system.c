/*****************************************************************************\
*									      *
*   File name:	    system.c						      *
*									      *
*   Description:    Invoke the command interpretor.			      *
*									      *
*   Notes:	    							      *
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
|   Function:	    system						      |
|									      |
|   Description:    Invoke the command interpretor.			      |
|									      |
|   Parameters:     char *pszArguments		Argument string		      |
|									      |
|   Returns:	    AX = MS-DOS error code				      |
|			    0 = Success 				      |
|			    1 = Invalid function			      |
|			    2 = Program not found			      |
|			    3 = Path not found				      |
|			    4 = Too many open files			      |
|			    5 = Access denied				      |
|			    8 = Not enough memory			      |
|			   10 = Bad environment 			      |
|			   11 = Invalid EXE file structure		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    2001/06/11 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int system(char *pszCommand)	// Command to pass to the command interpreter
    {
    char far *lpszComspec = _fgetenv("COMSPEC");// Get the command interpreter pathname
    
    return _dos_exec(lpszComspec, pszCommand);	// Run the command
    }

