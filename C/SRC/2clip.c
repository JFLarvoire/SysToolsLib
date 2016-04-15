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
*									      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.3.2"
#define PROGRAM_DATE    "2014-11-14"

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#define COMPLAIN(s) fprintf(stderr, "Error %d: %s", GetLastError(), s)

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */

#define BLOCKSIZE (4096)	// Number of characters that will be allocated in each loop.

#define NARGS 10		// Max number of command line arguments supported.

#if defined(_WIN64)
#define OS_NAME "Win64"
#elif defined(_WIN32)
#define OS_NAME "Win32"
#else
#define OS_NAME "Unknown_OS"
#endif

/* Local definitions */

#define CP_NULL ((UINT)-1) /* No code page. Can't use 0 as CP_ACP is 0 */

/* Function prototypes */

void usage(void);
int IsSwitch(char *pszArg);
#if 0
int ToClip(const char* pBuffer, size_t lBuf);
#endif
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
	printf(PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME "\n");
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
    if (!ToClipW(pwUnicodeBuf, nTotal)) {
      COMPLAIN("Failed to write to the clipboard!");
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

void usage(void)
    {
    printf("\
\n\
2clip version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME "\n\
\n\
Pipe text from stdin to the Windows clipboard\n\
\n\
Usage:\n\
\n\
    <command> | 2clip [switches]\n\
\n\
Switches:\n\
  -?        Display this help screen\n\
  -A        Assume input is ANSI text (Code page 1252)\n\
  -O        Assume input is OEM text (Code page 437)\n\
  -u        Assume input is Unicode text\n\
  -U        Assume input is UTF-8 text (Code page 65001)\n\
  -V        Display the program version\n\
\n\
By default, the input is assumed to be encoded in the current code page.\n\
\n"
"Author: Jean-Francois Larvoire"
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
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

int IsSwitch(char *pszArg)
    {
    switch (*pszArg)
	{
	case '-':
	case '/':
	    return TRUE;
	default:
	    return FALSE;
	}
    }

#if 0
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ToClip						      |
|									      |
|   Description:    Copy a buffer of text to the clipboard		      |
|									      |
|   Parameters:     const char *pBuffer     The data buffer to copy	      |
|		    size_t lBuf		    The number of bytes to copy       |
|									      |
|   Returns:	    TRUE if success, or FALSE if FAILURE.		      |
|									      |
|   History:								      |
|									      |
|    1997-05-09 JFL Adapted from Windows Developer's Journal sample           |
|    2004-08-11 JFL Fixed a bug that caused the last character to be lost.    |
|    2014-11-14 JFL Change NUL characters to spaces, to avoid truncating the  |
|		    clipboard data at the first NUL in the input stream.      |
*									      *
\*---------------------------------------------------------------------------*/

int ToClip(const char* pBuffer, size_t lBuf)
    {
    int iResult = FALSE;     /* assume failure */

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
		for (n=0; n<nWChars; n++) { /* Change NUL characters to spaces */
		  if (!pGlobalBuf[n]) pGlobalBuf[n] = pGlobalBuf[n] = ' ';
		}
		pGlobalBuf[lBuf++] = '\0'; // ~~JFL 2004-08-11 Append a NUL.
                GlobalUnlock(hClipData);
                if (SetClipboardData(CF_TEXT, hClipData))
		    iResult = TRUE; /* finally, success! */
                CloseClipboard();
                }
            else
                COMPLAIN("Insufficient memory for clipboard!");
            }
        else
            COMPLAIN("Could not empty clipboard!");
        }
    else
        COMPLAIN("Could not open clipboard!");

    return iResult;
    }
#endif

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
                CloseClipboard();
                }
            else
                COMPLAIN("Insufficient memory for clipboard!");
            }
        else
            COMPLAIN("Could not empty clipboard!");
        }
    else
        COMPLAIN("Could not open clipboard!");

    return iResult;
    }

