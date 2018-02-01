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
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.0"
#define PROGRAM_DATE    "2018-01-23"

#define _UTF8_SOURCE	/* Tell MsvcLibX that this program generates UTF-8 output */

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "iconv.h"	/* MsvcLibX output encoding definitions */
#include "debugm.h"	/* SysToolsLib debug macros */

// Define WIN32 replacements for common Standard C library functions.
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)

#define PUTERR(s) fprintf(stderr, "%s\n", s)

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define streqi(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

#if defined(_WIN64)
#define OS_NAME "Win64"
#elif defined(_WIN32)
#define OS_NAME "Win32"
#else
#define OS_NAME "Unknown_OS"
#endif

/* Constants for the CopyClip() routine */

enum {
  COPYCLIP,
  ENUMCLIP,
};

#define CP_NULL ((UINT)-1) /* No output code page. Can't use 0 as CP_ACP is 0 */

/* Global variables */

DEBUG_GLOBALS

/* Function prototypes */

char *version(int iVerbose);	    /* Build a string with the program versions */
void usage(void);
int IsSwitch(char *pszArg);
int CopyClip(UINT type, UINT codepage);
int EnumClip(void);
int _cdecl ReportWin32Error(char *pszExplanation, ...);

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
  UINT codepage = consoleCodePage; /* The code page to use for the output */

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
	printf("%s\n", version(TRUE));
	exit(0);
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
      break;
    case ENUMCLIP:
      EnumClip();
      break;
    default:
      PUTERR("Unexpected action requested.");
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

char *version(int iVerbose) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME DEBUG_VERSION;
  char *pszLibVer = ""
#if defined(_MSDOS) || defined(_WIN32)
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
    ;
  char *pszVer = NULL;
  if (iVerbose) {
    pszVer = malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  UINT cpANSI = GetACP();
  UINT cpOEM = GetOEMCP();
  printf("\n\
1clip version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME "\n\
\n\
Pipe text data from the Windows clipboard to stdout.\n\
\n\
Usage:\n\
\n\
    1clip [OPTIONS] | <command>\n\
\n\
Options:\n\
  -?      Display this help screen.\n\
  -a      Get the ANSI text from the clipboard.\n\
  -A      Output using the ANSI code page #%u encoding.\n\
  -b      Output binary data.\n"
#ifdef _DEBUG
"\
  -d      Output debug information.\n"
#endif
"\
  -h      Get the HTML data from the clipboard. (Encoded in UTF-8)\n\
  -l      List clipboard formats available.\n\
  -o      Get the OEM text from the clipboard.\n\
  -O      Output using the OEM code page #%u encoding.\n\
  -r      Get the RTF data from the clipboard.\n\
  -t N    Get format N. Default: 1 = plain text.\n\
  -u      Get the Unicode text from the clipboard. (Default)\n\
  -U      Output using the UTF-8 code page #65001 encoding.\n\
  -V      Display the program version\n\
\n\
By default, the output is encoded in the current code page.\n\
\n"
"Author: Jean-François Larvoire"
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, cpANSI, cpOEM
);

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
|   Description:    Display a message box with the last error.		      |
|                                                                             |
|   Parameters:     char *pszExplanation    Why we think this occured	      |
|                                                                             |
|   Returns:	    The number of characters written.        		      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    1998-11-19 jfl Created this routine.				      |
|    2005-06-10 JFL Added the message description, as formatted by the OS.    |
|    2010-10-08 JFL Adapted to a console application, output to stderr.       |
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
    n = wsprintf(szErrorMsg, "Error %ld, %s", dwErr, lpMsgBuf);
    LocalFree( lpMsgBuf ); // Free the buffer.
  } else { // Error, we did not find a description string for this error code.
    n = wsprintf(szErrorMsg, "Error %ld: ", dwErr);
  }

  va_start(pArgs, pszExplanation);
  n += wvsprintf(szErrorMsg+n, pszExplanation, pArgs);
  va_end(pArgs);

  return fprintf(stderr, "%s", szErrorMsg);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CopyClip						      |
|									      |
|   Description:    Copy text from the clipboard to the output file	      |
|									      |
|   Parameters:     UINT type		Clipboard data type. Ex: CF_TEXT      |
|		    UINT codepage	Output code page. Ex: CP_ACP.	      |
|					CP_NULL=((UINT)-1) ==> Output raw data|
|									      |
|   Returns:	    The number of characters copied.			      |
|									      |
|   History:								      |
|    2005-06-10 JFL Created this routine.                                     |
|    2010-10-08 JFL Added the type argument.				      |
|    2010-10-14 JFL Skip the HTML header.				      |
|    2014-03-31 JFL Added the second argument to set an ouput code page.      |
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
	  PUTERR("Not enough memory.");
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
	    PUTERR("Cannot convert the data to Unicode.");
	    goto cleanup;
	  }
	  DEBUG_PRINTF(("Converted %d chars to %d WCHARs\n", nChars, nWChars));
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
	  PUTERR("Cannot convert the data to the output code page.");
	  goto cleanup;
	}
	DEBUG_PRINTF(("Converted %d WCHARs to %d chars in CP %d\n", nWChars, nChars, codepage));
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
      if (!utf16) {
	fflush(stdout); // Necessary, else the following _setmode() reverses it _before_ the conversion is done. 
	_setmode(_fileno(stdout), _O_TEXT);
      }
      if (!iDone) {
	PUTERR("Cannot write to the output file.");
      } else {
	DEBUG_PRINTF(("Wrote %d bytes.\n", nChars));
      }
cleanup:
      GlobalUnlock(hCBData);
    }

    CloseClipboard();
  } else {
    ReportWin32Error("Could not open clipboard!");
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
    ReportWin32Error("Could not open clipboard!");
  }

  return nChars;
}


