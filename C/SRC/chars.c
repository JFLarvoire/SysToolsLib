/*****************************************************************************\
*									      *
*  File name:	    chars.c						      *
*									      *
*  Description:     Display all 8-bit characters in hexadecimal		      *
*									      *
*  Notes:	    Useful to see what characters look like in your current   *
*		    code page.						      *
*									      *
*		    In Unix, use the locale command to see what is supported: *
*		    locale charmap	Display the current character set     *
*		    locale -m		List all supported character sets     *
*		    locale -a		List all available locale files       *
*		    locale-gen LOCALE	Generate a new locale file	      *
*		    							      *
*  History:								      *
*    1995-11-03 JFL jf.larvoire@hp.com created this program.		      *
*    2015-01-08 JFL Fix output for Linux, including when encoding is UTF-8.   *
*		    Version 1.1.					      *
*    2017-03-06 JFL Added an optional code page argument for Windows.         *
*		    Version 1.2.					      *
*    2017-11-17 JFL Fixed the output of the NUL character.		      *
*    2019-01-14 JFL Added option -u to display Unicode characters or ranges.  *
*		    Improved error reporting when switching code pages.	      *
*		    Added option -v to display verbose information.	      *
*		    Version 1.4.					      *
*    2019-01-16 JFL Avoid outputing bytes \x80-\xFF by default for UTF-8 CPs. *
*		    Version 1.4.1.					      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.4.2.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.4.3.      *
*    2020-04-19 JFL Added support for MacOS. Version 1.5.                     *
*    2022-02-01 JFL Prevent a misalignment in Windows Terminal. Version 1.6.  *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 1.6.1.		      *
*    2023-01-14 JFL Allow building BIOS & LODOS versions. Version 1.6.2.      *
*    2023-01-16 JFL Make sure the output columns are always aligned, even     *
*		    when outputing undefined or 0-width characters.           *
*    2023-01-18 JFL Moved GetCursorPosition() to SysLib.		      *
*    2023-01-26 JFL Fixed the remaining alignment issues in DOS.	      *
*		    Merged the DOS & Windows exceptions into an ANSI case.    *
*		    This allows removing the Windows Terminal detection code. *
*		    Detect failures to get the cursor coordinates. This may   *
*		    happen in Unix, for example in FreeBSD.		      *
*    2023-01-27 JFL Use a simpler alignment method, which now works well and  *
*		    fast in all Unix versions.				      *
*		    Improved the system locale detection in Unix, and avoid   *
*		    displaying non-existent characters by default.	      *
*		    Display more consistent and helpful verbose messages.     *
*		    Display debug messages in the debug version only.         *
*    2023-01-30 JFL Added support for any locale in Unix, even if non-ASCII.  *
*		    In Windows, set the code page using new option -c/--cp.   *
*		    For single characters, display the UTF16 & UTF32 encodings.
*    2023-01-31 JFL Restructured to allow displaying any set of characters,   *
*		    with the char. code provided in several possible formats. *
*    2023-02-01 JFL Also allow passing a single character instead of a code.  *
*		    Version 2.0.					      *
*		    							      *
*       © Copyright 2016-2017 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Output characters, or tables of characters"
#define PROGRAM_NAME    "chars"
#define PROGRAM_VERSION "2.0"
#define PROGRAM_DATE    "2023-02-01"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _BIOS
#include <unistd.h>	/* For getpid(), isatty(), read(), write() */
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#include <windows.h>

#include <io.h>		/* For _setmode() */
#include <fcntl.h>	/* For _O_BINARY */

#define EOL "\r\n"

#define SUPPORTS_UTF8 TRUE
#define EXTRA_CHARS_IN_CONTROL_CODES TRUE
#define ANSI_IS_OPTIONAL TRUE	/* The old console is non-ANSI; The new terminal is ANSI */

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#include <io.h>		/* For _setmode() */
#if !(defined(_BIOS) || defined(_LODOS))
#include <fcntl.h>	/* For _O_BINARY */
#endif

#define EOL "\r\n"

#define SUPPORTS_UTF8 FALSE
#define EXTRA_CHARS_IN_CONTROL_CODES TRUE

#ifdef _BIOS
#define ANSI_IS_OPTIONAL FALSE	/* The BIOS never processes ANSI escape sequences */
#else
#define ANSI_IS_OPTIONAL TRUE	/* In DOS this depends on the use of ANSI.SYS */
#endif

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define EOL "\r\n"

#define SUPPORTS_UTF8 FALSE
#define EXTRA_CHARS_IN_CONTROL_CODES TRUE
#define ANSI_IS_OPTIONAL TRUE	/* Assuming it's like in DOS */

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#include <locale.h>
#include <ctype.h>
#include <errno.h>

#define EOL "\n"

#define SUPPORTS_UTF8 TRUE
#define EXTRA_CHARS_IN_CONTROL_CODES FALSE
#define ANSI_IS_OPTIONAL FALSE	/* Posix requires ANSI escape sequences support */

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#ifndef _BIOS
#define PUTC(c) fputc(c, stdout) /* Do not use printf %c for the char, else the NUL char is not written */
#else
#define PUTC(c) putchar((char)(c)) /* BIOSLIB's putchar() uses the BIOS int 10H */
#endif

#ifndef CDECL
#ifdef _MSC_VER
#define CDECL _cdecl /* LODOS builds generate a warning if main() does not specify this */
#else
#define CDECL
#endif
#endif

/* SysToolsLib include files */
#include "debugm.h"     /* SysToolsLib debug macros. Include first. */
#include "console.h"	/* SysLib console management. Ex: GetCursorPosition */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

char spaces[] = "            ";

void usage(void);

#if SUPPORTS_UTF8
typedef unsigned short WORD;
int ToUtf8(unsigned int c, char *b);
int ToUtf16(unsigned int c, WORD *w);
int FromUtf8(char *psz, int *pi);
#endif /* SUPPORTS_UTF8 */

#ifdef _WIN32
int GetCpArgv(UINT cp, char ***pArgv);
#endif

int Print(const char *pszString) { return printf("%s", pszString); } /* Output a string without a new line */
int Puts(const char *pszString) { return fputs(pszString, stdout); } /* Output a string without a new line */
#define PUTL(pszString) do { Puts(pszString); Puts(EOL); } while (0) /* Output a string with a new line in raw mode (LF untranslated) */
int ParseCharCode(char *pszString, int *pCode, int *pOutFlags);
int PrintCharCode(int iCode, int iFlags);
#define PCC_DUMP 0x01
#define PCC_UNI  0x02
int PrintRange(int iFirst, int iLast, int iVerbose);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     Program main initialization routine			      |
|									      |
|   Parameters      int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS			      |
|									      |
|   History								      |
|    1995-11-03 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int CDECL main(int argc, char *argv[]) {
  int i, j;
#ifdef _WIN32
  unsigned uCP = 0;
  unsigned uCP0 = GetConsoleOutputCP();
  unsigned uCP1 = uCP0;
  CPINFOEX cpi = {0};
#endif
  int iAll = FALSE;
  int iFirst = 0;
  int iLast = -1;
  int iMaxLast = 0;
  int iVerbose = FALSE;
  int iBase;
  int nBlock = 0;
#if SUPPORTS_UTF8
  int isUTF8 = FALSE;
#endif /* SUPPORTS_UTF8 */
#ifdef _UNIX
  char *pszLocale = setlocale(LC_ALL, "");
  char *pszNewLocale = pszLocale;
#endif
#ifndef _BIOS
  int isTTY = isatty(1);
#else
  int isTTY = TRUE; /* The BIOS version can only write to the console */
#endif
#if ANSI_IS_OPTIONAL
  int isANSI = FALSE; /* ANSI terminals interpret the ESC character */
  int iErr;
#endif
  int isMBCS = FALSE;
  int isASCII = FALSE;
  int iExitCode = 1;
  int n, m;
  int iDone = 0;
  int iOutFlags;	/* Context info returned by ParseCharCode() */

#if SUPPORTS_UTF8
#ifdef _WIN32
  char **argvCP0;
  char **argvANSI;
  char **argvUtf8;
  int argc0 = GetCpArgv(uCP0, &argvCP0);
  int argc8 = GetCpArgv(CP_UTF8, &argvUtf8);
  DEBUG_CODE(
    if ((argc0 != argc) || (argc8 != argc)) {
      fprintf(stderr, "Bug: argc = %d; argc0 = %d; argc8 = %d\n", argc, argc0, argc8);
    }
  )
  i = argc0 + argc8; /* Prevent an annoying warning in non-debug mode */
  /* The MSVC Standard C library converts Unicode arguments to the ANSI code page,
     independently of the current console code page. */
  argvANSI = argv;
  argv = argvUtf8; /* Change to arguments encoded in UTF-8 */
  isUTF8 = (uCP0 == CP_UTF8);
#endif
#ifdef _UNIX
  if (pszLocale) {
    if (strstr(pszLocale, "UTF-8") || strstr(pszLocale, "utf8")) isUTF8 = TRUE;
  }
#endif
#endif /* SUPPORTS_UTF8 */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {
      char *opt = arg+1;
      if (   streq(opt, "a")     /* -a: Display all characters */
	  || streq(opt, "-all")) {
	iAll = TRUE;
	continue;
      }
#ifdef _WIN32
      if (   streq(opt, "c")	/* -c: Set the code page */
	  || streq(opt, "-cp")) {
        char *pszCP = ((i+1)<argc) ? argv[++i] : "0";
	uCP = atoi(pszCP);
	if ((uCP <= 0) || (uCP > 65535)) {
	  fprintf(stderr, "Invalid code page: %s\n", arg);
	  return 1;
	}
	continue;
      }
#endif /* defined(_WIN32) */
#ifdef _DEBUG
      if (streq(opt, "d")) {
	DEBUG_ON();
	iVerbose = TRUE;
	continue;
      }
#endif
      if (   streq(opt, "h")	    /* Display usage */
	  || streq(opt, "help")	/* The historical name of that switch */
	  || streq(opt, "-help")
	  || streq(opt, "?")) {
	usage();
      }
#if SUPPORTS_UTF8
      if (   streq(opt, "u")     /* -u: Display unicode characters */
	  || streq(opt, "-unicode")) {
	int nc;
	/* if (((i+1)<argc) && sscanf(argv[++i], "%x%n", &iFirst, &nc)) { */
	if (((i+1)<argc) && ((nc = ParseCharCode(argv[++i], &iFirst, &iOutFlags)) != 0)) {
	  isUTF8 = ((iOutFlags & PCC_UNI) != 0);
#ifdef _WIN32
	  uCP = 65001;
#endif /* defined(_WIN32) */
	  iLast = iFirst;
	  DEBUG_PRINTF(("Code point 0x%X", iFirst));
	} else {
	  fprintf(stderr, "No or bad unicode code point.\n");
	  return 1;
	}
	arg = argv[i];
	if ((arg[nc] == '-') && sscanf(arg+nc+1, "%x", &iLast)) {
	  if (iLast < iFirst) iLast = iFirst;
	  DEBUG_PRINTF((" until 0x%X", iLast));
	}
	DEBUG_PRINTF(("\n"));
	continue;
      }
#endif /* SUPPORTS_UTF8 */
      if (   streq(opt, "v")     /* -a: Display all characters */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	continue;
      }
      if (   streq(opt, "V")     /* -V: Display the version */
	  || streq(opt, "-version")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
      fprintf(stderr, "Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    n = ParseCharCode(arg, &iFirst, &iOutFlags);
    DEBUG_PRINTF(("c0 = \\x%02X; c1 = \\x%02X\n", arg[0] & 0xFF, arg[1] & 0xFF));
    DEBUG_PRINTF(("n = %d; iFirst = \\x%02X; iOutFlags = 0x%X\n", n, iFirst, iOutFlags));
    if (n > 0) {
      int iLen = (int)strlen(arg);
      int iFlags = 0;
      iDone = 1;
      if (iVerbose) iFlags |= PCC_DUMP;
      if (n == iLen) {
#if SUPPORTS_UTF8
        int isUnicodeChar = isUTF8 || ((iOutFlags & PCC_UNI) != 0);
	if (isUnicodeChar) iFlags |= PCC_UNI;
#ifdef _WIN32
	if (isUnicodeChar && !isUTF8) {
	  DEBUG_PRINTF(("Switching to code page %d.\n", uCP));
	  fflush(stdout);
	  if (!SetConsoleOutputCP(CP_UTF8)) {
	    fprintf(stderr, "Failed to switch to code page %d.\n", uCP);
	    return 1;
	  }
	  uCP1 = CP_UTF8;
	  if (isUnicodeChar && (iFlags & PCC_DUMP) && (strlen(argvCP0[i]) == 1)) {
	    int iCode1 = argvCP0[i][0] & 0xFF;
	    if ((argvCP0[i][0] != '?') || (arg[0] == '?')) {
	      printf("CP%u \\x%02X\n", uCP0, iCode1);
	    } else {
	      printf("CP%u (undefined)\n", uCP0);
	    }
	  }
	} else {
	  DEBUG_PRINTF(("Active code page: %d\n", uCP0));
	}
#endif /* defined(_WIN32) */
#ifdef _UNIX
	if (isUnicodeChar && !isUTF8) {
	  DEBUG_PRINTF(("Switching to the C.UTF-8 locale.\n"));
	  fflush(stdout);
	  pszNewLocale = setlocale(LC_ALL, "C.UTF-8");
	  if (!pszNewLocale) {
	    fprintf(stderr, "Failed to switch to the C.UTF-8 locale.\n");
	    return 1;
	  }
	}
#endif /* defined(_UNIX) */
#endif /* SUPPORTS_UTF8 */
        PrintCharCode(iFirst, iFlags);
#if SUPPORTS_UTF8
#ifdef _WIN32
        if (uCP1 != uCP0) {
	  DEBUG_PRINTF(("Switching back to code page %d.\n", uCP0));
	  fflush(stdout);
	  if (!SetConsoleOutputCP(uCP0)) {
	    fprintf(stderr, "Failed to switch back to code page %d.\n", uCP0);
	    return 1;
	  }
	  uCP1 = uCP0;
        }
#endif /* defined(_WIN32) */
#ifdef _UNIX
        if (pszNewLocale != pszLocale) {
	  DEBUG_PRINTF(("Switching back to the %s locale.\n", pszLocale));
	  fflush(stdout);
	  if (!setlocale(LC_ALL, pszLocale)) {
	    fprintf(stderr, "Failed to switch back to the %s locale.\n", pszLocale);
	    return 1;
	  }
	}
#endif /* defined(_UNIX) */
#endif /* SUPPORTS_UTF8 */
        continue;
      }
      if (arg[n] != '-') goto arg_ignored;
      m = ParseCharCode(arg+n+1, &iLast, NULL);
      if (!m) goto arg_ignored;
      if ((n+1+m) != iLen) goto arg_ignored;
      PrintRange(iFirst, iLast, iVerbose);
      continue;
    }
arg_ignored:
    fprintf(stderr, "Unrecognized argument %s. Ignored.\n", arg);
  }

  if (iDone) return 0; /* Already displayed individual characters or ranges */

#ifdef _UNIX
  if (iVerbose) printf("The system locale is %s\n", pszLocale);
#endif /* _UNIX */

#if ANSI_IS_OPTIONAL
  /* Check if the console handles ANSI escape sequences */
  if (isTTY) {
    int iCol, iCol0;
    PUTC('\r'); /* Return to the first column (0 or 1 depending on systems) */
    fflush(stdout);
    iErr = GetCursorPosition(&iCol0, NULL);
    if (iErr) {
failed_to_get_cursor_coord:
      fprintf(stderr, "Failed to get the cursor coordinates\n");
      goto cleanup;
    }
    Puts(" \x1B[1D"); /* Output a space, then move the cursor back left */
    fflush(stdout);
    iErr = GetCursorPosition(&iCol, NULL);
    if (iErr) goto failed_to_get_cursor_coord;
    /* If it's an ANSI terminal, the cursor will be back on the initial column;
       Else a string like " ?[1D" will be displayed, with the cursor on column 4 or 5 */
    isANSI = (iCol == iCol0);
    DEBUG_PRINTF(("\niCol = %d\n", iCol));
    if (!isANSI) Puts("\r     \r"); /* Erase the garbage displayed on a non-ANSI terminal */
    if (iVerbose) printf("This %s an ANSI terminal\n", isANSI ? "is" : "isn't");
  }
#endif /* ANSI_IS_OPTIONAL */

#ifdef _WIN32
  if (uCP && (uCP != uCP0)) {
    if (iVerbose) printf("Switching to code page %d.\n", uCP);
    if (!SetConsoleOutputCP(uCP)) {
      fprintf(stderr, "Failed to switch to code page %d.\n", uCP);
      return 1;
    }
    uCP1 = uCP;
  } else {
    if (iVerbose) printf("Active code page: %d\n", uCP0);
  }
/****** Now on, All exits must be done via the cleanup label in the end ******/
  if (!GetCPInfoEx(uCP1, 0, &cpi)) {
    fprintf(stderr, "Error: Unknown Code Page %d.\n", uCP);
    goto cleanup;
  }
  /* Check if MSBCS code set */
  isMBCS = (cpi.MaxCharSize > 1);
#endif /* defined(_WIN32) */

#if SUPPORTS_UTF8
#ifdef _WIN32
  if (uCP1 == 65001) isUTF8 = TRUE;
  if (uCP1 == 20127) isASCII = TRUE;
#endif
#ifdef _UNIX
  if (pszLocale) {
    if (strstr(pszLocale, "UTF-8") || strstr(pszLocale, "utf8")) isUTF8 = TRUE;
  /* Note that Unix XTerm considers bytes \x80-\x9F as control sequences
     equivalent to ESC @, ESC A, ESC B, ..., ESC _ .
     Do not output them, else there may be unpredictable effects on the console,
     depending on what follows. */
    if (   streq(pszLocale, "C")
        || streq(pszLocale, "POSIX")
        || !strncmp(pszLocale, "C.", 2)
        || !strncmp(pszLocale, "C/", 2)) isASCII = TRUE;
  }
#endif
  if (isUTF8) isMBCS = TRUE;
#endif /* SUPPORTS_UTF8 */

#if SUPPORTS_UTF8
  if (isUTF8) {
    iMaxLast = 0x10FFFF;
    if (iVerbose) printf("This is 21-bits Unicode (0x00-0x10FFFF)\n");
  } else
#endif /* SUPPORTS_UTF8 */
#ifndef _MSDOS /* TODO: Detect MBCS locales in DOS */ 
  if (isMBCS) {
    iMaxLast = 0xFFFF; /* TODO: Can we detect the actual maximum value? */
    if (iVerbose) printf("This is a Multi-Byte Character Set\n");
  } else
#endif /* _MSDOS */
  if (!isASCII) {
    iMaxLast = 0xFF;
    if (iVerbose) printf("This is an 8-bits character set (0x00-0xFF)\n");
  }
#ifndef _MSDOS /* TODO: Is there an ASCII locale in DOS? */ 
  else {
    iMaxLast = 0x7F;
    if (iVerbose) printf("This is 7-bits ASCII (0x00-0x7F)\n");
  }
#endif /* _MSDOS */
  /* Default to the whole set for SBCSs, and to the first 128 characters for smaller or larger sets */
  if (iLast == -1) iLast = (iMaxLast == 0xFF) ? 0xFF : 0x7F;
  /* Warn the user if his requested is likely to fail to display anything */
  if (iLast > iMaxLast) {
    fprintf(stderr, "Warning: The last requested char. %02X is larger than the last possible one %02X\n", iLast, iMaxLast);
  }

#if defined(_WIN32) || (defined(_MSDOS) && !(defined(_BIOS) || defined(_LODOS)))
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode(1, _O_BINARY ); /* fileno(stdout) = 1 */
#endif

/*************** Now on, all output must use EOL instead of \n ***************/

#if SUPPORTS_UTF8
  if (iFirst == iLast) {
    if (!PrintCharCode(iFirst, iVerbose ? PCC_DUMP : 0)) {
      fprintf(stderr, "Invalid code point: 0x%X." EOL, iFirst);
      goto cleanup;
    }
    goto cleanup;
  }
#endif /* SUPPORTS_UTF8 */

  for (iBase = (iFirst & -0x80); iBase < ((iLast + 0x7F) & -0x80); iBase += 0x80) {
    int iCol = 0;
    int iDigits = 2;
    for (i=0x100; i; i<<=4) if (iBase >= i) iDigits += 1;
    if (nBlock) Puts(EOL);
    if (iVerbose || (iFirst != 0) || ((iLast != 0x7F) && (iLast != 0xFF))) {
      printf("[0x%02X-0x%02X]" EOL, iBase, iBase + 0x7F);
    }
    for (j=0; j<16; j++) {
      for (i=0; i<8; i++) {
	int k, l;
#if EXTRA_CHARS_IN_CONTROL_CODES
	int iRow0, iCol0, iRow1, iCol1;
#endif
	if (!(i&3)) iCol += Print("  ");

	l = k = (iBase + 16*i)+j;
	if ((k < iFirst) || (k > iLast)) {
	  iCol += printf("%.*s", iDigits+4, spaces);
	  continue;
	}
	if (!iAll) switch (k) {
#ifndef _UNIX /* All DOS and Windows terminals interpret these control codes */
	  case 0x07:		/* Bell */
	  case 0x08:		/* Backspace */
	  case 0x09:		/* Tabulation */
	  case 0x0A:		/* Line Feed */
	  case 0x0D:		/* Carrier Return */
	    l = ' ';
	    break;
#endif
	  default:
#ifdef _UNIX
	    if (iscntrl(k)) { /* All control characters in the current locale */
	      l = ' ';
	      break;
	    }
#endif
#if ANSI_IS_OPTIONAL /* DOS and Windows ANSI terminals choke on these control codes */
	    if (isANSI) {
	      switch (k) {
		case 0x00:	/* Null */
		case 0x1B:	/* Escape */
		case 0x7F:	/* Delete */
		  l = ' ';
		  break;
	      }
	    }
#endif /* ANSI_IS_OPTIONAL */
#if SUPPORTS_UTF8
	    if (isUTF8 & (k >= 0x80) && (k <= 0x9F)) {
	      l = ' '; /* \u80-\u9F are additional control characters */
	      break;
	    }
#endif /* SUPPORTS_UTF8 */
	    break;
	}

	iCol += printf("  %02X ", k); /* Print the numeric code, with at least 2 characters */
#if EXTRA_CHARS_IN_CONTROL_CODES
        iRow0 = iCol0 = 0; /* Prevent an (incorrect) uninitialized variable warning later on */
	if (isTTY && (l < ' ')) { /* Old DOS and Windows consoles display extra characters in the control codes range */
	  fflush(stdout);
	  iErr = GetCursorPosition(&iCol0, &iRow0);
	  if (iErr) goto failed_to_get_cursor_coord;
	}
#endif /* EXTRA_CHARS_IN_CONTROL_CODES */
#if SUPPORTS_UTF8
	if ((l > 0x7F) && isUTF8) {
	  char buf[5];
	  n = ToUtf8(l, buf);
	  buf[n] = '\0';
	  Puts(buf);
	} else
#endif /* SUPPORTS_UTF8 */
	PUTC(l); /* Do not use printf %c for the char, else the NUL char is not written */
	iCol += 1; /* Theoretically, the cursor should move 1 column forward, but there are exceptions */
#if EXTRA_CHARS_IN_CONTROL_CODES
	if (isTTY && (l < ' ')) { /* The cursor may make wild moves for some control codes */
	  fflush(stdout);
	  iErr = GetCursorPosition(&iCol1, &iRow1);
	  if (iErr) goto failed_to_get_cursor_coord;
	  if (iCol1 == iCol0 && iRow1 == iRow0) {
	    /* The cursor did not move, so add a space to keep the alignment of subsequent columns */
	    PUTC(' ');
	  } else if (iCol1 < iCol0) {
	    /* Most likely this is FF or VT, and the terminal interpreted it as CRLF */
	    SetCursorPosition(iCol0+1, iRow0-1);
	  }
	}
#endif /* EXTRA_CHARS_IN_CONTROL_CODES */
	if (isTTY && (l >= '\x7F')) { /* The cursor may not move for undefined or 0-width characters */
#ifdef _UNIX
	  printf("\x1B[%dG", iCol+1); /* Go to absolute column iCol (1-based in ANSI coordinates) */
#else /* DOS, Windows */
	  fflush(stdout);
	  GetCursorPosition(&iCol1, &iRow1);
	  if (iCol1 != iCol) SetCursorPosition(iCol, iRow1);
#endif
	}
      }
      Puts(EOL);
      iCol = 0;
      nBlock += 1;
    }
  }

  iExitCode = 0;

cleanup:
#ifdef _WIN32
  if (uCP && (uCP != uCP0)) {
    if (iVerbose) printf("Switching back to code page %d." EOL, uCP0);
    if (!SetConsoleOutputCP(uCP0)) {
      fprintf(stderr, "Failed to switch to code page %d." EOL, uCP0);
      return 1;
    }
  }
#endif

  return iExitCode;
}

void usage(void) {
#if SUPPORTS_UTF8 && _WIN32
  unsigned uCP0 = GetConsoleOutputCP();;
  if (uCP0 != 65001) SetConsoleOutputCP(65001);
#endif

  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: chars [SWITCHES] [CHAR|CHARCODE] ...\n\
\n\
Switches:\n\
  -?|-h|--help        Display this help screen\n\
  -a|--all            Output all characters, even control chars like CR LF, etc\n"
#ifdef _WIN32
"\
  -c|--cp CODEPAGE    Use CODEPAGE. Default: Use the current console code page\n"
#endif
#ifdef _DEBUG
"\
  -d|--debug          Display debug information\n"
#endif
#if SUPPORTS_UTF8
"\
  -u|--unicode CHARCODE-CHARCODE    Display a range of Unicode characters\n"
#endif
"\
  -v|--verbose        Display verbose information\n\
  -V|--version        Display this program version and exit\n\
\n\
Char Code: (X=[1-9,A-F] N=[1-9])  (Use -v to display the various encodings)\n\
  XX                  Hexadecimal code in the current code page. Ex: 41 for 'A'\n\
  XXH                 Optional H suffix for hexadecimal\n\
  NNT                 Optional T suffix for Decimal. Ex: 65T = 'A'\n\
  \\xXX                Hexadecimal code in the current code page. Ex: 41 for 'A'\n\
  \\tNN                Decimal code in the current code page. Ex: \\t65 for 'A'\n\
  \\oNN                Octal code in the current code page. Ex: \\o101 for 'A'\n"
#if SUPPORTS_UTF8
"\
  \\uXXXX              Unicode code point. Ex: \\u20AC for '€'\n\
  U+XXXX              Unicode code point. Ex: U+1F310 for '🌐'\n"
#endif
#ifndef _UNIX
"\n\
Note: Shows the characters that can be output with the C fputc() function.\n\
      In some code pages, it may be possible to display more by directly\n\
      storing characters into the video RAM buffer.\n\
"
#endif
#include "footnote.h"
);

#if SUPPORTS_UTF8 && _WIN32
  if (uCP0 != 65001) SetConsoleOutputCP(uCP0);
#endif

  exit(0);
}

#if SUPPORTS_UTF8
#define B(c) ((char)(c))
int ToUtf8(unsigned int c, char *b) {
  char *b0 = b;
  if (c<0x80) *b++=B(c);
  else if (c<0x800) *b++=B(192+c/64), *b++=B(128+c%64);
  else if (c-0xd800u<0x800) return 0;
  else if (c<0x10000) *b++=B(224+c/4096), *b++=B(128+c/64%64), *b++=B(128+c%64);
  else if (c<0x110000) *b++=B(240+c/262144), *b++=B(128+c/4096%64), *b++=B(128+c/64%64), *b++=B(128+c%64);
  else return 0;
  return (int)(b-b0);
}
int ToUtf16(unsigned int c, WORD *pw) {
  WORD *pw0 = pw;
  if (!(c>>16)) {
    *pw++ = (WORD)(c);
  } else {
    WORD w0, w1;
    c -= 0x10000;
    w0 = (WORD)(c & 0x3FF);
    c >>= 10;
    if (c > 0x3FF) return 0;
    w1 = (WORD)(c & 0x3FF);
    *pw++ = (WORD)(0xD800 + w1);
    *pw++ = (WORD)(0xDC00 + w0);
  }
  return (int)(pw-pw0);
}
/* Decode a UTF-8 string into the corresponding code point */
/* Don't complain about non canonic encodings (Ex: 3-byte encodings where 2 would have been enough) */
/* Returns the number of characters read. <=0 = Failed = Minus the number of invalid characters */

int FromUtf8(char *psz, int *pi) {
  int c0, c1, c2, c3, i=0, nChar=0;
  do {
    c0 = psz[nChar++] & 0xFF;
    if (!c0) return 0;			/* Empty string */
    i = c0;
    if (c0 < 0x80) break;		/* This is one ASCII byte */

    c1 = psz[nChar++] & 0xFF;
    if ((c1 & 0xC0) != 0x80) return -1;	/* Invalid UTF-8 tail byte => Incomplete 1-byte sequence */
    i = ((i & 0x1F) << 6) | (c1 & 0x3F);    /* 5 bits from c0 and 6 from c1 */
    if ((c0 & 0xE0) == 0xC0) break;	/* This is a 2-bytes UTF-8 sequence */

    c2 = psz[nChar++] & 0xFF;
    if ((c2 & 0xC0) != 0x80) return -2;	/* Invalid UTF-8 tail byte => Incomplete 2-byte sequence */
    i = ((i & 0x3FF) << 6) | (c2 & 0x3F);   /* 4 bits from c0 and 6 each from c1 & c2 */
    if ((c0 & 0xF0) == 0xE0) break;	/* This is a 3-bytes UTF-8 sequence */

    c3 = psz[nChar++] & 0xFF;
    if ((c3 & 0xC0) != 0x80) return -3;	/* Invalid UTF-8 tail byte => Incomplete 3-byte sequence */
    i = ((i & 0x7FFF) << 6) | (c3 & 0x3F);  /* 3 bits from c0 and 6 each from c1 to c3 */
    if ((c0 & 0xF8) == 0xF0) break;	/* This is a 4-bytes UTF-8 sequence */

    return -4;				/* Incomplete 4-byte or more sequence */
  } while (0);
  *pi = i;
  return nChar;
}

#endif /* SUPPORTS_UTF8 */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ParseCharCode					      |
|									      |
|   Description     Convert a string representing a character code	      |
|									      |
|   Parameters      char *pszString	    The input string	 	      |
|		    int *pCode		    Where to output the char. code    |
|									      |
|   Returns	    The number of characters read. 0=Failed		      |
|									      |
|   History								      |
|    2023-01-30 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int ParseCharCode(char *pszString, int *pCode, int *piFlags) {
  int nPrefix = 0;
  int nConv = 0;
  int nRead;
  char cFormat = '\0';
  char szFormat[5] = "%x%n";

  DEBUG_ENTER(("ParseCharCode(\"%s\", %p)" EOL, pszString, pCode));

  if (piFlags) *piFlags = 0;
  if (!pszString[0]) return 0;
  if (!pszString[1]) { /* A single 8-bits character */
    *pCode = pszString[0] & 0xFF;
    return 1;
  }
#if SUPPORTS_UTF8
  nRead = FromUtf8(pszString, pCode);
  if (nRead == (int)strlen(pszString)) { /* A single UTF-8 character */
    if (piFlags) *piFlags |= PCC_UNI;
    return nRead;
  }
#endif /* SUPPORTS_UTF8 */
  if (pszString[0] == '\\') switch (pszString[1]) {{ /* C or Python style \xXX, etc */
    /* We've already eliminated the case where pszString[1] == '\0' */
    case 'u':
    case 'U':
      if (piFlags) *piFlags |= PCC_UNI;
      /* Fallthrough to the 'x' case */
    case 'x':
    case 'X':
      cFormat = 'x';
      break;
    case 't':
    case 'T':
      cFormat = 'd';
      break;
    case 'o':
    case 'O':
      cFormat = 'o';
      break;
    default:
      break;
    }
  }
  if ((pszString[0] == 'U') && (pszString[1] == '+')) { /* Unicode standard U+XXXX */
    if (piFlags) *piFlags |= PCC_UNI;
    cFormat = 'x';
  }
  if (cFormat) {
    szFormat[1] = cFormat;
    nPrefix += 2;
  }
  if (!sscanf(pszString+nPrefix, szFormat, pCode, &nConv)) {
    RETURN_INT_COMMENT(0, ("Scan failed"));
  }
  nRead = nPrefix + nConv;
  while (!nPrefix) {
    if ((pszString[nRead] == 'u') || (pszString[nRead] == 'U')) {
      if (piFlags) *piFlags |= PCC_UNI;
      nRead += 1;
      break;
    }
    if ((pszString[nRead] == 'h') || (pszString[nRead] == 'H')) {
      nRead += 1;
      break;
    }
    if ((pszString[nRead] == 't') || (pszString[nRead] == 'T')) {
      int nConv0 = nConv;
      szFormat[1] = 'd';
      if (!sscanf(pszString+nPrefix, szFormat, pCode, &nConv)) {
	RETURN_INT_COMMENT(0, ("Rescan failed"));
      }
      if (nConv != nConv0) return 0; /* Ex: "1DT" */
      nRead = nPrefix + nConv + 1;
      break;
    }
    break;
  }
  RETURN_INT_COMMENT(nRead, ("Success"));
}

#ifdef _UNIX
char *GetCharmap(char *pszBuf, size_t nBufSize) {
  FILE *hPipe = popen("locale charmap", "r");
  if (!hPipe) return NULL;
  fgets(pszBuf, nBufSize, hPipe);
  pclose(hPipe);
  return pszBuf;
}
#endif

int PrintCharCode(int iCode, int iFlags) {
  DEBUG_PRINTF(("PrintCharCode(0x%02X, 0x%X)" EOL, iCode, iFlags));
#if SUPPORTS_UTF8
  if (iFlags & PCC_UNI) { /* Print a Unicode character */
    int i;
    char buf[5];
    int n8 = ToUtf8(iCode, buf);
    buf[n8] = '\0';
    if (!n8) {
      fprintf(stderr, "Invalid code point: 0x%X." EOL, iCode);
      return 0;
    }
    if (iFlags & PCC_DUMP) {
      WORD buf16[2];
      int n16 = ToUtf16(iCode, buf16);
      printf("Unicode U+%04X" EOL, iCode); /* The standard requires at least 4 hexadecimal characters */
      Puts("UTF-8  ");
      for (i=0; i<n8; i++) printf("\\x%02X", buf[i]&0xFF);
      Puts(EOL);
      Puts("UTF-16 ");
      for (i=0; i<n16; i++) printf("\\u%04X", buf16[i]);
      Puts(EOL);
      printf("UTF-32 \\U%08X" EOL, iCode);
      Puts("'");
    }
    Puts(buf);
  } else
#endif /* SUPPORTS_UTF8 */
  {		/* Print an 8-bits code page character */
    if (iFlags & PCC_DUMP) {
#ifdef _MSDOS
      printf("Char \\x%02X" EOL, iCode); /* Display at least 2 hexadecimal characters */
#endif
#ifdef _WIN32
      unsigned uCP = GetConsoleOutputCP();
      printf("CP%u \\x%02X" EOL, uCP, iCode); /* Display at least 2 hexadecimal characters */
#endif
#ifdef _UNIX
      char szCharmap[64];
      if (!GetCharmap(szCharmap, sizeof(szCharmap))) {
	fprintf(stderr, "Can't run `locale charmap`. %s" EOL, strerror(errno));
	return 0;
      }
      printf("%s \\x%02X" EOL, szCharmap, iCode); /* Display at least 2 hexadecimal characters */
#endif
      Puts("'");
    }
    Puts((char *)&iCode);
  }
  if (iFlags & PCC_DUMP) Puts("'" EOL);
  return 1;
}
int PrintRange(int iFirst, int iLast, int iVerbose) {
  return iFirst + iLast + iVerbose;
}

#ifdef _WIN32

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        BreakArgLine                                              |
|                                                                             |
|   Description     Break the Windows command line into standard C arguments  |
|                                                                             |
|   Parameters      LPSTR pszCmdLine    NUL-terminated argument line          |
|                   char *pszArg[]      Array of arguments pointers           |
|                                                                             |
|   Returns         int argc            Number of arguments found. -1 = Error |
|                                                                             |
|   Notes           MSVC library startup \" parsing rule is:                  |
|                   2N backslashes + " ==> N backslashes and begin/end quote  |
|                   2N+1 backslashes + " ==> N backslashes + literal "	      |
|                   N backslashes ==> N backslashes                           |
|                                                                             |
|   History 								      |
|    1993-10-05 JFL Initial implementation within devmain().		      |
|    1994-04-14 JFL Extracted from devmain, and created this routine.	      |
|    1995-04-07 JFL Extracted from llkinit.c.				      |
|    1996-09-26 JFL Adapted to Win32 programs.				      |
|    1996-12-11 JFL Use Windows string routines.			      |
|    2001-09-18 JFL Set argv[0] with actual module file name.		      |
|                   Manage quoted strings as a single argument.               |
|    2001-09-25 JFL Only process \x inside strings.                           |
|    2014-03-04 JFL Removed the C-style \-quoting of characters, which was    |
|                   convenient, but incompatible with MSVC argument parsing.  |
|                   Removed the limitation on the # of arguments.             |
|                   Made the code compatible with ANSI and UTF-8 encodings.   |
|    2017-02-05 JFL Check memory allocation errors, and if so return -1.      |
|    2020-02-06 JFL Don't escape the final " in a last arg like: "C:\"        |
*                                                                             *
\*---------------------------------------------------------------------------*/

int BreakArgLine(LPSTR pszCmdLine, char ***pppszArg) {
  int i, j;
  int argc = 0;
  char c, c0;
  char *pszCopy;
  int iString = FALSE;	/* TRUE = string mode; FALSE = non-string mode */
  int nBackslash = 0;
  char **ppszArg;
  int iArg = FALSE;	/* TRUE = inside an argument; FALSE = between arguments */

  ppszArg = (char **)malloc((argc+1)*sizeof(char *));
  if (!ppszArg) return -1;

  /* Make a local copy of the argument line */
  /* Break down the local copy into standard C arguments */

  pszCopy = malloc(lstrlen(pszCmdLine) + 1);
  if (!pszCopy) return -1;
  /* Copy the string, managing quoted characters */
  for (i=0, j=0, c0='\0'; ; i++) {
    c = pszCmdLine[i];
    /* printf("i=%d n=%d c0=%c c=%c\n", i, nBackslash, c0 ? c0 : ' ', c ? c : ' '); */
    if (!c) {		    /* End of argument line */
      for ( ; nBackslash; nBackslash--) pszCopy[j++] = '\\'; /* Output pending \s */
      pszCopy[j++] = c;
      break;
    }
    if ((!iArg) && (c != ' ') && (c != '\t')) { /* Beginning of a new argument */
      iArg = TRUE;
      ppszArg[argc++] = pszCopy+j;
      ppszArg = (char **)realloc(ppszArg, (argc+1)*sizeof(char *));
      if (!ppszArg) return -1;
      pszCopy[j] = c0 = '\0';
    }
    if (c == '\\') {	    /* Escaped character in string (maybe) */
      nBackslash += 1; 
      continue;
    }
    if (c == '"') {
      if (nBackslash && iString && !pszCmdLine[i+1]) continue; /* This really is the end of string, not an escaped " */
      if (nBackslash & 1) { /* Output N/2 \ and a literal " */
      	for (nBackslash >>= 1; nBackslash; nBackslash--) pszCopy[j++] = '\\';
	pszCopy[j++] = c0 = c;
	continue;
      }
      if (nBackslash) {	    /* Output N/2 \ and switch string mode */
      	for (nBackslash >>= 1; nBackslash; nBackslash--) pszCopy[j++] = '\\';
      }
      iString = !iString;
      continue;
    }
    for ( ; nBackslash; nBackslash--) pszCopy[j++] = '\\'; /* Output pending \s */
    if ((!iString) && ((c == ' ') || (c == '\t'))) { /* End of an argument */
      iArg = FALSE;
      c = '\0';
    }
    pszCopy[j++] = c0 = c;
  }

  ppszArg[argc] = NULL;
  *pppszArg = ppszArg;

  /* for (i=0; i<argc; i++) printf("BAL: arg=\"%s\"\n", ppszArg[i]); */

  return argc;
}

int GetCpArgv(UINT cp, char ***pArgv) {
  LPWSTR lpwCommandLine;
  int n;
  WCHAR wc;
  char *pNewBuf;
  char *pszArgLine;

  /* Get the Unicode command line */  
  lpwCommandLine = GetCommandLineW();
  /* Trim tail spaces */
  n = lstrlenW(lpwCommandLine);
  while (n && ((wc = lpwCommandLine[n-1]) != L'\0') && ((wc == L' ') || (wc == L'\t'))) lpwCommandLine[--n] = L'\0';
  /* Allocate space for the MBCS command-line copy */
  n += 1;	/* Count the final NUL */
  pszArgLine = malloc(4 * n); /* Worst case */
  if (!pszArgLine) return -1; /* Memory allocation failed */
  /* Convert the Unicode command line to the requested code page */
  n = WideCharToMultiByte(cp,			/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpwCommandLine,	/* lpWideCharStr, */
			  n,			/* cchWideChar, */
			  pszArgLine,		/* lpMultiByteStr, */
			  (4 * n),		/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  if (!n) {
    fprintf(stderr, "Warning: Can't convert the argument line to CP %u\n", cp);
    pszArgLine[0] = '\0';
  }
  pNewBuf = realloc(pszArgLine, n+1); /* Resize the memory block to fit the MBCS line */
  if (pNewBuf) pszArgLine = pNewBuf; /* Should not fail since we make it smaller, but may move */

  return BreakArgLine(pszArgLine, pArgv);
}

#endif /* defined(_WIN32) */

