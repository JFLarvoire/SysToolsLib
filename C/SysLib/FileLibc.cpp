/*****************************************************************************\
*                                                                             *
*   Filename:	    FileLibc.cpp					      *
*									      *
*   Description:    Standard C library file I/O routines		      *
*                                                                             *
*   Notes:	    This code implements the Standard C Library file I/O      *
*		    routines.						      *
*									      *
*		    OS-Independant routines are called FileXxxxx().	      *
*		    Offsets are QWORDs for compatibility with capable OSs.    *
*									      *
*   History:								      *
*    2008/04/21 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using fopen, etc */

#include <stdio.h>
#include <io.h>

#include "File.h"	// Public definitions for this module.

/*===========================================================================*\
*                                                                             *
*                     	    Standard C lib version                            *
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileLibcOpen					      |
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

/* Wrapper for fopen, to display debug info in debug mode */
FILE *FLO_fopen(char *pszName, char * pszMode)
{
    FILE *hf;
#ifdef _DEBUG
    if (iDebug)	printf("fopen(\"%s\", \"%s\") ", pszName);
#endif // _DEBUG
    hf = fopen(pszName, pszMode);
#ifdef _DEBUG
    if (iDebug)	printf("-> %p\n", hf);
#endif // _DEBUG
    return hf;
}

HANDLE FileLibcOpen(char *pszName, int iMode)
    {
    FILE *hFile;
    char *pszMode;
    
    if (iMode)
        pszMode = "rb";
    else
	pszMode = "a+b";

    hFile = FLO_fopen(pszName, pszMode);
    // If the file does not exist, and if we try to write to it, create an empty file.
    if ((!hFile) && (iMode == READWRITE)) hFile = FLO_fopen(pszName, "w+b");

    return hFile;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileLibcClose					      |
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

void FileLibcClose(HANDLE hFile)
    {
#ifdef _DEBUG
    if (iDebug)	printf("fclose(%p)\n", hFile);
#endif // _DEBUG

    fclose((FILE *)hFile);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileLibcSize					      |
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
|    2008/04/21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

QWORD FileLibcSize(HANDLE hFile)
    {
    return Dword2Qword((DWORD)_filelength(_fileno((FILE *)hFile)));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileLibcRead					      |
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
|									      |
|    2008/04/21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

/* MS-DOS tiny, small, medium memory models with a single data segment */
#if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
#pragma warning(disable:4704) /* in-line assembler precludes global optimizations */
static WORD DS(void) { /* Get the Data Segment */
  WORD w;
  _asm mov w, ds
  return w;
}
#pragma warning(default:4704) /* in-line assembler precludes global optimizations */
#endif

int FileLibcRead(HANDLE hFile, QWORD qwOffset, size_t nToRead, void FAR *pBuf)
    {
    size_t nRead;
#ifdef _DEBUG
    if (iDebug)
	{
	printf("fseek(hFile=%p, Offset=%lX, ...)\n", hFile, Qword2Dword(qwOffset));
	printf("fread(Buf@=%p, 1, N=%lX, hFile=%p) ", pBuf, (DWORD)nToRead, hFile);
	}
#endif // _DEBUG
/* MS-DOS tiny, small, medium memory models with a single data segment */
#if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
    if (DWORD1(pBuf) != DS()) {
#ifdef _DEBUG
	 if (iDebug) printf("Can't read into a far buffer! Read canceled.\n");
#endif // _DEBUG
	 return 2;
     }
#define FORCE_PVOID void *)(WORD)(DWORD
#else    /* Normal case for all Windows and Unix systems, and large DOS systems */
#define FORCE_PVOID void *
#endif /* MS-DOS M_I86?M memory models */
    fseek((FILE *)hFile, Qword2Dword(qwOffset), SEEK_SET);
    /* NOTE: The next fread() triggered a pointer truncation warning in MSDOS tiny and small modes.
             Leaving this here for now, as in most realistic cases,
             a near buffer will be used, so the cast will work fine.
	     And the sanity check above will catch the cases where it would not */
    nRead = fread((FORCE_PVOID)pBuf, 1, nToRead, (FILE *)hFile);
#ifdef _DEBUG
    if (iDebug) printf("-> nRead=%lX\n", (DWORD)nRead);
#endif
    return nRead != nToRead;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FileLibcWrite					      |
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
|									      |
|    2008/04/21 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int FileLibcWrite(HANDLE hFile, QWORD qwOffset, size_t nToWrite, void FAR *pBuf)
    {
    size_t nWritten;
#ifdef _DEBUG
    if (iDebug)
	{
	printf("fseek(hFile=%p, Offset=%lX, ...)\n", hFile, Qword2Dword(qwOffset));
	printf("fwrite(Buf@=%p, 1, Count=%lX, hFile=%p) ", pBuf, (DWORD)nToWrite, hFile);
	if (iReadOnly) printf("Read-only mode! Write canceled.\n");
	}
#endif // _DEBUG
    if (iReadOnly) return 0;
/* MS-DOS tiny, small, medium memory models with a single data segment */
#if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
    if (DWORD1(pBuf) != DS()) {
#ifdef _DEBUG
	 if (iDebug) printf("Can't write from a far buffer! Write canceled.\n");
#endif // _DEBUG
	 return 2;
     }
#define FORCE_PVOID void *)(WORD)(DWORD
#else    /* Normal case for all Windows and Unix systems, and large DOS systems */
#define FORCE_PVOID void *
#endif /* MS-DOS M_I86?M memory models */
    fseek((FILE *)hFile, Qword2Dword(qwOffset), SEEK_SET);
    /* NOTE: The next fwrite() triggered a pointer truncation warning in MSDOS tiny and small modes.
             Leaving this here for now, as in most realistic cases,
             a near buffer will be used, so the cast will work fine.
	     And the sanity check above will catch the cases where it would not */
    nWritten = fwrite((FORCE_PVOID)pBuf, 1, nToWrite, (FILE *)hFile);
#ifdef _DEBUG
    if (iDebug) printf("-> nWritten=%lX\n", (DWORD)nWritten);
#endif
    return nWritten != nToWrite;
    }

