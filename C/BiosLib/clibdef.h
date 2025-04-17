/*****************************************************************************\
*                                                                             *
*   Filename:	    CLIBDEF.H						      *
*									      *
*   Description:    Standard C library routines and constants definitions     *
*                                                                             *
*   Notes:	    Defines the subset of the standard C library routines     *
*		     available in BIOS.LIB.				      *
*		    Some functions are subsets of the standard function.      *
*		    A few related custom routines are also defined here for   *
*		     convenience.					      *
*									      *
*		    The routines defined in assembly language modules must    *
*		     use either the _cdecl or the _fastcall keywords, to show *
*		     what parameter passing convention has been implemented.  *
*									      *
*		    The routines defined in C language modules with variable  *
*		     number of arguments must use the _cdecl keyword.         *
*		    Else those with a fixed number of arguments must use the  *
*		     BIOSLIBCCC keyword, defined ahead of this file.	      *
*		     Currently BiosLib is compiled with the /Gr switch, which *
*		     makes them default to the _fastcall calling convention.  *
*		    							      *
*   History:								      *
*    1990-11-13 JFL Created this file.					      *
*    1990-11-15 JFL Added _cdecl keywords to printf & sprintf.		      *
*    1991-01-29 CBI Added declaration for RTC routines and cursor OFF/ON      *
*		      routines						      *
*    1991-02-06 CBI Fixed warning relative to TOLOWER			      *
*    1991-04-05 JFL Added routine strncpy prototype.			      *
*    1991-10-17 LTQ Added _strcat prototype.				      *
*    1993-07-09 JFL Moved the macro tolower() here.			      *
*    1993-07-12 JFL Moved the macros FP_SEG and FP_OFF here.		      *
*		      Added macro toupper().				      *
*    1993-07-30 JFL Removed the intrinsic pragma, not supported by C 7.0.     *
*		     Added routine strlwr prototype.			      *
*		     Added TRUE and FALSE constants.			      *
*    1993-10-06 JFL Separated this file from CLIBC.C.			      *
*		     Added the far string manipulation routines protos.       *
*    1994-03-10 JFL Changed size_t type from int to unsigned int, to be       *
*		      more compatible with stdio.h.			      *
*    1994-05-09 JFL Added prototype for exit.				      *
*    1995-04-07 JFL Standardized the definition for main.		      *
*    1995-09-14 JFL Added prototype for fmemcpy.			      *
*    1995-12-14 JFL Added prototype for strupr. 			      *
*    1999-09-06 JFL Added va_xxx definitions, and prototype for vsprintf().   *
*    2001-03-23 JFL Overload operator new and operator delete for C++.	      *
*		    Define distinct NEW() and DEL() macros for C and C++.     *
*		    But due to the above overload, both end up to malloc/free.*
*    2002-07-15 JFL Added routine _fstrlen().				      *
*    2015-10-27 JFL Added prototype for puts().				      *
*		    Added the generation of a library search record.	      *
*    2015-10-27 JFL Added prototype for fprintf().			      *
*    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
*		    Renamed putstr() as cputs().			      *
*    2022-11-25 JFL Explicitly specify the calling convention for all C       *
*		    functions, to make sure that minicom programs compiled    *
*		    in _MSDOS environments do call C lib functions correctly. *
*    2022-11-29 JFL Tweaks and fixes for BIOS/LODOS/DOS builds compatibility. *
*    2023-04-17 JFL Added malloc_last declaration.                            *
*    2025-03-10 JFL Added improvements to printf() and sprintf() internals.   *
*		    Define the UNUSED_ARG() macro for BiosLib.		      *
*		    							      *
*      (C) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __CLIBDEF_H__   /* Prevent multiple inclusions */
#define __CLIBDEF_H__

#ifndef BIOSLIBCCC
#define BIOSLIBCCC _fastcall /* The default C calling convention for BiosLib */
#endif

/* Force linking with the bios.lib library */
#ifndef BIOS_LIB
#define BIOS_LIB "bios.lib"
#pragma message("Adding pragma comment(lib, \"" BIOS_LIB "\")")
#pragma comment(lib, BIOS_LIB)
#endif /* defined(BIOS_LIB) */

#ifndef UNUSED_ARG
#define UNUSED_ARG(arg_name) do {break;} while (arg_name) /* Avoid an unused argument warning. No code generated. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FALSE 0
#define TRUE 1

#ifndef _SIZE_T_DEFINED		/* Prevent multiple inclusions */
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

/* Define NULL pointer value */

#ifdef __cplusplus
#define NULL 0			/* C++ requires this */
#else
#define NULL (void *)0		/* C will catch more errors this way. */
#endif

/* Macros to break C "far" pointers into their segment and offset components */
/* Can be used both as RValues and as LValues */

#define FP_SEG(fp) (*((unsigned _far *)&(fp)+1))
#define FP_OFF(fp) (*((unsigned _far *)&(fp)))

/* Functions defined as macros */

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define tolower(c) ((((c) >= 'A') && ((c) <= 'Z')) ? (c) + '\x20' : (c))
#define toupper(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c) - '\x20' : (c))

/* Variable arguments */

typedef char *va_list;
#define va_start(al, v) al = ((char *)&v + sizeof(v))
#define va_arg(al, t) ( *( ((t *)(al))++ ) )
#define va_end(al) al = (va_list)0

/* External references / function prototypes */

extern int _cdecl main(int argc, char *argv[]);
/* Note: Use cmain() instead of main to prevent loading the argument breaking routines. */

/* Intrinsic functions */

int _cdecl inp(int);
void _cdecl outp(int, int);
#pragma intrinsic(inp, outp)      /* Code faster and more compact this way */

/* In .asm files. Call specifiers _cdecl or _fastcall MUST be provided */

#if !(defined(_INC_STDLIB) || defined(__UTILDEF_H__)) /* Also prototyped in utildef.h for historical reasons */
extern unsigned short _psp;	/* Not defined for device drivers */
#endif
#define _getpid() _psp
#define getpid() _psp

extern void _fastcall _clearscreen(void);
extern int _fastcall kbhit(void);
extern void * malloc_base; /* The base of free memory */
extern void * malloc_last; /* The base of the last memory allocation */
extern void * _fastcall malloc(int);
extern void * BIOSLIBCCC realloc(void *, size_t);
#define free(p)	/* Place holder for future implementation. */ /* If implemented, also update C++ operator delete() at the end of this file */ 
/* Equivalent to MS-DOS-specific routines. TO DO: Implement them, really! */
#define _fmalloc malloc
#define _ffree free
#define _memavl() 999999
extern void _fastcall putchar(int);
extern int  _fastcall getchar(void);
extern int  _fastcall getch(void);
extern void _fastcall cursor_off(void);
extern void _fastcall cursor_on(void);
extern char * _fastcall strchr(const char *, int);
extern int  _fastcall strcmp(const char *, const char *);
extern int  _fastcall strncmp(const char *, const char *, int);
extern int  _fastcall memcmp(const void *, const void *, size_t);
extern char * _fastcall memcpy(void *, const void *, size_t);
extern char far * _cdecl _fmemcpy(void far *, const void far *, size_t);
extern char far * _cdecl _fmemmove(void far *, const void far *, size_t);
extern void _fastcall exit(int);
extern int  _fastcall rand(void);
extern void _fastcall srand(unsigned int seed);
extern int BIOSLIBCCC _fstrlen(char far *lpsz);
#define fstrlen _fstrlen

/* In .c files. Functions with a variable number of arguments must be _cdecl */
#define FILE void
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)
extern int _cdecl fprintf(FILE *, const char *, ...);
extern int _cdecl printf(const char *, ...);
extern int _cdecl sprintf(char *, const char *, ...);
extern int BIOSLIBCCC vsprintf(char *, const char *, va_list);
extern int BIOSLIBCCC sprintf1(char *, const char **);  /* Used internally by the library */
extern int _cdecl _snprintf(char *pszOutput, size_t uSize, const char *pszFormat, ...);
extern int BIOSLIBCCC _vsnprintf(char *pszOutput, size_t uSize, const char *pszFormat, va_list pArgs);
typedef void (*PSPRINTPROC)(char *pszOutput, char c);
extern int BIOSLIBCCC _vsnprintf1(PSPRINTPROC pSprintProc, char *pszOutput, size_t uSize, const char *pszFormat, va_list pArgs);

int atoi(const char *pszString);
long atol(const char *pszString);
extern int BIOSLIBCCC stcd_i(const char *, int *);      /* Used internally by the library */
extern int BIOSLIBCCC stch_i(const char *, int *);      /* Used internally by the library */
extern int BIOSLIBCCC stci_h(char *, int);		/* Used internally by the library */
extern int BIOSLIBCCC stci_d(char *, int);              /* Used internally by the library */
extern int BIOSLIBCCC stcli_h(char *, unsigned long);   /* Used internally by the library */
extern int BIOSLIBCCC stcli_d(char *, long);		/* Used internally by the library */
extern int BIOSLIBCCC stcu_d(char *, unsigned int);     /* Used internally by the library */
extern int BIOSLIBCCC stclh_u(const char *string, unsigned long *pul); /* Useful too */
extern int BIOSLIBCCC stcld_u(const char *string, unsigned long *pul); /* so is this one */

extern long BIOSLIBCCC strtol(const char *pszString, char **ppszEnd, int iBase);
extern unsigned long BIOSLIBCCC strtoul(const char *pszString, char **ppszEnd, int iBase);

extern int BIOSLIBCCC fputs(const char *, FILE *);      /* Standard routine, outputing a string and a new line to stdout */
extern int BIOSLIBCCC puts(const char *);               /* Standard routine, outputing a string and a new line to stdout */
extern int BIOSLIBCCC _cputs(const char *);             /* Microsoft-specific routine, outputing just the string to the console */
#define putstr _cputs		/* For a long time BiosLib had a putstr routine, then I learned about cputs which did the same */
#define cputs _cputs		/* And a non-standard-but-friendly alias */ 
extern char * BIOSLIBCCC gets(char *);			/* Input string from console */
#ifndef fflush						/* Also defined differently in LoDosLib */
#define fflush(hf)					/* Standard routine. Make it a noop in BiosLib */
#endif

extern int BIOSLIBCCC strlen(const char *);
extern char * BIOSLIBCCC strcpy(char *, const char *);
extern char * BIOSLIBCCC strncpy(char *, const char *, size_t);
extern char * BIOSLIBCCC strcat(char *, const char *);

extern char far * BIOSLIBCCC far _fstrncpy(char far *, const char far *, size_t);
extern char far * BIOSLIBCCC far _fstrncat(char far *, const char far *, size_t);
extern int BIOSLIBCCC _fstrcmp(const char far *, const char far *);
extern int BIOSLIBCCC _fcputs(const char far *);
#define fputstr _fcputs; /* For a long time I had an fputstr routine, then I learned about cputs which did the same as my putstr */
extern int BIOSLIBCCC _fmemcmp(void far *lp1, void far *lp2, size_t l);
extern void BIOSLIBCCC memset(void *pBuf, int c, size_t len);
extern void BIOSLIBCCC _fmemset(void far *lpBuf, int c, size_t len);

extern char * BIOSLIBCCC strlwr(char *);
extern char * BIOSLIBCCC strupr(char *);
#define _strlwr strlwr	/* Microsoft-specific, so the name should start with an _ */
#define _strupr strupr	/* Microsoft-specific, so the name should start with an _ */

extern size_t BIOSLIBCCC strspn(const char *string1, const char *string2);
extern size_t BIOSLIBCCC strcspn(const char *string1, const char *string2);
extern char * BIOSLIBCCC strpbrk(const char *string1, const char *string2);
extern char * BIOSLIBCCC strstr(const char *string1, const char *string2);

extern int _cdecl sscanf(const char *pszString, const char *pszFormat, ...);
extern int _cdecl vscanf(const char *pszString, const char **ppVarList);

#ifdef __cplusplus
} /* End of <extern "C"> block. */
#endif

/* Memory allocation macros */

/* Must be outside of the main <extern "C"> block. */
#ifndef __cplusplus
/* Convenient macros for C allowing to make it look like C++. */
#define NEW(object) ((object *)malloc(sizeof(object)))
#define DEL(object) free(object)
#else
/* Make sure C++ allocators get mapped to BIOSLIB functions */
inline void * _cdecl operator new(size_t uSize) {
  return malloc(uSize);
}
#pragma warning(disable:4100) /* Avoid warning "unreferenced formal parameter" */
inline void _cdecl operator delete(void *pMem) {
  free(pMem); /* TO DO: Remove pragmas if free is ever implemented */
}
#pragma warning(default:4100)
/* Same macros as above, for code ported from C. */
#define NEW(object) (new object)
#define DEL(object) (delete object)
#endif

#endif /* defined(__CLIBDEF_H__) */
