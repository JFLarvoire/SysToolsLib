/*****************************************************************************\
*                                                                             *
*   Name            inttypes.h                                                *
*                                                                             *
*   Description     C99 standard int routines and format specifiers           *
*                                                                             *
*   Notes           The 64-bits types are not supported by MSVC 1.x           *
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

#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#include <stdint.h>

/* 7.8 Format conversion of integer types */

typedef struct {
   intmax_t quot;
   intmax_t rem;
} imaxdiv_t;

/* 7.8.1 Macros for format specifiers */

#if !defined(__cplusplus) || defined(__STDC_FORMAT_MACROS)

/* The fprintf macros for signed integers are: */
#define PRId8       "d"
#define PRIi8       "d"
#define PRIdLEAST8  "d"
#define PRIiLEAST8  "d"
#define PRIdFAST8   "d"
#define PRIiFAST8   "d"

#define PRId16       "d"
#define PRIi16       "d"
#define PRIdLEAST16  "d"
#define PRIiLEAST16  "d"
#define PRIdFAST16   "d"
#define PRIiFAST16   "d"

#define PRId32       "ld"
#define PRIi32       "ld"
#define PRIdLEAST32  "ld"
#define PRIiLEAST32  "ld"
#define PRIdFAST32   "ld"
#define PRIiFAST32   "ld"

#if 0
#define PRId64       "lld"
#define PRIi64       "lld"
#define PRIdLEAST64  "lld"
#define PRIiLEAST64  "lld"
#define PRIdFAST64   "lld"
#define PRIiFAST64   "lld"
#endif

#define PRIdMAX     PRId32
#define PRIiMAX     PRIi32

#if INTPRTR_BIT == 16
#define PRIdPTR     PRId16
#define PRIiPTR     PRIi16
#else
#define PRIdPTR     PRId32
#define PRIiPTR     PRIi32
#endif

/* The fprintf macros for unsigned integers are: */
#define PRIo8       "o"
#define PRIu8       "u"
#define PRIx8       "x"
#define PRIX8       "X"
#define PRIoLEAST8  "o"
#define PRIuLEAST8  "u"
#define PRIxLEAST8  "x"
#define PRIXLEAST8  "X"
#define PRIoFAST8   "o"
#define PRIuFAST8   "u"
#define PRIxFAST8   "x"
#define PRIXFAST8   "X"

#define PRIo16       "o"
#define PRIu16       "u"
#define PRIx16       "x"
#define PRIX16       "X"
#define PRIoLEAST16  "o"
#define PRIuLEAST16  "u"
#define PRIxLEAST16  "x"
#define PRIXLEAST16  "X"
#define PRIoFAST16   "o"
#define PRIuFAST16   "u"
#define PRIxFAST16   "x"
#define PRIXFAST16   "X"

#define PRIo32       "lo"
#define PRIu32       "lu"
#define PRIx32       "lx"
#define PRIX32       "lX"
#define PRIoLEAST32  "lo"
#define PRIuLEAST32  "lu"
#define PRIxLEAST32  "lx"
#define PRIXLEAST32  "lX"
#define PRIoFAST32   "lo"
#define PRIuFAST32   "lu"
#define PRIxFAST32   "lx"
#define PRIXFAST32   "lX"

#if 0
#define PRIo64       "llo"
#define PRIu64       "llu"
#define PRIx64       "llx"
#define PRIX64       "llX"
#define PRIoLEAST64  "llo"
#define PRIuLEAST64  "llu"
#define PRIxLEAST64  "llx"
#define PRIXLEAST64  "llX"
#define PRIoFAST64   "llo"
#define PRIuFAST64   "llu"
#define PRIxFAST64   "llx"
#define PRIXFAST64   "llX"
#endif

#define PRIoMAX     PRIo32
#define PRIuMAX     PRIu32
#define PRIxMAX     PRIx32
#define PRIXMAX     PRIX32

#if INTPRTR_BIT == 16
#define PRIoPTR     PRIo16
#define PRIuPTR     PRIu16
#define PRIxPTR     PRIx16
#define PRIXPTR     PRIX16
#else
#define PRIoPTR     PRIo32
#define PRIuPTR     PRIu32
#define PRIxPTR     PRIx32
#define PRIXPTR     PRIX32
#endif

/* The fscanf macros for signed integers are: */
#define SCNd8       "d"
#define SCNi8       "d"
#define SCNdLEAST8  "d"
#define SCNiLEAST8  "d"
#define SCNdFAST8   "d"
#define SCNiFAST8   "d"

#define SCNd16       "d"
#define SCNi16       "d"
#define SCNdLEAST16  "d"
#define SCNiLEAST16  "d"
#define SCNdFAST16   "d"
#define SCNiFAST16   "d"

#define SCNd32       "ld"
#define SCNi32       "ld"
#define SCNdLEAST32  "ld"
#define SCNiLEAST32  "ld"
#define SCNdFAST32   "ld"
#define SCNiFAST32   "ld"

#if 0
#define SCNd64       "lld"
#define SCNi64       "lld"
#define SCNdLEAST64  "lld"
#define SCNiLEAST64  "lld"
#define SCNdFAST64   "lld"
#define SCNiFAST64   "lld"
#endif

#define SCNdMAX     SCNd32
#define SCNiMAX     SCNi32

#if INTPRTR_BIT == 16
#define SCNdPTR     SCNd16
#define SCNdiTR     SCNi16
#else
#define SCNdPTR     SCNd32
#define SCNdiTR     SCNi32
#endif

/* The fscanf macros for unsigned integers are: */
#define SCNo8       "o"
#define SCNu8       "u"
#define SCNx8       "x"
#define SCNX8       "X"
#define SCNoLEAST8  "o"
#define SCNuLEAST8  "u"
#define SCNxLEAST8  "x"
#define SCNXLEAST8  "X"
#define SCNoFAST8   "o"
#define SCNuFAST8   "u"
#define SCNxFAST8   "x"
#define SCNXFAST8   "X"

#define SCNo16       "o"
#define SCNu16       "u"
#define SCNx16       "x"
#define SCNX16       "X"
#define SCNoLEAST16  "o"
#define SCNuLEAST16  "u"
#define SCNxLEAST16  "x"
#define SCNXLEAST16  "X"
#define SCNoFAST16   "o"
#define SCNuFAST16   "u"
#define SCNxFAST16   "x"
#define SCNXFAST16   "X"

#define SCNo32       "lo"
#define SCNu32       "lu"
#define SCNx32       "lx"
#define SCNX32       "lX"
#define SCNoLEAST32  "lo"
#define SCNuLEAST32  "lu"
#define SCNxLEAST32  "lx"
#define SCNXLEAST32  "lX"
#define SCNoFAST32   "lo"
#define SCNuFAST32   "lu"
#define SCNxFAST32   "lx"
#define SCNXFAST32   "lX"

#if 0
#define SCNo64       "llo"
#define SCNu64       "llu"
#define SCNx64       "llx"
#define SCNX64       "llX"
#define SCNoLEAST64  "llo"
#define SCNuLEAST64  "llu"
#define SCNxLEAST64  "llx"
#define SCNXLEAST64  "llX"
#define SCNoFAST64   "llo"
#define SCNuFAST64   "llu"
#define SCNxFAST64   "llx"
#define SCNXFAST64   "llX"
#endif

#define SCNoMAX     SCNo32
#define SCNuMAX     SCNu32
#define SCNxMAX     SCNx32
#define SCNXMAX     SCNX32

#if INTPRTR_BIT == 16
#define SCNoPTR     SCNo16
#define SCNuPTR     SCNu16
#define SCNxPTR     SCNx16
#define SCNXPTR     SCNX16
#else
#define SCNoPTR     SCNo32
#define SCNuPTR     SCNu32
#define SCNxPTR     SCNx32
#define SCNXPTR     SCNX32
#endif

#endif /* __STDC_FORMAT_MACROS */

/* 7.8.2 Functions for greatest-width integer types */

/* 7.8.2.1 The imaxabs function */
#define imaxabs _abs32

/* 7.8.2.2 The imaxdiv function */

/* This is modified version of div() function from Microsoft's div.c found
   in %MSVC.NET%\crt\src\div.c */
#ifdef __STDC_IMAXDIV
inline imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) {
  imaxdiv_t result;

  result.quot = numer / denom;
  result.rem = numer % denom;

  if (numer < 0 && result.rem > 0) {
    /* did division wrong; must fix up */
    ++result.quot;
    result.rem -= denom;
  }

  return result;
}
#endif

/* 7.8.2.3 The strtoimax and strtoumax functions */
#define strtoimax strtol
#ifdef _UINT64_T_DEFINED
#ifdef _QWORD_H_
inline uint64_t strtoull(const char *pszString, char **ppszEnd, int iBase) {
  QWORD qw;
  int n = strtoqw(pszString, qw, iBase);
  if (ppszEnd) {
    *ppszEnd = (char *)(pszString + n);
  }
  return qw;
}
#endif
#define strtoumax strtoull
#else
#define strtoumax strtoul
#endif

/* 7.8.2.4 The wcstoimax and wcstoumax functions */
// #define wcstoimax _wcstolong
// #define wcstoumax _wcstoulong

#endif /* _INTTYPES_H_ */

