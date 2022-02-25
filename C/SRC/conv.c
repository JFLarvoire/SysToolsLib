/*****************************************************************************\
*                                                                             *
*   Filename	    conv.c						      *
*									      *
*   Contents	    Convert characters using Windows' conversion functions    *
*									      *
*   Notes:	    The default character sets (w for input, . for output)    *
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
*    2016-08-25 JFL Added the ability to convert a file in place. V 1.3.      *
*    2016-09-06 JFL Updated help to display the actual current code pages.    *
*    2016-09-07 JFL Added option -t to copy the source file time.             *
*    2016-09-08 JFL Added function RegGetString for 98/XP compatibility.      *
*		    Bug fix: An error while replacing a file in place caused  *
*		    it to be truncated to size 0.			      *
*    2016-09-09 JFL Now a UTF-8 app, that can process any Unicode pathname.   *
*		    Renamed option -t as -st. Added option -V.		      *
*                   Use MsvcLibX debugging macros. Version 1.4.		      *
*    2016-09-13 JFL Added new routine IsSameFile to detect equiv. pathnames.  *
*    2016-09-14 JFL Make sure the debug stream is always in text mode.	      *
*    2016-09-15 JFL Fixed several warnings. V 1.4.2.			      *
*    2017-03-15 JFL Changed the default console output encoding to Unicode.   *
*		    Changed the arguments syntax, to simply type encoded files.
*		    Autodetect the input encoding by default. V 2.0.	      *
*    2017-05-31 JFL Added error message for failures to backup or rename the  *
*		    output files.					      *
*                   Display MsvcLibX library version in DOS & Windows. V2.0.1.*
*    2017-08-25 JFL Use strerror() for compatibility with Unix. Version 2.0.2.*
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.2.0.3.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.0.4.      *
*    2019-10-24 JFL Added option -z to stop input on Ctrl-Z.		      *
*    2019-11-01 JFL Added option -Z to append a Ctrl-Z (EOF) to the output.   *
*		    Version 2.1.					      *
*    2020-05-10 JFL Test IMultiLanguage2::DetectInputCodepage() in debug mode.*
*		    Version 2.1.1.					      *
*    2020-08-11 JFL Added the detection of UTF-8 and UTF-16 without BOM.      *
*    2020-08-12 JFL Change the default encoding to UTF-8 on Windows 10 >= 2019.
*    2020-08-17 JFL Fixed a memory allocation bug causing a debug mode crash. *
*		    Version 2.2.					      *
*    2020-08-18 JFL Factored-out routine GetTrueWindowsVersion().             *
*    2020-08-19 JFL Added type * for IMultiLanguage2::DetectInputCodepage().  *
*		    Version 2.3.					      *
*    2020-08-29 JFL Removed unused routine ReportWin32Error(). Version 2.3.1. *
*    2020-12-10 JFL Added option -= as a synonym for -same. Version 2.3.2.    *
*    2021-05-03 JFL Do not change the file time if nothing changed.           *
*		    Merged in the temp and backup file management code from   *
*                   detab, trim, and remplace. Version 2.4.                   *
*    2021-05-28 JFL Added support for conversions from/to UTF-16 & UTF-32.    *
*		    Added option -s to request a strict conversion.	      *
*		    Removed support for the obsolete Symbol code page.	      *
*		    Fixed the default encoding when writing to the console.   *
*		    Moved the heuristic for detecting the input code page to  *
*		    the new MsvcLibX routine GuessEncoding(), and improved it.*
*    2021-05-29 JFL Added a workaround for Windows Terminal limitations on    *
*		    displaying Unicode characters beyond \U10000.	      *
*		    Version 2.5.					      *
*    2021-05-29 JFL Added debug option -tdi to test DetectInputCodepage().    *
*    2021-06-01 JFL Added debug option -tge to test GetBufferEncoding().      *
*    2021-06-02 JFL Fixed the COM API result analysis. Still poor though.     *
*		    Version 2.5.1.					      *
*    2022-02-24 JFL Fixed the input pipe and redirection detection.           *
*		    Version 2.5.2.					      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Convert characters from one character set to another"
#define PROGRAM_NAME "conv"
#define PROGRAM_VERSION "2.5.2"
#define PROGRAM_DATE    "2022-02-24"

#define _CRT_SECURE_NO_WARNINGS /* Avoid Visual C++ 2005 security warnings */
#define STRSAFE_NO_DEPRECATE	/* Avoid VC++ 2005 platform SDK strsafe.h deprecations */

#define _UTF8_SOURCE		/* Use and display any Unicode character */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>
#include <unistd.h>
#include <iconv.h>
#include <errno.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#error "There is no version of this program for MS-DOS yet."

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

/* if defined, the following flags inhibit the definitions in windows.h */
#define NOGDICAPMASKS	  /* CC_*, LC_*, PC_*, CP_*, TC_*, RC_ */
#define NOVIRTUALKEYCODES /* VK_* */
#define NOWINMESSAGES	  /* WM_*, EM_*, LB_*, CB_* */
#define NOWINSTYLES	  /* WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_* */
#define NOSYSMETRICS	  /* SM_* */
#define NOMENUS 	  /* MF_* */
#define NOICONS 	  /* IDI_* */
#define NOKEYSTATES	  /* MK_* */
#define NOSYSCOMMANDS	  /* SC_* */
#define NORASTEROPS	  /* Binary and Tertiary raster ops */
#define NOSHOWWINDOW	  /* SW_* */
/* #define OEMRESOURCE	     // OEM Resource values */
#define NOATOM		  /* Atom Manager routines */
#define NOCLIPBOARD	  /* Clipboard routines */
#define NOCOLOR 	  /* Screen colors */
#define NOCTLMGR	  /* Control and Dialog routines */
#define NODRAWTEXT	  /* DrawText() and DT_* */
#define NOGDI		  /* All GDI defines and routines */
#define NOKERNEL	  /* All KERNEL defines and routines */
/* #define NOUSER		  // All USER defines and routines */
/* #define NONLS		  // All NLS defines and routines */
#define NOMB		  /* MB_* and MessageBox() */
#define NOMEMMGR	  /* GMEM_*, LMEM_*, GHND, LHND, associated routines */
#define NOMETAFILE	  /* typedef METAFILEPICT */
#define NOMINMAX	  /* Macros min(a,b) and max(a,b) */
#define NOMSG		  /* typedef MSG and associated routines */
#define NOOPENFILE	  /* OpenFile(), OemToAnsi, AnsiToOem, and OF_* */
#define NOSCROLL	  /* SB_* and scrolling routines */
#define NOSERVICE	  /* All Service Controller routines, SERVICE_ equates, etc. */
#define NOSOUND 	  /* Sound driver routines */
#define NOTEXTMETRIC	  /* typedef TEXTMETRIC and associated routines */
#define NOWH		  /* SetWindowsHook and WH_* */
#define NOWINOFFSETS	  /* GWL_*, GCL_*, associated routines */
#define NOCOMM		  /* COMM driver routines */
#define NOKANJI 	  /* Kanji support stuff. */
#define NOHELP		  /* Help engine interface. */
#define NOPROFILER	  /* Profiler interface. */
#define NODEFERWINDOWPOS  /* DeferWindowPos routines */
#define NOMCX		  /* Modem Configuration Extensions */
#include <windows.h>

#include <io.h>

/* Define WIN32 replacements for common Standard C library functions. */
/* 2020-08-17 Bug fix: Do NOT redefine malloc/realloc/free as this breaks the release of blocks indirectly allocated by C library functions */
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)
#define chmod	_chmod		/* This one is standard, but MSVC thinks it's not */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

#include <Tchar.h>
#include <strsafe.h>

#define FAIL(msg) fail("%s", msg);
#define WIN32FAIL(msg) fail("Error %d: %s", GetLastError(), msg)

DWORD GetTrueWindowsVersion(void); /* Extend GetVersion() */

/* Define easy to use functions for reading registry values, compatible with old versions of Windows */
LONG RegGetString(HKEY rootKey, LPCTSTR pszKey, LPCTSTR pszValue, LPTSTR pszBuf, size_t nBufSize); /* Returns the string size, or (-error). */

/* My favorite string comparison routines. */
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define strieq(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

/* C front end to COM C++ method IMultiLanguage2::DetectInputCodepage() */
#include <MLang.h>
#include <objbase.h>
#pragma comment(lib, "ole32.lib")
HRESULT DetectInputCodepage(DWORD dwFlags, DWORD dwPrefCP, char *pszBuffer, INT *piSize, DetectEncodingInfo *lpInfo, INT *pnInfos);

#endif /* defined(_WIN32) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#error "This program is for Windows only. Use iconv instead on Unix systems."

#endif /* defined(__unix__) */

/********************** End of OS-specific definitions ***********************/

#define verbose(args) if (iVerbose) do {fprintf args;} while (0)

#define BLOCKSIZE (4096)	/* Number of characters that will be allocated in each loop. */

#define CP_AUTODETECT ((UINT)-2)

/* Global variables */

int iVerbose = 0;
FILE *mf;			/* Message output file */

void fail(char *pszFormat, ...) { /* Display an error message, and abort leaving no traces behind */
  va_list vl;
  int n = fprintf(stderr, "Error: ");

  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);    /* Not thread-safe on WIN32 ?!? */
  va_end(vl);
  fprintf(stderr, "\n");

  exit(1);
}

/* Function prototypes */

int IsSwitch(char *pszArg);
int is_redirected(FILE *f);	/* Check if a file handle is the console */
int is_pipe(FILE *f);		/* Check if a file handle is a pipe */
int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM, DWORD dwFlags);
size_t MimeWordDecode(char *pszBuf, size_t nBufSize, char *pszCharEnc);
int IsSameFile(char *pszPathname1, char *pszPathname2);
int isEncoding(char *pszEncoding, UINT *pCP, char **ppszMime);
int file_exists(const char *pszName);

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

char szUsage[] = 
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
\n\
    conv [OPTIONS] [[ICS [OCS]] INFILE [OUTFILE|-same]]\n\
\n\
Options:\n\
  -?        This help\n\
  -b        Add Byte Order Mark if needed\n\
  -B        Remove Byte Order Mark if present\n\
  -bak      When used with -same, create a backup file of the input file\n"
#ifdef _DEBUG
"\
  -d        Output debug information\n"
#endif
"\
  -F        Do not use best fit characters (Ex: é -> e) for missing ones\n\
  -=|-same  Modify the input file in place. (Default: Automatically detected)\n\
  -st       Set the output file time to the same time as the input file\n\
  -v        Display verbose information\n\
  -V        Display this program version\n\
  -z        Stop input on a Ctrl-Z (aka. SUB or EOF) character\n\
  -Z        Append a Ctrl-Z (aka. SUB or EOF) to the output\n\
\n\
ICS = Input Character Set, or code page number. Default = Detect input encoding\n\
OCS = Output Character Set, or code page number. Default = cmd.exe code page\n\
INFILE = Input file pathname. Default or \"-\" = Read from stdin\n\
OUTFILE = Output file pathname. Default or \"-\" = Write to stdout\n\
\n\
Character Sets: One of the following codes, or a code page number such as 1252\n\
  ?         Detect the input data encoding w. a simple heuristic (dflt for input)\n\
  *         Detect the input data encoding w. Windows' IMultiLanguage2 COM API\n\
  .         Current Console CP (CP %d in this shell) (See note 1)\n\
  w         Windows System CP  (CP %d on this system) (See note 1)\n\
  d         DOS default CP     (CP %d on this system)\n\
  m         Macintosh CP       (CP %d on this system)\n\
  u         UTF-16             (CP 1200) (See note 1)\n\
  a         US-ASCII           (CP 20127)\n\
  7         UTF-7              (CP 65000)\n\
  8         UTF-8              (CP 65001) (See note 1)\n\
  16        UTF-16             (CP 1200) (See note 1)\n\
  32        UTF-32             (CP 12000) (See note 1)\n\
\n\
Notes:\n\
1) If not explicitely specified, the default output encoding is UTF-16 for\n\
output to the console; The current console code page for output to a pipe;\n\
UTF-8 for output to a file on Windows 10 2019 H1 or later; Windows system code\n\
page for output to a file on all older versions of Windows.\n\
2) If one of the symbolic character sets above is specified, also decodes the\n\
mime encoded strings in the input stream. Not done if using numeric CP numbers.\n\
\n"
"Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
;

void usage(int iRet) {
  size_t nBufSize = strlen(szUsage) + 1;
  size_t nBufSize1 = nBufSize + 32;
  char *pszBuffer = (char*)malloc(nBufSize1);
  char *pszKey = "SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage";
  UINT uConsoleCP = GetConsoleOutputCP();
  UINT uWindowsCP = GetACP();	/* From HKLM / [pszKey] / ACP */
  UINT uDosCP = GetOEMCP();	/* From HKLM / [pszKey] / OEMCP */
  UINT uMacCP = 10000;		/* From HKLM / [pszKey] / MACCP */
  char szMacCP[10] = "";
  LONG lErr;

  lErr = RegGetString(HKEY_LOCAL_MACHINE, pszKey, "MACCP", szMacCP, sizeof(szMacCP));
  if (!lErr) uMacCP = atoi(szMacCP);
  nBufSize1 = sprintf(pszBuffer, szUsage, uConsoleCP, uWindowsCP, uDosCP, uMacCP);
  printf("%s", pszBuffer);	/* Display help */
  exit(iRet);
}

#pragma warning(disable:4706)	/* Ignore the "assignment within conditional expression" warning */

int __cdecl main(int argc, char *argv[]) {
  size_t  nRead = 0;		/* Number of characters read in one loop. */
  size_t  nTotal = 0;		/* Total number of characters read. */
  size_t  nBufSize = BLOCKSIZE;
  char   *pszBuffer = (char*)malloc(BLOCKSIZE);
  int     i;
  char   *pszInType = NULL;	/* Input character set Windows/Mac/DOS/UTF-8 */
  char   *pszOutType = NULL;	/* Output character set Windows/Mac/DOS/UTF-8 */
  char   *pszDefaultType = "w";	/* Default encoding, if we don't know better */
  int     iBOM = 0;		/* 1=Output BOM; -1=Output NO BOM; 0=unchanged */
  FILE *sf = NULL;		/* Source file handle */
  FILE *df = NULL;		/* Destination file handle */
  char *pszInName = NULL;	/* Source file name */
  char *pszOutName = NULL;	/* Destination file name */
  char *pszTmpName = NULL;	/* Temporary file name */
  long lnChanges = 0;		/* Number of changes */
  char szBakName[FILENAME_MAX+1];
  int iSameFile = FALSE;	/* Modify the input file in place. */
  int iBackup = FALSE;		/* With iSameFile, back it up first. */
  int iCopyTime = FALSE;	/* If true, set the out file time = in file time. */
  struct stat sInTime = {0};
  char *pszPathCopy = NULL;
  char *pszDirName = NULL;	/* Output file directory */
  int iErr;
  int iCtrlZ = FALSE;		/* If true, stop input on a Ctrl-Z */
  int iCtrlZ2 = FALSE;		/* If true, append a Ctrl-Z to the output */
  DWORD dwFlags = 0;		/* Flags to pass to WideCharToMultiByte() */
  DEBUG_CODE(
  int iTestGuess = FALSE;	/* If TRUE, test GuessEncoding() & exit */
  int iTestDic = FALSE;		/* If TRUE, test DetectInputCodepage() & exit */
  )
  DWORD dwDicFlags = 0;		/* Flags to pass to DetectInputCodepage() test */
  DWORD cpDicPref = 0;		/* Preferred code page to pass to "" */
  

  if (!pszBuffer) {
fail_no_mem:
    FAIL("Not enough memory.");
  }

  /* Open a new message file stream for debug and verbose messages */
  if (is_redirected(stdout)) {	/* If stdout is redirected to a file or a pipe */
    /* Then use stderr to make sure they're visible. */
    /* Drawback: Some scripting shells (Ex: tclsh) will think our program has failed. */
    mf = stderr;
  } else {
    /* Else use stdout to avoid the above drawback. */
    /* This requires duplicating the handle, to make sure it remains in text mode,
       as stdout may be switched to binary mode further down */
    mf = fdopen(dup(fileno(stdout)), "wt");
    /* Disable buffering in both files, else the output may not appear in the programmed order */
    setvbuf(stdout, NULL, _IONBF, 0); /* Disable buffering for stdio */
    setvbuf(mf, NULL, _IONBF, 0); /* Disable buffering for dup of stdio */
  }

  /* Process arguments */

  for (i=1; i<argc; i++) {
    char *pszArg = argv[i];
    if (IsSwitch(pszArg)) {	    /* It's a switch */
      char *pszOpt = pszArg+1;
      if (streq(pszOpt, "?")) {		/* -?: Help */
      	usage(0);
      }
      if (streq(pszOpt, "A")) {		/* Output ANSI */
	pszOutType = "a";
	continue;
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
#ifdef _DEBUG
      if (streq(pszOpt, "d")) {
	DEBUG_ON();
	iVerbose = TRUE;
	continue;
      }
#endif
      if (streq(pszOpt, "F")) {		/* -F: No best fit characters */
	dwFlags |= WC_NO_BEST_FIT_CHARS;
	continue;
      }
      if (streq(pszOpt, "O")) {		/* Output OEM type */
	pszOutType = "o";
	continue;
      }
      if (   streq(pszOpt, "=")
	  || streq(pszOpt, "same")
	  || streq(pszOpt, "-same")) {	/* -same: Output to the input file */
	iSameFile = TRUE;
	continue;
      }
      if (streq(pszOpt, "st")) {	/* -st: Copy the input file time to the output file */
	iCopyTime = TRUE;
	continue;
      }
#ifdef _DEBUG
      if (streq(pszOpt, "tdc")) {	/* -tdc: Test DupAndConvert() */
	if ((i+1) < argc) {
	  BOOL bUsedDef;
	  pszArg = strdup(argv[++i]);
	  printf("CP_UTF8: %s\n", pszArg);
	  pszArg = DupAndConvertEx(pszArg, CP_UTF8, CP_ACP, dwFlags, NULL, &bUsedDef);
	  printf("CP_ACP:  %s\n", pszArg);
	  printf("bUsedDef = %s\n", bUsedDef ? "TRUE" : "FALSE");
	}
	return 0;
      }
      if (streq(pszOpt, "tdi")) {	/* -tdc: Test DetectInputCodepage() */
      	iTestDic = TRUE;
      	pszInType = "*";
	if ((i+1) < argc) dwDicFlags = (DWORD)strtol(argv[++i], NULL, 16);
	if ((i+1) < argc) cpDicPref = (DWORD)atoi(argv[++i]);
	continue;
      }
      if (streq(pszOpt, "tge")) {	/* -tdc: Test GetBufferEncoding() */
      	iTestGuess = TRUE;
      	pszInType = "?";
	continue;
      }
#endif
      if (streq(pszOpt, "u")) {		/* Output UTF16 */
	pszOutType = "u";
	continue;
      }
      if (streq(pszOpt, "U")) {		/* Output UTF8 */
	pszOutType = "8";
	continue;
      }
      if (streq(pszOpt, "v")) {		/* -v: Verbose */
	iVerbose += 1;
	continue;
      }
      if (streq(pszOpt, "V")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
#ifdef _DEBUG
      if (streq(pszOpt, "xd")) {
	XDEBUG_ON();
	iVerbose = TRUE;
	continue;
      }
#endif
      if (streq(pszOpt, "z")) {		/* -z: Stop input on Ctrl-Z */
	iCtrlZ = TRUE;
	continue;
      }
      if (streq(pszOpt, "Z")) {		/* -z: Append a Ctrl-Z to the output */
	iCtrlZ2 = TRUE;
	continue;
      }
      /* Unsupported switches are ignored */
      continue;
    }
    if ((!pszInType) && isEncoding(pszArg, NULL, NULL)) {
      pszInType = pszArg;
      continue;
    }
    if ((!pszOutType) && isEncoding(pszArg, NULL, NULL)) {
      pszOutType = pszArg;
      continue;
    }
    if (!pszInName) {
      pszInName = pszArg;
      continue;
    }
    if (!pszOutName) {
      pszOutName = pszArg;
      continue;
    }
    /* Unexpected arguments are ignored */
    fprintf(stderr, "Warning: Unexpected argument ignored: %s\n", pszArg);
  }

  /* Report what the message stream is */
  DEBUG_CODE(
    if (mf == stderr) {	/* If stdout is redirected to a file or a pipe */
      DEBUG_FPRINTF((mf, "// Debug output sent to stderr.\n"));
    } else {
      DEBUG_FPRINTF((mf, "// Debug output sent to file #%d.\n", fileno(mf)));
    }
  )

  /* If we're on a recent version of Windows 10, update the default encoding to UTF-8 */
#ifdef _WIN32
  {
    DWORD dwVersion = GetTrueWindowsVersion();
    unsigned major = (unsigned)(LOBYTE(LOWORD(dwVersion)));
    unsigned minor = (unsigned)(HIBYTE(LOWORD(dwVersion)));
    unsigned build = (unsigned)(HIWORD(dwVersion));
    if ( (major > 10) || ((major == 10) && ((minor > 0) || (build >= 18298)))) {
      /* Since Windows 10 build 18298 (2018/12/10), Notepad defaults to UTF-8 without BOM for new files */
      /* https://blogs.windows.com/windowsexperience/2018/12/10/announcing-windows-10-insider-preview-build-18298/ */
      pszDefaultType = "8";
    }
    DEBUG_FPRINTF((mf, "pszDefaultType = \"%s\"\n", pszDefaultType));
  }
#endif /* defined(_WIN32) */

  if (!pszInType) {
    pszInType = "?";	/* Detect the input data encoding */
  }
  if (!pszOutType) {
    if (((!pszOutName) || streq(pszOutName, "-")) && (is_pipe(stdout) || isatty(_fileno(stdout)))) {
      pszOutType = ".";
    } else { /* Writing to a file */
      pszOutType = pszDefaultType;
    }
  }
  verbose((mf, "Input type argument: %s\n", pszInType));
  verbose((mf, "Output type argument: %s\n", pszOutType));

  /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
  _setmode( _fileno( stdin ), _O_BINARY );
  fflush(stdout); /* Make sure any previous output is done in text mode */
  if ((*pszOutType == '.') && isatty(1)) {
    DEBUG_FPRINTF((mf, "// Optimizing output to the console by forcing it to be UTF-8\n"));
    pszOutType = "8"; /* Convert to UTF-8 to output Unicode to the console, that MsvcLibX initialized in wide mode */
    if (!iBOM) iBOM = -1; /* Remove the BOM, which is not correctly interpreted by the console */
    /* As of 2021-05-29 the Windows Terminal fails to output Unicode characters in supplemental planes
       (i.e. characters beyond \U10000) when writing to the console in wide mode.
       But this works in code page 65001 when writing UTF-8 in binary mode.
       The old console fails to write them in all cases.
       So switch to binary mode when in code page 65001, to at least correctly display 
       supplemental characters in Windows Terminal. */
    if (GetConsoleOutputCP() == 65001) {
      DEBUG_FPRINTF((mf, "// Make sure Unicode chars beyond \\U10000 get displayed correctly\n"));
      _setmode( _fileno( stdout ), _O_BINARY );
    }
  } else {
    DEBUG_FPRINTF((mf, "// Make sure the output is not translated by the C library\n"));
    _setmode( _fileno( stdout ), _O_BINARY );
  }
#endif

  if ((!pszInName) || streq(pszInName, "-")) {
    sf = stdin;
    iSameFile = FALSE;	/* Meaningless in this case. Avoid issues below. */
  } else {
    sf = fopen(pszInName, "rb");
    if (!sf) fail("Can't open file %s\n", pszInName);
    stat(pszInName, &sInTime);
  }
  if ((!pszOutName) || streq(pszOutName, "-")) {
    if (pszOutName) iSameFile = FALSE;	/* If pszOutName is "-", iSameFile is meaningless */
    if (!iSameFile) {
      df = stdout;
    } else {
      pszOutName = pszInName;
    }
  } else { /*  Ignore the -iSameFile argument. Instead, verify if they're actually the same. */
    iSameFile = IsSameFile(pszInName, pszOutName);
    if (iBackup && !file_exists(pszOutName)) iBackup = FALSE; /* There's nothing to backup */
  }
  if (iSameFile || iBackup) { /* Then write to a temporary file */
    int iFile;
    /* But do as if we were writing directly to the target file.
       Test the write rights before wasting time on the conversion */
    df = fopen(pszOutName, "r+");
    if (!df) goto open_df_failed;
    fclose(df);
    df = NULL;
    /* OK, we have write rights, so go ahead with the conversion */
    DEBUG_FPRINTF((mf, "// %s. Writing to a temp file.\n", iSameFile ? "In and out files are the same" : "Backup requested"));
    pszPathCopy = strdup(pszOutName);
    if (!pszPathCopy) goto fail_no_mem;
    pszDirName = dirname(pszPathCopy);
    pszTmpName = strdup(pszDirName);
    if (pszTmpName) pszTmpName = realloc(pszTmpName, strlen(pszTmpName)+10);
    if (!pszTmpName) goto fail_no_mem;
    strcat(pszTmpName, DIRSEPARATOR_STRING "dtXXXXXX");
    iFile = mkstemp(pszTmpName);
    if (iFile == -1) fail("Can't create temporary file %s. %s\n", pszTmpName, strerror(errno));
    df = fdopen(iFile, "wb+");
    if (!df) goto open_df_failed;
    if (iBackup) { /* Create the name of an *.bak file in the same directory */
      char *pszNameCopy = strdup(pszInName);
      char *pszBaseName = basename(pszNameCopy);
      char *pc;
      if (!pszNameCopy) goto fail_no_mem;
      strcpy(szBakName, pszDirName);
      strcat(szBakName, DIRSEPARATOR_STRING);
      pc = strrchr(pszBaseName, '.');
      if (pc) {
	if (SAMENAME(pc, ".bak")) {
	  fail("Can't backup file %s\n", pszInName);
	}
	*pc = '\0';			/* Remove the extension */
      }
      strcat(szBakName, pszBaseName);	/* Copy the base name without the extension */
      strcat(szBakName, ".bak");	/* Set extension to .bak */
      free(pszNameCopy);		/* We don't need that copy anymore */
    }
  } else {
    DEBUG_FPRINTF((mf, "// In and out files are distinct. Writing directly to the out file.\n"));
  }
  if (!df) {
    df = fopen(pszOutName, "wb");
    if (!df) {
open_df_failed:
      if (sf != stdout) fclose(sf);
      fail("Can't open file %s\n", pszOutName);
    }
  }

  /* Go for it */

  DEBUG_FPRINTF((mf, "// Reading the input from %s\n", (sf == stdin) ? "stdin" : pszInName));
  while (!feof(sf)) {
    if (!iCtrlZ) {
      nRead = fread(pszBuffer+nTotal, 1, BLOCKSIZE, sf);
    } else { /* Read characters 1 by 1, to avoid blocking if the EOF character is not on a BLOCKSIZE boundary */
      for (nRead = 0; nRead < BLOCKSIZE; nRead++) {
	char c;
	if (!fread(&c, 1, 1, sf)) break;
	if (c == '\x1A') break; /* We got a SUB <==> EOF character */
	pszBuffer[nTotal+nRead] = c;
      }
    }
    verbose((mf, "Read %d input bytes.\n", (int)nRead));
    nTotal += nRead;
    if ((nTotal+BLOCKSIZE)>nBufSize) {
      if (!((pszBuffer = (char*)realloc(pszBuffer, nTotal + BLOCKSIZE)))) {
	goto fail_no_mem;
      }
      nBufSize += BLOCKSIZE;
    }
    if (iCtrlZ && (nRead < BLOCKSIZE)) break;
    Sleep(0);	    /* Release end of time-slice */
  }
  /* 
    Windows provides an API that is supposed to detect a text encoding.
    They warn that this cannot be reliably detected in all cases, which is true.
    I implemented this in the hope that this API would give better results than
    my simple heuristic that follows. But unfortunately, the results are bad.
    It basically fails most of the times, except when the text is pure ASCII.
    Anyway, I'm leaving this in, in case of a future miracle. Like maybe Windows
    using open-source libraries like uchardet, or Google's ced.
  */
  if (*pszInType == '*') { /* Then use the COM API IMultiLanguage2::DetectInputCodepage() */
    HRESULT hr;
    UINT cp = cpDicPref; /* I tried setting this to GetACP(), but the end results are even worse */
    int iSize = (int)nTotal;
    DetectEncodingInfo dei[10];
    int iCount = sizeof(dei) / sizeof(DetectEncodingInfo); // # of elements of dei
    int iMaxConf = -32767; /* For very small texts, iConfidence == -1 */
    static char szCP[6];

    DEBUG_FPRINTF((mf, "Assuming the input to be CP %u\n", (unsigned int)cp));
    hr = DetectInputCodepage(dwDicFlags, cp, pszBuffer, &iSize, dei, &iCount);
    if (FAILED(hr)) {
      fprintf(stderr, "IMultiLanguage2::DetectInputCodepage() failed\n");
      pszInType = "?"; /* Fall back to using our simple heuristics */
    } else {
      DEBUG_FPRINTF((mf, "IMultiLanguage2::DetectInputCodepage(%u, ...) found in the first %d bytes:\n", (unsigned int)cp, iSize));
      for (i=0; i<iCount; i++) {
      	int iConfidence = (int)(dei[i].nConfidence); /* For very small texts, iConfidence == -1 */
      	cp = (unsigned int)(dei[i].nCodePage);
	DEBUG_FPRINTF((mf, "CP %u, in %d%% of the text, with %d%% confidence.\n", cp, (int)(dei[i].nDocPercent), iConfidence));
	if (iConfidence > iMaxConf) {
	  iMaxConf = iConfidence;
	  sprintf(szCP, "%u", cp);
	  pszInType = szCP;
	}
      }
    }
    DEBUG_CODE(if (iTestDic) iVerbose = TRUE;)
    verbose((mf, "Windows' IMultiLanguage2 COM API detected CP: %s\n", pszInType));
    DEBUG_CODE(if (iTestDic) return 0;)
  }
  /*
    A simple heuristic for selecting among the most common cases in Windows:
    The Windows system code page; ASCII; UTF-7; UTF-8; UTF-16; UTF-32
  */
  if (*pszInType == '?') { /* Then use heuristics to detect the input data encoding */
    char szMsg[100];
    char szValue[10];
    UINT cp = GetBufferEncoding(pszBuffer, nTotal, 0); /* 2021-05-28 Moved to MsvcLibX */
    /* Ideally we should default to the current console code page for input from a pipe,
       and to the ANSI code page for input from a file.
       But the most common use case being to correct the output to the console
       when it's in the wrong code page, ie. NOT in the current console code page,
       then defaulting to the ANSI code page now is the best choice.
       Also we have the . argument to specify the current console code page. */
     switch (cp) {
       case CP_UNDEFINED: sprintf(szMsg, "Unrecognized encoding, possibly binary"); pszInType="w"; break;
       case CP_ACP: sprintf(szMsg, "Windows system code page %d", systemCodePage); pszInType="w"; break;
       case CP_ASCII: sprintf(szMsg, "US-ASCII code page %d", CP_ASCII); pszInType="a"; break;
       case CP_UTF7: sprintf(szMsg, "UTF-7 code page %d", CP_UTF7); pszInType="7"; break;
       case CP_UTF8: sprintf(szMsg, "UTF-8 code page %d", CP_UTF8); pszInType="8"; break;
       case CP_UTF16: sprintf(szMsg, "UTF-16 code page %d", CP_UTF16); pszInType="16"; break;
       case CP_UTF32: sprintf(szMsg, "UTF-32 code page %d", CP_UTF32); pszInType="32"; break;
       default: sprintf(szMsg, "Code page %d", cp); sprintf(szValue, "%u", cp); pszInType=szValue; break;
     }
    DEBUG_CODE(if (iTestGuess) iVerbose = TRUE;)
    verbose((mf, "Heuristic detected input type: %s\n", szMsg));
    DEBUG_CODE(if (iTestGuess) return 0;)
  }

  /* Do the conversion */
  if (nTotal > 0) {
    size_t nOutBufSize = 4*nTotal + 4;	/* Size may double for ansi -> utf8 cases, or even quadruple for ascii->utf32 */
    size_t nOutputSize;		/* Size actually used in the out. buf. */
    char *pszOutBuf = (char*)malloc(nOutBufSize);
    char *pszCharEnc = NULL;	/* Character encoding */

    if (!pszOutBuf) goto fail_no_mem;

    /* Optionally decode mime encoded words left in */
    isEncoding(pszInType, NULL, &pszCharEnc);
    if (pszCharEnc) {
      size_t nTotal0 = nTotal;
      if (streq(pszCharEnc, "utf-8")) { /* Workaround for the unknown \xC3\x20 sequence: Use \xC3\xA0 instead. */
	size_t n;
	for (n=0; n<(nTotal-1); n++) {
	  if (memcmp(pszBuffer+n, "\xC3\x20", 2) == 0) {
	    pszBuffer[++n] = '\xA0';
	    lnChanges += 1;
	  }
	}
      }
      nTotal = MimeWordDecode(pszBuffer, nTotal, pszCharEnc);
      if (nTotal != nTotal0) lnChanges += 1;
    }

    /* Use Windows' character set conversion routine. */
    nOutputSize = ConvertCharacterSet(pszBuffer, nTotal, pszOutBuf, nOutBufSize,
					    pszInType, pszOutType, iBOM, dwFlags);
    /* TO DO: Find a way to count the actual number of characters changed */
    if ((nOutputSize != nTotal) || memcmp(pszBuffer, pszOutBuf, nTotal)) lnChanges += 1;

    if (!fwrite(pszOutBuf, nOutputSize, 1, df)) {
      WIN32FAIL("Cannot write to the output file.");
    }
  }
  if (iCtrlZ2) putc('\x1A', stdout);

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);
  DEBUG_FPRINTF((mf, "// Writing done\n"));

  if (iSameFile && !lnChanges) { /* Nothing changed */
    iErr = unlink(pszTmpName); 	/* Remove the temporary output file */
  } else {
    if (iSameFile || iBackup) {
      if (iBackup) {	/* Create an *.bak file in the same directory */
#if !defined(_MSVCLIBX_H_)
        DEBUG_FPRINTF((mf, "unlink(\"%s\");\n", szBakName));
#endif
	iErr = unlink(szBakName); 	/* Remove the .bak if already there */
	if ((iErr == -1) && (errno != ENOENT)) {
	  fail("Can't delete file %s. %s\n", szBakName, strerror(errno));
	}
	DEBUG_FPRINTF((mf, "rename(\"%s\", \"%s\");\n", pszOutName, szBakName));
	iErr = rename(pszOutName, szBakName);	/* Rename the source as .bak */
	if (iErr == -1) {
	  fail("Can't backup %s. %s\n", pszOutName, strerror(errno));
	}
      } else {		/* iSameFile==TRUE && iBackup==FALSE. Don't keep a backup of the input file */
#if !defined(_MSVCLIBX_H_)
	DEBUG_FPRINTF((mf, "unlink(\"%s\");\n", pszInName));
#endif
	iErr = unlink(pszInName); 	/* Remove the original file */
	if (iErr == -1) {
	  fail("Can't delete file %s. %s\n", pszInName, strerror(errno));
	}
      }
      DEBUG_FPRINTF((mf, "rename(\"%s\", \"%s\");\n", pszTmpName, pszOutName));
      iErr = rename(pszTmpName, pszOutName); /* Rename the temporary file as the destination */
      if (iErr == -1) {
	fail("Can't create %s. %s\n", pszOutName, strerror(errno));
      }
    }

    /* Copy the file mode flags */
    if (df != stdout) {
      int iMode = sInTime.st_mode;
      DEBUG_PRINTF(("chmod(\"%s\", 0x%X);\n", pszOutName, iMode));
      iErr = chmod(pszOutName, iMode); /* Try making the target file writable */
      DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
    }

    /* Optionally copy the timestamp */
    if (!lnChanges) iCopyTime = TRUE; /* Always set the same time if there was no data change */
    if ((sf != stdin) && (df != stdout) && iCopyTime) {
      struct utimbuf sOutTime = {0};
      sOutTime.actime = sInTime.st_atime;
      sOutTime.modtime = sInTime.st_mtime;
      utime(pszOutName, &sOutTime);
    }
  }

  verbose((mf, "Exiting. %s.\n", lnChanges ? "Data converted" : "Data unchanged"));
  return 0;
}

#pragma warning(default:4706)	/* Restore the "assignment within conditional expression" warning */

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
|   Function:	    is_redirected					      |
|									      |
|   Description:    Check if a FILE is a device or a disk file. 	      |
|									      |
|   Parameters:     FILE *f		    The file to test		      |
|									      |
|   Returns:	    TRUE if the FILE is a disk file			      |
|									      |
|   Notes:	    Designed for use with the stdin and stdout FILEs. If this |
|		    routine returns TRUE, then they've been redirected.       |
|									      |
|   History:								      |
|    2004-04-05 JFL Added a test of the S_IFIFO flag, for pipes under Windows.|
*									      *
\*---------------------------------------------------------------------------*/

int is_redirected(FILE *f) {
  int err;
  struct stat st;
  int h;

  h = fileno(f);			/* Get the file handle */
  err = fstat(h, &st);			/* Get information on that handle */
  if (err) return FALSE;		/* Cannot tell more if error */
  return (   (S_ISREG(st.st_mode))	/* Tell if device is a regular file */
	  || (S_ISFIFO(st.st_mode))	/* or it's a FiFo */
	 );
}

int is_pipe(FILE *f) {
  int err;
  struct stat st;			/* Use MSC 6.0 compatible names */
  int h;

  h = fileno(f);			/* Get the file handle */
  err = fstat(h, &st);			/* Get information on that handle */
  if (err) return FALSE;		/* Cannot tell more if error */
  return (S_ISFIFO(st.st_mode));	/* It's a FiFo */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ConvertCharacterSet					      |
|                                                                             |
|   Description:    Convert the characters                      	      |
|                                                                             |
|   Parameters:     char *pszInput          The input buffer                  |
|                   size_t nInputSize       Number of bytes in the input buf. |
|                   char *pszOutput         The output buffer                 |
|                   size_t nOutputSize      Number of bytes in the output buf.|
|                   char *pszInputSet       The input buffer encoding         |
|                   char *pszOutputSet      The output buffer encoding        |
|                   int iBOM                0=Leave 1=Add -1=Remove BOM       |
|                   DWORD dwFlags           Flags for WideCharToMultiByte()   |
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

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#ifndef CP_UTF16	/* Not defined in WinNls.h for Windows SDK 8.1 */
#define CP_UTF16 1200	/* "Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications" */
#endif

int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM, DWORD dwFlags) {
  wchar_t *pszWideBuf;             /* Intermediate buffer for wide characters. */
  size_t nWideSize = (4*nInputSize) + 4; /* Size in bytes of that wide buffer. */
  UINT uCPin=0, uCPout=0;
  int nWide, nOut;

  DEBUG_FPRINTF((mf, "ConvertCharacterSet(pszIn, %ld, pszOut, %ld, %s, %s, %d)\n", \
	 (long)nInputSize, (long)nOutputSize, pszInputSet, pszOutputSet, iBOM));
  DEBUG_CODE(iIndent += DEBUG_INDENT_STEP;)

  if (!isEncoding(pszInputSet, &uCPin, NULL)) {
    FAIL("Unknown input character set");
  }
  if (!isEncoding(pszOutputSet, &uCPout, NULL)) {
    FAIL("Unknown output character set");
  }

  verbose((mf, "uCPin = %d , uCPout = %d\n", (int)uCPin, (int)uCPout));
  
  /* Use MsvcLibX's ConvertBuf() routine for everything in one step if possible */
  if (!iBOM) {
    nOut = ConvertBufEx(pszInput, nInputSize, uCPin, pszOutput, nOutputSize, uCPout, dwFlags, NULL, NULL);
    if (nOut == -1) {
      FAIL("Conversion failed");
    }
    RETURN_INT(nOut);
  }

  /* Allocate a buffer for an intermediate UTF-16 string */
  pszWideBuf = (wchar_t *)malloc(nWideSize);
  pszWideBuf += 1;	/* Leave room for adding a BOM if needed */
  nWideSize -= 2;

  /* Convert to intermediate wide characters */
  DEBUG_FPRINTF((mf, "ConvertBuf(%p, %ld, %u, %p, %ld, %u)\n", \
		    pszInput, (long)nInputSize, uCPin, (char *)pszWideBuf, (long)nWideSize, CP_UTF16));
  nOut = ConvertBufEx(pszInput, nInputSize, uCPin, (char *)pszWideBuf, nWideSize, CP_UTF16, 0, NULL, NULL);
  if (nOut == -1) FAIL("Cannot convert the input!");
  nWide = (int)nOut / 2;

  /* Add or remove the BOM if requested or needed */
  switch (uCPout) {
  case CP_UTF7:
  case CP_UTF8:
  case CP_UTF16:
  case CP_UTF32:
    break;	/* Apply the user request for all Unicode encodings */
  default:	/* Any other code page requires removing the BOM, if any */
    iBOM = -1;
    break;
  }
#pragma warning(disable:4428)	/* Ignore the "universal-character-name encountered in source" warning */
  switch (iBOM) {
  case -1:
    if (*pszWideBuf == L'\uFEFF') {	/* If there's a BOM, then remove it */
      pszWideBuf += 1;
      nWideSize -= 2;
      nWide -= 1;
    }
    break;
  case 1:
    if (*pszWideBuf != L'\uFEFF') {	/* If there's no BOM, then add one */
      pszWideBuf -= 1;
      nWideSize += 2;
      *pszWideBuf = L'\uFEFF';
      nWide += 1;
    }
    break;
  case 0:				/* Leave the BOM (if present) unchanged */
  default:
    break;
  }
#pragma warning(default:4428)	/* Restore the "universal-character-name encountered in source" warning */

  /* Convert back to ANSI characters */
  DEBUG_FPRINTF((mf, "ConvertBuf(%p, %ld, %u, %p, %ld, %u)\n", \
		    (char *)pszWideBuf, (long)nWide * 2, CP_UTF16, pszOutput, (long)nOutputSize, uCPout));
  nOut = ConvertBufEx((char *)pszWideBuf, nWide * 2, CP_UTF16, pszOutput, nOutputSize, uCPout, dwFlags, NULL, NULL);
  if (nOut == -1) FAIL("Cannot convert the output!");

  RETURN_INT(nOut);
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

#pragma warning(disable:4295)	/* Ignore the "array is too small to include a terminating null character" warning */
unsigned char alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#pragma warning(default:4295)	/* Restore the "array is too small to include a terminating null character" warning */

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

  nOutputSize -= 1; /* Reserver 1 byte for the final NUL. */
  char_count = 0;
  bits = 0;
  while (nInputSize-- && nOutputSize) {
    c = *(pszInput++);
    if (c == '=') break; /* End of Mime sequence */
    if (!inalphabet[c]) continue; /* Ignore error silently */
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
    case 1: /* Error: base64 encoding incomplete: at least 2 bits missing */
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

#pragma warning(disable:4706)	/* Ignore the "assignment within conditional expression" warning */

size_t MimeWordDecode(char *pszBuf, size_t nBufSize, char *pszCharEnc)
{
  char *pszBuf2 = (char*)malloc(nBufSize + 1);
  char *pszHeader = (char*)malloc(strlen(pszCharEnc) + 8);
  char *psz00, *psz0, *psz1, *psz20, *psz2;
  char cEnc; /* Character encoding. Q or B. */
  size_t nHeader;
  char c;
  size_t n;
  int i;

  DEBUG_FPRINTF((mf, "MimeWordDecode(0x%p, %lu, \"%s\").\n", pszBuf, (unsigned long)nBufSize, pszCharEnc));

  if (!pszBuf2 || !pszHeader) FAIL("Not enough memory for Mime word decoding!");
  nHeader = sprintf(pszHeader, "=?%s?", pszCharEnc);

  memcpy(pszBuf2, pszBuf, nBufSize);
  pszBuf2[nBufSize] = '\0';
  psz00 = psz0 = pszBuf2;
  psz20 = psz2 = pszBuf;

  /* Scan the input data and decode it. */
  while ((psz1 = stristr(psz0, pszHeader)))
  {
    DEBUG_FPRINTF((mf, "while(): psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    if ((size_t)(psz0-psz00) >= nBufSize) FAIL("Oops, went too far!");
    /* Copy input up to the Mime word header. */
    n = psz1-psz0;
    strncpy(psz2, psz0, n);
    psz0 += n + nHeader;
    psz2 += n;
    /* We've seen case for such words in quoted strings. Remove the opening quote. */
    if (n && (*(psz2-1) == '"')) psz2 -= 1;
    /* Get the encoding byte. */
    cEnc = *(psz0++);
    if (*(psz0++) != '?') /* Skip the second ? */
      fail("Bad Mime encoded-word header: %20.20s", psz0-(nHeader+2));
    /* Get the rest of the encoded word. */
    for (psz1=psz0; (c=*psz1)>' '; psz1++) ;
    n = psz1-psz0;
    DEBUG_FPRINTF((mf, "Encoding: psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    /* Decode the word */
    switch (toupper(cEnc))
    {
    case 'Q': /* Quoted-printable */
      for ( ; psz0 < psz1; psz0++)
      {
        DEBUG_FPRINTF((mf, "for(): psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
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
    case 'B': /* Base64 */
      psz2 += Base64Decode(psz2, nBufSize-(psz2-psz20), psz0, n);
      psz0 += n;
      break;
    default:
      FAIL("Unexpected character encoding");
      break;
    }
    /* Skip the following separator spaces if there's another encoded word afterwards */
    psz1 = psz0;
    while (isspace(*psz1)) psz1++;
    if (!strncmp(psz1, "=?", 2)) psz0 = psz1;
  }
  /* Output the end of the data, if any */
  n = nBufSize-(psz0-psz00);
  memcpy(psz2, psz0, n);
  psz2 += n;

  /* Cleanup */
  free(pszBuf2);
  free(pszHeader);

  return psz2-psz20; /* The output length. */
}

#pragma warning(default:4706)	/* Restore the "assignment within conditional expression" warning */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    RegGetString					      |
|									      |
|   Description     Get a registry string value				      |
|									      |
|   Parameters:     HKEY rootKey            One of HKEY_LOCAL_MACHINE, ...    |
|                   LPCTSTR pszKey          Key name                          |
|                   LPCTSTR pszValue        Value name                        |
|                   LPTSTR pszBuf           Output buffer                     |
|		    size_t nBufSize	    Output buffer size		      |
|		    							      |
|   Returns	    0 = Success, else WIN32 system error code		      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2016-09-08 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32
LONG RegGetString(HKEY rootKey, LPCTSTR pszKey, LPCTSTR pszValue, LPTSTR pszBuf, size_t nBufSize) {
  DWORD dwSize = (DWORD)nBufSize;
  HKEY hKey;
  LONG lErr;			/* Win32 error */
  DWORD dwType;			/* Actual type of the value read */

  lErr = RegOpenKey(rootKey, pszKey, &hKey);
  if (lErr) return lErr;
  lErr = RegQueryValueEx(hKey, pszValue, NULL, &dwType, (LPBYTE)pszBuf, &dwSize);
  RegCloseKey(hKey);
  if (lErr) return lErr;
  if (dwType != REG_SZ) {
    lErr = ERROR_DATATYPE_MISMATCH;
    SetLastError(lErr);
    return lErr;
  }
#if 0	/* Based on the doc for RegGetValue, I had a doubt about whether the NUL
	   was included in the returned data or not */ 
  printf("dwSize = %d\n", (int)dwSize);
  if (dwSize && pszBuf[dwSize-1]) {
    printf("Adding the missing NUL\n");
    if (nBufSize > dwSize) {
      pszBuf[dwSize] = '\0';	/* Add a terminating NUL */
    } else {
      return ERROR_MORE_DATA;	/* Buffer is too small to store the terminating NUL */
    }
  }
#endif
  return 0;
}
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSameFile						      |
|									      |
|   Description     Check if two pathnames refer to the same file	      |
|									      |
|   Parameters:     char *pszPathname1	    The first pathname to check	      |
|                   char *pszPathname2	    The second pathname to check      |
|                   							      |
|   Returns	    1 = Same file; 0 = Different files			      |
|									      |
|   Notes	    Constraints:					      |
|		    - Do not change the files.				      |
|		    - Fast => Avoid resolving links when not necessary.	      |
|		    - Works even if the files do not exist yet.		      |
|		    							      |
|		    Must define a SAMENAME constant, that refers to a file    |
|		    name comparison routine. This routine is OS-dependant,    |
|		    as comparisons are case-dependant in Unix, but not in     |
|		    Windows.						      |
|		    							      |
|   History								      |
|    2016-09-12 JFL Created this routine				      |
|    2020-08-14 JFL Added USE_WIN32_API to allow testing with the alternative.|
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32
#define USE_WIN32_API 1
#else
#define USE_WIN32_API 0
#endif

int IsSameFile(char *pszPathname1, char *pszPathname2) {
  int iSameFile;
  char *pszBuf1 = NULL;
  char *pszBuf2 = NULL;
#if USE_WIN32_API
  WIN32_FILE_ATTRIBUTE_DATA attr1;
  WIN32_FILE_ATTRIBUTE_DATA attr2;
#else
  struct stat attr1;
  struct stat attr2;
#endif /* USE_WIN32_API */
  int bDone1;
  int bDone2;
  DEBUG_CODE(
  char *pszReason;
  )

  DEBUG_ENTER(("IsSameFile(\"%s\", \"%s\");\n", pszPathname1, pszPathname2));

  /* First try the obvious: Compare the input arguments */
  if (streq(pszPathname1, pszPathname2)) {
    DEBUG_CODE(pszReason = "Exact same pathnames";)
    iSameFile = TRUE;
IsSameFile_done:
    free(pszBuf1);
    free(pszBuf2);
    RETURN_INT_COMMENT(iSameFile, ("%s\n", pszReason));
  }

  /* Then try a simple attributes comparison, to quickly detect different files */
#if USE_WIN32_API
  bDone1 = (int)GetFileAttributesEx(pszPathname1, GetFileExInfoStandard, &attr1);
  bDone2 = (int)GetFileAttributesEx(pszPathname2, GetFileExInfoStandard, &attr2);
#else
  bDone1 = stat(pszPathname1, &attr1) + 1;
  bDone2 = stat(pszPathname2, &attr2) + 1;
#endif /* USE_WIN32_API */
  if (bDone1 != bDone2) {
    DEBUG_CODE(pszReason = "One exists and the other does not";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  if ((!bDone1) && SAMENAME(pszPathname1, pszPathname2)) {
    DEBUG_CODE(pszReason = "They will be the same";)
    iSameFile = TRUE;
    goto IsSameFile_done;
  }
  if ((bDone1) && memcmp(&attr1, &attr2, sizeof(attr1))) {
    DEBUG_CODE(pszReason = "They're different sizes, times, etc";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  /* They look very similar now: Names differ, but same size, same dates, same attributes */

  /* Get the canonic names, with links resolved, to see if they're actually the same or not */
  pszBuf1 = realpath(pszPathname1, NULL);
  pszBuf2 = realpath(pszPathname2, NULL);
  if ((!pszBuf1) || (!pszBuf2)) {
    DEBUG_CODE(pszReason = "Not enough memory for temp buffers";)
    iSameFile = FALSE;
    goto IsSameFile_done;
  }
  iSameFile = SAMENAME(pszBuf1, pszBuf2);
  DEBUG_LEAVE(("return %d; // \"%s\" %c= \"%s\";\n", iSameFile, pszBuf1, iSameFile ? '=' : '!', pszBuf2));
  free(pszBuf1);
  free(pszBuf2);
  return iSameFile; 
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    isEncoding						      |
|									      |
|   Description     Check if if an argument refers to a charater encoding     |
|									      |
|   Parameters:     char *pszEncoding	    Input string               	      |
|                   UINT *pCP               If not NULL, output the code page |
|                   char **ppszMime   	    If not MULL, output the mime type |
|                   							      |
|   Returns	    1 = It is an encoding; 0 It's not 			      |
|									      |
|   Notes	    Avoids duplicating the code for input & output args.      |
|		    							      |
|   History								      |
|    2017-03-15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int isEncoding(char *pszEncoding, UINT *pCP, char **ppszMime) {
  UINT cp = CP_UNDEFINED;
  char *pszMime = NULL;
  /* Check if it's a 1-character shortcut */
  if (pszEncoding[0] && !pszEncoding[1]) {
    switch (tolower(pszEncoding[0])) {
    case '?':			/* Autodetect the encoding */
    case '*':			/* Autodetect the encoding */
      cp = CP_AUTODETECT;
      break;
    case '.':			/* Console CP */
    case 'c':			/* Console CP */
      cp = GetConsoleOutputCP();
      break;
    case '0':			/* ANSI = Windows GUI CP */
    case 'w':			/* ANSI = Windows GUI CP */
      cp = CP_ACP;		/* ANSI */
      pszMime = "windows-1252";
      break;
    case '1':			/* DOS = Default console CP */
    case 'd':			/* DOS = Default console CP */
    case 'o':			/* OEM = Default console CP */
      cp = CP_OEMCP;		/* DOS */
      pszMime = "ms-dos";
      break;
    case '2':			/* Mac CP */
    case 'm':			/* Mac CP */
      cp = CP_MACCP;		/* Mac */
      pszMime = "macintosh";
      break;
    case 'a':			/* ASCII */
      cp = CP_ASCII;		/* US-ASCII */
      break;
    case '7':			/* UTF-7 */
      cp = CP_UTF7;		/* UTF-7 */
      pszMime = "utf-7";
      break;
    case '8':			/* UTF-8 */
      cp = CP_UTF8;		/* UTF-8 */
      pszMime = "utf-8";
      break;
    case 'u':			/* UTF-16 */
      cp = CP_UTF16;		/* UTF-16 */
      pszMime = "utf-16";
      break;
    }
    if (cp != CP_UNDEFINED) {
      if (pCP) *pCP = cp;
      if (ppszMime) *ppszMime = pszMime;
      return TRUE;
    }
  }
  /* Check if the string is an integer from 0 to 65535 */
  if (sscanf(pszEncoding, "%u", &cp)) {
    char buf[32];
    sprintf(buf, "%u", cp);
    if ((!strcmp(buf, pszEncoding)) && (cp <  65536)) {
      if (cp == 16) cp = CP_UTF16; /* Special case: Not an actual code page, but a request for UTF-16 */
      if (cp == 32) cp = CP_UTF32; /* Special case: Not an actual code page, but a request for UTF-32 */
      if (pCP) *pCP = cp;
      if (ppszMime) *ppszMime = NULL; /* Do not decode MIME strings in this case */
      return TRUE;
    }
  }
  /* Unrecognized string */
  return FALSE;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DetectInputCodepage					      |
|									      |
|   Description     C front end to COM IMultiLanguage2::DetectInputCodepage() |
|									      |
|   Parameters:     Same as the COM API.				      |
|                   							      |
|   Returns	    Same as the COM API.				      |
|									      |
|   Notes	    See the IMultiLanguage2 interface doc:		      |
|   https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/aa741001%28v%3dvs.85%29
|		    							      |
|   History								      |
|    2020-05-10 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

HRESULT DetectInputCodepage(DWORD dwFlags, DWORD dwPrefCP, char *pszBuffer, INT *piSize, DetectEncodingInfo *lpInfo, INT *pnInfos) {
  HRESULT hr;
  IMultiLanguage2 *pML;

  // Initialize COM
  hr = CoInitialize(NULL);
  if (FAILED(hr)) return hr;

  // Obtain the MLang IMultiLanguage2 object interface
  hr = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, &IID_IMultiLanguage2, (LPVOID *)&pML);
  if (FAILED(hr)) goto cleanup_and_exit;

  // Call the requested C++ method
  hr = pML->lpVtbl->DetectInputCodepage(
      pML,			// C++ this,
      dwFlags,			// dwFlag,
      dwPrefCP,			// dwPrefWinCodePage,
      pszBuffer,		// *pSrcStr,
      piSize,			// *pcSrcSize,
      lpInfo,			// *lpEncoding,
      pnInfos			// *pnScores
  );
  if (FAILED(hr)) goto cleanup_and_exit;

cleanup_and_exit:
  CoUninitialize();
  
  return hr;
}

#endif /* defined(_WIN32) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetTrueWindowsVersion				      |
|									      |
|   Description     Extend GetVersion() to work on Windows versions >= 8.1    |
|									      |
|   Parameters	    None						      |
|                   							      |
|   Returns	    Same as GetVersion()				      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2020-08-18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4996)       /* Ignore the deprecated name warning */

#ifdef _WIN32

DWORD GetTrueWindowsVersion(void) {
  DWORD dwVersion = GetVersion();
  unsigned major = (unsigned)(LOBYTE(LOWORD(dwVersion)));
  unsigned minor = (unsigned)(HIBYTE(LOWORD(dwVersion)));
  unsigned build = (unsigned)(HIWORD(dwVersion));
  DEBUG_FPRINTF((mf, "GetVersion(); // %u.%u.%u\n", major, minor, build));
  /* But in Windows 8.1 and later, GetVersionEx() lies about the true version,
     and pretends to be version 6.2, that is Windows 8.0.
     So instead, query the Windows kernel DLL version. */
  if ( (major > 6) || ((major == 6) && (minor >= 2)) ) {
    char *pszKernel32 = "kernel32.dll";
    DWORD  dwHandle;
    DWORD  dwSize;
#pragma comment(lib, "version.lib")
    if ((dwSize = GetFileVersionInfoSize(pszKernel32, &dwHandle)) > 0) {
      LPBYTE lpBuffer = malloc((size_t)dwSize);
      if (lpBuffer) {
	if (GetFileVersionInfo(pszKernel32, dwHandle, dwSize, lpBuffer)) {
	  VS_FIXEDFILEINFO *pfi;
	  UINT size = 0;
	  if (VerQueryValue(lpBuffer, "\\", (LPVOID *)&pfi, &size)) {
	    major = ( pfi->dwFileVersionMS >> 16 ) & 0xffff;
	    minor = ( pfi->dwFileVersionMS >>  0 ) & 0xffff;
	    build = ( pfi->dwFileVersionLS >> 16 ) & 0xffff;
	    dwVersion = major | (minor << 8) | (build << 16);
	  }
	}
	free(lpBuffer);
      }
    }
  }
  DEBUG_FPRINTF((mf, "GetTrueWindowsVersion(); // %u.%u.%u\n", major, minor, build));
  return dwVersion;
}

#endif /* defined(_WIN32) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    file_exists						      |
|									      |
|   Description     Check if a pathname refers to an existing file	      |
|									      |
|   Parameters:     char *pszPathname	    The pathname to check	      |
|                   							      |
|   Returns	    1 = It's a file; 0 = It does not exist, or it's not a file|
|									      |
|   Notes	    Uses stat, to check what's really behind links	      |
|		    							      |
|   History								      |
|    2020-04-04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int file_exists(const char *pszName) {	/* Does this file exist? (TRUE/FALSE) */
  struct stat st;
  int iErr;

  DEBUG_ENTER(("file_exists(\"%s\");\n", pszName));

  iErr = stat(pszName, &st);		/* Get the status of that file */
  if (iErr) RETURN_CONST(FALSE);	/* It does not exist, or is inaccessible anyway */

  RETURN_INT(S_ISREG(st.st_mode));
}

