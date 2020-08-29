/*****************************************************************************\
*                                                                             *
*   Filename	    1clip.c						      *
*									      *
*   Contents	    Pipe the clipboard text contents to standard output	      *
*									      *
*   Notes:	    							      *
*									      *
*   History								      *
*    1997-05-09 JFL Adapted 2clip.c from Windows Developer's Journal sample.  *
*    2005-06-10 JFL Created this reverse program. Version 1.0.		      *
*    2006-04-03 JFL Improved error reporting. Version 1.01.		      *
*    2010-10-05 JFL Added the -l option.				      *
*    2010-10-08 JFL Changed to a simple console application.		      *
*		    Added options -t and -h.				      *
*		    Version 1.1.					      *
*    2010-10-14 JFL Use RegisterClipboardFormat to identify the HTML format.  *
*		    Skip the HTML format clipboard-specific header.	      *
*		    Add an HTML header specifying the UTF-8 encoding.	      *
*		    Bugfix: Output clipboard contents with untranslated LF.   *
*		    Version 1.1.1.					      *
*    2012-03-02 JFL Updated the help message.                                 *
*		    Version 1.1.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 1.1.3.                 *
*    2013-09-06 JFL Added options -o and -u. Version 1.2.                     *
*    2014-03-31 JFL Added support for local code pages.                       *
*		    Added options -A, -b, -O , -U to override the default CP. *
*		    Version 1.3.					      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2016-09-23 JFL Removed warnings. No functional code change.              *
*    2017-06-28 JFL Fixed the link warning.				      *
*                   Help displays the actual ANSI and OEM code pages.         *
*    2017-10-12 JFL Fixed the -t option parsing. Version 1.3.2.		      *
*    2017-12-05 JFL Display the HTML Format header in dbg mode. Version 1.3.3.*
*		    Added option -r to get RTF data.			      *
*    2018-01-23 JFL Bugfix: Option -t alone did not default to CF_TEXT.       *
*                   Bugfix: Debug printf("Printing ...") could cause a hang.  *
*                   Use MsvcLibX for output to the console. Version 2.0.      *
*                   This fixes the v1.3 issues with large output in CP 65001. *
*    2018-11-02 JFL Improved ReportWin32Error(). Version 2.0.1.		      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.2.0.2.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.0.3.      *
*    2019-09-27 JFL Fixed a formatting error in ReportWin32Error(), and       *
*                   Prepend the program name to the output. Version 2.0.4.    *
*    2019-11-01 JFL Added option -Z to append a Ctrl-Z (EOF) to the output.   *
*		    Version 2.1.					      *
*    2020-01-19 JFL Fixed the default HTML output to be UTF8 in any console CP.
*    2020-04-25 JFL Rewrote ReportWin32Error as PrintWin32Error(); Added new  *
*		    PrintCError(), and use them instead of PUTERR(). V 2.1.2. *
*    2020-08-29 JFL Merged in changes from another PrintWin32Error(). V 2.1.3.*
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Copy text from the Windows clipboard to stdout"
#define PROGRAM_NAME    "1clip"
#define PROGRAM_VERSION "2.1.3"
#define PROGRAM_DATE    "2020-08-29"

#define _UTF8_SOURCE	/* Tell MsvcLibX that this program generates UTF-8 output */

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <iconv.h>	/* MsvcLibX output encoding definitions */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings and routine. Include last. */

// Define WIN32 replacements for common Standard C library functions.
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define streqi(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

/* Constants for the CopyClip() routine */

enum {
  COPYCLIP,
  ENUMCLIP,
};

#define CP_NULL ((UINT)-1)	/* No output code page. Can't use 0 as CP_ACP is 0 */
#define CP_AUTO ((UINT)-2)	/* Autoselect code page. Can't use 0 as CP_ACP is 0 */

/* Global variables */

DEBUG_GLOBALS

#if _DEBUG
UINT ActualCP(UINT cp) { /* Return the actual code page number for special constants */
  switch (cp) {
    case CP_ACP: return GetACP();	/* CP_ACP = 0 = Default Windows code page */
    case CP_OEMCP: return GetOEMCP();	/* CP_OEMCP = 1 = Default DOS code page */
    default: return cp;	 /* Anything else is an actual code page number */
  }
}
#endif

/* Function prototypes */

void usage(void);
int IsSwitch(char *pszArg);
int CopyClip(UINT type, UINT codepage);
int EnumClip(void);
int PrintCError(const char *pszExplanation, ...);
int PrintWin32Error(const char *pszExplanation, ...);

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
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int i;
  int iAction = COPYCLIP;
  UINT type = CF_UNICODETEXT;
  UINT codepage = CP_AUTO;	    /* The code page to use for the output. Default: Choose automatically */
  int iCtrlZ = FALSE;		    /* If true, append a Ctrl-Z to the output */

  /* Process arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {		    /* It's a switch */
      if (streq(arg+1, "?")) {			/* -?: Help */
	usage();                            /* Display help */
	return 0;
      }
      if (streq(arg+1, "a")) {			/* -a: Get the ANSI type */
	type = CF_TEXT;
	continue;
      }
      if (streq(arg+1, "A")) {			/* -A: Output as ANSI text */
	codepage = CP_ACP;
	continue;
      }
      if (streq(arg+1, "b")) {			/* -b: Binary output */
	codepage = CP_NULL;
	continue;
      }
      DEBUG_CODE(
      if (streq(arg+1, "d")) {			/* -d: Debug */
	iDebug = 1;
	continue;
      }
      )
      if (streq(arg+1, "h")) {			/* -h: Get the HTML */
	type = RegisterClipboardFormat("HTML Format");
	continue;
      }
      if (streq(arg+1, "l")) {			/* -l: List */
	iAction = ENUMCLIP;
	continue;
      }
      if (streq(arg+1, "o")) {			/* -o: Get the OEM type */
	type = CF_OEMTEXT;
	continue;
      }
      if (streq(arg+1, "O")) {			/* -O: Output as OEM text */
	codepage = CP_OEMCP;
	continue;
      }
      if (streq(arg+1, "r")) {			/* -r: Get the RTF */
	type = RegisterClipboardFormat("Rich Text Format");
	continue;
      }
      if (streq(arg+1, "t")) {			/* -t: Type */
      	type = CF_TEXT;
	if (((i+1) < argc) && !IsSwitch(argv[i+1])) sscanf(argv[++i], "%u", &type);
	continue;
      }
      if (streq(arg+1, "u")) {			/* -u: Get the Unicode type */
	type = CF_UNICODETEXT;
	continue;
      }
      if (streq(arg+1, "U")) {			/* -U: Output as UTF-8 text */
	codepage = CP_UTF8;
	continue;
      }
      if (streq(arg+1, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      if (streq(arg+1, "Z")) {			/* -z: Append a Ctrl-Z to the output */
	iCtrlZ = TRUE;
	continue;
      }
      fprintf(stderr, "Unsupported switch %s ignored.\n", arg);
      continue;
    }
    fprintf(stderr, "Unexpected argument %s ignored.\n", arg);
  }
  DEBUG_PRINTF(("The current console code page is %d\n", consoleCodePage));
  DEBUG_PRINTF(("The selected output code page is %d\n", codepage));

  /* Go for it */
  switch (iAction) {
    case COPYCLIP:
      CopyClip(type, codepage);
      if (iCtrlZ) putc('\x1A', stdout);
      break;
    case ENUMCLIP:
      EnumClip();
      break;
    default:
      fprintf(stderr, "Unexpected action requested.\n");
      break;
  }

  DEBUG_PRINTF(("Exiting\n"));
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
  // UINT cpCurrent = GetConsoleOutputCP();

  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
\n\
    1clip [OPTIONS] | <command>\n\
\n\
Options:\n\
  -?      Display this help screen\n\
  -a      Get the ANSI text from the clipboard\n\
  -A      Output using the ANSI encoding (Code page %u)\n\
  -b      Output binary data\n"
#ifdef _DEBUG
"\
  -d      Output debug information\n"
#endif
"\
  -h      Get the HTML data from the clipboard (Encoded in UTF-8)\n\
  -l      List clipboard formats available\n\
  -o      Get the OEM text from the clipboard\n\
  -O      Output using the OEM encoding (Code page %u)\n\
  -r      Get the RTF data from the clipboard\n\
  -t N    Get format N. Default: 1 = plain text\n\
  -u      Get the Unicode text from the clipboard (Default)\n\
  -U      Output using the UTF-8 encoding (Code page 65001)\n\
  -V      Display the program version\n\
  -Z      Append a Ctrl-Z (aka. SUB or EOF) to the output\n\
\n\
Default output encoding: The current console code page (Code page %u)\n\
\n"
"Author: Jean-François Larvoire"
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, cpANSI, cpOEM, consoleCodePage // cpCurrent
);

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

int PrintCError(const char *pszFormat, ...) {
  va_list vl;
  int n;
  int e = errno; /* The initial C errno when this routine starts */

  n = fprintf(stderr, PROGRAM_NAME EXE_SUFFIX ": Error: ");

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
|   Function	    PrintWin32Error					      |
|                                                                             |
|   Description     Display a message with the last WIN32 error.	      |
|                   							      |
|   Parameters      char *pszFormat	The error context, or NULL            |
|		    ...			Optional arguments		      |
|                   							      |
|   Returns	    The number of characters written        		      |
|                   							      |
|   Notes	    							      |
|                   							      |
|   History								      |
|    1997-03-25 JFL Created another PrintWin32Error routine.		      |
|    1998-11-19 JFL Created routine ReportWin32Error.			      |
|    2005-06-10 JFL Added the message description, as formatted by the OS.    |
|    2010-10-08 JFL Adapted to a console application, output to stderr.       |
|    2015-01-15 JFL Converted PrintWin32Error to using UTF-8.		      |
|    2018-11-02 JFL Allow pszExplanation to be NULL.			      |
|                   Make sure WIN32 msg and explanation are on the same line. |
|    2019-09-27 JFL Fixed a formatting error.                                 |
|                   Prepend the program name to the output.		      |
|    2020-04-25 JFL Redesigned to avoid using a fixed size buffer.	      |
|                   Reordered the output so that it sounds more natural.      |
|                   Renamed ReportWin32Error as PrintWin32Error.	      |
|    2020-08-28 JFL If no message found, display both dec. and hexa. codes.   |
|    2020-08-29 JFL Merged in 2015-01-15 UTF-8 support from the other routine.|
*                                                                             *
\*---------------------------------------------------------------------------*/

#define WIN32_ERROR_PREFIX PROGRAM_NAME EXE_SUFFIX ": Error: "
// #define WIN32_ERROR_PREFIX "Error: "

int PrintWin32Error(const char *pszFormat, ...) {
  va_list vl;
  int n = 0;
  size_t l;
  LPWSTR lpwMsgBuf;
  LPSTR  lpMsgBuf;
  DWORD dwError = GetLastError(); // The initial WIN32 error when this routine starts

#if defined(WIN32_ERROR_PREFIX)
  n += fprintf(stderr, "%s", WIN32_ERROR_PREFIX);
#endif

  if (pszFormat) {
    va_start(vl, pszFormat);
    n += vfprintf(stderr, pszFormat, vl);
    n += fprintf(stderr, ". ");
    va_end(vl);
  }

  l = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		     FORMAT_MESSAGE_FROM_SYSTEM | 
		     FORMAT_MESSAGE_IGNORE_INSERTS,
		     NULL,
		     dwError,
		     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		     (LPWSTR)&lpwMsgBuf, // Address of the allocated buffer address
		     0,
		     NULL);
  if (l) {
    // Remove the trailing dot and CRLF, if any
    // (The CR causing problems later on, as the LF would be expanded with a second CR)
    if (l && (lpwMsgBuf[l-1] == L'\n')) lpwMsgBuf[--l] = L'\0';
    if (l && (lpwMsgBuf[l-1] == L'\r')) lpwMsgBuf[--l] = L'\0';
    if (l && (lpwMsgBuf[l-1] == L'.')) lpwMsgBuf[--l] = L'\0';
    // Convert the trimmed message to UTF-8
    l = 2*(l+1); // Worst case increase for UTF16 to UTF8 conversion
    lpMsgBuf = (char *)LocalAlloc(LPTR, l);
    if (lpMsgBuf) {
      l = WideCharToMultiByte(CP_UTF8, 0, lpwMsgBuf, -1, lpMsgBuf, (int)l, NULL, NULL);
      if (l) n += fprintf(stderr, "%s.\n", lpMsgBuf);
      LocalFree(lpMsgBuf); // Free the UTF-8 buffer
    }
    LocalFree(lpwMsgBuf); // Free the UTF-16 buffer
  }
  if (!l) { // We did not find a description string for this error code, or could not display it
    n += fprintf(stderr, "Win32 error %lu (0x%08lX).\n", dwError, dwError);
  }

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CopyClip						      |
|									      |
|   Description:    Copy text from the clipboard to the standard output file  |
|									      |
|   Parameters:     UINT type		Clipboard data type. Ex: CF_TEXT      |
|		    UINT codepage	Output code page. Ex: CP_ACP.	      |
|					CP_NULL=((UINT)-1) ==> Output raw data|
|					CP_AUTO=((UINT)-2) ==> Autoselect CP. |
|									      |
|   Returns:	    The number of characters copied.			      |
|									      |
|   History:								      |
|    2005-06-10 JFL Created this routine.                                     |
|    2010-10-08 JFL Added the type argument.				      |
|    2010-10-14 JFL Skip the HTML header.				      |
|    2014-03-31 JFL Added the second argument to set an ouput code page.      |
|    2020-01-19 JFL Added CP_AUTO constant management.                        |
|		    Fixed the default HTML output to be UTF8 in any console CP|
*									      *
\*---------------------------------------------------------------------------*/

char szOldHeader[] = "<html>\r\n\
<body>";
#define LOLDHEADER (sizeof(szOldHeader) - 1)

char szNewHeader[] = "<html>\r\n\
<head>\r\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\r\n\
</head>\r\n\
<body>";
#define LNEWHEADER (sizeof(szNewHeader) - 1)

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

int CopyClip(UINT type, UINT codepage) {
  int nChars = 0;
  int iDone;
  int isUtf8 = FALSE;
  int utf16 = FALSE;	// If true, output UTF-16

  DEBUG_PRINTF(("CopyClip(%d, %d);\n", type, codepage));

  if (OpenClipboard(NULL)) {
    HANDLE hCBData;
    LPSTR lpString;
    int errno0;

    hCBData = GetClipboardData(type);
    if (hCBData) {
      lpString = GlobalLock(hCBData);
      // nChars = lstrlen(lpString); // This works only for text types
      nChars = (int)GlobalSize(hCBData);
      switch (type) {
	case CF_TEXT:
	case CF_OEMTEXT:
	  nChars -= 1; break; // Remove the trailing NUL
	case CF_UNICODETEXT:
	  nChars -= 2; break; // Remove the trailing unicode NUL
	default: {
	  char buf[128];
	  int n;
	  if (   (type > CF_PRIVATEFIRST)
	    && ((n = GetClipboardFormatName(type, buf, sizeof(buf))) != 0)
	    && streq(buf, "HTML Format")) { // Special case for HTML:
	    // https://msdn.microsoft.com/en-us/library/aa767917.aspx
	    char *pc;
	    int iFirst = 0;
	    int iLast = nChars - 1; // Remove the trailing NUL
	    isUtf8 = TRUE; // HTML is already encoded in UTF8 in the clipboard
	    if (codepage == CP_AUTO) { // For HTML, the default CP choice is special:
	      if (isConsole(_fileno(stdout))) { // Make sure all characters are displayed correctly
	      	codepage = consoleCodePage;
	      } else { // For both pipes and files, output the HTML encoded in UTF-8
	      	codepage = CP_UTF8;
	      }
	    }
	    DEBUG_CODE(
	    if (!iDebug) { // Show the unmodified header in debug mode
	    )
	      // Skip the clipboard information header
	      if ((pc = strstr(lpString, "\nStartHTML:")) != NULL) sscanf(pc+11, "%d", &iFirst);
	      if ((pc = strstr(lpString, "\nEndHTML:")) != NULL) sscanf(pc+9, "%d", &iLast);
	      // Update the HTML header to declare it's encoded in UTF-8
	      if (iFirst > (LNEWHEADER - LOLDHEADER)) {
		if (!strncmp(lpString + iFirst, szOldHeader, LOLDHEADER)) {
		  iFirst -= LNEWHEADER - LOLDHEADER;
		  strncpy(lpString + iFirst, szNewHeader, LNEWHEADER);
		}
	      }
	    DEBUG_CODE(
	    }
	    if (iDebug) {
	      fflush(stdout); // Necessary in case we've added debugging output before this
	      _setmode(_fileno(stdout), _O_BINARY);
	      fwrite(lpString, 1, iFirst, stdout); /* Display the HTML Format header */
	      fflush(stdout);
	      _setmode(_fileno(stdout), _O_TEXT); // In case we have more debugging output
	    }
	    )
	    lpString += iFirst;
	    nChars = iLast - iFirst;
	    break;
	  }
	}
      }
      /* For data types other than HTML, the default output CP is the console CP */
      if (codepage == CP_AUTO) codepage = consoleCodePage;
      /* Check if MsvcLibX can output UTF-16 */
      if (isConsole(_fileno(stdout)) && (codepage == consoleCodePage)) {
	DEBUG_PRINTF(("Writing Unicode\n"));
      	utf16 = TRUE;
      	codepage = CP_UTF8;
      }
      /* Convert the output to the requested code page */
      if ((codepage != CP_NULL) && (!(isUtf8 && (codepage == CP_UTF8)))) {
	int nWChars;
      	WCHAR *pwszBuf = (WCHAR *)malloc(2 * nChars); /* Worst case for Unicode encoding */
      	char *pszBuf = malloc(4 * nChars); /* Worst case for UTF-8 encoding */
      	if (!pwszBuf || !pszBuf) {
	  PrintCError("Can't convert the data to Unicode");
	  exit(1);
      	}
      	/* First, convert multi-byte formats to Unicode */
      	if (type != CF_UNICODETEXT) {
      	  UINT codepage0 = CP_ACP;
      	  switch (type) {
      	  case CF_TEXT:
      	    codepage0 = CP_ACP;
      	    break;
      	  case CF_OEMTEXT:
      	    codepage0 = CP_OEMCP;
      	    break;
      	  default:
      	    if (isUtf8) codepage0 = CP_UTF8;
      	    break;
      	  }
	  nWChars = MultiByteToWideChar(codepage0,	/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
				        0,		/* dwFlags, */
				        lpString,	/* lpMultiByteStr, */
				        nChars,		/* cbMultiByte, */
				        pwszBuf,	/* lpWideCharStr, */
				        nChars		/* cchWideChar, */
				        );
	  if (!nWChars) {
	    PrintWin32Error("Can't convert the data to Unicode");
	    goto cleanup;
	  }
	  DEBUG_PRINTF(("Converted %d chars in CP %u to %d WCHARs\n", nChars, ActualCP(codepage0), nWChars));
        } else {
          pwszBuf = (WCHAR *)lpString;
          nWChars = nChars / 2;
        }
        /* Then convert Unicode back to the final output code page */
	nChars = WideCharToMultiByte(codepage,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
				     0,			/* dwFlags, */
				     pwszBuf,		/* lpWideCharStr, */
				     nWChars,		/* cchWideChar, */
				     pszBuf,		/* lpMultiByteStr, */
				     4 * nChars,	/* cbMultiByte, */
				     NULL,		/* lpDefaultChar, */
				     NULL		/* lpUsedDefaultChar */
				     );
	if (!nChars) {
	  PrintWin32Error("Cannot convert the data to the output code page.");
	  goto cleanup;
	}
	DEBUG_PRINTF(("Converted %d WCHARs to %d chars in CP %u\n", nWChars, nChars, ActualCP(codepage)));
	lpString = pszBuf;
	// lpString[nChars] = '\0'; 
      }
      /* Unless writing UTF16 to the console, switch to binary mode, to write
         exactly what we got from the clipboard, even Unix line endings */
      if (!utf16) {
	fflush(stdout); // Necessary in case we've added debugging output before this
      	_setmode(_fileno(stdout), _O_BINARY);
      }
      iDone = (int)fwrite(lpString, nChars, 1, stdout);
      errno0 = errno;
      if (!utf16) {
	fflush(stdout); // Necessary, else the following _setmode() reverses it _before_ the conversion is done. 
	_setmode(_fileno(stdout), _O_TEXT);
      }
      if (!iDone) {
	errno = errno0;
	PrintCError("Cannot write to the output file");
      } else {
	DEBUG_PRINTF(("Wrote %d bytes.\n", nChars));
      }
cleanup:
      GlobalUnlock(hCBData);
    }

    CloseClipboard();
  } else {
    PrintWin32Error("Could not open the clipboard");
  }

  return nChars;
}

#pragma warning(default:4996)	/* Restore the deprecated name warning */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    EnumClip						      |
|									      |
|   Description:    Enumerate clipboard formats available		      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    The number of characters copied.			      |
|									      |
|   History:								      |
|    2010-09-21 JFL Created this routine.                                     |
*									      *
\*---------------------------------------------------------------------------*/

typedef struct {
  UINT n;
  char *flag;
  char *desc;
} clipFormat;

clipFormat cf[] = {
  {1, "CF_TEXT", "Text"},
  {2, "CF_BITMAP", "BMP Bitmap"},
  {3, "CF_METAFILEPICT", "Metafile picture"},
  {4, "CF_SYLK", "Microsoft Symbolic Link format"},
  {5, "CF_DIF", "Software Arts' Data Interchange Format"},
  {6, "CF_TIFF", "Tagged-image file format"},
  {7, "CF_OEMTEXT", "Text in the OEM character set"},
  {8, "CF_DIB", "Device Independant Bitmap"},
  {9, "CF_PALETTE", "Color palette"},
  {10, "CF_PENDATA", "Pen data"},
  {11, "CF_RIFF", "Audio data more complex than the standard wave format"},
  {12, "CF_WAVE", "Audio data in one of the standard wave formats"},
  {13, "CF_UNICODETEXT", "Unicode text"},
  {14, "CF_ENHMETAFILE", "Enhanced metafile"},
  {15, "CF_HDROP", "HDROP list of files"},
  {16, "CF_LOCALE", "Locale identifier for the text in the clipboard."},
  {17, "CF_DIBV5", "Device Independant Bitmap v5"},
  {0x0080, "CF_OWNERDISPLAY", "Owner-display format"},
  {0x0081, "CF_DSPTEXT", "Private text format"},
  {0x0082, "CF_DSPBITMAP", "Private bitmap format"},
  {0x0083, "CF_DSPMETAFILEPICT", "Private metafile picture format"},
  {0x008E, "CF_DSPENHMETAFILE", "Private enhanced metafile format"},
#if 0
  {0x0200, "CF_PRIVATEFIRST", "Start of a range for private clipboard formats"},
  {0x02FF, "CF_PRIVATELAST", "End of a range for private clipboard formats"},
  {0x0300, "CF_GDIOBJFIRST", "Start of range for application-defined GDI objects"},
  {0x03FF, "CF_GDIOBJLAST", "End of range for application-defined GDI objects"},
#endif
};
#define NCLIPFORMATS (sizeof(cf) / sizeof(clipFormat))

#define CFLISTSIZE 100

int CompareUint(const UINT *pu1, const UINT *pu2) {
  if (*pu1 < *pu2) return -1;
  if (*pu1 > *pu2) return 1;
  return 0;
}

int EnumClip(void) {
  int nChars = 0;
  UINT cfList[CFLISTSIZE];
  int ncf = 0;		// Number of clipboard formats
  UINT icf;		// Current clipboard format type
  char *pszFormat = "%-6d %-16s %s\n";
  int j;

  if (OpenClipboard(NULL)) {
    for (icf = 0; (icf = EnumClipboardFormats(icf)) != 0; ) {
      cfList[ncf++] = icf;
    }
    qsort(cfList, ncf, sizeof(UINT), CompareUint); // Sort list by numeric order
    for (j = 0; j < ncf; j++) {
      int i;
      int found = FALSE;
      icf = cfList[j];
      for (i=0; i<NCLIPFORMATS; i++) {
	if (cf[i].n == icf) {
	  found = TRUE;
	  nChars += printf(pszFormat, icf, cf[i].flag, cf[i].desc);
	  break;
	}
      }
      if (!found) {
      	int n;
      	char buf[128];
      	n = GetClipboardFormatName(icf, buf, sizeof(buf));
      	if (!n) strcpy(buf, "Unknown clipboard format");
	nChars += printf(pszFormat, icf, "", buf);
      }	        		
    }

    CloseClipboard();
  } else {
    PrintWin32Error("Could not open the clipboard");
  }

  return nChars;
}


