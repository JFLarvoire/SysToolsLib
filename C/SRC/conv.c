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
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.4.2"
#define PROGRAM_DATE    "2016-09-15"

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

/* Use MsvcLibX Library's debugging macros */
#include "debugm.h"
DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#error "There is no version of this program for MS-DOS yet."

#endif /* defined(_MSDOS) */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/*  Automatically defined when targeting a Win32 applic. */

/*  if defined, the following flags inhibit the definitions in windows.h */
#define NOGDICAPMASKS	  /*  CC_*, LC_*, PC_*, CP_*, TC_*, RC_ */
#define NOVIRTUALKEYCODES /*  VK_* */
#define NOWINMESSAGES	  /*  WM_*, EM_*, LB_*, CB_* */
#define NOWINSTYLES	  /*  WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_* */
#define NOSYSMETRICS	  /*  SM_* */
#define NOMENUS 	  /*  MF_* */
#define NOICONS 	  /*  IDI_* */
#define NOKEYSTATES	  /*  MK_* */
#define NOSYSCOMMANDS	  /*  SC_* */
#define NORASTEROPS	  /*  Binary and Tertiary raster ops */
#define NOSHOWWINDOW	  /*  SW_* */
/*  #define OEMRESOURCE	     // OEM Resource values */
#define NOATOM		  /*  Atom Manager routines */
#define NOCLIPBOARD	  /*  Clipboard routines */
#define NOCOLOR 	  /*  Screen colors */
#define NOCTLMGR	  /*  Control and Dialog routines */
#define NODRAWTEXT	  /*  DrawText() and DT_* */
#define NOGDI		  /*  All GDI defines and routines */
#define NOKERNEL	  /*  All KERNEL defines and routines */
/*  #define NOUSER		  // All USER defines and routines */
/*  #define NONLS		  // All NLS defines and routines */
#define NOMB		  /*  MB_* and MessageBox() */
#define NOMEMMGR	  /*  GMEM_*, LMEM_*, GHND, LHND, associated routines */
#define NOMETAFILE	  /*  typedef METAFILEPICT */
#define NOMINMAX	  /*  Macros min(a,b) and max(a,b) */
#define NOMSG		  /*  typedef MSG and associated routines */
#define NOOPENFILE	  /*  OpenFile(), OemToAnsi, AnsiToOem, and OF_* */
#define NOSCROLL	  /*  SB_* and scrolling routines */
#define NOSERVICE	  /*  All Service Controller routines, SERVICE_ equates, etc. */
#define NOSOUND 	  /*  Sound driver routines */
#define NOTEXTMETRIC	  /*  typedef TEXTMETRIC and associated routines */
#define NOWH		  /*  SetWindowsHook and WH_* */
#define NOWINOFFSETS	  /*  GWL_*, GCL_*, associated routines */
#define NOCOMM		  /*  COMM driver routines */
#define NOKANJI 	  /*  Kanji support stuff. */
#define NOHELP		  /*  Help engine interface. */
#define NOPROFILER	  /*  Profiler interface. */
#define NODEFERWINDOWPOS  /*  DeferWindowPos routines */
#define NOMCX		  /*  Modem Configuration Extensions */
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

/*  Define WIN32 replacements for common Standard C library functions. */
#define malloc(size) (void *)LocalAlloc(LMEM_FIXED, size)
#define realloc(pBuf, l) (void *)LocalReAlloc((HLOCAL)pBuf, l, LMEM_MOVEABLE)	/*  Returns fixed memory, despite the flag name. */
#define free(pBuf) LocalFree((HLOCAL)pBuf)
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define SAMENAME strieq		/* File name comparison routine */

/*  Avoid deprecation warnings */
#define tempnam	_tempnam
#define strdup	_strdup
#define dup	_dup
#define fdopen	_fdopen
#define fileno	_fileno

#include <Tchar.h>
#include <strsafe.h>

int _cdecl ReportWin32Error(_TCHAR *pszExplanation, ...);

#define FAIL(msg) fail("%s", msg);
#define WIN32FAIL(msg) fail("Error %d: %s", GetLastError(), msg)

/*  Define easy to use functions for reading registry values, compatible with old versions of Windows */
LONG RegGetString(HKEY rootKey, LPCTSTR pszKey, LPCTSTR pszValue, LPTSTR pszBuf, size_t nBufSize); /*  Returns the string size, or (-error). */

/*  My favorite string comparison routines. */
#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define strieq(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

#endif /* defined(_WIN32) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#error "This program is for Windows only. Use iconv instead on Unix systems."

#endif /* defined(__unix__) */

/********************** End of OS-specific definitions ***********************/

#define verbose(args) if (iVerbose) do {fprintf args;} while (0)

#define BLOCKSIZE (4096)	/* Number of characters that will be allocated in each loop. */

/* Global variables */

int iVerbose = 0;
FILE *mf;			/* Message output file */
char *pszOutName = NULL;	/* Destination file name */
FILE *df = NULL;		/* Destination file handle */

void fail(char *pszFormat, ...) { /* Display an error message, and abort leaving no traces behind */
  va_list vl;
  int n = 0;

  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);    /*  Not thread-safe on WIN32 ?!? */
  va_end(vl);
  fprintf(stderr, "\n");

  /*  Remove the incomplete output file, if any */
  if (df && (df != stdout)) {
    fclose(df);
    if (pszOutName) unlink(pszOutName);
  }

  exit(1);
}

/* Function prototypes */

char *version(void);
int IsSwitch(char *pszArg);
int is_redirected(FILE *f);	    /* Check if a file handle is the console */
int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM);
size_t MimeWordDecode(char *pszBuf, size_t nBufSize, char *pszCharEnc);
int IsSameFile(char *pszPathname1, char *pszPathname2);

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
    conv [OPTIONS] [ICS [OCS [INFILE [OUTFILE|-same]]]]\n\
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
  -same     Modify the input file in place. (Default: Automatically detected)\n\
  -st       Set the output file time to the same time as the input file.\n\
  -v        Display verbose information\n\
  -V        Display this program version\n\
\n\
ICS = Input Character Set, or code page number. Default = Windows code page\n\
OCS = Output Character Set, or code page number. Default = cmd.exe code page\n\
INFILE = Input file pathname. Default or \"-\" = Read from stdin\n\
OUTFILE = Output file pathname. Default or \"-\" = Write to stdout\n\
\n\
Character Sets: One of the following codes, or a code page number such as 1252\n\
  .         Console Active CP (CP %d in this shell)    (default)\n\
  w         Windows System CP (CP %d on this system)\n\
  d         DOS default CP    (CP %d on this system)\n\
  m         Macintosh CP      (CP %d on this system)\n\
  7         UTF-7             (CP 65000)\n\
  8         UTF-8             (CP 65001)\n\
  16        UTF-16            (CP 1200)\n\
  s         Symbol            (CP 42)\n\
\n\
If one of the symbolic character sets above is specified, also decodes the\n\
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
  size_t  nRead = 0;		/*  Number of characters read in one loop. */
  size_t  nTotal = 0;		/*  Total number of characters read. */
  size_t  nBufSize = BLOCKSIZE;
  char   *pszBuffer = (char*)malloc(BLOCKSIZE);
  int     i;
  char   *pszInType = NULL;	/*  Input character set Windows/Mac/DOS/UTF-8 */
  char   *pszOutType = NULL;	/*  Output character set Windows/Mac/DOS/UTF-8 */
  int     iBOM = 0;		/*  1=Output BOM; -1=Output NO BOM; 0=unchanged */
  FILE *sf = NULL;		/*  Source file handle */
  char *pszInName = NULL;	/*  Source file name */
  char szBakName[FILENAME_MAX+1];
  int iSameFile = FALSE;	/*  Modify the input file in place. */
  int iBackup = FALSE;		/*  With iSameFile, back it up first. */
  int iCopyTime = FALSE;	/*  If true, set the out file time = in file time. */
  struct stat sInTime = {0};
  char *pszPathCopy = NULL;
  char *pszDirName = NULL;	/*  Output file directory */

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
      if (streq(pszOpt, "same")) {	/* -same: Output to the input file */
	iSameFile = TRUE;
	continue;
      }
      if (streq(pszOpt, "st")) {	/* -t: Copy the input file time to the output file */
	iCopyTime = TRUE;
	continue;
      }
      if (streq(pszOpt, "v")) {		/* -v: Verbose */
	iVerbose += 1;
	continue;
      }
      if (streq(pszOpt, "V")) {
	printf("%s\n", version());
	exit(0);
      }
      /*  Unsupported switches are ignored */
      continue;
    }
    if (!pszInType) {
	pszInType = pszArg;
	continue;
    }
    if (!pszOutType) {
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
    /*  Unexpected arguments are ignored */
  }

  /* Report what the message stream is */
  DEBUG_CODE(
    if (mf == stderr) {	/* If stdout is redirected to a file or a pipe */
      DEBUG_FPRINTF((mf, "// Debug output sent to stderr.\n"));
    } else {
      DEBUG_FPRINTF((mf, "// Debug output sent to file #%d.\n", fileno(mf)));
    }
  )

  if (!pszInType) {
      pszInType = "w";
  }
  if (!pszOutType) {
      pszOutType = ".";
  }
  verbose((mf, "Input character set: %s\n", pszInType));
  verbose((mf, "Output character set: %s\n", pszOutType));

  /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
  _setmode( _fileno( stdin ), _O_BINARY );
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  if ((!pszInName) || streq(pszInName, "-")) {
    sf = stdin;
    iSameFile = FALSE;	/*  Meaningless in this case. Avoid issues below. */
  } else {
    sf = fopen(pszInName, "rb");
    if (!sf) fail("Can't open file %s\n", pszInName);
    stat(pszInName, &sInTime);
  }
  if ((!pszOutName) || streq(pszOutName, "-")) {
    if (!iSameFile) df = stdout;
  } else { /*  Ignore the -iSameFile argument. Instead, verify if they're actually the same. */
    iSameFile = IsSameFile(pszInName, pszOutName);
  }
  if (iSameFile) {
    DEBUG_FPRINTF((mf, "// In and out files are the same. Writing to a temp file.\n"));
    pszPathCopy = strdup(pszInName);
    if (!pszPathCopy) goto fail_no_mem;
    pszDirName = dirname(pszPathCopy);
    pszOutName = tempnam(pszDirName, "conv.");
    DEBUG_FPRINTF((mf, "tempnam(\"%s\", \"conv.\"); // \"%s\"\n", pszDirName, pszOutName));
    if (iBackup) {	/* Create an *.bak file in the same directory */
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
      if (sf != stdout) fclose(sf);
      fail("Can't open file %s\n", pszOutName);
    }
  }

  /* Go for it */

  while (!feof(sf)) {
    nRead = fread(pszBuffer+nTotal, 1, BLOCKSIZE, sf);
    verbose((mf, "Read %d input bytes.\n", (int)nRead));
    nTotal += nRead;
    if ((nTotal+BLOCKSIZE)>nBufSize) {
      if (!((pszBuffer = (char*)realloc(pszBuffer, nTotal + BLOCKSIZE)))) {
	goto fail_no_mem;
      }
      nBufSize += BLOCKSIZE;
    }
    Sleep(0);	    /*  Release end of time-slice */
  }
  if (nTotal > 0) {
    size_t nOutBufSize = 2*nTotal + 4;	/*  Size may double for ansi -> utf8 cases */
    size_t nOutputSize;		/*  Size actually used in the out. buf. */
    char *pszOutBuf = (char*)malloc(nOutBufSize);
    char *pszCharEnc = NULL;	/*  Character encoding */

    if (!pszOutBuf) goto fail_no_mem;

    /*  Optionally decode mime encoded words left in */
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
	      { /*  Workaround for the unknown \xC3\x20 sequence: Use \xC3\xA0 instead. */
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

    /*  Use Windows' character set conversion routine. */
    nOutputSize = ConvertCharacterSet(pszBuffer, nTotal, pszOutBuf, nOutBufSize,
					    pszInType, pszOutType, iBOM);

    if (!fwrite(pszOutBuf, nOutputSize, 1, df)) {
      WIN32FAIL("Cannot write to the output file.");
    }
  }

  if (sf != stdin) fclose(sf);
  if (df != stdout) fclose(df);

  if (iSameFile) {
    if (iBackup) {	/* Create an *.bak file in the same directory */
      unlink(szBakName); 		/* Remove the .bak if already there */
      DEBUG_FPRINTF((mf, "Rename \"%s\" as \"%s\"\n", pszInName, szBakName));
      rename(pszInName, szBakName);	/* Rename the source as .bak */
    } else {		/* Don't keep a backup of the input file */
      DEBUG_FPRINTF((mf, "Remove \"%s\"\n", pszInName));
      unlink(pszInName); 		/* Remove the original file */
    }
    DEBUG_FPRINTF((mf, "Rename \"%s\" as \"%s\"\n", pszOutName, pszInName));
    rename(pszOutName, pszInName);	/* Rename the destination as the source */
    pszOutName = pszInName;
  }

  if ((sf != stdin) && (df != stdout) && iCopyTime) {
    struct utimbuf sOutTime = {0};
    sOutTime.actime = sInTime.st_atime;
    sOutTime.modtime = sInTime.st_mtime;
    utime(pszOutName, &sOutTime);
  }

  verbose((mf, "Exiting\n"));
  return 0;
}

#pragma warning(default:4706)	/* Restore the "assignment within conditional expression" warning */

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
	  );
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

#ifndef S_IFIFO
#define S_IFIFO         0010000         /* pipe */
#endif

int is_redirected(FILE *f)
    {
    int err;
    struct stat buf;			/* Use MSC 6.0 compatible names */
    int h;

    h = fileno(f);			/* Get the file handle */
    err = fstat(h, &buf);		/* Get information on that handle */
    if (err) return FALSE;		/* Cannot tell more if error */
    return (   (buf.st_mode & S_IFREG)	/* Tell if device is a regular file */
            || (buf.st_mode & S_IFIFO)	/* or it's a FiFo */
	   );
    }

#ifdef _WIN32		/*  Automatically defined when targeting a Win32 applic. */

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
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /*  Default language */
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL ))	{ /*  Display both the error code and the description string. */
	hRes = StringCbPrintf(szErrorMsg, sizeof(szErrorMsg),
		                  _T("Error %ld: %s"), dwErr, lpMsgBuf);
	LocalFree( lpMsgBuf ); /*  Free the buffer. */
    } else { /*  Error, we did not find a description string for this error code. */
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

#endif /*  defined(_WIN32) */

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

#ifdef _WIN32		/*  Automatically defined when targeting a Win32 applic. */

#ifndef CP_UTF16	/*  Not defined in WinNls.h for Windows SDK 8.1 */
#define CP_UTF16 1200	/*  "Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications" */
#endif

int ConvertCharacterSet(char *pszInput, size_t nInputSize,
			char *pszOutput, size_t nOutputSize,
			char *pszInputSet, char *pszOutputSet,
			int iBOM)
    {
    wchar_t *pszWideBuf;             /*  Intermediate buffer for wide characters. */
    size_t nWideSize = (2*nInputSize) + 4; /*  Size in bytes of that wide buffer. */
    UINT uCPin=0, uCPout=0;
    int nWide, nOut;

    DEBUG_FPRINTF((mf, "ConvertCharacterSet(pszIn, %ld, pszOut, %ld, %s, %s, %d)\n", \
           (long)nInputSize, (long)nOutputSize, pszInputSet, pszOutputSet, iBOM));

    switch (tolower(*pszInputSet)) {	/*  "w" <==> "win" <==> "Windows" */
      case '.':
	uCPin = GetConsoleOutputCP();
	break;
      case 'w':
	uCPin = CP_ACP;		/*  ANSI */
	break;
      case 'm':
	uCPin = CP_MACCP;	/*  Mac */
	break;
      case 'd':
	uCPin = CP_OEMCP;	/*  DOS */
	break;
      case 's':
	uCPin = CP_SYMBOL;	/*  Symbol */
	break;
      default:
	if (!sscanf(pszInputSet, "%u", &uCPin)) {
	  FAIL("Unknown input character set");
	}
	switch (uCPin) {
	  case 7:
	    uCPin = CP_UTF7;	/*  UTF-7 */
	    break;
	  case 8:
	    uCPin = CP_UTF8;	/*  UTF-8 */
	    break;
	  case 16:
	    uCPin = CP_UTF16;	/*  UTF-16 */
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
	uCPout = CP_ACP;	/*  ANSI */
	break;
      case 'm':
	uCPout = CP_MACCP;	/*  Mac */
	break;
      case 'd':
	uCPout = CP_OEMCP;	/*  DOS */
	break;
      case 's':
	uCPout = CP_SYMBOL;	/*  Symbol */
	break;
      default:
	if (!sscanf(pszOutputSet, "%u", &uCPout)) {
	  FAIL("Unknown output character set");
	}
	switch (uCPout) {
	  case 7:
	    uCPout = CP_UTF7;	/*  UTF-7 */
	    break;
	  case 8:
	    uCPout = CP_UTF8;	/*  UTF-8 */
	    break;
	  case 16:
	    uCPout = CP_UTF16;	/*  UTF-16 */
	    break;
	  default:
	    break;
	}
	break;
    }

    verbose((mf, "uCPin = %d , uCPout = %d\n", (int)uCPin, (int)uCPout));

    /* Allocate a buffer for an intermediate UTF-16 string */
    pszWideBuf = (wchar_t *)malloc(nWideSize);
    pszWideBuf += 1;	/*  Leave room for adding a BOM if needed */
    nWideSize -= 2;

    /* Convert to intermediate wide characters */
    if (uCPin != CP_UTF16) {
      nWide = MultiByteToWideChar(uCPin, 0, pszInput, (int)nInputSize, pszWideBuf, (int)nWideSize);
      if (nWide == 0)
	WIN32FAIL("Cannot convert the input!");
      else if (nWide == ERROR_NO_UNICODE_TRANSLATION)
	WIN32FAIL("Invalid UTF-8 input!");
      else verbose((mf, "Input conversion to Unicode returned %d wide characters.\n", nWide));
    } else {
      memcpy((char *)pszWideBuf, pszInput, nInputSize);
      nWide = (int)nInputSize / 2;
    }

    /* Add or remove the BOM if requested or needed */
    switch (uCPout) {
    case CP_UTF7:
    case CP_UTF8:
    case CP_UTF16:
      break;	/*  Apply the user request for all Unicode encodings */
    default:	/*  Any other code page requires removing the BOM, if any */
      iBOM = -1;
      break;
    }
#pragma warning(disable:4428)	/* Ignore the "universal-character-name encountered in source" warning */
    switch (iBOM) {
    case -1:
      if (*pszWideBuf == L'\uFEFF') {	/*  If there's a BOM, then remove it */
      	pszWideBuf += 1;
      	nWideSize -= 2;
      	nWide -= 1;
      }
      break;
    case 1:
      if (*pszWideBuf != L'\uFEFF') {	/*  If there's no BOM, then add one */
      	pszWideBuf -= 1;
      	nWideSize += 2;
      	*pszWideBuf = L'\uFEFF';
      	nWide += 1;
      }
      break;
    case 0:				/*  Leave the BOM (if present) unchanged */
    default:
      break;
    }
#pragma warning(default:4428)	/* Restore the "universal-character-name encountered in source" warning */

    /* Convert back to ANSI characters */
    if (uCPout != CP_UTF16) {
      nOut = WideCharToMultiByte(uCPout, 0, pszWideBuf, nWide, pszOutput, (int)nOutputSize, NULL, NULL);
      if (nOut == 0)
	  WIN32FAIL("Cannot convert the output!");
      else verbose((mf, "Output conversion from Unicode returned %d bytes.\n", nOut));
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

  nOutputSize -= 1; /*  Reserver 1 byte for the final NUL. */
  char_count = 0;
  bits = 0;
  while (nInputSize-- && nOutputSize) {
    c = *(pszInput++);
    if (c == '=') break; /*  End of Mime sequence */
    if (!inalphabet[c]) continue; /*  Ignore error silently */
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
    case 1: /*  Error: base64 encoding incomplete: at least 2 bits missing */
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
  char cEnc; /*  Character encoding. Q or B. */
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

  /*  Scan the input data and decode it. */
  while ((psz1 = stristr(psz0, pszHeader)))
  {
    DEBUG_FPRINTF((mf, "while(): psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    if ((size_t)(psz0-psz00) >= nBufSize) FAIL("Oops, went too far!");
    /*  Copy input up to the Mime word header. */
    n = psz1-psz0;
    strncpy(psz2, psz0, n);
    psz0 += n + nHeader;
    psz2 += n;
    /*  We've seen case for such words in quoted strings. Remove the opening quote. */
    if (n && (*(psz2-1) == '"')) psz2 -= 1;
    /*  Get the encoding byte. */
    cEnc = *(psz0++);
    if (*(psz0++) != '?') /*  Skip the second ? */
      fail("Bad Mime encoded-word header: %20.20s", psz0-(nHeader+2));
    /*  Get the rest of the encoded word. */
    for (psz1=psz0; (c=*psz1)>' '; psz1++) ;
    n = psz1-psz0;
    DEBUG_FPRINTF((mf, "Encoding: psz0=%d, psz1=%d, psz2=%d\n", psz0-psz00, psz1-psz00, psz2-psz20));
    /*  Decode the word */
    switch (toupper(cEnc))
    {
    case 'Q': /*  Quoted-printable */
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
    case 'B': /*  Base64 */
      psz2 += Base64Decode(psz2, nBufSize-(psz2-psz20), psz0, n);
      psz0 += n;
      break;
    default:
      FAIL("Unexpected character encoding");
      break;
    }
    /*  Skip the following separator spaces if there's another encoded word afterwards */
    psz1 = psz0;
    while (isspace(*psz1)) psz1++;
    if (!strncmp(psz1, "=?", 2)) psz0 = psz1;
  }
  /*  Output the end of the data, if any */
  n = nBufSize-(psz0-psz00);
  memcpy(psz2, psz0, n);
  psz2 += n;

  /*  Cleanup */
  free(pszBuf2);
  free(pszHeader);

  return psz2-psz20; /*  The output length. */
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
  LONG lErr;			/*  Win32 error */
  DWORD dwType;			/*  Actual type of the value read */

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
      pszBuf[dwSize] = '\0';	/*  Add a terminating NUL */
    } else {
      return ERROR_MORE_DATA;	/*  Buffer is too small to store the terminating NUL */
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
*									      *
\*---------------------------------------------------------------------------*/

int IsSameFile(char *pszPathname1, char *pszPathname2) {
  int iSameFile;
  char *pszBuf1 = NULL;
  char *pszBuf2 = NULL;
#if defined _WIN32
  WIN32_FILE_ATTRIBUTE_DATA attr1;
  WIN32_FILE_ATTRIBUTE_DATA attr2;
#else
  struct stat attr1;
  struct stat attr2;
#endif /* defined _WIN32 */
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
#if defined _WIN32
  bDone1 = (int)GetFileAttributesEx(pszPathname1, GetFileExInfoStandard, &attr1);
  bDone2 = (int)GetFileAttributesEx(pszPathname2, GetFileExInfoStandard, &attr2);
#else
  bDone1 = stat(pszPathname1, &attr1) + 1;
  bDone2 = stat(pszPathname2, &attr2) + 1;
#endif /* defined _WIN32 */
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

