/*****************************************************************************\
*									      *
*   File name:	    dosexec.c						      *
*									      *
*   Description:    Load and execute an application			      *
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
|   Function:	    _dos_exec						      |
|									      |
|   Description:    Call the MS-DOS function 4BH "exec".		      |
|									      |
|   Parameters:     char far *pszProgram	Program name		      |
|		    char far *lpszArguments	Argument string		      |
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
|    1996/09/24 JFL Created this routine				      |
|    2001/06/11 JFL Changed arguments to far pointers			      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int _cdecl _dos_exec(char far *lpszProgram, char far *lpszArguments)
    {
    struct
	{
	WORD wSegEnv;
	char far *lpszArgs;
	WORD wFcb1Offset;
	WORD wFcb1Segment;
	WORD wFcb2Offset;
	WORD wFcb2Segment;
	} ParmBlock;
    char szArgs[130];			    // 128 chars + \r and \0
    int n;

    _fstrncpy(szArgs+2, lpszArguments, sizeof(szArgs)-4);
    szArgs[sizeof(szArgs)-2] = '\0';
    n = strlen(szArgs+2);
    szArgs[0] = (char)(n+1);		    // Count the initial space
    szArgs[1] = ' ';			    // This is the initial space
    szArgs[n+2] = '\r'; 		    // Add a trailing CR
    szArgs[n+3] = '\0';

    ParmBlock.wSegEnv = 0;		    // Inherit caller's environment
    ParmBlock.lpszArgs = szArgs;
    ParmBlock.wFcb1Offset = 0x5C;	    // Inherit caller's FCBs
    ParmBlock.wFcb1Segment = _psp;
    ParmBlock.wFcb2Offset = 0x6C;
    ParmBlock.wFcb2Segment = _psp;

    _asm
	{
	push	ds
	lds	dx, lpszProgram		    // DS:DX = Program
	push	ss
	pop	es
	lea	bx, ParmBlock		    // ES:BX = Parameter block
	mov	ax, 4B00H
	int	21H
	jc	error
	xor	ax, ax
error:
	pop	ds
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)

#pragma warning(default:4704)	// Ignore the inline assembler etc... warning
