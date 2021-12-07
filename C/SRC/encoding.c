/*****************************************************************************\
*                                                                             *
*   Filename	    encoding.c						      *
*									      *
*   Contents	    Find the encoding of text files			      *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2021-06-02 JFL Created this program.                                     *
*    2021-06-03 JFL Restructured error messages output.                       *
*    2021-12-07 JFL Updated help screen.                                      *
*		    							      *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Find the encoding of text files"
#define PROGRAM_NAME "encoding"
#define PROGRAM_VERSION "0.9"
#define PROGRAM_DATE    "2021-12-07"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
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

/* C front end to COM C++ method IMultiLanguage2::DetectInputCodepage() */
#include <MLang.h>
#include <objbase.h>
#pragma comment(lib, "ole32.lib")
HRESULT DetectInputCodepage(DWORD dwFlags, DWORD dwPrefCP, char *pszBuffer, INT *piSize, DetectEncodingInfo *lpInfo, INT *pnInfos);

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define IGNORECASE TRUE

#endif /* defined(_WIN32) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#error "This program is for Windows only. Use iconv instead on Unix systems."

#endif /* defined(__unix__) */

/********************** End of OS-specific definitions ***********************/

/* My favorite string comparison routines. */
#define streq(s1, s2) (!strcmp(s1, s2))     /* Test if strings are equal */
#define strieq(s1, s2) (!strcmpi(s1, s2))   /* Idem, not case sensitive */

#define verbose(args) if (iVerbose) do {fprintf args;} while (0)

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */
#define BLOCKSIZE (4096)	/* Number of characters that will be allocated in each loop. */
#else
#define BLOCKSIZE (1024*1024)	/* Number of characters that will be allocated in each loop. */
#endif

#define FLAG_VERBOSE	0x0001		/* Display the pathname operated on */
#define FLAG_RECURSE	0x0002		/* Recursive operation */
#define FLAG_NOCASE	0x0004		/* Ignore case */

typedef enum {
  METHOD_LIBX,
  METHOD_COM
} method_t;

typedef struct {
  int iFlags;			/* ShowAllFilesEncoding() options */
  int *pNProcessed;		/* Optional pointer to a number of files tested */
  method_t method;		/* Which encoding detection method to use */
  DWORD dwFlags;		/* Method-specific flags */
  UINT cpHint;			/* COM API specific code page hint */
} encoding_detection_opts;

/* Global variables */

char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int iVerbose = 0;
FILE *mf;			/* Message output file */

/* Function prototypes */

int printError(char *pszFormat, ...);	/* Print errors in a consistent format */
int IsSwitch(char *pszArg);
int ShowAllFilesEncoding(char *pszName, encoding_detection_opts *pOpts);
int ShowFileEncoding(char *pszName, encoding_detection_opts *pOpts);
char *NewPathName(const char *path, const char *name);
int GetProgramNames(char *argv0);
int isEffectiveDir(const char *pszPath);

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

void usage(int iRet) {
  printf("%s", PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
\n\
    encoding [OPTIONS] [PATHNAME [...]]\n\
\n\
Options:\n\
  -?                This help\n\
  -com [OPTIONS]    Use Windows IMultiLanguage2 COM API\n\
"
#ifdef _DEBUG
"\
  -d                Output debug information\n"
#endif
"\
  -libx [OPTIONS]   Use MsvcLibX heuristics (Default)\n\
  -r                Scan subdirectories recursively\n\
  -v                Display verbose information\n\
  -V                Display this program version\n\
\n\
Pathname: A file pathname or [DIRECTORY\\]wildcards or - for stdin. Default: -\n\
\n\
MsvcLibX options:\n\
  flags: Hexadecimal combination of the following flags. Default=0=All set\n\
");
  printf("    0x%02X      Test if Binary\n", BE_TEST_BINARY);
  printf("    0x%02X      Test if ASCII\n", BE_TEST_ASCII);
  printf("    0x%02X      Test if Windows system code page\n", BE_TEST_SYSTEM);
  printf("    0x%02X      Test if UTF-8\n", BE_TEST_UTF8);
  printf("    0x%02X      Test if UTF-16\n", BE_TEST_UTF16);
  printf("    0x%02X      Test if UTF-32\n", BE_TEST_UTF32);
  printf("\
\n\
COM API options: See IMultiLanguage2::DetectInputCodepage doc on the Web\n\
  flags: Hexadecimal value passed in dwFlag. Default: 0\n\
  cp: Preferred code page. Default: 0=Let Windows choose\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");

  exit(iRet);
}

#pragma warning(disable:4706)	/* Ignore the "assignment within conditional expression" warning */

int __cdecl main(int argc, char *argv[]) {
  int     i;
  encoding_detection_opts opts = {0};
  int nPathsProcessed = 0;

  mf = stdout;

  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  /* Process arguments */

  for (i=1; i<argc; i++) {
    char *pszArg = argv[i];
    if (IsSwitch(pszArg)) {	    /* It's a switch */
      char *pszOpt = pszArg+1;
      if (streq(pszOpt, "?")) {		/* -?: Help */
      	usage(0);
      }
#ifdef _DEBUG
      if (streq(pszOpt, "d")) {
	DEBUG_ON();
	iVerbose = TRUE;
	continue;
      }
#endif
      if (streq(pszOpt, "com")) {	/* -com: Test Windows DetectInputCodepage() */
      	opts.method = METHOD_COM;
	if (((i+1) < argc) && isdigit(argv[i+1][0])) opts.dwFlags = (DWORD)strtol(argv[++i], NULL, 16);
	if (((i+1) < argc) && isdigit(argv[i+1][0])) opts.cpHint = (DWORD)atoi(argv[++i]);
	continue;
      }
      if (streq(pszOpt, "libx")) {	/* -tdc: Test MsvcLibX GetBufferEncoding() */
      	opts.method = METHOD_LIBX;
	if (((i+1) < argc) && isdigit(argv[i+1][0])) opts.dwFlags = (DWORD)strtol(argv[++i], NULL, 16);
	continue;
      }
      if (streq(pszOpt, "r")) {		/* Process files recursively in all subdirectories */
	opts.iFlags |= FLAG_RECURSE;
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
      /* Unsupported switches are ignored */
      fprintf(stderr, "Warning: Unexpected switch ignored: %s\n", pszArg);
      continue;
    }
    /* Process every argument as a file name */
    ShowAllFilesEncoding(pszArg, &opts);
    nPathsProcessed += 1;
  }

  if (!nPathsProcessed) ShowAllFilesEncoding("-", &opts);

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ShowAllFilesEncoding				      |
|									      |
|   Description     Detect the encoding of a set of files, and display it     |
|									      |
|   Parameters      char *pszName	The file pathname, with wildcards     |
|		    encoding_detection_opts *pOpts	Options		      |
|		    							      |
|   Returns	    0=Success, else error				      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2021-06-02 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int ShowAllFilesEncoding(char *path, encoding_detection_opts *pOpts) {
  char *pPath;
  char *pName;
  char *pPath2 = NULL;
  char *pPath3 = NULL;
  int iErr;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFNM = (pOpts->iFlags & FLAG_NOCASE) ? FNM_CASEFOLD : 0;
  size_t len;

  DEBUG_ENTER(("ShowAllFilesEncoding(\"%s\");\n", path));

  if ((!path) || !(len = strlen(path))) RETURN_INT_COMMENT(1, ("path is empty\n"));

  if ((!strpbrk(path, "*?")) && !(pOpts->iFlags & FLAG_RECURSE)) {	/* If there are no wild cards */
    iErr = ShowFileEncoding(path, pOpts);	    /* Remove that file, and we're done */
    if (iErr) nErr += 1;
    goto cleanup_and_return;
  }

  pPath2 = strdup(path);
  if (!pPath2) {
out_of_memory:
    printError("Out of memory");
fail:
    nErr += 1;
    goto cleanup_and_return;
  }
  pPath = dirname(pPath2);
  pPath3 = strdup(path);
  if (!pPath3) goto out_of_memory;
  pName = basename(pPath3);

  if (strpbrk(pPath, "*?")) { /* If there are wild cards in the path */
    printError("Error: Wild cards aren't allowed in the directory name");
    goto fail;
  }

  pDir = opendirx(pPath);
  if (!pDir) {
    printError("Error: Can't access \"%s\": %s", pPath, strerror(errno));
    goto fail;
  }
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */
  while ((pDE = readdirx(pDir))) { /* readdirx() ensures d_type is set */
    char *pPathname = NewPathName(pPath, pDE->d_name);
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (!pPathname) goto out_of_memory;
    switch (pDE->d_type) {
      case DT_DIR:
process_files_in_subdirectory:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	if (pOpts->iFlags & FLAG_RECURSE) {
	  char *pPathname2 = NewPathName(pPathname, pName);
	  if (!pPathname2) goto out_of_memory;
      	  nErr += ShowAllFilesEncoding(pPathname2, pOpts);
      	  free(pPathname2);
      	}
      	break;
      default:
      	if (fnmatch(pName, pDE->d_name, iFNM) == FNM_NOMATCH) {
      	  if (   (pDE->d_type == DT_LNK)
      	      && (pOpts->iFlags & FLAG_RECURSE)
      	      && (isEffectiveDir(pPathname))) goto process_files_in_subdirectory;
      	  break;
      	}
      	if (pOpts->iFlags & FLAG_VERBOSE) printf("%s\n", pPathname);
	iErr = ShowFileEncoding(pPathname, pOpts);
	if (iErr) {
          /* The error message has already been displayed by ShowFileEncoding() */
	  nErr += 1; /* Continue the directory scan, looking for other files to process */
	} else {
	  if (pOpts->pNProcessed) *(pOpts->pNProcessed) += 1; /* Number of files successfully processed */
	}
      	break;
    }
    free(pPathname);
  }
  closedirx(pDir);

cleanup_and_return:
  free(pPath2);
  free(pPath3);

  RETURN_INT_COMMENT(nErr, (nErr ? "%d detections failed\n" : "Success\n", nErr));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ShowFileEncoding					      |
|									      |
|   Description     Detect the encoding of one file, and display it	      |
|									      |
|   Parameters      char *pszName			The file pathname     |
|		    encoding_detection_opts *pOpts	Options		      |
|		    							      |
|   Returns	    0=Success, else error				      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2021-06-02 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int ShowFileEncoding(char *pszName, encoding_detection_opts *pOpts) {
  FILE *sf = NULL;		/* Source file handle */
  size_t  nRead = 0;		/* Number of characters read in one loop. */
  size_t  nTotal = 0;		/* Total number of characters read. */
  size_t  nBufSize = BLOCKSIZE;
  char   *pszBuffer = (char*)malloc(BLOCKSIZE);
  UINT cp = CP_UNDEFINED;
  char *pszEncoding;
  int i;
  char *pszDisplayName = pszName;
  
  if (streq(pszName, "-")) pszName = NULL;
  if (!pszName) pszDisplayName = "stdin";
  
  if (!pszBuffer) {
fail_no_mem:
    printError("Not enough memory for reading %s", pszDisplayName);
    return 1;
  }

  if (!pszName) {
    sf = stdin;
#if defined(_MSDOS) || defined(_WIN32)
    _setmode(_fileno(stdin), _O_BINARY); /* Force stdin to untranslated */
#endif
  } else {
    sf = fopen(pszName, "rb");
    if (!sf) {
      printError("Can't open file %s", pszDisplayName);
      return 1;
    }
  }

  /* Read the data */
  DEBUG_FPRINTF((mf, "// Reading the input from %s\n", (sf == stdin) ? "stdin" : pszName));
  while (!feof(sf)) {
    nRead = fread(pszBuffer+nTotal, 1, BLOCKSIZE, sf);
    nTotal += nRead;
    if ((nTotal+BLOCKSIZE)>nBufSize) {
      if (!((pszBuffer = (char*)realloc(pszBuffer, nTotal + BLOCKSIZE)))) {
	goto fail_no_mem;
      }
      nBufSize += BLOCKSIZE;
    }
    Sleep(0);	    /* Release end of time-slice */
  }
  verbose((mf, "Read %d input bytes.\n", (int)nTotal));

  /* 
    Windows provides an API that is supposed to detect a text encoding.
    They warn that this cannot be reliably detected in all cases, which is true.
    I implemented this in the hope that this API would give better results than
    my simple heuristic that follows. But unfortunately, the results are bad.
    It basically fails most of the times, except when the text is pure ASCII.
    Anyway, I'm leaving this in, in case of a future miracle. Like maybe Windows
    using open-source libraries like uchardet, or Google's ced.
  */
  if (pOpts->method == METHOD_COM) { /* Then use the COM API IMultiLanguage2::DetectInputCodepage() */
    HRESULT hr;
    int iSize = (int)nTotal;
    DetectEncodingInfo dei[10];
    int iCount = sizeof(dei) / sizeof(DetectEncodingInfo); // # of elements of dei
    int iMaxConf = -32767; /* For very small texts, iConfidence == -1 */
    static char szCP[6];

    DEBUG_FPRINTF((mf, "Assuming the input to be CP %u\n", pOpts->cpHint));
    hr = DetectInputCodepage(pOpts->dwFlags, pOpts->cpHint, pszBuffer, &iSize, dei, &iCount);
    if (FAILED(hr)) {
      fprintf(stderr, "IMultiLanguage2::DetectInputCodepage() failed\n");
    } else {
      DEBUG_FPRINTF((mf, "IMultiLanguage2::DetectInputCodepage() found in the first %d bytes:\n", iSize));
      for (i=0; i<iCount; i++) {
      	int iConfidence = (int)(dei[i].nConfidence); /* For very small texts, iConfidence == -1 */
      	UINT cp2 = (unsigned int)(dei[i].nCodePage);
	DEBUG_FPRINTF((mf, "CP %u, in %d%% of the text, with %d%% confidence.\n", cp2, (int)(dei[i].nDocPercent), iConfidence));
	if (iConfidence > iMaxConf) {
	  iMaxConf = iConfidence;
	  cp = cp2;
	  sprintf(szCP, "%u", cp);
	}
      }
    }
    verbose((mf, "Windows' IMultiLanguage2 COM API detected CP: %s\n", szCP));
  }

  /*
    MsvcLibX provides a simple heuristic for selecting among the most common cases in Windows:
    Binary; ASCII; The Windows system code page; UTF-8; UTF-16; UTF-32
  */
  if (pOpts->method == METHOD_LIBX) { /* Then use MsvcLibX heuristics to detect the input data encoding */
    char szMsg[100];
    char szValue[10];
    cp = GetBufferEncoding(pszBuffer, nTotal, pOpts->dwFlags);
    /* Ideally we should default to the current console code page for input from a pipe,
       and to the ANSI code page for input from a file. */
    switch (cp) {
      case CP_UNDEFINED: sprintf(szMsg, "Unrecognized encoding, possibly binary"); break;
      case CP_ACP: sprintf(szMsg, "Windows system code page %d", systemCodePage); break;
      case CP_ASCII: sprintf(szMsg, "US-ASCII code page %d", CP_ASCII); break;
      case CP_UTF7: sprintf(szMsg, "UTF-7 code page %d", CP_UTF7); break;
      case CP_UTF8: sprintf(szMsg, "UTF-8 code page %d", CP_UTF8); break;
      case CP_UTF16: sprintf(szMsg, "UTF-16 code page %d", CP_UTF16); break;
      case CP_UTF32: sprintf(szMsg, "UTF-32 code page %d", CP_UTF32); break;
      default: sprintf(szMsg, "Code page %d", cp); sprintf(szValue, "%u", cp); break;
    }
    verbose((mf, "MsvcLibX detected input type: %s\n", szMsg));
  }

  switch (cp) {
    char szEnc[8];
    case CP_UNDEFINED: pszEncoding = "Binary"; break;
    case CP_ACP: pszEncoding = "Windows"; break;
    case CP_ASCII: pszEncoding = "ASCII"; break;
    case CP_UTF7: pszEncoding = "UTF-7"; break;
    case CP_UTF8: pszEncoding = "UTF-8"; break;
    case CP_UTF16: pszEncoding = "UTF-16"; break;
    case CP_UTF32: pszEncoding = "UTF-32"; break;
    default: sprintf(szEnc, "CP%d", cp); pszEncoding = szEnc; break;
  }
  printf("%s\t%s\n", pszEncoding, pszName ? pszName : "");

  free(pszBuffer);

  return 0;
}

#pragma warning(default:4706)	/* Restore the "assignment within conditional expression" warning */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProgramNames					      |
|									      |
|   Description     Extract the program names from argv[0]		      |
|									      |
|   Parameters      char *argv[0]					      |
|									      |
|   Returns	    0							      |
|									      |
|   Notes	    Sets global variables program and progcmd.		      |
|		    Designed to work independantly of MsvcLibX.		      |
|		    							      |
|   History								      |
|    2018-03-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    printError						      |
|									      |
|   Description     Print error messages with a consistent format	      |
|									      |
|   Parameters      char *pszFormat					      |
|		    ...							      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    Uses global variables program and progcmd,		      |
|		    set by GetProgramNames().				      |
|		    							      |
|   History								      |
|    2018-05-31 JFL Created this routine				      |
|    2021-06-03 JFL Do not append a .					      |
*									      *
\*---------------------------------------------------------------------------*/

int printError(char *pszFormat, ...) {
  va_list vl;
  int n;

  n = fprintf(stderr, "%s: Error: ", program);
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, "\n");
  va_end(vl);

  return n;
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
|   Function	    isEffectiveDir					      |
|									      |
|   Description     Check if pathname refers to an existing directory	      |
|									      |
|   Parameters      const char *path		The directory name	      |
|		    							      |
|   Returns	    TRUE or FALSE					      |
|		    							      |
|   Notes	    Resolves links to see what they point to		      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2020-03-16 JFL Use stat instead of lstat, it faster and simpler!         |
*									      *
\*---------------------------------------------------------------------------*/

int isEffectiveDir(const char *pszPath) {
  struct stat sStat;
  int iErr = stat(pszPath, &sStat);
  if (iErr) return 0;
  return S_ISDIR(sStat.st_mode);
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
|   Function	    NewPathName						      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    Pointer to the new pathname, or NULL if allocation failed.|
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
|    2019-02-06 JFL Added call to new routine TrimDot, removing all ./ parts. |
*									      *
\*---------------------------------------------------------------------------*/

void TrimDot(char *path) { /* Remote ./ parts in the path */
  char *pIn = path;
  char *pOut = path;
  char c;
  int first = TRUE;
  for ( ; (c = *(pIn++)) != '\0'; ) {
    if (first && (c == '.') && (*pIn == DIRSEPARATOR_CHAR)) {
      pIn += 1; /* Eat up the / and continue */
      continue;
    }
    *(pOut++) = c;
    first = (   (c == DIRSEPARATOR_CHAR)
#if OS_HAS_DRIVES
	     || ((pIn == (path+2)) && (c == ':'))
#endif
            );
  }
  *(pOut++) = c;
}

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  TrimDot(buf);
  return buf;
}

