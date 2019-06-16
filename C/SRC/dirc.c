/*****************************************************************************\
*		    							      *
*   Filename:	    dirc.c						      *
*		    							      *
*   Description:    List one or two directories side by side, sorted.	      *
*		    							      *
*   Notes:	    In Windows, output names using the current code page.     *
*		    This can be overridden by using switches -A, -O, -U.      *
*		    							      *
*		    Uses our custom debugging macros in debugm.h.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug   # Debug version *
*		    							      *
*		    TO DO: Distinguish files by case under Unix.	      *
*		    							      *
*		    The DOS version must be compiled in large memory mode.    *
*                   Use the MSVC /AL switch.				      *
*		    							      *
*		    Do not use \n in printf(). Instead call printflf().       *
*		    This allows to manage the /p option correctly.	      *
*		    							      *
*		    Do not use the strcpy() and strncpy() functions.	      *
*		    Instead use the strncpyz() macro, which is safer.         *
*		    							      *
*   History:								      *
*    1990-03-21 JFL Initial version 1.0 created by jf.larvoire@hp.com.	      *
*    1990-06-22 JFL Added argument -b, -d and -bd to list only common         *
*		     or different files. Version 1.1.			      *
*    1992-02-28 JFL Adapted to OS/2.					      *
*		    Added third argument to specify a search pattern.	      *
*		    The pattern may also be specified behind the	      *
*		    Directory name like in the DOS dir command.		      *
*		    Directories listed first.				      *
*    1992-03-06 JFL List directories first.				      *
*    1992-03-31 JFL Allow to specify the pattern for each directory.	      *
*    1993-06-14 JFL Added switch /fc to actually compare data.		      *
*    1993-06-15 JFL Added display of the number of files listed.	      *
*		    Version 1.3.					      *
*    1993-09-30 JFL Fixed a bug with the /fc switch, that always found	      *
*		    files equal if they had the same size. Version 1.31.      *
*		    Version 1.31					      *
*    1993-10-06 JFL Display dates in the year/month/date format.	      *
*		    Added the -f switch. Renamed -fc as -c.		      *
*		    Version 1.4.					      *
*    1993-10-15 JFL Added recursive searches with switch -s.		      *
*		    Version 1.5.					      *
*    1993-10-18 JFL List unmatching subtrees.				      *
*		    Added the -z flag.					      *
*		    Version 1.51.					      *
*    1994-03-16 JFL Added arguments -from and -to.			      *
*		    Fixed a display problem when recursing to subdirs.        *
*		    Version 1.52					      *
*    1994-03-17 JFL Removed limitation on recursion depth.		      *
*		    Added date range verification.			      *
*		    Version 1.6.					      *
*    1994-03-18 JFL Fixed a bug in routine makepathname when specifying	      *
*		    only a drive letter.				      *
*		    Version 1.61.					      *
*    1994-09-28 JFL Added option -i.					      *
*		    Version 1.62.					      *
*    1994-10-12 JFL Changed to model large.				      *
*		    Support 2600 files instead of 1200. 		      *
*		    Improved the -i switch to work for +/- 23 hours.	      *
*		    Version 1.63.					      *
*    1994-10-13 JFL Increased the stack size to support more subdirectory     *
*		    levels. Version 1.64.				      *
*    1994-12-16 JFL Reintroduced the MS-DOS code. Allows to conditionally     *
*		    compile either the DOS or the bound DOS+OS/2 version.     *
*		    Version 1.65.					      *
*    1995-01-16 JFL Dynamically allocate fif structures, and sort an	      *
*		    array of pointers to the structures instead of an	      *
*		    array of structures as before. This allows to	      *
*		    process a number of files limited only by the amount      *
*		    of memory, instead as a fixed number as before.	      *
*		    Do case-insensitive name comparisons.		      *
*    1995-01-17 JFL Added a Win32 version of the file search routines.	      *
*    1995-01-19 JFL Replaced all \n in printf formats by calls to the	      *
*		    new routine printflf. Allows pauses on full screens.      *
*    1995-01-20 JFL Dynamically allocate the fif node names. This allows      *
*		    to support long file names, with minimal overhead.	      *
*    1995-02-03 JFL Final version 2.0. No change. Features:		      *
*		    Dual mode DOS & Win32 instead of DOS & OS/2.	      *
*		    Unlimited number of files under Win32 & OS/2.	      *
*		    Greatly increased limit under DOS.			      *
*		    Long file name support (Except under DOS).		      *
*		    New /p option to pause display after each page.	      *
*		    Returns Errorlevel 1 if out of memory.		      *
*		    Returns Errorlevel 2 if requested path not found.	      *
*    1995-06-12 JFL Added option -j. Modified datecmp() to support it.	      *
*		    Added option -k. Modified cmpfif() to support it.	      *
*		    Renamed datecmp() as CompareToNext().		      *
*		    Made filecompare() independant of dirc.		      *
*		    Improved display of long file names.		      *
*		    Fixed minor bugs.					      *
*			 - A file and a directory with the same name	      *
*			    listed on the same line			      *
*			 - The size of files matching with the -i option      *
*			    was not tested.				      *
*		    Version 2.1.					      *
*    1995-06-19 JFL Fixed bug with broken use of filecompare(). 	      *
*		    Version 2.11.					      *
*    1995-06-23 JFL The filecompare() use fix was still not correct!	      *
*		    Change usage explanation for the -c option. Its	      *
*		    misunderstanding caused the request for the -j option!    *
*		    Version 2.12.					      *
*    1995-08-08 JFL Changed the default file name comparison to case	      *
*		    insensitive. Reversed the meaning of the -k switch.       *
*		    Version 2.13.					      *
*    1995-09-25 JFL Fixed minor bug with case of subdirectories.	      *
*		    Version 2.14.					      *
*    1995-12-22 JFL Compare all characters of long file names, and not	      *
*		    only the 32 first ones as before. Version 2.15.	      *
*    1996-08-28 JFL Ignore +/- 2 seconds errors in -i mode. Version 2.16.     *
*    1996-09-16 JFL Ignore +/-2s errs combined with TZ errs. Version 2.17.    *
*    1999-01-14 JFL Use application-independant versions of srch1st/next/done.*
*		    Added the -t switch.				      *
*		    Added the -u switch.				      *
*		    Version 2.20.					      *
*    1999-06-15 JFL Added switch -env. Version 2.21.			      *
*    1999-06-29 JFL Fixed bug in SetMasterEnv, causing crash under NT. v2.22. *
*    2000-12-04 JFL Added the -x switch.				      *
*		    Added routine chdirx for DOS, to go into paths longer     *
*		     than 63 characters. (DOS boxes in Windows 9X only).      *
*		    Version 2.23.					      *
*    2005-05-03 JFL Added a Unix port.      				      *
*		    Can now be compiled as C++.				      *
*		    Adjust the output width to the display width.	      *
*		    Added the -w switch to force a width.		      *
*		    Fixed bug in DOS version of GetScreenRows(). This caused  *
*		     the -p option to display one line too many in Win32!     *
*		    Version 2.30.					      *
*    2006-11-29 JFL Fixed bug in the option -f in the Unix port.              *
*		    Tag <link> and !device! files. (Unix and Windows)         *
*		    Version 2.31.					      *
*    2006-12-07 JFL Changed option -r to an alias of {-d -f -s -z}.           *
*		    Merged the feature history into the change history.       *
*		    Version 2.32.					      *
*    2008-01-04 JFL Added definition to avoid Visual C++ 2005 warnings.       *
*    2008-01-24 JFL Dynamically adjust name and size fields boundaries.       *
*		    Version 2.33.					      *
*    2008-01-25 JFL Make the termcap library optional under Unix, using tput. *
*                   Use direntry.d_type if defined, else use S_ISXXX macros.  *
*		    Fixed an old bug in affiche1, that could cause a crash!   *
*                   Don't display a logo, and undocumented the -nologo option.*
*		    Version 2.34.					      *
*    2008-02-04 JFL Under Unix... Fixed a bug with broken links;              *
*                   Flag as a directory links that do point to a directory;   *
*                   Updated the output format to look more Unix-like.         *
*		    Version 2.35.					      *
*    2009-01-20 JFL Corrected a typo in help. Version 2.36.                   *
*    2009-02-16 JFL Restructured to support file sizes > 4GB correctly.       *
*    2009-02-24 JFL Restructured to use standard C99 int types.               *
*		    Fixed a bug in affiche1 (overflow and name truncations!)  *
*		    Version 2.40.					      *
*    2009-04-14 JFL Changed the default on Unix to use case in file names.    *
*		    Added option -K to obtain the reverse effect.             *
*		    Version 2.41.					      *
*    2009-04-14 JFL Bugfix: WIN32 files sizes were incorrectly recorded > 2GB.*
*		    Version 2.42.					      *
*    2012-01-18 JFL Use the new debugging framework in debug.h.	   	      *
*		    Fixed option -K. Added option -V.			      *
*		    Version 2.4.3.					      *
*    2012-02-04 JFL Changed version format to: VERSION DATE OS [DEBUG]	      *
*		    Added version strings for more OS. Added option -V.	      *
*		    Version 2.4.4.					      *
*    2012-05-01 JFL Allow -from and -to dates as YYYY-MM-DD or YYYY/MM/DD.    *
*		    Allow continuing after failing to enter a subdirectory.   *
*		    Version 2.4.5.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.4.6.                 *
*    2013-02-21 JFL Output dates in the YYYY-MM-DD format. Version 2.4.7.     *
*    2013-02-22 JFL Fixed a warning in Linux.                                 *
*    2014-01-21 JFL Use a much larger buffer for 32-bits apps, to improve     *
*                   file comparison performance. Version 2.4.8.               *
*    2014-02-14 JFL Removed all OS-specific directory search routines, and    *
*		    use instead standard C library routines in MsvcLibX.lib.  *
*		    Added support for symlinks, and junctions in Windows.     *
*    2014-02-20 JFL Added option -L to compare link targets, instead of links.*
*		    Allow negative number of days relative to today.          *
*    2014-02-26 JFL Use MsvcLibX LocalFileTime() to display local file times  *
*		    in Windows the same way cmd.exe and explorer.exe do.      *
*    2014-02-27 JFL Added support for Unicode file names.		      *
*		    Output using the current console code page.               *
*		    Added options -A, -O, -U to force using another encoding. *
*    2014-02-28 JFL Convert command-line arguments to UTF-8 too.	      *
*    2014-03-20 JFL Correct the arguments case. (DOS and Windows only)        *
*    2014-04-01 JFL Removed the -env option for WIN32, which does not work,   *
*		    and was only required for very old DOS scripts.           *
*		    Version 3.0.  					      *
*    2014-07-02 JFL Changed macro RETURN() to RETURN_CONST().		      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Bug fix: Adjust TIMET_MAX, to work for 32-bits time_t.    *
*		    Version 3.0.1.  					      *
*    2015-10-15 JFL In recursive mode, only show statistics if they're not 0. *
*		    Version 3.0.2.  					      *
*    2015-11-13 JFL Removed WINVER definition.                                *
*		    Version 3.0.3.  					      *
*    2015-12-14 JFL Bug fix: Failed to start in Windows XP due to missing fct.*
*		    Bug fix: DOS version failed to read root directories.     *
*		    Version 3.0.4.  					      *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 3.0.5.  					      *
*    2017-10-02 JFL Fixed a conditional compilation bug in MSDOS.	      *
*		    Version 3.0.6.    					      *
*    2018-04-24 JFL Use PATH_MAX and NAME_MAX from limits.h.                  *
*    2018-04-25 JFL Dynamically allocate PATHNAME variables from the heap in  *
*		    Windows and Unix, to avoid stack overflow issues.         *
*    2018-04-26 JFL Make sure WIN32 pathname buffers are larger than 260 B.   *
*		    Do not display the | path separator when listing one dir. *
*		    Changed the -debug option to -D.                          *
*		    Option -V now displays the MsvcLibX library version.      *
*    2018-04-30 JFL Check errors for all calls to chdir() and getcwd().	      *
*		    Rewrote finis() so that it displays errors internally.    *
*		    Version 3.1.    					      *
*    2019-01-09 JFL Corrected assignments in conditional expressions.	      *
*		    Replaced all global configuration variables by one bit    *
*		    field passed recursively to all subroutines.	      *
*    2019-01-10 JFL Added option -ct to report equal files with != times.     *
*		    Version 3.2.    					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.3.2.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 3.2.2.      *
*		    							      *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Compare directories side by side, sorted by file names"
#define PROGRAM_NAME    "dirc"
#define PROGRAM_VERSION "3.2.2"
#define PROGRAM_DATE    "2019-06-12"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _ISOC99_SOURCE      /* Tell the GNU library that we support C99 syntax */
#define __STDC_LIMIT_MACROS /* Make sure C99 macros are defined in C++ */
#define __STDC_CONSTANT_MACROS

#define FALSE 0
#define TRUE 1

#define _BSD_SOURCE    		/* Include extra BSD-specific functions. Implied by _GNU_SOURCE. */
#define _LARGEFILE_SOURCE	/* Force using 64-bits file sizes if possible */
#define _GNU_SOURCE		/* Replaces nicely all the above */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */

#ifndef USE_TERMCAP
#define USE_TERMCAP 0		/* 1=Use termcap lib; 0=Don't */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <inttypes.h> /* Actually we just need stdint.h, but Tru64 doesn't have it */
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <iconv.h>
#include <stdarg.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#ifndef UINTMAX_MAX /* For example Tru64 doesn't define it */
typedef unsigned long uintmax_t;
#endif

#define TIME_T_MAX ((time_t)((~0UL) >> 1))

/* These are non standard routines, but the leading _ is annoying */ 
#define strlwr _strlwr
#define strupr _strupr
#define stricmp _stricmp
#define strnicmp _strnicmp

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#define DIRSEPARATOR '/'
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define IGNORECASE FALSE
#define HAS_DRIVES FALSE

#include <strings.h>

#define _getch getchar

#include <ctype.h>

#if 0
static char *strlwr(char *pString)
{
  char c;
  char *pc = pString;
  for ( ; (c=*pc); pc++) *pc = tolower(c);
  return pString;
}
#endif
static char *strupr(char *pString)
{
  char c;
  char *pc = pString;
  for ( ; (c=*pc); pc++) *pc = toupper(c);
  return pString;
}
#undef stricmp
#define stricmp strcasecmp
#undef strnicmp
#define strnicmp strncasecmp

#ifndef FNM_MATCH
#define FNM_MATCH	0	/* Simpler than testing != FNM_NO_MATCH */ 
#endif

#define SetMasterEnv(string, value) setenv(string, value, 1)

/* DOS File attribute constants */

#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */

#define CDECL

#define LocalFileTime localtime

#define CountCharacters(string, cp) (int)mbstowcs(NULL, string, 0) /* Count the number of characters (not bytes!) in that string */
/* TO DO: Use wcswidth instead. (mbstowcs reports the # of characters, which is NOT the # of columns for Asian ideographs.) */
/*        Use mbsrtowcs to generate the wchar_t version first. */
/*        For Windows, see http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c */

#endif /* __unix__ */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#include <conio.h>		/* For getch() */

#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define IGNORECASE TRUE
#define HAS_DRIVES TRUE

#define _A_SHORT 0x80    /* Dummy attribute for forcing short DOS names */

int SetMasterEnv(char *pszName, char *pszValue);

#define CDECL

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */

#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define IGNORECASE TRUE
#define HAS_DRIVES TRUE

#include <conio.h>		/* For getch() */

int SetMasterEnv(char *pszName, char *pszValue);

#define CDECL _cdecl

#define OFFSET_OF(pointer) ((uint16_t)(uint32_t)(void far *)pointer)
#define SEGMENT_OF(pointer) ((uint16_t)(((uint32_t)(void far *)pointer) >> 16))

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#include <dos.h>
#include <direct.h>
#include <conio.h>
#include <io.h>

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h"

#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#define IGNORECASE TRUE

#define CDECL _cdecl

#endif /* _OS2 */

/*********************************** Other ***********************************/

#if (!defined(DIRSEPARATOR)) || (!defined(EXE_OS_NAME))
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

/* Local definitions */

#define PATHNAME_SIZE PATH_MAX		/* Buffer size for holding pathnames, including NUL */
#define NODENAME_SIZE (NAME_MAX+1)	/* Buffer size for holding file names, including NUL */

#if PATHNAME_SIZE < 1000 /* Simplified implementation for MSDOS */
  #define PATHNAME_BUFS_IN_HEAP 0	/* Alloc them on the stack */
  #define NEW_PATHNAME_BUF(var) char var[PATHNAME_SIZE]
#else	/* Avoid stack overflows by storing too large variables on stack */
  #define PATHNAME_BUFS_IN_HEAP 1	/* Alloc them in the memory heap */
  #define NEW_PATHNAME_BUF(var) char *var = malloc(PATHNAME_SIZE)
#endif
#if !PATHNAME_BUFS_IN_HEAP
  #define TRIM_PATHNAME_BUF(var) do {} while (0)
  #define FREE_PATHNAME_BUF(var) do {} while (0)
#else
  #define TRIM_PATHNAME_BUF(var) do {char *p = realloc(var, strlen(var)+1); if (p) var = p;} while (0)
  #define FREE_PATHNAME_BUF(var) free(var);
#endif

typedef enum {
  bFALSE = 0,
  bTRUE = 1
} BOOL;

#define SIGN(x)  ( ((x) >= 0) ? 1 : -1 )
#define SIGN3(x) ( ((x) == 0) ? 0 : SIGN(x) )

#define MISMATCH (-32767)
#define DATE_MISMATCH (-32766)

#define RETCODE_SUCCESS 0	    /* Return codes processed by finis() */
#define RETCODE_NO_FILE 1	    // Note: Value 1 required by HP Preload -env option
#define RETCODE_TOO_MANY_FILES 2    // Note: Value 2 required by HP Preload -env option
#define RETCODE_NO_MEMORY 3
#define RETCODE_INACCESSIBLE 4

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

/* File attributes not defined by MS-DOS (which stops at 0x20) */
#define _A_LINK     0x40
#define _A_DEVICE   0x80

typedef int (*PSTATFUNC)(const char *path, struct stat *buf);

/* Directory scan functions definitions */

typedef struct fif {	    /* OS-independant FInd File structure */
  char *name; 			/* File node name, ending with a NUL */
  struct stat st;		/* File size, time, etc */
#ifndef _MSDOS
  char *target; 		/* Link target name, for links */
#endif
  int column;
  struct fif *next;
} fif;

/* Configuration flags recursively passed to all local subroutines */

typedef struct {
  int verbose:1;	/* Display the pathname operated on */
  int recurse:1;	/* Recursive operation */
  int nocase:1;		/* Ignore case */
  int both:1;		/* List only files present in both paths */
  int diff:1;		/* List only files that don't match */
  int compare:1;	/* Compare files contents to verify equality */
  int zero:1;		/* Don't display anything if no file */
  int notz:1;		/* Ignore time zone differences */
  int notime:1;		/* Ignore file date and time altogether */
  int upper:1;		/* Display names in upper case */
  int cont:1;		/* Continue after errors */
  int dtime:1;		/* Report equal files with != times */
  /* Caution: If more than 16 flags are defined, t_opts becomes a long in MS-DOS
              This would force to change DEBUG_ENTER() format strings for MS-DOS! */
} t_opts;

/* Global variables */

#if HAS_DRIVES
char init_drive;                    /* Initial drive */
#endif
char init_dir[PATHNAME_SIZE];       /* Initial directory */
int iPause = 0;			    /* If > 0, number of lines between pauses */
char path1[PATHNAME_SIZE] = {0};    /* First path scanned */
char path2[PATHNAME_SIZE] = {0};    /* Second path scanned */
fif *firstfif = NULL;		    /* Pointer to the first allocated fif structure */
long lNFileFound = 0;		    /* Total number of distinct files found */
long lLFileFound = 0;		    /* Total number of left files found */
long lRFileFound = 0;		    /* Total number of right files found */
long lEFileFound = 0;		    /* Total number of equal files found */
uintmax_t llLTotalSize = 0;	    /* Total size of left files found */
uintmax_t llRTotalSize = 0;	    /* Total size of right files found */
uintmax_t llETotalSize = 0;	    /* Total size of equal files found */
int iRows = 0;                      /* Number of rows of the display */
int iCols = 0;                      /* Number of columns of the display */
int iYearWidth = 2;		    /* Width of the year field displayed */
int iNameWidth = 12;		    /* Width of the file name field displayed */
int iSizeWidth = 8;		    /* Width of the file size field displayed */
PSTATFUNC pStat = lstat;	    /* Function to use for getting file infos */
#ifdef _WIN32
// UINT cp = 0;			    /* Initial console code page */
#define cp codePage		    /* Initial console code page in iconv.c */
#endif


/* Function prototypes */

void usage(void);                   /* Display a brief help and exit */
void finis(int retcode, ...);       /* Return to the initial drive & exit */
int IsSwitch(char *pszArg);	    /* Is this a command-line switch? */

int lis(char *, char *, int, int, int, time_t, time_t, t_opts); /* Scan a directory */
int CDECL cmpfif(const fif **fif1, const fif **fif2, int ignorecase);
void trie(fif **ppfif, int nfif, t_opts);
int affiche(fif **, int, int, t_opts); /* Display sorted list on two columns */
void affichePaths(void);
int affiche1(fif *pfif, int col, t_opts);
int descend(char *from, char *to,
            char *pattern, int attrib,
            t_opts opts,
	    time_t datemin, time_t datemax);
fif **AllocFifArray(size_t nfif);   /* Allocate an array of fif pointers */
void FreeFifArray(fif **fiflist);

int makepathname(char *, char *, char *);
int filecompare(char *, char *);    /* Compare two files */
int CompareToNext(fif **, t_opts);  /* Compare dates w. next entry in fiflist */

int GetScreenRows(void);	    /* Get the number of rows of a text screen */
int GetScreenColumns(void);	    /* Get the number of columns of a text screen */
void printflf(void);		    /* Print a line feed, and possibly pause */

int parse_date(char *token, time_t *pdate);

char *getdir(char *, int);          /* Get the current drive directory */

#ifndef __unix__
int FixNameCase(char *pszPathname); /* Correct the case of an existing pathname */
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    C program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|									      |
|    1990-03-21 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char *from = NULL;          /* What directory to list */
  char *to = NULL;            /* What directory to compare it to */
  char *pattern = NULL;       /* Wildcards pattern */
  t_opts opts = {0};	      /* User-defined options */
  NEW_PATHNAME_BUF(path);     /* Temporary pathname */
  int i;
  int nfif = 0;
  /* File attributes to search, ie. all but disk labels. */
#if defined(__unix__)
  int attrib = (_A_SUBDIR | _A_SYSTEM | _A_HIDDEN | _A_LINK | _A_DEVICE);
#elif defined(_WIN32)
  int attrib = (_A_SUBDIR | _A_SYSTEM | _A_HIDDEN | _A_LINK);
#else // DOS or OS/2
  int attrib = (_A_SUBDIR | _A_SYSTEM | _A_HIDDEN);
#endif
  int ndir;                   /* Directory number */
  time_t datemin = 0;		/* Minimum date stamp */
  time_t datemax = TIME_T_MAX;/* Maximum date stamp */
  char *dateminarg = NULL;    /* Minimum date argument */
  char *datemaxarg = NULL;    /* Maximum date argument */
  fif **fiflist;		/* Array of fif pointers for sorting */
  int iStats = FALSE;
#ifdef _MSDOS
  char *pszOneToEnv = NULL;	/* Copy one file name to environment variable */
#endif
  int iBudget;

  /* Set the default options */
  opts.cont = 1;		/* Continue in case of error */
#if IGNORECASE			/* If TRUE, ignore case in file names */
  opts.nocase = 1;
#endif

#if HAS_DRIVES
  init_drive = (char)_getdrive();
#endif
  getdir(init_dir, PATHNAME_SIZE);

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      char *opt = arg+1;
#ifdef _WIN32
      if (streq(opt, "A")) {	/* Force encoding output with the ANSI code page */
	cp = CP_ACP;
	continue;
      }
#endif
      if (streq(opt, "b")) {
	opts.both = 1;
	continue;
      }
      if (streq(opt, "bd")) {
	opts.both = 1;
	opts.diff = 1;
	continue;
      }
      if (streq(opt, "c")) {
	opts.compare = 1;
	continue;
      }
      if (streq(opt, "ct")) {
	opts.compare = 1;
	opts.dtime = 1;
	continue;
      }
      if (streq(opt, "d")) {
	opts.diff = 1;
	continue;
      }
      DEBUG_CODE(
	if ((streq(opt, "D")) || (streq(opt, "debug"))) {
	  DEBUG_ON();
	  continue;
	}
      )
      if (streq(opt, "e")) {
	opts.cont = 0;
	continue;
      }
      if (streq(opt, "E")) {
	opts.cont = 1;
	continue;
      }
#ifdef _MSDOS
      if (streq(opt, "env")) {
	if ((i+1) < argc)
	  pszOneToEnv = argv[++i];
	else
	  usage();
	continue;
      }
#endif
      if (streq(opt, "f")) {
	attrib &= ~_A_SUBDIR;   /* Clear the directory attribute */
	continue;
      }
      if (streq(opt, "from")) {
	dateminarg = argv[++i];
	if (!parse_date(dateminarg, &datemin)) {
	  printf("Invalid date format: -from %s", dateminarg);
	  printflf();
	  dateminarg = NULL;
	}
	continue;
      }
      if (streq(opt, "g")) {	/* Debug: Get screen dimensions */
	printf("\nScreen size: %d lines", GetScreenRows());
	printf(" * %d columns\n", GetScreenColumns());
	finis(0);
      }
      if (   streq(opt, "help")
	  || streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "i")) {
	opts.notz = 1;		/* Ignore time zone differences */
	continue;
      }
      if (streq(opt, "j")) {
	opts.notime = 1;	/* Ignore file date and time completely */
	continue;
      }
      if (streq(opt, "K")) {
	opts.nocase = 1;	/* Ignore case completely in file names */
	continue;
      }
      if (streq(opt, "k")) {
	opts.nocase = 0;	/* Consider case meaninful in file names */
	continue;
      }
      if (streq(opt, "L")) {
	pStat = stat;		/* Compare link targets */
	continue;
      }
      if (streq(opt, "nologo")) { /* Kept for compatibility with old scripts using it. */
	continue;		  /* Do nothing */
      }
#ifdef _WIN32
      if (streq(opt, "O")) {	/* Force encoding output with the OEM code page */
	cp = CP_OEMCP;
	continue;
      }
#endif
      if (streq(opt, "p")) {
	iPause = GetScreenRows() - 1; /* Pause once per screen */
	continue;
      }
      if (streq(opt, "r")) {	/* Alias for -d -f -s -z */
	opts.diff = 1;
	attrib &= ~_A_SUBDIR;   /* Clear the directory attribute */
	opts.recurse = 1;
	opts.zero = 1;
	continue;
      }
      if (streq(opt, "s")) {	/* Recursive search */
	opts.recurse = 1;
	continue;
      }
      if (streq(opt, "t")) {	/* Display statistics */
	iStats = TRUE;
	continue;
      }
      if (streq(opt, "to")) {
	datemaxarg = argv[++i];
	if (!parse_date(datemaxarg, &datemax)) {
	  printf("Invalid date format: -to %s", datemaxarg);
	  printflf();
	  datemaxarg = NULL;
	}
	datemax += 86399;	/* Index of the last second of that day */
	continue;
      }
      if (streq(opt, "u")) {	/* Display names in upper case */
	opts.upper = 1;
	continue;
      }
#ifdef _WIN32
      if (streq(opt, "U")) {	/* Force encoding output with the UTF-8 code page */
	cp = CP_UTF8;
	continue;
      }
#endif
      if (streq(opt, "v")) {	/* Verbose mode */
	opts.verbose = 1;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	finis(0);
      }
      if (streq(opt, "w")) {	/* Set output width */
	if (((i+1) < argc) && !IsSwitch(argv[i+1])) {
	  iCols = atoi(argv[++i]);
	}
	continue;
      }
#ifdef _WIN32
      if (streq(opt, "x")) {
	attrib |= _A_SHORT;     /* Dummy attribute to force using short names */
	continue;
      }
#endif
      if (streq(opt, "z")) {
	opts.zero = 1;
	continue;
      }
      printf("Unrecognized switch %s. Ignored.", arg);
      printflf();
      continue;
    } /* End if it's a switch */
    /* If it's an argument */
    if (!from) {
      from = arg;
      continue;
    }
    if (!to) {
      to = arg;
      continue;
    }
    if (!pattern) {
      pattern = arg;
      continue;
    }
    printf("Unexpected argument \"%s\" ignored.", arg);
    printflf();
    break;  /* Ignore other arguments */
  }

#if defined(_WIN32)
  DEBUG_PRINTF(("// Outputing using code page %d\n", cp));
#endif

  /* Dynamically size columns based on screen width */
  iRows = GetScreenRows();
  if (!iCols) iCols = GetScreenColumns(); // If not forced by the -w option
  if (iCols < 80) iCols = 100; // Don't allow anything below 80. Dflt=100.
  iBudget = (iCols - 4) / 2;  // Space available for each directory output.
  iBudget -= 18; // Remove minimal date/time fieds: " YY/MM/DD HH:MM:SS"
  if (iBudget >= 22) {
    iYearWidth = 4; // Display the century too.
    iBudget -= 2;
  }
  if (iBudget > (iNameWidth + iSizeWidth)) {
    int iExtra = iBudget - (iNameWidth + iSizeWidth);
    iSizeWidth += iExtra/4;
    iNameWidth += (iExtra - iExtra/4);
  }

#ifdef _OS2
/* Make sure to include os2.h at the beginning of this file, and before that
  to define the INCL_DOSMISC constant to enable the necessary section */

  DEBUG_CODE_IF_ON(
    uint8_t mode;

    DosGetMachineMode(&mode);
    switch (mode) {
      case MODE_REAL:
	printf("Running in DOS mode.\n");
	break;
      case MODE_PROTECTED:
	printf("Running in OS/2 mode.\n");
	break;
      default:
	printf("Running neither in DOS nor in OS/2 mode!?!\n");
	break;
    }
  )
#endif

  getdir(path, PATHNAME_SIZE);    /* Get current directory full pathname */
  if (!from) from = path;

#ifndef __unix__
  FixNameCase(from);
  if (to) FixNameCase(to);
#endif // !defined(__unix__)

  nfif = lis(from, pattern, 0, ndir=1, attrib, datemin, datemax, opts);
  if (to) nfif = lis(to, pattern, nfif, ++ndir, attrib, datemin, datemax, opts);
  DEBUG_PRINTF(("nfif = %d;\n", nfif));

  fiflist = AllocFifArray(nfif);
  trie(fiflist, nfif, opts);
  affiche(fiflist, nfif, ndir, opts);
  FreeFifArray(fiflist);

  if (opts.recurse) {
    descend(from, to, pattern, attrib, opts, datemin, datemax);
    if (lNFileFound) { /* Only list the total if it's not null */
      printflf();
      printf("Total: %ld files or directories listed.", lNFileFound);
      printflf();
    }
  }

#ifdef _MSDOS
  /* Move the single line found to the given environment variable */
  if (pszOneToEnv) {	// If the -env option was specified
    switch (lLFileFound) {	// How many lines were found?
      case 0:
	finis(RETCODE_NO_FILE, NULL);
      case 1:
	i = SetMasterEnv(pszOneToEnv, fiflist[0]->name);
	if (i) printf("Out of environment space.\n");
	finis(RETCODE_SUCCESS);
      default:
	finis(RETCODE_TOO_MANY_FILES, NULL);
    }
  }
#endif

  if (iStats) {
    printflf();
    printf("Listed %ld files in %s. Total size %"PRIuMAX" bytes.",
		lLFileFound, from, llLTotalSize);
    printflf();
  }
  if (iStats && to) {
    printf("Listed %ld files in %s. Total size %"PRIuMAX" bytes.",
		lRFileFound, to, llRTotalSize);
    printflf();
    printf("%ld files were equal. Total size %"PRIuMAX" bytes.",
		lEFileFound, llETotalSize);
    printflf();
  }

  finis(RETCODE_SUCCESS);
  return 0; // Satisfy the compiler.
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help screen 			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    Does not return					      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

#if IGNORECASE != 0
#define MATCHCASEDEFAULT ""
#define IGNORECASEDEFAULT " (default)"
#else
#define MATCHCASEDEFAULT " (default)"
#define IGNORECASEDEFAULT ""
#endif

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
  dirc [SWITCHES] PATHNAME1 [PATHNAME2] [PATTERN]\n\
\n\
Pathname: directory_name[\\pattern]\n\
\n\
Pattern: An optional global wildcards pattern. Default: " PATTERN_ALL "\n\
If no pathname2 is entered, list only one directory, sorted.\n\
If no pathname1 is entered, list the current directory.\n\
\n\
Switches:\n\
  -?          Display this help message and exit.\n"
#ifdef _WIN32
"\
  -A          Force encoding the output using the ANSI character set.\n"
#endif
"\
  -b          Display only the files present in both directories.\n\
  -d          Display only files which are different.\n\
  -bd         Both.\n\
  -c          Compare the actual data of the files. May take a long time!\n\
  -ct         Compare data, and flag with a ~ equal files with different time.\n"
#ifdef _DEBUG
"\
  -D          Output debug information.\n"
#endif
"\
  -e          Stop when failing to enter a directory.\n\
  -E          Silently skip inaccessible directories. (default)\n"
#ifdef _MSDOS
"\
  -env VAR    Store the first file name found into environment variable VAR.\n"
#endif
"\
  -f          List files only, but not subdirectories.\n\
  -i          Ignore integer number of hours differences, up to +/- 23 hours.\n\
  -j          Ignore date/time completely.\n\
  -k          Consider case in file name comparisons." MATCHCASEDEFAULT "\n\
  -K          Ignore case in file name comparisons." IGNORECASEDEFAULT "\n\
  -L          Compare link targets, instead of the links themselves\n"
#ifdef _WIN32
"\
  -O          Force encoding the output using the OEM character set.\n"
#endif
"\
  -p          Pause for each page displayed.\n\
  -r          Same as {-d -f -s -z}\n\
  -s          Compare matching subdirectories too.\n\
  -t	      Display statistics about total number of files, sizes, etc.\n\
  -u	      Convert all displayed names to upper case.\n"
#ifdef _WIN32
"\
  -U          Force encoding the output using the UTF-8 character encoding.\n"
#endif
"\
  -v          Verbose mode\n\
  -V          Display this program version and exit.\n\
  -w COLS     Set the output width. Default: The display width.\n"
#ifdef _WIN32
"\
  -x          Display Short File Names.\n\
"
#endif
"\
  -z          Don't list a directory if no file is to appear in it.\n\
  -from Y/M/D List only files starting from that date. Also -from -D days.\n\
  -to Y/M/D   List only files up to that date. Also -to -D days.\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);

  finis(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    finis						      |
|									      |
|   Description:    Display opt. msgs; Restore current path and drv, and exit.|
|									      |
|   Parameters:     retcode	The exit code				      |
|		    pszFormat	Optional format string to pass to printf      |
|		    ...		Optional printf arguments		      |
|		    							      |
|   Notes:	    If retcode is not 0, but no error message is needed, it   |
|		    is necessary to pass a NULL for pszFormat.		      |
|		    							      |
|   Returns:	    Does not return					      |
|									      |
|   History:								      |
|    2018-04-30 JFL Redesigned with optional error messages to display.       |
*									      *
\*---------------------------------------------------------------------------*/

void finis(int retcode, ...) {
  if (retcode) { /* There might be an error message to report */
    va_list vl;
    char *pszFormat;

    va_start(vl, retcode);
    pszFormat = va_arg(vl, char *);
    if (pszFormat) { /* There is an error message to report */
      fprintf(stderr, "dirc: Error: ");
      vfprintf(stderr, pszFormat, vl);
      fputs(".\n", stderr);
    }
    va_end(vl);
  }

#if HAS_DRIVES
  _chdrive(init_drive);
#endif
  chdir(init_dir); /* Don't test errors, as we're likely to be here due to another error */

  exit(retcode);
}

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
#ifndef __unix__
	  || (*pszArg == '/')
#endif
	 ); /* It's a switch */
}

/******************************************************************************
*                                                                             *
*       Function:       lis                                                   *
*                                                                             *
*       Description:    Scan the directory, and fill the fif table            *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *startdir	Directory to scan. If "NUL", don't scan.      *
*         char *pattern		Wildcard pattern.                             *
*         int nfif		Number of files/directories found so far.     *
*         int col		1 = left column; 2 = right column.            *
*         int attrib		Bit 15: List directories exclusively.         *
*                       	Bits 7-0: File/directory attribute.           *
*         time_t datemin	Minimal date, or 0 if no minimum.             *
*         time_t datemax	Maximal date, or 0 if no maximum.             *
*         t_opts opts		User-defined options.                         *
*                                                                             *
*       Return value:   Total number of files/directories in fif array.       *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

int lis(char *startdir, char *pattern, int nfif, int col, int attrib,
	    time_t datemin, time_t datemax, t_opts opts) {
#if HAS_DRIVES
  char initdrive;                 /* Initial drive. Restored when done. */
#endif
  NEW_PATHNAME_BUF(initdir);	    /* Initial directory. Restored when done. */
  NEW_PATHNAME_BUF(path);	    /* Temporary pathname */
  NEW_PATHNAME_BUF(pathname);
  int err;
  char pattern2[NODENAME_SIZE];
  char *pname;
  DIR *pDir;
  struct dirent *pDirent;
  char *pcd;

  DEBUG_ENTER(("lis(\"%s\", \"%s\", %d, %d, 0x%X, 0x%lX, 0x%lX, 0x%X);\n", startdir, pattern,
	       nfif, col, attrib, (unsigned long)datemin, (unsigned long)datemax, opts));

#if PATHNAME_BUFS_IN_HEAP
  if ((!initdir) || (!path)) {
    FREE_PATHNAME_BUF(initdir);
    FREE_PATHNAME_BUF(path);
    FREE_PATHNAME_BUF(pathname);
    RETURN_INT_COMMENT(nfif, ("Out of memory\n"));
  }
#endif

  if (!stricmp(startdir, "nul")) {  /* Dummy name, used as place holder */
    FREE_PATHNAME_BUF(initdir);
    FREE_PATHNAME_BUF(path);
    FREE_PATHNAME_BUF(pathname);
    RETURN_INT_COMMENT(nfif, ("NUL\n"));
  }

  if (nfif == 0) firstfif = NULL; /* Make sure the linked list ends there */

  if (!pattern) pattern = PATTERN_ALL;
  strncpyz(pattern2, pattern, NODENAME_SIZE);

#if HAS_DRIVES
  initdrive = (char)_getdrive();

  if (startdir[1] == ':') {
    if (startdir[0] >= 'a') startdir[0] -= 'a' - 'A';
    DEBUG_PRINTF(("chdrive(%c);\n", startdir[0]));
    err = _chdrive(startdir[0] - '@');
    if (err) finis(RETCODE_INACCESSIBLE, "Cannot access drive %c", startdir[0]);
    startdir += 2;
  }
#endif

  pcd = getdir(initdir, PATHNAME_SIZE);
  if (!pcd) finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");

  if (startdir[0] == DIRSEPARATOR) {
    strncpyz(path, startdir, PATHNAME_SIZE);
  } else {
    char szSeparator[2];
    szSeparator[0] = DIRSEPARATOR;
    szSeparator[1] = '\0';
    /* Make the path absolute */
    strncpyz(path, initdir, PATHNAME_SIZE);
    if (path[1]) strcat(path, szSeparator);
    strcat(path, startdir);
  }

  err = FALSE;                        /* Assume success */
  if (startdir[0]) {
#if !HAS_MSVCLIBX
    DEBUG_PRINTF(("chdir(\"%s\");\n", path));
#endif
    err = chdir(path);
  }

  if (err) {
    char *pc;

    /* Directory not found. See if this is because of a file name pattern */
    pc = strrchr(path, DIRSEPARATOR);   /* Search for the trailing backslash */
    if (pc) {
      /* If found, assume a pattern follows */
      strncpyz(pattern2, pc+1, NODENAME_SIZE);

      if (pc > path) {		/* Remove the pattern. General case */
	  *pc = '\0';		/* Remove the backslash and wildcards */
      } else {			/* Special case of the root directory */
	  path[1] = '\0';	/* Same thing but leave the backslash */
      }

      DEBUG_PRINTF(("// Backtrack 1 level and split pattern\n"));
#if !HAS_MSVCLIBX
      DEBUG_PRINTF(("chdir(\"%s\");\n", path));
#endif
      err = chdir(path);
    }

    if (!pc || err) {
      if (opts.verbose || !opts.cont) {
	fprintf(stderr, "dirc: Error: Cannot access directory %s.\n", path);
      }
      if (opts.cont) {
	FREE_PATHNAME_BUF(initdir);
	FREE_PATHNAME_BUF(path);
	FREE_PATHNAME_BUF(pathname);
	RETURN_INT_COMMENT(nfif, ("Cannot access directory %s\n", path));
      }
      finis(RETCODE_INACCESSIBLE, NULL);
    }
  }

  pcd = getcwd(path, PATHNAME_SIZE);
  if (!pcd) finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");

  if (col == 1) {
    strncpyz(path1, path, PATHNAME_SIZE);
  } else {
    strncpyz(path2, path, PATHNAME_SIZE);
  }

  /* start looking for all files */
  pDir = opendir(path);
  if (pDir) {
    while ((pDirent = readdir(pDir)) != NULL) {
      struct stat st;
      DEBUG_CODE(
	char *reason;
	char szType[16];
	sprintf(szType, "d_type=%u", (unsigned)(pDirent->d_type));
      )

      makepathname(pathname, path, pDirent->d_name);
#if !_DIRENT2STAT_DEFINED
      pStat(pathname, &st);
      if (!pDirent->d_type) { /* Some filesystems don't set this field */
	/* Since we've called stat anyway, copy the type from there */
	if (S_ISREG(st.st_mode)) pDirent->d_type = DT_REG;
	if (S_ISDIR(st.st_mode)) pDirent->d_type = DT_DIR;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
	if (S_ISLNK(st.st_mode)) pDirent->d_type = DT_LNK;
#endif
#if defined(S_ISBLK) && S_ISBLK(S_IFBLK) /* In DOS it's defined, but always returns 0 */
	if (S_ISBLK(st.st_mode)) pDirent->d_type = DT_BLK;
#endif
#if defined(S_ISCHR) && S_ISCHR(S_IFCHR) /* In DOS it's defined, but always returns 0 */
	if (S_ISCHR(st.st_mode)) pDirent->d_type = DT_CHR;
#endif
#if defined(S_ISFIFO) && S_ISFIFO(S_IFFIFO) /* In DOS it's defined, but always returns 0 */
	if (S_ISFIFO(st.st_mode)) pDirent->d_type = DT_FIFO;
#endif
#if defined(S_ISSOCK) && S_ISSOCK(S_IFSOCK) /* In DOS it's defined, but always returns 0 */
	if (S_ISSOCK(st.st_mode)) pDirent->d_type = DT_SOCK;
#endif
      }
#else
      if (pStat == lstat) {
	dirent2stat(pDirent, &st);
      } else {
	stat(pathname, &st);
      }
#endif
      DEBUG_PRINTF(("// Found %10s %12s %lx\n",
	    (pDirent->d_type == DT_DIR) ? "Directory" :
	    (pDirent->d_type == DT_LNK) ? "Link" :
	    (pDirent->d_type == DT_REG) ? "File" :
	    szType,
	    pDirent->d_name,
	    (unsigned long)(st.st_mtime)));
      DEBUG_CODE(reason = "it's .";)
      if (    !streq(pDirent->d_name, ".")  /* skip . and .. */
	      DEBUG_CODE(&& ((reason = "it's ..") != NULL))
	   && !streq(pDirent->d_name, "..")
	      DEBUG_CODE(&& ((reason = "it's not a directory") != NULL))
	   && (   !(attrib & 0x8000)	  /* Skip files if dirs only */
		|| (pDirent->d_type == DT_DIR))
	      DEBUG_CODE(&& ((reason = "it's a directory") != NULL))
	   && (   (attrib & _A_SUBDIR)	  /* Skip dirs if files only */
		|| (pDirent->d_type != DT_DIR))
	      DEBUG_CODE(&& ((reason = "the date is out of range") != NULL))
	   && (st.st_mtime >= datemin) /* Skip files outside date range */
	   && (st.st_mtime <= datemax)
	      DEBUG_CODE(&& ((reason = "the pattern does not match") != NULL))
	   && (fnmatch(pattern2, pDirent->d_name, FNM_CASEFOLD) == FNM_MATCH)
	 ) {
	fif *pfif;

	DEBUG_PRINTF(("// OK\n"));
	pfif = (fif *)malloc(sizeof(fif));
	pname = strdup(pDirent->d_name);
	if (!pfif || !pname) {
	  closedir(pDir);
	  finis(RETCODE_NO_MEMORY, "Out of memory for directory access");
	}
	pfif->name = pname;
	pfif->st = st;
#if _MSVCLIBX_STAT_DEFINED
	DEBUG_PRINTF(("st.st_Win32Attrs = 0x%08X\n", pfif->st.st_Win32Attrs));
	DEBUG_PRINTF(("st.st_ReparseTag = 0x%08X\n", pfif->st.st_ReparseTag));
#endif /* _MSVCLIBX_STAT_DEFINED */
#ifndef _MSDOS
	pfif->target = NULL;
	if (pDirent->d_type == DT_LNK) {
	  char *pTarget = malloc(PATHNAME_SIZE);
	  int lTarget;
	  if (!pTarget) {
	    closedir(pDir);
	    finis(RETCODE_NO_MEMORY, "Out of memory");
	  }
	  lTarget = (int)readlink(pathname, pTarget, PATHNAME_SIZE);
	  if (lTarget != -1) {
	    pTarget[lTarget] = '\0';
	    pTarget = realloc(pTarget, lTarget+1);
	    pfif->target = pTarget;
	  } else {
	    free(pTarget);
	  }
	}
#endif
	// Note: The pointer to the name is copied as well.
	pfif->column = col;
	pfif->next = firstfif;
	firstfif = pfif;
	nfif += 1;
      } else {
	DEBUG_PRINTF(("// Ignored because %s\n", reason));
      }
    }

    closedir(pDir);
  }
#if !HAS_MSVCLIBX
  DEBUG_PRINTF(("chdir(\"%s\");\n", initdir));
#endif
  err = chdir(initdir);         /* Restore the initial directory */
  if (err) {
    finis(RETCODE_INACCESSIBLE, "Cannot return to directory \"%s\"\n:", initdir);
  }

#if HAS_DRIVES
  DEBUG_PRINTF(("chdrive(%c);\n", initdrive + '@'));
  _chdrive(initdrive);
#endif

  FREE_PATHNAME_BUF(initdir);
  FREE_PATHNAME_BUF(path);
  FREE_PATHNAME_BUF(pathname);
  RETURN_INT(nfif);
}

/******************************************************************************
*                                                                             *
*       Function:       cmpfif                                                *
*                                                                             *
*       Description:    Compare two files, for sorting the file list          *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         fif **fif1    First file structure                                  *
*         fif **fif2    Second file structure                                 *
*                                                                             *
*       Return value:    0 : Equal                                            *
*                       <0 : file1<file2                                      *
*                       >0 : file1>file2                                      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

int CDECL cmpfif(const fif **fif1, const fif **fif2, int ignorecase) {
  int ret;
  int bIsDir1, bIsDir2;

  /* List directories before files */
#if _MSVCLIBX_STAT_DEFINED
  bIsDir1 = (((*fif1)->st.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
  bIsDir2 = (((*fif2)->st.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
#else
  bIsDir1 = S_ISDIR((*fif1)->st.st_mode);
  bIsDir2 = S_ISDIR((*fif2)->st.st_mode);
#endif
  ret = bIsDir2 - bIsDir1;
  if (ret) return ret;

  /* If both files, or both directories, sort case-independantly */
  ret = strnicmp((*fif1)->name, (*fif2)->name, NODENAME_SIZE);
  if (ret) return ret;

  /* If same name except for the case, sort upper case first */
  if (!ignorecase) {  /* But do it only if requested */
    ret = strncmp((*fif1)->name, (*fif2)->name, NODENAME_SIZE);
    if (ret) return ret;
  }

  /* If same names, list column 1 before column 2 */
  return (*fif1)->column - (*fif2)->column;
}

int CDECL cmpfifCase(const fif **fif1, const fif **fif2) {
  return cmpfif(fif1, fif2, FALSE);
}

int CDECL cmpfifNoCase(const fif **fif1, const fif **fif2) {
  return cmpfif(fif1, fif2, TRUE);
}

typedef int (* CDECL CMPFUNC)(const void *p1, const void *p2); // Strict type for C++

void trie(fif **ppfif, int nfif, t_opts opts) {
  if (opts.nocase) {
    qsort(ppfif, nfif, sizeof(fif *), (CMPFUNC)cmpfifNoCase);
  } else {
    qsort(ppfif, nfif, sizeof(fif *), (CMPFUNC)cmpfifCase);
  }
}

/******************************************************************************
*                                                                             *
*       Function:       affiche                                               *
*                                                                             *
*       Description:    Display the files found side by side                  *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         fif **ppfif   Sorted array of file info structure pointers          *
*         int nfif      Number of files found (Total of both sides)           *
*         int ndirs     Number of directories  1 or 2                         *
*         t_opts opts	User-defined options		                      *
*                                                                             *
*       Return value:   0=Success; !0=Failure                                 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

int affiche(fif **ppfif, int nfif, int ndirs, t_opts opts) {
  int i;
  int col = 1;                      /* Current column */
  int difference;
  int nfiles = 0;
  int paths_done = FALSE;

  DEBUG_ENTER(("affiche(...);\n"));

  for (i=0; i<nfif; i++) {
    difference = CompareToNext(ppfif+i, opts); /* Compare ith file date with (i+1)th */

    if (opts.diff && (difference == 0)) {
      i += 1;
      continue;                   /* skip both if files match */
    }

    if (opts.both && (col == 1) && (difference == MISMATCH)) {
      continue;                    /* If both and no matching file, skip */
    }

    /* Ready to display the first file */

    if (!paths_done) {             /* But diplay the paths names first */
      affichePaths();
      paths_done = TRUE;
    }

    if ((col == 1) && (ppfif[i]->column == 2)) {
      if (opts.both) continue; /* If both and no matching file, skip */
      affiche1(NULL, 1, opts);
      printf(" < ");
      col = 2;
    }

    affiche1(ppfif[i], col, opts); /* Display file characteristics */

    /* Compute statistics about files displayed */

    if (col == 1) {
      lLFileFound += 1;
      llLTotalSize += ppfif[i]->st.st_size;
      if (!difference) {
	lEFileFound += 1;
	llETotalSize += ppfif[i]->st.st_size;
      }
    } else {
      lRFileFound += 1;
      llRTotalSize += ppfif[i]->st.st_size;
    }

    /* Display the comparison results */

    if (ndirs == 1) {	       /* If one directory, go to next line */
      nfiles += 1;
      printflf();
      continue;
    }

    if (col == 1) {
      col = 2;
      switch (difference) {
	case 0:
	  printf(" = ");
	  break;
	case 1:
	  printf(" > ");
	  break;
	case -1:
	  printf(" < ");
	  break;
	case DATE_MISMATCH:
	  printf(" ~ ");
	  break;
	case MISMATCH:
	  nfiles += 1;
	  printf(" >");
	  printflf();
	  col = 1;
	  break;
	default:
	  nfiles += 1;
	  printf(" ?!?");
	  printflf();
	  col = 1;
	  break;
      }
    } else {
      nfiles += 1;
      printflf();
      col = 1;
    }
  }

  if (col == 2) {
    nfiles += 1;
    printflf();
  }

  if (opts.zero && !nfiles) RETURN_CONST(0);

  if (!paths_done) affichePaths();
  printflf();
  printf("%d files or directories listed.", nfiles);
  printflf();
  lNFileFound += nfiles;

  RETURN_CONST(0);
}

void affichePaths(void) {
  int l;
  int iColumnSize = (iCols/2) - 2;

  printflf();

  l = CountCharacters(path1, CP_UTF8);
  printf("%s", path1);
  if (path2[0]) {
    if (l <= iColumnSize) {
	printf("%*s", iColumnSize - l, "");
    } else {
      printflf();
      printf("%*s", iColumnSize, "");
    }

    l = CountCharacters(path2, CP_UTF8);
    if (l <= iColumnSize) {
      printf(" | %*s", iColumnSize - l, "");
    } else {
      int iCols2 = iCols-(l+1);
      printf(" |");
      printflf();
      if (iCols2 > 0) printf("%*s", iCols2, "");
    }
    printf("%s", path2);
  }

  printflf();
  printflf();
}

int affiche1(fif *pfif, int col, t_opts opts) {
  int seconde;
  int minute;
  int heure;
  int jour;
  int mois;
  int an;
  char nicename[NODENAME_SIZE+6]; /* 3 for prefix and 3 for suffix */
  char *pNicename = nicename+3;   /* Leave room for prefix */
  int iShowSize = 1;
  int l;
  int iNamePlusSize = iNameWidth + iSizeWidth;
  int iColumnSize = (iCols/2) - 2;
  char szSize[22] = ""; /* Buffer for the file size string, good for 64-bit sizes */
  int nSize = 0;        /* Size of the szSize string */
  struct tm *pTime;

  DEBUG_ENTER(("affiche1(%p, %d, 0x%X);\n", pfif, col, opts));

  if (!pfif) {    /* Display spaces occupying same length as normal output */
    printf("%*s", iColumnSize, "");
    RETURN_CONST(0);
  }

  pTime = LocalFileTime(&(pfif->st.st_mtime)); // Time of last data modification
  seconde = pTime->tm_sec;
  minute = pTime->tm_min;
  heure = pTime->tm_hour;
  jour = pTime->tm_mday;
  mois = pTime->tm_mon + 1;
  an = pTime->tm_year + 1900;

  strncpyz(pNicename, pfif->name, NODENAME_SIZE);
  // ~~jfl 1995-01-17 Removed strlwr to distinguish case in file names
#ifdef _MSDOS
  strlwr(pNicename);                   /* More readable in lower case */
#endif
  if (opts.upper) strupr(pNicename);	/* Do just the opposite if requested */

  /* Output the name */
  if (S_ISDIR(pfif->st.st_mode)) {
#if 1
#if defined(__unix__)
    { /* Append an OS-dependant directory separator */
      int n = (int)strlen(pNicename);
      pNicename[n++] = DIRSEPARATOR;
      pNicename[n] = '\0';
    }
#endif /* defined(__unix__) */
#else  /* Experiment with [brackets] around the directory name for Windows */
#if defined(__unix__)
    strcat(pNicename, "/");
#else
    *(--pNicename) = '[';
    strcat(pNicename, "]");
#endif /* defined(__unix__) */
#endif /* 1 */
    iShowSize = 0;
  }
  if (   S_ISCHR(pfif->st.st_mode)
#if defined(S_ISBLK) && S_ISBLK(S_IFBLK) /* In DOS it's defined, but always returns 0 */
      || S_ISBLK(pfif->st.st_mode)
#endif // defined(S_ISBLK)
     ) {
    // strcat(pNicename, " !");
    iShowSize = 0;
  }
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(pfif->st.st_mode)) {
#if 0 && defined(_WIN32) && _MSVCLIBX_STAT_DEFINED
    if ((pfif->st.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
      strcat(pNicename, "\\"); /* Junctions and symlinkds behave like directories in Windows */
    }
#endif
    strcat(pNicename, " -> ");
    if (pfif->target) strcat(pNicename, pfif->target);
    iShowSize = 0; /* Ignore the size for links. In Windows, it's always 0. In Linux it's the link name size. */
  }
#endif // defined(S_ISLNK)
#if defined(S_ISFIFO) && S_ISFIFO(S_IFIFO) /* In DOS it's defined, but always returns 0 */
  if (S_ISFIFO(pfif->st.st_mode)) {
    strcat(pNicename, "|");
    iShowSize = 0;
  }
#endif // defined(S_ISFIFO)
#if defined(S_ISSOCK) && S_ISSOCK(S_IFSOCK) /* In DOS it's defined, but always returns 0 */
  if (S_ISSOCK(pfif->st.st_mode)) {
    strcat(pNicename, "=");
    iShowSize = 0;
  }
#endif // defined(S_ISSOCK)
  l = printf("%s", pNicename);	   /* Returns the number of bytes in the name */
#if !defined(_MSDOS)	/* For DOS, the # of bytes is the same as the # of characters */ 
  l = CountCharacters(pNicename, CP_UTF8);
#endif

  /* Output the size */
  if (iShowSize) { /* This is a normal file, and we need to display the size */
    int nBytes = sizeof(pfif->st.st_size); /* Could this be made a compile-time constant? */
    char *pszFormat = (nBytes == 4) ? "%"PRIu32 : "%"PRIu64;
    nSize = sprintf(szSize, pszFormat, pfif->st.st_size);
  } else {         /* This is a special file, do not display a size */
#if !defined(__unix__)
    if (S_ISDIR(pfif->st.st_mode)) { // This is a directory
#if defined(_WIN32)
      nSize = sprintf(szSize, "<DIR>     "); // Add 5 spaces to align with <JUNCTION> and <SYMLINKD>
#elif defined(_MSDOS)
      nSize = sprintf(szSize, "<DIR>  "); // Allow fitting output in 80 columns
#endif
    }
#endif

    if (S_ISCHR(pfif->st.st_mode)) {
      nSize = sprintf(szSize, "<CHARDEV>"); // This is a character device
    }
#if defined(S_ISBLK) && S_ISBLK(S_IFBLK) /* In DOS it's defined, but always returns 0 */
    if (S_ISBLK(pfif->st.st_mode)) {
      nSize = sprintf(szSize, "<BLCKDEV>"); // This is a block device
    }
#endif // defined(S_ISBLK)

#if defined(_WIN32) && _MSVCLIBX_STAT_DEFINED
    if (S_ISLNK(pfif->st.st_mode)) {
      if (pfif->st.st_ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
	nSize = sprintf(szSize, "<JUNCTION>"); // This is a junction
      } else if (pfif->st.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) {
	nSize = sprintf(szSize, "<SYMLINKD>"); // This is a symlinkd
      }
    }
#endif

#if defined(S_ISFIFO) && S_ISFIFO(S_IFIFO) /* In DOS it's defined, but always returns 0 */
    if (S_ISFIFO(pfif->st.st_mode)) {
      nSize = sprintf(szSize, "<FIFO>   "); // This is a fifo
    }
#endif // defined(S_ISFIFO)
#if defined(S_ISSOCK) && S_ISSOCK(S_IFSOCK) /* In DOS it's defined, but always returns 0 */
    if (S_ISSOCK(pfif->st.st_mode)) {
      nSize = sprintf(szSize, "<SOCKET> "); // This is a network socket
    }
#endif // defined(S_ISSOCK)
  }
  /* Make sure name+size fits in the column */
  if ((l+1+nSize) > iNamePlusSize) { /* It does not. Output the size on the next line */ 
    printflf();
    l = 0; /* Current offset on the next line */
    if (col == 2) printf("%*s", iColumnSize+3, ""); /* + 3 for separation */
  }
  printf("%*s %s", iNamePlusSize-(l+1+nSize), "", szSize);

  /* Output the date and time */
  if (iYearWidth == 2) {
    printf(" %02d-%02d-%02d %02d:%02d:%02d",
	    an % 100, mois, jour, heure, minute, seconde);
  } else {
    printf(" %04d-%02d-%02d %02d:%02d:%02d",
	    an, mois, jour, heure, minute, seconde);
  }
  RETURN_CONST(0);
}

/******************************************************************************
*                                                                             *
*       Function:       CompareToNext                                         *
*                                                                             *
*       Description:    Compare a file date and time with the next entry      *
*                                                                             *
*       Arguments:                                                            *
*         fif **ppfif   Sorted array of file info structure pointers          *
*         t_opts opts	User-defined options		                      *
*                                                                             *
*       Return value:   0=Same file; <0 Older than next; >0 Newer than next.  *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

int CompareToNext(fif **ppfif, t_opts opts) { /* Compare file date with next entry in fiflist */
  long deltatime;                 /* Date and Time difference, in seconds */
  int deltasize;                  /* Sign of the difference, or 0 if equal */
  fif *pfif1;
  fif *pfif2;
  int dif;

  pfif1 = ppfif[0];
  pfif2 = ppfif[1];
  DEBUG_ENTER(("CompareToNext(%p, 0x%X); // \"%s\" / \"%s\"\n", ppfif, opts, pfif1->name, pfif2?pfif2->name:""));
  if (!pfif2) DEBUG_RETURN_INT(MISMATCH, "No next entry");	/* No next entry */

  /* ~~jfl 95/06/12 Can't compare a file to a directory */
  dif = S_ISDIR(pfif1->st.st_mode);
  dif ^= S_ISDIR(pfif2->st.st_mode);
  if (dif) DEBUG_RETURN_INT(MISMATCH, "Types differ");

  /* Compare names, with or without case depending on command */
  if (opts.nocase) {
    dif = stricmp(pfif1->name, pfif2->name);
  } else {
    dif = strcmp(pfif1->name, pfif2->name);
  }
  if (dif) DEBUG_RETURN_INT(MISMATCH, "Names differ");	/* Names don't match */

  deltatime = (long)pfif1->st.st_mtime;
  deltatime -= (long)pfif2->st.st_mtime;

  if (pfif1->st.st_size < pfif2->st.st_size) {
    deltasize = -1;
  } else if (pfif1->st.st_size > pfif2->st.st_size) {
    deltasize = 1;
  } else {
    deltasize = 0;
  }

  /* If in filecomp mode, check if same data files with different dates */
  if (opts.compare && !deltasize && !S_ISDIR(pfif1->st.st_mode)) { /* Let the actual data decide */
    NEW_PATHNAME_BUF(name1);
    NEW_PATHNAME_BUF(name2);

    makepathname(name1, path1, pfif1->name);
    makepathname(name2, path2, pfif2->name);
    dif = filecompare(name1, name2);
    FREE_PATHNAME_BUF(name1);
    FREE_PATHNAME_BUF(name2);
    if (!dif) {
      if (deltatime && opts.dtime) {
	DEBUG_RETURN_INT(DATE_MISMATCH, "Contents are identical but dates differ");
      }
      DEBUG_RETURN_INT(0, "Contents are identical"); /* They are actually identical */
    }
    if (deltatime) DEBUG_RETURN_INT(SIGN(deltatime), "Timestamps and contents differ");
    DEBUG_RETURN_INT(SIGN(dif), "Contents differ");
  }

  /* If same file name, compare date and time */
  if (!opts.notime && deltatime) {     /* If comparison expected, and there is a difference */
    int sign;               /* Sign of deltatime */

    sign = (deltatime > 0) ? 1 : -1;        /* Sign of the difference */

    if (opts.notz) {   /* Special case if we wish to ignore time-zone differences */
      /* Ignore differences that are an integer number of hours  less than 24 hours */
      int deltaseconds;

      deltaseconds = (int)(deltatime % 3600);
      if (deltaseconds < 0) deltaseconds += 3600;
      deltatime /= 3600;                   /* Convert difference to hours */
      if ((deltatime > 23) || (deltatime < -23))
	  DEBUG_RETURN_INT(sign, "Timestamps differ > 1 day"); /* More than 1 day of diff. */
      switch (deltaseconds) { // ~~jfl 1996-09-16
	case 0:
	case 2:
	case 3600-2:
	/* Assume the difference isn't meaningful. Go compare the size. */
	break;
	default:
	DEBUG_RETURN_INT(sign, "Timestamps differ > 2 seconds");
      }
    } else {      /* Common case where any date & time difference counts */
      DEBUG_RETURN_INT(sign, "Timestamps differ");
    }
  }

  /* If same date & time, compare size */
  if (deltasize) DEBUG_RETURN_INT(deltasize, "Sizes differ (yet times match)");

  DEBUG_RETURN_INT(0, "Same files");
}

/******************************************************************************
*                                                                             *
*       Function:       filecompare                                           *
*                                                                             *
*       Description:    Compare the contents of two files                     *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *name1   Pathname of first file                                *
*         char *name2   Pathname of second file                               *
*                                                                             *
*       Return value:   0=Same contents                                       *
*                       1/-1=Length difference                                *
*                       2/-2=Data difference                                  *
*                       3/-3=One of the files is missing                      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*        1995-06-12 JFL Made this routine generic (Independant of DIRC)       *
*        2014-01-21 JFL Use a much larger buffer for 32-bits apps, to improve *
*                       performance.                                          *
*                                                                             *
******************************************************************************/

#ifdef _MSDOS		/* If it's a 16-bits app, use a 4K buffer. */
#define FBUFSIZE 4096
#else			/* Else for 32-bits or 64-bits apps, use a 4M buffer */
#define FBUFSIZE (4096 * 1024)
#endif

int filecompare(char *name1, char *name2) { /* Compare two files */
  static char *pbuf1 = NULL;
  static char *pbuf2 = NULL;
  FILE *f1;
  FILE *f2;
  size_t l1, l2;
  int dif;

  DEBUG_ENTER(("filecompare(\"%s\", \"%s\");\n", name1, name2));

  if (!pbuf1) {
    pbuf1 = (char *)malloc(FBUFSIZE);
    pbuf2 = (char *)malloc(FBUFSIZE);
    if (!pbuf2) {
      finis(RETCODE_NO_MEMORY, "Out of memory");   /* Note: This is still DIRC-specific */
    }
  }

  /* For links, compare the link targets */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  {
    int err1, err2;
    struct stat st1;
    struct stat st2;
    err1 = lstat(name1, &st1);
    err2 = lstat(name2, &st2);
    if ((!err1) && S_ISLNK(st1.st_mode) && (!err2) && S_ISLNK(st2.st_mode)) {
#if defined(_WIN32) && _MSVCLIBX_STAT_DEFINED
      if ((st1.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) && (st2.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY))
#else
      err1 = stat(name1, &st1);
      err2 = stat(name2, &st2);
      if (err1 && err2) RETURN_INT_COMMENT(0, ("Both dead links. Ignore.\n"));
      if (err1) RETURN_INT_COMMENT(-3, ("The first link is dead.\n"));
      if (err2) RETURN_INT_COMMENT( 3, ("The second link is dead.\n"));
      if (S_ISDIR(st1.st_mode) && S_ISDIR(st2.st_mode))
#endif
	{
	int n1 = (int)readlink(name1, pbuf1, FBUFSIZE);
	int n2 = (int)readlink(name2, pbuf2, FBUFSIZE);
	if ((n1 == -1) && (n2 == -1)) RETURN_INT_COMMENT(0, ("Both dead links. Ignore.\n"));
	if (n1 == -1) RETURN_INT_COMMENT(-3, ("The first link is dead.\n"));
	if (n2 == -1) RETURN_INT_COMMENT( 3, ("The second link is dead.\n"));
	pbuf1[n1] = '\0';
	pbuf2[n2] = '\0';
	dif = strcmp(pbuf1, pbuf2);
	RETURN_INT_COMMENT(dif, ("Link targets are %s\n", dif ? "different" : "identical"));
      }
    }
  }
#endif // OS supporting links

  /* For files or links to files, compare the data itself */
  f1 = fopen(name1, "rb");
  f2 = fopen(name2, "rb");
  if ((!f1) && (!f2)) RETURN_INT_COMMENT(0, ("Neither file exists.\n"));
  if (!f1) {
    fclose(f2);
    RETURN_INT_COMMENT(-3, ("The first file does not exist.\n"));
  }
  if (!f2) {
    fclose(f1);
    RETURN_INT_COMMENT( 3, ("The second file does not exist.\n"));
  }

  dif = 0;
  while ((l1 = fread(pbuf1, 1, FBUFSIZE, f1)) != 0) {
    l2 = fread(pbuf2, 1, FBUFSIZE, f2);
    if (l1 > l2) {dif = 1; break;}
    if (l1 < l2) {dif = -1; break;}
    dif = memcmp(pbuf1, pbuf2, l1);
    if (dif) {
      dif = (dif > 0) ? 2 : -2;
      break;   /* If different data found, return immediately */
    }
  }

  fclose(f1);
  fclose(f2);

  RETURN_INT_COMMENT(dif, ("Files are %s\n", dif ? "different" : "identical"));
}

/******************************************************************************
*                                                                             *
*       Function:       descend                                               *
*                                                                             *
*       Description:    Go down the directory trees recursively               *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *from		First directory to list.                      *
*         char *to		Second directory to list, or NULL.            *
*         char *switches	Switch string to pass to next level.          *
*         int attrib		Search attribute                              *
*         t_opts opts		User-defined options	                      *
*	  time_t datemin	First date to consider			      *
*	  time_t datemax	Last date to consider			      *
*                                                                             *
*       Return value:   0=Success; !0=Failure                                 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	 1993-10-15 JFL  Initial implementation 			      *
*	 1994-03-17 JFL  Rewritten to recurse within the same appli. instance.*
*                                                                             *
******************************************************************************/

int descend(char *from, char *to, char *pattern,
                int attrib, t_opts opts,
		time_t datemin, time_t datemax) {
  int nfif;
  int i;
  fif **directories;
  fif **ppfif;
  uint16_t wFlags = 0x8000 | _A_SUBDIR | _A_SYSTEM | _A_HIDDEN;
  NEW_PATHNAME_BUF(name1);
  NEW_PATHNAME_BUF(name2);

  DEBUG_ENTER(("descend(\"%s\", \"%s\", \"%s\", 0x%X, 0x%X, 0x%lX, 0x%lX);\n", from, to, pattern,
	       attrib, opts, (unsigned long)datemin, (unsigned long)datemax));

#if PATHNAME_BUFS_IN_HEAP
  if ((!name1) || (!name2)) {
    FREE_PATHNAME_BUF(name1);
    FREE_PATHNAME_BUF(name2);
    RETURN_CONST_COMMENT(0, ("Out of memory\n"));
  }
#endif

  /* Get all subdirectories */
  nfif = 0;
  if (from) nfif = lis(from, PATTERN_ALL, nfif, 1, wFlags, 0, TIME_T_MAX, opts);
  if (to) nfif = lis(to, PATTERN_ALL, nfif, 2, wFlags, 0, TIME_T_MAX, opts);
  directories = AllocFifArray(nfif);
  trie(directories, nfif, opts);

  for (i=0; i<nfif; i++) {
    char *pname1;
    char *pname2;
    char *pn1;
    int nfif2;
    int ndir;

    pn1 = directories[i]->name;
    path1[0] = path2[0] = '\0'; /* Cleanup static title buffers */
    pname1 = pname2 = NULL;
    ndir = 1;
    if (from) {
      makepathname(name1, from, pn1);
      pname1 = name1;
      DEBUG_PRINTF(("// Descent possible into %s\n", name1));
    }
    name2[0] = '\0';
    if (to) {
      makepathname(name2, to, pn1);
      pname2 = name2;
      ndir = 2;
      DEBUG_PRINTF(("// and into %s\n", name2));
    }
    if (   to
	 && ((i+1) < nfif)
	 && (opts.nocase ?
		!stricmp(directories[i]->name, directories[i+1]->name)
	      : streq(directories[i]->name, directories[i+1]->name)  ) ) {
      /* Both subdirectories match */
      i += 1;
      nfif2 = lis(name1, pattern, 0, 1, attrib, datemin, datemax, opts);
      if (to) nfif2 = lis(name2, pattern, nfif2, 2, attrib, datemin, datemax, opts);
      ppfif = AllocFifArray(nfif2);
      trie(ppfif, nfif2, opts);
      affiche(ppfif, nfif2, ndir, opts);
      FreeFifArray(ppfif);

      descend(pname1, pname2, pattern, attrib, opts, datemin, datemax);
    } else if (!opts.both) {
      DEBUG_PRINTF(("// There is no directory %s",
		 (directories[i]->column == 1) ? name2 : name1));
      if (directories[i]->column == 1) {
	pname2 = NULL;
	nfif2 = lis(name1, pattern, 0, 1, attrib, datemin, datemax, opts);
      } else {
	pname1 = NULL;
	nfif2 = lis(name2, pattern, 0, 2, attrib, datemin, datemax, opts);
      }
      ppfif = AllocFifArray(nfif2);
      trie(ppfif, nfif2, opts);
      affiche(ppfif, nfif2, ndir, opts);
      FreeFifArray(ppfif);

      descend(pname1, pname2, pattern, attrib, opts, datemin, datemax);
    }
  } /* End for */

  FreeFifArray(directories);
  FREE_PATHNAME_BUF(name1);
  FREE_PATHNAME_BUF(name2);
  RETURN_CONST(0);
}

/******************************************************************************
*                                                                             *
*       Function:       AllocFifArray                                         *
*                                                                             *
*       Description:    Allocate an array of fif pointers, to be sorted.      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         int nfif      Number of fif structures.                             *
*                                                                             *
*       Return value:   The array address. Aborts the program if failure.     *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

fif **AllocFifArray(size_t nfif) {
  fif **ppfif;
  fif *pfif;
  size_t i;

  /* Allocate an array for sorting */
  ppfif = (fif **)malloc((nfif+1) * sizeof(fif *));
  if (!ppfif) finis(RETCODE_NO_MEMORY, "Out of memory for fif array");
  /* Fill the array with pointers to the list of structures */
  pfif = firstfif;
  for (i=0; i<nfif; i++) {
    ppfif[i] = pfif;
    pfif = pfif->next;
  }
  /* Make sure following the linked list ends with the array */
  if (nfif) ppfif[nfif-1]->next = NULL;
  /* Make sure FreeFifArray works in all cases. */
  ppfif[nfif] = NULL;

  return ppfif;
}

/******************************************************************************
*                                                                             *
*       Function:       FreeFifArray                                          *
*                                                                             *
*       Description:    Free an array of fif pointers, and the fifs too.      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         fif **pfif    Pointer to the fif array.                             *
*                                                                             *
*       Return value:   None                                                  *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

void FreeFifArray(fif **ppfif) {
  fif **ppf;

  for (ppf=ppfif; *ppf; ppf++) {
    free((*ppf)->name);
#ifndef _MSDOS
    free((*ppf)->target);
#endif
    free(*ppf);
  }

  free(ppfif);

  return;
}

/******************************************************************************
*                                                                             *
*       Function:       makepathname                                          *
*                                                                             *
*       Description:    Build a full pathname from a directory & node names   *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *buf     Buffer where to store the full name                   *
*         char *path    Pathname                                              *
*         char *node    Node name                                             *
*                                                                             *
*       Return value:   0=Success; !0=Failure                                 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	1994-03-18 JFL Do not append a \ after ':', such as in "C:".	      *
*                                                                             *
******************************************************************************/

int makepathname(char *buf, char *path, char *node) {
  int l;

  strncpyz(buf, path, PATHNAME_SIZE - 1);
  l = (int)strlen(buf);
  if (l && (buf[l-1] != DIRSEPARATOR) && (buf[l-1] != ':')) {
    buf[l++] = DIRSEPARATOR;
    buf[l] = 0;
  }
  strncpyz(buf+l, node, PATHNAME_SIZE - l);
  return 0;
}

/*****************************************************************************\
*                                                                             *
*   Function:       getdir                                                    *
*                                                                             *
*   Description:    Get the current directory, without the DOS/Windows drive. *
*                                                                             *
*   Arguments:                                                                *
*                                                                             *
*     char *buf     Buffer for the directory name                             *
*     int len       Size of the above buffer                                  *
*                                                                             *
*   Return value:   The buffer address if success, or NULL if failure.        *
*                                                                             *
*   Notes:                                                                    *
*                                                                             *
*   History:                                                                  *
*    2005-05-03 JFL Added support for Unix.                                   *
*                                                                             *
\*****************************************************************************/

char *getdir(char *buf, int len) { /* Get current directory without the drive */
  NEW_PATHNAME_BUF(line);
  char *pszRoot = line;

#if PATHNAME_BUFS_IN_HEAP
  if (!line) return NULL;
#endif

  if (!getcwd(line, PATHNAME_SIZE)) {
    FREE_PATHNAME_BUF(line);
    return NULL;
  }

  if (len > PATHNAME_SIZE) len = PATHNAME_SIZE;

  if (line[1] == ':') pszRoot = line+2;
  strncpyz(buf, pszRoot, len);  /* Copy path without the drive letter */

  FREE_PATHNAME_BUF(line);
  return buf;
  }

/*****************************************************************************\
*                                                                             *
*   Function:       parse_date                                                *
*                                                                             *
*   Description:    Parse a date on a command line switch                     *
*                                                                             *
*   Arguments:                                                                *
*                                                                             *
*     char *token	Date token from the command line                      *
*     time_t *pdate	Pointer to the date field to update	   	      *
*                                                                             *
*   Return value:   TRUE if done, FALSE if invalid date found                 *
*                                                                             *
*   Notes:                                                                    *
*                                                                             *
*   History:                                                                  *
*    2012-04-30 JFL Allow dates formatted either as YYYY/MM/DD or YYYY-MM-DD. *
*    2014-02-14 JFL Changed the output to a time_t.                           *
*    2014-02-20 JFL Allow negative number of days relative to today.          *
*                                                                             *
\*****************************************************************************/

int parse_date(char *token, time_t *pdate) {
  int day, month, year;
  char c[8]; /* We expect 1 char + 1 NUL, but leave extra space in case of misformed dates */
  struct tm sTM = {0};

  if (*token == '-') { /* Assume an integer number of days from 00:00:00 today */
    if (!sscanf(token+1, "%d", &day)) return FALSE;
    *pdate = time(NULL) - 86400 * (time_t)day;
    *pdate -= (*pdate % 86400);
    return TRUE;
  }
  if (sscanf(token, "%d%[-/]%d%[-/]%d", &year, &c[0], &month, &c[0], &day) < 5) return FALSE;

  // Compute the year relative to 1970, allowing (70-99,0-69) = 1970-2069
  if (year < 0) return FALSE;
  if (year < 100) {
    year += 30; // year -= 70; year += 100;
    year %= 100;    // Result valid from 1970 to 2069
    year += 1970;
  }
  if (year < 1970) return FALSE;

  if (month < 1) return FALSE;
  if (month > 12) return FALSE;

  if (day < 1) return FALSE;
  if (day > 31) return FALSE;
  
  sTM.tm_year = year - 1900;	/* tm years are 1900-based */
  sTM.tm_mon = month - 1;	/* tm months are 0-based */
  sTM.tm_mday = day;		/* tm days are 1-based. Logic, isn't it? */

  *pdate = mktime(&sTM);
  return (*pdate != (time_t)-1L);
}

/******************************************************************************
*                                                                             *
*	Function:	GetScreenRows					      *
*                                                                             *
*	Description:	Get the number of rows on the text screen	      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	 None								      *
*                                                                             *
*	Return value:	Per the function definition			      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*                               OS/2 Version                                  *
*                                                                             *
******************************************************************************/

#ifdef _OS2

/* Make sure to include os2.h at the beginning of this file, and before that
    to define the INCL_VIO constant to enable the necessary section */

int GetScreenRows(void) {
  VIOMODEINFO vmi;

  VioGetMode(&vmi, 0);

  return vmi.row;
}

#endif

/******************************************************************************
*                                                                             *
*                               WIN32 Version                                 *
*                                                                             *
******************************************************************************/

#ifdef _WIN32

/* Make sure to include windows.h at the beginning of this file, and especially
    the kernel section */

int GetScreenRows(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    return 0;   /* Disable pause mode if console size unknown */

  return csbi.srWindow.Bottom + 1 - csbi.srWindow.Top;
}

int GetScreenColumns(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    return 0;   /* Disable pause mode if console size unknown */

  return csbi.srWindow.Right + 1 - csbi.srWindow.Left;
}

#endif

/******************************************************************************
*                                                                             *
*                               MS_DOS Version                                *
*                                                                             *
******************************************************************************/

#ifdef _MSDOS

int GetScreenRows(void) {
  uint8_t far *fpc;

  fpc = (uint8_t far *)0x00400084L;	// *fpc = Index of the last row
  return *fpc + 1;               	// Number of rows
}

int GetScreenColumns(void) {
  return *(int far *)0x0040004AL;
}

#endif

/*****************************************************************************\
*									      *
*				 Unix Version				      *
*									      *
\*****************************************************************************/

/* Requires linking with the  -ltermcap option */

#ifdef __unix__

#if defined(USE_TERMCAP) && USE_TERMCAP

#include <termcap.h>

static char term_buffer[2048];
static int tbInitDone = FALSE;

int init_terminal_data() {
  char *termtype;
  int success;

  if (tbInitDone) return 0;

  termtype = getenv ("TERM");
  if (termtype == 0) {
    printf("Specify a terminal type with `setenv TERM <yourtype>'.\n");
    exit(1);
  }

  success = tgetent (term_buffer, termtype);
  if (success < 0) {
    printf("Could not access the termcap data base.\n");
    exit(1);
  }
  if (success == 0) {
    printf("Terminal type `%s' is not defined.\n", termtype);
    exit(1);
  }

  tbInitDone = TRUE;
  return 0;
}

int GetScreenRows(void) {
  init_terminal_data();
  return tgetnum("li");
}

int GetScreenColumns(void) {
  init_terminal_data();
  return tgetnum ("co");
}

#else

extern int errno;

/* Execute a command, and capture its output */
#define TEMP_BLOCK_SIZE 1024
char *Exec(char *pszCmd) {
  size_t nBufSize = 0;
  char *pszBuf = malloc(0);
  size_t nRead = 0;
  int iPid = getpid();
  char szTempFile[32];
  char *pszCmd2 = malloc(strlen(pszCmd) + 32);
  int iErr;
  FILE *hFile;
  sprintf(szTempFile, "/tmp/RowCols.%d", iPid);
  sprintf(pszCmd2, "%s >%s", pszCmd, szTempFile);
  iErr = system(pszCmd2);
  if (iErr) {
    free(pszBuf);
    return NULL;
  }
  /* Read the temp file contents */
  hFile = fopen(szTempFile, "r");
  while (1) {
    char *pszBuf2 = realloc(pszBuf, nBufSize + TEMP_BLOCK_SIZE);
    if (!pszBuf2) break;
    pszBuf = pszBuf2;
    nRead = fread(pszBuf+nBufSize, 1, TEMP_BLOCK_SIZE, hFile);
    nBufSize += TEMP_BLOCK_SIZE;
    if (nRead < TEMP_BLOCK_SIZE) break;
    if (feof(hFile)) break;
  }
  fclose(hFile);
  /* Cleanup */
  remove(szTempFile);
  free(pszCmd2);
  return pszBuf; /* Must be freed by the caller */
}

int GetScreenRows(void) {
  int nRows = 25; // Default for VGA screens
  // char *pszRows = getenv("LINES");
  // if (pszRows) nRows = atoi(pszRows);
  char *pszBuf = Exec("tput lines");
  if (pszBuf) {
    nRows = atoi(pszBuf);
    free(pszBuf);
  }
  return nRows;
}

int GetScreenColumns(void) {
  int nCols = 80; // Default for VGA screens
  // char *pszCols = getenv("COLUMNS");
  // if (pszCols) nCols = atoi(pszCols);
  char *pszBuf = Exec("tput cols");
  if (pszBuf) {
    nCols = atoi(pszBuf);
    free(pszBuf);
  }
  return nCols;
}

#endif

#endif

/******************************************************************************
*                                                                             *
*                         End of OS-specific routines                         *
*                                                                             *
******************************************************************************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|	Function:	printflf					      |
|                                                                             |
|	Description:	Print a line feed, and possibly pause on full screens |
|                                                                             |
|       Arguments:                                                            |
|                                                                             |
|	 None								      |
|                                                                             |
|	Return value:	None						      |
|                                                                             |
|       Notes:                                                                |
|                                                                             |
|   History:								      |
|    2008-01-25 JFL Added test for Control-C and ESC.                         |
*                                                                             *
\*---------------------------------------------------------------------------*/

void printflf(void) {
  static int nlines = 0;
  int c;
  
  printf("\n");
  
  if (!iPause) return;
  
  nlines += 1;
  if (nlines < iPause) return;
  nlines = 0;
  
  fflush(stdin);		/* Flush any leftover characters */
  printf("Press any key to continue... ");
  c = _getch();			/* Pause until a key is pressed */
  fflush(stdin);		/* Flush any additional characters that may be left */
  printf("\r                                   \r");
  if (c==3 || c==27) finis(0);	/* Ctrl-C or ESC pressed. Exit immediately. */
  return;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetPsp						      |
|									      |
|   Description:    Get the program segment prefix for the current process    |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    The PSP segment					      |
|									      |
|   History:								      |
|									      |
|    1995-06-19 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

#pragma warning(disable:4704)	// Ignore the inline assembler warning

uint16_t GetPsp(void) {
  register uint16_t psp;
  _asm {
    mov	ah, 51h
    int	21h
    mov	psp, bx
  }
  return psp;
// #pragma warning(disable:4035)	// Ignore the no return value warning
}
// #pragma warning(default:4035)
#pragma warning(default:4704)	// Restore the inline assembler warning

#endif // _MSDOS

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    SetMasterEnv					      |
|									      |
|   Description:    Set an environment variable in COMMAND.COM master envir.  |
|									      |
|   Parameters:     char *pszEnv	Environment variable name	      |
|		    char *pszValue	Its new value. If NULL or "", zap it! |
|									      |
|   Returns:	    0=Done; 1=Out of environment space. 		      |
|									      |
|   Notes:	    Function setenv() is useless here, as it modifies the     |
|		    local copy of the environment!			      |
|									      |
|   History:								      |
|									      |
|     1998-10-14 JFL Initial implementation.				      |
|     1998-10-20 JFL Write directly to the master environment.		      |
|     1999-02-12 JFL Added the ability to delete environment variables.       |
|     1999-02-15 JFL MS-DOS 7 does NOT change name to upper case.	      |
|     1999-06-29 JFL Fixed bug: Arena size is in paragraphs, not bytes.       |
|		     Also size read from wrong arena, causing crash under NT! |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

#define FARPTR(seg, off) ((void far *)(((uint32_t)(uint16_t)(seg) << 16) | (uint16_t)off))
#define FARWORD(seg, off) (*(uint16_t far *)FARPTR(seg, off))
#define WORD1(dw) (*((uint16_t *)(&(dw))+1))

int SetMasterEnv(char *pszName, char *pszValue) {
  uint16_t wPsp;		// PSP segment
  uint16_t wParentPsp;	// Parent process PSP segment
  uint16_t wEnv;		// Environment segment
  char far *lpEnv;		// Environment base address
  uint16_t wEnvSize;		// Environment size
  char far *lpName;		// Current variable name
  char far *lpc;
  char c;

  // ~~jfl 1999-02-15 Under MS-DOS 7, lower case variable names are possible.
  // strupr(pszName);    // MS-DOS SET command changes variable to upper case.

  // Walk up the parent chain until command.com, which is its own parent.
  for (wPsp = GetPsp();	// The Program Segment Prefix for this program
       wPsp != (wParentPsp = FARWORD(wPsp, 0x16));
       wPsp = wParentPsp) {	// Walk up our parent process' PSP
       DEBUG_PRINTF(("PSP=%04X, Parent at %04X\n", wPsp, wParentPsp));
     }

  // Get command.com environment
  wEnv = FARWORD(wPsp, 0x2C);
  lpEnv = (char far *)FARPTR(wEnv, 0);
  // ~~jfl 1999-06-29. Convert paras to bytes.
  wEnvSize = FARWORD(WORD1(lpEnv)-1, 3) << 4;
  DEBUG_PRINTF(("Environment at %lp, size 0x%X\n", lpEnv, wEnvSize));
  // Scan the environment for our variable
  for (lpName=lpEnv; *lpName; ) {
    char *pcn;

    for (lpc = lpName, pcn = pszName; *pcn; lpc++, pcn++) {
      if (*lpc != *pcn) break;
    }

    if ((!*pcn) && (*lpc == '=')) {
      lpc += 1; // lpc = environment string value
      break;    // Found!
    }

    while (*lpName) lpName++;   // Skip that environment string
    lpName++;		    // Skip the trailing NUL
  }
  DEBUG_CODE_IF_ON(
    char string[100];
    _fstrncpy(string, lpName, sizeof(string));
    if (string[0]) {
      printf("Found \"%s\"\n", string);
    } else {
      printf("%s not found.\n", pszName);
    }
  )

  // Move up the rest of the environment
  if (*lpName) {
    char far *lpNextName;
    for (lpc=lpName; *lpc; lpc++) ;
    lpNextName = lpc + 1;
    while (*lpNextName) {
      for (lpc=lpNextName; (c=*lpc) != '\0'; lpc++) *(lpName++) = c;
      *(lpName++) = '\0';
      lpNextName = lpc+1;
    }
  }
  *lpName = '\0';		 	   // Temporary end of list
  if (!pszValue || !pszValue[0]) return 0;	// Just delete the string

  // Append our string
  wEnvSize -= (uint16_t)(lpName-lpEnv);
  if (wEnvSize < (uint16_t)(strlen(pszName)+strlen(pszValue)+3)) {
    DEBUG_PRINTF(("Out of environment space\n"));
    return 1;	   // Out of environment space
  }
  while ((*(lpName++)=*(pszName++)) != '\0');	// Copy the name and trailing NUL
  *(lpName-1) = '=';				// Overwrite the previous NUL
  while ((*(lpName++)=*(pszValue++)) != '\0');	// Copy the value and trailing NUL
  *(lpName++) = '\0'; 				// Append the final NUL

  DEBUG_PRINTF(("String \"%s=%s\" added.\n", pszName, pszValue));

  return 0;
}

#endif

#if 0 // Actually this fails in XP. Compiling-out for now.
#ifdef _WIN32

/* Note: In Windows, this function sets the user environment in the registry,
	 not the local copy in the local shell.
	 Anyway this feature was only requested long ago for an MS-DOS batch file.
	 So I'm leaving this Windows version here only for the sake of the art. */

int SetMasterEnv(char *pszName, char *pszValue) {
  LONG lResult;
  // User environment variables are stored in the Registry in the key: 
  // HKEY_CURRENT_USER\Environment
  // System variables are stored in the key: 
  // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
  // 
  lResult = RegSetKeyValue(HKEY_CURRENT_USER, "Environment", pszName, REG_SZ, pszValue, lstrlen(pszValue)+1);
  return (int)lResult;
}

#endif
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FixNameCase						      |
|									      |
|   Description     Correct the file name case if needed		      |
|									      |
|   Notes	    Returns TRUE if something was changed in the name.        |
|									      |
|   History								      |
|    2013-03-27 JFL Initial implementattion.				      |
|    2014-03-20 JFL Bug fix: Make sure the drive letter is upper case.        |
|		    Bug fix: Avoid an unnecessary search if the path is empty.|
*									      *
\*---------------------------------------------------------------------------*/

#ifndef __unix__

int FixNameCase(char *pszPathname) {
  char *pszPath = pszPathname;
  char *pszName;
  DIR *pDir;
  struct dirent *pDE;
  int iModified = FALSE;
  int lDrive = 0;
  char szRootDir[] = "C:\\";
  char szDriveCurDir[] = "C:.";

  DEBUG_ENTER(("FixNameCase(\"%s\");\n", pszPathname));

  if (pszPathname[0] && (pszPathname[1] == ':')) { /* There's a drive letter */
    char c, C;
    lDrive = 2;
    c = pszPathname[0];
    C = (char)toupper(c);
    if (c != C) {
      pszPathname[0] = C;	/* Make sure the drive is upper case */
      iModified = TRUE;
    }
    if (!pszPathname[2]) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
  }

  pszName = strrchr(pszPathname, '\\');
  if (pszName) { /* There's a path separator */
    if (pszName != (pszPathname + lDrive)) { /* Possibly a drive letter, then a parent path name */
      *(pszName++) = '\0';
      iModified |= FixNameCase(pszPathname); /* Recursively fix the parent pathname */
    } else { /* Possibly a drive letter, then a root directory name */
      if (lDrive) { /* A drive letter, then a root directory name */
	pszPath = szRootDir; // Use the "C:\\" copy on the stack to make sure the routine is reentrant
	pszPath[0] = pszPathname[0];
      } else { /* Just a root directory name */
	pszPath = "\\";
      }
      pszName += 1;
    }
  } else { /* No path separator */
    pszName = pszPathname;
    if (lDrive) { /* A drive letter, then a file name */
      pszPath = szDriveCurDir; // Use the "C:." copy on the stack to make sure the routine is reentrant
      pszPath[0] = pszPathname[0];
      pszName += 2;  /* Skip the drive letter */
    } else {	  /* Just the file name */
      pszPath = ".";
    }
  }
  if (!*pszName) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));

  /* Scan all directory entries that match the requested name */
  pDir = opendir(pszPath);
  if (!pDir) {
    if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
    RETURN_BOOL_COMMENT(FALSE, ("Cannot open directory \"%s\"\n", pszPath));
  }
  while ((pDE = readdir(pDir)) != NULL) {
    if (_stricmp(pszName, pDE->d_name)) continue; /* Names differ */
    if (strcmp(pszName, pDE->d_name)) { /* If the names match, but the case differs */
      strcpy(pszName, pDE->d_name);	/* Correct the name */
      iModified = TRUE;
    }
    break;
  }
  closedir(pDir);

  if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
  RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
}

#endif // !defined(__unix__)

