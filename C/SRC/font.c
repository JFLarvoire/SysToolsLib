/*****************************************************************************\
*		    							      *
*   Filename	    font.c						      *
*		    							      *
*   Description     Get information about fonts on this system		      *
*		    							      *
*   Notes	    Windows font and text functions:                          *
*    https://msdn.microsoft.com/en-us/library/windows/desktop/dd144821(v=vs.85).aspx
*		    							      *
*   History								      *
*    2018-01-24 JFL Created this program.				      *
*    2018-03-07 JFL Corrected the help screen.				      *
*    2018-05-02 JFL Corrected again the help screen.			      *
*		    							      *
*         © Copyright 2018 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.0.2"
#define PROGRAM_DATE    "2018-05-02"

#define _UTF8_SOURCE
#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>         /* For _setmode() */
#include <fcntl.h>      /* For _setmode() */
#include "debugm.h"

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#include <windows.h>

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define OS_NAME "DOS"

#endif

/*********************************** Other ***********************************/

#ifndef OS_NAME
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

#define LIST_FONTS 1
#define SELECT_FIXED_WITDH 2

/* Forward declarations */
char *version(int iLibsVer);
void usage(void);
int IsSwitch(char *pszArg);
int ListFonts(DWORD dwCriteria);
int ListConsoleFonts(void);

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
  int iVerbose = FALSE;
  DWORD dwFlags = 0;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      if (   streq(arg+1, "help")
	  || streq(arg+1, "-help")
	  || streq(arg+1, "h")
	  || streq(arg+1, "?")) {
	usage();
      }
      if (streq(arg+1, "c")) {	/* List console fonts */
	ListConsoleFonts();
	return 0;
      }
      if (streq(arg+1, "f")) {	/* List fixed width fonts */
	dwFlags = LIST_FONTS | SELECT_FIXED_WITDH;
	continue;
      }
      if (streq(arg+1, "l")) {	/* List all fonts */
	dwFlags = LIST_FONTS;
	continue;
      }
      if (streq(arg+1, "v")) {	/* Verbose infos */
	iVerbose = TRUE;
	continue;
      }
      if (streq(arg+1, "V")) {	/* Display version */
	printf("%s\n", version(TRUE));
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    printf("Unexpected argument %s. Ignored.\n", arg);
    continue;
  }

  if (dwFlags) ListFonts(dwFlags);

  return 0;
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

/* Get the program version string, optionally with libraries versions */
char *version(int iLibsVer) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME DEBUG_VERSION;
  char *pszVer = NULL;
  if (iLibsVer) {
    char *pszLibVer = ""
#if defined(_MSVCLIBX_H_)	/* If used MsvcLibX */
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
#if defined(__SYSLIB_H__)	/* If used SysLib */
#include "syslib_version.h"
	  " ; SysLib " SYSLIB_VERSION
#endif
    ;
    pszVer = (char *)malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  printf("\n\
font version %s - Get information about this system's fonts.\n\
\n\
Usage: font [SWITCHES] [CODEPAGE]\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n\
  -c          List the console font sizes. (Only current size in Windows 10)\n\
  -f          List fixed width fonts installed.\n\
  -l          List all fonts installed.\n\
  -v          Display verbose information\n\
  -V          Display this program version and exit\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
", version(FALSE));
#ifdef __unix__
  printf("\n");
#endif

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
|   Function:	    ListConsoleFonts					      |
|                                                                             |
|   Description:    List fonts sizes available for the command console font   |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   The number of fonts found                                 |
|                                                                             |
|   Notes:	    Uses undocumented functions described in:		      |
|    http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/   |
|                                                                             |
|   History:								      |
|    2018-01-24 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

// Undocumented console functions from http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/
typedef struct _CONSOLE_FONT {
   DWORD index;
   COORD dim;
} CONSOLE_FONT;

BOOL WINAPI SetConsoleFont(HANDLE hOutput, DWORD fontIndex);
BOOL WINAPI GetConsoleFontInfo(HANDLE hOutput, BOOL bMaximize, DWORD numFonts, CONSOLE_FONT* info);
DWORD WINAPI GetNumberOfConsoleFonts(void);
BOOL WINAPI SetConsoleIcon(HICON hIcon);

typedef BOOL (WINAPI *PGETCONSOLEFONTINFO)(HANDLE hOutput, BOOL bMaximize, DWORD numFonts, CONSOLE_FONT* info);
typedef DWORD (WINAPI *PGETNUMBEROFCONSOLEFONTS)(void);

// GetCurrentConsoleFontEx does not exist in Windows XP => Get its address dynamically.
typedef BOOL (WINAPI *PGETCURRENTCONSOLEFONTEX)(HANDLE h, BOOL b, PCONSOLE_FONT_INFOEX p);

int ListConsoleFonts() {
  HANDLE hConsole;
  PGETCURRENTCONSOLEFONTEX pGetCurrentConsoleFontEx;
  CONSOLE_FONT_INFOEX cfix = {0};
  CONSOLE_FONT_INFO cfi = {0};
  char szFaceName[4*LF_FACESIZE];
  PGETNUMBEROFCONSOLEFONTS pGetNumberOfConsoleFonts;
  DWORD nFonts = 0;
  PGETCONSOLEFONTINFO pGetConsoleFontInfo;
  CONSOLE_FONT fonts[100] = {0}; // It's actually font sizes, and there are usually about 10 sizes available.
  int i;

  // hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
  if (hConsole == INVALID_HANDLE_VALUE) return 0;

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
      DEBUG_CODE(
	wprintf(L"Console font: %s\n", cfix.FaceName); // Show the garbage, before clearing it.
      )
      cfix.FaceName[0] = 0; // Garbage there on some PCs: Three non-0 bytes, then all 0s
    }
    WideCharToMultiByte(GetConsoleOutputCP(), 0, cfix.FaceName, lstrlenW(cfix.FaceName)+1, szFaceName, sizeof(szFaceName), NULL, NULL);
    printf("Console font: [%s] %s\n", pszType, szFaceName); // Some PCs return an empty name. Others return garbage, cleared above.
    // if (isRaster) printf("Warning: Raster fonts may be limited to code page %d!\n", GetOEMCP());
    DEBUG_CODE(
      for (i=0; i<10; i++) printf("%04X ", cfix.FaceName[i]);
      printf("\n");
    )
    printf("Family = 0x%02X\n", cfix.FontFamily);

    // cfix.dwFontSize is correct in in >= Vista: 0
    DEBUG_CODE(
      printf("dwFontSize = [%dx%d]\n", cfix.dwFontSize.X, cfix.dwFontSize.Y);
    )
    // But like for XP, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfix.nFont);
    printf("Font size: %d x %d\n", coord.X, coord.Y);

    printf("Font size #%d\n", cfix.nFont); // Index in list of sizes returned by GetNumberOfConsoleFonts().
  } else if (GetCurrentConsoleFont(hConsole, FALSE, &cfi)) { // This one works in XP
    COORD coord;
    // cfi.dwFontSize actually contains in XP: The window size in # characters. Ex: 120x75
    // cfi.dwFontSize is correct in >= Vista: 0
    DEBUG_CODE(
      printf("dwFontSize = [%dx%d]\n", cfi.dwFontSize.X, cfi.dwFontSize.Y);
    )
    // So instead, to work in both XP and Vista+, query the actual size using GetConsoleFontSize()
    coord = GetConsoleFontSize(hConsole, cfi.nFont);
    printf("Font size: %d x %d\n", coord.X, coord.Y);

    printf("Font size #%d\n", cfi.nFont); // Index in list of sizes returned by GetNumberOfConsoleFonts().
  }

  // Now enumerate the available size, using the undocumented functions fount at
  // http://blogs.microsoft.co.il/pavely/2009/07/23/changing-console-fonts/
  pGetNumberOfConsoleFonts = (PGETNUMBEROFCONSOLEFONTS)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNumberOfConsoleFonts");
  if (pGetNumberOfConsoleFonts) nFonts = pGetNumberOfConsoleFonts();
  pGetConsoleFontInfo = (PGETCONSOLEFONTINFO)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetConsoleFontInfo");
  if (pGetConsoleFontInfo) {
    if (nFonts > 100) nFonts = 100; // Avoid table overflow
    pGetConsoleFontInfo(hConsole, FALSE, nFonts, fonts);
    if (nFonts) {
      printf("Available sizes:\n");
      for (i=0; (DWORD)i<nFonts; i++) {
	printf("Font size #%d: [%dx%d]\n", i, fonts[i].dim.X, fonts[i].dim.Y);
	if (fonts[i].index != (unsigned)i) printf("Warning: Design error. Indexes don't match: (i=%d) != (index=%d)\n", i, fonts[i].index);
      }
    }
    DEBUG_CODE( // Test if the table contains something, even if the count says 0 in Windows 10
      if (!nFonts) {
	pGetConsoleFontInfo(hConsole, FALSE, 100, fonts);
	for (i=0; i<100; i++) if (fonts[i].dim.X) {
	  printf("Font size #%d: Index %d [%dx%d]\n", i, fonts[i].index, fonts[i].dim.X, fonts[i].dim.Y);
	}
      }
    )
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
|   Return value:   The number of fonts found                                 |
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

