/*****************************************************************************\
*                                                                             *
*   File name:	    setmenv.c						      *
*									      *
*   Description:    Set Master Environment Variable			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/06/29 JFL Created this file					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Definitions */

#include "clibdef.h"
#include "lodos.h"

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    SetMasterEnv					      |
|									      |
|   Description:    Set an environment variable in COMMAND.COM master envir.  |
|									      |
|   Parameters:     char *pszEnv	Environment variable name	      |
|		    char *pszValue	Its new value. If NULL or "", zap it! |
|									      |
|   Returns:	    0=Done; 1=Out of environment space. 		      |
|									      |
|   Notes:	    Function setenv() is useless here, as it modifies the     |
|		    local copy of the environment!			      |
|									      |
|   History:								      |
|									      |
|     1998/10/14 JFL Initial implementation.				      |
|     1998/10/20 JFL Write directly to the master environment.		      |
|     1999/02/12 JFL Added the ability to delete environment variables.       |
|     1999/02/15 JFL MS-DOS 7 does NOT change name to upper case.	      |
|     1999/06/29 JFL Fixed bug: Arena size is in paragraphs, not bytes.       |
|		     Also size read from wrong arena, causing crash under NT! |
*									      *
\*---------------------------------------------------------------------------*/

#define FARPTR(seg, off) ((void far *)(((DWORD)(WORD)(seg) << 16) | (WORD)off))
#define FARWORD(seg, off) (*(WORD far *)FARPTR(seg, off))
#define WORD1(dw) (*((WORD *)(&(dw))+1))

int SetMasterEnv(char *pszName, char *pszValue)
    {
    WORD wPsp;		// PSP segment
    WORD wParentPsp;	// Parent process PSP segment
    WORD wEnv;		// Environment segment
    char far *lpEnv;	// Environment base address
    WORD wEnvSize;	// Environment size
    char far *lpName;	// Current variable name
    char far *lpc;
    char c;

    // ~~jfl 1999/02/15 Under MS-DOS 7, lower case variable names are possible.
    // strupr(pszName);    // MS-DOS SET command changes variable to upper case.

    // Walk up the parent chain until command.com, which is its own parent.
    for (wPsp = GetPsp();	// The Program Segment Prefix for this program
	 wPsp != (wParentPsp = FARWORD(wPsp, 0x16));
	 wPsp = wParentPsp)	// Walk up our parent process' PSP
	 {
#if NEEDED
	 if (iVerbose) printf("PSP=%04X, Parent at %04X\n", wPsp, wParentPsp);
#endif
	 }

    // Get command.com environment
    wEnv = FARWORD(wPsp, 0x2C);
    lpEnv = FARPTR(wEnv, 0);
    // ~~jfl 1999/06/29. Convert paras to bytes.
    wEnvSize = FARWORD(WORD1(lpEnv)-1, 3) << 4;
#if NEEDED
    if (iVerbose) printf("Environment at %lp, size %X\n", lpEnv, wEnvSize);
#endif
    // Scan the environment for our variable
    for (lpName=lpEnv; *lpName; )
	{
	char *pcn;

	for (lpc = lpName, pcn = pszName; *pcn; lpc++, pcn++)
	    {
	    if (*lpc != *pcn) break;
	    }

	if ((!*pcn) && (*lpc == '='))
	    {
	    lpc += 1; // lpc = environment string value
	    break;    // Found!
	    }

	while (*lpName) lpName++;   // Skip that environment string
	lpName++;		    // Skip the trailing NUL
	}
#if NEEDED
    if (iVerbose)
	{
	char string[100];
	_fstrncpy(string, lpName, sizeof(string));
	if (string[0])
	    printf("Found \"%s\"\n", string);
	else
	    printf("%s not found.\n", pszName);
	}
#endif

    // Move up the rest of the environment
    if (*lpName)
	{
	char far *lpNextName;
	for (lpc=lpName; *lpc; lpc++) ;
	lpNextName = lpc + 1;
	while (*lpNextName)
	    {
	    for (lpc=lpNextName; c=*lpc; lpc++) *(lpName++) = c;
	    *(lpName++) = '\0';
	    lpNextName = lpc+1;
	    }
	}
    *lpName = '\0';		    // Temporary end of list
    if (!pszValue || !pszValue[0]) return 0;	// Just delete the string

    // Append our string
    wEnvSize -= (lpName-lpEnv);
    if (wEnvSize < (WORD)(strlen(pszName)+strlen(pszValue)+3))
	{
#if NEEDED
	if (iVerbose) printf("Out of environment space\n");
#endif
	return 1;	   // Out of environment space
	}
    while (*(lpName++)=*(pszName++));	// Copy the name and trailing NUL
    *(lpName-1) = '=';			// Overwrite the previous NUL
    while (*(lpName++)=*(pszValue++));	// Copy the value and trailing NUL
    *(lpName++) = '\0'; 		// Append the final NUL

#if NEEDED
    if (iVerbose) printf("String \"%s=%s\" added.\n", pszName, pszValue);
#endif

    return 0;
    }
