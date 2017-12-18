/*****************************************************************************\
*									      *
*   File name:	    getcwd.c						      *
*									      *
*   Description:    Get the current working directory			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/10/01 JFL Created this file					      *
*									      *
*      (c) Copyright 1999-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _getcwd						      |
|									      |
|   Description:    Get the durrent working directory			      |
|									      |
|   Parameters:     char *pszBuf	Where to store its name 	      |
|		    size_t iLen 	The buffer length		      |
|									      |
|   Returns:	    The buffer if success, or NULL is error		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1999/10/01 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler warning

char *_getcwd(char *pszBuf, int iLen)
    {
    char szLocal[FILENAME_MAX];
    char *pszRet = pszBuf;

    if (!pszRet) pszRet = malloc(FILENAME_MAX);

    _asm
	{
	lea	si, szLocal + 1
	xor	dl, dl
	mov	ah, 47H
	int	21H
	jnc	ok
	mov	pszRet, 0
ok:
	}

    if (pszRet)
	{
	szLocal[0] = '\\';
	strncpy(pszBuf, szLocal, iLen);
	pszBuf[iLen-1] = '\0';
	}

    return pszRet;
    }

#pragma warning(default:4704)	// Restore the inline assembler warning
