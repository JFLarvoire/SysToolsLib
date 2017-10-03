/*****************************************************************************\
*                                                                             *
*   Filename	    GetLongPathName.c					      *
*									      *
*   Description:    UTF-8 version of WIN32's GetLongPathName()		      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2016-09-12 JFL Created this file, from the routine in truename.c.	      *
*    2017-03-20 JFL Include stdio.h, to get the UTF-8 version of printf.      *
*    2017-10-02 JFL Fixed support for pathnames >= 260 characters.	      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

#define _UTF8_LIB_SOURCE

#include <windows.h>	/* Also includes MsvcLibX' WIN32 UTF-8 extensions */
#include <limits.h>
#include <stdio.h>
#include "debugm.h"	/* MsvcLibX debugging macros */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetLongPathNameU					      |
|                                                                             |
|   Description:    Get the long pathname for a short UTF-8 path.	      |
|                                                                             |
|   Parameters:     See WIN32's GetLongPathName()			      |
|                                                                             |
|   Return value:   The length of the full pathname, or 0 if error	      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2015-12-18 JFL Created this routine.				      |
|    2017-10-02 JFL Removed the pathname length limitation. Also use far less |
|                   stack memory.					      |
*                                                                             *
\*---------------------------------------------------------------------------*/

DWORD WINAPI GetLongPathNameU(LPCTSTR lpShortName, LPTSTR lpBuf, DWORD nBufferLength) {
  WCHAR *pwszShortName = NULL;
  WCHAR *pwBuf = NULL;
  int n;
  int lResult;
  int lName = lstrlen(lpShortName);
  int lNameNul = lName + 1;
  int lwBuf = lNameNul + MAX_PATH; /* In most cases, the long path will be shorter than MAX_PATH. If not, the buffer will be extended below. */

  DEBUG_ENTER(("GetLongPathNameU(\"%s\", %p, %d);\n", lpShortName, lpBuf, nBufferLength));

  pwszShortName = GlobalAlloc(GMEM_FIXED, sizeof(WCHAR) * lNameNul);
  if (!pwszShortName) {
out_of_mem:
    RETURN_INT_COMMENT(0, ("Not enough memory\n"));
  }
  n = MultiByteToWideChar(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  lpShortName,		/* lpMultiByteStr, */
			  lstrlen(lpShortName)+1,  /* cbMultiByte, */
			  pwszShortName,	/* lpWideCharStr, */
			  lNameNul		/* cchWideChar, */
			  );
  if (!n) {
    GlobalFree(pwszShortName);
    RETURN_INT_COMMENT(0, ("Failed to convert the short name to Unicode\n"));
  }

realloc_wBuf:
  pwBuf = GlobalAlloc(GMEM_FIXED, sizeof(WCHAR) * lwBuf);
  if (!pwBuf) {
    GlobalFree(pwszShortName);
    goto out_of_mem;
  }
  lResult = (int)GetLongPathNameW(pwszShortName, pwBuf, lwBuf);
  if (lResult > lwBuf) { /* In the possible but unlikely case that the result does not fit in pwBuf, */
    GlobalFree(pwBuf);	  /* then extend the buffer and retry. */
    lwBuf = lResult;
    goto realloc_wBuf;
  }
  GlobalFree(pwszShortName);	 /* We won't need this buffer anymore */
  if (!lResult) {
    GlobalFree(pwBuf);
    RETURN_INT_COMMENT(0, ("GetLongPathNameW() failed\n"));
  }

  /* nRead = UnicodeToBytes(pwStr, len, buf, bufsize); */
  n = WideCharToMultiByte(CP_UTF8,		/* CodePage, (CP_ACP, CP_OEMCP, CP_UTF8, ...) */
			  0,			/* dwFlags, */
			  pwBuf,		/* lpWideCharStr, */
			  lResult + 1,		/* cchWideChar, */
			  lpBuf,		/* lpMultiByteStr, */
			  (int)nBufferLength,	/* cbMultiByte, */
			  NULL,			/* lpDefaultChar, */
			  NULL			/* lpUsedDefaultChar */
			  );
  GlobalFree(pwBuf);
  if (!n) RETURN_INT_COMMENT(0, ("Failed to convert the Long name from Unicode\n"));

  RETURN_INT_COMMENT(n-1, ("\"%s\"\n", lpBuf));
}

#endif /* defined(_WIN32) */

