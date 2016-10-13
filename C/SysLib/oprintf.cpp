/*****************************************************************************\
*                                                                             *
*   File name:	    oprintf.cpp						      *
*                                                                             *
*   Description:    Generalized C++ object formatting			      *
*                                                                             *
*   Notes:          Design goals:					      *
*                   - Same ease of use a the C printf() routine.              *
*                   - Supports built-in C++ types and user-defined classes.   *
*                   - Easily extensible without modifying the print routine.  *
*                   - Recursive: The format string can modify itself.         *
*                   - Simple and short code, not relying on large libraries.  *
*                                                                             *
*                   All internal names prefixed with OPF, for Object PrintF.  *
*                                                                             *
*		    External names derived from the equivalent printf()       *
*                   family routine, with an o in front. Example:              *
*                   int oprintf(char *pszFormat, ...);                        *
*                                                                             *
*                   Principle:                                                *
*                   All arguments are responsible for formatting themselves.  *
*                   Formatting strings are defined as: {anything}             *
*                   The oprint routine passes each formatting string (without *
*                    the enclosing {}) to each argument formatting routine.   *
*                   The returned string replaces the formatting.              *
*                                                                             *
*                   All oprint arguments are automatically converted to an    *
*                    intermediate OPFARG object.                              *
*                   The conversion routine is built in the OPFARG class       *
*                    constructors list for C built-in types.                  *
*                   The conversion routine must be provided by user-defined   *
*                    classes as an operator OPFARG() member function.         *
*                   Automatic conversion is done by predefining overloaded    *
*                    versions of oprintf routines with 0 to N (currently 3)   *
*                    OPFARG arguments.                                        *
*                                                                             *
*                   TO DO:                                                    *
*                   - Add support for other standard C built-in types.        *
*                   - Add support for other printf family-like routines.      *
*                   - Automate generation of variable argument list routines, *
*                      using macros or templates.                             *
*                   - Automate generation of C built-in types management      *
*                      routines using templates.                              *
*                   - Use va_list macros for accessing argument lists.        *
*                                                                             *
*   History:								      *
*    2001/03/21 JFL Created this module.				      *
*                                                                             *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "oprintf.h"

#ifndef MINICOMS
#define putstr(string) fputs(string, stdout) // puts() without appending an \n.
#endif // defined(MINICOMS)

#define FALSE 0
#define TRUE 1

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Method          OPFARG::~OPFARG()					      |
|									      |
|   Description	    Class destructor					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2001/03/28 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

OPFARG::~OPFARG() 
    {
    OPF_DEBUG_PRINTF(("OPFARG(%p)::~OPFARG()\n", this));
    if (uSize) free(pObj); // Free the copy of the C data copy malloc-ated with this.
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Method          OPFARG::OPFARG()					      |
|									      |
|   Description	    Default class constructors				      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2001/03/28 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

// Default constructor.
OPFARG::OPFARG() 
    {
    OPF_DEBUG_PRINTF(("OPFARG(%p)::OPFARG()\n", this)); 
    pObj = NULL; 
    popfProc = (OPFPROC *)NULL; 
    uSize=0;
    }

// Copy constructor
OPFARG::OPFARG(const OPFARG& o2)
    {
    OPF_DEBUG_PRINTF(("OPFARG(%p)::OPFARG(OPFARG& %p)\n", this, &o2));
    pObj = o2.pObj;
    popfProc = o2.popfProc;
    uSize = o2.uSize;
    }

// Friendly constructor for use by conversion methods
OPFARG::OPFARG(OPFPROC *popf, void *po, size_t u) 
    {
    OPF_DEBUG_PRINTF(("OPFARG(%p)::OPFARG(OPFPROC *%p, void *%p, %u)\n", this, popf, po, u)); 
    popfProc=popf; 
    uSize=u;
    if (uSize)	// If requested to allocate a copy...
	{	// (Necessary for built-in C types)
        pObj=malloc(uSize);	// pObj points to the copy
	// TO DO: Manage malloc() errors
	if (pObj) memcpy(pObj, po, uSize);
	}
    else
	{
	pObj = po;		// pObj points to the original object
	}
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Method          OPFARG::OPFARG(const int i)				      |
|									      |
|   Description     C int type management				      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2001/03/20 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

OPFMANAGE(OPF_CHAR);
OPFMANAGE(OPF_UCHAR);
OPFMANAGE(OPF_INT);
OPFMANAGE(OPF_UINT);
OPFMANAGE(OPF_LONG);
OPFMANAGE(OPF_ULONG);
#ifdef _WIN32
OPFMANAGE(OPF_QWORD);
#endif
OPFMANAGE(OPF_PVOID);
// OPFMANAGE(OPF_PCHAR);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Functions       printf family					      |
|									      |
|   Description     Print to standard output                                  |
|									      |
|   Parameters	    char *pszFormat	    Format string                     |
|		    OPFARG oa   	    Formattable object list           |
|		     ...						      |
|									      |
|   Returns	    The size of the resulting string. 			      |
|									      |
|   Notes	    The CDECL modifier was added to force the C argument      |
|		    passing convention. That is all arguments on the stack.   |
|		    If they're all on the stack, then the OPFARG array is     |
|		    already available on the stack, and it's possible to      |
|		    optimize things by using this array directly.	      |
|									      |
|   History								      |
|    2001/03/21 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

// Forward reference to the internal routine that does the actual work.
int opfvsnprintf(char *pszBuf, size_t iSize, const char **pszForm, const OPFARG ***pppoa, int *pnArgs);

int CDECL oprintf(const char *pszForm)
    {
    return ovprintf(pszForm, NULL, 0);
    }

#if defined(_MSC_VER)
#pragma warning(disable:4100) /* Avoid warnings "unreferenced formal parameter" */
#endif /* defined(_MSC_VER) */

int CDECL oprintf(const char *pszForm, OPFA oa0)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[1];
    poaList[0] = &oa0;
#endif
    return ovprintf(pszForm, poaList, 1);
    }

int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[2];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
#endif
    return ovprintf(pszForm, poaList, 2);
    }

int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[3];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
#endif
    return ovprintf(pszForm, poaList, 3);
    }

int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[4];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
    poaList[3] = &oa3;
#endif
    return ovprintf(pszForm, poaList, 4);
    }

int CDECL oprintf(const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[5];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
    poaList[3] = &oa3;
    poaList[4] = &oa4;
#endif
    return ovprintf(pszForm, poaList, 5);
    }

#if defined(_MSC_VER)
#pragma warning(default:4100)
#endif /* defined(_MSC_VER) */

// Generalized routine, akin to Standard C library's vprintf().
int ovprintf(const char *pszForm, const OPFARG **ppoa, int nArgs)
    {
    char szBuf[1024];
    int iLen = opfvsnprintf(szBuf, sizeof(szBuf), &pszForm, &ppoa, &nArgs);
    putstr(szBuf);
    return iLen;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Functions       snprintf family					      |
|									      |
|   Description     Print to a string.					      |
|									      |
|   Parameters	    char *pszBuf	    Output string       	      |
|		    size_t iSize	    Output buffer size                |
|		    char *pszFormat	    Format string                     |
|		    OPFARG oa   	    Formattable object list           |
|		     ...						      |
|									      |
|   Returns	    The size of the resulting string. 			      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2001/03/20 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if (!SNPRINTF_DEFINED)	// If necessary, redefine Microsoft's convenient _snprintf().
int _snprintf(char *pszBuffer, size_t nBufSize, const char *pszFormat, ... )
    {
    va_list args;
    int nRet;

    va_start(args, pszFormat);
    nRet = vsnprintf(pszBuffer, nBufSize, pszFormat, args);
    va_end(args);

    return nRet;
    }
#endif // !SNPRINTF_DEFINED

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm)
    {
    return ovsnprintf(pszBuf, iSize, pszForm, NULL, 0);
    }

#if defined(_MSC_VER)
#pragma warning(disable:4100) /* Avoid warnings "unreferenced formal parameter" */
#endif /* defined(_MSC_VER) */

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[1];
    poaList[0] = &oa0;
#endif
    return ovsnprintf(pszBuf, iSize, pszForm, poaList, 1);
    }

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[2];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
#endif
    return ovsnprintf(pszBuf, iSize, pszForm, poaList, 2);
    }

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[3];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
#endif
    return ovsnprintf(pszBuf, iSize, pszForm, poaList, 3);
    }

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[4];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
    poaList[3] = &oa3;
#endif
    return ovsnprintf(pszBuf, iSize, pszForm, poaList, 4);
    }

int CDECL osnprintf(char *pszBuf, size_t iSize, const char *pszForm, OPFA oa0, OPFA oa1, OPFA oa2, OPFA oa3, OPFA oa4)
    {
#if ARGS_ON_STACK
    const OPFARG **poaList;
    poaList = (const OPFARG **)((&pszForm)+1);
#else
    const OPFARG *poaList[5];
    poaList[0] = &oa0;
    poaList[1] = &oa1;
    poaList[2] = &oa2;
    poaList[3] = &oa3;
    poaList[4] = &oa4;
#endif
    return ovsnprintf(pszBuf, iSize, pszForm, poaList, 5);
    }

#if defined(_MSC_VER)
#pragma warning(default:4100)
#endif /* defined(_MSC_VER) */

// Generalized routine, akin to Microsoft C _vsnprintf().
int ovsnprintf(char *pszBuf, size_t iSize, const char *pszForm, const OPFARG **ppoa, int nArgs)
    {
    return opfvsnprintf(pszBuf, iSize, &pszForm, &ppoa, &nArgs);
    }

// Internal implementation routine
int opfvsnprintf(char *pszBuf, size_t iSize, const char **ppszForm, const OPFARG ***pppoa, int *pnArgs)
    {
    const char *pIn = *ppszForm;
    char *pOut = pszBuf;
    char szForm[64];	// For temporary copy of {formats}.
    int i;
    char c;

    OPF_DEBUG_PRINTF(("opfvsnprintf(\"%s\", %p={pObj %p, popf %p, sz %d}, ...)\n", *ppszForm, **pppoa, (**pppoa)->pObj, (**pppoa)->popfProc, (**pppoa)->uSize));
    
    for (int iDone=FALSE; !iDone; )
        {
        if (iSize <= 1) break;	// There's no room for anything else but the trailing nul.
        switch (c = *(pIn++))
            {
            case '\0':
            case '}':
                iDone = TRUE;
                break;
            case '{':
                if (!*pnArgs) // Error: There are no more arguments in the input list.
                    {
                    iDone = TRUE;	// Prevent catastrophies
                    break;
                    }
                // Copy and recursively process the {} block.
                opfvsnprintf(szForm, sizeof(szForm), &pIn, pppoa, pnArgs);
                // Decode the {} block.
                OPF_DEBUG_PRINTF(("opfvsnprintf calling OPFARG(%p) popfProc at %p\n", **pppoa, (**pppoa)->popfProc));
                i = ((**pppoa)->popfProc)(pOut, iSize, szForm, **pppoa);
                (*pppoa)++;	// We used one object from the argument list
                // Don't rely on the compiler to move *ppoa, as some compilers pass OPFARG
                // objects by reference, and others pass it by value.
                // (*ppoa) = (OPFARG *)((size_t)*ppoa + opfArgSize);
                
                (*pnArgs)--;
                pOut += i;	// Advance the local output pointer
                iSize -= i;
                break;
            case '\\':
                c = *(pIn++);
                // Fall through and copy, without processing it, the next character.
            default:
                *(pOut++) = c;
                iSize -= 1;
                break;
            }
        }
    if (iSize > 0) *pOut = '\0';	// Append a trailing nul
    *ppszForm = pIn;
    return (int)(pOut - pszBuf);
    }

