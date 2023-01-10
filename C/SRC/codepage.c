/*****************************************************************************\
*		    							      *
*   Filename	    codepage.c						      *
*		    							      *
*   Description     Get information about code pages on this system	      *
*		    							      *
*   Notes	    Common code page numbers:                                 *
*		    437   OEM codepage. Default MS-DOS & cmd.exe USA code page*
*		    1252  ANSI codepage. Default Windows USA code page.       *
*		    65001 UTF-8 codepage. Allows displaying any Unicode char. *
*                                                                             *
*		    Code pages other that 437 require using a TrueType font.  *
*		    The cmd.exe raster font only supports code page 437.      *
*		    							      *
*		    Code pages are listed in the registry:		      *
*		    HKLM:\SYSTEM\CurrentControlSet\Control\Nls\CodePage       *
*		    To get information about a code page, use GetCPInfoEx().  *
*		    							      *
*		    IBM list of code pages "Coded Character Set Identifiers": *
*   http://www-01.ibm.com/software/globalization/ccsid/ccsid_registered.html  *
*		    IBM "Supported territory codes and code pages":	      *
*   https://www.ibm.com/support/knowledgecenter/en/SSEPGG_9.7.0/com.ibm.db2.luw.admin.nls.doc/doc/r0004565.html
*		    							      *
*		    IANA list of character sets, w. links to their definitions:
*   https://www.iana.org/assignments/character-sets/character-sets.xhtml      *
*		    							      *
*   History								      *
*    2014-02-27 JFL Created this program.                                     *
*    2014-03-20 JFL Updated help.                                             *
*    2017-03-05 JFL By default, display the system and console code pages.    *
*    2017-03-06 JFL Added a database of known codepage descriptions, but      *
*		    first query Windows using GetCPInfoEx().		      *
*    2017-03-07 JFL Display a character table, and an ASCII table for N=0.    *
*    2017-03-09 JFL Display the code page name & infos before its char. table.*
*		    Version 1.1.					      *
*    2017-03-16 JFL Display the default console code page (OEMCP). V 1.1.1.   *
*    2018-01-25 JFL Display console font information. V 1.2.		      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.2.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.2.2.      *
*    2021-05-21 JFL Fixed the output of C0 & C1 control codes in MS Terminal. *
*		    Corrected wrong comments about switching code pages.      *
*		    Output the C cedilla in my first name in the help.	      *
*		    Added option -l as an alias to -i to list installed CPs.  *
*		    Version 1.3.					      *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 1.3.1.		      *
*		    							      *
*         © Copyright 2017 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Get information about code pages on this system"
#define PROGRAM_NAME    "codepage"
#define PROGRAM_VERSION "1.3.1"
#define PROGRAM_DATE    "2022-10-19"

/* Do NOT use _UTF8_SOURCE with MsvcLibX, as we want to test 8-bit code pages output */

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>         /* For _setmode() */
#include <fcntl.h>      /* For _setmode() */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

/************************ Win32-specific definitions *************************/

#if defined(_WIN32)	/* Automatically defined when targeting a Win32 applic. */

#include <windows.h>

/************************ MS-DOS-specific definitions ************************/

#elif defined(_MSDOS)	/* Automatically defined when targeting an MS-DOS app. */

#error "TO DO: Implement code page support for DOS"

/*********************************** Other ***********************************/

#else
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

typedef struct _codepage_t {
  int num;
  char *name;
  char *description;
} codepage_t;

/* 
  Code Page Identifiers information from:
  https://msdn.microsoft.com/en-us/library/windows/desktop/dd317756.aspx
*/

codepage_t codepages[] = {
  {37, "IBM037", "IBM EBCDIC US-Canada"},
  {437, "IBM437", "OEM United States"},
  {500, "IBM500", "IBM EBCDIC International"},
  {708, "ASMO-708", "Arabic (ASMO 708)"},
  {709, "", "Arabic (ASMO-449+, BCON V4)"},
  {710, "", "Arabic - Transparent Arabic"},
  {720, "DOS-720", "Arabic (Transparent ASMO); Arabic (DOS)"},
  {737, "ibm737", "OEM Greek (formerly 437G); Greek (DOS)"},
  {775, "ibm775", "OEM Baltic; Baltic (DOS)"},
  {850, "ibm850", "OEM Multilingual Latin 1; Western European (DOS)"},
  {852, "ibm852", "OEM Latin 2; Central European (DOS)"},
  {855, "IBM855", "OEM Cyrillic (primarily Russian)"},
  {857, "ibm857", "OEM Turkish; Turkish (DOS)"},
  {858, "IBM00858", "OEM Multilingual Latin 1 + Euro symbol"},
  {860, "IBM860", "OEM Portuguese; Portuguese (DOS)"},
  {861, "ibm861", "OEM Icelandic; Icelandic (DOS)"},
  {862, "DOS-862", "OEM Hebrew; Hebrew (DOS)"},
  {863, "IBM863", "OEM French Canadian; French Canadian (DOS)"},
  {864, "IBM864", "OEM Arabic; Arabic (864)"},
  {865, "IBM865", "OEM Nordic; Nordic (DOS)"},
  {866, "cp866", "OEM Russian; Cyrillic (DOS)"},
  {869, "ibm869", "OEM Modern Greek; Greek, Modern (DOS)"},
  {870, "IBM870", "IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2"},
  {874, "windows-874", "ANSI/OEM Thai (ISO 8859-11); Thai (Windows)"},
  {875, "cp875", "IBM EBCDIC Greek Modern"},
  {932, "shift_jis", "ANSI/OEM Japanese; Japanese (Shift-JIS)"},
  {936, "gb2312", "ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)"},
  {949, "ks_c_5601-1987", "ANSI/OEM Korean (Unified Hangul Code)"},
  {950, "big5", "ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)"},
  {1026, "IBM1026", "IBM EBCDIC Turkish (Latin 5)"},
  {1047, "IBM01047", "IBM EBCDIC Latin 1/Open System"},
  {1140, "IBM01140", "IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)"},
  {1141, "IBM01141", "IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)"},
  {1142, "IBM01142", "IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)"},
  {1143, "IBM01143", "IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)"},
  {1144, "IBM01144", "IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)"},
  {1145, "IBM01145", "IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)"},
  {1146, "IBM01146", "IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)"},
  {1147, "IBM01147", "IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)"},
  {1148, "IBM01148", "IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)"},
  {1149, "IBM01149", "IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)"},
  {1200, "utf-16", "Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications"},
  {1201, "unicodeFFFE", "Unicode UTF-16, big endian byte order; available only to managed applications"},
  {1250, "windows-1250", "ANSI Central European; Central European (Windows)"},
  {1251, "windows-1251", "ANSI Cyrillic; Cyrillic (Windows)"},
  {1252, "windows-1252", "ANSI Latin 1; Western European (Windows)"},
  {1253, "windows-1253", "ANSI Greek; Greek (Windows)"},
  {1254, "windows-1254", "ANSI Turkish; Turkish (Windows)"},
  {1255, "windows-1255", "ANSI Hebrew; Hebrew (Windows)"},
  {1256, "windows-1256", "ANSI Arabic; Arabic (Windows)"},
  {1257, "windows-1257", "ANSI Baltic; Baltic (Windows)"},
  {1258, "windows-1258", "ANSI/OEM Vietnamese; Vietnamese (Windows)"},
  {1361, "Johab", "Korean (Johab)"},
  {10000, "macintosh", "MAC Roman; Western European (Mac)"},
  {10001, "x-mac-japanese", "Japanese (Mac)"},
  {10002, "x-mac-chinesetrad", "MAC Traditional Chinese (Big5); Chinese Traditional (Mac)"},
  {10003, "x-mac-korean", "Korean (Mac)"},
  {10004, "x-mac-arabic", "Arabic (Mac)"},
  {10005, "x-mac-hebrew", "Hebrew (Mac)"},
  {10006, "x-mac-greek", "Greek (Mac)"},
  {10007, "x-mac-cyrillic", "Cyrillic (Mac)"},
  {10008, "x-mac-chinesesimp", "MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)"},
  {10010, "x-mac-romanian", "Romanian (Mac)"},
  {10017, "x-mac-ukrainian", "Ukrainian (Mac)"},
  {10021, "x-mac-thai", "Thai (Mac)"},
  {10029, "x-mac-ce", "MAC Latin 2; Central European (Mac)"},
  {10079, "x-mac-icelandic", "Icelandic (Mac)"},
  {10081, "x-mac-turkish", "Turkish (Mac)"},
  {10082, "x-mac-croatian", "Croatian (Mac)"},
  {12000, "utf-32", "Unicode UTF-32, little endian byte order; available only to managed applications"},
  {12001, "utf-32BE", "Unicode UTF-32, big endian byte order; available only to managed applications"},
  {20000, "x-Chinese_CNS", "CNS Taiwan; Chinese Traditional (CNS)"},
  {20001, "x-cp20001", "TCA Taiwan"},
  {20002, "x_Chinese-Eten", "Eten Taiwan; Chinese Traditional (Eten)"},
  {20003, "x-cp20003", "IBM5550 Taiwan"},
  {20004, "x-cp20004", "TeleText Taiwan"},
  {20005, "x-cp20005", "Wang Taiwan"},
  {20105, "x-IA5", "IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)"},
  {20106, "x-IA5-German", "IA5 German (7-bit)"},
  {20107, "x-IA5-Swedish", "IA5 Swedish (7-bit)"},
  {20108, "x-IA5-Norwegian", "IA5 Norwegian (7-bit)"},
  {20127, "us-ascii", "US-ASCII (7-bit)"},
  {20261, "x-cp20261", "T.61"},
  {20269, "x-cp20269", "ISO 6937 Non-Spacing Accent"},
  {20273, "IBM273", "IBM EBCDIC Germany"},
  {20277, "IBM277", "IBM EBCDIC Denmark-Norway"},
  {20278, "IBM278", "IBM EBCDIC Finland-Sweden"},
  {20280, "IBM280", "IBM EBCDIC Italy"},
  {20284, "IBM284", "IBM EBCDIC Latin America-Spain"},
  {20285, "IBM285", "IBM EBCDIC United Kingdom"},
  {20290, "IBM290", "IBM EBCDIC Japanese Katakana Extended"},
  {20297, "IBM297", "IBM EBCDIC France"},
  {20420, "IBM420", "IBM EBCDIC Arabic"},
  {20423, "IBM423", "IBM EBCDIC Greek"},
  {20424, "IBM424", "IBM EBCDIC Hebrew"},
  {20833, "x-EBCDIC-KoreanExtended", "IBM EBCDIC Korean Extended"},
  {20838, "IBM-Thai", "IBM EBCDIC Thai"},
  {20866, "koi8-r", "Russian (KOI8-R); Cyrillic (KOI8-R)"},
  {20871, "IBM871", "IBM EBCDIC Icelandic"},
  {20880, "IBM880", "IBM EBCDIC Cyrillic Russian"},
  {20905, "IBM905", "IBM EBCDIC Turkish"},
  {20924, "IBM00924", "IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)"},
  {20932, "EUC-JP", "Japanese (JIS 0208-1990 and 0212-1990)"},
  {20936, "x-cp20936", "Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)"},
  {20949, "x-cp20949", "Korean Wansung"},
  {21025, "cp1025", "IBM EBCDIC Cyrillic Serbian-Bulgarian"},
  {21027, "", "(deprecated)"},
  {21866, "koi8-u", "Ukrainian (KOI8-U); Cyrillic (KOI8-U)"},
  {28591, "iso-8859-1", "ISO 8859-1 Latin 1; Western European (ISO)"},
  {28592, "iso-8859-2", "ISO 8859-2 Central European; Central European (ISO)"},
  {28593, "iso-8859-3", "ISO 8859-3 Latin 3"},
  {28594, "iso-8859-4", "ISO 8859-4 Baltic"},
  {28595, "iso-8859-5", "ISO 8859-5 Cyrillic"},
  {28596, "iso-8859-6", "ISO 8859-6 Arabic"},
  {28597, "iso-8859-7", "ISO 8859-7 Greek"},
  {28598, "iso-8859-8", "ISO 8859-8 Hebrew; Hebrew (ISO-Visual)"},
  {28599, "iso-8859-9", "ISO 8859-9 Turkish"},
  {28603, "iso-8859-13", "ISO 8859-13 Estonian"},
  {28605, "iso-8859-15", "ISO 8859-15 Latin 9"},
  {29001, "x-Europa", "Europa 3"},
  {38598, "iso-8859-8-i", "ISO 8859-8 Hebrew; Hebrew (ISO-Logical)"},
  {50220, "iso-2022-jp", "ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)"},
  {50221, "csISO2022JP", "ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)"},
  {50222, "iso-2022-jp", "ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)"},
  {50225, "iso-2022-kr", "ISO 2022 Korean"},
  {50227, "x-cp50227", "ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)"},
  {50229, "", "ISO 2022 Traditional Chinese"},
  {50930, "", "EBCDIC Japanese (Katakana) Extended"},
  {50931, "", "EBCDIC US-Canada and Japanese"},
  {50933, "", "EBCDIC Korean Extended and Korean"},
  {50935, "", "EBCDIC Simplified Chinese Extended and Simplified Chinese"},
  {50936, "", "EBCDIC Simplified Chinese"},
  {50937, "", "EBCDIC US-Canada and Traditional Chinese"},
  {50939, "", "EBCDIC Japanese (Latin) Extended and Japanese"},
  {51932, "euc-jp", "EUC Japanese"},
  {51936, "EUC-CN", "EUC Simplified Chinese; Chinese Simplified (EUC)"},
  {51949, "euc-kr", "EUC Korean"},
  {51950, "", "EUC Traditional Chinese"},
  {52936, "hz-gb-2312", "HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)"},
  {54936, "GB18030", "Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)"},
  {57002, "x-iscii-de", "ISCII Devanagari"},
  {57003, "x-iscii-be", "ISCII Bangla"},
  {57004, "x-iscii-ta", "ISCII Tamil"},
  {57005, "x-iscii-te", "ISCII Telugu"},
  {57006, "x-iscii-as", "ISCII Assamese"},
  {57007, "x-iscii-or", "ISCII Odia"},
  {57008, "x-iscii-ka", "ISCII Kannada"},
  {57009, "x-iscii-ma", "ISCII Malayalam"},
  {57010, "x-iscii-gu", "ISCII Gujarati"},
  {57011, "x-iscii-pa", "ISCII Punjabi"},
  {65000, "utf-7", "Unicode (UTF-7)"},
  {65001, "utf-8", "Unicode (UTF-8)"},
};
#define NCODEPAGES (sizeof(codepages) / sizeof(codepage_t))

/* Get the actual name of a code page */
char *GetCPName(int iCP, LPCPINFOEX lpCpi) {
  int i;
  char *pszName = "";
  CPINFOEX cpi;
  if (!lpCpi) lpCpi = &cpi;
  if (GetCPInfoEx(iCP, 0, lpCpi)) { /* Most code pages have a good descrition in the CPINFOEX structure */
    /* (Including many that are not listed in the static list above) */
    pszName = lpCpi->CodePageName;
    /* Make a copy because we can't return the address of this one in the local stack frame */
    /* Skip the code page number copy at the beginning of the CPINFOEX name */
    while (strchr("0123456789", *pszName)) pszName++;
    while (isspace(*pszName)) pszName++;	/* Skip spaces after the number */
    if (*pszName == '(') pszName++;		/* Remove the leading '(' */
    pszName = strdup(pszName);
    i = (int)strlen(pszName) - 1;
    if (pszName[i] == ')') pszName[i] = '\0'; /* Remove the trailing ')' */
  }
  /* But some code pages have a description that's an empty string "" */
  if (!*pszName) { /* In this case, search it in the static list above */
    for (i=0; i<NCODEPAGES; i++) {
      if (codepages[i].num == iCP) {
	pszName = codepages[i].description;
	break;
      }
    }
  }
  return pszName;
}

/* Forward declarations */
void usage(void);
int CheckConsoleFont(void);

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

int nCP = 0;

/* Windows 10 has 140 code pages installed. IBM's list contains 787 entries */
/* With the generalization of UTF-8, it's unlikely that new code pages will be created. */
int iCPList[1024]; /* So assume that 1024 entries is more than enough */
#define CPLISTSIZE (sizeof(iCPList) / sizeof(iCPList[0]))

/* The code pages are enumerated in alphabetical order, not numeric order.
   So build an array of numbers, then sort it. */
BOOL CALLBACK EnumCodePagesProc(LPTSTR lpCodePageString) {
  int iCP = atoi(lpCodePageString);
  /* printf("EnumCodePagesProc(\"%s\");\n", lpCodePageString); */
  if (nCP == CPLISTSIZE) {
    fprintf(stderr, "Too many codes pages.\n");
    exit(1);
  }
  iCPList[nCP++] = iCP;
  /* printf("%s\t%s\n", lpCodePageString, GetCPName(iCP)); */
  return TRUE;
}

/* Numeric comparison routine for sorting the iCPList array */
int compareInts(const void *p1, const void *p2) {
  return *(int *)p1 - *(int *)p2;
}

int main(int argc, char *argv[]) {
  int i;
  int iVerbose = FALSE;
  DWORD dwFlags = 0;
  int iCP = 0;
  BOOL bDone;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      if (   streq(arg+1, "help")
	  || streq(arg+1, "-help")
	  || streq(arg+1, "h")
	  || streq(arg+1, "?")) {
	usage();
      }
      if (streq(arg+1, "c")) {	/* Change to CP N. NOT SHOWN IN HELP, because side effects unknown: CHCP does not see the change. */
	iCP = atoi(argv[++i]);
	bDone = SetConsoleOutputCP(iCP);
	if (!bDone) {
	  fprintf(stderr, "Error: Failed to change to Code Page %d\n", iCP);
	  exit(1);
	}
	exit(0);
      }
      if (   streq(arg+1, "i")	/* List code pages installed */
      	  || streq(arg+1, "l")) {
	dwFlags = CP_INSTALLED;
	continue;
      }
      if (streq(arg+1, "s")) {	/* List code pages supported */
	dwFlags = CP_SUPPORTED;
	continue;
      }
      if (streq(arg+1, "v")) {	/* Verbose infos */
	iVerbose = TRUE;
	continue;
      }
      if (streq(arg+1, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    /* If it's an argument */
    if (!iCP && strchr("0123456789", arg[0])) {
      iCP = atoi(arg);
      if (!iCP) iCP = 1; /* There's not code page 1, and we need a !0 value for the tests to fire */
      continue;
    }
    printf("Unexpected argument %s. Ignored.\n", arg);
    continue;
  }

  if (dwFlags) {	/* List code pages */
    /* Important:
	Changing code pages only works if the console uses a fixed-pitch Unicode font.
	For example: The Lucida Console TrueType font.
	Raster fonts do NOT support Unicode.
	Ref: http://support.microsoft.com/kb/99795
    */
    EnumSystemCodePages(EnumCodePagesProc, dwFlags);
    qsort(iCPList, nCP, sizeof(iCPList[0]), compareInts);
    printf("Name\tBytes\tDescription\n");
    printf("-------\t-------\t--------------------------------------------------------\n");
    for (i=0; i<nCP; i++) {
      CPINFOEX cpi;
      char *pszName;
      iCP = iCPList[i];
      pszName = GetCPName(iCP, &cpi);
      printf("%d\t%d\t%s\n", iCP, cpi.MaxCharSize, pszName);
    }
    printf("Total: %d code pages\n", nCP);
    /* Observation:
    	On Windows 10, both CP_INSTALLED and CP_SUPPORTED report 140 code pages.
    	On Windows 7 & Windows 8, both CP_INSTALLED and CP_SUPPORTED report 135 code pages.
    	On Windows XP, there are only 55 installed for 134 supported.
    	In all 4 OSs, code pages 65000 (UTF-7) and 65001 (UTF-8) are installed.
    */
  } else if (iCP) {	/* Temporarily switch to UTF-16 mode, and display the code page characters */
    CPINFOEX cpi = {0};
    int iCP0 = GetConsoleOutputCP();
    int j, k, b, kMin=8, kMax=16, nBlocks=1;
    char szInfos[1024] = "";
    int nInfos = 0;
    if (iVerbose) printf("Initial code page: %d\n", iCP0);
    if (iCP == 1) { /* Display the ASCII set, which is the same in any code page. */
      kMin=0; kMax=8;
      iCP = iCP0;
      printf("ASCII table:\n");
    } else {
      if (!GetCPInfoEx(iCP, 0, &cpi)) {
	fprintf(stderr, "Error: Unknown Code Page %d.\n", iCP);
	exit(1);
      }
      /* Check if MSBCS code set */
      if (cpi.MaxCharSize > 1) {
      	nInfos = sprintf(szInfos, "(MBCS, %d bytes max)", cpi.MaxCharSize);
      } else {
      	nInfos = sprintf(szInfos, "(SBCS)");
      }
      /* Check if the first half is ASCII-compatible */
      if (iCP != 1252) for (i=0x20; i<0x7F; i++) {
	int i2;
	long w;
	int n;
	w = i2 = 0;
	n = MultiByteToWideChar(iCP, 0, (LPCSTR)&i, 1, (LPWSTR)&w, 2);
	n = WideCharToMultiByte(1252, 0, (LPCWSTR)&w, n, (LPSTR)&i2, 2, NULL, NULL);
	if (i != i2) {
	  nInfos += sprintf(szInfos+nInfos, " Incompatible with ASCII");
	  kMin=0; kMax=8; nBlocks = 2; /* So display both the ASCII block and the extended block */
	  break;
	}
      }
      if (nBlocks == 1) {
      	nInfos += sprintf(szInfos+nInfos, " ASCII-compatible");
      }
      /* Use GetCPName() and not cpi.CodePageName, as the latter has a missing names */
      printf("Code page %d: %s %s\n", iCP, GetCPName(iCP, NULL), szInfos);
      if (cpi.MaxCharSize > 1) {
      	printf("Tentative information: (N)=Lead byte of N bytes (1)=Tail byte (X)=Invalid\n");
      }
    }
    if (iVerbose) printf("Switching to 16-bit mode.\n");
    /* Make sure everything gets to the console before switching the mode */
    fflush(stdout); fflush(stderr);
    _setmode(fileno(stdout), _O_U16TEXT);
    /* Display a table of characters specific to that code page */
    for (b=0; b<nBlocks; b++, kMin+=8, kMax+=8) {
      if (b > 0) wprintf(L"\n");
      for (j=0; j<16; j++) {
	for (k=kMin; k<kMax; k++) {
	  int c, n, l;
	  WCHAR wstr[3] = {0};
	  wchar_t c1 = L' ';
	  wchar_t c2 = L' ';
	  if (!(k&3)) wprintf(L"  ");
	  c = n = (16*k)+j;
	  /* Check if this is a lead byte for an MBCS code page */
	  if (iCP == 65001) {	/* For UTF-8, the cpi.LeadByte table is not defined, but the UTF-8 spec is clear */
	    if (n & 0x80) {	/* Yes, this is a lead or tail byte */
	      c1 = L'(';
	      c2 = L')';
	      if        ((n & 0xC0U) == 0x80U) {	/* This is a tail byte */
		wstr[0] = L'1';
	      } else if ((n & 0xE0U) == 0xC0U) {	/* This is a lead byte of 2 bytes */
		wstr[0] = L'2';
	      } else if ((n & 0xF0U) == 0xE0U) {	/* This is a lead byte of 3 bytes */
		wstr[0] = L'3';
	      } else if ((n & 0xF8U) == 0xF0U) {	/* This is a lead byte of 4 bytes */
		wstr[0] = L'4';
	      } else {					/* Invalid byte */
		wstr[0] = L'X';
	      }
	    }
	  } else { /* Use the cpi.LeadByte table. We don't know how many bytes follow. */
	    for (l=0; (l<MAX_LEADBYTES) && cpi.LeadByte[l] && cpi.LeadByte[l+1]; l+=2) {
	      if ((cpi.LeadByte[l] <= c) && (cpi.LeadByte[l+1] <= c)) {	/* Yes, this is a lead byte */
		c1 = L'(';
		wstr[0] = L'2'; /* For lack of better info, assume a 2-byte sequence */ 
		c2 = L')';
		break;
	      }
	    }
	  }
	  if (!wstr[0]) {
	    /* XP and earlier silently drop invalid characters if MB_ERR_INVALID_CHARS is not used */
	    if (!MultiByteToWideChar(iCP, MB_ERR_INVALID_CHARS, (LPCSTR)&c, 1, wstr, 3)) wstr[0] = (wchar_t)0xFFFD;
	  }
	  if (wstr[0] < L' ') wstr[0] += (wchar_t)0x2400; /* C0 control char. Use the corresponding control picture instead */
	  if (wstr[0] == L'\x7F') wstr[0] = (wchar_t)0x2421; /* Delete char. Use the DEL control picture instead */
	  if ((wstr[0] >= L'\x80') && (wstr[0] < L'\xA0')) wstr[0] = (wchar_t)0xFFFD; /* C1 control char. Use the undefined character instead */
	  wprintf(L" %02X%c%s%c", n, c1, wstr, c2);
	}
	wprintf(L"\n");
      }
    }
    if (iVerbose) wprintf(L"Returning to 8-bit mode.\n");
    /* Make sure everything gets to the console before switching back the mode */
    fflush(stdout); fflush(stderr);
    _setmode(fileno(stdout), _O_U16TEXT);
  } else {		/* Display the system and console code pages */
    int iCCP = GetConsoleOutputCP();
    int iOCP = GetOEMCP();
    int iACP = GetACP();
    printf("Current console code page: %d = %s\n", iCCP, GetCPName(iCCP, NULL));
    printf("Default console code page: %d = %s\n", iOCP, GetCPName(iOCP, NULL));
    printf("System code page: %d = %s\n", iACP, GetCPName(iACP, NULL));
    // Now check the console font, to allow detecting incompatibility cases.
    // (As raster fonts usually only support the default console code page)
    CheckConsoleFont();
  }

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

void usage(void) {
  wchar_t wcCCedilla = (wchar_t)0xE7; // 'ç';
  char szCCedilla[4] = {0};
  int iCCP = GetConsoleOutputCP();
  if (!WideCharToMultiByte(iCCP, 0, &wcCCedilla, 1, szCCedilla, 3, NULL, NULL)) szCCedilla[0] = 'c';
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
  codepage [SWITCHES] [CODEPAGE]\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n\
  -l          List code pages installed. (In XP, {installed} < {supported})\n\
  -s          List code pages supported\n\
  -v          Display verbose information\n\
  -V          Display this program version and exit\n\
\n\
Codepage: N = One of the installed code pages:\n\
Display a table with the specific characters for that code page.\n\
(Visible only if the console font contains the requested characters.)\n\
If N is 0: List ASCII characters.\n\
Default: Show the system and console code pages.\n\
\n\
Sample code page numbers:\n\
  437         OEM US codepage = MS-DOS & cmd.exe default USA code page\n\
  1252        ANSI Latin 1 codepage = Windows USA & west european code page\n\
  65001       UTF-8 codepage. Allows displaying any Unicode character\n\
\n\
Note that code pages other than 437 require cmd.exe using a TrueType font.\n\
The cmd.exe raster font only supports code page 437.\n\
\n\
Author: Jean-Fran%sois Larvoire - jf.larvoire@free.fr\n\
Sources and updates: https://github.com/JFLarvoire/SysToolsLib\n"
, szCCedilla);

  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CheckConsoleFont					      |
|                                                                             |
|   Description:    Check the console font compatibility with code pages      |
|                                                                             |
|   Parameters:                                                               |
|                                                                             |
|   Return value:   Nothing                                                   |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2018-01-24 JFL Created this routine.                   		      |
*                                                                             *
\*---------------------------------------------------------------------------*/

#if _MSC_VER <= 1400
typedef struct _CONSOLE_FONT_INFOEX {
  ULONG cbSize;
  DWORD nFont;
  COORD dwFontSize;
  UINT  FontFamily;
  UINT  FontWeight;
  WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#endif

// GetCurrentConsoleFontEx does not exist in Windows XP => Get its address dynamically.
typedef BOOL (WINAPI *PGETCURRENTCONSOLEFONTEX)(HANDLE h, BOOL b, PCONSOLE_FONT_INFOEX p);

int CheckConsoleFont() {
  // HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // This returns a wrong handle if stdout is redirected. So instead, use:
  HANDLE hConsole;
  PGETCURRENTCONSOLEFONTEX pGetCurrentConsoleFontEx;
  CONSOLE_FONT_INFOEX cfix = {0};
  char *pszRasterFormat = "%s: Raster fonts may only display correctly code page %d characters!\n";

  hConsole = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
  if (hConsole == INVALID_HANDLE_VALUE) {
    printf(pszRasterFormat, "Note", GetOEMCP());
    return 0;
  }

  // GetCurrentConsoleFontEx does not exist in Windows XP => Get its address dynamically.
  pGetCurrentConsoleFontEx = (PGETCURRENTCONSOLEFONTEX)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetCurrentConsoleFontEx"); 
  cfix.cbSize = sizeof(cfix);
  if (pGetCurrentConsoleFontEx && pGetCurrentConsoleFontEx(hConsole, FALSE, &cfix)) {
    char *pszType;
    char szFaceName[4*LF_FACESIZE]; // Worst case converting cfix.FaceName to UTF8
    int isRaster = FALSE;

// See https://msdn.microsoft.com/en-us/library/windows/desktop/dd145132(v=vs.85).aspx
#define TMPF_POSTSCRIPT (TMPF_DEVICE | TMPF_VECTOR | TMPF_FIXED_PITCH)

    if (cfix.FontFamily & TMPF_TRUETYPE) {
      pszType = "TrueType";
    } else if (cfix.FontFamily & TMPF_VECTOR) {
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
    WideCharToMultiByte(GetConsoleOutputCP(), 0, cfix.FaceName, lstrlenW(cfix.FaceName)+1, szFaceName, sizeof(szFaceName), NULL, NULL);
    printf("Console font: [%s] %s\n", pszType, szFaceName); // Some PCs return an empty name. Others return garbage, cleared above.
    if (isRaster) printf(pszRasterFormat, "Warning", GetOEMCP());
  } else { // This is XP, and we can't tell what font type the console is using
                  printf(pszRasterFormat, "Note", GetOEMCP());
  }
  CloseHandle(hConsole);
  return 0;
}
