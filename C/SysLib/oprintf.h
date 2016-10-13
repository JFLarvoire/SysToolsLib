/*****************************************************************************\
*                                                                             *
*   File name:	    oprintf.h						      *
*                                                                             *
*   Description:    Generalized C++ object formatting			      *
*                                                                             *
*   Notes:          TO DO: 						      *
*		    Automate prototype generation using macros or templates.  *
*                                                                             *
*   History:								      *
*    2000-03-21 JFL Created this module.				      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
\*****************************************************************************/

#ifndef _OPRINTF_H	// Prevent multiple inclusions
#define _OPRINTF_H

#ifdef HAS_SYSLIB
#include "SysLib.h"	/* SysLib Library core definitions */
#endif

#include <stddef.h>	// For size_t

#define OPF_TRACE 0	// 1=Print trace of execution; 0=Don't.
#if OPF_TRACE
#define OPF_DEBUG_PRINTF(args) printf args
#else
#define OPF_DEBUG_PRINTF(args)
#endif

/*********************** MS-DOS-specific definitions *************************/

#if defined(_MSDOS)				// 16-bits MS-DOS

#define SNPRINTF_DEFINED 1	// Microsoft C library defines _snprintf().

#ifndef CDECL
#define CDECL _cdecl		    // Keyword for guarantying classic C argument passing.
#endif
#define ARGS_ON_STACK 1		    // Arguments on stack

#endif // defined(_MSDOS)

/************************ WIN32-specific definitions *************************/

#if defined(_WIN32)				// 32-bits Windows

#define SNPRINTF_DEFINED 1	// Microsoft C library defines _snprintf().

#ifndef CDECL
#define CDECL _cdecl		    // Keyword for guarantying classic C argument passing.
#endif
#define ARGS_ON_STACK 1		    // Arguments on stack

#endif // defined(_WIN32)

/************************ Tru64-specific definitions *************************/

#if defined(__digital__) && defined(__unix__)	// 64-bits Alpha Tru64 Unix
#define _TRU64	    // Define an identification constant similar to the ones for DOS and Windows.

#define CDECL

#define SNPRINTF_DEFINED 0	// We have to define _snprintf() ourselves.
int _snprintf(char *pszBuffer, size_t nBufSize, const char *pszFormat, ... );

#endif // defined(__digital__) && defined(__unix__)

/************************ Linux-specific definitions *************************/

#if defined(__linux__)
#define _LINUX	    // Define an identification constant similar to the ones for DOS and Windows.

#define CDECL

#define SNPRINTF_DEFINED 0	// We have to define _snprintf() ourselves.
int _snprintf(char *pszBuffer, size_t nBufSize, const char *pszFormat, ... );

#endif // defined(__linux__)

/******************** End of OS-specific definitions *************************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Macros          OPFDECLARE / OPFMANAGE				      |
|									      |
|   Description     Declare / Implement routines for managing built-in types. |
|									      |
|   Notes	    This would be done better with templates, but the	      |
|		    16-bits C++ compiler for DOS does not support them.       |
|									      |
|		    OPFCONVERT allocates a copy of the input data. 	      |
|		    This is necessary, as the compiler does not preserve      |
|		    temporary data of types char, int, etc.		      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

// Declaration that goes in OPFARG definition.
#define OPFDECLARE(ctype, type) \
    typedef ctype type; \
    OPFARG(const type)

// Macros that go in the CPP implementation file.
#define OPFFORMAT(type) \
static int opfFormat##type(char *pszOut, size_t uSize, const char *pszForm, const OPFARG *popfArg) \
    { \
    OPF_DEBUG_PRINTF(("opfFormat" #type "(\"%s\", %p)\n" , pszForm, popfArg)); \
    return _snprintf(pszOut, uSize, pszForm, *(OPFARG::type *)(popfArg->pObj)); \
    }
    
#define OPFCONVERT(type) \
OPFARG::OPFARG(const OPFARG::type data) \
    { \
    OPF_DEBUG_PRINTF(("OPFARG(%p)::OPFARG() = (" #type ")0x%08lX\n" , this, data, 0)); \
    OPFARG::type *pCopy = (OPFARG::type *)malloc(sizeof(OPFARG::type)); \
    /* TO DO: Manage malloc() errors */ \
    if (pCopy) *pCopy = data; \
    popfProc = opfFormat##type; \
    pObj = pCopy; \
    uSize = sizeof(OPFARG::type); /* Size of copy to free after use */ \
    }

#define OPFMANAGE(type) \
OPFFORMAT(type); \
OPFCONVERT(type)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Class           OPFARG						      |
|									      |
|   Description     Generic argument used by oprintf family routines          |
|									      |
|   Notes								      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

class OPFARG;   // oprintf() family argument type.

// Generic formatting method.
typedef int OPFPROC(char *pszOut, size_t uSize, const char *pszForm, const OPFARG *popfArg);

class OPFARG    // Now define the class.
    {
public:		// The members must be public, as they will be accessed by user-provided conversion routines.
    void *pObj;
    OPFPROC *popfProc;
    size_t uSize;
    // Default destructor.
    ~OPFARG();
    // Default constructor.
    OPFARG();
    // Copy constructor
    OPFARG(const OPFARG& o2);
    // Friendly constructor for use by conversion methods
    OPFARG(OPFPROC *popf, void *po, size_t u = 0);
    // Conversion constructors, for built-in C types.
    OPFDECLARE(char, OPF_CHAR);
    OPFDECLARE(unsigned char, OPF_UCHAR);
    OPFDECLARE(int, OPF_INT);
    OPFDECLARE(unsigned int, OPF_UINT);
    OPFDECLARE(long, OPF_LONG);
    OPFDECLARE(unsigned long, OPF_ULONG);
#ifdef _WIN32
    OPFDECLARE(unsigned __int64, OPF_QWORD);
#endif
    OPFDECLARE(void *, OPF_PVOID);  // Will be good for any pointer, such as <char *>.

    // TO DO: Add as many constructors as C++ built-in types to format
    };

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Functions       printf family					      |
*									      *
\*---------------------------------------------------------------------------*/

#define OPFA const OPFARG&

int CDECL oprintf(const char *pszForm);
int CDECL oprintf(const char *pszForm, OPFA oa0);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5, OPFA oa6);
int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5, OPFA oa6, OPFA oa7);

// Generalized routine, akin to Standard C library's vprintf().
int ovprintf(const char *pszForm, const OPFARG **ppoa, int nArgs);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Functions       _snprintf family					      |
*									      *
\*---------------------------------------------------------------------------*/

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5, OPFA oa6);
int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4, OPFA oa5, OPFA oa6, OPFA oa7);

// Generalized routine, akin to Microsoft C _vsnprintf().
int ovsnprintf(char *pszBuf, size_t iSize, const char *pszForm, const OPFARG **ppoa, int nArgs);

#endif // ifndef _OPRINTF_H

