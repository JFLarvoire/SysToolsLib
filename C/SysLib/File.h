/*****************************************************************************\
*                                                                             *
*   Filename:	    File.h						      *
*									      *
*   Description:    OS-independant file access routines			      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application, and  *
*		    yet give access to 64-bits files under OS capable of it.  *
*									      *
*		    OS-Independant routines are called FileXxxxx().	      *
*									      *
*   History:								      *
*    2008-04-21 JFL Created this file.					      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_FILE_H_
#define _SYSLIB_FILE_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/*****************************************************************************\
*                                                                             *
*                          Basic type definitions                             *
*                                                                             *
\*****************************************************************************/

#define TRUE 1
#define FALSE 0

/* Redefine these basic types. Compatible with Windows and EFI. */
#include "qword.h"		/* Also defines BYTE, WORD, DWORD. */

typedef void FAR *LPVOID;
typedef void * HANDLE;		/* Must be a near pointer */

/* Definitions for iMode bits. */
#define READWRITE 0	/* Bit 0 = Write protection. */
#define READONLY 1

/* OS-independant routines */
HANDLE FileOpen(char *pszName, int iMode);
void FileClose(HANDLE hFile);
QWORD FileSize(HANDLE hFile);
int FileRead(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);
int FileWrite(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);

/* Standard C library file I/O routines */
HANDLE FileLibcOpen(char *pszName, int iMode);
void FileLibcClose(HANDLE hDrive);
QWORD FileLibcSize(HANDLE hFile);
int FileLibcRead(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);
int FileLibcWrite(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);

/* Windows NT specific 64-bit file I/O routines */
#ifdef _WIN32
HANDLE FileW32Open(char *pszName, int iMode);
void FileW32Close(HANDLE hDrive);
QWORD FileW32Size(HANDLE hFile);
int FileW32Read(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);
int FileW32Write(HANDLE hDrive, QWORD qwOffset, size_t nBytes, LPVOID pBuf);
#endif /* _WIN32 */

#ifdef _WIN32

#define FileOpen  FileW32Open
#define FileClose FileW32Close
#define FileSize  FileW32Size
#define FileRead  FileW32Read
#define FileWrite FileW32Write

#else /* For anything else, use the standard C library version. */

#define FileOpen  FileLibcOpen
#define FileClose FileLibcClose
#define FileSize  FileLibcSize
#define FileRead  FileLibcRead
#define FileWrite FileLibcWrite

#endif /* _WIN32 */

/*************** Global variables to be defined by the caller ****************/

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef _DEBUG
extern int iDebug;	/* Defined in main module. If TRUE, display debug infos. */
extern int iVerbose;	/* Defined in main module. If TRUE, display progress infos. */
#include <stdio.h>	/* For printf(). */
#endif /* _DEBUG */

extern int iReadOnly;	/* Defined in main module. If TRUE, do not write to disk. */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _SYSLIB_FILE_H_ */
