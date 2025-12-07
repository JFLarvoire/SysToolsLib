/*****************************************************************************\
*                                                                             *
*   Filename	    fileid.c						      *
*									      *
*   Description     Get file IDs for DOS and Windows			      *
*                                                                             *
*   Notes 	     							      *
*		    							      *
*   History								      *
*    2022-01-05 JFL Created this module.				      *
*    2025-12-06 JFL Extend MlxGetFileID() as MlxGetFileAttributesAndID().     *
*                                                                             *
*         © Copyright 2022 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ security warnings */

#define _UTF8_LIB_SOURCE /* Generate the UTF-8 version of routines */

/* Microsoft C libraries include files */
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>	/* Public definitions for this module */
/* MsvcLibX library extensions */
#include "msvclibx.h"
#include "debugm.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    MlxGetFileAttributesAndID				      |
|									      |
|   Description     Get the attributes and unique ID for a file or directory  |
|									      |
|   Parameters      const char *path	The file name			      |
|		    WIN32_FILE_ATTRIBUTE_DATA *pAttr  Where to store attribs. |
|		    			Optional. See GetFileAttributesEx().  |
|		    FILE_ID *pFID	Where to store the ID. Optional.      |
|		    			See GetFileInformationByHandle().     |
|		    BOOL bLink		1 = Open links; 0 = Open their targets|
|		    							      |
|   Returns	    BOOL bDone, and errno set if not done		      |
|		    							      |
|   Notes	    The ID is similar in concept to the Unix inode number:    |
|		    A number uniquely identifying a file or directory.	      |
|		    							      |
|		    To get a file ID at the Windows cmd prompt, run:	      |
|		    fsutil file queryfileid <filename>			      |
|		    							      |
|		    WIN32 APIs comparison:				      |
|		    GetFileInformationByHandle() GetFileInformationByHandleEx |
|		    				   (FileIdInfo)		      |
|		    In Windows 95 & later	 In Windows 8 & later	      |
|		    32-bits volume ID		 64-bits volume Serial Number |
|		    	(32-bits volume ID = Low 32 bits of Serial Number)    |
|		    64-bits file ID		 128-bits file ID	      |
|		    	(Both are identical on NTFS systems)		      |
|		    							      |
|   History								      |
|    2021-12-27 JFL Created this routine				      |
|    2025-12-06 JFL Renamed GetFileID() as MlxGetFileAttributesAndID(),	      |
|		    adding arguments, and redefining the old name as a macro. |
|		    This allows collecting WIN32_FILE_ATTRIBUTE_DATA at the   |
|		    same time as the device and file IDs, and also to select  |
|		    if we want the information for a link or its target.      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

#include <windows.h>

BOOL bMlxStatSetInode = 0; /* Control whether lstat() & stat() do call MlxGetFileIDW() here */

#define USE_EXTENDED_FUNCTION 1 /* 0 = Smaller function, but may not return unique IDs on ReFS */

#if USE_EXTENDED_FUNCTION

/* The GetFileInformationByHandleEx() function is not available in Windows 95 */
/* The FileIdInfo class info is only available in Windows 8 and later */
/* https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle */
/* https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex */
/* https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info */
int bHasFileIdInfo = -1; /* -1=Unknown; 0=No; 2=Yes, Windows supports GetFileInformationByHandleEx(FileIdInfo) */
typedef BOOL (*PGETFILEINFORMATIONBYHANDLEEX)(HANDLE, int, LPVOID, DWORD);
PGETFILEINFORMATIONBYHANDLEEX pGetFileInformationByHandleEx; /* Pointer to GetFileInformationByHandleEx() */
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_WIN8)
#define FileIdInfo 18 /* Defined in WinBase.h, starting in the Windows 8 SDK */
#endif

#pragma warning(disable:4996)       /* Ignore the deprecated name warning */

#endif /* USE_EXTENDED_FUNCTION */

BOOL MlxGetFileAttributesAndIDW(const WCHAR *pwszName, WIN32_FILE_ATTRIBUTE_DATA *pAttr, FILE_ID *pFID, BOOL bLink) {
  HANDLE hFile;
  BOOL bDone;
  BY_HANDLE_FILE_INFORMATION fi;

  DEBUG_WENTER((L"MlxGetFileAttributesAndIDW(\"%s\", %p, %d);\n", pwszName, pFID, bLink));

#if USE_EXTENDED_FUNCTION

  /* Check once if GetFileInformationByHandleEx(FileIdInfo) is available */
  if (bHasFileIdInfo == -1) {
    DWORD dwVersion = GetVersion();
    int iMajor = (int)(LOBYTE(LOWORD(dwVersion)));
    int iMinor = (int)(HIBYTE(LOWORD(dwVersion)));
    pGetFileInformationByHandleEx = (PGETFILEINFORMATIONBYHANDLEEX)GetProcAddress(
      GetModuleHandle("kernel32.dll"), "GetFileInformationByHandleEx"
    );
    bHasFileIdInfo = (pGetFileInformationByHandleEx && (
                      (iMajor > 6) || ((iMajor == 6) && (iMinor > 1)))); /* Windows 8 = version 6.2 */
    DEBUG_PRINTF(("Version = %d.%d; pGetFileInformationByHandleEx = %p; bHasFileIdInfo = %d;\n",
		  iMajor, iMinor, pGetFileInformationByHandleEx, bHasFileIdInfo));
  }

#endif /* USE_EXTENDED_FUNCTION */

  hFile = CreateFileW(pwszName,
		      0, /* 0 = Neither read or write. Was GENERIC_READ. */
		      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		      NULL,
		      OPEN_EXISTING,
		      FILE_FLAG_BACKUP_SEMANTICS | (bLink ? FILE_FLAG_OPEN_REPARSE_POINT : 0),
		      NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    errno = Win32ErrorToErrno();
    DEBUG_WLEAVE((L"return FALSE; // Failed to open \"%s\"\n", pwszName));
    return 0;
  }

  XDEBUG_PRINTF(("GetFileInformationByHandle(%p, %p);\n", hFile, &fi));
  bDone = GetFileInformationByHandle(hFile, &fi);
  if (!bDone) {
    errno = Win32ErrorToErrno();
    CloseHandle(hFile);
    RETURN_BOOL_COMMENT(0, ("GetFileInformationByHandle() failed.\n", GetLastError()));
  }

  if (pAttr) {  
    pAttr->dwFileAttributes = fi.dwFileAttributes;
    pAttr->ftCreationTime   = fi.ftCreationTime;
    pAttr->ftLastAccessTime = fi.ftLastAccessTime;
    pAttr->ftLastWriteTime  = fi.ftLastWriteTime;
    pAttr->nFileSizeHigh    = fi.nFileSizeHigh;
    pAttr->nFileSizeLow     = fi.nFileSizeLow;
  }

  if (pFID) {
#if USE_EXTENDED_FUNCTION
    if (bHasFileIdInfo) {
      XDEBUG_PRINTF(("GetFileInformationByHandleEx(%p, FileIdInfo, %p, %d);\n", hFile, pFID, sizeof(FILE_ID)));
      bDone = pGetFileInformationByHandleEx(hFile, FileIdInfo, pFID, sizeof(FILE_ID));
      if (bDone) goto cleanup_and_return_done;
      DEBUG_PRINTF(("  // GetFileInformationByHandleEx() failed. Win32 error 0x%lX. Using the base ID infos.\n", GetLastError()));
    }
#endif /* USE_EXTENDED_FUNCTION */
    pFID->dwIDVol0 = fi.dwVolumeSerialNumber;
    pFID->dwIDVol1 = 0;
    pFID->dwIDFil0 = fi.nFileIndexLow;
    pFID->dwIDFil1 = fi.nFileIndexHigh;
    pFID->dwIDFil2 = 0;
    pFID->dwIDFil3 = 0;
  }

#if USE_EXTENDED_FUNCTION
cleanup_and_return_done:
#endif /* USE_EXTENDED_FUNCTION */
  CloseHandle(hFile);
  RETURN_BOOL_COMMENT(1, ("Volume ID 0x%08lX, File ID 0x%lX%07lX, Attributes 0x%04lX\n",
			  pFID->dwIDVol0, pFID->dwIDFil1, pFID->dwIDFil0, pAttr->dwFileAttributes));
}

BOOL MlxGetFileAttributesAndID(const char *pszName, WIN32_FILE_ATTRIBUTE_DATA *pAttr, FILE_ID *pFID, BOOL bLink) {
  WCHAR *pwszName;
  BOOL bDone;

  /* Convert the pathname to a unicode string, with the proper extension prefixes if it's longer than 260 bytes */
  pwszName = MultiByteToNewWidePath(CP_UTF8, pszName);
  if (!pwszName) { /* errno set already */
    RETURN_BOOL_COMMENT(0, ("Not enough memory\n"));
  }

  bDone = MlxGetFileAttributesAndIDW(pwszName, pAttr, pFID, bLink);

  free(pwszName);

  return bDone;
}

#endif /* defined(_WIN32) */

