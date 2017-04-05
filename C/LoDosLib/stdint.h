/*****************************************************************************\
*                                                                             *
*   Name            stdint.h                                                  *
*                                                                             *
*   Description     C99 standard types                                        *
*                                                                             *
*   Notes           The 64-bits types are not supported by MSVC 1.x           *
*                   In C++, adds support for 64-bits via "qword.h".           *
*                   Disable if not desired by defining NO_QWORD.              *
*                                                                             *
*   History                                                                   *
*    2009/02/23 JFL Created this file                                         *
*                                                                             *
*      (c) Copyright 2009-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#if !defined(_MSC_VER) || _MSC_VER >= 1000
#error "Use this header only with Microsoft Visual C++ 16-bits compilers!"
#endif /* _MSC_VER */

#ifndef _STDINT_H_
#define _STDINT_H_

#include <limits.h>

#if defined(__cplusplus) && !defined(NO_QWORD)
#define NO_OPRINTF
#include "qword.h"
#define LLONG_MIN (_QWORD(0x00000001, 0x80000000))
#define LLONG_MAX (_QWORD(0xFFFFFFFF, 0x7FFFFFFF))
#define ULLONG_MAX (_QWORD(0xFFFFFFFF, 0xFFFFFFFF))
#endif

/* 7.18.1 Integer types */

/* 7.18.1.1 Exact-width integer types */
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed long       int32_t;
/* typedef signed long long  int64_t; */
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned long     uint32_t;
/* typedef unsigned long long uint64_t; */
#ifdef _QWORD_H_
typedef QWORD uint64_t;
#define _UINT64_T_DEFINED
#endif

/* 7.18.1.2 Minimum-width integer types */
typedef int8_t    int_least8_t;
typedef int16_t   int_least16_t;
typedef int32_t   int_least32_t;
/* typedef int64_t   int_least64_t; */
typedef uint8_t   uint_least8_t;
typedef uint16_t  uint_least16_t;
typedef uint32_t  uint_least32_t;
/* typedef uint64_t  uint_least64_t; */
#ifdef _UINT64_T_DEFINED
typedef uint64_t  uint_least64_t;
#endif

/* 7.18.1.3 Fastest minimum-width integer types */
typedef int8_t    int_fast8_t;
typedef int16_t   int_fast16_t;
typedef int32_t   int_fast32_t;
/* typedef int64_t   int_fast64_t; */
typedef uint8_t   uint_fast8_t;
typedef uint16_t  uint_fast16_t;
typedef uint32_t  uint_fast32_t;
/* typedef uint64_t  uint_fast64_t; */
#ifdef _UINT64_T_DEFINED
typedef uint64_t  uint_fast64_t;
#endif

/* 7.18.1.4 Integer types capable of holding object pointers */
#if defined(_M_I86TM) || defined(_M_I86SM) || defined(_M_I86MM)
/* Tiny, small and medium memory models have 16-bits data pointers */
#define INTPRTR_BIT 16
   typedef int16_t  intptr_t;
   typedef uint16_t uintptr_t;
#else
/* Compact, large and huge memory models have 32-bits data pointers */
#define INTPRTR_BIT 32
   typedef int32_t  intptr_t;
   typedef uint32_t uintptr_t;
#endif

/* 7.18.1.5 Greatest-width integer types */
typedef int32_t   intmax_t;
#ifdef _UINT64_T_DEFINED
typedef uint64_t  uintmax_t;
#else
typedef uint32_t  uintmax_t;
#endif


/* 7.18.2 Limits of specified-width integer types */

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

/* 7.18.2.1 Limits of exact-width integer types */
#define INT8_MIN     ((int8_t)SCHAR_MIN)
#define INT8_MAX     SCHAR_MAX
#define INT16_MIN    ((int16_t)SHRT_MIN)
#define INT16_MAX    SHRT_MAX
#define INT32_MIN    ((int32_t)LONG_MIN)
#define INT32_MAX    LONG_MAX
/* #define INT64_MIN    ((int64_t)LLONG_MIN) */
/* #define INT64_MAX    LLONG_MAX */
#define UINT8_MAX    UCHAR_MAX
#define UINT16_MAX   USHRT_MAX
#define UINT32_MAX   ULONG_MAX
/* #define UINT64_MAX   ULLONG_MAX */
#ifdef _UINT64_T_DEFINED
#define UINT64_MAX   ULLONG_MAX
#endif

/* 7.18.2.2 Limits of minimum-width integer types */
#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST8_MAX    INT8_MAX
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST16_MAX   INT16_MAX
#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST32_MAX   INT32_MAX
/* #define INT_LEAST64_MIN   INT64_MIN */
/* #define INT_LEAST64_MAX   INT64_MAX */
#define UINT_LEAST8_MAX   UINT8_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define UINT_LEAST32_MAX  UINT32_MAX
/* #define UINT_LEAST64_MAX  UINT64_MAX */
#ifdef _UINT64_T_DEFINED
#define UINT_LEAST64_MAX  UINT64_MAX
#endif

/* 7.18.2.3 Limits of fastest minimum-width integer types */
#define INT_FAST8_MIN    INT8_MIN
#define INT_FAST8_MAX    INT8_MAX
#define INT_FAST16_MIN   INT16_MIN
#define INT_FAST16_MAX   INT16_MAX
#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST32_MAX   INT32_MAX
/* #define INT_FAST64_MIN   INT64_MIN */
/* #define INT_FAST64_MAX   INT64_MAX */
#define UINT_FAST8_MAX   UINT8_MAX
#define UINT_FAST16_MAX  UINT16_MAX
#define UINT_FAST32_MAX  UINT32_MAX
/* #define UINT_FAST64_MAX  UINT64_MAX */
#ifdef _UINT64_T_DEFINED
#define UINT_FAST64_MAX  UINT64_MAX
#endif

/* 7.18.2.4 Limits of integer types capable of holding object pointers */
#if INTPRTR_BIT == 16
#define INTPTR_MIN   INT16_MIN
#define INTPTR_MAX   INT16_MAX
#define UINTPTR_MAX  UINT16_MAX
#else
#define INTPTR_MIN   INT32_MIN
#define INTPTR_MAX   INT32_MAX
#define UINTPTR_MAX  UINT32_MAX
#endif

/* 7.18.2.5 Limits of greatest-width integer types */
#define INTMAX_MIN   INT32_MIN
#define INTMAX_MAX   INT32_MAX
#ifdef _UINT64_T_DEFINED
#define UINTMAX_MAX  UINT64_MAX
#else
#define UINTMAX_MAX  UINT32_MAX
#endif

/* 7.18.3 Limits of other integer types */

#if INTPRTR_BIT == 16
#define PTRDIFF_MIN  INT16_MIN
#define PTRDIFF_MAX  INT16_MAX
#else
#define PTRDIFF_MIN  INT32_MIN
#define PTRDIFF_MAX  INT32_MAX
#endif

#define SIG_ATOMIC_MIN  INT_MIN
#define SIG_ATOMIC_MAX  INT_MAX

#ifndef SIZE_MAX
#if INTPRTR_BIT == 16
#define SIZE_MAX  UINT16_MAX
#else
#define SIZE_MAX  UINT32_MAX
#endif
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN  0
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX  UINT16_MAX
#endif

#define WINT_MIN  0
#define WINT_MAX  UINT16_MAX

#endif /* __STDC_LIMIT_MACROS */


/* 7.18.4 Limits of other integer types */

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)

/* 7.18.4.1 Macros for minimum-width integer constants */

#define INT8_C(val)  val
#define INT16_C(val) val
#define INT32_C(val) val##L
/* #define INT64_C(val) val##LL */

#define UINT8_C(val)  val
#define UINT16_C(val) val
#define UINT32_C(val) val##UL
/* #define UINT64_C(val) val##ULL */
#ifdef _UINT64_T_DEFINED
#ifdef _QWORD_H_
#define UINT64_C(val) _QWORD(val)
#else
#define UINT64_C(val) val##ULL
#endif
#endif

/* 7.18.4.2 Macros for greatest-width integer constants */
#define INTMAX_C   INT32_C
#ifdef _UINT64_T_DEFINED
#define UINTMAX_C  UINT64_C
#else
#define UINTMAX_C  UINT32_C
#endif

#endif /* __STDC_CONSTANT_MACROS */

#endif /* _STDINT_H_ */

