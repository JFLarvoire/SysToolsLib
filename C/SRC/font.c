/*****************************************************************************\
*		    							      *
*   Filename	    font.c						      *
*		    							      *
*   Description     Manage the console fonts				      *
*		    							      *
*   Notes	    Windows font and text functions:                          *
*    https://docs.microsoft.com/en-us/windows/desktop/gdi/font-and-text-functions
*    https://docs.microsoft.com/en-us/windows/console/console-reference       *
*		    							      *
*   History								      *
*    2018-01-24 JFL Created this program.				      *
*    2018-03-07 JFL Corrected the help screen.				      *
*    2018-05-02 JFL Corrected again the help screen.			      *
*    2018-12-06 JFL Added functions ShowConsoleFont(), SetConsoleFontSize(),  *
*		     SetConsoleFontName().				      *
*		    Use ShowConsoleFont() by default.			      *
*		    Removed the hardcoded limit on the # of supported fonts.  *
*    2018-12-07 JFL Renamed switches and changed arguments.		      *
*		    Added option -s. Version 2.0.			      *
*    2019-04-03 JFL Fixed the font setting, that did not work well with       *
*                   TrueType fonts.					      *
*		    Added an optional weight argument.			      *
*		    Version 2.1.					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.2.1.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.1.2.      *
*		    							      *
*         © Copyright 2018 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage the console fonts"
#define PROGRAM_NAME    "font"
#define PROGRAM_VERSION "2.1.2"
#define PROGRAM_DATE    "2019-06-12"

#define _UTF8_SOURCE
#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>         /* For _setmode() */
#include <fcntl.h>      /* For _setmode() */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#include <windows.h>

// Undocumented console functions from http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/
typedef struct _CONSOLE_FONT {
   DWORD index;
   COORD dim;
} CONSOLE_FONT;

BOOL WINAPI SetConsoleFont(HANDLE hOutput, DWORD fontIndex);
BOOL WINAPI GetConsoleFontInfo(HANDLE hOutput, BOOL bMaximize, DWORD numFonts, CONSOLE_FONT* info);
DWORD WINAPI GetNumberOfConsoleFonts(void);
BOOL WINAPI SetConsoleIcon(HICON hIcon);
// Get and SetCurrentConsoleFontEx do not exist in Windows XP => Get their addresses dynamically.
// BOOL WINAPI SetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);

typedef BOOL (WINAPI *PSETCONSOLEFONT)(HANDLE hOutput, DWORD fontIndex);
typedef BOOL (WINAPI *PGETCONSOLEFONTINFO)(HANDLE hOutput, BOOL bMaximize, DWORD numFonts, CONSOLE_FONT* info);
typedef DWORD (WINAPI *PGETNUMBEROFCONSOLEFONTS)(void);
typedef BOOL (WINAPI *PSETCURRENTCONSOLEFONTEX)(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
typedef BOOL (WINAPI *PGETCURRENTCONSOLEFONTEX)(HANDLE h, BOOL b, PCONSOLE_FONT_INFOEX p);

/*********************************** Other ***********************************/

#else
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

#define LIST_FONTS 1
#define SELECT_FIXED_WITDH 2

/* Global variables */
int iVerbose = FALSE;

/* Forward declarations */
void usage(void);
int IsSwitch(char *pszArg);
int ListFonts(DWORD dwCriteria);
int ShowConsoleFont(DWORD dwFlags);	// Display the current console font
int ListConsoleFontSizes(void);		// List available sizes for the console font
int SetConsoleFontSize(int iIndex);	// Select one of the available sizes ""
int SetConsoleFontName(char *pszName, char *pszSize, char *pszWeight);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    C program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|    2014-02-05 JFL Created this routine                                      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int i;
  DWORD dwFlags = 0;
  char *pszName = NULL;
  char *pszSize = NULL;
  char *pszWeight = NULL;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      char *opt = arg+1;
      if (   streq(opt, "help")
	  || streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "a")) {	/* List all fonts */
	dwFlags = LIST_FONTS;
	continue;
      }
      if (streq(opt, "c")) {	/* Change the console font sizes */
      	if (((i+1)<argc) && !IsSwitch(argv[i+1])) {
      	  return SetConsoleFontSize(atoi(argv[++i]));
      	}
	fprintf(stderr, "Error: Missing size index.\n", arg);
	return 1;
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_MORE();
	  iVerbose = TRUE;
	  continue;
	}
      )
      if (streq(opt, "l")) {	/* List fixed width fonts usable in the console */
	dwFlags = LIST_FONTS | SELECT_FIXED_WITDH;
	continue;
      }
      if (streq(opt, "p")) {	/* Display font properties, including all sizes */
	return ListConsoleFontSizes();
      }
      if (streq(opt, "s")) {	/* Save the current font setting */
	return ShowConsoleFont(1);
      }
      if (streq(opt, "v")) {	/* Verbose infos */
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      fprintf(stderr, "Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    // It's an argument: The font name, and optional size
    if (!pszName) {
      pszName = arg;
      continue;
    }
    if ((!pszSize) && (strpbrk(arg, "xX ") || (atoi(arg) < 100))) {
      pszSize = arg;
      continue;
    }
    if ((!pszWeight) && (strpbrk(arg, "wW") || (atoi(arg) >= 100))) {
      pszWeight = arg;
      continue;
    }
    fprintf(stderr, "Unexpected argument %s. Ignored.\n", arg);
    continue;
  }

  if (pszName) {
    return SetConsoleFontName(pszName, pszSize, pszWeight);
  }
  
  if (dwFlags) {
    return ListFonts(dwFlags);
  }
  
  return ShowConsoleFont(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help screen 			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    Does not return					      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: font [SWITCHES] [FONT_NAME [FONT_SIZE] [FONT_WEIGHT]]\n\
\n\
Switches:\n\
  -?        Display this help message and exit\n\
  -a        List all fonts installed on the system\n\
  -c N      Change to console font size #N, from the -p switch list\n"
#ifdef _DEBUG
"\
  -d        Output debug information\n"
#endif
"\
  -l        List fixed width fonts usable in the console\n\
  -p        List the console font properties, and alternate sizes (1) (2)\n\
  -s        Save the current console font name and size in a reusable format\n\
  -v        Display verbose information\n\
  -V        Display this program version and exit\n\
\n\
Default: Display the current console font type, name, size, and weight\n\
FONT_NAME: One of the font names listed by the -l switch\n\
FONT_SIZE: The new font size. Ex: 14 or 8x14 or \"8 14\" or \"8 x 14\"\n\
FONT_WEIGHT: The new font weight. Ex: 400 or \"W400\"\n\
\n\
Notes:\n\
(1) The Win32 APIs we use give no info about alternate sizes in Windows 10.\n\
    In all cases, the alternate sizes are only inferred from available data.\n\
    Small sizes should be good, but large sizes are more likely to be wrong.\n\
(2) The Weight value is not reliable in some versions of Windows.\n\
    Normally: 400=normal weight, 700=bold\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|                                                                             |
|   Description:    Test if an argument is a command-line switch.             |
|                                                                             |
|   Parameters:     char *pszArg	    Would-be argument		      |
|                                                                             |
|   Return value:   TRUE or FALSE					      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  return (   (*pszArg == '-')
#ifndef __unix__
	  || (*pszArg == '/')
#endif
         ); /* It's a switch */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ShowConsoleFont					      |
|                                                                             |
|   Description:    Show the current console font			      |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   0 = Success, 1 = error                                    |
|                                                                             |
|   Notes:	    Uses undocumented functions described in:		      |
|    http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/   |
|                                                                             |
|   History:								      |
|    2018-12-06 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int ShowConsoleFont(DWORD dwFlags) {
  HANDLE hConsole;
  PGETCURRENTCONSOLEFONTEX pGetCurrentConsoleFontEx;
  CONSOLE_FONT_INFOEX cfix = {0};
  CONSOLE_FONT_INFO cfi = {0};
  char szFaceName[4*LF_FACESIZE];
  int n = 0;

  // hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
  if (hConsole == INVALID_HANDLE_VALUE) return 1;

  // GetCurrentConsoleFontEx does not exist in Windows XP => Get its address dynamically.
  pGetCurrentConsoleFontEx = (PGETCURRENTCONSOLEFONTEX)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetCurrentConsoleFontEx"); 
  cfix.cbSize = sizeof(cfix);
  if (pGetCurrentConsoleFontEx && pGetCurrentConsoleFontEx(hConsole, FALSE, &cfix)) {
    // I verified that cfix.cbSize never changes in Windows 7-10, even if the FaceName field contains garbage.
    char *pszType = "";
    COORD coord;
    int isRaster = FALSE;

    if (cfix.FontFamily & TMPF_TRUETYPE) {
      pszType = "TrueType";
    } else if (cfix.FontFamily & TMPF_VECTOR) {
      BYTE TMPF_POSTSCRIPT = TMPF_DEVICE | TMPF_VECTOR | TMPF_FIXED_PITCH;
      if ((cfix.FontFamily & TMPF_POSTSCRIPT) == TMPF_POSTSCRIPT) {
	pszType = "PostScript";
      } else {
	pszType = "Vector";
      }
    } else {
      pszType = "Raster";
      isRaster = TRUE;
    }

    if ((!cfix.FaceName[2]) && ((int)(cfix.FaceName[0]) > 0xFF)) {
      cfix.FaceName[0] = 0; // Garbage there on some PCs: Three non-0 bytes, then all 0s
    }
    WideCharToMultiByte(CP_UTF8, 0, cfix.FaceName, lstrlenW(cfix.FaceName)+1, szFaceName, sizeof(szFaceName), NULL, NULL);
    // Some PCs return an empty name. Others return garbage, cleared above.
    if (streq(pszType, "Raster") && !szFaceName[0]) {
      DEBUG_PRINTF(("# No font. It's Raster, so assume Terminal\n"));
      strcpy(szFaceName, "Terminal"); // In Vista, the terminal font is missing by default
    }
    // printf("Console font: [%s] %s\n", pszType, szFaceName);

    // But like for XP, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfix.nFont);

    if (dwFlags & 1) {	// Display font params in a reusable form
      n = printf("\"%s\" %dx%d", szFaceName, coord.X, coord.Y);
      if (cfix.FontWeight != FW_NORMAL) n += printf(" W%u", cfix.FontWeight);
      n += printf("\n");
    } else {		// Display font params in a user-friendly form
      n = printf("[%s] %s (%d x %d)", pszType, szFaceName, coord.X, coord.Y);
      if (iVerbose) n += printf(" PF%02.2X", cfix.FontFamily);
      if (iVerbose || (cfix.FontWeight != FW_NORMAL)) n += printf(" W%u", cfix.FontWeight);
      n += printf("\n");
    }
  } else if (GetCurrentConsoleFont(hConsole, FALSE, &cfi)) { // This one works in XP
    COORD coord;
    // cfi.dwFontSize actually contains in XP: The window size in # characters. Ex: 120x75
    // cfi.dwFontSize is correct in >= Vista: 0
    DEBUG_PRINTF(("# Window: %d x %d\n", cfi.dwFontSize.X, cfi.dwFontSize.Y));
    // So instead, to work in both XP and Vista+, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfi.nFont);
    if (dwFlags & 1) {	// Display font params in a reusable form
      n = printf("\"\" %dx%d\n", coord.X, coord.Y);
    } else {		// Display font params in a user-friendly form
      n = printf("#%d (%d x %d)\n", cfi.nFont, coord.X, coord.Y);
    }
  }
  return (n == 0); // TRUE=1=error ; FALSE=0=No error
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ListConsoleFontSizes				      |
|                                                                             |
|   Description:    List fonts sizes available for the command console font   |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   0 = Success, 1 = error                                    |
|                                                                             |
|   Notes:	    Uses undocumented functions described in:		      |
|    http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/   |
|                                                                             |
|   History:								      |
|    2018-01-24 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int ListConsoleFontSizes() {
  HANDLE hConsole;
  PGETCURRENTCONSOLEFONTEX pGetCurrentConsoleFontEx;
  CONSOLE_FONT_INFOEX cfix = {0};
  CONSOLE_FONT_INFO cfi = {0};
  char szFaceName[4*LF_FACESIZE];
  PGETNUMBEROFCONSOLEFONTS pGetNumberOfConsoleFonts;
  int nFonts = 0;
  int iFont = 0;
  PGETCONSOLEFONTINFO pGetConsoleFontInfo;
  CONSOLE_FONT *fonts = NULL;
  CONSOLE_FONT nofont = {0};
  COORD coord = {0};
  int i, n;
  DEBUG_CODE(
    BOOL bDone;
  )

  // hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
  if (hConsole == INVALID_HANDLE_VALUE) return 1;

  // GetCurrentConsoleFontEx does not exist in Windows XP => Get its address dynamically.
  pGetCurrentConsoleFontEx = (PGETCURRENTCONSOLEFONTEX)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetCurrentConsoleFontEx"); 
  cfix.cbSize = sizeof(cfix);
  if (pGetCurrentConsoleFontEx && pGetCurrentConsoleFontEx(hConsole, FALSE, &cfix)) {
    // I verified that cfix.cbSize never changes in Windows 7-10, even if the FaceName field contains garbage.
    char *pszType = "";
    int isRaster = FALSE;

    if (cfix.FontFamily & TMPF_TRUETYPE) {
      pszType = "TrueType";
    } else if (cfix.FontFamily & TMPF_VECTOR) {
      BYTE TMPF_POSTSCRIPT = TMPF_DEVICE | TMPF_VECTOR | TMPF_FIXED_PITCH;
      if ((cfix.FontFamily & TMPF_POSTSCRIPT) == TMPF_POSTSCRIPT) {
	pszType = "PostScript";
      } else {
	pszType = "Vector";
      }
    } else {
      pszType = "Raster";
      isRaster = TRUE;
    }

    if ((!cfix.FaceName[2]) && ((int)(cfix.FaceName[0]) > 0xFF)) {
      DEBUG_WPRINTF((L"Console font: %s\n", cfix.FaceName)); // Show the garbage, before clearing it.
      cfix.FaceName[0] = 0; // Garbage there on some PCs: Three non-0 bytes, then all 0s
    }
    WideCharToMultiByte(GetConsoleOutputCP(), 0, cfix.FaceName, lstrlenW(cfix.FaceName)+1, szFaceName, sizeof(szFaceName), NULL, NULL);
    // Some PCs return an empty name. Others return garbage, cleared above.
    if (streq(pszType, "Raster") && !szFaceName[0]) strcpy(szFaceName, "Terminal"); // In Vista, the terminal font is missing by default
    // printf("Console font: [%s] %s\n", pszType, szFaceName);
    printf("Type\t%s\n", pszType);
    printf("Name\t%s\n", szFaceName);
    // if (isRaster) printf("Warning: Raster fonts may be limited to code page %d!\n", GetOEMCP());
    DEBUG_CODE_IF_ON(
      printf("# ");
      for (i=0; i<10; i++) printf("%04X ", cfix.FaceName[i]);
      printf("\n");
    )
    printf("Family\t0x%02X\n", cfix.FontFamily);

    // cfix.dwFontSize is correct in OS >= Vista:
    DEBUG_PRINTF(("dwFontSize = (%d x %d)\n", cfix.dwFontSize.X, cfix.dwFontSize.Y));
    // But like for XP, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfix.nFont);
    printf("Size\t%d x %d\n", coord.X, coord.Y);

    printf("Weight\t%d\n", cfix.FontWeight);

    iFont = (int)cfix.nFont; // Index in list of font sizes returned by GetConsoleFontInfo().
  } else if (GetCurrentConsoleFont(hConsole, FALSE, &cfi)) { // This one works in XP
    // cfi.dwFontSize actually contains in XP: The window size in # characters. Ex: 120x75
    // cfi.dwFontSize is correct in >= Vista: 0
    DEBUG_PRINTF(("Window = %d x %d\n", cfi.dwFontSize.X, cfi.dwFontSize.Y));
    // So instead, to work in both XP and Vista+, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfi.nFont);
    printf("Size\t%d x %d\n", coord.X, coord.Y);

    iFont = (int)cfi.nFont; // Index in list of font sizes returned by GetConsoleFontInfo().
  }

  // Now enumerate the available size, using the undocumented functions fount at
  // http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/
  pGetNumberOfConsoleFonts = (PGETNUMBEROFCONSOLEFONTS)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNumberOfConsoleFonts");
  if (pGetNumberOfConsoleFonts) {
    nFonts = (int)pGetNumberOfConsoleFonts();
    DEBUG_PRINTF(("GetNumberOfConsoleFonts() returns %d\n", nFonts));
  }
  printf("Count\t%d\n", nFonts); // # of fonts returned by GetConsoleFontInfo().
  pGetConsoleFontInfo = (PGETCONSOLEFONTINFO)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetConsoleFontInfo");
  if (pGetConsoleFontInfo) {
    n = nFonts;
    if (nFonts == 0) n = 100; // Windows 10 returns 0! Try reading 100 anyway
    fonts = (CONSOLE_FONT *)calloc(n, sizeof(CONSOLE_FONT));
    if (!fonts) {
      fprintf(stderr, "Error: Can't allocate memory for %d fonts\n", n);
      return 1;
    }
    DEBUG_PRINTF(("GetConsoleFontInfo()\n"));
    DEBUG_CODE(bDone = )
    pGetConsoleFontInfo(hConsole, FALSE, n, fonts);
    DEBUG_PRINTF(("  return %d\n", bDone));
    for ( ; n>0; n--) {
      if (memcmp(fonts+(n-1), &nofont, sizeof(CONSOLE_FONT))) break;
    }
    if (n != nFonts) {
      nFonts = n;
      if (iVerbose) ("# There are actually %d fonts\n", nFonts);
    }
    if (nFonts) {
      int iXPixels, iYPixels;
      printf("Index\t%d\n", iFont); // Index in list of sizes returned by GetConsoleFontInfo().
      iXPixels = (int)(fonts[iFont].dim.X * coord.X);
      iYPixels = (int)(fonts[iFont].dim.Y * coord.Y);
      for (i=0; i<nFonts; i++) {
      	int iX, iY;
      	iX = (int)fonts[i].dim.X;
      	iX = (iXPixels + (iX /2)) / iX;
      	iY = (int)fonts[i].dim.Y;
      	iY = (iYPixels + (iY /2)) / iY;
	printf("#%d\t%d x %d    \t%d x %d\n", i, iX, iY, fonts[i].dim.X, fonts[i].dim.Y);
	if (fonts[i].index != (unsigned)i) printf("Warning: Design error. Indexes don't match: (i=%d) != (index=%d)\n", i, fonts[i].index);
      }
    }
  }

  CloseHandle(hConsole);
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ListFonts						      |
|                                                                             |
|   Description:    List fonts that have given properties		      |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   0 = Success, 1 = error                                    |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2018-01-24 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

typedef struct _ENUMPARAMS {
  DWORD dwFlags;
  int nFonts;
} ENUMPARAMS;

// See https://msdn.microsoft.com/en-us/library/windows/desktop/dd145132(v=vs.85).aspx
#define TMPF_POSTSCRIPT (TMPF_DEVICE | TMPF_VECTOR | TMPF_FIXED_PITCH)

// #include <Wingdi.h>
#pragma comment(lib, "Gdi32.lib")
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
// LOGFONT structure: https://msdn.microsoft.com/en-us/library/windows/desktop/dd145037(v=vs.85).aspx
// TEXTMETRIC struct: https://msdn.microsoft.com/en-us/library/windows/desktop/dd145132(v=vs.85).aspx
int CALLBACK FontEnumFunc(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData) {
  ENUMPARAMS *pParams = (ENUMPARAMS *)lpData;
  DWORD dwFlags = pParams->dwFlags;
  char szType[16];
  char *pszType = "";
  char szCharSet[16];
  char *pszCharSet = "";

  if (   (dwFlags & SELECT_FIXED_WITDH)
      && !(   ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH)
	   && ((lplf->lfPitchAndFamily & 0xF0) == FF_MODERN) // FF = Fixed Font
	   )
      ) return 1; // Continue

  // The font matches our search criteria. Count it.
  pParams->nFonts += 1;

  // *_FONTTYPE defined in winddi.h
  if (dwType & RASTER_FONTTYPE) {
    pszType = "Raster";
  } else if (dwType & DEVICE_FONTTYPE) {
    if ((lptm->tmPitchAndFamily & TMPF_POSTSCRIPT) == TMPF_POSTSCRIPT) {
      pszType = "PostScript";
    } else {
      pszType = "Device";
    }
  } else if (dwType & TRUETYPE_FONTTYPE) {
    pszType = "TrueType";
  } else sprintf(pszType = szType, "Type 0x%02X", (int)dwType);

  // if (lptm->tmPitchAndFamily & TMPF_TRUETYPE) {
  //   pszType = "TrueType";
  // } else if (lptm->tmPitchAndFamily & TMPF_VECTOR) {
  //   if ((lptm->tmPitchAndFamily & TMPF_POSTSCRIPT) == TMPF_POSTSCRIPT) {
  //     pszType = "PostScript";
  //   } else {
  //     pszType = "Vector";
  //   }
  // } else {
  //   pszType = "Raster";
  //   isRaster = TRUE;
  // }

  // From wingdi.h:
  switch (lplf->lfCharSet) {
    case ANSI_CHARSET:        pszCharSet = "ANSI";        break;  // 0
    case DEFAULT_CHARSET:     pszCharSet = "DEFAULT";     break;  // 1
    case SYMBOL_CHARSET:      pszCharSet = "SYMBOL";      break;  // 2
    case SHIFTJIS_CHARSET:    pszCharSet = "SHIFTJIS";    break;  // 128
    case HANGEUL_CHARSET:     pszCharSet = "HANGEUL";     break;  // 129
  //case HANGUL_CHARSET:      pszCharSet = "HANGUL";      break;  // 129
    case GB2312_CHARSET:      pszCharSet = "GB2312";      break;  // 134
    case CHINESEBIG5_CHARSET: pszCharSet = "CHINESEBIG5"; break;  // 136
    case OEM_CHARSET:         pszCharSet = "OEM";         break;  // 255
    case JOHAB_CHARSET:       pszCharSet = "JOHAB";       break;  // 130
    case HEBREW_CHARSET:      pszCharSet = "HEBREW";      break;  // 177
    case ARABIC_CHARSET:      pszCharSet = "ARABIC";      break;  // 178
    case GREEK_CHARSET:       pszCharSet = "GREEK";       break;  // 161
    case TURKISH_CHARSET:     pszCharSet = "TURKISH";     break;  // 162
    case VIETNAMESE_CHARSET:  pszCharSet = "VIETNAMESE";  break;  // 163
    case THAI_CHARSET:        pszCharSet = "THAI";        break;  // 222
    case EASTEUROPE_CHARSET:  pszCharSet = "EASTEUROPE";  break;  // 238
    case RUSSIAN_CHARSET:     pszCharSet = "RUSSIAN";     break;  // 204
    case MAC_CHARSET:         pszCharSet = "MAC";         break;  // 77
    case BALTIC_CHARSET:      pszCharSet = "BALTIC";      break;  // 186
    default: sprintf(pszCharSet = szCharSet, "type 0x%02X", (int)lplf->lfCharSet); break;
  }

  if (dwFlags & LIST_FONTS) {
    printf("'\\x%02X'-'\\x%02X'   %-11s %-12s %s\n",
           (int)lptm->tmFirstChar, (int)lptm->tmLastChar, pszCharSet, pszType, lplf->lfFaceName);
  }
  return 1; // Continue
}
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */

int ListFonts(DWORD dwFlags) {
  ENUMPARAMS params = {0};
  HDC hDC;

  hDC = GetWindowDC(GetConsoleWindow());
  params.dwFlags = dwFlags;
  printf("CHARACTERS      CHARSET     TYPE         NAME\n");
  EnumFonts(hDC, NULL, FontEnumFunc, (LPARAM)&params);
  printf("Total: %d fonts\n", params.nFonts);

  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SetConsoleFontSize					      |
|                                                                             |
|   Description:    Set the Nth console font size                             |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   0 = Success, 1 = error                                    |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2018-12-06 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int SetConsoleFontSize(int iIndex) {
  BOOL bDone = FALSE;
  CONSOLE_FONT *fonts = NULL;
  CONSOLE_FONT nofont = {0};
  HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
  PSETCONSOLEFONT pSetConsoleFont = (PSETCONSOLEFONT)GetProcAddress(hKernel32, "SetConsoleFont");
  PGETNUMBEROFCONSOLEFONTS pGetNumberOfConsoleFonts = (PGETNUMBEROFCONSOLEFONTS)GetProcAddress(hKernel32, "GetNumberOfConsoleFonts");
  PGETCONSOLEFONTINFO pGetConsoleFontInfo = (PGETCONSOLEFONTINFO)GetProcAddress(hKernel32, "GetConsoleFontInfo");
  int nFonts = 0;
  HANDLE hConsole = INVALID_HANDLE_VALUE;
  int index;
  int n;
  
  if ((!pGetNumberOfConsoleFonts) || (!pGetConsoleFontInfo) || (!pSetConsoleFont)) {
    fprintf(stderr, "Error: Can't get console APIs\n");
    goto exit_SetConsoleFontSize;
  }

  // hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
  if (hConsole == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Error: Can't get a console handle\n");
    goto exit_SetConsoleFontSize;
  }

  nFonts = (int)pGetNumberOfConsoleFonts();
  DEBUG_PRINTF(("GetNumberOfConsoleFonts() returns %d\n", nFonts));
  n = nFonts;
  if (nFonts == 0) n = 100; // Windows 10 returns 0! Try reading 100 anyway.
  fonts = (CONSOLE_FONT *)calloc(n, sizeof(CONSOLE_FONT));
  if (!fonts) {
    fprintf(stderr, "Error: Can't allocate memory for %d fonts\n", n);
    goto exit_SetConsoleFontSize;
  }
  DEBUG_PRINTF(("GetConsoleFontInfo()\n"));
  bDone = pGetConsoleFontInfo(hConsole, FALSE, n, fonts);
  DEBUG_PRINTF(("  return %d\n", bDone));
  bDone = FALSE; // We're not done yet
  for (; n>0; n--) { // Search the last non-empy entry
    if (memcmp(fonts+(n-1), &nofont, sizeof(CONSOLE_FONT))) break;
  }
  if (n != nFonts) {
    nFonts = n;
    if (iVerbose) printf("There are actually %d font sizes\n", nFonts);
  }

  if (iIndex >= nFonts) {
    fprintf(stderr, "Error: There are only %d fonts\n", nFonts);
    goto exit_SetConsoleFontSize;
  }
  if (!memcmp(fonts+iIndex, &nofont, sizeof(CONSOLE_FONT))) {
    fprintf(stderr, "Error: Font %d is missing\n", iIndex);
    goto exit_SetConsoleFontSize;
  }

  index = fonts[iIndex].index;
  if (iVerbose) printf("Setting font #%d/%d, with index %d\n", iIndex, nFonts, index);
  bDone = pSetConsoleFont(hConsole, index);
  DEBUG_PRINTF(("bDone = %d\n", bDone));
exit_SetConsoleFontSize:
  if (hConsole != INVALID_HANDLE_VALUE) CloseHandle(hConsole);
  return !bDone; // TRUE=1=error ; FALSE=0=No error
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SetConsoleFontName					      |
|                                                                             |
|   Description:    Set the console font by name                              |
|                                                                             |
|   Parameters:     char *pszName       Required.                             |
|		    char *pszSize       Optional. Ex: 8x16 or "8 x 16" ...    |
|		    char *pszWeight     Optional. Default: FW_NORMAL = 400    |
|                                                                             |
|   Return value:   0 = Success, 1 = error                                    |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2018-12-06 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

typedef struct {
  char *pszName;	// The font name, for which to query information
  UINT uFontFamily;	// The only information we're interested in so far
} sFontParams;

#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
// LOGFONT structure: https://msdn.microsoft.com/en-us/library/windows/desktop/dd145037(v=vs.85).aspx
// TEXTMETRIC struct: https://msdn.microsoft.com/en-us/library/windows/desktop/dd145132(v=vs.85).aspx
int CALLBACK FontParamsCB(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData) {
  sFontParams *pFontParams = (sFontParams *)lpData;

  if (lstrcmpi(lplf->lfFaceName, pFontParams->pszName)) return 1; // Not the required font. Continue
  
  // *_FONTTYPE defined in winddi.h
  pFontParams->uFontFamily = FF_DONTCARE;		// Bits 7:4
  // The constants names and values below are slightly inconsistent
  // if (dwType & RASTER_FONTTYPE) {
  //   pFontParams->uFontFamily |= TMPF_FIXED_PITCH;
  // } else if (dwType & DEVICE_FONTTYPE) {
  //   pFontParams->uFontFamily |= TMPF_DEVICE;
  // } else if (dwType & TRUETYPE_FONTTYPE) {
  //   pFontParams->uFontFamily |= TMPF_TRUETYPE;
  // }
  // But according to the MS docs, the families are the same in both structures.
  pFontParams->uFontFamily |= dwType & 0x0F;	// Bits 3:0
  return 0;
}
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */

int SetConsoleFontName(char *pszName, char *pszSize, char *pszWeight) {
  CONSOLE_FONT_INFOEX cfix = {0};
  HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
  PGETCURRENTCONSOLEFONTEX pGetCurrentConsoleFontEx = (PGETCURRENTCONSOLEFONTEX)GetProcAddress(hKernel32, "GetCurrentConsoleFontEx");
  PSETCURRENTCONSOLEFONTEX pSetCurrentConsoleFontEx = (PSETCURRENTCONSOLEFONTEX)GetProcAddress(hKernel32, "SetCurrentConsoleFontEx");
  HANDLE hConsole;
  BOOL bDone;
  sFontParams fontParams = {0};
  HDC hDC;

  if ((!pGetCurrentConsoleFontEx) || (!pSetCurrentConsoleFontEx)) {
    fprintf(stderr, "Error: Can't get Get or SetCurrentConsoleFontEx() addresses\n");
    return 1;
  }

  // hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
  if (hConsole == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Error: Can't get a console handle\n");
    return 1;
  }

  // Get the current font size
  cfix.cbSize = sizeof cfix;
  pGetCurrentConsoleFontEx(hConsole, FALSE, &cfix);
  // cfix.dwFontSize.X;
  // cfix.dwFontSize.Y;

  if (pszSize) {
    int iX = 0;
    int iY = 0;
    if (sscanf(pszSize, "%d", &iX)) {
      char *p = strpbrk(pszSize, " \txX");
      if (p) {
      	p = strpbrk(p, "0123456789");
	if (p) sscanf(p, "%d", &iY);
      }
    }
    if (iY) {		// We got both X and Y
      cfix.dwFontSize.X = (SHORT)iX;
      cfix.dwFontSize.Y = (SHORT)iY;
    } else if (iX) {	// We got only 1 number
      cfix.dwFontSize.Y = (SHORT)iX;
    }
  }

  // Identify the new font pitch and family
  hDC = GetWindowDC(GetConsoleWindow());
  fontParams.pszName = pszName;
  EnumFonts(hDC, NULL, FontParamsCB, (LPARAM)&fontParams);
  if (fontParams.uFontFamily) {
    DEBUG_PRINTF(("Found family = 0x%X\n", fontParams.uFontFamily));
    cfix.FontFamily = fontParams.uFontFamily;
  } else {
    DEBUG_PRINTF(("Family not found\n"));
    cfix.FontFamily = FF_DONTCARE;
  }
  
  // Change the font
  cfix.nFont = 0;
  // cfix.FontFamily = FF_DONTCARE;
  // Note: Setting cfix.FontFamily = FF_DONTCARE; does not work correctly with
  // font "Liberation Mono". The font is indeed set, but it does not look good.
  // It's as if font smoothing were disabled. Tests with various values:
  //   cfix.FontFamily = 0x36;	// Good (This is the family value returned in a window where the font still looks good)
  //   cfix.FontFamily = 0x06;	// Good
  //   cfix.FontFamily = 0x30;	// Bad
  //   cfix.FontFamily = 0x34;	// Good. Bit 2 = TMPF_TRUETYPE
  //   cfix.FontFamily = 0x32;	// Bad.  Bit 1 = TMPF_VECTOR
  // Fixed by querying the font family above.
  
  if (pszWeight) {
    if ((pszWeight[0] & '\xDF') == 'W') pszWeight++;
    cfix.FontWeight = atoi(pszWeight);
  } else {
    cfix.FontWeight = FW_NORMAL;
  }
  
  MultiByteToWideChar(CP_UTF8, 0, pszName, (int)strlen(pszName)+1, cfix.FaceName, sizeof(cfix.FaceName));
  if (iVerbose) printf("Setting font \"%s\" size (%d x %d) family 0x%X weigth %d\n", pszName, cfix.dwFontSize.X, cfix.dwFontSize.Y, cfix.FontFamily, cfix.FontWeight);
  bDone = pSetCurrentConsoleFontEx(hConsole, FALSE, &cfix);
  DEBUG_PRINTF(("bDone = %d\n", bDone));
  if (!bDone) {
    fprintf(stderr, "Error: Failed to set font \"%s\" size (%d x %d) family 0x%X weigth %d\n", pszName, cfix.dwFontSize.X, cfix.dwFontSize.Y, cfix.FontFamily, cfix.FontWeight);
  }

  CloseHandle(hConsole);
  return !bDone; // TRUE=1=error ; FALSE=0=No error
}

