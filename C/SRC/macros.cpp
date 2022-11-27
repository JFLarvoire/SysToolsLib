/*****************************************************************************\
*                                                                             *
*   File name	    macros.c				         	      *
*									      *
*   Description     Display common C predefined macros & limits definitions   *
*									      *
*   Notes	    gcc can display all its predefined macros this way:       *
*		    echo . | gcc -dM -E - | sort -d -f			      *
*		    							      *
*		    Likewise, many Unix C/C++ compilers can display theirs:   *
*		    cc -dM -E -x c /dev/null | sort -d -f		      *
*		    or							      *
*		    cc -dM -E -x c++ /dev/null | sort -d -f		      *
*		    							      *
*		    Microsoft Visual C++ has a similar ability since v16.8:   *
*		    echo.>empty.c					      *
*		    cl /nologo /EP /Zc:preprocessor /PD /TC empty.c | sort    *
*		    or							      *
*		    cl /nologo /EP /Zc:preprocessor /PD /TP empty.cpp	      *
*		    							      *
*		    There's no equivalent feature in older MSVC versions.     *
*		    Also we're not 100% sure that the above commands report   *
*		    all the macros that are actually defined. Hence this pgm. *      
*		    							      *
*		    Microsoft C macros references:			      *
*		    https://msdn.microsoft.com/en-us/library/b0084kay.aspx    *
*		    							      *
*   History								      *
*    2005-05-10 JFL Created this file.					      *
*    2012-02-02 JFL Added tons of definitions from GCC.			      *
*    2014-04-22 JFL Removed an annoying Visual C++ warning.		      *
*                   Fixed the command-line argument parsing.		      *
*    2014-04-22 JFL Added processor types _M_X64, _M_ARM, _M_ARM64.	      *
*    2022-10-17 JFL Output valid #define directives, to match Unix compilers  *
*		    (and now MSVC 16.8+) built-in abilities.                  *
*		    Added a help screen, and a -V (get version) option.	      *
*		    Fixed warnings in MS-DOS and MacOS LLVM builds.	      *
*		    Version 2.0.					      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\******************************************************************************/

#define PROGRAM_DESCRIPTION "Display common C predefined macros & limits definitions"
#define PROGRAM_NAME    "macros"
#define PROGRAM_VERSION "2.0"
#define PROGRAM_DATE    "2022-10-17"

#ifdef _MSC_VER
#pragma warning(disable:4127) /* Ignore the "conditional expression is constant" warning */
#ifdef _MSDOS
#pragma warning(disable:4703) /* Ignore the "'main' : function too large for global optimizations" warning */
#endif
#endif

/* Avoid including too many include files, because we're first and foremost
   interested in the predefined macros, that the compiler itself defines. */
/* Also include only standard C include files. Nothing from MsvcLibX, etc. */
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define FALSE 0
#define TRUE 1

int iVerbose = 0;

#define streq(s1, s2) (!strcmp(s1,s2))

#define stringize( x ) #x                /* Convert a macro name to a string */
#define stringizex( x ) stringize( x )   /* Convert a macro value to a string */

#if defined(__GNUC__) || defined(__clang__)
/* It's actually clang that complains every time PRINTVAL() is invoked:
   warning: adding 'int' to a string does not append to the string [-Wstring-plus-int]
   The procedure for disabling warnings is actually the same for both compilers: */
#pragma GCC diagnostic ignored "-Wpragmas" /* Avoid further warnings for compilers the don't support the "-Wstring-plus-int" warning */ 
#pragma GCC diagnostic ignored "-Wstring-plus-int" /* Disable the "-Wstring-plus-int" warning */
#endif /* defined(__GNUC__) || defined(__clang__) */

/* Display the macro definition */
/* Discard the 1st character to avoid preprocessor errors in case of a blank macro. */
/* => Always invoke with one non-blank character ahead of the macro name.
      Ex: PRINTVAL(=__STDC__); */
#define PRINTVAL( x ) do { \
  const char *pszName = #x + 1; \
  const char *pszValue = stringize( x ) + 1; \
  if (strcmp(pszName, pszValue)) { \
    printf("#define %s %s", pszName, pszValue); \
    if (iVerbose) printf(" // \"%s\"", pszValue); \
    printf("\n"); \
  } else { /* Not 100% certain, but most likely. */ \
    if (iVerbose) printf("#undef %s\n", pszName); \
  } \
} while (0)

/* Forward references */
int IsSwitch(char *pszArg);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Program main routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|    2005/05/10 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: macros [OPTIONS]\n\
\n\
Options:\n\
  -?|-h       Display this help message and exit.\n\
  -v          Verbose mode. List macros evaluated and found undefined.\n\
  -V          Display this program version and exit.\n\
"
#if !(defined(_MSDOS) || defined(_WIN32)) /* Unix, MacOS, etc */
"\n"
#endif
);
}

int _cdecl main(int argc, char *argv[]) {
  int i;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {
      char *opt = arg+1;
      if (   streq(opt, "?")
	  || streq(opt, "h")
	  || streq(opt, "-help")) {
	usage();
	return 0;
      }
      if (streq(opt, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(PROGRAM_VERSION " " PROGRAM_DATE);
	return 0;
      }
      printf("Unrecognized switch ignored: %s\n", arg);
      continue;
    }
    printf("Unexpected argument ignored: %s\n", arg);
    continue;
  }

  /* Debug our own macros */
/*
#define BLANK
  PRINTVAL(=BLANK);
#define ONE 1
  PRINTVAL(=ONE);
  PRINTVAL(=TWO);
*/

  /* Language */
  PRINTVAL(=__cplusplus);
  PRINTVAL(=LANGUAGE_C);
  PRINTVAL(=_LANGUAGE_C);
  PRINTVAL(=__LANGUAGE_C);
  PRINTVAL(=__LANGUAGE_C__);

  /* ANSI */
  PRINTVAL(=__STDC__);
  PRINTVAL(=__FILE__);
  PRINTVAL(=__DATE__);
  PRINTVAL(=__LINE__);
  PRINTVAL(=__TIME__);
  PRINTVAL(=__TIMESTAMP__);
  PRINTVAL(=__STDC_HOSTED__);
  PRINTVAL(=__STDCPP_THREADS__);

  /* ANSI C99 */
  PRINTVAL(=__func__);
  PRINTVAL(=__FUNC__);
  PRINTVAL(=__FUNCTION__);
  PRINTVAL(=__FUNCDNAME__);		/* MS extension for decorated C++ names */
  PRINTVAL(=__FUNCSIG__);		/* MS extension for call type. Ex: cdecl */

  /* Debug mode */
  PRINTVAL(=DEBUG);
  PRINTVAL(=_DEBUG);
  PRINTVAL(=__DEBUG);
  PRINTVAL(=__DEBUG__);

  /* Implementation limits */
  PRINTVAL(=_INTEGRAL_MAX_BITS);
  PRINTVAL(=__CHAR_BIT__);
  PRINTVAL(=__DECIMAL_DIG__);
  PRINTVAL(=__SCHAR_MAX__);
  PRINTVAL(=__WCHAR_MAX__);
  PRINTVAL(=__INT_MAX__);
  PRINTVAL(=__LONG_MAX__);
  PRINTVAL(=__LONG_LONG_MAX__);
  PRINTVAL(=__SHRT_MAX__);
  PRINTVAL(=__WINT_MAX__);
  PRINTVAL(=__SIZE_MAX__);
  PRINTVAL(=__PTRDIFF_MAX__);
  PRINTVAL(=__INTMAX_MAX__);
  PRINTVAL(=__UINTMAX_MAX__);
  PRINTVAL(=__SIG_ATOMIC_MAX__);
  PRINTVAL(=__INT8_MAX__);
  PRINTVAL(=__INT16_MAX__);
  PRINTVAL(=__INT32_MAX__);
  PRINTVAL(=__INT64_MAX__);
  PRINTVAL(=__UINT8_MAX__);
  PRINTVAL(=__UINT16_MAX__);
  PRINTVAL(=__UINT32_MAX__);
  PRINTVAL(=__UINT64_MAX__);
  PRINTVAL(=__INT_LEAST8_MAX__);
  PRINTVAL(=__INT_LEAST16_MAX__);
  PRINTVAL(=__INT_LEAST32_MAX__);
  PRINTVAL(=__INT_LEAST64_MAX__);
  PRINTVAL(=__UINT_LEAST8_MAX__);
  PRINTVAL(=__UINT_LEAST16_MAX__);
  PRINTVAL(=__UINT_LEAST32_MAX__);
  PRINTVAL(=__UINT_LEAST64_MAX__);
  PRINTVAL(=__INT_FAST8_MAX__);
  PRINTVAL(=__INT_FAST16_MAX__);
  PRINTVAL(=__INT_FAST32_MAX__);
  PRINTVAL(=__INT_FAST64_MAX__);
  PRINTVAL(=__UINT_FAST8_MAX__);
  PRINTVAL(=__UINT_FAST16_MAX__);
  PRINTVAL(=__UINT_FAST32_MAX__);
  PRINTVAL(=__UINT_FAST64_MAX__);
  PRINTVAL(=__INTPTR_MAX__);
  PRINTVAL(=__UINTPTR_MAX__);

  PRINTVAL(=__WCHAR_MIN__);
  PRINTVAL(=__WINT_MIN__);
  PRINTVAL(=__SIG_ATOMIC_MIN__);

  /* Types */
  PRINTVAL(=_CHAR_UNSIGNED);
  PRINTVAL(=_LONGLONG);
  PRINTVAL(=__SIZE_TYPE__);
  PRINTVAL(=__PTRDIFF_TYPE__);
  PRINTVAL(=__WCHAR_TYPE__);
  PRINTVAL(=__WINT_TYPE__);
  PRINTVAL(=__INTMAX_TYPE__);
  PRINTVAL(=__UINTMAX_TYPE__);
  PRINTVAL(=__SIG_ATOMIC_TYPE__);
  PRINTVAL(=__INT8_TYPE__);
  PRINTVAL(=__INT16_TYPE__);
  PRINTVAL(=__INT32_TYPE__);
  PRINTVAL(=__INT64_TYPE__);
  PRINTVAL(=__UINT8_TYPE__);
  PRINTVAL(=__UINT16_TYPE__);
  PRINTVAL(=__UINT32_TYPE__);
  PRINTVAL(=__UINT64_TYPE__);
  PRINTVAL(=__INT_LEAST8_TYPE__);
  PRINTVAL(=__INT_LEAST16_TYPE__);
  PRINTVAL(=__INT_LEAST32_TYPE__);
  PRINTVAL(=__INT_LEAST64_TYPE__);
  PRINTVAL(=__UINT_LEAST8_TYPE__);
  PRINTVAL(=__UINT_LEAST16_TYPE__);
  PRINTVAL(=__UINT_LEAST32_TYPE__);
  PRINTVAL(=__UINT_LEAST64_TYPE__);
  PRINTVAL(=__INT_FAST8_TYPE__);
  PRINTVAL(=__INT_FAST16_TYPE__);
  PRINTVAL(=__INT_FAST32_TYPE__);
  PRINTVAL(=__INT_FAST64_TYPE__);
  PRINTVAL(=__UINT_FAST8_TYPE__);
  PRINTVAL(=__UINT_FAST16_TYPE__);
  PRINTVAL(=__UINT_FAST32_TYPE__);
  PRINTVAL(=__UINT_FAST64_TYPE__);
  PRINTVAL(=__INTPTR_TYPE__);
  PRINTVAL(=__UINTPTR_TYPE__);

  /* Limits in <limits.h> */
  PRINTVAL(=CHAR_BIT);
  PRINTVAL(=CHAR_MAX);
  PRINTVAL(=UCHAR_MAX);
  PRINTVAL(=SCHAR_MAX);
  PRINTVAL(=CHAR_MIN);
  PRINTVAL(=SCHAR_MIN);
  PRINTVAL(=INT_MAX);
  PRINTVAL(=INT_MIN);
  PRINTVAL(=LONG_MAX);
  PRINTVAL(=LONG_MIN);
  PRINTVAL(=LLONG_MAX);
  PRINTVAL(=LLONG_MIN);
  PRINTVAL(=MB_LEN_MAX);
  PRINTVAL(=SCHAR_MAX);
  PRINTVAL(=SCHAR_MIN);
  PRINTVAL(=SHRT_MAX);
  PRINTVAL(=SHRT_MIN);
  PRINTVAL(=UCHAR_MAX);
  PRINTVAL(=USHRT_MAX);
  PRINTVAL(=UINT_MAX);
  PRINTVAL(=ULONG_MAX);
  PRINTVAL(=ULLONG_MAX);

  /* C compiler */

  PRINTVAL(=__VERSION__);

  /* CLANG compiler */
  PRINTVAL(=__clang__);

  /* GCC compiler */
  PRINTVAL(=__GNUC__);
  PRINTVAL(=__GNUC_MINOR__);
  PRINTVAL(=__GNUC_PATCHLEVEL__);
  PRINTVAL(=__GNUG__);

  PRINTVAL(=__STRICT_ANSI__);
  PRINTVAL(=__BASE_FILE__);
  PRINTVAL(=__INCLUDE_LEVEL__);
  PRINTVAL(=__ELF__);
  PRINTVAL(=__OPTIMIZE__);
  PRINTVAL(=__OPTIMIZE_SIZE__);
  PRINTVAL(=__NO_INLINE__);
  PRINTVAL(=__GNUC_GNU_INLINE__);
  PRINTVAL(=__GNUC_STDC_INLINE__);
  PRINTVAL(=__CHAR_UNSIGNED__);
  PRINTVAL(=__WCHAR_UNSIGNED__);
  PRINTVAL(=__REGISTER_PREFIX__);
  PRINTVAL(=__USER_LABEL_PREFIX__);
  PRINTVAL(=__INT8_C);
  PRINTVAL(=__INT16_C);
  PRINTVAL(=__INT32_C);
  PRINTVAL(=__INT64_C);
  PRINTVAL(=__UINT8_C);
  PRINTVAL(=__UINT16_C);
  PRINTVAL(=__UINT32_C);
  PRINTVAL(=__UINT64_C);
  PRINTVAL(=__INTMAX_C);
  PRINTVAL(=__UINTMAX_C);
  PRINTVAL(=__SIZEOF_INT__);
  PRINTVAL(=__SIZEOF_LONG__);
  PRINTVAL(=__SIZEOF_LONG_LONG__);
  PRINTVAL(=__SIZEOF_SHORT__);
  PRINTVAL(=__SIZEOF_POINTER__);
  PRINTVAL(=__SIZEOF_FLOAT__);
  PRINTVAL(=__SIZEOF_DOUBLE__);
  PRINTVAL(=__SIZEOF_LONG_DOUBLE__);
  PRINTVAL(=__SIZEOF_SIZE_T__);
  PRINTVAL(=__SIZEOF_WCHAR_T__);
  PRINTVAL(=__SIZEOF_WINT_T__);
  PRINTVAL(=__SIZEOF_PTRDIFF_T__);
  PRINTVAL(=__BYTE_ORDER__);
  PRINTVAL(=__ORDER_LITTLE_ENDIAN__);
  PRINTVAL(=__ORDER_BIG_ENDIAN__);
  PRINTVAL(=__ORDER_PDP_ENDIAN__);
  PRINTVAL(=__FLOAT_WORD_ORDER__);
  PRINTVAL(=__DEPRECATED);
  PRINTVAL(=__EXCEPTIONS);
  PRINTVAL(=__GXX_RTTI);
  PRINTVAL(=__USING_SJLJ_EXCEPTIONS__);
  PRINTVAL(=__GXX_EXPERIMENTAL_CXX0X__);
  PRINTVAL(=__GXX_WEAK__);
  PRINTVAL(=__NEXT_RUNTIME__);
  PRINTVAL(=__LP64__);
  PRINTVAL(=_LP64);
  PRINTVAL(=__SSP__);
  PRINTVAL(=__SSP_ALL__);
  PRINTVAL(=__TIMESTAMP__);
  PRINTVAL(=__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1);
  PRINTVAL(=__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2);
  PRINTVAL(=__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4);
  PRINTVAL(=__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8);
  PRINTVAL(=__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16);
  PRINTVAL(=__GCC_HAVE_DWARF2_CFI_ASM);
  PRINTVAL(=__FP_FAST_FMA);
  PRINTVAL(=__FP_FAST_FMAF);
  PRINTVAL(=__FP_FAST_FMAL);

  /* Visual C++ compiler */
  PRINTVAL(=_MSC_VER);
  PRINTVAL(=_QC);
  PRINTVAL(=_FAST);
  PRINTVAL(=_PCODE);
  PRINTVAL(=_MSC_EXTENSIONS);
  PRINTVAL(=_VC_NODEFAULTLIB);
  PRINTVAL(=_CPPUNWIND);
  PRINTVAL(=_CPPRTTI);
  PRINTVAL(=_CAP_PROFILING);
  PRINTVAL(=_M_CEE);
  PRINTVAL(=_M_CEE_PURE);
  PRINTVAL(=_M_CEE_SAFE);
  PRINTVAL(=_Wp16);
  PRINTVAL(=_Wp32);
  PRINTVAL(=_Wp64);
  PRINTVAL(=_Wp128);
  PRINTVAL(=__MSVC_RUNTIME_CHECKS);
  PRINTVAL(=_MSC_BUILD);
  PRINTVAL(=_MSC_FULL_VER);
  PRINTVAL(=_MT);
  PRINTVAL(=_MD);
  PRINTVAL(=__CLR_VER);
  PRINTVAL(=_MSVC_LANG);

  PRINTVAL(=_WINDLL);
  PRINTVAL(=_DLL);
  PRINTVAL(=_MANAGED);

  PRINTVAL(=_OPENMP);

  /* OS */
  /* http://beefchunk.com/documentation/lang/c/pre-defined-c/preos.html */
  
  PRINTVAL(=linux);
  PRINTVAL(=_linux);
  PRINTVAL(=__linux);
  PRINTVAL(=__linux__);

  PRINTVAL(=unix);
  PRINTVAL(=_unix);
  PRINTVAL(=__unix);
  PRINTVAL(=__unix__);

  PRINTVAL(=__FreeBSD__);
  PRINTVAL(=__NetBSD__);
  PRINTVAL(=__OpenBSD__);

  PRINTVAL(=MSDOS);
  PRINTVAL(=_MSDOS);
  PRINTVAL(=__MSDOS);
  PRINTVAL(=__MSDOS__);
  PRINTVAL(=__DOS__);

  PRINTVAL(=OS2);
  PRINTVAL(=_OS2);
  PRINTVAL(=__OS2);
  PRINTVAL(=__OS2__);

  PRINTVAL(=WINDOWS);
  PRINTVAL(=_WINDOWS);
  PRINTVAL(=__WINDOWS);
  PRINTVAL(=__WINDOWS__);

  PRINTVAL(=WIN32);
  PRINTVAL(=_WIN32);
  PRINTVAL(=__WIN32);
  PRINTVAL(=__WIN32__);

  PRINTVAL(=WIN64);
  PRINTVAL(=_WIN64);
  PRINTVAL(=__WIN64);
  PRINTVAL(=__WIN64__);

  PRINTVAL(=__MINGW32__);	/* Minimalist GNU for Windows */
  PRINTVAL(=__MINGW64__);

  PRINTVAL(=__CYGWIN__);	/* Cygwin */
  PRINTVAL(=__CYGWIN32__);
  PRINTVAL(=__CYGWIN64__);

  PRINTVAL(=__INTERIX);		/* aka. Unix subsystem for Windows */

  PRINTVAL(=__MACH__);		/* MacOS */
  PRINTVAL(=__APPLE__);
  PRINTVAL(=__APPLE_CC__);
  PRINTVAL(=__BSD__);
  PRINTVAL(=__STRICT_BSD__);

  PRINTVAL(=_AIX);
  PRINTVAL(=AMIGA);
  PRINTVAL(=hpux);
  PRINTVAL(=__hpux);
  PRINTVAL(=__GNU__);		/* Hurd */
  PRINTVAL(=__osf__);		/* OSF/1 */
  PRINTVAL(=__QNX__);
  PRINTVAL(=sun);		/* Solaris or SunOS */
  PRINTVAL(=__sun);		/* Solaris or SunOS */
  PRINTVAL(=VMS);

  /* Processor type */
  PRINTVAL(=_X86_);
  PRINTVAL(=M_I86);
  PRINTVAL(=_M_I86);

  PRINTVAL(=M_I8086);
  PRINTVAL(=_M_I8086);

  PRINTVAL(=M_I286);
  PRINTVAL(=_M_I286);

  PRINTVAL(=M_I386);
  PRINTVAL(=_M_I386);
  
  /* x86 16-bits memory models, defined by MSVC 1.x */
  PRINTVAL(=M_I86TM);
  PRINTVAL(=_M_I86TM);	/* Tiny memory model: CS=DS=ES=SS */
  PRINTVAL(=M_I86SM);
  PRINTVAL(=_M_I86SM);	/* Small memory model: DS=SS, multiple code segments */
  PRINTVAL(=M_I86MM);
  PRINTVAL(=_M_I86MM);	/* Medium memory model: DS=SS, multiple code segments */
  PRINTVAL(=M_I86CM);
  PRINTVAL(=_M_I86CM);	/* Compact memory model: Single code segment, multiple data segments */
  PRINTVAL(=M_I86LM);
  PRINTVAL(=_M_I86LM);	/* Large memory model: Multiple code and data segments */
  PRINTVAL(=M_I86HM);
  PRINTVAL(=_M_I86HM);	/* Huge memory model: Multiple code and data segments; single array may be >64 KB */
  PRINTVAL(=_M_I86xM);

  /* 32 and 64-bits processor types */
  PRINTVAL(=_M_IX86);
  PRINTVAL(=_M_IX86_FP);
  PRINTVAL(=_M_ALPHA);
  PRINTVAL(=_M_IA64);
  PRINTVAL(=_M_MPPC);
  PRINTVAL(=_M_MRX000);
  PRINTVAL(=_M_PPC);
  PRINTVAL(=_M_X64);
  PRINTVAL(=_M_ARM);
  PRINTVAL(=_M_ARM_ARMV7VE);
  PRINTVAL(=_M_ARM_FP);
  PRINTVAL(=_M_ARM64);

  PRINTVAL(=i386);
  PRINTVAL(=_i386);
  PRINTVAL(=__i386);
  PRINTVAL(=__i386__);

  PRINTVAL(=i486);
  PRINTVAL(=_i486);
  PRINTVAL(=__i486);
  PRINTVAL(=__i486__);

  PRINTVAL(=i586);
  PRINTVAL(=_i586);
  PRINTVAL(=__i586);
  PRINTVAL(=__i586__);

  PRINTVAL(=i686);
  PRINTVAL(=_i686);
  PRINTVAL(=__i686);
  PRINTVAL(=__i686__);

  PRINTVAL(=alpha);
  PRINTVAL(=__alpha);
  PRINTVAL(=__alpha__);

  PRINTVAL(=__IA64__);

  PRINTVAL(=__AMD64__);

  return 0;
}

/* Things that don't work... */
/*
#define condmac( x ) #if x "yes" #else "no" #endif
printf("condmac(0) = " condmac(0));
printf("condmac(1) = " condmac(1));
*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|                                                                             |
|   Description:    Test if an argument is a command-line switch.             |
|                                                                             |
|   Parameters:     char *pszArg	    Would-be argument		      |
|                                                                             |
|   Return value:   TRUE or FALSE					      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  return (   (*pszArg == '-')
#if defined(_MSDOS) || defined(_WIN32)
	  || (*pszArg == '/')
#endif
	 ); /* It's a switch */
}

