/*****************************************************************************\
*                                                                             *
*   Filename	    2note.c						      *
*									      *
*   Contents	    Pipe the output of a console application to Notepad	      *
*									      *
*   Notes:	    							      *
*									      *
*   History								      *
*    2018-02-08 JFL Adapted from 2clip.c.				      *
*    2018-02-13 JFL Use MsvcLibX for UTF-8 I/O, and debugm.h for OS macros.   *
*		    Convert Unix \n line endings to Windows \r\n. Version 1.0.*
*    2018-11-02 JFL Fixed a memory reallocation failure when converting \n.   *
*		    Improved ReportWin32Error().			      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.1.0.2.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.0.3.      *
*		    							      *
*         © Copyright 2018 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Copy text from stdin to the Windows Notepad"
#define PROGRAM_NAME    "2note"
#define PROGRAM_VERSION "1.0.3"
#define PROGRAM_DATE    "2019-06-12"

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
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

// Report an error.
void COMPLAIN(char *s) {
  DWORD e = GetLastError();
  if (e) {
    fprintf(stderr, "Error %uld: %s", e, s);
  } else {
    fprintf(stderr, "Error: %s", s);
  }
}

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */

#define BLOCKSIZE (4096)	// Number of characters that will be allocated in each loop.

/* Local definitions */

#define CP_NULL ((UINT)-1) /* No code page. Can't use 0 as CP_ACP is 0 */

/* Function prototypes */

void usage(void);
int IsSwitch(char *pszArg);
int ToClip(const char* pBuffer, size_t lBuf, UINT cf);
int ToNotepadW(const WCHAR* pwBuffer, size_t nWChars);

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
|    2018-02-08 JFL Adapted from 2clip.c.		                      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  size_t  nTotal = 0;		// Total number of characters read.
  char   *pBuffer;
  int i;
  UINT codepage;
  int iDone;

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
      if (streq(arg+1, "O")) {		/* Assume the input is OEM text */
	codepage = CP_OEMCP;
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
      fprintf(stderr, "Unsupported switch %s ignored.\n", arg);
      continue;
    }
    fprintf(stderr, "Unexpected argument %s ignored.\n", arg);
  }

  /* Go for it */

  /* Force stdin to untranslated */
  _setmode( _fileno( stdin ), _O_BINARY );

  pBuffer = (char*)malloc(0);
  while (!feof(stdin)) {
    pBuffer = realloc(pBuffer, nTotal + BLOCKSIZE);
    if (!pBuffer) {
      COMPLAIN("Insufficient memory!");
      exit(1);
    }
    nTotal += fread(pBuffer+nTotal, 1, BLOCKSIZE, stdin);
    Sleep(0);	    // Release end of time-slice
  }
  if (nTotal > 0) {
    WCHAR *pwUnicodeBuf = (WCHAR *)pBuffer;
    /* This is text. Convert it to Unicode to avoid codepage issues */
    if (codepage != CP_NULL) {
      pwUnicodeBuf = (WCHAR *)malloc(2*(nTotal));
      if (!pwUnicodeBuf) {
	COMPLAIN("Insufficient memory!");
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
	COMPLAIN("Can't convert the intput to Unicode!");
	exit(1);
      }
    } else {
      nTotal /= 2;	/* # of wide unicode characters */
    }
    iDone = ToNotepadW(pwUnicodeBuf, nTotal);
    if (!iDone) {
      COMPLAIN("Failed to write to Notepad!");
      exit(1);
    }
  } else {
    /* This is actually not a bug: The input CAN be empty */
  }

  return 0;
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
  -A        The input is ANSI text (Code page %u)\n\
  -O        The input is OEM text (Code page %u)\n\
  -u        The input is UTF-16 text (Unicode)\n\
  -U        The input is UTF-8 text (Code page 65001)\n\
  -V        Display the program version\n\
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
|									      |
|    1997-03-04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
    case '/':
      return TRUE;
    default:
      return FALSE;
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ReportWin32Error					      |
|                                                                             |
|   Description:    Display a message with the last error.		      |
|                   							      |
|   Parameters:     char *pszExplanation    Why we think this occured, or NULL|
|                   ...			    Optional arguments to format      |
|                   							      |
|   Returns:	    The number of characters written.        		      |
|                   							      |
|   Notes:	    Danger: Known issue: We use a fixed 4KB buffer for the    |
|		    error message, and do not check for overflows. This is    |
|		    more than enough for all actual error message, but be     |
|		    cautious anyway when using optional formatted arguments.  |
|                   							      |
|   History:								      |
|    1998-11-19 jfl Created this routine.				      |
|    2005-06-10 JFL Added the message description, as formatted by the OS.    |
|    2010-10-08 JFL Adapted to a console application, output to stderr.       |
|    2018-11-02 JFL Allow pszExplanation to be NULL.			      |
|                   Make sure WIN32 msg and explanation are on the same line. |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl ReportWin32Error(char *pszExplanation, ...) {
  char szErrorMsg[4096];
  va_list pArgs;
  int n;
  LPVOID lpMsgBuf;
  DWORD dwErr = GetLastError();

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
    n = wsprintf(szErrorMsg, "Error %uld: %s", dwErr, lpMsgBuf);
    LocalFree(lpMsgBuf); // Free the buffer.
    // Remove the trailing new line
    if (n && (szErrorMsg[n-1] == '\n')) szErrorMsg[--n] = '\0';
    if (n && (szErrorMsg[n-1] == '\r')) szErrorMsg[--n] = '\0';
  } else { // Error, we did not find a description string for this error code.
    n = wsprintf(szErrorMsg, "Error %ld.", dwErr);
  }

  if (pszExplanation) {
    szErrorMsg[n++] = ' '; // Add a blank separator
    va_start(pArgs, pszExplanation);
    n += wvsprintf(szErrorMsg+n, pszExplanation, pArgs);
    va_end(pArgs);
  }

  return fprintf(stderr, "%s\n", szErrorMsg);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FindMainWindow					      |
|									      |
|   Description:    Find the main window for a given process		      |
|									      |
|   Parameters:     unsigned long process_id	The process ID number	      |
|									      |
|   Returns:	    The hWND if found, or NULL if not.			      |
|									      |
|   History:								      |
|    2018-02-08 JFL Adapted from the sample published in		      |
|		    https://stackoverflow.com/questions/1888863/how-to-get-main-window-handle-from-process-id
*									      *
\*---------------------------------------------------------------------------*/

BOOL IsMainWindow(HWND handle) {   
  return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

typedef struct _handle_data {
  unsigned long process_id;
  HWND best_handle;
} handle_data;

BOOL CALLBACK FindMainWindow_CallBack(HWND hWnd, LPARAM lParam) {
  handle_data* pData = (handle_data*)lParam;
  unsigned long process_id = 0;
  if (!IsMainWindow(hWnd)) return TRUE; /* Keep searching */
  GetWindowThreadProcessId(hWnd, &process_id);
  if (pData->process_id != process_id) return TRUE; /* Keep searching */
  pData->best_handle = hWnd;
  return FALSE; /* Found. Stop searching */
}

HWND FindMainWindow(unsigned long process_id) {
  handle_data data;
  data.process_id = process_id;
  data.best_handle = 0;
  EnumWindows(FindMainWindow_CallBack, (LPARAM)&data);
  return data.best_handle;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ToNotepadW						      |
|									      |
|   Description:    Copy a buffer of Unicode text to the clipboard	      |
|		    							      |
|   Parameters:     const WCHAR *pwBuffer	The data buffer to copy	      |
|		    size_t nWChars		The number of WCHARs to copy  |
|		    							      |
|   Returns:	    TRUE if success, or FALSE if FAILURE.		      |
|		    							      |
|   History:								      |
|    2014-03-31 JFL Created this routine, based on the ANSI ToClip().         |
|    2014-11-14 JFL Change NUL characters to spaces, to avoid truncating the  |
|		    clipboard data at the first NUL in the input stream.      |
|    2018-11-02 JFL Fix the reallocation failure, by directly allocating the  |
|		    right size.						      |
*									      *
\*---------------------------------------------------------------------------*/

int ToNotepadW(const WCHAR* pwBuffer, size_t nWChars) {
  STARTUPINFO si = {0};
  PROCESS_INFORMATION pi = {0};
  HWND hMainWnd = 0;
  HWND hWnd;
  WCHAR* pGlobalBuf;
  size_t n, m, nLF;
  int iResult = FALSE; /* Assume failure */

  /* Check if the input contains Unix LF line endings */
  for (n=nLF=0; n<nWChars; n++) {
    if ((pwBuffer[n] != L'\r') && (pwBuffer[n+1] == L'\n')) nLF += 1;
  }
  /* Allocate a global buffer with extra space to allow converting all \n to \r\n */
  pGlobalBuf = (WCHAR *)GlobalAlloc(GMEM_FIXED, (nWChars+nLF+1)*sizeof(WCHAR));
  if (!pGlobalBuf) {
    ReportWin32Error("Can't allocate a global buffer.");
    return FALSE;
  }
  /* Copy the buffer, making sure everything's appears correctly in Notepad */
  for (n=m=0; n<nWChars; n++, m++) { /* Copy text, changing NUL to ' ', and \n to \r\n */
    WCHAR wc = pwBuffer[n];
    if (!wc) wc = L' ';
    pGlobalBuf[m] = wc;
    if ((wc != L'\r') && (pwBuffer[n+1] == L'\n')) pGlobalBuf[++m] = L'\r';
  }
  pGlobalBuf[m] = L'\0'; /* Append a final NUL */

  /* Start the child process. */ 
  if (!CreateProcess(NULL,		// Module name
		     "notepad.exe",	// Command line
		     NULL,    		// Process handle not inheritable
		     NULL,    		// Thread handle not inheritable
		     FALSE,   		// Set handle inheritance to FALSE
		     0,       		// No creation flags
		     NULL,    		// Use parent's environment block
		     NULL,    		// Use parent's starting directory 
		     &si,     		// Pointer to STARTUPINFO structure
		     &pi )    		// Pointer to PROCESS_INFORMATION structure
     ) {
    ReportWin32Error("CreateProcess failed.");
    goto cleanup;
  }

  /* Wait until the Notepad main window appears */
  for (n=0; n<100; n++) {
    Sleep(100);	/* Wait 100ms */
    hMainWnd = FindMainWindow(pi.dwProcessId);
    if (hMainWnd) break;
  }
  if (!hMainWnd) {
    fprintf(stderr, "Failed to get Notepad main window handle.\n");
    goto cleanup;
  }
  /* Find Notepad content area window */
  hWnd = FindWindowEx(hMainWnd, 0, "Edit", NULL);
  if (!hWnd) {
    fprintf(stderr, "Failed to get Notepad edit window handle.\n");
    goto cleanup;
  }

  /* Set the Notepad content text */
  SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)pGlobalBuf);
  iResult = TRUE; /* Done */

  /* Cleanup and exit */
cleanup:
  GlobalFree(pGlobalBuf);
  return iResult;
}
