/*****************************************************************************\
*									      *
*	FILENAME:	windef.h				              *
*									      *
*	CONTENTS:	Windows-like definitions			      *
*									      *
*	Author: 	Jean-Francois Larvoire			94/05/14      *
*									      *
*	Notes:		Contains a limited subset of WINDOWS.H, for use by    *
*			applications that will run under DOS, but that want   *
*			to be easily portable to Windows.		      *
*									      *
*	History:							      *
*	  95/02/16 JFL	Added support for C++.				      *
*			Moved functions definitions to PMODE.H. 	      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _WINDEF_H

#define EXPORT		// Must not be exported under DOS
#define _WINDEF_H

#ifdef __cplusplus
extern "C" {
#endif

/****** Common definitions and typedefs ******/

#define VOID void

#define NEAR _near
#define FAR _far
#define PASCAL _pascal
#define CDECL _cdecl

#define WINAPI _pascal
#define CALLBACK _far _pascal

/****** Simple types & common helper macros ******/

typedef int BOOL;
#define FALSE 0
#define TRUE 1

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef unsigned int UINT;

typedef signed long LONG;

#define LOBYTE(x) ((BYTE)(x))
#define HIBYTE(x) ((BYTE)(((WORD)(x)) >> 8))

#define LOWORD(x) ((WORD)(DWORD)(x))
#define HIWORD(x) ((WORD)(((DWORD)(x)) >> 16))

#define MAKELONG(low, high) ((LONG)(  ((WORD)(low)) \
				    | (((DWORD)((WORD)(high))) << 16)  ))

/****** Common pointer types ******/

#ifndef NULL
#define NULL 0
#endif

typedef VOID FAR *LPVOID;

#define MAKELP(sel, off) ((LPVOID)MAKELONG((off), (sel)))
#define SELECTOROF(lp) HIWORD(lp)
#define OFFSETOF(lp) LOWORD(lp)

/****** Common handle types ******/

typedef UINT HANDLE;
typedef UINT HWND;
typedef UINT HGLOBAL;

typedef void (CALLBACK *FARPROC)(void);

#ifdef __cplusplus
}
#endif

#endif
