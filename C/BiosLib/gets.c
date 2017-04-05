/*****************************************************************************\
*									      *
*   File name:	    gets.c						      *
*									      *
*   Description:    Input a line from stdin				      *
*									      *
*   Notes:	    This version for NoDosLib inputs from the console.	      *
*									      *
*   History:								      *
*    2000/09/28 JFL Created this module.				      *
*									      *
*      (c) Copyright 2000-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    gets						      |
|									      |
|   Description:    Duplicate Standard C library's gets().		      |
|									      |
|   Parameters:     char *pszBuf	Where to store the input	      |
|									      |
|   Returns:	    pszBuf 						      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|     2000/09/28 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

char *gets(char *pszBuf)
    {
    char c;
    char *pc = pszBuf;
    
    while (TRUE)
        {
        c = getch();
        if (c=='\r') 
            {
            printf("\n");
            break;
            }
        if ((c=='\x08') && (pc>pszBuf))
            {
            printf("\x08 \x08");
            pc -= 1;
            continue;
            }
        putchar(c);
        *pc = c;
        pc += 1;
        }    
    *pc = '\0';
            
    return pszBuf;
    }

