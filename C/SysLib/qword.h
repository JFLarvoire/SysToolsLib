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
*    2020-04-19 JFL Define and use consistently DWORD_DEFINED & QWORD_DEFINED.*
*    2021-11-05 JFL Make sure all definitions for Unix use stdint.h defs.     *
*		    Allow including qword.h and oprintf.h in any order.	      *
*    2021-11-07 JFL Add QWORD_WIDTH definition, and all 2014-standard widths. *
*									      *
\*****************************************************************************/

#ifndef _QWORD_H_	/* Prevent multiple inclusions */
#define _QWORD_H_

#if HAS_SYSLIB
#include "SysLib.h"	/* SysLib Library core definitions */
#endif

/* Integer width definitions, like the 2014 standard ones in limits.h */
#define BYTE_WIDTH  8
#define WORD_WIDTH  16
#define DWORD_WIDTH 32
#define QWORD_WIDTH 64

/* Standard C integer types widths definitions from TS 18661-1:2014 */
/* This allows comparing QWORDs widths to that of standard types, without
   using the sizeof() keyword, which is not usable by the preprocessor */
#define __STDC_WANT_IEC_60559_BFP_EXT__ /* We want *_WIDTH macros from TS 18661-1:2014 */
#include <limits.h>
#ifndef ULONG_WIDTH /* If we did not get it, try recreating it using older macros, more likely to be defined */
#  if ULONG_MAX == 4294967295UL
#    define ULONG_WIDTH 32 /* The common case in Windows, and in Unix x86 systems */
#  elif ULONG_MAX == 18446744073709551615UL
#    define ULONG_WIDTH 64 /* The common case in Unix x86_64 systems */
#  else
#    define ULONG_WIDTH 0  /* Unusual case. Can this actually happen? */
#    pragma warning("qword.h: Unknown ULONG width. Please check and correct")
#  endif
/*
#define QWORD_STRINGIZE(x) #x // Convert the raw argument to a string
#define QWORD_VALUEIZE(x) QWORD_STRINGIZE(x) // Substitute the argument, then convert its value to a string
#  pragma message("qword.h: ULONG_WIDTH = " QWORD_VALUEIZE(ULONG_WIDTH))
#else
#  pragma message("limits.h: ULONG_WIDTH = " QWORD_VALUEIZE(ULONG_WIDTH))
*/
#endif

/* There are 3 possible types of QWORD definitions */
#define QWORD_UINT64 0	/* Native compiler type */
#define QWORD_CLASS  1	/* A C++ class */
#define QWORD_STRUCT 2	/* A C structure */

/* Define x86 register types */
#ifndef DWORD_DEFINED
#define DWORD_DEFINED

#if defined(_MSDOS) || defined(_WIN32)
typedef unsigned char   BYTE;   /*  8-bits unsigned integer */
typedef unsigned short	WORD;	/* 16-bits unsigned integer */
typedef unsigned long	DWORD;	/* 32-bits unsigned integer */
#else /* Ex: defined(__unix__) */
#include <stdint.h>
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
#endif

/* Define the FAR qualifier for DOS, but not for Windows */
#ifdef _H2INC
  #define _MSDOS /* The h2inc.exe tool is for generating DOS code */
#endif
#ifndef FAR
  #ifdef _MSDOS
    #define FAR far
  #elif defined(_WIN32)
    #define FAR
  #else /* Anything else, for example Unix, has no notion of NEAR or FAR */
    #define FAR
  #endif
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

#ifdef _MSC_VER
#pragma warning(disable:4505) /* Avoid warnings "unreferenced local function has been removed" */
#pragma warning(disable:4710) /* Avoid warnings "function 'QWORD::operator=' not expanded" */
#endif

/* -------------------- C++ class definition for MS-DOS -------------------- */

#ifdef _MSDOS	/* 16-bits program */

#pragma message("Using QWORD C++ class definition.")
#define QWORD_DEFINED QWORD_CLASS

#if HAS_SYSLIB && !defined(NO_OPRINTF)
#include "oprintf.h"	/* Must be included after defining QWORD_DEFINED */
#endif /* HAS_SYSLIB && !defined(NO_OPRINTF) */

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

/* ---------------- unsigned __int64 definition for Windows ---------------- */

#ifdef _WIN32	/* 32-bits or 64-bits Windows program */

#if 1

#pragma message("Using QWORD WIN32 unsigned __int64 C++ definition.")
#define QWORD_DEFINED QWORD_UINT64

typedef unsigned __int64 QWORD;

#else /* Unsuccessful attempt at defining QWORD as a class, sharing the Unix implementation */
      /* The problem is that the CPP programs compilation fails, saying it can't choose between multiple operator overloads */
#if HAS_MSVCLIBX
#include <stdint.h>
#else /* !HAS_MSVCLIBX */
typedef __int8            int8_t;
typedef __int16           int16_t;
typedef __int32           int32_t;
typedef __int64           int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;
#endif /* HAS_MSVCLIBX */

/* Then use the common [#if !defined(QWORD_DEFINED)] section below to define the QWORD class */

#endif /* 0 */

#endif /* defined(_WIN32) */

/* --------------------- uint64_t definition for Unix ---------------------- */

#if !defined(_MSDOS) && !defined(_WIN32) /* Anything else, for example Unix programs */

#include <stdint.h>

#if 0 /* Unsuccessful attempt at using a simple uint64_t like in WIN32 */
      /* The problem is that for the oprintf() support, this forces adding
         an OPFDECLARE block in class OPFARG definition in oprintf.h, and an
         OPFMANAGE-style implementation implementation in oprintf.cpp, and
         there were conflicts with the unsigned long and unsigned long long
         definitions there that I never managed to resolve */
/* #pragma message("Using <stdint.h> uint64_t definition.") */
#define QWORD_DEFINED QWORD_UINT64

typedef uint64_t QWORD;

#endif /* 0 */

#endif /* !defined(_MSDOS) && !defined(_WIN32) */

/* ----------------------- uint64_t class definition ----------------------- */

#if !defined(QWORD_DEFINED)

/* #pragma message("Using <stdint.h> uint64_t based class.") */
#define QWORD_DEFINED QWORD_CLASS

#if HAS_SYSLIB && !defined(NO_OPRINTF)
#include "oprintf.h"	/* Must be included after defining QWORD_DEFINED */
#endif /* HAS_SYSLIB && !defined(NO_OPRINTF) */

class QWORD {
protected:
    uint64_t qw;
public:
    /* Replace constructors by friend functions. */
    friend QWORD& _QWORD();
    friend QWORD& _QWORD(DWORD dw);
    friend QWORD& _QWORD(DWORD dw0, DWORD dw1);
    friend QWORD& _QWORD(uint64_t qw2);
    friend QWORD& _QWORD(const QWORD& qw2);
    /* Constructors. Only defined if QWORD_CONSTRUCTORS is pre-defined. */
#ifndef NO_QWORD_CONSTRUCTORS
    #define TEMP_QWORD(arg) QWORD(arg)
    inline QWORD() {}; /* Do not initialize contents. It's useless. */
    inline QWORD(int16_t  v2) {qw = v2;};
    inline QWORD(int32_t  v2) {qw = v2;};
    inline QWORD(int64_t  v2) {qw = v2;};
    inline QWORD(uint16_t v2) {qw = v2;};
    inline QWORD(DWORD v2) {qw = v2;};
    inline QWORD(uint64_t v2) {qw = v2;};
    inline QWORD(DWORD lo, DWORD hi) {qw = hi; qw <<= 32; qw |= lo;};
    /* Use the default copy constructor. */
#else /* defined(NO_QWORD_CONSTRUCTORS) ==> Use _QWORD instead. */
    #define TEMP_QWORD(arg) _QWORD(arg)
#endif /* !defined(NO_QWORD_CONSTRUCTORS) */
    /* Assignment operators */
    inline QWORD& operator=(int16_t  v2) { qw = v2; return *this; };
    inline QWORD& operator=(int32_t  v2) { qw = v2; return *this; };
    inline QWORD& operator=(int64_t  v2) { qw = v2; return *this; };
    inline QWORD& operator=(uint16_t v2) { qw = v2; return *this; };
    inline QWORD& operator=(DWORD v2) { qw = v2; return *this; };
    inline QWORD& operator=(uint64_t v2) { qw = v2; return *this; };
    /* Arithmetic operators */
#ifndef ALL_OPERATORS_NEEDED	/* Used for experimentation with C++ classes */
#define ALL_OPERATORS_NEEDED 1	/* Default: Do define all arithmetic operators */
#endif

#if ALL_OPERATORS_NEEDED
    inline QWORD operator+(const QWORD& qw2) const { QWORD qw3 = qw2; return qw3 += *this; };
    inline QWORD operator+(uint64_t v2) const { QWORD qw3 = v2; return qw3 += *this; };
    inline QWORD operator+(DWORD    v2) const { QWORD qw3 = v2; return qw3 += *this; };
    inline QWORD operator+(uint16_t v2) const { QWORD qw3 = v2; return qw3 += *this; };
    inline QWORD operator+(int64_t  v2) const { QWORD qw3 = v2; return qw3 += *this; };
    inline QWORD operator+(int32_t  v2) const { QWORD qw3 = v2; return qw3 += *this; };
    inline QWORD operator+(int16_t  v2) const { QWORD qw3 = v2; return qw3 += *this; };
#endif /* ALL_OPERATORS_NEEDED */
    inline QWORD& operator+=(const QWORD& qw2) { qw += qw2.qw; return *this; };
    inline QWORD& operator+=(uint64_t v2) { qw += v2; return *this; };
    inline QWORD& operator+=(DWORD    v2) { qw += v2; return *this; };
    inline QWORD& operator+=(uint16_t v2) { qw += v2; return *this; };
    inline QWORD& operator+=(int64_t  v2) { qw += v2; return *this; };
    inline QWORD& operator+=(int32_t  v2) { qw += v2; return *this; };
    inline QWORD& operator+=(int16_t  v2) { qw += v2; return *this; };
    
    inline QWORD& operator++() { qw += 1; return *this; };	/* Prefix operator ++ */
    inline QWORD& operator++(int) { qw += 1; return *this; };	/* Postfix operator ++ */
    
#if ALL_OPERATORS_NEEDED
    inline QWORD operator-(const QWORD& qw2) const { QWORD qw3 = *this; return qw3 -= qw2; };
    inline QWORD operator-(int16_t  v2) const { QWORD qw3 = *this; return qw3 -= v2; };
    inline QWORD operator-(uint16_t v2) const { QWORD qw3 = *this; return qw3 -= v2; };
    inline QWORD operator-(int32_t  v2) const { QWORD qw3 = *this; return qw3 -= v2; };
    inline QWORD operator-(DWORD    v2) const { QWORD qw3 = *this; return qw3 -= v2; };
    inline QWORD operator-(int64_t  v2) const { QWORD qw3 = *this; return qw3 -= v2; };
    inline QWORD operator-(uint64_t v2) const { QWORD qw3 = *this; return qw3 -= v2; };
#endif /* ALL_OPERATORS_NEEDED */
    inline QWORD& operator-=(const QWORD& qw2) { qw -= qw2.qw; return *this; }
    inline QWORD& operator-=(int16_t  v2) { qw -= v2; return *this; };
    inline QWORD& operator-=(uint16_t v2) { qw -= v2; return *this; };
    inline QWORD& operator-=(int32_t  v2) { qw -= v2; return *this; };
    inline QWORD& operator-=(DWORD    v2) { qw -= v2; return *this; };
    inline QWORD& operator-=(int64_t  v2) { qw -= v2; return *this; };
    inline QWORD& operator-=(uint64_t v2) { qw -= v2; return *this; };
    
    inline QWORD& operator--() { qw -= 1; return *this; };	/* Prefix operator -- */
    inline QWORD& operator--(int) { qw -= 1; return *this; };	/* Postfix operator -- */

#if ALL_OPERATORS_NEEDED
    inline QWORD operator*(int16_t  v2) const { QWORD qw3 = *this; return qw3 *= v2; };
    inline QWORD operator*(uint16_t v2) const { QWORD qw3 = *this; return qw3 *= v2; };
    inline QWORD operator*(int32_t  v2) const { QWORD qw3 = *this; return qw3 *= v2; };
    inline QWORD operator*(DWORD    v2) const { QWORD qw3 = *this; return qw3 *= v2; };
#endif /* ALL_OPERATORS_NEEDED */
    inline QWORD& operator*=(int16_t  v2) { qw *= v2; return *this; };
    inline QWORD& operator*=(uint16_t v2) { qw *= v2; return *this; };
    inline QWORD& operator*=(int32_t  v2) { qw *= v2; return *this; };
    inline QWORD& operator*=(DWORD    v2) { qw *= v2; return *this; };
    
#if ALL_OPERATORS_NEEDED
    inline QWORD operator/(int16_t  v2) const { QWORD qw3 = *this; return qw3 /= v2; };
    inline QWORD operator/(uint16_t v2) const { QWORD qw3 = *this; return qw3 /= v2; };
    inline QWORD operator/(int32_t  v2) const { QWORD qw3 = *this; return qw3 /= v2; };
    inline QWORD operator/(DWORD    v2) const { QWORD qw3 = *this; return qw3 /= v2; };
#endif /* ALL_OPERATORS_NEEDED */
    inline QWORD& operator/=(int16_t  v2) { qw /= v2; return *this; };
    inline QWORD& operator/=(uint16_t v2) { qw /= v2; return *this; };
    inline QWORD& operator/=(int32_t  v2) { qw /= v2; return *this; };
    inline QWORD& operator/=(DWORD    v2) { qw /= v2; return *this; };

    inline DWORD operator%(DWORD dw) const { return (DWORD)(qw % dw); };
    inline QWORD& operator%=(DWORD dw) { qw %= dw; return *this; };

    inline QWORD& operator<<=(int i) { qw <<= i; return *this; };
    inline QWORD& operator>>=(int i) { qw >>= i; return *this; };
#if ALL_OPERATORS_NEEDED
    QWORD operator<<(int i) const { QWORD qw3 = *this; return qw3 <<= i; };
    QWORD operator>>(int i) const { QWORD qw3 = *this; return qw3 >>= i; };
#endif /* ALL_OPERATORS_NEEDED */
    
    /* Comparison operators */
    inline int operator!(void) const { return qw != 0; };
    
    inline int operator==(const QWORD& qw2) const { return qw == qw2.qw; };
    inline int operator==(uint64_t v2) const { return qw == v2; };
    inline int operator==(DWORD v2) const { return qw == v2; };
    inline int operator==(uint16_t v2) const { return qw == v2; };

    inline int operator!=(const QWORD& qw2) const { return qw != qw2.qw; };
    inline int operator!=(uint64_t v2) const { return qw != v2; };
    inline int operator!=(DWORD v2) const { return qw != v2; };
    inline int operator!=(uint16_t v2) const { return qw != v2; };

    inline int operator>(const QWORD& qw2) const { return qw > qw2.qw; };
    inline int operator>(uint64_t v2) const { return qw > v2; };
    inline int operator>(DWORD v2) const { return qw > v2; };
    inline int operator>(uint16_t v2) const { return qw > v2; };

    inline int operator>=(const QWORD& qw2) const { return qw >= qw2.qw; };
    inline int operator>=(uint64_t v2) const { return qw >= v2; };
    inline int operator>=(DWORD v2) const { return qw >= v2; };
    inline int operator>=(uint16_t v2) const { return qw >= v2; };
    
    inline int operator<(const QWORD& qw2) const { return qw < qw2.qw; };
    inline int operator<(uint64_t v2) const { return qw < v2; };
    inline int operator<(DWORD v2) const { return qw < v2; };
    inline int operator<(uint16_t v2) const { return qw < v2; };
    
    inline int operator<=(const QWORD& qw2) const { return qw <= qw2.qw; };
    inline int operator<=(uint64_t v2) const { return qw <= v2; };
    inline int operator<=(DWORD v2) const { return qw <= v2; };
    inline int operator<=(uint16_t v2) const { return qw <= v2; };

    /* Conversion operators */
    inline operator double() const { return (double)qw; };
    inline operator unsigned long() const { return (unsigned long)qw; };
    inline operator long() const { return (long)qw; };
    inline operator unsigned int() const { return (unsigned int)qw; };
    inline operator int() const { return (int)qw; };
    inline operator unsigned short() const { return (unsigned short)qw; };
    inline operator short() const { return (short)qw; };
#ifdef ULLONG_MAX
    inline operator unsigned long long() const { return (unsigned long long)qw; };
    inline operator long long() const { return (long long)qw; };
#endif

    /* Formatting functions */
    friend char *qwtox(const QWORD &qw, char *pBuf);
    friend int printfx(char *pszFormat, const QWORD &qw);
    friend int xtoqw(char *pBuf, QWORD &qw);
#ifndef NO_OPRINTF
    static int opfFormat(char *pszOut, size_t iSize, const char *pszForm, const OPFARG *popfArg);
    operator OPFARG();
#endif /* !defined(NO_OPRINTF) */
    };

inline QWORD& _QWORD() { QWORD *pqw = new QWORD(0); return *pqw; }
inline QWORD& _QWORD(DWORD dw) { QWORD *pqw = new QWORD(dw); return *pqw; }
inline QWORD& _QWORD(DWORD dw0, DWORD dw1) { QWORD *pqw = new QWORD(dw0, dw1); return *pqw; }
inline QWORD& _QWORD(uint64_t qw2) { QWORD *pqw = new QWORD(qw2); return *pqw; }
inline QWORD& _QWORD(const QWORD& qw2) { QWORD *pqw = new QWORD(qw2); return *pqw; }

#endif /* !defined(QWORD_DEFINED) */

/* ----------------- Helper functions for Windows and Unix ----------------- */

#if QWORD_DEFINED != QWORD_CLASS

char *qwtox(const QWORD &qw, char *pBuf);
int printfx(char *pszFormat, const QWORD &qw);
int xtoqw(char *pBuf, QWORD &qw);

/* Pseudo constructors, implemented as friend functions. */
/* This is done to avoid problems with structures containing QWORDs, */
/*  which generate compilation errors if any constructor is defined. */

inline QWORD& _QWORD() { QWORD *pqw = new QWORD; return *pqw = 0; }
inline QWORD& _QWORD(DWORD dw) { QWORD *pqw = new QWORD; return *pqw = dw; }
inline QWORD& _QWORD(DWORD dw0, DWORD dw1) { QWORD *pqw = new QWORD; return *pqw = ((QWORD)(DWORD)(dw0) + ((QWORD)(DWORD)(dw1) << 32)); }

inline double Qword2Double(QWORD qw)
    { return ((2*((double)(QWORD)((qw)/2)))+((int)(qw)&1)); }	/* Unsigned __qword conversion not implemented! */

#endif /* QWORD_DEFINED != QWORD_CLASS */

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

#define QWORD_DEFINED QWORD_STRUCT

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

#define QWORD_DEFINED QWORD_UINT64

#endif /* _WIN32 */


#ifndef QWORD_DEFINED	/* Anything else, for example Unix programs */

#include <stdint.h>

/* #pragma message("Using <stdint.h> uint64_t definition.") */
typedef uint64_t QWORD;
#define QWORD_DEFINED QWORD_UINT64

#endif /* !defined(QWORD_DEFINED) */


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

#endif /* !defined(__cplusplus) */

DWORD xtodw(char *psz);

/* Useful constants defined in qword.cpp */
extern QWORD qwZero;		/* Zero */

/* Define pointers to QWORDs */
typedef QWORD FAR *LPQWORD;

#endif /* _QWORD_H_ */
