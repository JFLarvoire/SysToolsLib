/*****************************************************************************\
*                                                                             *
*   Filename	    err2errno.c						      *
*									      *
*   Description:    Convert a WIN32 error to a Unix errno		      *
*                                                                             *
*   Notes:	                                                              *
*		    							      *
*   History:								      *
*    2014-02-17 JFL Created this module.				      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Microsoft C libraries include files */
#include <errno.h>
#include <stdio.h>
/* MsvcLibX library extensions */
#include "debugm.h"


#ifdef _WIN32

#include <windows.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    Win32ErrorToErrno					      |
|									      |
|   Description:    Convert a Win32 system error code to a Posix errno        |
|									      |
|   Parameters:     DWORD dwError		The Win32 error code          |
|									      |
|   Returns:	    The corresponding Posix errno			      |
|									      |
|   Notes:	    There's no 1-to-1 correspondance between Windows and      |
|		    Posix errors. This routine attempts to convert codes for  |
|		    the most likely errors for this module.		      |
|		    Please add those you encounter, that end up in the	      |
|		    default category, and incorrectly return EIO by default.  |
|									      |
|   History:								      |
|    2014-02-05 JFL Created this routine                                      |
|    2014-03-05 JFL Added the default call to _get_errno_from_oserr().        |
|    2015-12-07 JFL Use the new error conversion routine name in the UCRT.    |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_UCRT)
#define _get_errno_from_oserr __acrt_errno_from_os_error /* The name changed in the UCRT */
#endif

/* Equivalent function in MSVC library. Does not know about symlink errors. */
extern int __cdecl _get_errno_from_oserr(unsigned long oserrno);

int Win32ErrorToErrno() {
  DWORD dwError = GetLastError();

  DEBUG_CODE({
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR)&lpMsgBuf, 0, NULL);
    DEBUG_PRINTF(("// Win32 error %d (0x%X): %s", dwError, dwError, lpMsgBuf));
    LocalFree( lpMsgBuf );
  });

  switch (dwError) {
    case ERROR_PRIVILEGE_NOT_HELD: /* Not running with the SE_CREATE_SYMBOLIC_LINK_NAME privilege */
    case ERROR_ACCESS_DENIED:
    case ERROR_REPARSE_ATTRIBUTE_CONFLICT:
      return EACCES;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return ENOENT;
    case ERROR_NOT_ENOUGH_MEMORY:
      return ENOMEM;
    case ERROR_INSUFFICIENT_BUFFER:
      return E2BIG;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
      return EEXIST;
    case ERROR_WRITE_PROTECT:
      return EROFS;
    case ERROR_HANDLE_DISK_FULL:
      return ENOSPC;
    case ERROR_NOT_A_REPARSE_POINT:
    case ERROR_REPARSE_TAG_MISMATCH:
    case ERROR_INVALID_FLAGS:
    case ERROR_INVALID_PARAMETER:
      return EINVAL;
    case ERROR_INVALID_REPARSE_DATA:
    case ERROR_REPARSE_TAG_INVALID:
      return EBADF; /* Not supposed to happen in Posix OSs, but may happen when experimenting with junction() IOCTLs. */
    case ERROR_NO_UNICODE_TRANSLATION:
      return EILSEQ;
    default:
      return _get_errno_from_oserr(dwError); /* Let MSVC library decide */
  }
}

#endif /* _WIN32 */

