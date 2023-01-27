/*****************************************************************************\
*									      *
*  File name:	    chars.c						      *
*									      *
*  Description:     Display all 8-bit characters in hexadecimal		      *
*									      *
*  Notes:	    Useful to see what characters look like in your current   *
*		    code page.						      *
*									      *
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
*		    Version 1.7.					      *
*		    							      *
*       © Copyright 2016-2017 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Output character tables"
#define PROGRAM_NAME    "chars"
#define PROGRAM_VERSION "1.7"
#define PROGRAM_DATE    "2023-01-27"

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
int ToUtf8(unsigned int c, char *b);
#endif /* SUPPORTS_UTF8 */

int Print(const char *pszString) { return printf("%s", pszString); } /* Output a string without a new line */
int Puts(const char *pszString) { return fputs(pszString, stdout); } /* Output a string without a new line */
#define PUTL(pszString) do { Puts(pszString); Puts(EOL); } while (0) /* Output a string with a new line in raw mode (LF untranslated) */

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

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {
      char *opt = arg+1;
      if (   streq(opt, "a")     /* -a: Display all characters */
	  || streq(opt, "-all")) {
	iAll = TRUE;
	continue;
      }
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
	if (((i+1)<argc) && sscanf(argv[++i], "%x%n", &iFirst, &nc)) {
#ifdef _WIN32
	  isUTF8 = TRUE;
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
#ifdef _WIN32
    if (!uCP) {
      uCP = atoi(arg);
      if ((uCP <= 0) || (uCP > 65535)) {
	fprintf(stderr, "Invalid code page: %s\n", arg);
	return 1;
      }
      continue;
    }
#endif /* defined(_WIN32) */
    fprintf(stderr, "Unrecognized argument %s. Ignored.\n", arg);
  }

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
    if (strstr(pszLocale, "UTF-8")) isUTF8 = TRUE;
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
    char buf[5];
    int n = ToUtf8(iFirst, buf);
    buf[n] = '\0';
    if (!n) {
      fprintf(stderr, "Invalid code point: 0x%X." EOL, iFirst);
      goto cleanup;
    }
    if (iVerbose) {
      Puts("UTF-8 ");
      for (i=0; i<n; i++) printf("\\x%02X ", buf[i]&0xFF);
      Puts(EOL);
    }
    PUTL(buf);
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
	    if (k < 0x20 || k == 0x7F) { /* All ASCII control characters */
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
	  int n = ToUtf8(l, buf);
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
\n"
#ifdef _WIN32
"Usage: chars [SWITCHES] [CODEPAGE]\n"
#else
"Usage: chars [SWITCHES]\n"
#endif
"\n\
Switches:\n\
  -?|-h|--help        Display this help screen\n\
  -a|--all            Output all characters, even control chars like CR LF, etc\n"
#ifdef _DEBUG
"\
  -d|--debug          Display debug information\n"
#endif
#if SUPPORTS_UTF8
"\
  -u|--unicode X[-Y]  Display a Unicode character, or a range of characters\n"
#endif
"\
  -v|--verbose        Display verbose information\n\
  -V|--version        Display this program version and exit\n\
"
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
#endif /* SUPPORTS_UTF8 */
