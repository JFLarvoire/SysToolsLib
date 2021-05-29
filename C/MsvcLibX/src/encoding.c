/*****************************************************************************\
*                                                                             *
*   Filename	    encoding.c						      *
*									      *
*   Description     Heuristics for detecting the text encoding in a buffer    *
*                                                                             *
*   Notes	    Choose among the most common cases in Windows:	      *
*		    - The Windows System Encoding (CP_ACP)		      *
*		    - UTF-8						      *
*		    - UTF-16						      *
*		    Also some limited support for UTF-7 and UTF-32.	      *
*		    							      *
*   History								      *
*    2021-05-18 JFL Created this module, based on code in conv.c.	      *
*                                                                             *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ security warnings */

/* Microsoft C libraries include files */
#include <errno.h>
#include <stdio.h>
#include <string.h>
/* MsvcLibX library extensions */
#include "iconv.h"
#include "debugm.h"

#ifdef _MSDOS

#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */

/* TO DO: Add support for DOS code pages! */

#endif

#ifdef _WIN32

#include <WinNLS.h>	/* Actually already included by iconv.h */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GuessEncoding					      |
|									      |
|   Description     Use heuristics to detect the encoding in a text buffer    |
|									      |
|   Parameters      							      |
|		    							      |
|   Returns	    The likely encoding code page			      |
|		    							      |
|   Notes	    Limited to the most common cases in Windows:	      |
|		    - The Windows System Encoding (CP_ACP)		      |
|		    - UTF-8						      |
|		    - UTF-16						      |
|		    Also some limited support for UTF-7 and UTF-32.	      |
|		    							      |
|		    The goal is to be fast, with a reasonably good accuracy.  |
|		    							      |
|   History								      |
|    2021-05-18 JFL Created this routine, based on code in conv.c.	      |
*									      *
\*---------------------------------------------------------------------------*/

UINT GuessEncoding(char *pszBuffer, size_t nBufSize) {
  UINT cp;
  DEBUG_CODE(char *pszMsg = NULL;)
  
  DEBUG_ENTER(("GuessEncoding(%p, %d);\n", pszBuffer, nBufSize));

  /* First look for a Unicode BOM: https://en.wikipedia.org/wiki/Byte_order_mark */
  if ((nBufSize >= 3) && !memcmp(pszBuffer, "\xEF\xBb\xBF", 3)) { /* UTF-8 BOM */
    cp = CP_UTF8;
    DEBUG_CODE(pszMsg = "Detected a UTF-8 BOM";)
  } else if ((nBufSize >= 4) && !memcmp(pszBuffer, "\xFF\xFE\x0\x0", 4)) { /* UTF-32 BOM */
    cp = CP_UTF32;
    DEBUG_CODE(pszMsg = "Detected a UTF-32 BOM";)
  } else if ((nBufSize >= 2) && !memcmp(pszBuffer, "\xFF\xFE", 2)) { /* UTF-16 BOM */
    cp = CP_UTF16;
    DEBUG_CODE(pszMsg = "Detected a UTF-16 BOM";)
#if 0
  } else if ((nBufSize >= 2) && !memcmp(pszBuffer, "\xFE\xFF", 2)) { /* UTF-16 BE BOM */
    cp = 1201;
    DEBUG_CODE(pszMsg = "Detected a UTF-16 BE BOM";)
#endif
  } else if ((nBufSize >= 5) && !memcmp(pszBuffer, "+/v", 3)) { /* UTF-7 BOM */
    /* The UTF-7 BOM is "+/v8-" if followed by self-representing ASCII characters, else "+/vXX...XX- */
    cp = CP_UTF7;
    DEBUG_CODE(pszMsg = "Detected a UTF-7 BOM";)
  } else { /* No Unicode BOM. Try detecting UTF-8 or UTF-16 or UTF-32 without BOM */
    size_t n;
    size_t nNonASCII = 0;
    size_t nOddNUL = 0;
    size_t nEvenNUL = 0;
    size_t nWideNUL = 0;
    size_t nNonUtf32 = 0;
    int isValidUTF8 = TRUE;
    for (n=0; n<(nBufSize-1); n+=2) {
      if (!*(WORD *)(pszBuffer+n)) {
      	nWideNUL += 1;
      	if (nWideNUL > 1) break;
      }
    }
    for (n=0; n<(nBufSize-3); n+=4) {
      if (*(DWORD *)(pszBuffer+n) & 0xFFE00000) {
      	nNonUtf32 += 1;
      	break;
      }
    }
    for (n=0; n<nBufSize; n++) {
      char c = pszBuffer[n];
      if (!c) {
	if (n & 1) {
	  nOddNUL += 1;
	} else {
	  nEvenNUL += 1;
	}
	continue;
      }
/* See https://en.wikipedia.org/wiki/UTF-8 */
#define IS_ASCII(c)     ((c&0x80) == 0)
#define IS_LEAD_BYTE(c) ((c&0xC0) == 0xC0)
#define IS_TAIL_BYTE(c) ((c&0xC0) == 0x80)
      if (IS_ASCII(c)) continue;
      nNonASCII += 1;
      if (isValidUTF8) { /* No need to keep validating if we already know it's invalid */
	if (IS_LEAD_BYTE(c)) {
	  int nTailBytesExpected = 0;
	  if ((c&0x20) == 0) {
	    nTailBytesExpected = 1;
	    if ((c == '\xC0') || (c == '\xC1')) isValidUTF8 = FALSE; /* Overlong encoding of 7-bits ASCII */
	  } else if ((c&0x10) == 0) {
	    nTailBytesExpected = 2;
	  } else if ((c&0x08) == 0) {
	    nTailBytesExpected = 3;
	    if ((c >= '\xF5') && (c <= '\xF7')) isValidUTF8 = FALSE; /* Encoding of invalid Unicode chars > \u10FFFF */
	  } else {	/* No valid Unicode character requires a 5-bytes or more encoding */
	    isValidUTF8 = FALSE;
	    continue;
	  }
	  /* Then make sure that the expected tail bytes are all there */
	  for ( ; nTailBytesExpected && (++n < nBufSize); nTailBytesExpected--) {
	    c = pszBuffer[n];
	    if (!IS_ASCII(c)) nNonASCII += 1;
	    if (!IS_TAIL_BYTE(c)) { /* Invalid UTF-8 sequence */
	      isValidUTF8 = FALSE;
	      break;
	    }
	  }
	  if (nTailBytesExpected) isValidUTF8 = FALSE; /* Incomplete UTF-8 sequence at the end of the buffer */
	} else { /* Invalid UTF-8 tail byte not preceeded by a lead byte */
	  isValidUTF8 = FALSE;
	} /* End if (IS_LEAD_BYTE(c)) */
      } /* End if (isValidUTF8) */
    } /* End for each byte in pszBuffer[] */
    /* Heuristics for identifying an encoding from the information gathered so far.
       Note that this choice is probabilistic. It may not be correct in all cases. */
    if ((nWideNUL > 1) && !nNonUtf32) {
      cp = CP_UTF32; /* Assume it's UTF-32 */
      DEBUG_CODE(pszMsg = "Detected UTF-32 without BOM";)
    } else if ((nEvenNUL + nOddNUL) > 1) { /* There are NUL bytes, so it's probably a kind of UTF-16 */
      /* TO DO: Try distinguishing UTF-16 LE and UTF-16 BE */
      cp = CP_UTF16; /* Assume it's UTF-16 LE for now, which is the default in Windows */
      DEBUG_CODE(pszMsg = "Detected UTF-16 without BOM";)
    } else if (nNonASCII && isValidUTF8) {
      cp = CP_UTF8; /* We've verified this is valid UTF-8 */
      DEBUG_CODE(pszMsg = "Detected UTF-8 without BOM";)
    } else if (nNonASCII) {
      /* Default to the current console code page for input from a pipe,
	 and to the ANSI code page for input from a file. */
      cp = CP_ACP; /* Default to the Windows encoding */
      DEBUG_CODE(pszMsg = "Default to the Windows ANSI code page";)
    } else {
      cp = CP_ASCII;
      DEBUG_CODE(pszMsg = "Everything was plain ASCII";)
    }
    DEBUG_PRINTF(("nOddNUL = %lu\n", (unsigned long)nOddNUL));
    DEBUG_PRINTF(("nEvenNUL = %lu\n", (unsigned long)nEvenNUL));
    DEBUG_PRINTF(("nNonASCII = %lu\n", (unsigned long)nNonASCII));
    DEBUG_PRINTF(("isValidUTF8 = %d\n", isValidUTF8));
    DEBUG_PRINTF(("nWideNUL = %lu\n", (unsigned long)nWideNUL));
    DEBUG_PRINTF(("nNonUtf32 = %lu\n", (unsigned long)nNonUtf32));
  }
  DEBUG_LEAVE(("return %u; // %s\n", cp, pszMsg));
  return cp;
}

#endif

