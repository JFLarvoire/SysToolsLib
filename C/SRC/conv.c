/*****************************************************************************\
*                                                                             *
*   Filename	    conv.c						      *
*									      *
*   Contents	    Convert characters using Windows' conversion functions    *
*									      *
*   Notes:	    This file must be encoded with the Windows 1252 code page,*
*		    due to the presence of non-ASCII characters in the help.  *
*		    This allows building it without needing itself for	      *
*		    converting UTF-8 back to CP 1252.			      *
*		    							      *
*		    The default character sets (w for input, . for output)    *		    							      *
*		    are chosen to allow typing a Windows file on the console, *
*		    by running: type WINDOWS_FILE | conv		      *
*		    or simply: conv <WINDOWS_FILE			      *
*		    							      *
*   History								      *
*    2006-09-10 JFL Created this program.                                     *
*    2007-11-07 JFL Fixed a bug if more than 4096 bytes were read. V 1.01.    *
*    2007-11-10 JFL Added the ability to decode Mime encoded words. V 1.1.    *
*    2008-03-12 JFL Fixed a bug which caused free to generate an error.       *
*		    Suppressed a number of warnings. V 1.1a.		      *
*    2008-12-02 JFL Added support for UTF-7. V1.2.                            *
*    2008-12-10 JFL Added a workaround for a non-standard UTF-8. V1.21.       *
*    2009-06-22 JFL Added decoding of base64 headers. V1.22.                  *
*    2012-10-18 JFL Added my name in the help. Version 1.2.3.                 *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2016-08-23 JFL Added support for UTF-16.				      *
*		    Added . to specify the current code page.		      *
*    2016-08-24 JFL Added support for BOM addition/removal.                   *
*		    Added support for any code page number.		      *
*    2016-08-25 JFL Added the ability to convert a file in place.             *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.3"
#define PROGRAM_DATE    "2016-08-25"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		// Automatically defined when targeting a Win32 applic.

#define TARGET_WIN32

// if defined, the following flags inhibit the definitions in windows.h
#define NOGDICAPMASKS	  // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES	  // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES	  // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS	  // SM_*
#define NOMENUS 	  // MF_*
#define NOICONS 	  // IDI_*
#define NOKEYSTATES	  // MK_*
#define NOSYSCOMMANDS	  // SC_*
#define NORASTEROPS	  // Binary and Tertiary raster ops
#define NOSHOWWINDOW	  // SW_*
// #define OEMRESOURCE	     // OEM Resource values
#define NOATOM		  // Atom Manager routines
#define NOCLIPBOARD	  // Clipboard routines
#define NOCOLOR 	  // Screen colors
#define NOCTLMGR	  // Control and Dialog routines
#define NODRAWTEXT	  // DrawText() and DT_*
#define NOGDI		  // All GDI defines and routines
#define NOKERNEL	  // All KERNEL defines and routines
// #define NOUSER		  // All USER defines and routines
// #define NONLS		  // All NLS defines and routines
#define NOMB		  // MB_* and MessageBox()
#define NOMEMMGR	  // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE	  // typedef METAFILEPICT
#define NOMINMAX	  // Macros min(a,b) and max(a,b)
#define NOMSG		  // typedef MSG and associated routines
#define NOOPENFILE	  // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL	  // SB_* and scrolling routines
#define NOSERVICE	  // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND 	  // Sound driver routines
#define NOTEXTMETRIC	  // typedef TEXTMETRIC and associated routines
#define NOWH		  // SetWindowsHook and WH_*
#define NOWINOFFSETS	  // GWL_*, GCL_*, associated routines
#define NOCOMM		  // COMM driver routines
#define NOKANJI 	  // Kanji support stuff.
#define NOHELP		  // Help engine interface.
#define NOPROFILER	  // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX		  // Modem Configuration Extensions
#include <windows.h>

#include <io.h>

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

// Define WIN32 replacements for common Standard C library functions.
#define malloc(size) (void *)LocalAlloc(LMEM_FIXED, size)
#define realloc(pBuf, l) (void *)LocalReAlloc((HLOCAL)pBuf, l, LMEM_MOVEABLE)	// Returns fixed memory, despite the flag name.
#define free(pBuf) LocalFree((HLOCAL)pBuf)
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)

#include <Tchar.h>
#include <strsafe.h>

int _cdecl ReportWin32Error(_TCHAR *pszExplanation, ...);

int fail(char *pszFormat, ...)
    {
    va_list vl;
    int n = 0;

    va_start(vl, pszFormat);
    n += vfprintf(stderr, pszFormat, vl);    // Not thread-safe on WIN32 ?!?
    va_end(vl);
    fprintf(stderr, "\n");
    exit(1); \
    }
#define FAIL(msg) fail("%s", msg);
#define WIN32FAIL(msg) fail("Error %d: %s", GetLastError(), msg)

#endif

/********************** End of OS-specific definitions ***********************/

// My favorite string comparison routines.
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define streqi(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

#define message(msg) do {fprintf(stderr, "%s", msg); fflush(stderr);} while (0)
#define verbose(args) if (iVerbose) do {fprintf args; fflush(stderr);} while (0)
#define debug(args) if (iVerbose > 1) do {fprintf args; fflush(stderr);} while (0)

#define BLOCKSIZE (4096)	// Number of characters that will be allocated in each loop.

/* Global variables */

int iVerbose = 0;

/* Function prototypes */

int IsSwitch(char *pszArg);
int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM);
size_t MimeWordDecode(char *pszBuf, size_t nBufSize, char *pszCharEnc);

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
|    2006-09-10 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

char szUsage[] = "\
\n\
conv version " PROGRAM_VERSION " " PROGRAM_DATE "\n\
\n\
Convert characters from various character sets.\n\
\n\
Usage:\n\
\n\
    conv [options] [ics [ocs [inputfile [outputfile]]]]\n\
\n\
Options:\n\
  -?        This help\n\
  -b        Add Byte Order Mark if needed\n\
  -B        Remove Byte Order Mark if present\n\
  -bak      When used with -same, create a backup file of the input file\n\
  -same     Convert a file in place\n\
  -v        Display verbose information\n\
\n\
ics = Input Character Set, or code page number. Default = Windows code page\n\
ocs = Output Character Set, or code page number. Default = cmd.exe code page\n\
inputfile = Input file pathname. Default or - = Read from stdin\n\
outputfile = Output file pathname. Default or - = Write to stdout\n\
\n\
Character Sets:\n\
  .         The current cmd.exe Code Page (default)\n\
  w         Windows   (CP 1252 for the en-us version of Windows)\n\
  d         DOS       (CP 437 for the en-us version of Windows)\n\
  m         Macintosh (CP 10000)\n\
  7         UTF-7     (CP 65000)\n\
  8         UTF-8     (CP 65001)\n\
  16        UTF-16    (CP 1200)\n\
  s         Symbol    (CP 42)\n\
\n\
If one of the symbolic character sets above is specified, also decodes the\n\
mime encoded strings in the input stream. Not done if using numeric CP numbers.\n\
\n"
"Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
/* Note: The ç above is OK, as we'll use our own conversion routine to display this help. */
;

void usage(int iRet) {
  size_t  nBufSize = strlen(szUsage) + 1;
  char *pszBuffer = (char*)malloc(2*nBufSize);

  // Convert the help message to the console code page.
  ConvertCharacterSet(szUsage, nBufSize, pszBuffer, 2*nBufSize, "w", ".", 0);
  message(pszBuffer);             /* Display help */
  exit(iRet);
}

int __cdecl main(int argc, char *argv[]) {
  size_t  nRead = 0;		// Number of characters read in one loop.
  size_t  nTotal = 0;		// Total number of characters read.
  size_t  nBufSize = BLOCKSIZE;
  char   *pszBuffer = (char*)malloc(BLOCKSIZE);
  int     i;
  char   *pszInType = NULL;	// Input character set Windows/Mac/DOS/UTF-8
  char   *pszOutType = NULL;	// Output character set Windows/Mac/DOS/UTF-8
  int     iBOM = 0;		// 1=Output BOM; -1=Output NO BOM; 0=unchanged
  FILE *sf = NULL;		// Source file handle
  FILE *df = NULL;		// Destination file handle
  char *pszInName = NULL;	// Source file name
  char *pszOutName = NULL;	// Destination file name
  char szBakName[FILENAME_MAX+1];
  int iSameFile = FALSE;	// Modify the input file in place.
  int iBackup = FALSE;		// With iSameFile, back it up first.
  int iKeepTime = TRUE;		// If true, set the out file time = in file time.
  struct stat sInTime = {0};

  if (!pszBuffer) FAIL("Not enough memory.");

    /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    _setmode( _fileno( stderr ), _O_BINARY );
#endif

  /* Process arguments */

  for (i=1; i<argc; i++) {
    if (IsSwitch(argv[i])) {	    /* It's a switch */
      char *pszOpt = argv[i]+1;
      if (streq(pszOpt, "?")) {		/* -?: Help */
      	usage(0);
      }
      if (streq(pszOpt, "b")) {		/* -b: Output a BOM */
	iBOM = 1;
	continue;
      }
      if (streq(pszOpt, "B")) {		/* -B: Output no BOM */
	iBOM = -1;
	continue;
      }
      if (streq(pszOpt, "bak")) {	/* -bak: Backup the input file */
	iBackup = TRUE;
	continue;
      }
      if (streq(pszOpt, "d")) {		/* -d: Debug */
	iVerbose += 2;
	continue;
      }
      if (streq(pszOpt, "same")) {	/* -same: Output to the input file */
	iSameFile = TRUE;
	continue;
      }
      if (streq(pszOpt, "v")) {		/* -v: Verbose */
	iVerbose += 1;
	continue;
      }
      // Unsupported switches are ignored
      continue;
    }
    if (!pszInType) {
	pszInType = argv[i];
	continue;
    }
    if (!pszOutType) {
	pszOutType = argv[i];
	continue;
    }
    if (!pszInName) {
	pszInName = argv[i];
	continue;
    }
    if (!pszOutName) {
	pszOutName = argv[i];
	continue;
    }
    // Unexpected arguments are ignored
  }

  if (!pszInType) {
      pszInType = "w";
  }
  if (!pszOutType) {
      pszOutType = ".";
  }
  verbose((stderr, "Input character set: %s\n", pszInType));
  verbose((stderr, "Output character set: %s\n", pszOutType));

  if ((!pszInName) || streq(pszInName, "-")) {
    sf = stdin;
    iSameFile = FALSE;	// Meaningless in this case. Avoid issues below.
  } else {
    sf = fopen(pszInName, "rb");
    if (!sf) fail("Can't open file %s\r\n", pszInName);
    stat(pszInName, &sInTime);
  }
  if ((!pszOutName) || streq(pszOutName, "-")) {
    if (!iSameFile) df = stdout;
  } else {
    iSameFile = streqi(pszOutName, pszInName); // Ignore the -iSameFile argument
  }
  if (iSameFile) {
    char *pc;

    fclose(sf);		/* We've got to move it or back it up first */

    if (iBackup) {	/* Create an *.bak file in the same directory */
      strcpy(szBakName, pszInName);
      pc = strrchr(szBakName, '.');
      if (pc && !strchr(pc, '\\'))	/* If extension in name & not in path */
	  strcpy(pc, ".bak"); 		/* Change extension to .bak */
      else
	  strcat(szBakName, ".bak");	/* Set extension to .bak */
      unlink(szBakName); 		/* Remove the .bak if already there */
      debug((stderr, "Rename \"%s\" \"%s\"\n", pszInName, szBakName));
      rename(pszInName, szBakName);	/* Rename the source as .bak */
    } else {		/* Move the source file to the %TEMP% directory */
      /* TO DO: Manage possible errors in the operations below */
      strcpy(szBakName, getenv("TEMP"));
      strcat(szBakName, "\\");
      pc = strrchr(pszInName, '\\');	/* Find the last path separator */
      if (pc) pc += 1; else pc = pszInName; /* Locate the file name */
      if (pc[0] && (pc[1]==':')) pc += 2;   /* Skip the drive letter, if any */
      strcat(szBakName, pc);
      unlink(szBakName); 		/* Remove the .bak if already there */
      debug((stderr, "Move \"%s\" \"%s\"\n", pszInName, szBakName));
#ifdef _WIN32
      MoveFile(pszInName, szBakName);
#else
      system("move /Y \"pszInName\" \"szBakName\");
#endif
    }

    sf = fopen(szBakName, "rb");
    if (!sf) fail("Can't open file %s\r\n", szBakName);
    pszOutName = pszInName;
  }
  if (!df) {
    df = fopen(pszOutName, "wb");
    if (!df) {
      if (sf != stdout) fclose(sf);
      fail("Can't open file %s\r\n", pszOutName);
    }
  }

  /* Go for it */

  while (!feof(sf)) {
    nRead = fread(pszBuffer+nTotal, 1, BLOCKSIZE, sf);
    verbose((stderr, "Read %d input bytes.\n", (int)nRead));
    nTotal += nRead;
    if ((nTotal+BLOCKSIZE)>nBufSize) {
      if (!((pszBuffer = (char*)realloc(pszBuffer, nTotal + BLOCKSIZE)))) {
	FAIL("Not enough memory.");
      }
      nBufSize += BLOCKSIZE;
    }
    Sleep(0);	    // Release end of time-slice
  }
  if (nTotal > 0) {
    size_t nOutBufSize = 2*nTotal + 4;	// Size may double for ansi -> utf8 cases
    size_t nOutputSize;		// Size actually used in the out. buf.
    char *pszOutBuf = (char*)malloc(nOutBufSize);
    char *pszCharEnc = NULL;	// Character encoding

    if (!pszOutBuf) FAIL("Not enough memory.");

    // Optionally decode mime encoded words left in
    switch (tolower(*pszInType)) {
      case 'd':
	pszCharEnc = "ms-dos";
	break;
      case 'm':
	pszCharEnc = "macintosh";
	break;
      case 's':
	pszCharEnc = "symbol";
	break;
      case 'w':
	pszCharEnc = "windows-1252";
	break;
      default:
	if (sscanf(pszInType, "%d", &i)) {
	  switch (i) {
	    case 7:
	      pszCharEnc = "utf-7";
	      break;
	    case 8:
	      pszCharEnc = "utf-8";
	      { // Workaround for the unknown \xC3\x20 sequence: Use \xC3\xA0 instead.
		size_t n;
		for (n=0; n<(nTotal-1); n++)
		    if (memcmp(pszBuffer+n, "\xC3\x20", 2) == 0) pszBuffer[++n] = '\xA0';
	      }
	      break;
	    case 16:
	      pszCharEnc = "utf-16";
	      break;
	    default:
	      break;
	  }
	}
	break;
    }
    if (pszCharEnc) {
      nTotal = MimeWordDecode(pszBuffer, nTotal, pszCharEnc);
    }

    // Use Windows' character set conversion routine.
    nOutputSize = ConvertCharacterSet(pszBuffer, nTotal, pszOutBuf, nOutBufSize,
					    pszInType, pszOutType, iBOM);

    if (!fwrite(pszOutBuf, nOutputSize, 1, df)) {
      WIN32FAIL("Cannot write to the output file.");
    }
  }

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);

  // if (iSameFile && !iBackup) unlink(szBakName);     /* Don't keep the %TEMP% copy */

  if ((sf != stdin) && (df != stdout) && iKeepTime) {
    struct utimbuf sOutTime = {0};
    sOutTime.actime = sInTime.st_atime;
    sOutTime.modtime = sInTime.st_mtime;
    utime(pszOutName, &sOutTime);
  }

  verbose((stderr, "Exiting\n"));
  return 0;
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
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg)
    {
    switch (*pszArg)
	{
	case '-':
#if defined(_WIN32) || defined(_MSDOS)
	case '/':
#endif
	    return (pszArg[1] != '\0');
	default:
	    return FALSE;
	}
    }

#ifdef _WIN32		// Automatically defined when targeting a Win32 applic.

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ReportWin32Error					      |
|                                                                             |
|   Description:    Output a message with a description of the last error.    |
|                                                                             |
|   Parameters:     _TCHAR *pszExplanation	Context description	      |
|                   ...				Optional arguments to format  |
|                                                                             |
|   Returns:	    The number of characters output.        		      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    1998/11/19 JFL Created this routine.				      |
|    2005/06/10 JFL Added the message description, as formatted by the OS.    |
|    2007/07/03 JFL Updated to support both ASCII and Unicode modes.	      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl ReportWin32Error(_TCHAR *pszExplanation, ...) {
    _TCHAR szErrorMsg[1024];
    va_list pArgs;
    int n;
    LPVOID lpMsgBuf;
    DWORD dwErr = GetLastError();
    HRESULT hRes;

    if (FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    dwErr,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL ))	{ // Display both the error code and the description string.
	hRes = StringCbPrintf(szErrorMsg, sizeof(szErrorMsg),
		                  _T("Error %ld: %s"), dwErr, lpMsgBuf);
	LocalFree( lpMsgBuf ); // Free the buffer.
    } else { // Error, we did not find a description string for this error code.
	hRes = StringCbPrintf(szErrorMsg, sizeof(szErrorMsg), _T("Error %ld: "), dwErr);
    }
    if (SUCCEEDED(hRes)) {
	n = lstrlen(szErrorMsg);
    } else {
	n = 0;
    }

    va_start(pArgs, pszExplanation);
    hRes = StringCbVPrintf(szErrorMsg+n, sizeof(szErrorMsg) - (n*sizeof(TCHAR)),
		                   pszExplanation, pArgs);
    va_end(pArgs);

    if (FAILED(hRes)) lstrcpy(szErrorMsg, _T("Failed to format the error"));

    return _tprintf(_T("%s"), szErrorMsg);
}

#endif // defined(_WIN32)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ConvertCharacterSet					      |
|                                                                             |
|   Description:    Convert the characters                      	      |
|                                                                             |
|   Parameters:     char *pszInput          The input buffer                  |
|                                                                             |
|   Returns:	 	                                                      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2006-09-10 JFL Created this routine.				      |
|    2016-08-23	JFL Added support for UTF-16.				      |
|		    Added . to specify the current code page.		      |
|    2016-08-24 JFL Added support for BOM addition/removal.                   |
|		    Added support for any code page number.		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32		// Automatically defined when targeting a Win32 applic.

#ifndef CP_UTF16	// Not defined in WinNls.h for Windows SDK 8.1
#define CP_UTF16 1200	// "Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications"
#endif

int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM)
    {
    wchar_t *pszWideBuf;             // Intermediate buffer for wide characters.
    size_t nWideSize = (2*nInputSize) + 4; // Size in bytes of that wide buffer.
    UINT uCPin=0, uCPout=0;
    int nWide, nOut;

    debug((stderr, "ConvertCharacterSet(pszIn, %ld, pszOut, %ld, %s, %s, %d)\n", \
           (long)nInputSize, (long)nOutputSize, pszInputSet, pszOutputSet, iBOM));

    switch (tolower(*pszInputSet)) {
      case '.':
	uCPin = GetConsoleOutputCP();
	break;
      case 'w':
	uCPin = CP_ACP;		// ANSI
	break;
      case 'm':
	uCPin = CP_MACCP;		// Mac
	break;
      case 'd':
	uCPin = CP_OEMCP;		// DOS
	break;
      case 's':
	uCPin = CP_SYMBOL;	// Symbol
	break;
      default:
	if (!sscanf(pszInputSet, "%u", &uCPin)) {
	  FAIL("Unknown input character set");
	}
	switch (uCPin) {
	  case 7:
	    uCPin = CP_UTF7;	// UTF-7
	    break;
	  case 8:
	    uCPin = CP_UTF8;	// UTF-8
	    break;
	  case 16:
	    uCPin = CP_UTF16;	// UTF-16
	    break;
	  default:
	    break;
	}
	break;
    }
    switch (tolower(*pszOutputSet)) {
      case '.':
	uCPout = GetConsoleOutputCP();
	break;
      case 'w':
	uCPout = CP_ACP;	// ANSI
	break;
      case 'm':
	uCPout = CP_MACCP;	// Mac
	break;
      case 'd':
	uCPout = CP_OEMCP;	// DOS
	break;
      case 's':
	uCPout = CP_SYMBOL;	// Symbol
	break;
      default:
	if (!sscanf(pszOutputSet, "%u", &uCPout)) {
	  FAIL("Unknown output character set");
	}
	switch (uCPout) {
	  case 7:
	    uCPout = CP_UTF7;	// UTF-7
	    break;
	  case 8:
	    uCPout = CP_UTF8;	// UTF-8
	    break;
	  case 16:
	    uCPout = CP_UTF16;	// UTF-16
	    break;
	  default:
	    break;
	}
	break;
    }

    verbose((stderr, "uCPin = %d , uCPout = %d\n", (int)uCPin, (int)uCPout));

    /* Allocate a buffer for an intermediate UTF-16 string */
    pszWideBuf = (wchar_t *)malloc(nWideSize);
    pszWideBuf += 1;	// Leave room for adding a BOM if needed
    nWideSize -= 2;

    /* Convert to intermediate wide characters */
    if (uCPin != CP_UTF16) {
      nWide = MultiByteToWideChar(uCPin, 0, pszInput, (int)nInputSize, pszWideBuf, (int)nWideSize);
      if (nWide == 0)
	WIN32FAIL("Cannot convert the input!");
      else if (nWide == ERROR_NO_UNICODE_TRANSLATION)
	WIN32FAIL("Invalid UTF-8 input!");
      else verbose((stderr, "Input conversion to Unicode returned %d wide characters.\n", nWide));
    } else {
      memcpy((char *)pszWideBuf, pszInput, nInputSize);
      nWide = nInputSize / 2;
    }

    /* Add or remove the BOM if requested or needed */
    switch (uCPout) {
    case CP_UTF7:
    case CP_UTF8:
    case CP_UTF16:
      break;	// Apply the user request for all Unicode encodings
    default:	// Any other code page requires removing the BOM, if any
      iBOM = -1;
      break;
    }
    switch (iBOM) {
    case -1:
      if (*pszWideBuf == L'\uFEFF') {	// If there's a BOM, then remove it
      	pszWideBuf += 1;
      	nWideSize -= 2;
      	nWide -= 1;
      }
      break;
    case 1:
      if (*pszWideBuf != L'\uFEFF') {	// If there's no BOM, then add one
      	pszWideBuf -= 1;
      	nWideSize += 2;
      	*pszWideBuf = L'\uFEFF';
      	nWide += 1;
      }
      break;
    case 0:				// Leave the BOM (if present) unchanged
    default:
      break;
    }

    /* Convert back to ANSI characters */
    if (uCPout != CP_UTF16) {
      nOut = WideCharToMultiByte(uCPout, 0, pszWideBuf, nWide, pszOutput, (int)nOutputSize, NULL, NULL);
      if (nOut == 0)
	  WIN32FAIL("Cannot convert the output!");
      else verbose((stderr, "Output conversion from Unicode returned %d bytes.\n", nOut));
    } else {
      nOut = nWide * 2;
      memcpy(pszOutput, (char *)pszWideBuf, nOut);
    }

    return nOut;
    }

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    Base64Decode					      |
|                                                                             |
|   Description:    Decode a string encoded in base64.			      |
|                                                                             |
|   Parameters:     char *pszOutput         The output buffer                 |
|                   size_t nOutputSize      The output buffer size            |
|                   char *pszInput          The input buffer encoded in b.64  |
|                   size_t nInputSize       The input buffer size             |
|                                                                             |
|   Returns:	    The length of the output string                           |
|                                                                             |
|   Notes:	    See RFC 1421 for base 64 encoding definition.             |
|                   This routine silently ignores input errors.               |
|                                                                             |
|   History:								      |
|    2009/06/22 JFL Created this routine.				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

unsigned char alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t Base64Decode(
  char *pszOutput,
  size_t nOutputSize,
  const char *pszInput,
  size_t nInputSize
) {
  static char inalphabet[256], decoder[256];
  int i, c, char_count;
  long bits;
  char *pszOutput0 = pszOutput;

  for (i = 0; i < (sizeof alphabet) - 1; i++) {
    inalphabet[alphabet[i]] = 1;
    decoder[alphabet[i]] = (char)i;
  }

  nOutputSize -= 1; // Reserver 1 byte for the final NUL.
  char_count = 0;
  bits = 0;
  while (nInputSize-- && nOutputSize) {
    c = *(pszInput++);
    if (c == '=') break; // End of Mime sequence
    if (!inalphabet[c]) continue; // Ignore error silently
    bits |= decoder[c];
    char_count++;
    if (char_count == 4) {
      *(pszOutput++) = (char)(bits >> 16);
      if (!--nOutputSize) break;
      *(pszOutput++) = (char)(bits >> 8);
      if (!--nOutputSize) break;
      *(pszOutput++) = (char)(bits);
      if (!--nOutputSize) break;
      bits = 0;
      char_count = 0;
    } else {
      bits <<= 6;
    }
  }
  if (nOutputSize) switch (char_count) {
    case 1: // Error: base64 encoding incomplete: at least 2 bits missing
      break;
    case 2:
      *(pszOutput++) = (char)(bits >> 10);
      nOutputSize -= 1;
      break;
    case 3:
      *(pszOutput++) = (char)(bits >> 16);
      if (!--nOutputSize) break;
      *(pszOutput++) = (char)(bits >> 8);
      nOutputSize -= 1;
      break;
  }
  *pszOutput = '\0';
  return pszOutput - pszOutput0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MimeWordDecode					      |
|                                                                             |
|   Description:    Remove Mime encoded words for a given character set.      |
|                                                                             |
|   Parameters:     char *pszBuf            The input buffer                  |
|                   size_t nBufSize         The input buffer content size     |
|                   char *pszCharEnc        Character encoding. Examples:     |
|                                           utf-8, windows-1252, etc...       |
|   Returns:	 	                                                      |
|                                                                             |
|   Notes:	    See RFC 2047 for Mime encoded word specifications.	      |
|                   (Used in email headers)                                   |
|                                                                             |
|   History:								      |
|    2007/10/09 JFL Created this routine.				      |
|    2009/06/22 JFL Added the decoding of words encoded in base64.            |
|                   Bugfix: Skip spaces between adjacent encoded words only.  |
*                                                                             *
\*---------------------------------------------------------------------------*/

char *stristr(char *pszString, char *pszSubString)
{
  int dif;
  char *psz;
  size_t n = strlen(pszSubString);

  for (psz=pszString; *psz; psz++)
  {
    dif = _strnicmp(psz, pszSubString, n);
    if (!dif) return psz;
  }

  return NULL;
}

size_t MimeWordDecode(char *pszBuf, size_t nBufSize, char *pszCharEnc)
{
  char *pszBuf2 = (char*)malloc(nBufSize + 1);
  char *pszHeader = (char*)malloc(strlen(pszCharEnc) + 8);
  char *psz00, *psz0, *psz1, *psz20, *psz2;
  char cEnc; // Character encoding. Q or B.
  size_t nHeader;
  char c;
  size_t n;
  int i;

  debug((stderr, "MimeWordDecode(0x%p, %lu, \"%s\").\n", pszBuf, (unsigned long)nBufSize, pszCharEnc));

  if (!pszBuf2 || !pszHeader) FAIL("Not enough memory for Mime word decoding!");
  nHeader = sprintf(pszHeader, "=?%s?", pszCharEnc);

  memcpy(pszBuf2, pszBuf, nBufSize);
  pszBuf2[nBufSize] = '\0';
  psz00 = psz0 = pszBuf2;
  psz20 = psz2 = pszBuf;

  // Scan the input data and decode it.
  while (psz1 = stristr(psz0, pszHeader))
  {
    debug((stderr, "while(): psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    if ((size_t)(psz0-psz00) >= nBufSize) FAIL("Oops, went too far!");
    // Copy input up to the Mime word header.
    n = psz1-psz0;
    strncpy(psz2, psz0, n);
    psz0 += n + nHeader;
    psz2 += n;
    // We've seen case for such words in quoted strings. Remove the opening quote.
    if (n && (*(psz2-1) == '"')) psz2 -= 1;
    // Get the encoding byte.
    cEnc = *(psz0++);
    if (*(psz0++) != '?') // Skip the second ?
      fail("Bad Mime encoded-word header: %20.20s", psz0-(nHeader+2));
    // Get the rest of the encoded word.
    for (psz1=psz0; (c=*psz1)>' '; psz1++) ;
    n = psz1-psz0;
    debug((stderr, "Encoding: psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    // Decode the word
    switch (toupper(cEnc))
    {
    case 'Q': // Quoted-printable
      for ( ; psz0 < psz1; psz0++)
      {
        debug((stderr, "for(): psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
	if ((size_t)(psz0-psz00) > nBufSize) FAIL("Oops, went too far!");
	c = *psz0;
	switch (c)
	{
	case '?':
	  c = *(psz0+1);
	  if (c == '=') psz0++;
	  break;
	case '_':
	  *(psz2++) = ' ';
	  break;
	case '=':
	  i = '?';
	  psz0 += sscanf(++psz0, "%02X", &i);
	  *(psz2++) = (char)i;
	  break;
	default:
	  *(psz2++) = c;
	  break;
	}
      }
      break;
    case 'B': // Base64
      psz2 += Base64Decode(psz2, nBufSize-(psz2-psz20), psz0, n);
      psz0 += n;
      break;
    default:
      FAIL("Unexpected character encoding");
      break;
    }
    // Skip the following separator spaces if there's another encoded word afterwards
    psz1 = psz0;
    while (isspace(*psz1)) psz1++;
    if (!strncmp(psz1, "=?", 2)) psz0 = psz1;
  }
  // Output the end of the data, if any
  n = nBufSize-(psz0-psz00);
  memcpy(psz2, psz0, n);
  psz2 += n;

  // Cleanup
  free(pszBuf2);
  free(pszHeader);

  return psz2-psz20; // The output length.
}

