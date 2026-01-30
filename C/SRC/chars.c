/*****************************************************************************\
*									      *
*  File name:	    chars.c						      *
*									      *
*  Description:     Display all 8-bit characters in hexadecimal		      *
*									      *
*  Notes:	    The primary goal is to view what characters can actually  *
*		    be displayed on the current console or terminal.	      *
*		    This is particularly useful in cases where the console or *
*		    terminal can only display an 8-bits character set.	      *
*		    Which is the default case in most european and american   *
*		    versions of DOS and Windows.			      *
*		    This is why we must not use MsvcLibX's support for UTF-8. *
*		    							      *
*		    A secondary goal is to view Unicode characters on consoles*
*		    or terminals that support it.			      *
*		    In this case, in Windows, the console code page is 	      *
*		    switched to code page 65001, supporting UTF8.	      *
*		    							      *
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
*    2023-02-06 JFL Added support for multiple ranges, and removed option -u. *
*		    Moved operations to the new DetectAnsi() & PrintRange().  *
*    2023-02-09 JFL More consistent management of alternate code pages.       *
*		    Corrected and improved the help screen.		      *
*		    Detect the current character set in DOS.		      *
*    2023-02-10 JFL Added option -q to display the char, but not its code.    *
*    2023-02-13 JFL Added the list of Unicode 15 character blocks.	      *
*		    Added option -b to list the known Unicode blocks.         *
*    2023-02-16 JFL Allow displaying a block by name or partial name.	      *
*    2023-02-17 JFL Display the Unicode block name for tables, and in verbose *
*		    mode for individual characters.			      *
*		    Version 2.0.					      *
*    2023-04-17 JFL Moved the LODOS-specific realloc() hack to BiosLib. v2.0.1.
*    2023-10-09 JFL Added support for UTF-8 bytes codes on the command line.  *
*		    Added support for doubly encoded UTF-8 chars on cmd. line.*
*		    In Windows, added option -8 as a synonym for -c 65001.    *
*		    In Windows, display both the current code page character  *
*		    code and the unicode code point.			      *
*    2023-10-15 JFL In Windows, display the Windows code page instead of the  *
*		    Unicode block name for non Unicode character ranges.      *
*		    Fixed character ranges which output � for all characters. *
*		    Version 2.1.					      *
*    2023-12-20 JFL Added the ability to decompose accented Unicode characters.
*		    Version 2.2.					      *
*    2025-06-11 JFL Fixed a missing line when run from the top of the console.*
*		    Version 2.2.1.					      *
*    2026-01-29 JFL Make sure _not_ to use any MsvcLibX modified output fnct. *
*		    Fixed the output of double-width Unicode characters.      *
*    2026-01-30 JFL Bugfix: Output the full UTF-8 sequence in code page 65001.*
*		    Version 2.3.					      *
*		    							      *
*       © Copyright 2016-2017 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Show characters and their codes"
#define PROGRAM_NAME    "chars"
#define PROGRAM_VERSION "2.3"
#define PROGRAM_DATE    "2026-01-30"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX	/* Make it easier to test for all Unix-alike OSs */
#define _GNU_SOURCE /* But don't define it for DOS/Windows, because we do not want to use MsvcLibX code page support */
#endif

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

#include <dos.h>
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

#ifdef _UNIX	/* Defined above if defined(__unix__) || defined(__MACH__) */

#include <locale.h>
#include <ctype.h>
#include <errno.h>

#define EOL "\n"

#define SUPPORTS_UTF8 TRUE
#define EXTRA_CHARS_IN_CONTROL_CODES FALSE
#define ANSI_IS_OPTIONAL FALSE	/* Posix requires ANSI escape sequences support */

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#if defined(_MSDOS) || defined(_WIN32)
#define MICROSOFT_OS 1
#else
#define MICROSOFT_OS 0
#endif

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

#ifdef MSVCLIBX_VERSION /* If built using MsvcLibX */
/* Undefine all modified routines defined in MsvcLibX' stdio.h */
#undef vfprintf
#undef fprintf
#undef printf
#undef wprintf
#undef fputs
#undef fputws
#undef fputc
#undef puts
#endif /* MSVCLIBX_VERSION */

/* SysToolsLib include files */
#include "debugm.h"     /* SysToolsLib debug macros. Include first. */
#include "console.h"	/* SysLib console management. Ex: GetCursorPosition */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

#define stristr strcasestr	/* A shorter alias to the standard routine */

/* Chars Flags, passed to and from the subroutines */
#define CF_VERBOSE  0x01	/* Verbose mode */
#define CF_QUIET    0x02	/* Quiet mode */
#define CF_ALL	    0x04	/* Output all characters, even control characters */
#define CF_UNICODE  0x08	/* The user passed a Unicode code point */
#define CF_TTY 	    0x10	/* The output goes to a terminal */
#define CF_ANSI	    0x20	/* The terminal processes ANSI escape sequences */
#define CF_UTF8	    0x40	/* The terminal can display UTF-8 characters */

typedef struct _CHAR_DEF {
  int iCode;
  int iFlags;
#ifdef _WIN32
  int iArg;
#endif /* defined(_WIN32) */
} CHAR_DEF;

typedef struct _RANGE_DEF {
  int iFirst;
  int iLast;			/* The last character, or -1 for a single one */
  int iFlags;
#ifdef _WIN32
  char *pszArgCP1;		/* If single char, the console Code Page argument */
  char *pszArgUTF8;		/* If single char, the UTF-8 Code Page argument */
#endif /* defined(_WIN32) */
} RANGE_DEF;

#if SUPPORTS_UTF8

typedef struct _UNICODE_BLOCK { /* Unicode block range and name definition */
  int iFirst;
  int iLast;
  char *pszName;
} UNICODE_BLOCK;

/* List of Unicode blocks, based on https://en.wikipedia.org/wiki/Unicode_block */
/* For updates, use the more easily usable list: https://www.unicode.org/Public/UNIDATA/Blocks.txt */
UNICODE_BLOCK unicodeBlock[] = {
  /* Plane 0: Basic Multilingual Plane */
  {0x0000, 0x007F, "Basic Latin (ASCII)"},
  {0x0080, 0x00FF, "Latin-1 Supplement"},
  {0x0100, 0x017F, "Latin Extended-A"},
  {0x0180, 0x024F, "Latin Extended-B"},
  {0x0250, 0x02AF, "IPA Extensions"},
  {0x02B0, 0x02FF, "Spacing Modifier Letters"},
  {0x0300, 0x036F, "Combining Diacritical Marks"},
  {0x0370, 0x03FF, "Greek and Coptic"},
  {0x0400, 0x04FF, "Cyrillic"},
  {0x0500, 0x052F, "Cyrillic Supplement"},
  {0x0530, 0x058F, "Armenian"},
  {0x0590, 0x05FF, "Hebrew"},
  {0x0600, 0x06FF, "Arabic"},
  {0x0700, 0x074F, "Syriac"},
  {0x0750, 0x077F, "Arabic Supplement"},
  {0x0780, 0x07BF, "Thaana"},
  {0x07C0, 0x07FF, "NKo"},
  {0x0800, 0x083F, "Samaritan"},
  {0x0840, 0x085F, "Mandaic"},
  {0x0860, 0x086F, "Syriac Supplement"},
  {0x0870, 0x089F, "Arabic Extended-B"},
  {0x08A0, 0x08FF, "Arabic Extended-A"},
  {0x0900, 0x097F, "Devanagari"},
  {0x0980, 0x09FF, "Bengali"},
  {0x0A00, 0x0A7F, "Gurmukhi"},
  {0x0A80, 0x0AFF, "Gujarati"},
  {0x0B00, 0x0B7F, "Oriya"},
  {0x0B80, 0x0BFF, "Tamil"},
  {0x0C00, 0x0C7F, "Telugu"},
  {0x0C80, 0x0CFF, "Kannada"},
  {0x0D00, 0x0D7F, "Malayalam"},
  {0x0D80, 0x0DFF, "Sinhala"},
  {0x0E00, 0x0E7F, "Thai"},
  {0x0E80, 0x0EFF, "Lao"},
  {0x0F00, 0x0FFF, "Tibetan"},
  {0x1000, 0x109F, "Myanmar"},
  {0x10A0, 0x10FF, "Georgian"},
  {0x1100, 0x11FF, "Hangul Jamo"},
  {0x1200, 0x137F, "Ethiopic"},
  {0x1380, 0x139F, "Ethiopic Supplement"},
  {0x13A0, 0x13FF, "Cherokee"},
  {0x1400, 0x167F, "Unified Canadian Aboriginal Syllabics"},
  {0x1680, 0x169F, "Ogham"},
  {0x16A0, 0x16FF, "Runic"},
  {0x1700, 0x171F, "Tagalog"},
  {0x1720, 0x173F, "Hanunoo"},
  {0x1740, 0x175F, "Buhid"},
  {0x1760, 0x177F, "Tagbanwa"},
  {0x1780, 0x17FF, "Khmer"},
  {0x1800, 0x18AF, "Mongolian"},
  {0x18B0, 0x18FF, "Unified Canadian Aboriginal Syllabics Extended"},
  {0x1900, 0x194F, "Limbu"},
  {0x1950, 0x197F, "Tai Le"},
  {0x1980, 0x19DF, "New Tai Lue"},
  {0x19E0, 0x19FF, "Khmer Symbols"},
  {0x1A00, 0x1A1F, "Buginese"},
  {0x1A20, 0x1AAF, "Tai Tham"},
  {0x1AB0, 0x1AFF, "Combining Diacritical Marks Extended"},
  {0x1B00, 0x1B7F, "Balinese"},
  {0x1B80, 0x1BBF, "Sundanese"},
  {0x1BC0, 0x1BFF, "Batak"},
  {0x1C00, 0x1C4F, "Lepcha"},
  {0x1C50, 0x1C7F, "Ol Chiki"},
  {0x1C80, 0x1C8F, "Cyrillic Extended-C"},
  {0x1C90, 0x1CBF, "Georgian Extended"},
  {0x1CC0, 0x1CCF, "Sundanese Supplement"},
  {0x1CD0, 0x1CFF, "Vedic Extensions"},
  {0x1D00, 0x1D7F, "Phonetic Extensions"},
  {0x1D80, 0x1DBF, "Phonetic Extensions Supplement"},
  {0x1DC0, 0x1DFF, "Combining Diacritical Marks Supplement"},
  {0x1E00, 0x1EFF, "Latin Extended Additional"},
  {0x1F00, 0x1FFF, "Greek Extended"},
  {0x2000, 0x206F, "General Punctuation"},
  {0x2070, 0x209F, "Superscripts and Subscripts"},
  {0x20A0, 0x20CF, "Currency Symbols"},
  {0x20D0, 0x20FF, "Combining Marks for Symbols"},
  {0x2100, 0x214F, "Letterlike Symbols"},
  {0x2150, 0x218F, "Number Forms"},
  {0x2190, 0x21FF, "Arrows"},
  {0x2200, 0x22FF, "Mathematical Operators"},
  {0x2300, 0x23FF, "Miscellaneous Technical"},
  {0x2400, 0x243F, "Control Pictures"},
  {0x2440, 0x245F, "Optical Character Recognition"},
  {0x2460, 0x24FF, "Enclosed Alphanumerics"},
  {0x2500, 0x257F, "Box Drawing"},
  {0x2580, 0x259F, "Block Elements"},
  {0x25A0, 0x25FF, "Geometric Shapes"},
  {0x2600, 0x26FF, "Miscellaneous Symbols"},
  {0x2700, 0x27BF, "Dingbats"},
  {0x27C0, 0x27EF, "Miscellaneous Mathematical Symbols-A"},
  {0x27F0, 0x27FF, "Supplemental Arrows-A"},
  {0x2800, 0x28FF, "Braille Patterns"},
  {0x2900, 0x297F, "Supplemental Arrows-B"},
  {0x2980, 0x29FF, "Miscellaneous Mathematical Symbols-B"},
  {0x2A00, 0x2AFF, "Supplemental Mathematical Operators"},
  {0x2B00, 0x2BFF, "Miscellaneous Symbols and Arrows"},
  {0x2C00, 0x2C5F, "Glagolitic"},
  {0x2C60, 0x2C7F, "Latin Extended-C"},
  {0x2C80, 0x2CFF, "Coptic"},
  {0x2D00, 0x2D2F, "Georgian Supplement"},
  {0x2D30, 0x2D7F, "Tifinagh"},
  {0x2D80, 0x2DDF, "Ethiopic Extended"},
  {0x2DE0, 0x2DFF, "Cyrillic Extended-A"},
  {0x2E00, 0x2E7F, "Supplemental Punctuation"},
  {0x2E80, 0x2EFF, "CJK Radicals Supplement"},
  {0x2F00, 0x2FDF, "Kangxi Radicals"},
/* 0x2FE0, 0x2FEF, "Last hole in the BMP" */
  {0x2FF0, 0x2FFF, "Ideographic Description Characters"},
  {0x3000, 0x303F, "CJK Symbols and Punctuation"},
  {0x3040, 0x309F, "Hiragana"},
  {0x30A0, 0x30FF, "Katakana"},
  {0x3100, 0x312F, "Bopomofo"},
  {0x3130, 0x318F, "Hangul Compatibility Jamo"},
  {0x3190, 0x319F, "Kanbun"},
  {0x31A0, 0x31BF, "Bopomofo Extended"},
  {0x31C0, 0x31EF, "CJK Strokes"},
  {0x31F0, 0x31FF, "Katakana Phonetic Extensions"},
  {0x3200, 0x32FF, "Enclosed CJK Letters and Months"},
  {0x3300, 0x33FF, "CJK Compatibility"},
  {0x3400, 0x4DBF, "CJK Unified Ideographs Extension A"},
  {0x4DC0, 0x4DFF, "Yijing Hexagram Symbols"},
  {0x4E00, 0x9FFF, "CJK Unified Ideographs"},
  {0xA000, 0xA48F, "Yi Syllables"},
  {0xA490, 0xA4CF, "Yi Radicals"},
  {0xA4D0, 0xA4FF, "Lisu"},
  {0xA500, 0xA63F, "Va"},
  {0xA640, 0xA69F, "Cyrillic Extended-B"},
  {0xA6A0, 0xA6FF, "Bamum"},
  {0xA700, 0xA71F, "Modifier Tone Letters"},
  {0xA720, 0xA7FF, "Latin Extended-D"},
  {0xA800, 0xA82F, "Syloti Nagri"},
  {0xA830, 0xA83F, "Common Indic Number Forms"},
  {0xA840, 0xA87F, "Phags-pa"},
  {0xA880, 0xA8DF, "Saurashtra"},
  {0xA8E0, 0xA8FF, "Devanagari Extended"},
  {0xA900, 0xA92F, "Kayah Li"},
  {0xA930, 0xA95F, "Rejang"},
  {0xA960, 0xA97F, "Hangul Jamo Extended-A"},
  {0xA980, 0xA9DF, "Javanese"},
  {0xA9E0, 0xA9FF, "Myanmar Extended-B"},
  {0xAA00, 0xAA5F, "Cham"},
  {0xAA60, 0xAA7F, "Myanmar Extended-A"},
  {0xAA80, 0xAADF, "Tai Viet"},
  {0xAAE0, 0xAAFF, "Meetei Mayek Extensions"},
  {0xAB00, 0xAB2F, "Ethiopic Extended-A"},
  {0xAB30, 0xAB6F, "Latin Extended-E"},
  {0xAB70, 0xABBF, "Cherokee Supplement"},
  {0xABC0, 0xABFF, "Meetei Mayek"},
  {0xAC00, 0xD7AF, "Hangul Syllables"},
  {0xD7B0, 0xD7FF, "Hangul Jamo Extended-B"},	
  {0xD800, 0xDB7F, "High Surrogates"},
  {0xDB80, 0xDBFF, "High Private Use Surrogates"},
  {0xDC00, 0xDFFF, "Low Surrogates"},
  {0xE000, 0xF8FF, "Private Use"},
  {0xF900, 0xFAFF, "CJK Compatibility Ideographs"},
  {0xFB00, 0xFB4F, "Alphabetic Presentation Forms"},
  {0xFB50, 0xFDFF, "Arabic Presentation Forms-A"},
  {0xFE00, 0xFE0F, "Variation Selectors"},
  {0xFE10, 0xFE1F, "Vertical Forms"},
  {0xFE20, 0xFE2F, "Combining Half Marks"},
  {0xFE30, 0xFE4F, "CJK Compatibility Forms"},
  {0xFE50, 0xFE6F, "Small Form Variants"},
  {0xFE70, 0xFEFE, "Arabic Presentation Forms-B"},
  {0xFEFF, 0xFEFF, "Specials"},
  {0xFF00, 0xFFEF, "Halfwidth and Fullwidth Forms"},
  {0xFFF0, 0xFFFF, "Specials"},

  /* Plane 1: Supplementary Multilingual Plane */
  {0x10000, 0x1007F, "Linear B Syllabary"},
  {0x10080, 0x100FF, "Linear B Ideograms"},
  {0x10100, 0x1013F, "Aegean Numbers"},
  {0x10140, 0x1018F, "Ancient Greek Numbers"},
  {0x10190, 0x101CF, "Ancient Symbols"},
  {0x101D0, 0x101FF, "Phaistos Disc"},
  {0x10280, 0x1029F, "Lycian"},
  {0x102A0, 0x102DF, "Carian"},
  {0x102E0, 0x102FF, "Coptic Epact Numbers"},
  {0x10300, 0x1032F, "Old Italic"},
  {0x10330, 0x1034F, "Gothic"},
  {0x10350, 0x1037F, "Old Permic"},
  {0x10380, 0x1039F, "Ugaritic"},
  {0x103A0, 0x103DF, "Old Persian"},
  {0x10400, 0x1044F, "Deseret"},
  {0x10450, 0x1047F, "Shavian"},
  {0x10480, 0x104AF, "Osmanya"},
  {0x104B0, 0x104FF, "Osage"},
  {0x10500, 0x1052F, "Elbasan"},
  {0x10530, 0x1056F, "Caucasian Albanian"},
  {0x10570, 0x105BF, "Vithkuqi"},
  {0x10600, 0x1077F, "Linear A"},
  {0x10780, 0x107BF, "Latin Extended-F"},
  {0x10800, 0x1083F, "Cypriot Syllabary"},
  {0x10840, 0x1085F, "Imperial Aramaic"},
  {0x10860, 0x1087F, "Palmyrene"},
  {0x10880, 0x108AF, "Nabataean"},
  {0x108E0, 0x108FF, "Hatran"},
  {0x10900, 0x1091F, "Phoenician"},
  {0x10920, 0x1093F, "Lydian"},
  {0x10980, 0x1099F, "Meroitic Hieroglyphs"},
  {0x109A0, 0x109FF, "Meroitic Cursive"},
  {0x10A00, 0x10A5F, "Kharoshthi"},
  {0x10A60, 0x10A7F, "Old South Arabian"},
  {0x10A80, 0x10A9F, "Old North Arabian"},
  {0x10AC0, 0x10AFF, "Manichaean"},
  {0x10B00, 0x10B3F, "Avestan"},
  {0x10B40, 0x10B5F, "Inscriptional Parthian"},
  {0x10B60, 0x10B7F, "Inscriptional Pahlavi"},
  {0x10B80, 0x10BAF, "Psalter Pahlavi"},
  {0x10C00, 0x10C4F, "Old Turkic"},
  {0x10C80, 0x10CFF, "Old Hungarian"},
  {0x10D00, 0x10D3F, "Hanifi Rohingya"},
  {0x10E60, 0x10E7F, "Rumi Numeral Symbols"},
  {0x10E80, 0x10EBF, "Yezidi"},
  {0x10EC0, 0x10EFF, "Arabic Extended-C"},
  {0x10F00, 0x10F2F, "Old Sogdian"},
  {0x10F30, 0x10F6F, "Sogdian"},
  {0x10F70, 0x10FAF, "Old Uyghur"},
  {0x10FB0, 0x10FDF, "Chorasmian"},
  {0x10FE0, 0x10FFF, "Elymaic"},
  {0x11000, 0x1107F, "Brahmi"},
  {0x11080, 0x110CF, "Kaithi"},
  {0x110D0, 0x110FF, "Sora Sompeng"},
  {0x11100, 0x1114F, "Chakma"},
  {0x11150, 0x1117F, "Mahajani"},
  {0x11180, 0x111DF, "Sharada"},
  {0x111E0, 0x111FF, "Sinhala Archaic Numbers"},
  {0x11200, 0x1124F, "Khojki"},
  {0x11280, 0x112AF, "Multani"},
  {0x112B0, 0x112FF, "Khudawadi"},
  {0x11300, 0x1137F, "Grantha"},
  {0x11400, 0x1147F, "Newa"},
  {0x11480, 0x114DF, "Tirhuta"},
  {0x11580, 0x115FF, "Siddham"},
  {0x11600, 0x1165F, "Modi"},
  {0x11660, 0x1167F, "Mongolian Supplement"},
  {0x11680, 0x116CF, "Takri"},
  {0x11700, 0x1174F, "Ahom"},
  {0x11800, 0x1184F, "Dogra"},
  {0x118A0, 0x118FF, "Warang Citi"},
  {0x11900, 0x1195F, "Dives Akuru"},
  {0x119A0, 0x119FF, "Nandinagari"},
  {0x11A00, 0x11A4F, "Zanabazar Square"},
  {0x11A50, 0x11AAF, "Soyombo"},
  {0x11AB0, 0x11ABF, "Unified Canadian Aboriginal Syllabics Extended-A"},
  {0x11AC0, 0x11AFF, "Pau Cin Hau"},
  {0x11B00, 0x11B5F, "Devanagari Extended-A"},
  {0x11C00, 0x11C6F, "Bhaiksuki"},
  {0x11C70, 0x11CBF, "Marchen"},
  {0x11D00, 0x11D5F, "Masaram Gondi"},
  {0x11D60, 0x11DAF, "Gunjala Gondi"},
  {0x11EE0, 0x11EFF, "Makasar"},
  {0x11F00, 0x11F5F, "Kawi"},
  {0x11FB0, 0x11FBF, "Lisu Supplement"},
  {0x11FC0, 0x11FFF, "Tamil Supplement"},
  {0x12000, 0x123FF, "Cuneiform"},
  {0x12400, 0x1247F, "Cuneiform Numbers and Punctuation"},
  {0x12480, 0x1254F, "Early Dynastic Cuneiform"},
  {0x12F90, 0x12FFF, "Cypro-Minoan"},
  {0x13000, 0x1342F, "Egyptian Hieroglyphs"},
  {0x13430, 0x1345F, "Egyptian Hieroglyph Format Controls"},
  {0x14400, 0x1467F, "Anatolian Hieroglyphs"},
  {0x16800, 0x16A3F, "Bamum Supplement"},
  {0x16A40, 0x16A6F, "Mro"},
  {0x16A70, 0x16ACF, "Tangsa"},
  {0x16AD0, 0x16AFF, "Bassa Vah"},
  {0x16B00, 0x16B8F, "Pahawh Hmong"},
  {0x16E40, 0x16E9F, "Medefaidrin"},
  {0x16F00, 0x16F9F, "Miao"},
  {0x16FE0, 0x16FFF, "Ideographic Symbols and Punctuation"},
  {0x17000, 0x187FF, "Tangut"},
  {0x18800, 0x18AFF, "Tangut Components"},
  {0x18B00, 0x18CFF, "Khitan Small Script"},
  {0x18D00, 0x18D7F, "Tangut Supplement"},
  {0x1AFF0, 0x1AFFF, "Kana Extended-B"},
  {0x1B000, 0x1B0FF, "Kana Supplement"},
  {0x1B100, 0x1B12F, "Kana Extended-A"},
  {0x1B130, 0x1B16F, "Small Kana Extension"},
  {0x1B170, 0x1B2FF, "Nushu"},
  {0x1BC00, 0x1BC9F, "Duployan"},
  {0x1BCA0, 0x1BCAF, "Shorthand Format Controls"},
  {0x1CF00, 0x1CFCF, "Znamenny Musical Notation"},
  {0x1D000, 0x1D0FF, "Byzantine Musical Symbols"},
  {0x1D100, 0x1D1FF, "Musical Symbols"},
  {0x1D200, 0x1D24F, "Ancient Greek Musical Notation"},
  {0x1D2C0, 0x1D2DF, "Kaktovik Numerals"},
  {0x1D2E0, 0x1D2FF, "Mayan Numerals"},
  {0x1D300, 0x1D35F, "Tai Xuan Jing Symbols"},
  {0x1D360, 0x1D37F, "Counting Rod Numerals"},
  {0x1D400, 0x1D7FF, "Mathematical Alphanumeric Symbols"},
  {0x1D800, 0x1DAAF, "Sutton SignWriting"},
  {0x1DF00, 0x1DFFF, "Latin Extended-G"},
  {0x1E000, 0x1E02F, "Glagolitic Supplement"},
  {0x1E030, 0x1E08F, "Cyrillic Extended-D"},
  {0x1E100, 0x1E14F, "Nyiakeng Puachue Hmong"},
  {0x1E290, 0x1E2BF, "Toto"},
  {0x1E2C0, 0x1E2FF, "Wancho"},
  {0x1E4D0, 0x1E4FF, "Nag Mundari"},
  {0x1E7E0, 0x1E7FF, "Ethiopic Extended-B"},
  {0x1E800, 0x1E8DF, "Mende Kikakui"},
  {0x1E900, 0x1E95F, "Adlam"},
  {0x1EC70, 0x1ECBF, "Indic Siyaq Numbers"},
  {0x1ED00, 0x1ED4F, "Ottoman Siyaq Numbers"},
  {0x1EE00, 0x1EEFF, "Arabic Mathematical Alphabetic Symbols"},
  {0x1F000, 0x1F02F, "Mahjong Tiles"},
  {0x1F030, 0x1F09F, "Domino Tiles"},
  {0x1F0A0, 0x1F0FF, "Playing Cards"},
  {0x1F100, 0x1F1FF, "Enclosed Alphanumeric Supplement"},
  {0x1F200, 0x1F2FF, "Enclosed Ideographic Supplement"},
  {0x1F300, 0x1F5FF, "Miscellaneous Symbols and Pictographs"},
  {0x1F600, 0x1F64F, "Emoticons"},
  {0x1F650, 0x1F67F, "Ornamental Dingbats"},
  {0x1F680, 0x1F6FF, "Transport and Map Symbols"},
  {0x1F700, 0x1F77F, "Alchemical Symbols"},
  {0x1F780, 0x1F7FF, "Geometric Shapes Extended"},
  {0x1F800, 0x1F8FF, "Supplemental Arrows-C"},
  {0x1F900, 0x1F9FF, "Supplemental Symbols and Pictographs"},
  {0x1FA00, 0x1FA6F, "Chess Symbols"},
  {0x1FA70, 0x1FAFF, "Symbols and Pictographs Extended-A"},
  {0x1FB00, 0x1FBFF, "Symbols for Legacy Computing"},

  /* Plane 2: Supplementary Ideographic Plane */
  {0x20000, 0x2A6DF, "CJK Unified Ideographs Extension B"},
  {0x2A700, 0x2B73F, "CJK Unified Ideographs Extension C"},
  {0x2B740, 0x2B81F, "CJK Unified Ideographs Extension D"},
  {0x2B820, 0x2CEAF, "CJK Unified Ideographs Extension E"},
  {0x2CEB0, 0x2EBEF, "CJK Unified Ideographs Extension F"},
  {0x2F800, 0x2FA1F, "CJK Compatibility Ideographs Supplement"},

  /* Plane 3: Tertiary Ideographic Plane */
  {0x30000, 0x3134F, "CJK Unified Ideographs Extension G"},
  {0x31350, 0x323AF, "CJK Unified Ideographs Extension H"},

  /* Plane 14: Supplementary Special-purpose Plane */
  {0xE0000, 0xE007F, "Tags"},
  {0xE0100, 0xE01EF, "Variation Selectors Supplement"},

  /* Planes 15 and 16: Supplementary Private Use Areas */
  {0xF0000, 0xFFFFF, "Supplementary Private Use Area-A"},
  {0x100000, 0x10FFFF, "Supplementary Private Use Area-B"},
};

#define N_UNICODE_BLOCKS (sizeof(unicodeBlock) / sizeof(UNICODE_BLOCK))

#endif /* SUPPORTS_UTF8 */

#if EXTRA_CHARS_IN_CONTROL_CODES
  int nRows;		/* Number of rows on the console */
#endif

/* Forward references */
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
#if defined(_MSDOS)
int PrintCharCode(int iCode, int iFlags, unsigned uCP2, int iCode2);
#elif defined(_WIN32)
int PrintCharCode(int iCode, int iFlags, unsigned uCP2, char cCode2[], int nCode2);
#else /*  defined(_UNIX) */
int PrintCharCode(int iCode, int iFlags, char cCode2[], int nCode2);
#endif
int PrintRange(int iFirst, int iLast, int iFlags);
int DetectAnsi(int iFlags);

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
  int i;
#ifdef _WIN32
  unsigned uCP = GetConsoleOutputCP();	/* The current console code page */
  unsigned uCP0 = uCP;			/* The initial console code page */
  unsigned uCP1 = 0;			/* The requested console code page */
  CPINFOEX cpi = {0};
#endif
  int iFirst = 0;
  int iLast = -1;
  int iMaxChar = 0;
  int iVerbose = FALSE;
#if SUPPORTS_UTF8
  int isUTF8 = FALSE;
  int iBlock;
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
#endif
  int isMBCS = FALSE;
  int isASCII = FALSE;
  int iExitCode = 1;
  int n, m;
  int iFlags = 0;	/* Input flags, passed to subroutines */
  int iOutFlags;	/* Context info returned by ParseCharCode() */
  CHAR_DEF *charDefs = NULL;
  int nChars = 0;
  RANGE_DEF *rangeDefs = NULL;
  int nRanges = 0;

#ifdef _MSDOS
  unsigned uCP0, uCP;
#ifndef _BIOS
  union REGS inRegs, outRegs;
  inRegs.x.ax = 0x6601;	/* Get Global Code Page */
  intdos(&inRegs, &outRegs);
  uCP0 = uCP = outRegs.x.bx;
#else
  uCP0 = uCP = 437;	/* The BIOS uses its own code page */
#endif /* _BIOS */
#endif /* _MSDOS */

#if SUPPORTS_UTF8
#ifdef _WIN32
  char **argvCP0;
  char **argvCP1;
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
#endif /* defined(_WIN32) */
#ifdef _UNIX
  if (pszLocale) {
    if (stristr(pszLocale, "UTF-8") || stristr(pszLocale, "utf8")) isUTF8 = TRUE;
  }
#endif /* defined(_UNIX) */
#endif /* SUPPORTS_UTF8 */

  if (isTTY) iFlags |= CF_TTY;

#if EXTRA_CHARS_IN_CONTROL_CODES
  nRows = GetConRows();
#endif

  /* Process the command-line arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {
      char *opt = arg+1;
#ifdef _WIN32
      if (   streq(opt, "8")	/* -8: Set the UTF8 code page */
	  || streq(opt, "-utf8")) {
	uCP1 = 65001;
	continue;
      }
#endif /* defined(_WIN32) */
      if (   streq(opt, "a")     /* -a: Display all characters */
	  || streq(opt, "-all")) {
	iFlags |= CF_ALL;
	continue;
      }
#if SUPPORTS_UTF8
      if (   streq(opt, "b")     /* -b: List Unicode Blocks */
	  || streq(opt, "-blocks")) {
        for (iBlock=0, iLast=0; iBlock<N_UNICODE_BLOCKS; iBlock++, iLast++) {
	  iFirst = unicodeBlock[iBlock].iFirst;
	  DEBUG_CODE(
	    if (iVerbose && (iFirst < iLast)) {
	      fprintf(stderr, "U+%04X-U+%04X %s\n", iLast, iFirst, "BLOCK ORDERING ERROR");
	    }
	  )
	  if (iVerbose && (iFirst > iLast)) {
	    printf("U+%04X-U+%04X %s\n", iLast, iFirst-1, "UNASSIGNED");
	  }
	  iLast = unicodeBlock[iBlock].iLast;
	  printf("U+%04X-U+%04X %s\n", iFirst, iLast, unicodeBlock[iBlock].pszName);
	}
	return 0;
      }
#endif /* SUPPORTS_UTF8 */
#ifdef _WIN32
      if (   streq(opt, "c")	/* -c: Set the code page */
	  || streq(opt, "-cp")) {
        char *pszCP = ((i+1)<argc) ? argv[++i] : "0";
	uCP1 = atoi(pszCP);
	if ((uCP1 <= 0) || (uCP1 > 65535)) {
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
	return 0;
      }
      if (   streq(opt, "q")     /* -q: Quiet mode: Minimal output */
	  || streq(opt, "-quiet")) {
	iFlags |= CF_QUIET;
	continue;
      }
      if (   streq(opt, "v")     /* -v: Verbose mode: Detailed output */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	iFlags |= CF_VERBOSE;
	continue;
      }
      if (   streq(opt, "V")     /* -V: Display the version */
	  || streq(opt, "-version")) {
	puts(DETAILED_VERSION);
	return 0;
      }
      fprintf(stderr, "Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    /* Else it's a normal argument */
    /* First look for known Unicode range names */
#if SUPPORTS_UTF8
    if (strlen(arg) > 2) {
      int iFound = 0;
      for (iBlock=0; iBlock<N_UNICODE_BLOCKS; iBlock++) {
      	char *pszBlock = unicodeBlock[iBlock].pszName;
	if (stristr(pszBlock, arg)) {
	  rangeDefs = realloc(rangeDefs, (nRanges+1) * sizeof(RANGE_DEF));
	  if (!rangeDefs) goto not_enough_memory;
	  rangeDefs[nRanges].iFirst = unicodeBlock[iBlock].iFirst;
	  rangeDefs[nRanges].iLast = unicodeBlock[iBlock].iLast;
	  rangeDefs[nRanges].iFlags = iFlags | CF_UNICODE;
	  nRanges += 1;
	  iFound = 1;
	}
      }
      if (iFound) continue;
    }
#endif /* SUPPORTS_UTF8 */
    n = ParseCharCode(arg, &iFirst, &iOutFlags);
#if SUPPORTS_UTF8
    if (isUTF8) {
      iOutFlags |= CF_UNICODE; /* Default to Unicode, even for \xXX cases */
    }
#endif /* SUPPORTS_UTF8 */
    DEBUG_PRINTF(("c0 = \\x%02X; c1 = \\x%02X\n", arg[0] & 0xFF, arg[1] & 0xFF));
    DEBUG_PRINTF(("n = %d; iFirst = \\x%02X; iOutFlags = 0x%X\n", n, iFirst, iOutFlags));
    if (n > 0) {
      if (!arg[n]) { /* If this code is the complete argument */
      	charDefs = realloc(charDefs, (nChars+1) * sizeof(CHAR_DEF));
      	if (!charDefs) {
not_enough_memory:
	  fprintf(stderr, "Not enough memory.\n");
	  return 1;
	}
	charDefs[nChars].iCode = iFirst;
	charDefs[nChars].iFlags = iOutFlags;
#ifdef _WIN32
	charDefs[nChars].iArg = i;
#endif /* defined(_WIN32) */
	nChars += 1;
        continue;
      }
      if (arg[n] != '-') goto arg_ignored;
      m = ParseCharCode(arg+n+1, &iLast, NULL);
#if SUPPORTS_UTF8
      if (isUTF8) iOutFlags |= CF_UNICODE; /* Default to Unicode, even for \xXX cases */
#endif
      DEBUG_PRINTF(("m = %d; iLast = \\x%02X; iOutFlags = 0x%X\n", m, iLast, iOutFlags));
      if (!m) goto arg_ignored;
      if (arg[n+1+m]) goto arg_ignored; /* More follows in the argument */
      rangeDefs = realloc(rangeDefs, (nRanges+1) * sizeof(RANGE_DEF));
      if (!rangeDefs) goto not_enough_memory;
      if (iFirst <= iLast) {	/* The normal case */
	rangeDefs[nRanges].iFirst = iFirst;
	rangeDefs[nRanges].iLast = iLast;
      } else {			/* Inverse ends */
	rangeDefs[nRanges].iFirst = iLast;
	rangeDefs[nRanges].iLast = iFirst;
      }
      rangeDefs[nRanges].iFlags = iOutFlags;
      nRanges += 1;
      continue;
    }
arg_ignored:
    fprintf(stderr, "Unrecognized argument %s. Ignored.\n", arg);
  }

#if MICROSOFT_OS
  if (iVerbose) printf("The console code page is %d\n", uCP0);
#endif /* MICROSOFT_OS */
#ifdef _UNIX
  if (iVerbose) printf("The system locale is %s\n", pszLocale);
#endif /* UNIX */

#if defined(_MSDOS) && !defined(_BIOS)
  if ((uCP0 > 930U) && (uCP0 < 950U)) isMBCS = 1; /* Asian MBCS supported by DOS are all in this range */
#endif /* defined(_MSDOS) */
#ifdef _WIN32
  if (uCP1 && (uCP1 != uCP)) {
    if (iVerbose) printf("Switching to code page %d.\n", uCP1);
    if (!SetConsoleOutputCP(uCP1)) {
      fprintf(stderr, "Failed to switch to code page %d.\n", uCP1);
      return 1;
    }
    uCP = uCP1;
    isUTF8 = (uCP1 == CP_UTF8);
    GetCpArgv(uCP1, &argvCP1);
  } else {
    uCP1 = uCP0;
    argvCP1 = argvCP0;
  }
/****** Now on, All exits must be done via the cleanup label in the end ******/
  if (!GetCPInfoEx(uCP, 0, &cpi)) {
    fprintf(stderr, "Error: Can't get info about Code Page %d\n", uCP);
    goto cleanup;
  }
  /* Check if MSBCS code set */
  isMBCS = (cpi.MaxCharSize > 1);
#endif /* defined(_WIN32) */
#ifdef _UNIX
  /* TODO: Detect MBCS in Unix. (Other than IUTF-8, which is flagged as MBCS further down) */
#endif /* defined(_UNIX) */

#if SUPPORTS_UTF8
#ifdef _WIN32
  if (uCP == 65001) isUTF8 = TRUE;
  if (uCP == 20127) isASCII = TRUE;
#endif
#ifdef _UNIX
  if (pszLocale) {
    if (stristr(pszLocale, "UTF-8") || stristr(pszLocale, "utf8")) isUTF8 = TRUE;
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
  if (isUTF8) iFlags |= (CF_UTF8 | CF_UNICODE);
#endif /* SUPPORTS_UTF8 */

#if SUPPORTS_UTF8
  if (isUTF8) {
    iMaxChar = 0x10FFFF;
    if (iVerbose) printf("This is 21-bits Unicode (0x00-0x10FFFF)\n");
  } else
#endif /* SUPPORTS_UTF8 */
#ifndef _MSDOS /* TODO: Detect MBCS locales in DOS */ 
  if (isMBCS) {
    iMaxChar = 0xFFFF; /* TODO: Can we detect the actual maximum value? */
    if (iVerbose) printf("This is a Multi-Byte Character Set\n");
  } else
#endif /* _MSDOS */
  if (!isASCII) {
    iMaxChar = 0xFF;
    if (iVerbose) printf("This is an 8-bits character set (0x00-0xFF)\n");
  }
#ifndef _MSDOS /* TODO: Is there an ASCII locale in DOS? */ 
  else {
    iMaxChar = 0x7F;
    if (iVerbose) printf("This is 7-bits ASCII (0x00-0x7F)\n");
  }
#endif /* _MSDOS */

  /* Default to the whole set for SBCSs, and to the first 128 characters for smaller or larger sets */
  if ((!nChars) && (!nRanges)) {
    rangeDefs = realloc(rangeDefs, (nRanges+1) * sizeof(RANGE_DEF));
    if (!rangeDefs) goto not_enough_memory;
    rangeDefs[nRanges].iFirst = 0;
    rangeDefs[nRanges].iLast = (iMaxChar == 0xFF) ? 0xFF : 0x7F;
    rangeDefs[nRanges].iFlags = iFlags;
    nRanges += 1;
  }

  /* In the (not recommended) case where there are both tables and individual
     characters, then display the individual characters in the end */
  for (i=0; i<nChars; i++) {
    rangeDefs = realloc(rangeDefs, (nRanges+1) * sizeof(RANGE_DEF));
    if (!rangeDefs) goto not_enough_memory;
    rangeDefs[nRanges].iFirst = charDefs[i].iCode;
    rangeDefs[nRanges].iLast = -1;
    rangeDefs[nRanges].iFlags = charDefs[i].iFlags;
#ifdef _WIN32
    rangeDefs[nRanges].pszArgCP1 = argvCP1[charDefs[i].iArg];
    rangeDefs[nRanges].pszArgUTF8 = argv[charDefs[i].iArg];
#endif /* defined(_WIN32) */
    nRanges += 1;
  }

#if ANSI_IS_OPTIONAL
  if (nRanges) { /* Only necessary when displaying tables */
    /* Check if the console handles ANSI escape sequences */
    isANSI = DetectAnsi(iFlags);
    if (isANSI) iFlags |= CF_ANSI;
  }
#endif /* ANSI_IS_OPTIONAL */

#if MICROSOFT_OS && !(defined(_BIOS) || defined(_LODOS))
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode(1, _O_BINARY ); /* fileno(stdout) = 1 */
#endif

/*************** Now on, all output must use EOL instead of \n ***************/

  /* Display character ranges in tables */
  iExitCode = 0;
  for (i=0; i<nRanges; i++) {
    int iMax = iMaxChar;
    char c = 'x';
#ifdef _WIN32
    WCHAR wcBuf[3] = {0};
#endif
    char cBuf[8] = {0};
    int nInBuf = 0;
#if SUPPORTS_UTF8
    int isUnicodeChar = isUTF8 || ((rangeDefs[i].iFlags & CF_UNICODE) != 0);
    iFlags &= ~CF_UNICODE;
    if (isUnicodeChar) iFlags |= CF_UNICODE;
#endif
    iFirst = rangeDefs[i].iFirst;
    iLast = rangeDefs[i].iLast;

    /* Warn the user if his requested is likely to fail to display anything */
#if SUPPORTS_UTF8
    if (iFlags & CF_UNICODE) {
      iMax = 0x10FFFF;
      c = 'u';
    }
#endif
    if (iLast > iMax) {
      fprintf(stderr, "Warning: The last requested char. \\%c%02X is larger than the last possible one \\%c%02X\n", c, iLast, c, iMax);
    }
    DEBUG_PRINT_INT_VAR(iFirst);
    DEBUG_PRINT_INT_VAR(iLast);

    /* Change the code page if needed */
#if SUPPORTS_UTF8
    DEBUG_PRINT_INT_VAR(isUnicodeChar);
    DEBUG_PRINT_INT_VAR(isUTF8);
    if (isUnicodeChar && !isUTF8) {
#ifdef _WIN32
      /* Convert the Unicode character to the requested code page */
      *(int *)wcBuf = iFirst;
      nInBuf = WideCharToMultiByte(uCP1,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			           0,			/* dwFlags, */
			           wcBuf,		/* lpWideCharStr, */
			           -1,			/* cchWideChar, */
			           cBuf,		/* lpMultiByteStr, */
			           sizeof(cBuf),	/* cbMultiByte, */
			           NULL,		/* lpDefaultChar, */
			           NULL			/* lpUsedDefaultChar */
			           );
      if (!nInBuf) {
	DEBUG_PRINTF(("Can't convert the first character to CP %u" EOL, uCP1));
      } else {
      	DEBUG_CODE(
      	int j;
	DEBUG_PRINTF(("iFirst[CP %u] = ", uCP1));
      	for (j=0; cBuf[j]; j++) DEBUG_PRINTF(("\\x%02X", cBuf[j] & 0xFF));
	DEBUG_PRINTF((EOL));
	)
      }
      if (iVerbose) printf("Switching to code page %d" EOL, CP_UTF8);
      fflush(stdout);
      if (!SetConsoleOutputCP(CP_UTF8)) {
	fprintf(stderr, "Failed to switch to code page %d" EOL, CP_UTF8);
	return 1;
      }
      uCP = CP_UTF8;
#endif /* defined(_WIN32) */
#ifdef _UNIX
      if (iVerbose) printf("Switching to the C.UTF-8 locale" EOL);
      fflush(stdout);
      pszNewLocale = setlocale(LC_ALL, "C.UTF-8");
      if (!pszNewLocale) {
	fprintf(stderr, "Failed to switch to the C.UTF-8 locale" EOL);
	return 1;
      }
#endif /* defined(_UNIX) */
      iFlags |= CF_UTF8;
      isUTF8 = TRUE;
    } else if (isUnicodeChar && isUTF8) {
      nInBuf = ToUtf8(iFirst, cBuf);
    }

    if (isUTF8 && !isUnicodeChar) {
#ifdef _WIN32
      if (iVerbose) printf("Switching back to code page %d" EOL, uCP1);
      fflush(stdout);
      if (!SetConsoleOutputCP(uCP1)) {
	fprintf(stderr, "Failed to switch back to code page %d" EOL, uCP1);
	return 1;
      }
      uCP = uCP1;
#endif /* defined(_WIN32) */
#ifdef _UNIX
      if (iVerbose) printf("Switching back to the %s locale" EOL, pszLocale);
      fflush(stdout);
      pszNewLocale = setlocale(LC_ALL, pszLocale);
      if (!pszNewLocale) {
	fprintf(stderr, "Failed to switch back to the %s locale" EOL, pszLocale);
	return 1;
      }
#endif /* defined(_UNIX) */
      iFlags &= ~CF_UTF8;
      isUTF8 = FALSE;
    }
#endif /* SUPPORTS_UTF8 */

    if (iLast > iFirst) {
      if (i) Puts(EOL);
      iExitCode += PrintRange(iFirst, iLast, iFlags);
    } else {
#if defined(_MSDOS)
      iExitCode += PrintCharCode(iFirst, iFlags, uCP, iFirst);
#elif defined(_WIN32)
      int uCP2 = 0;
      char *pBytes = cBuf;
      char *pszArgCP1 = rangeDefs[i].pszArgCP1;
      /* char *pszArgUTF8 = rangeDefs[i].pszArgUTF8; */
      DEBUG_CODE(
      int j;
      DEBUG_PRINTF(("pszArgCP1 = \""));
      for (j=0; pszArgCP1[j]; j++) DEBUG_PRINTF(("\\x%02X", pszArgCP1[j] & 0xFF));
      DEBUG_PRINTF(("\"" EOL));
      )
      if (isUnicodeChar /* && (iFlags & CF_VERBOSE) */ && (strlen(pszArgCP1) > 0)) {
      	uCP2 = uCP1;
	if (nInBuf && (pBytes[0] == '?') && (iFirst != '?')) {
	  pBytes = NULL;
	  nInBuf = 0;
	}
      }
      iExitCode += PrintCharCode(iFirst, iFlags, uCP2, pBytes, nInBuf);
#else
      char *pBytes = nInBuf ? cBuf : NULL;
      iExitCode += PrintCharCode(iFirst, iFlags, pBytes, nInBuf);
#endif /* defined(_WIN32) */
    }
  }

#ifdef _WIN32
cleanup:
  if (uCP != uCP0) {
    if (iVerbose) printf("Switching back to code page %d" EOL, uCP0);
    if (!SetConsoleOutputCP(uCP0)) {
      fprintf(stderr, "Failed to switch to code page %d" EOL, uCP0);
      return 1;
    }
  }
#endif
#ifdef _UNIX
  if (pszNewLocale != pszLocale) {
    DEBUG_PRINTF(("Switching back to the %s locale\n", pszLocale));
    fflush(stdout);
    if (!setlocale(LC_ALL, pszLocale)) {
      fprintf(stderr, "Failed to switch back to the %s locale\n", pszLocale);
      return 1;
    }
  }
#endif /* defined(_UNIX) */

#if MICROSOFT_OS && !(defined(_BIOS) || defined(_LODOS))
  fflush(stdout); /* Make sure any previous output is done in binary mode */
  _setmode(1, _O_TEXT ); /* fileno(stdout) = 1 */
#endif

  return iExitCode;
}

void usage(void) {
#if MICROSOFT_OS
#define CHARSET "code page"
#else
#define CHARSET "char. set"
#endif
#ifdef _WIN32
  unsigned uCP0 = GetConsoleOutputCP();;
  if (uCP0 != CP_UTF8) SetConsoleOutputCP(CP_UTF8);
#endif
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
"
#if SUPPORTS_UTF8
"\
Usage: chars [SWITCHES] [CHAR|CHAR_CODE|CHAR_RANGE|BLOCK_NAME] ...\n\
"
#else
"\
Usage: chars [SWITCHES] [CHAR|CHAR_CODE|CHAR_RANGE] ...\n\
"
#endif
"\
\n\
Switches:\n\
  -?|-h|--help      Display this help screen\n\
"
#ifdef _WIN32
"\
  -8|--utf8         Same as -c 65001\n\
"
#endif
"\
  -a|--all          Output all characters, even control chars like CR LF, etc\n\
"
#if SUPPORTS_UTF8
"\
  -b|--blocks       List all Unicode 15 blocks\n\
"
#endif
#ifdef _WIN32
"\
  -c|--cp CODEPAGE  Use CODEPAGE. Default: Use the current console CP %u\n\
"
#endif
#ifdef _DEBUG
"\
  -d|--debug        Display debug information\n\
"
#endif
"\
  -q|--quiet        Display only the character, but not its code\n\
  -v|--verbose      Display verbose information\n\
  -V|--version      Display this program version and exit\n\
\n\
Char. Code: X=[1-9,A-F] N=[1-9] (Use -v to display the various encodings)\n\
  XX                Hexadecimal code in the current "CHARSET". Ex: 41 for 'A'\n\
  \\xXX              Hexadecimal code in the current "CHARSET". Ex: \\x41 for 'A'\n\
  \\tNN              Decimal code in the current "CHARSET". Ex: \\t65 for 'A'\n\
  \\oNN              Octal code in the current "CHARSET". Ex: \\o101 for 'A'\n\
"
#if SUPPORTS_UTF8
"\
  \\uXXXX            Unicode code point. Ex: \\u20AC for '€'\n\
  U+XXXX            Unicode code point. Ex: U+1F310 for '🌐'\n\
  Alternatives:     XXH = \\xXX, NNT = \\tNN, NNO = \\oNN, XXXXU = \\uXXXX\n\
  \\xXX\\xXX...       A sequence of UTF-8 byte codes. Ex: \\xC3\\xA7 for 'ç'\n\
  CC...             A character encoded twice in UTF-8. Ex: \"Ã§\" for 'ç'\n\
"
#else
"\
  Alternatives:     XXH = \\xXX, NNT = \\tNN, NNO = \\oNN\n\
"
#endif
"\
\n\
Char. Range: (CHAR|CHAR_CODE)-(CHAR|CHAR_CODE)    Ex: A-Z or 41H-5AH\n\
"
#if SUPPORTS_UTF8
"\
\n\
Block Name: Full or partial name of a Unicode block, as listed by option -b.\n\
      Ex: latin\n\
"
#endif
"\
\n\
Note: By default, displays a table with the complete "CHARSET" for Single-Byte\n\
      Character Sets (SBCS), else with the first 128 characters for Multi-Byte\n\
"
#if SUPPORTS_UTF8
"\
      Character Sets (MBCS) such as UTF-8.\n\
Note: Characters beyond the 128th aren't supported for MBCS other than UTF-8.\n\
"
#else
"\
      Character Sets (MBCS).\n\
Note: Characters beyond the 128th aren't supported for MBCS.\n\
"
#endif
#if MICROSOFT_OS
"\
Note: In some environments, it may be possible to display more characters\n\
      by directly accessing the console or the video RAM buffer.\n\
"
#endif
#ifdef _UNIX
"\
Note: To know the current "CHARSET", run `locale charmap`.\n\
"
#endif
#include "footnote.h"
#ifdef _WIN32
, uCP0
#endif
);

#ifdef _WIN32
  if (uCP0 != CP_UTF8) SetConsoleOutputCP(uCP0);
#endif
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ToUtf8, ToUtf16, FromUtf8				      |
|									      |
|   Description     Conversion routines from code point to UTF-8 and back     |
|									      |
|   Parameters      							      |
|		    							      |
|   Returns	    The number of UTF-8 bytes processed; <= 0 if error	      |
|		    							      |
|   History								      |
|    2019-01-14 JFL Created the ToUtf8 routine				      |
*									      *
\*---------------------------------------------------------------------------*/

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
  DEBUG_ENTER(("FromUtf8(\"%s\", %p)" EOL, psz, pi));
  if (!psz[0]) RETURN_INT(0);		/* Empty string */
  do {
    c0 = psz[nChar++] & 0xFF;
    if ((c0 & 0xC0) == 0x80) RETURN_INT(-1);	/* Starting with a UTF-8 tail byte => Invalid 1-byte sequence */
    i = c0;
    if (c0 < 0x80) break;		/* This is one ASCII byte */

    c1 = psz[nChar++] & 0xFF;
    if ((c1 & 0xC0) != 0x80) RETURN_INT(-1);	/* Invalid UTF-8 tail byte => Incomplete 1-byte sequence */
    i = ((i & 0x1F) << 6) | (c1 & 0x3F);    /* 5 bits from c0 and 6 from c1 */
    if ((c0 & 0xE0) == 0xC0) break;	/* This is a 2-bytes UTF-8 sequence */

    c2 = psz[nChar++] & 0xFF;
    if ((c2 & 0xC0) != 0x80) RETURN_INT(-2);	/* Invalid UTF-8 tail byte => Incomplete 2-byte sequence */
    i = ((i & 0x3FF) << 6) | (c2 & 0x3F);   /* 4 bits from c0 and 6 each from c1 & c2 */
    if ((c0 & 0xF0) == 0xE0) break;	/* This is a 3-bytes UTF-8 sequence */

    c3 = psz[nChar++] & 0xFF;
    if ((c3 & 0xC0) != 0x80) RETURN_INT(-3);	/* Invalid UTF-8 tail byte => Incomplete 3-byte sequence */
    i = ((i & 0x7FFF) << 6) | (c3 & 0x3F);  /* 3 bits from c0 and 6 each from c1 to c3 */
    if ((c0 & 0xF8) == 0xF0) break;	/* This is a 4-bytes UTF-8 sequence */

    RETURN_INT(-4);				/* Incomplete 4-byte or more sequence */
  } while (0);
  *pi = i;
  RETURN_INT_COMMENT(nChar, ("char = '\\U%08lX'" EOL, i));
}

/* Convert a hexadecimal byte dump to the equivalent byte. Ex: "41" -> 'A' or "\x41" -> 'A' */
int FromHexByteDump(char *psz, char *pc) {
  int nHex=0, nChar=0, i;
  DEBUG_ENTER(("FromHexByteDump(\"%s\", %p)" EOL, psz, pc));
  if ((psz[0] == '\\') && (psz[1] == 'x')) nChar = 2;
  if ((!sscanf(psz+nChar, "%2x%n", &i, &nHex)) || !nHex) RETURN_INT(0);
  *pc = (char)i;
  nChar += nHex;
  RETURN_INT_COMMENT(nChar, ("char = '\\x%02X'" EOL, i & 0xFF));
}

/* Convert a UTF-8 dump to the unicode code point. Ex: "C3A7" -> L'ç' or "\xC3\xA7" -> L'ç' */
int FromUtf8Dump(char *psz, int *pi) {
  int n, nBytes, nChars=0;
#define SZUTF8BUFSIZE 6
  char szUtf8[SZUTF8BUFSIZE];
  int iUtf8Index[SZUTF8BUFSIZE];
  DEBUG_ENTER(("FromUtf8Dump(\"%s\", %p)" EOL, psz, pi));
  for (nBytes=0; nBytes<(SZUTF8BUFSIZE-1); nBytes++) {
    iUtf8Index[nBytes] = nChars;
    n = FromHexByteDump(psz+nChars, szUtf8+nBytes);
    if (!n) break;
    nChars += n;
  }
  if (!nBytes) RETURN_INT(0);
  szUtf8[nBytes] = '\0';
  n = FromUtf8(szUtf8, pi);
  n = (n >= 0) ? iUtf8Index[nBytes] : -iUtf8Index[nBytes];
  RETURN_INT_COMMENT(n, ("char = '\\U%08X'" EOL, *pi));
}

/* Convert a doubly UTF-8 encoded character. Ex: "Ã§" -> L'ç' */
int FromDoubleUtf8(char *psz, int *pi) {
  int n, c, nIn, nOut;
  char szUtf8[SZUTF8BUFSIZE];
  DEBUG_ENTER(("FromDoubleUtf8(\"%s\", %p)" EOL, psz, pi));
  for (nIn=nOut=0; nOut<(SZUTF8BUFSIZE-1); nOut++) {
    n = FromUtf8(psz+nIn, &c);
    if (n < 0) RETURN_INT_COMMENT(0, ("Not UTF-8" EOL));
    if (!n) break;
    nIn += n;
    if (c > 0xFF) RETURN_INT_COMMENT(0, ("Does not fit in a byte: 0x%X" EOL, c));
    szUtf8[nOut] = (char)c;
  }
  if (!nOut) RETURN_INT_COMMENT(0, ("Empty string" EOL));
  szUtf8[nOut] = '\0';
  n = FromUtf8(szUtf8, pi);
  if (n <= 0) RETURN_INT_COMMENT(0, ("Not double UTF-8" EOL));
  RETURN_INT_COMMENT(nIn, ("char = '\\U%08X'" EOL, *pi));
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
|		    int *piFlags	    Optional output flags	      |
|		    							      |
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

  DEBUG_ENTER(("ParseCharCode(\"%s\", %p, %p)" EOL, pszString, pCode, piFlags));

  if (piFlags) *piFlags = 0;
  if (!pszString[0]) RETURN_INT_COMMENT(0, ("Empty string" EOL));
  if ((!pszString[1]) || (pszString[1] == '-')) { /* A single 8-bits character */
    *pCode = pszString[0] & 0xFF;
    RETURN_INT_COMMENT(1, ("A single 8-bits character" EOL));
  }
#if SUPPORTS_UTF8
  nRead = FromUtf8Dump(pszString, pCode);	/* "\xC3\xA7" -> L'ç' */
  if ((nRead > 0) && ((!pszString[nRead]) || (pszString[nRead] == '-'))) { /* A single UTF-8 character */
    if (piFlags) *piFlags |= CF_UNICODE;
    RETURN_INT_COMMENT(nRead, ("A single UTF-8 character dump" EOL));
  }
  if ((pszString[0] & 0xFF) > 0x7F) {
    nRead = FromDoubleUtf8(pszString, pCode);	/* Ex: "Ã§" -> L'ç' */ 
    if ((nRead > 0) && ((!pszString[nRead]) || (pszString[nRead] == '-'))) { /* A single UTF-8 character */
      if (piFlags) *piFlags |= CF_UNICODE;
      RETURN_INT_COMMENT(nRead, ("A single UTF-8 character doubly encoded" EOL));
    }
    nRead = FromUtf8(pszString, pCode);		/* Ex: "ç" -> L'ç' */
    if ((nRead > 0) && ((!pszString[nRead]) || (pszString[nRead] == '-'))) { /* A single UTF-8 character */
      if (piFlags) *piFlags |= CF_UNICODE;
      RETURN_INT_COMMENT(nRead, ("A single UTF-8 character" EOL));
    }
  }
#endif /* SUPPORTS_UTF8 */
  if (pszString[0] == '\\') switch (pszString[1]) {{ /* C or Python style \xXX, etc */
    /* We've already eliminated the case where pszString[1] == '\0' */
    case 'u':
    case 'U':
      if (piFlags) *piFlags |= CF_UNICODE;
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
    if (piFlags) *piFlags |= CF_UNICODE;
    cFormat = 'x';
  }
  if (cFormat) {
    szFormat[1] = cFormat;
    nPrefix += 2;
  }                                                             
  if (!sscanf(pszString+nPrefix, szFormat, pCode, &nConv)) {
    RETURN_INT_COMMENT(0, ("Scan failed" EOL));
  }
  nRead = nPrefix + nConv;
  while (!nPrefix) {
    if ((pszString[nRead] == 'u') || (pszString[nRead] == 'U')) {
      if (piFlags) *piFlags |= CF_UNICODE;
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
	RETURN_INT_COMMENT(0, ("Rescan failed" EOL));
      }
      if (nConv != nConv0) RETURN_INT_COMMENT(0, ("Bad suffix" EOL));; /* Ex: "1DT" */
      nRead = nPrefix + nConv + 1;
      break;
    }
    if ((pszString[nRead] == 'o') || (pszString[nRead] == 'O')) {
      int nConv0 = nConv;
      szFormat[1] = 'o';
      if (!sscanf(pszString+nPrefix, szFormat, pCode, &nConv)) {
	RETURN_INT_COMMENT(0, ("Rescan failed" EOL));
      }
      if (nConv != nConv0) RETURN_INT_COMMENT(0, ("Bad suffix" EOL));; /* Ex: "9O" */
      nRead = nPrefix + nConv + 1;
      break;
    }
    break;
  }
  RETURN_INT_COMMENT(nRead, ("iCode = %02X, iFlags = %X" EOL, *pCode, piFlags ? *piFlags : 0));
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    PrintCharCode					      |
|									      |
|   Description     Display the character for a given code point	      |
|									      |
|   Parameters      							      |
|		    							      |
|   Returns	    1=Error. 0=Success					      |
|									      |
|   History								      |
|    2023-01-30 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _UNIX
char *GetCharmap(char *pszBuf, size_t nBufSize) {
  FILE *hPipe = popen("locale charmap", "r");
  if (!hPipe) return NULL;
  fgets(pszBuf, nBufSize, hPipe);
  pclose(hPipe);
  return pszBuf;
}
#endif

#if defined(_MSDOS)
int PrintCharCode(int iCode, int iFlags, unsigned uCP2, int iCode2) {
  char buf[5];
  DEBUG_ENTER(("PrintCharCode(0x%02X, 0x%X, %u, 0x%02X)" EOL, iCode, iFlags, uCP2, iCode2));
#elif defined(_WIN32)
int PrintCharCode(int iCode, int iFlags, unsigned uCP2, char cCode2[], int nCode2) {
  char buf[5];
  WCHAR wcBuf[3];
  int nwc;
  int j;
  DEBUG_ENTER(("PrintCharCode(0x%02X, 0x%X, %u, ", iCode, iFlags, uCP2));
#else
int PrintCharCode(int iCode, int iFlags, char cCode2[], int nCode2) {
  char buf[5]; 
  int j;
  DEBUG_ENTER(("PrintCharCode(0x%02X, 0x%X, ", iCode, iFlags));
#endif
#if !defined(_MSDOS)
  DEBUG_CODE(
  if (cCode2) {
    DEBUG_PRINTF(("\""));
    for (j=0; j<nCode2; j++) DEBUG_PRINTF(("\\x%02X", cCode2[j] & 0xFF));
    DEBUG_PRINTF(("\", %d", nCode2));
  } else {
    DEBUG_PRINTF(("NULL"));
  }
  )
  DEBUG_PRINTF((")" EOL));
#endif

#if SUPPORTS_UTF8
  if (iFlags & CF_UNICODE) { /* Print a Unicode character */
    int i;
    int n8 = ToUtf8(iCode, buf);
    buf[n8] = '\0';
    if (!n8) {
      fprintf(stderr, "Invalid code point: 0x%X." EOL, iCode);
      RETURN_INT(1);
    }
    if (iFlags & CF_VERBOSE) {
      WORD buf16[2];
      int n16 = ToUtf16(iCode, buf16);
      int iBlock;
      printf("Unicode U+%04X" EOL, iCode); /* The standard requires at least 4 hexadecimal characters */
      for (iBlock=0; iBlock<N_UNICODE_BLOCKS; iBlock++) {
	if (   (iCode >= unicodeBlock[iBlock].iFirst)
	    && (iCode <= unicodeBlock[iBlock].iLast)) {
	  printf("Block [U+%04X-U+%04X] %s" EOL, unicodeBlock[iBlock].iFirst,
		 unicodeBlock[iBlock].iLast, unicodeBlock[iBlock].pszName);
	  break;
	}
      }
      Puts("UTF-8  ");
      for (i=0; i<n8; i++) printf("\\x%02X", buf[i]&0xFF);
      Puts(EOL);
      Puts("UTF-16 ");
      for (i=0; i<n16; i++) printf("\\u%04X", buf16[i]);
      Puts(EOL);
      printf("UTF-32 \\U%08X" EOL, iCode);
#ifdef _WIN32
      /* Try splitting accented characters to their decomposed form */
      if (n16 == 1) {
      	WORD w = buf16[0];
	int nDec = FoldStringW(MAP_COMPOSITE, &w, 1, buf16, 2);
	if (!nDec) {
	  fprintf(stderr, "FoldString() failed. Error %d.\n", GetLastError());
	}
	if (nDec > 1) {
	  Puts("Decomposed ");
	  for (i=0; i<nDec; i++) printf("\\u%04X", buf16[i]);
	  Puts(EOL);
	}
      }
      /* Output the equivalent character code in the selected code page */
      if (uCP2) {
	printf("CP%u ", uCP2);
      	if (cCode2) {
	  for (j=0; j < nCode2; j++) printf("\\x%02X", cCode2[j] & 0xFF);
	} else {
	  Puts("(undefined)");
	}
	Puts(EOL);
      }
#endif
    } else if (!(iFlags & CF_QUIET)) {
#if defined(_WIN32) || defined(_UNIX)
      if (cCode2) {
	for (j=0; j < nCode2; j++) printf("\\x%02X", cCode2[j] & 0xFF);
	Puts(" ");
      }
#endif
      printf("\\u%04X ", iCode);
    }
  } else
#endif /* SUPPORTS_UTF8 */
  {		/* Print an 8-bits code page character */
    buf[0] = (char)iCode;
    buf[1] = '\0';
#ifdef _WIN32
    if (!uCP2) uCP2 = GetConsoleOutputCP();
    nwc = MultiByteToWideChar(uCP2,			    /* CodePage */
			      0,			    /* dwFlags */
			      (LPCCH)buf,		    /* lpMultiByteStr */
			      2,			    /* cbMultiByte */
			      wcBuf,			    /* lpWideCharStr */
			      sizeof(wcBuf)/sizeof(WCHAR)   /* cchWideChar */
			     );
#endif
    if (iFlags & CF_VERBOSE) {
#ifdef _MSDOS
      printf("CP%u \\x%02X" EOL, uCP2, iCode2); /* Display at least 2 hexadecimal characters */
#endif
#ifdef _WIN32
      printf("CP%u \\x%02X" EOL, uCP2, iCode); /* Display at least 2 hexadecimal characters */
      if (nwc > 1) printf("UTF-16 \\u%04X" EOL, *(int *)wcBuf);
#endif
#ifdef _UNIX
      char szCharmap[64];
      if (!GetCharmap(szCharmap, sizeof(szCharmap))) {
	fprintf(stderr, "Can't run `locale charmap`. %s" EOL, strerror(errno));
	RETURN_INT(1);
      }
      printf("%s \\x%02X" EOL, szCharmap, iCode); /* Display at least 2 hexadecimal characters */
#endif
    } else if (!(iFlags & CF_QUIET)) {
      printf("\\x%02X ", iCode);
#ifdef _WIN32
      if (nwc > 1) printf("\\u%04X ", *(int *)wcBuf);
#endif
    }
  }
  if (iFlags & CF_VERBOSE) Puts("'");
  Puts(buf);
  if (iFlags & CF_VERBOSE) {
    Puts("'" EOL);
  } else if (!(iFlags & CF_QUIET)) {
    Puts(EOL);
  }

  RETURN_INT(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    PrintRange						      |
|									      |
|   Description     Display a table with a range of character		      |
|									      |
|   Parameters      							      |
|		    							      |
|   Returns	    1=Error. 0=Success					      |
|									      |
|   History								      |
|    2023-02-06 JFL Extracted this routine from the main routine.	      |
*									      *
\*---------------------------------------------------------------------------*/

int PrintRange(int iFirst, int iLast, int iFlags) {
  int i, j;
  int iBase;
  int nBlock = 0;
  int iVerbose = ((iFlags & CF_VERBOSE) != 0);
  int iAll = ((iFlags & CF_ALL) != 0);
  int isTTY = ((iFlags & CF_TTY) != 0);
#if ANSI_IS_OPTIONAL
  int isANSI = ((iFlags & CF_ANSI) != 0);
#endif
  int iErr;
#if SUPPORTS_UTF8
  int isUTF8 = ((iFlags & CF_UTF8) != 0);
  int isUnicode = ((iFlags & CF_UNICODE) != 0);
  char *pszBlock = NULL;	/* Name of the Unicode block */
  int iBlock;			/* Index of the Unicode block */
#ifdef _WIN32
  char szCodePage[16];
#endif
#endif

  DEBUG_ENTER(("PrintRange(0x%02X, 0x%02X, 0x%X)" EOL, iFirst, iLast, iFlags));

  DEBUG_PRINTF(("isTTY = %d" EOL, isTTY));
#if ANSI_IS_OPTIONAL
  DEBUG_PRINTF(("isANSI = %d" EOL, isANSI));
#endif
#if SUPPORTS_UTF8
  DEBUG_PRINTF(("isUTF8 = %d" EOL, isUTF8));
  DEBUG_PRINTF(("isUnicode = %d" EOL, isUnicode));

  if (isUnicode) {
    for (iBlock=0; iBlock<N_UNICODE_BLOCKS; iBlock++) {
      if (   (iFirst == unicodeBlock[iBlock].iFirst)
	  && (iLast == unicodeBlock[iBlock].iLast)) {
	pszBlock = unicodeBlock[iBlock].pszName;
	break;
      }
    }
  }
#ifdef _WIN32
  else if (iFirst >= 0x80) {
    sprintf(szCodePage, "Code Page %d", (int)GetConsoleOutputCP());
    pszBlock = szCodePage;
  }
#endif /* defined(_WIN32) */
#endif /* SUPPORTS_UTF8 */

  for (iBase = (iFirst & -0x80); iBase < ((iLast + 0x7F) & -0x80); iBase += 0x80) {
    int iCol = 0;
    int iDigits = 2;
    for (i=0x100; i; i<<=4) if (iBase >= i) iDigits += 1;
    if (nBlock) Puts(EOL);

#if SUPPORTS_UTF8
    if (pszBlock) {
      if (!nBlock) {
      	printf("[0x%02X-0x%02X] %s" EOL, iFirst, iLast, pszBlock);
      }
    } else
#endif /* SUPPORTS_UTF8 */
    if (iVerbose || (iFirst != 0) || ((iLast != 0x7F) && (iLast != 0xFF))) {
      printf("[0x%02X-0x%02X]" EOL, iBase, iBase + 0x7F);
    }
    for (j=0; j<16; j++) {
      for (i=0; i<8; i++) {
	int k, l;
#if EXTRA_CHARS_IN_CONTROL_CODES
	int iRow0, iCol0;
#endif
	int iRow1, iCol1;

	if (!(i&3)) iCol += Print("  ");

	l = k = (iBase + 16*i)+j;
	if ((k < iFirst) || (k > iLast)) {
	  iCol += printf("%*s", iDigits+4, "");
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
	    if (isUnicode & (k >= 0x80) && (k <= 0x9F)) {
	      l = ' '; /* \u80-\u9F are additional control characters */
	      break;
	    }
#endif /* SUPPORTS_UTF8 */
	    break;
	}

	iCol += printf(" %02X ", k); /* Print the numeric code, with at least 2 characters, possibly up to 5 */
#if EXTRA_CHARS_IN_CONTROL_CODES
        iRow0 = iCol0 = 0; /* Prevent an (incorrect) uninitialized variable warning later on */
	if (isTTY && (l < ' ')) { /* Old DOS and Windows consoles display extra characters in the control codes range */
	  fflush(stdout);
	  iErr = GetCursorPosition(&iCol0, &iRow0);
	  if (iErr) goto failed_to_get_cursor_coord;
	}
#endif /* EXTRA_CHARS_IN_CONTROL_CODES */
#if SUPPORTS_UTF8
	if ((l > 0x7F) && isUTF8 && isUnicode) {
	  char buf[5];
	  int n = ToUtf8(l, buf);
	  buf[n] = '\0';
	  Puts(buf);
	} else
#endif /* SUPPORTS_UTF8 */
	PUTC(l); /* Do not use printf %c for the char, else the NUL char is not written */
	/* For ASCII characters, the cursor will move 1 column forward;
	   For Control Codes, it can stay still, or move a lot, possibly on another line;
	   For non-ASCII characters, ex. Unicode, it can move 0, 1, or 2 columns */
	if (isTTY) {
	  iCol += 2; /* We want to end up 2 columns to the right in all cases */
	  if ((l < ' ') || (l >= '\x7F')) { /* Problematic characters */
	    fflush(stdout);
	    iErr = GetCursorPosition(&iCol1, &iRow1);
	    if (iErr) {
#if EXTRA_CHARS_IN_CONTROL_CODES
failed_to_get_cursor_coord:
#endif
	      fprintf(stderr, "Failed to get the cursor coordinates\n");
	      RETURN_INT(1);
	    }
#if EXTRA_CHARS_IN_CONTROL_CODES
	    if (l < ' ') { /* The cursor may make wild moves for some control codes */
	      if (iCol1 < iCol0) {
		/* Most likely this is FF or VT, and the terminal interpreted it as CRLF */
		/* Return the cursor to its initial location... Which may have scrolled up 1 line */
		int iScrolled = (iRow0 == (nRows -1)) ? 1 : 0; /* The display scrolled if the cursor was on the last line */
		iCol1 = iCol0;
		iRow1 = iRow0-iScrolled;
		SetCursorPosition(iCol1, iRow1);
	      }
	    }
#endif /* EXTRA_CHARS_IN_CONTROL_CODES */
#ifdef _UNIX
	    iCol1 -= 1; /* It's 1-based in Unix */
#endif
	    while (iCol1++ < iCol) PUTC(' '); /* The cursor may move 0, 1, or 2 characters */
	  } else {  /* The cursor did move 1 column for ASCII chars */
	    PUTC(' '); /* Make it move a second column */
	  }
	} else { /* Writing to a file or a pipe. No way to control columns alignments */
	  PUTC(' '); /* Make it at least look good for ASCII */
	}
      }
      Puts(EOL);
      iCol = 0;
      nBlock += 1;
    }
  }

  RETURN_INT(0);
}

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

#ifdef _WIN32

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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DetectAnsi						      |
|									      |
|   Description     Check if a terminal processes ANSI escape sequences	      |
|									      |
|   Parameters      							      |
|		    							      |
|   Returns	    1=Yes it does. 0=No it does not			      |
|									      |
|   History								      |
|    2023-02-06 JFL Extracted this routine from the main routine.	      |
*									      *
\*---------------------------------------------------------------------------*/

#if ANSI_IS_OPTIONAL

int DetectAnsi(int iFlags) { /* Check if the console handles ANSI escape sequences */
  int iVerbose = ((iFlags & CF_VERBOSE) != 0);
  int isTTY = ((iFlags & CF_TTY) != 0);
  int isANSI;
  int iErr;
  if (isTTY) {
    int iCol, iCol0;
    PUTC('\r'); /* Return to the first column (0 or 1 depending on systems) */
    fflush(stdout);
    iErr = GetCursorPosition(&iCol0, NULL);
    if (iErr) { /* This cannot fail in DOS/Windows, but this CAN fail in Unix */
failed_to_get_cursor_coord:
      fprintf(stderr, "Failed to get the cursor coordinates\n");
      return -1;
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
  } else {		/* This is a file, not a terminal */
    isANSI = FALSE;	/* So it does not react to ANSI escape sequences */
  }
  return isANSI;
}

#endif /* ANSI_IS_OPTIONAL */
