/*****************************************************************************\
*                                                                             *
*   Filename:	    printerr.c						      *
*									      *
*   Description:    Display a formatted WIN32 error message.		      *
*									      *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <windows.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MyAnsiToOem      					      |
|									      |
|   Description:    Convert a string from the ANSI characters to the OEM's.   |
|									      |
|   Parameters:     char *pszString	ANSI string 			      |
|									      |
|   Returns:	    0 = FAILED; Non 0 = Done.      			      |
|									      |
|   Notes:	    There's a AnsiToOem macro that does almost the same thing.|
|									      |
|   History:								      |
|									      |
|    1998/04/10 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int ConvertCodePage(char *pszString, int iOldCp, int iNewCp)
    {
    size_t len;
    PWSTR pwszName;
    int iDone;

    len = strlen(pszString);
    // oemprintf("Initial string %s\n", pszString);
    pwszName = (PWSTR)malloc(2*(len+1));
    iDone = MultiByteToWideChar(iOldCp, MB_PRECOMPOSED, pszString, len, pwszName, len+1);
    if (!iDone) PrintWin32Error("Failed to convert \"%s\" to wide chars.\n", pszString);
    iDone = WideCharToMultiByte(iNewCp, 0, pwszName, len, pszString, len+1, NULL, NULL);
    if (!iDone) PrintWin32Error("Failed to convert \"%s\" back to normal chars.\n", pszString);
    // oemprintf("Converted string %s\n", pszString);
    free(pwszName);

    return iDone;
    }

int MyAnsiToOem(char *pszString)
    {
    return ConvertCodePage(pszString, CP_ACP, CP_OEMCP);
    }

int MyOemToAnsi(char *pszString)
    {
    return ConvertCodePage(pszString, CP_OEMCP, CP_ACP);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    oemprintf						      |
|									      |
|   Description:    printf a line, converting it to the OEM charset first.    |
|									      |
|   Parameters:     Same as printf					      |
|									      |
|   Returns:	    Same as printf					      |
|									      |
|   Notes:	    Output limited to 1024 characters.			      |
|									      |
|   History:								      |
|									      |
|    1998/06/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl oemprintf(const char *pszFormat, ...)
    {
    char szLine[1024];
    va_list pArgs;
    int n;

    va_start(pArgs, pszFormat);
    n = vsprintf(szLine, pszFormat, pArgs);
    MyAnsiToOem(szLine);
    printf("%s", szLine);
    return n;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PrintWin32Error					      |
|									      |
|   Description:    Display an error message, and the last win32 error text.  |
|									      |
|   Parameters:     char *pszFormat	Format string			      |
|		    ... 		Optional parameters		      |
|									      |
|   Returns:	    The number of characters written			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/03/25 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int PrintWin32Error(char *pszFormat, ...)
    {
    DWORD dwError;
    va_list pArgs;
    LPSTR lpMsgBuf;
    int n;

    dwError = GetLastError();
    va_start(pArgs, pszFormat);
    n = vprintf(pszFormat, pArgs);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,
		  dwError,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		  (LPTSTR)&lpMsgBuf, // Address of the allocated buffer address
		  0,
		  NULL
		 );
    n += oemprintf("Error %08lX: %s\n", dwError, lpMsgBuf);
    LocalFree(lpMsgBuf);

    return n;
    }

