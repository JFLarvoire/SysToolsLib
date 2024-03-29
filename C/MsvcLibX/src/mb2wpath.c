/*****************************************************************************\
*                                                                             *
*   Filename	    mb2wpath.c						      *
*									      *
*   Description:    WIN32 utility routine MultiByteToWidePath()		      *
*                                                                             *
*   Notes:	    Used to overcome the 260-byte limitation of many Windows  *
*		    file management APIs.				      *
*		    							      *
*   History:								      *
*    2014-07-01 JFL Created this module.				      *
*    2017-10-02 JFL Added routine MultiByteToNewWidePath().		      *
*                   Lowered the threshold for using a long name prefix, to    *
*                   work around limits in functions like CreateDirectoryW().  *
*    2017-10-04 JFL Added routine MultiByteToNewWideString().		      *
*                   Added the management of relative paths.                   *
*    2017-10-25 JFL Fixed MultiByteToWidePath support for paths with / seps.  *
*                   Added routine TrimLongPathPrefix().			      *
*    2017-10-31 JFL Improved a debug message.                                 *
*    2018-04-27 JFL Moved MultiByteToNewWideString to iconv.c.                *
*		    Split CorrectWidePath() off of MultiByteToWidePath().     *
*		    Added CorrectNewWidePath().				      *
*    2021-12-07 JFL Detect abnormal paths that also require a \\?\ prefix.    *
*                                                                             *
*         � Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _UTF8_LIB_SOURCE /* Generate the UTF-8 version of printf routines */

#include "msvclibx.h"
#include "debugm.h"

#if defined(_WIN32)

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#pragma warning(disable:4996)        /* Ignore the deprecated name warning */

#include <windows.h>
#include <direct.h>	/* For _getdrive() */
#include <string.h>
#include <errno.h>
#include <unistd.h>	/* For CompactPathW() */
#include <iconv.h>	/* For MultiByteToNewWideString() */

#define CRITICAL_LENGTH 240 /* Documented as 255 or 260, but some APIs (ex: CreateDirectoryW) fail between 240 and 250 */

/* Test if a pathname refers to a device name like NUL, CON, COM1, LPT1, ... */
BOOL MlxIsDosDeviceW(LPCWSTR pwszPath) {
  /* First the simple cases */
  if (!wcsicmp(pwszPath, L"NUL")) return TRUE;
  if (!wcsicmp(pwszPath, L"CON")) return TRUE;
  if (!wcsicmp(pwszPath, L"PRN")) return TRUE;
  if (!wcsicmp(pwszPath, L"AUX")) return TRUE;
  /* Then detect COM1...COM9 and LPT1...LPT9 */
  /* Note that COM10, LPT10, etc, if they exist, are only accessible as \\.\COM10, etc */
  if ((!wcsnicmp(pwszPath, L"COM", 3)) || (!wcsnicmp(pwszPath, L"LPT", 3))) {
    if ((pwszPath[3] >= '1') && (pwszPath[3] <= '9') && !pwszPath[4]) return TRUE;
  }
  return FALSE;
}

/* Test if a pathname contains parts that would be renormalized by WIN32 APIs */
/* See https://docs.microsoft.com/en-us/archive/blogs/jeremykuhne/path-normalization */
BOOL MlxIsAbnormalPathW(LPCWSTR pwszPath) {
  LPCWSTR pwsz;
  int lPart;
  WCHAR wBuf[CRITICAL_LENGTH+4] = L"C:\\";
  WCHAR wBuf2[CRITICAL_LENGTH+32];
  int lBuf2 = (int)(sizeof(wBuf2)/sizeof(WCHAR));
  int lResult;
  
  if ((!pwszPath) || (!pwszPath[0])) return FALSE; /* Prevent issues */

  if (lstrlenW(pwszPath) >= CRITICAL_LENGTH) return TRUE;

  if (MlxIsDosDeviceW(pwszPath)) return FALSE;

  /* Make sure that each part if normal */
  if (pwszPath[1] == L':') pwszPath += 2; /* Skip the drive letter */
  for (pwsz = pwszPath; *pwsz; pwsz += lPart) {
    lPart = (int)wcscspn(pwsz, L"\\/");
    if (!lPart) {	/* The first character is a \ or a / */
      lPart = 1;	/* Skip it */
      continue;
    }
    wcsncpy(wBuf+3, pwsz, lPart);
    wBuf[3+lPart] = L'\0';
    if ((!wcscmp(wBuf+3, L".")) || (!wcscmp(wBuf+3, L".."))) continue;
    if (MlxIsDosDeviceW(wBuf+3)) return TRUE;	/* One part is named like a DOS device */
    lResult = (int)GetFullPathNameW(wBuf, lBuf2, wBuf2, NULL); /* Normalize the path */
    if (!lResult) return TRUE;			/* Normalization failed */
    if (lResult >= lBuf2) return TRUE;		/* Normalization wants to make a huge change */
    if (wcscmp(wBuf, wBuf2)) return TRUE;	/* Normalization made a small change */
    /* The normalization did not change anything in that part. Try the next part. */
  }

  return FALSE; /* OK, it's a normal WIN32 pathname */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    MultiByteToWidePath					      |
|									      |
|   Description	    Convert a pathname to a unicode string, inserting the     |
|		     proper \\?\ prefix if it's longer than 260 bytes	      |
|									      |
|   Parameters      UINT nCodePage	    The pathname code page	      |
|		    LPCSTR pszName	    The pathname to convert	      |
|		    LPWSTR pwszBuf	    Output buffer		      |
|		    int nWideBufSize	    Output buffer size in WCHARs      |
|									      |
|   Returns	    0 = Failure, and errno set.				      |
|		    >0 = # of WCHARS in the unicode string.		      |
|		    							      |
|   Notes	    Also convert relative paths to absolute paths, to prevent |
|		    WIN32 APIs failures if the absolute path passes the limit.|
|									      |
|   History								      |
|    2014-07-01 JFL Created this routine                               	      |
|    2017-10-02 JFL Changed the return value to the string size. Was size+1.  |
|    2017-10-04 JFL Added the management of relative paths.                   |
|    2017-10-25 JFL Fixed support for paths with / separators.                |
|    2017-10-30 JFL Bugfix: Don't add a \\?\ prefix if there's one already.   |
|		    Fixed a memory leak when converting to an absolute path.  |
|    2021-12-07 JFL Detect abnormal paths that also require a \\?\ prefix.    |
*                   							      *
\*---------------------------------------------------------------------------*/

int MultiByteToWidePath(
  UINT nCodePage,
  LPCSTR pszName,
  LPWSTR pwszBuf,
  int nWideBufSize
) {
  int n;
  LPWSTR pwszName = MultiByteToNewWideString(nCodePage, pszName);
  if (!pwszName) return 0;
  n = CorrectWidePath(pwszName, pwszBuf, nWideBufSize);
  free(pwszName);
  return n;
}

int CorrectWidePath(
  LPWSTR pwszName,
  LPWSTR pwszBuf,
  int nWideBufSize
) {
  int n = 0;
  int lName;
  LPWSTR pwszName0 = pwszName;
  LPWSTR pwszBuf0 = pwszBuf;
  LPWSTR pwsz;

  /* DEBUG_WPRINTF((L"CorrectWidePath(\"%s\", %p, %d);\n", pwszName, pwszBuf, nWideBufSize)); */

  lName = lstrlenW(pwszName);

  /* Replace all / with \ */
  /* TODO: Do that in a copy of the string, and change the pwszName argument to a LPCWSTR */
  for (pwsz = pwszName; *pwsz; pwsz++) if (*pwsz == L'/') *pwsz = L'\\';

  /* Relative pathnames will cause failures if the absolute name passes the 260 character limit */
  if (MlxIsDosDeviceW(pwszName)) {				/* If this is a DOS device name */
    goto copy_the_name;		/* It needs no change */
  } else if (   (pwszName[0] != L'\\')				/* If this is not an absolute path */
	     && (   (pwszName[1] && (pwszName[1] != L':'))	/* And there's no drive letter */
		 || (!pwszName[2])				   /* Or it's just a drive letter */
		 || (pwszName[2] != L'\\'))) {			   /* Or there's no absolute path after the drive letter */
    /* Then it's a relative path. Convert it to an absolute path to prevent WIN32 APIs failures */
    int iDrive = 0;
    WCHAR *pwszRelPath = pwszName;
    int lRelPath = lName;
    int iDrive0 = _getdrive();
    int iLen;
    WCHAR *pwszBuf2;
    /* TODO: Use the new ConcatPathW routine to avoid code duplication. Possibly move code from here to there. */
    pwszBuf2 = malloc(sizeof(WCHAR) * WIDE_PATH_MAX);
    if (!pwszBuf2) goto cleanup_and_return;
    if (pwszName[1] == L':') {
      iDrive = pwszName[0] - L'@';		/* A=1, B=2, ... */
      if (iDrive > 0x20) iDrive -= 0x20;	/* The drive letter was lower case */
      pwszRelPath += 2;
      lRelPath -= 2;
    }
    if (iDrive && (iDrive != iDrive0)) _chdrive(iDrive);
    /* iLen = (int)GetCurrentDirectoryW(WIDE_PATH_MAX, pwszBuf2); */
    if (!getcwdW(pwszBuf2, WIDE_PATH_MAX)) {
      free(pwszBuf2);
      goto cleanup_and_return;
    }
    iLen = lstrlenW(pwszBuf2);
    if (iDrive && (iDrive != iDrive0)) _chdrive(iDrive0);
    if (pwszBuf2[iLen-1] != L'\\') pwszBuf2[iLen++] = L'\\';
    lstrcpyW(pwszBuf2+iLen, pwszRelPath);
    iLen += lRelPath;
    iLen = CompactPathW(pwszBuf2, pwszBuf2, WIDE_PATH_MAX);
    pwszBuf2 = ShrinkBuf(pwszBuf2, (iLen+1) * sizeof(WCHAR)); /* Avoid wasting space */
    if (MlxIsAbnormalPathW(pwszBuf2)) { /* Then processing this pathname requires prepending a special prefix */
      DEBUG_WPRINTF((L"// Relative name \"%s\" changed to \"%s\"\n", pwszName, pwszBuf2));
      pwszName = pwszBuf2;
      lName = iLen;
      if (strncmpW(pwszName, L"\\\\?\\", 4)) { /* If there's not already one returned by GetCurrentDirectoryW() */
	/* Then prepend it with "\\?\" to get 32K Unicode paths instead of 260-byte ANSI paths */
	goto prepend_win32_prefix;
      }
    } else { /* OK, we don't need the absolute path. Free its buffer. */
      free(pwszBuf2);
    }
  } else if (MlxIsAbnormalPathW(pwszName)) { /* Then processing this absolute pathname requires prepending a special prefix */
    /* See http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx */
    if (!strncmpW(pwszName, L"\\\\?\\", 4)) {		/* The name is in the Win32 file namespace */
      /* Do nothing, the "\\?\" extended-length prefix is already there */
    } else if (!strncmpW(pwszName, L"\\\\.\\", 4)) {	/* The name is in the Win32 device namespace */
      /* Do nothing, devices names should not be changed */
    } else if (!strncmpW(pwszName, L"\\\\", 2)) {		/* The name is a UNC path */
      /* Then prepend it with "\\?\UNC\" to get 32K Unicode paths instead of 260-byte ANSI paths */
      if (nWideBufSize <= 8) goto fail_no_space;
      lstrcpyW(pwszBuf, L"\\\\?\\UNC\\");
      pwszBuf += 8;
      nWideBufSize -= 8;
      pwszName += 2; /* Skip the initial \\ in the UNC name */
      lName -= 2;
    } else if (pwszName[0] == L'\\') {	/* The name is an absolute path with no drive */
      /* Then prepend it with "\\?\" to get 32K Unicode paths instead of 260-byte ANSI paths */
      if (nWideBufSize <= 6) goto fail_no_space;
      lstrcpyW(pwszBuf, L"\\\\?\\");
      /* And also add the drive, as it's required in the Win32 file namespace */
      pwszBuf[4] = L'@' + (wchar_t)_getdrive(); /* _getdrive() returns 1 for drive A, 2 for B, etc */
      pwszBuf[5] = L':';
      pwszBuf += 6;
      nWideBufSize -= 6;
    } else if (pwszName[0] && (pwszName[1] == L':') && (pwszName[2] == L'\\')) { /* The name is an absolute path */
prepend_win32_prefix:
      if (nWideBufSize <= 4) goto fail_no_space;
      /* Then prepend it with "\\?\" to get 32K Unicode paths instead of 260-byte ANSI paths */
      lstrcpyW(pwszBuf, L"\\\\?\\");
      pwszBuf += 4;
      nWideBufSize -= 4;
    } else { /* Else this is a relative pathname. Extended-length is not supported for them. */
      /* But we've already handled the relative name case previously, so this should not happen */
      DEBUG_PRINTF(("// Assert failed. Please review the CorrectWidePath() logic.\n"));
    }
  }

  if (lName >= nWideBufSize) {
fail_no_space:
    errno = ENOSPC;
    goto cleanup_and_return;
  }
copy_the_name:
  lstrcpyW(pwszBuf, pwszName);
  DEBUG_CODE(
    if (pwszBuf != pwszBuf0) {
      DEBUG_WPRINTF((L"// Long name changed to \"%s\"\n", pwszBuf0));
    }
  );
  n = lName + (int)(pwszBuf - pwszBuf0);   /* Count the added prefix length, if any */

cleanup_and_return:
  if (pwszName != pwszName0) free(pwszName);
  return n;
}

LPWSTR CorrectNewWidePath(LPWSTR pwszName) {
  LPWSTR pwszBuf = malloc(WIDE_PATH_MAX * sizeof(WCHAR));
  int n;
  if (!pwszBuf) return NULL;

  n = CorrectWidePath(pwszName, pwszBuf, WIDE_PATH_MAX);
  if (!n) {
    free(pwszBuf);
    return NULL;
  }

  pwszBuf = ShrinkBuf(pwszBuf, (n+1) * sizeof(WCHAR));
  return pwszBuf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    MultiByteToNewWidePath				      |
|									      |
|   Description	    Convert a pathname to a new unicode string, inserting the |
|		     proper \\?\ prefix if it's longer than 260 bytes	      |
|									      |
|   Parameters      UINT nCodePage	    The pathname code page	      |
|		    LPCSTR pszName	    The pathname to convert	      |
|									      |
|   Returns	    NULL = Failure, and errno set.			      |
|		    Else a pointer to the allocated buffer with the string.   |
|		    							      |
|   Notes	    Also convert relative paths to absolute paths, to prevent |
|		    WIN32 APIs failures if the absolute path passes the limit.|
|									      |
|   History								      |
|    2017-10-02 JFL Created this routine                               	      |
*                   							      *
\*---------------------------------------------------------------------------*/

/* Allocate a new wide string, and set errno in case of failure */
LPWSTR MultiByteToNewWidePath(
  UINT nCodePage,
  LPCSTR pszName
) {
  WCHAR *pwszName;
  int lName = WIDE_PATH_MAX;	/* Number of WCHARS in the unicode name (Worst case) */
  int n;
  WCHAR *pwszName2;

  pwszName = malloc(sizeof(WCHAR) * lName);
  if (!pwszName) return NULL;

  n = MultiByteToWidePath(nCodePage, pszName, pwszName, lName);
  if (!n) {
    free(pwszName);
    return NULL;
  }
  n += 1;	/* Count the final NUL */

  pwszName2 = ShrinkBuf(pwszName, sizeof(WCHAR) * n); 
  return pwszName2;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    TrimLongPathPrefix					      |
|									      |
|   Description	    Remove the \\?\ and \\?\UNC\ prefixes from a pathname     |
|									      |
|   Parameters      LPSTR pszName	    The pathname to trim	      |
|									      |
|   Returns	    The length of the final string			      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-10-25 JFL Created this routine                               	      |
*                   							      *
\*---------------------------------------------------------------------------*/

int TrimLongPathPrefix(LPSTR pszName) {
  int n = lstrlen(pszName);
  if (!strncmp(pszName, "\\\\?\\", 4)) {
    int iLen = 4;
    LPSTR lpTo = pszName;
    if (!strncmp(pszName, "\\\\?\\UNC\\", 8)) { /* Change "\\?\UNC\server\share" to "\\server\share" */
      iLen = 6;
      lpTo = pszName+2;
    }
    n -= iLen; /* Actual length of the output path (with NUL, without prefix) */
    memmove(lpTo, lpTo+iLen, n+1);
  }
  return n;
}

#endif /* defined(_WIN32) */

