/*****************************************************************************\
*									      *
*   Filename:	    windef.h					              *
*									      *
*   Description:    Windows-like definitions				      *
*									      *
*   Notes:	    Contains a limited subset of WINDOWS.H, for use by	      *
*		    applications that will run under DOS, but that want	      *
*		    to be easily portable to Windows.			      *
*									      *
*   History:								      *
*    1994-05-14 JFL Jean-Francois Larvoire created this file.		      *
*    1995-02-16 JFL Added support for C++.				      *
*		    Moved functions definitions to PMODE.H.		      *
*    2017-06-29 JFL Include the real windef.h if it's available.	      *
*									      *
*      (C) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _WINDEF_H

#ifdef _H2INC
  #define _MSDOS	/* The h2inc.exe tool is for generating 16-bits DOS code */
#endif

#ifndef WSDKINCLUDE	/* Windows SDK include directory pathname */

#define EXPORT		/* Must not be exported under DOS */
#define _WINDEF_H

#ifdef __cplusplus
extern "C" {
#endif

/****** Common definitions and typedefs ******/

#define VOID void

#undef NEAR
#undef FAR
#undef PASCAL
#undef CDECL
#undef WINAPI

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

#else /* defined(WSDKINCLUDE) */

/* Then use the actual windef.h from the Windows SDK */

/* Macros for working around the lack of a #include_next directive */
#define WINDEF_CONCAT1(a,b) a##b /* Concatenate the raw arguments */
#define WINDEF_CONCAT(a,b) WINDEF_CONCAT1(a,b) /* Substitute the arguments, then concatenate the values */
#define WINDEF_STRINGIZE1(x) #x /* Convert the raw argument to a string */
#define WINDEF_STRINGIZE(x) WINDEF_STRINGIZE1(x) /* Substitute the argument, then convert its value to a string */
#define WINSDK_INCLUDE_FILE(relpath) WINDEF_STRINGIZE(WINDEF_CONCAT(WSDKINCLUDE,WINDEF_CONCAT(/,relpath))) /* Windows SDK include files */

#define REAL_WINDEF WINSDK_INCLUDE_FILE(windef.h)
#pragma message("#include \"" REAL_WINDEF "\"")
#include REAL_WINDEF

#endif /* !defined(WSDKINCLUDE) */

#endif /* _WINDEF_H */
