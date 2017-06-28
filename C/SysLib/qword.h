/*****************************************************************************\
*                                                                             *
*   Filename:	    qword.h						      *
*									      *
*   Description:    Define a 64-bits integer class usable in C++ arithmetics. *
*                   A structure-based makeshift if also provided for C.       *
*                                                                             *
*   Notes:	    MSVC 1.5 lacks a 64-bits integer type.		      *
*		    We needed one to use EFI include files in DOS programs.   *
*		    Constraints:					      *
*		    - QWORD type usable as base types in computations.        *
*			==> Redefine all operator xx functions.		      *
*		    - No constructor option, to avoid problems with unions.   *
*			==> Use friend functions _QWORD for initializations.  *
*									      *
*		    IMPORTANT: There is a bug in the compiler (?) which	      *
*		    causes compilation errors if QWORD members are included   *
*		    in C unions. This problem does NOT happen in the absence  *
*		    of constructors for the QWORD class.		      *
*		    This bug appears for example when including efi.h.        *
*		    To work around this bug, predefine the constant	      *
*		    NO_QWORD_CONSTRUCTORS before including qword.h.	      *
*		    Then use the friend functions _QWORD(...) to explicitely  *
*		    create QWORDs if needed. Be ware of memory leaks in this  *
*		    case.						      *
*									      *
*		    IMPORTANT: By default, this file defines member functions *
*		    for printing QWORDs with the SysLib oprintf() routine.    *
*		    Including qword.h will drag in the oprintf function.      *
*		    If this is not desired, for example to make the program   *
*		    smaller, it is possible to avoid this by predefining      *
*		    constant NO_OPRINTF before including qword.h.	      *
*		    Note that using oprintf is smaller and more efficient     *
*		    than converting QWORDs to double.			      *
*									      *
*		    TO DO:						      *
*		    - Add Conversion function to decimal string, etc.	      *
*									      *
*   History:								      *
*    2000-09-15 JFL Created this file.                                        *
*    2002-02-07 JFL Added _QWORD() macros in the C version.                   *
*    2003-01-16 JFL Added operations + += - -= * *= / /= with integer args.   *
*		    Changed arguments QWORD& to const QWORD&.		      *
*		    Added optional QWORD constructors. Do NOT define them if  *
*		    constant NO_QWORD_CONSTRUCTORS is pre-defined.	      *
*    2003-01-17 JFL Conditionally compiled-out all operatorX functions. They  *
*		    may be useless when operatorX= functions are defined.     *
*		    Added operator%=.					      *
*		    Fixed memory leak of operatorX functions.		      *
*    2003-01-20 JFL Added const specifier for all methods not changing value. *
*		    Moved inline very simple functions to make code smaller.  *
*    2009-02-25 JFL Made inline every function defined here. Most aren't used.*
*    2010-10-08 JFL Bugfix: Removed inline keyword on QWORD& operator+=.      *
*    2015-08-18 JFL Added operator<< and operator>>.                          * 
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2017-06-28 JFL Disable warning "function 'QWORD::operator=' not expanded"*
*									      *
\*****************************************************************************/

#ifndef _QWORD_H_	/* Prevent multiple inclusions */
#define _QWORD_H_

#ifdef HAS_SYSLIB
#include "SysLib.h"	/* SysLib Library core definitions */
#endif

/* Define x86 register types */
#ifndef DWORD_DEFINED
#define DWORD_DEFINED

typedef unsigned char   BYTE;   /*  8-bits unsigned integer */
typedef unsigned short	WORD;	/* 16-bits unsigned integer */
typedef unsigned long	DWORD;	/* 32-bits unsigned integer */

/* Define the FAR qualifier for DOS, but not for Windows */
#ifdef _H2INC
  #define _MSDOS /* The h2inc.exe tool is for generating DOS code */
#endif
#ifndef FAR
  #ifdef _MSDOS
    #define FAR far
  #endif /* _MSDOS */
  #ifdef _WIN32
    #define FAR
  #endif /* _WIN32 */
  #ifdef __unix__
    #define FAR
  #endif /* __unix__ */
#endif /* FAR */

/* Common far pointers based on these types */
typedef void FAR *	LPVOID;
typedef BYTE FAR *	LPBYTE;
typedef WORD FAR *	LPWORD;
typedef DWORD FAR *	LPDWORD;

/* Manipulate bytes, words and dwords. Can be used both as rvalue and as lvalue. */
#define DWORD0(qw) (((DWORD *)(&(qw)))[0])
#define DWORD1(qw) (((DWORD *)(&(qw)))[1])

#define WORD0(qw) (((WORD *)(&(qw)))[0])
#define WORD1(qw) (((WORD *)(&(qw)))[1])
#define WORD2(qw) (((WORD *)(&(qw)))[2])
#define WORD3(qw) (((WORD *)(&(qw)))[3])

#define BYTE0(qw) (((BYTE *)(&(qw)))[0])
#define BYTE1(qw) (((BYTE *)(&(qw)))[1])
#define BYTE2(qw) (((BYTE *)(&(qw)))[2])
#define BYTE3(qw) (((BYTE *)(&(qw)))[3])
#define BYTE4(qw) (((BYTE *)(&(qw)))[4])
#define BYTE5(qw) (((BYTE *)(&(qw)))[5])
#define BYTE6(qw) (((BYTE *)(&(qw)))[6])
#define BYTE7(qw) (((BYTE *)(&(qw)))[7])

/* Reference a BYTE, WORD, DWORD, or QWORD, at offset N in a structure */
#define BYTE_AT(p, n) (*(BYTE *)((BYTE *)(p)+(int)(n)))
#define WORD_AT(p, n) (*(WORD *)((BYTE *)(p)+(int)(n)))
#define DWORD_AT(p, n) (*(DWORD *)((BYTE *)(p)+(int)(n)))
#define QWORD_AT(p, n) (*(QWORD *)((BYTE *)(p)+(int)(n)))

#endif /* defined(DWORD_DEFINED) */

/*****************************************************************************\
*           First the full-fledged implementation for C++.                    *
\*****************************************************************************/

#ifdef __cplusplus

#ifndef NO_OPRINTF
#include "oprintf.h"
#endif /* !defined(NO_OPRINTF) */

#ifdef _MSC_VER
#pragma warning(disable:4505) /* Avoid warnings "unreferenced local function has been removed" */
#pragma warning(disable:4710) /* Avoid warnings "function 'QWORD::operator=' not expanded" */
#endif

#ifdef _MSDOS	/* 16-bits program */

#pragma message("Using QWORD C++ class definition.")
#define QWORD_CLASS 1

class QWORD
    {
protected:
    unsigned long dw0;
    unsigned long dw1;
public:
    /* Replace constructors by friend functions. */
    friend QWORD& _QWORD();
    friend QWORD& _QWORD(unsigned long dw);
    friend QWORD& _QWORD(unsigned long dw0, unsigned long dw1);
    friend QWORD& _QWORD(const QWORD& qw);
    /* Constructors. Only defined if QWORD_CONSTRUCTORS is pre-defined. */
#ifndef NO_QWORD_CONSTRUCTORS
    #define TEMP_QWORD(arg) QWORD(arg)
    QWORD() {}; /* Do not initialize contents. It's useless. */
    inline QWORD(int i) {dw0 = (long)i; dw1 = (i<0)?-1L:0;};
    inline QWORD(long l) {dw0 = l; dw1 = (l<0)?-1L:0;};
    inline QWORD(unsigned int u) {dw0 = u; dw1 = 0;};
    inline QWORD(unsigned long dw) {dw0 = dw; dw1 = 0;};
    inline QWORD(unsigned long lo, unsigned long hi) {dw0 = lo; dw1 = hi;};
    /* Use the default copy constructor. */
#else /* defined(NO_QWORD_CONSTRUCTORS) ==> Use _QWORD instead. */
    #define TEMP_QWORD(arg) _QWORD(arg)
#endif /* !defined(NO_QWORD_CONSTRUCTORS) */
    /* Assignment operators */
    inline QWORD& QWORD::operator=(int i) { dw0 = (long)i; dw1 = (i<0)?-1L:0; return *this; };
    inline QWORD& QWORD::operator=(long l) { dw0 = l; dw1 = (l<0)?-1L:0; return *this; };
    inline QWORD& QWORD::operator=(unsigned int u) { dw0 = u; dw1 = 0; return *this; };
    inline QWORD& QWORD::operator=(unsigned long dw) {dw0 = dw; dw1 = 0; return *this; };
    /* Arithmetic operators */
#ifndef ALL_OPERATORS_NEEDED	/* Used for experimentation with C++ classes */
#define ALL_OPERATORS_NEEDED 1	/* Default: Do define all arithmetic operators */
#endif

#if ALL_OPERATORS_NEEDED
    inline QWORD operator+(const QWORD& qw2) const { QWORD qw; qw = qw2; return qw += *this; };
    inline QWORD operator+(unsigned long dw2) const { QWORD qw; qw = dw2; return qw += *this; };
    inline QWORD operator+(unsigned int u) const { QWORD qw; qw = u; return qw += *this; };
    inline QWORD operator+(long l) const { QWORD qw; qw = l; return qw += *this; };
    inline QWORD operator+(int i) const { QWORD qw; qw = i; return qw += *this; };
#endif /* ALL_OPERATORS_NEEDED */
/*    inline QWORD& operator+=(const QWORD& qw2) {unsigned long dwLow; dwLow = dw0 + qw2.dw0; dw1 += qw2.dw1; if (dwLow < dw0) dw1 += 1; dw0 = dwLow; return *this; }; */
    QWORD& operator+=(const QWORD& qw2);
/*    inline QWORD& operator+=(unsigned long dw2) {unsigned long dwLow; dwLow = dw0 + dw2; if (dwLow < dw0) dw1 += 1; dw0 = dwLow; return *this; }; */
    QWORD& operator+=(unsigned long dw2);
    inline QWORD& operator+=(unsigned int u) { return *this += (DWORD)u; };
    inline QWORD& operator+=(long l) { QWORD qw; qw = l; return *this += qw; };
    inline QWORD& operator+=(int i) { QWORD qw; qw = i; return *this += qw; };
    
    inline QWORD& QWORD::operator++() { dw0 += 1; if (!dw0) /* Carry */ dw1 += 1; return *this; };	/* Prefix operator ++ */
    inline QWORD& QWORD::operator++(int) { dw0 += 1; if (!dw0) /* Carry */ dw1 += 1; return *this; };	/* Postfix operator ++ */
    
#if ALL_OPERATORS_NEEDED
    QWORD operator-(const QWORD& qw2) const;
    inline QWORD operator-(int i) const { QWORD qw; qw = i; return *this - qw; };
    inline QWORD operator-(unsigned int u) const { QWORD qw; qw = u; return *this - qw; };
    inline QWORD operator-(long l) const { QWORD qw; qw = l; return *this - qw; };
    inline QWORD operator-(unsigned long dw) const { QWORD qw; qw = dw; return *this - qw; };
#endif /* ALL_OPERATORS_NEEDED */
    QWORD& operator-=(const QWORD& qw2);
    inline QWORD& operator-=(int i) { QWORD qw; qw = i; return *this -= qw; };
    inline QWORD& operator-=(unsigned int u) { QWORD qw; qw = u; return *this -= qw; };
    inline QWORD& operator-=(long l) { QWORD qw; qw = l; return *this -= qw; };
    inline QWORD& operator-=(unsigned long dw)  { QWORD qw; qw = dw; return *this -= qw; };
    
    inline QWORD& QWORD::operator--() { if (!dw0) /* Carry */ dw1 -= 1; dw0 -= 1; return *this; };	/* Prefix operator -- */
    inline QWORD& QWORD::operator--(int) { if (!dw0) /* Carry */ dw1 -= 1; dw0 -= 1; return *this; };	/* Postfix operator -- */

#if ALL_OPERATORS_NEEDED
    QWORD operator*(DWORD dw2) const;
    inline QWORD operator*(int i) const { return *this * (DWORD)i; };
    inline QWORD operator*(unsigned int u) const { return *this * (DWORD)u; };
    inline QWORD operator*(long l) const { return *this * (DWORD)l; };
#endif /* ALL_OPERATORS_NEEDED */
    QWORD& operator*=(DWORD dw2);
    inline QWORD& operator*=(int i) { return *this *= (DWORD)i; };
    inline QWORD& operator*=(unsigned int u) { return *this *= (DWORD)u; };
    inline QWORD& operator*=(long l) { return *this *= (DWORD)l; };
    
#if ALL_OPERATORS_NEEDED
    QWORD operator/(DWORD dw2) const;
    inline QWORD operator/(int i) const { return *this / (DWORD)i; };
    inline QWORD operator/(unsigned int u) const { return *this / (DWORD)u; };
    inline QWORD operator/(long l) const { return *this / (DWORD)l; };
#endif /* ALL_OPERATORS_NEEDED */
    QWORD& operator/=(DWORD dw2);
    inline QWORD& operator/=(int i) { return *this /= (DWORD)i; };
    inline QWORD& operator/=(unsigned int u) { return *this /= (DWORD)u; };
    inline QWORD& operator/=(long l) { return *this /= (DWORD)l; };

    DWORD operator%(DWORD dw2) const;
    QWORD& operator%=(DWORD dw2);

    QWORD& operator<<=(int i);
    QWORD& operator>>=(int i);
#if ALL_OPERATORS_NEEDED
    QWORD operator<<(int i) const;
    QWORD operator>>(int i) const;
#endif /* ALL_OPERATORS_NEEDED */
    
    /* Comparison operators */
    int operator!(void) const;
    int operator==(const QWORD& qw2) const;
    int operator!=(const QWORD& qw2) const;
    int operator>(const QWORD& qw2) const;
    int operator>=(const QWORD& qw2) const;
    int operator<(const QWORD& qw2) const;
    int operator<=(const QWORD& qw2) const;
    /* Comparison to DWORDs operators */
    int operator==(unsigned long dw) const;
    int operator!=(unsigned long dw) const;
    int operator>(unsigned long dw) const;
    int operator>=(unsigned long dw) const;
    int operator<(unsigned long dw) const;
    int operator<=(unsigned long dw) const;
    /* Conversion operators */
    operator double() const;
    inline operator unsigned long() const { return dw0; };
    inline operator long() const { return (long)dw0; };
    inline operator unsigned short() const { return (unsigned short)dw0; };
    inline operator int() const { return (int)dw0; };
    /* Formatting functions */
    friend char *qwtox(const QWORD &qw, char *pBuf);
    friend int printfx(char *pszFormat, const QWORD &qw);
    friend int xtoqw(char *pBuf, QWORD &qw);
#ifndef NO_OPRINTF
    static int opfFormat(char *pszOut, size_t iSize, const char *pszForm, const OPFARG *popfArg);
    operator OPFARG();
#endif /* !defined(NO_OPRINTF) */
    };

#define Qword2Double(x) ((double)(x))	/* Dummy routine. For compatibility with C version.     */

#endif	/* defined(_MSDOS) */


#ifdef _WIN32	/* 32-bits or 64-bits Windows program */

#pragma message("Using QWORD WIN32 unsigned __int64 C++ definition.")
typedef unsigned __int64 QWORD;
#define QWORD_CLASS 0

#endif /* defined(_WIN32) */


#ifdef __unix__	/* Unix program */

#include <stdint.h>

/* #pragma message("Using <stdint.h> uint64_t definition.") */
typedef uint64_t QWORD;
#define QWORD_CLASS 0

#endif /* defined(__unix__) */


#if !QWORD_CLASS 

char *qwtox(const QWORD &qw, char *pBuf);
int printfx(char *pszFormat, const QWORD &qw);
int xtoqw(char *pBuf, QWORD &qw);

/* Pseudo constructors, implemented as friend functions. */
/* This is done to avoid problems with structures containing QWORDs, */
/*  which generate compilation errors if any constructor is defined. */

inline QWORD& _QWORD() { QWORD *pqw = new QWORD; return *pqw = 0; }
inline QWORD& _QWORD(unsigned long dw) { QWORD *pqw = new QWORD; return *pqw = dw; }
inline QWORD& _QWORD(unsigned long dw0, unsigned long dw1) { QWORD *pqw = new QWORD; return *pqw = ((QWORD)(DWORD)(dw0) + ((QWORD)(DWORD)(dw1) << 32)); }

inline double Qword2Double(QWORD qw)
    { return ((2*((double)(QWORD)((qw)/2)))+((int)(qw)&1)); }	/* Unsigned __qword conversion not implemented! */

#endif /* !QWORD_CLASS */

#define Qword2Dword(qw) ((DWORD)(QWORD)(qw))
#define Dword2Qword(dw) _QWORD(dw)
int qwtostr(QWORD qw, char *pszString, int iBase);
int strtoqw(const char *pszString, QWORD &qw, int iBase);

/*****************************************************************************\
*           Then the makeshift implementation for plain C.                    *
\*****************************************************************************/

#else /* !defined(cplusplus) */

/* Provide a QWORD structure for C programs. */

#ifdef _MSDOS /* 16-bits version */

#pragma message("Using QWORD MSDOS 2 * 32-bits C structure definition.")

#ifdef _H2INC	/* MASM h2inc.exe converter */
/*XLATOFF*/
#define QWORD double	/* Trick for making it 8-bytes long, without generating any conflicting definition with MASM's QWORD. */
/*XLATON*/
#else		/* MSVC 1.5 compiler */
typedef struct tagQWORD
    {
    DWORD dw0;
    DWORD dw1;
    } QWORD;
#endif

double Qword2Double(QWORD qw);
#define Qword2Dword(qw) DWORD0(qw)
QWORD Dword2Qword(DWORD dw);
#define _QWORD(dw) Dword2Qword(dw)
char *qwtox(const QWORD qw, char *pBuf);

#endif /* _MSDOS */


#ifdef _WIN32 /* 32-bits or 64-bits Windows version */

#pragma message("Using QWORD WIN32 unsigned __int64 C definition.")
typedef unsigned __int64 QWORD;

#endif /* _WIN32 */


#ifdef __unix__	/* Unix version */

#include <stdint.h>

/* #pragma message("Using <stdint.h> uint64_t definition.") */
typedef uint64_t QWORD;

#endif /* __unix__ */


#if defined(_WIN32) || defined(__unix__) /* All 32 or 64 bits compilers support 64-bits integers */

double Qword2Double(QWORD qw);
#define Qword2Dword(qw) ((DWORD)(qw))
#define Dword2Qword(dw) ((QWORD)(dw))
#define _QWORD(dw) Dword2Qword(dw)
#define qwtox(qw, pBuf)	_ui64toa(qw, pBuf, 16) /* Use stdlib.h routine _ui64toa(), in base 16. */

#endif /* 32 or 64 bits versions */

int printfx(char *pszFormat, const QWORD qw);
int xtoqw(char *pBuf, QWORD *pqw);

/*****************************************************************************\
*            End of makeshift implementation for plain C.                    *
\*****************************************************************************/

#endif /* !defined(cplusplus) */

DWORD xtodw(char *psz);

/* Useful constants defined in qword.cpp */
extern QWORD qwZero;		/* Zero */

/* Define pointers to QWORDs */
typedef QWORD FAR *LPQWORD;

#endif /* _QWORD_H_ */
