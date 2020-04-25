/*****************************************************************************\
*                                                                             *
*   Filename	    2clip.c						      *
*									      *
*   Contents	    Pipe the output of a console application to the clipboard *
*									      *
*   Notes:	    							      *
*									      *
*   History								      *
*    1997-05-09 JFL Adapted from Windows Developer's Journal sample.          *
*		    Changed variables names to hungarian convention.	      *
*		    Added loop to wait for the completion of the input task.  *
*		    Added command line analysis, for the /? argument.	      *
*		    Version 1.0.					      *
*    2004-08-11 JFL Added support for input sizes > 64 KB.		      *
*		    Version 1.1.					      *
*    2008-03-12 JFL Removed the complaint about an empty input buffer.        *
*		    Version 1.1a.					      *
*    2012-03-01 JFL Changed this to a console program.			      *
*		    Version 1.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 1.2.1.                 *
*    2014-03-31 JFL Added support for local code pages.                       *
*		    Added options -A, -O , -u, -U to override the default CP. *
*		    Version 1.3.					      *
*    2014-04-18 JFL Fixed a bug which prevented work in code page 1252.       *
*		    Version 1.3.1.					      *
*    2014-11-14 JFL Change NUL characters to spaces, to avoid truncating the  *
*		    clipboard data at the first NUL in the input stream.      *
*		    Removed the sz prefix in buffer variables names.	      *
*		    Version 1.3.2.					      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2017-12-05 JFL Added options -h and -r, for HTML and RTF. Version 1.4.   *
*    2018-01-08 JFL Remove the UTF8 BOM when writing RTF. Version 1.4.1.      *
*    2018-08-31 JFL Added the -d debug option. Version 1.4.2.		      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.1.4.3.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.4.4.      *
*    2019-10-31 JFL Added option -z to stop input on Ctrl-Z. Version 1.5.     *
*    2020-04-25 JFL Rewrote ReportWin32Error as PrintWin32Error(); Added new  *
*		    PrintCError(), and use them instead of COMPLAIN().        *
*		    Converted the source to UTF-8 and output help as Unicode. *
*		    Added option --lock in debug mode. Version 1.6.	      *
*                   							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Copy text from stdin to the Windows clipboard"
#define PROGRAM_NAME    "2clip"
#define PROGRAM_VERSION "1.6"
#define PROGRAM_DATE    "2020-04-25"

#define _UTF8_SOURCE	/* Tell MsvcLibX that this program generates UTF-8 output */

#ifndef _WIN32
#error "Unsuported OS. This program is Windows-specific."
#endif

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <conio.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */

#define BLOCKSIZE (4096)	// Number of characters that will be allocated in each loop.

/* Local definitions */

#define CP_NULL ((UINT)-1) /* No code page. Can't use 0 as CP_ACP is 0 */

/* Global variables */

DEBUG_GLOBALS

/* Function prototypes */

void usage(void);
int IsSwitch(char *pszArg);
int _cdecl PrintCError(char *pszExplanation, ...);
int _cdecl PrintWin32Error(char *pszExplanation, ...);
int ToClip(const char* pBuffer, size_t lBuf, UINT cf);
int ToClipW(const WCHAR* pwBuffer, size_t nWChars);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Program main routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   Notes:	    This routine is renamed main_, else WinMain would not be  |
|		    processed.						      |
|									      |
|   History:								      |
|									      |
|    2001-09-18 JFL Created this routine.				      |
|    2008-03-12 JFL Removed the complaint about an empty input buffer.        |
|    2014-03-31 JFL Added the conversion to Unicode.                          |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  size_t  nTotal = 0;		// Total number of characters read.
  char   *pBuffer;
  int i;
  UINT codepage;
  UINT type = 0;
  int iDone;
  int iRet;
  int isHTML = FALSE;
  int isRTF = FALSE;
  int iCtrlZ = FALSE;		/* If true, stop input on a Ctrl-Z */

  /* Record the console code page, to allow converting the output accordingly */
  codepage = GetConsoleOutputCP();

  /* Process arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	    /* It's a switch */
      if (streq(arg+1, "?")) {		/* -?: Help */
	usage();                            /* Display help */
	return 0;
      }
      if (streq(arg+1, "A")) {		/* Assume the input is ANSI text */
	codepage = CP_ACP;
	continue;
      }
      DEBUG_CODE(
      if (streq(arg+1, "d")) {		/* -d: Debug */
	iDebug = 1;
	continue;
      }
      )
      if (streq(arg+1, "h")) {		/* Register the input as HTML */
	type = RegisterClipboardFormat("HTML Format");
	isHTML = TRUE;
	continue;
      }
      DEBUG_CODE(
      if (streq(arg+1, "-lock")) {	/* --lock: Lock the clipboard, for testing access conflicts */
      	HWND hWND = FindWindow("ConsoleWindowClass", NULL);
	if (OpenClipboard(hWND)) {	  /* Give clipboard ownership to the console window */
	  printf("Clipboard locked. Press ESC to exit.\n");
	  while ((i = _getch()) != EOF) if (i == '\x1B') break;
	  CloseClipboard();
	  printf("Clipboard unlocked.\n");
	  exit(0);
	} else {
	  PrintWin32Error("Can't lock the clipboard");
	  exit(1);
	}
      }
      )
      if (streq(arg+1, "O")) {		/* Assume the input is OEM text */
	codepage = CP_OEMCP;
	continue;
      }
      if (streq(arg+1, "r")) {		/* Register the input as RTF */
	type = RegisterClipboardFormat("Rich Text Format");
	isRTF = TRUE;
	continue;
      }
      if (streq(arg+1, "u")) {		/* Assume the input is Unicode */
	codepage = CP_NULL;
	continue;
      }
      if (streq(arg+1, "U")) {		/* Assume the input is UTF-8 text */
	codepage = CP_UTF8;
	continue;
      }
      if (streq(arg+1, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      if (streq(arg+1, "z")) {		/* -z: Stop input on Ctrl-Z */
	iCtrlZ = TRUE;
	continue;
      }
      fprintf(stderr, PROGRAM_NAME ".exe: Unsupported switch %s ignored.\n", arg);
      continue;
    }
    fprintf(stderr, PROGRAM_NAME ".exe: Unexpected argument %s ignored.\n", arg);
  }

  /* Go for it */
  DEBUG_PRINTF(("The selected code page is %d\n", codepage));

  /* Force stdin to untranslated */
  _setmode( _fileno( stdin ), _O_BINARY );

  pBuffer = (char*)malloc(0);
  while (!feof(stdin)) {
    pBuffer = realloc(pBuffer, nTotal + BLOCKSIZE);
    if (!pBuffer) {
      PrintCError("Can't read all input");
      exit(1);
    }
    if (!iCtrlZ) {
      nTotal += fread(pBuffer+nTotal, 1, BLOCKSIZE, stdin);
    } else { /* Read characters 1 by 1, to avoid blocking if the EOF character is not on a BLOCKSIZE boundary */
      int nRead;
      for (nRead = 0; nRead < BLOCKSIZE; nRead++) {
	char c;
	if (!fread(&c, 1, 1, stdin)) break;
	if (c == '\x1A') break; /* We got a SUB <==> EOF character */
	pBuffer[nTotal+nRead] = c;
      }
      nTotal += nRead;
      if (nRead < BLOCKSIZE) break;
    }
    Sleep(0);	    // Release end of time-slice
  }
  if (nTotal > 0) {
    WCHAR *pwUnicodeBuf = (WCHAR *)pBuffer;
    if (type) {	/* This is a specific data type */
      char header[256];
      if (isHTML) { // https://msdn.microsoft.com/en-us/library/aa767917.aspx
      	char *pszFormat = "Version:0.9\r\nStartHTML:%d\r\nEndHTML:%d\r\nStartFragment:%d\r\nEndFragment:%d\r\n";
      	int nHead = 50; /* Initial estimate for the number of characters in the header. (After formatting!) */
      	char *pc;
      	int iFragment = 0;
      	int iEndFragment = (int)nTotal;
	if (((pc = strstr(pBuffer, "<body")) != NULL) && ((pc = strstr(pc, ">")) != NULL)) iFragment = (int)(pc+1-pBuffer);
	if ((pc = strstr(pBuffer, "</body")) != NULL) iEndFragment = (int)(pc-pBuffer);
      	for (i = 0; i != nHead; nHead = i) { // Repeat until the string length stabilizes
	  i = sprintf(header, pszFormat, nHead, nHead+nTotal, nHead+iFragment, nHead+iEndFragment);
	}
	pBuffer = realloc(pBuffer, nTotal + nHead);
	memmove(pBuffer+nHead, pBuffer, nTotal);
	memmove(pBuffer, header, nHead);
	nTotal += nHead;
      } else if (isRTF) { // We must remove the BOM, which confuses MS Word
      	if ((nTotal >= 3) && !strncmp(pBuffer, "\xEF\xBB\xBF", 3)) {
      	  pBuffer += 3;		// Skip the UTF8 BOM
      	  nTotal -= 3;
      	}
      }
      iDone = ToClip(pBuffer, nTotal, type);
    } else {	/* This is text. Convert it to Unicode to avoid codepage issues */
      if (codepage != CP_NULL) {
	pwUnicodeBuf = (WCHAR *)malloc(2*(nTotal));
	if (!pwUnicodeBuf) {
	  PrintCError("Can't convert the intput to Unicode");
	  exit(1);
	}
	nTotal = MultiByteToWideChar(codepage,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
				     0,			/* dwFlags, */
				     pBuffer,		/* lpMultiByteStr, */
				     (int)nTotal,		/* cbMultiByte, */
				     pwUnicodeBuf,	/* lpWideCharStr, */
				     (int)nTotal		/* cchWideChar, */
				     );
	if (!nTotal) {
	  PrintWin32Error("Can't convert the intput to Unicode");
	  exit(1);
	}
      } else {
	nTotal /= 2;	/* # of wide unicode characters */
      }
      iDone = ToClipW(pwUnicodeBuf, nTotal);
    }
    iRet = !iDone; /* Return exit code 0 is the copy was done; 1 if it was not */
  } else {
    /* This is actually not a bug: The input CAN be empty */
    iRet = 0;
  }

  return iRet;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:       usage                                                     |
|                                                                             |
|   Description:    Display a brief help for this program                     |
|                                                                             |
|   Parameters:     None                                                      |
|                                                                             |
|   Returns:        None                                                      |
|                                                                             |
|   Notes:	    Use tabulations in the help string to ensure alignement   |
|		    in the message box. 				      |
|                                                                             |
|   Updates:                                                                  |
|                                                                             |
|    1997-05-09 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

void usage(void) {
  UINT cpANSI = GetACP();
  UINT cpOEM = GetOEMCP();
  UINT cpCurrent = GetConsoleOutputCP();

  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
\n\
    <command> | " PROGRAM_NAME " [switches]\n\
\n\
Switches:\n\
  -?        Display this help screen\n\
  -A        Assume input is ANSI text (Code page %u)\n\
"
#ifdef _DEBUG
"\
  -d        Output debug information.\n\
"
#endif
"\
  -h        Register input as HTML\n\
"
#ifdef _DEBUG
"\
  --lock    Lock the clipboard, for testing access conflicts.\n\
"
#endif
"\
  -O        Assume input is OEM text (Code page %u)\n\
  -r        Register input as RTF\n\
  -u        Assume input is Unicode text\n\
  -U        Assume input is UTF-8 text (Code page 65001)\n\
  -V        Display the program version\n\
  -z        Stop input on a Ctrl-Z (aka. SUB or EOF) character\n\
\n\
Default input encoding: The current console code page (Code page %u).\n\
\n"
"Author: Jean-François Larvoire"
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, cpANSI, cpOEM, cpCurrent);

  return;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description     Test if a command line argument is a switch.	      |
|									      |
|   Parameters      char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
#if defined(_WIN32) || defined(_MSDOS)
    case '/':
#endif
      return (*(short*)pszArg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    PrintCError						      |
|									      |
|   Description     Print C library error messages with a consistent format   |
|									      |
|   Parameters      char *pszFormat	The error context, or NULL            |
|		    ...			Optional arguments		      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2020-04-25 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl PrintCError(char *pszFormat, ...) {
  va_list vl;
  int n;
  int e = errno; /* The initial C errno when this routine starts */

  n = fprintf(stderr, PROGRAM_NAME
#if defined(_MSDOS) || defined(_WIN32)
		      ".exe"
#endif
		      ": ");

  if (pszFormat) {
    va_start(vl, pszFormat);
    n += vfprintf(stderr, pszFormat, vl);
    n += fprintf(stderr, ". ");
    va_end(vl);
  }

  n += fprintf(stderr, "%s.\n", strerror(e));

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PrintWin32Error					      |
|                                                                             |
|   Description:    Display a message with the last error.		      |
|                   							      |
|   Parameters      char *pszFormat	The error context, or NULL            |
|		    ...			Optional arguments		      |
|                   							      |
|   Returns:	    The number of characters written.        		      |
|                   							      |
|   Notes:	    							      |
|                   							      |
|   History:								      |
|    1998-11-19 JFL Created routine ReportWin32Error.			      |
|    2005-06-10 JFL Added the message description, as formatted by the OS.    |
|    2010-10-08 JFL Adapted to a console application, output to stderr.       |
|    2018-11-02 JFL Allow pszExplanation to be NULL.			      |
|                   Make sure WIN32 msg and explanation are on the same line. |
|    2019-09-27 JFL Fixed a formatting error.                                 |
|                   Prepend the program name to the output.		      |
|    2020-04-25 JFL Redesigned to avoid using a fixed size buffer.	      |
|                   Reordered the output so that it sounds more natural.      |
|                   Renamed as PrintWin32Error.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl PrintWin32Error(char *pszFormat, ...) {
  va_list vl;
  int n;
  char FAR *lpMsgBuf;
  DWORD dwErr = GetLastError(); /* The initial WIN32 error when this routine starts */

  n = fprintf(stderr, PROGRAM_NAME
#if defined(_MSDOS) || defined(_WIN32)
		      ".exe"
#endif
		      ": ");

  if (pszFormat) {
    va_start(vl, pszFormat);
    n += vfprintf(stderr, pszFormat, vl);
    n += fprintf(stderr, ". ");
    va_end(vl);
  }

  if (FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dwErr,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL )) { // Display both the error code and the description string.
    int l = lstrlen(lpMsgBuf);
    // Remove the trailing new line and dot, if any.
    if (l && (lpMsgBuf[l-1] == '\n')) lpMsgBuf[--l] = '\0';
    if (l && (lpMsgBuf[l-1] == '\r')) lpMsgBuf[--l] = '\0';
    if (l && (lpMsgBuf[l-1] == '.')) lpMsgBuf[--l] = '\0';
    n += fprintf(stderr, "%s.\n", lpMsgBuf);
    LocalFree(lpMsgBuf); // Free the buffer.
  } else { // Error, we did not find a description string for this error code.
    n += fprintf(stderr, "Win32 error %lu.\n", dwErr);
  }

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ToClip						      |
|									      |
|   Description:    Copy a buffer of text to the clipboard		      |
|									      |
|   Parameters:     const char *pBuffer     The data buffer to copy	      |
|		    size_t lBuf		    The number of bytes to copy       |
|		    UINT cf		    Clipboard Format. Dflt: CF_TEXT   |
|									      |
|   Returns:	    TRUE if success, or FALSE if FAILURE.		      |
|									      |
|   History:								      |
|    1997-05-09 JFL Adapted from Windows Developer's Journal sample           |
|    2004-08-11 JFL Fixed a bug that caused the last character to be lost.    |
|    2014-11-14 JFL Change NUL characters to spaces, to avoid truncating the  |
|		    clipboard data at the first NUL in the input stream.      |
|    2017-12-05 JFL Added the cf argument.                                    |
*									      *
\*---------------------------------------------------------------------------*/

int ToClip(const char* pBuffer, size_t lBuf, UINT cf)
    {
    int iResult = FALSE;	/* assume failure */
    if (!cf) cf = CF_TEXT;	/* If not set, use the default Clipboard Format */

    if (OpenClipboard(NULL))
        {
	if (EmptyClipboard())
            {
            HGLOBAL hClipData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, lBuf+1);
            if (hClipData)
                {
                size_t n;
		char* pGlobalBuf = (char *)GlobalLock(hClipData);
		CopyMemory(pGlobalBuf, pBuffer, lBuf);
		for (n=0; n<lBuf; n++) { /* Change NUL characters to spaces */
		  if (!pGlobalBuf[n]) pGlobalBuf[n] = pGlobalBuf[n] = ' ';
		}
		pGlobalBuf[lBuf++] = '\0'; // ~~JFL 2004-08-11 Append a NUL.
                GlobalUnlock(hClipData);
                if (SetClipboardData(cf, hClipData))
		    iResult = TRUE; /* finally, success! */
		else
		    PrintWin32Error("Failed to write to the clipboard");
                CloseClipboard();
                }
            else
                PrintWin32Error("Insufficient memory for the clipboard");
            }
        else
            PrintWin32Error("Could not empty the clipboard");
        }
    else
        PrintWin32Error("Could not open the clipboard");

    return iResult;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ToClipW						      |
|									      |
|   Description:    Copy a buffer of Unicode text to the clipboard	      |
|									      |
|   Parameters:     const WCHAR *pwBuffer	The data buffer to copy	      |
|		    size_t nWChars		The number of WCHARs to copy  |
|									      |
|   Returns:	    TRUE if success, or FALSE if FAILURE.		      |
|									      |
|   History:								      |
|    2014-03-31 JFL Created this routine, based on the ANSI ToClip().         |
|    2014-11-14 JFL Change NUL characters to spaces, to avoid truncating the  |
|		    clipboard data at the first NUL in the input stream.      |
*									      *
\*---------------------------------------------------------------------------*/

int ToClipW(const WCHAR* pwBuffer, size_t nWChars)
    {
    int iResult = FALSE;     /* assume failure */

    if (OpenClipboard(NULL))
        {
	if (EmptyClipboard())
            {
            HGLOBAL hClipData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (nWChars+1)*sizeof(WCHAR));
            if (hClipData)
                {
                size_t n;
		WCHAR* pGlobalBuf = (WCHAR *)GlobalLock(hClipData);
		CopyMemory(pGlobalBuf, pwBuffer, nWChars*sizeof(WCHAR));
		for (n=0; n<nWChars; n++) { /* Change NUL characters to spaces */
		  if (!pGlobalBuf[n]) pGlobalBuf[n] = pGlobalBuf[n] = ' ';
		}
		pGlobalBuf[nWChars++] = '\0'; // ~~JFL 2004-08-11 Append a NUL.
                GlobalUnlock(hClipData);
                if (SetClipboardData(CF_UNICODETEXT, hClipData))
		    iResult = TRUE; /* finally, success! */
		else
		    PrintWin32Error("Failed to write to the clipboard");
                CloseClipboard();
                }
            else
                PrintWin32Error("Insufficient memory for the clipboard");
            }
        else
            PrintWin32Error("Could not empty the clipboard");
        }
    else
        PrintWin32Error("Could not open the clipboard");

    return iResult;
    }

