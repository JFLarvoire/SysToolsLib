/*****************************************************************************\
*                                                                             *
*   Filename:	    FileW32.cpp						      *
*									      *
*   Description:    WIN32-specific 64-bits file I/O routines		      *
*                                                                             *
*   Notes:	    This code implements the NT-specific 64-bits file I/O     *
*		    routines for WIN32.					      *
*									      *
*		    OS-Independant routines are called FileXxxxx().	      *
*		    Offsets are QWORDs, supported here by the OS.	      *
*									      *
*		    For Windows 9X family, all access are done through the    *
*		    FileLibcXxxxx() subroutines, in file FileLibc.cpp.	      *
*                   (As the 64-bits file I/O functions are for NT only.)      *
*									      *
*   History:								      *
*    2008-04-21 JFL Created this file.					      *
*    2016-04-13 JFL Don't use GetFileSizeEx() & SetFilePointerEx() for WIN95. *
*    2018-03-05 JFL Added support for 64-bits read/write sizes in WIN64.      *
*    2021-03-12 JFL Fixed FileW32Read and FileW32Write that hung on errors.   *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "File.h"	// Public definitions for this module.

#include <windows.h>

/*===========================================================================*\
*                                                                             *
*                   WIN32 - NT-specific - 64-bits file I/O                    *
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileW32Open						      |
|									      |
|   Description:    Get a handle for a given file.			      |
|									      |
|   Parameters:     char* pszName   File name.				      |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The file handle, or NULL if open failed.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FileXxxx family.     |
|									      |
|   History:								      |
|									      |
|    2008/04/21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE FileW32Open(char *pszName, int iMode)
    {
    HANDLE hFile;
    DWORD dwAccessMode = GENERIC_READ;

#if defined(_WIN95)
    if (GetVersion() >= 0x80000000)		// Windows 95/98/ME
	return FileLibcOpen(pszName, iMode);
#endif /* defined(_WIN95) */

#ifdef _DEBUG
    if (iDebug)	printf("CreateFile(\"%s\", %s, ...) ", pszName,
                       iMode ? "GENERIC_READ" : "GENERIC_READ|GENERIC_WRITE");
#endif // _DEBUG

    if (iMode == READWRITE) dwAccessMode |= GENERIC_WRITE;
    hFile = CreateFile(pszName, dwAccessMode, 0, NULL, OPEN_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) hFile = NULL;

#ifdef _DEBUG
    if (iDebug)	printf("-> %p\n", hFile);
#endif // _DEBUG

    return hFile;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileW32Close					      |
|									      |
|   Description:    Release resources reserved by FileOpen.		      |
|									      |
|   Parameters:     HANDLE hFile	    file handle.		      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the FileXxxx family.     |
|									      |
|   History:								      |
|									      |
|    2008/04/21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void FileW32Close(HANDLE hFile)
    {
#if defined(_WIN95)
    if (GetVersion() >= 0x80000000)   		// Windows 95/98/ME
	FileLibcClose(hFile);
#endif /* defined(_WIN95) */

#ifdef _DEBUG
    if (iDebug)	printf("CloseHandle(%p)\n", hFile);
#endif // _DEBUG

    CloseHandle(hFile);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileW32Size						      |
|									      |
|   Description:    Get the size of a file.				      |
|									      |
|   Parameters:     HANDLE hFile	    file handle.		      |
|									      |
|   Returns:	    The file size.					      |
|									      |
|   Notes:	    This is an OS-independant API in the FileXxxx family.     |
|									      |
|   History:								      |
|									      |
|    2008-04-21 JFL Created this routine				      |
|    2016-04-13 JFL Don't use GetFileSizeEx() for WIN95.		      |
*									      *
\*---------------------------------------------------------------------------*/

QWORD FileW32Size(HANDLE hFile)
    {
    QWORD qw;

#if defined(_WIN95)
    if (GetVersion() >= 0x80000000)   		// Windows 95/98/ME
	return FileLibcSize(hFile);

    /* Windows 95/98 don't support files > 4GB, and GetFileSizeEx() is missing there. */
    /* Still we want to support 64-bits file sizes if run in NT */
    DWORD0(qw) = GetFileSize(hFile, &DWORD1(qw));
#else
    /* Keep this simple, and use the sane function for getting a 64-bits size */
    GetFileSizeEx(hFile, (PLARGE_INTEGER)&qw);
#endif /* defined(_WIN95) */

    return qw;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileW32Read						      |
|									      |
|   Description:    Read N bytes from a file.				      |
|									      |
|   Parameters:     HANDLE hFile 		Specifies the file 	      |
|		    QWORD qwOffset              Offset to read from           |
|		    size_t nToRead              Number of bytes to read       |
|		    void *pBuf                  Input buffer.                 |
|								              |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FileXxxx family.     |
|									      |
|   History:								      |
|    2008-04-21 JFL Created this routine				      |
|    2016-04-13 JFL Don't use SetFilePointerEx() for WIN95.		      |
|    2018-03-05 JFL Added support for 64-bits read sizes in WIN64.            |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN64)	/* Actually #if (sizeof(size_t) > sizeof(DWORD)) */
#define NEED_LOOP 1		/* Then we must loop reading smaller chunks */
#define CHUNK_SIZE 0x40000000	/* Read 1 GB chuncks */
#endif

int FileW32Read(HANDLE hFile, QWORD qwOffset, size_t nToRead, void FAR *pBuf)
    {
    BOOL bDone;
    size_t nRead = 0;
    DWORD dwToRead;
    DWORD dwRead = 0;
    size_t nLeft;

#if defined(_WIN95)
    if (GetVersion() >= 0x80000000)		// Windows 95/98/ME
	return FileLibcRead(hFile, qwOffset, nToRead, pBuf);
#endif /* defined(_WIN95) */

#ifdef _DEBUG
    if (iDebug) printf("FileW32Read(hFile=%p, Offset=%I64X, N=%IX, Buf@=%Fp)\n", hFile, qwOffset, nToRead, pBuf);
    if (iDebug) printf("SetFilePointerEx(hFile=%p, Offset=%I64X, ...) ", hFile, qwOffset);
#endif
    {
#if defined(_WIN95)
    /* Windows 95/98 don't support files > 4GB, and SetFilePointerEx() is missing there. */
    /* Still we want to support 64-bits file sizes if run in NT */
    QWORD qw2;
    DWORD1(qw2) = DWORD1(qwOffset);
    DWORD0(qw2) = SetFilePointer(hFile, DWORD0(qwOffset), (PLONG)(void*)&DWORD1(qw2), FILE_BEGIN);
    bDone = (qw2 == qwOffset);
#else
    /* Keep this simple, and use the sane function for setting a 64-bits offset */
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = qwOffset;
    bDone = SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN);
#endif /* defined(_WIN95) */
    }
#ifdef _DEBUG
    if (iDebug) printf("-> bDone=%d\n", (int)bDone);
#endif

#ifdef _DEBUG
    if (iDebug) printf("ReadFile(hFile=%p, Buf@=%p, N=%IX, ...) ", hFile, pBuf, nToRead);
#endif
#if NEED_LOOP
    for (nLeft = nToRead; bDone && nLeft; nLeft -= dwRead) {
      dwToRead = (nLeft > CHUNK_SIZE) ? CHUNK_SIZE : (DWORD)nLeft;
#else
      dwToRead = nLeft = nToRead;
#endif
      if (bDone) {
	dwRead = 0;
	bDone = ReadFile(hFile, (LPBYTE)pBuf+nRead, dwToRead, &dwRead, NULL);
	nRead += dwRead;
      }
#if NEED_LOOP
      if ((!bDone) || (dwRead < dwToRead)) break;
    }
#endif
#ifdef _DEBUG
    if (iDebug) printf("-> bDone=%d nRead=%IX\n", (int)bDone, nRead);
#endif
    return (nToRead != nRead);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileW32Write					      |
|									      |
|   Description:    Write N bytes to a file.				      |
|									      |
|   Parameters:     HANDLE hFile 		Specifies the file 	      |
|		    QWORD qwOffset              Offset to read from           |
|		    size_t nToWrite             Number of bytes to write      |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the FileXxxx family.     |
|									      |
|   History:								      |
|    2008-04-21 JFL Created this routine				      |
|    2016-04-13 JFL Don't use SetFilePointerEx() for WIN95.		      |
|    2018-03-05 JFL Added support for 64-bits write sizes in WIN64.           |
*									      *
\*---------------------------------------------------------------------------*/

int FileW32Write(HANDLE hFile, QWORD qwOffset, size_t nToWrite, void FAR *pBuf)
    {
    BOOL bDone;
    size_t nWritten= 0;
    DWORD dwToWrite;
    DWORD dwWritten = 0;
    size_t nLeft;

#if defined(_WIN95)
    if (GetVersion() >= 0x80000000)     	// Windows 95/98/ME
	return FileLibcWrite(hFile, qwOffset, nToWrite, pBuf);
#endif /* defined(_WIN95) */

#ifdef _DEBUG
    if (iDebug) printf("FileW32Write(hFile=%p, Offset=%I64X, N=%IX, Buf@=%Fp)\n", hFile, qwOffset, nToWrite, pBuf);
    if (iReadOnly) printf("Read-only mode! Write canceled.\n", hFile, qwOffset, nToWrite);
#endif // _DEBUG
    if (iReadOnly) return 0;
#ifdef _DEBUG
    if (iDebug) printf("SetFilePointerEx(hFile=%p, Offset=%I64X, ...) ", hFile, qwOffset);
#endif // _DEBUG
    {
#if defined(_WIN95)
    /* Windows 95/98 don't support files > 4GB, and SetFilePointerEx() is missing there. */
    /* Still we want to support 64-bits file sizes if run in NT */
    QWORD qw2;
    DWORD1(qw2) = DWORD1(qwOffset);
    DWORD0(qw2) = SetFilePointer(hFile, DWORD0(qwOffset), (PLONG)(void*)&DWORD1(qw2), FILE_BEGIN);
    bDone = (qw2 == qwOffset);
#else
    /* Keep this simple, and use the sane function for setting a 64-bits offset */
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = qwOffset;
    bDone = SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN);
#endif /* defined(_WIN95) */
    }
#ifdef _DEBUG
    if (iDebug) printf("-> bDone=%d\n", (int)bDone);
#endif

#ifdef _DEBUG
    if (iDebug) printf("WriteFile(hFile=%p, Buf@=%p, N=%IX, ...) ", hFile, pBuf, nToWrite);
#endif // _DEBUG
#if NEED_LOOP
    for (nLeft = nToWrite; bDone && nLeft; nLeft -= dwWritten) {
      dwToWrite = (nLeft > CHUNK_SIZE) ? CHUNK_SIZE : (DWORD)nLeft;
#else
      dwToWrite = nLeft = nToWrite;
#endif
      if (bDone) {
	dwWritten = 0;
      	bDone = WriteFile(hFile, (LPBYTE)pBuf+nWritten, dwToWrite, &dwWritten, NULL);
	nWritten += dwWritten;
      }
#if NEED_LOOP
      if ((!bDone) || (dwWritten < dwToWrite)) break;
    }
#endif
#ifdef _DEBUG
    if (iDebug) printf("-> bDone=%d nWritten=%IX\n", (int)bDone, nWritten);
#endif
    return (nToWrite != nWritten);
    }

