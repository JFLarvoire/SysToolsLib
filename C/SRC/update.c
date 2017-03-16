/*****************************************************************************\
*                                                                             *
*   FILENAME:	    update.c						      *
*									      *
*   PURPOSE:	    Copy files only if the destination file is older	      *
*									      *
*   DESCRIPTION:    Get time of both files. Copy if the destination file      *
*		    does not exist or if it is older than the source.	      *
*									      *
*   Notes:	    Uses our custom debugging macros in debugm.h.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug   # Debug version *
*		    							      *
*   		    This program is based on the "deplace" program. Some      *
*		    comments may still refer to this old program.	      *
*									      *
*		    The target directory name can be specified with a         *
*		    trailing backslash. It is actually a good practice to     *
*		    specify it, because this makes it clear that this is a    *
*                   directory name.                                           *
*		    Note however that if the name is enclosed in "quotes",    *
*		    that trailing backslash will be considered as an escape   *
*                   character for the following quote. Ex:                    *
*		    Command line input		C argument strings	      *
*		    C:\Windows			"C:\\Windows"		      *
*		    C:\Windows\			"C:\\Windows\\"		      *
*		    "C:\Windows"		"C:\\Windows"		      *
*		    "C:\Windows\"		"C:\\Windows\""		      *
*		    "C:\Windows\\"		"C:\\Windows\\"		      *
*		    C:\Program Files		"C:\\Program" "Files"	      *
*		    C:\Program Files\		"C:\\Program" "Files\\"	      *
*		    "C:\Program Files"		"C:\\Program Files"	      *
*		    "C:\Program Files\"		"C:\\Program Files\""	      *
*		    "C:\Program Files\\"	"C:\\Program Files\\"	      *
*		    Version 2.2 of this program includes a workaround for     *
*		    this common mistake, and changes back a trailing " into   *
*                   a trailing \.                                             *
*		    							      *
*		    TO DO: Change copy(), update(), etc, return type to a     *
*		    string pointer, and return NULL for success or a          *
*		    dynamically created error message in case of error.	      *
*		    							      *
*   History:								      *
*    1986-04-01 JFL jf.larvoire@hp.com created this program.		      *
*    1987-05-07 JFL Adapted to Lattice 3.10				      *
*    1992-05-20 JFL Adapted to Microsoft C. Adapted to OS/2. Switch /?.       *
*    1994-10-11 GB  Add -nologo and -noempty options			      *
*		    Version 1.2.					      *
*    1996-10-14 JFL Adapted to Win32. Version 2.0.			      *
*    2001-01-07 JFL Added -v option; Display list of files copied.	      *
*		    Version 2.1.					      *
*    2010-03-04 JFL Changed pathname buffers sizes to PATHNAME_SIZE.          *
*                   Cleaned up obsolete C syntax and library calls.           *
*                   Added a workaround for the trailing \ issue.              *
*                   Output just the file names. Use -v to get the old details.*
*		    Version 2.2.					      *
*    2010-03-10 JFL Added option -p. Version 2.2a.                            *
*    2010-04-09 JFL Removed debugging options. Version 2.2b.                  *
*    2011-09-06 JFL Added the ability to update to a file with a != name.     *
*		    Version 2.3.					      *
*    2012-10-01 JFL Added support for a Win64 version. No new features.       *
*		    Version 2.3.1.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.3.2.                 *
*    2013-02-21 JFL Use the standard directory access functions.              *
*                   Side bug fix: Updating *.c will not copy *.c~ anymore.    *
*                   Added a Linux version.                                    *
*    2013-03-10 JFL Changed the command line syntax to be compatible with     *
*                   wildcards on Linux shells.                                *
*                   Added support for files > 4GB.                            *
*                   Version 3.0.0.					      *
*    2013-03-15 JFL Copy file permissions under Linux.                        *
*                   Added resiliency:                                         *
*                   When reading fails to start, avoid deleting the target.   *
*                   In case of error later on, delete incomplete copies.      *
*                   Version 3.0.1.					      *
*    2014-02-12 JFL Added support for symlinks in all operating systems,      *
*                   and symlinkds and junctions in Windows.                   *
*                   Improved the debugging output.                            *
*                   Version 3.1.                                              *
*    2014-02-21 JFL Added option -r for recursive updates.                    *
*    2014-02-26 JFL Use MsvcLibX LocalFileTime() to display local file times  *
*		    in Windows the same way cmd.exe and explorer.exe do.      *
*    2014-02-28 JFL Added support for UTF-8 pathnames, and output them in     *
*                   the current code page.                                    *
*                   Version 3.2.                                              *
*    2014-06-04 JFL Rebuilt in Windows with support for utimes() in MsvcLibX. *
*                   Version 3.2.1.                                            *
*    2014-06-13 JFL Fixed bug copying files > 2GB in Windows.		      *
*                   Version 3.2.2.                                            *
*    2014-07-01 JFL Fixed bug when copying files from the root directory.     *
*                   Report non ASCII file names correctly in error messages.  *
*    2014-07-02 JFL Added support for pathnames > 260 bytes in Windows.	      *
*                   Changed macro RETURN() to RETURN_CONST(), etc.	      *
*    2014-07-04 JFL Copy the date of directories too.                         *
*    2014-07-09 JFL Fixed a bug when updating existing links.                 *
*                   Version 3.3.                                              *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*                   Realigned help. Version 3.3.1.                            *
*    2015-01-06 JFL Bug fix: Delete the target if it's not the same type as   *
*                   the source file; But don't in test mode. Version 3.3.2.   *
*    2015-01-08 JFL Work around issue with old versions of Linux that define  *
*                   lchmod and lutimes, but implement only stubs that always  *
*                   fail. Version 3.3.3.                                      *
*    2015-01-12 JFL Bug fix: In recursive mode, an incorrect directory name   *
*                   was sometimes displayed. (But the correct directory was   *
*                   copied.) Version 3.3.4.                                   *
*    2015-12-14 JFL Bug fix: References to D: actually accessed D:\ in Windows.
*                   Bug fix: Failed to start in Windows XP due to missing fct.*
*                   Bug fix: Writing to disconnected drive returned many errs.*
*		    Bug fix: DOS version failed to read root directories.     *
*                   Version 3.3.5.                                            *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 3.3.6.  					      *
*    2016-04-12 JFL Added option -S to show destination names.                *
*		    Version 3.4.    					      *
*    2016-05-10 JFL Added option -F/--force to overwrite read-only files.     *
*		    Version 3.5.    					      *
*    2016-09-13 JFL Minor tweaks to fix compilation in Linux.                 *
*    2017-01-30 JFL Improved mkdirp(), to avoid useless error messages.       *
*                   Added a workaround for the WIN32 _fullpath() bug.         *
*		    Version 3.5.1.    					      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "3.5.1"
#define PROGRAM_DATE    "2017-01-30"

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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <sys/time.h>		/* For lutimes() */
#include <utime.h>		/* For struct utimbuf */
#include <dirent.h>		/* We use the DIR type and the dirent structure */
#include <unistd.h>		/* For the access function */
#include <fnmatch.h>
#include <iconv.h>
#include <inttypes.h>

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#ifdef _MSC_VER
#pragma warning(disable:4001)	/* Ignore the // C++ comment warning */
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATHNAME_SIZE PATH_MAX
#define NODENAME_SIZE FILENAME_MAX
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#define _filelength(hFile) _filelengthi64(hFile)

/* Front-end to _fullpath, with work around for trail spaces bug */
char *fullpath(char *absPath, const char *relPath, size_t maxLength);  

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define OS_NAME "DOS"

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE 13		/* 8.3 name length = 8+1+3+1 = 13 */
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#define fullpath _fullpath

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define OS_NAME "OS/2"

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATHNAME_SIZE CCHMAXPATH	/* FILENAME_MAX incorrect in stdio.h */
#define NODENAME_SIZE CCHMAXPATHCOMP
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#define fullpath _fullpath

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Automatically defined when targeting a Unix app. */

#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE FILENAME_MAX
#define PATTERN_ALL "*"     		/* Pattern matching all files */

#define _MAX_PATH  FILENAME_MAX
#define _MAX_DRIVE 3
#define _MAX_DIR   FILENAME_MAX
#define _MAX_FNAME FILENAME_MAX
#define _MAX_EXT   FILENAME_MAX

#ifndef _S_IREAD
#define _S_IREAD __S_IREAD
#endif
#ifndef _S_IWRITE
#define _S_IWRITE __S_IWRITE
#endif
#ifndef _S_IEXEC
#define _S_IEXEC __S_IEXEC
#endif

#define _stricmp strcasecmp

/* Redefine Microsoft-specific routines */
off_t _filelength(int hFile);
#define fullpath(absPath, relPath, maxLength) realpath(relPath, absPath)
#define LocalFileTime localtime

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

#ifndef min
#define min(a,b) ( ((a)<(b)) ? (a) : (b) )
#endif

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#ifdef _MSDOS
#define BUFFERSIZE 16384
#else
#define BUFFERSIZE (1024L * 1024L)
#endif
char *buffer;       /* Pointer on the intermediate copy buffer */

static int test = 0;			/* Flag indicating Test mode */
static int show = 0;			/* 0=Show source; 1=Show dest */
static int fresh = 0;			/* Flag indicating freshen mode */
static int force = 0;			/* Flag indicating force mode */
static int iVerbose = FALSE;		/* Flag for displaying verbose information */
static int copyempty = TRUE;		/* Flag for copying empty file */
static int iPause = 0;			/* Flag for stop before exit */
#ifdef __unix__
static int iFnmFlag = 0;		/* Case-sensitive pattern matching */
#else
static int iFnmFlag = FNM_CASEFOLD;	/* Case-insensitive pattern matching */
#endif
static int iRecur = 0;			/* Recursive update */
#ifdef _WIN32
// UINT cp = 0;				/* Initial console code page */
#define cp codePage			/* Initial console code page in iconv.c */
#endif

/* forward references */

char *version(void);			/* Build this program version string */
void usage(void);			/* Display usage */
int IsSwitch(char *pszArg);		/* Is this a command-line switch? */
int updateall(char *, char *);		/* Copy a set of files if newer */
int update(char *, char *);		/* Copy a file if newer */
#if defined(S_ISLNK)
int update_link(char *, char *);	/* Copy a link if newer */
#endif
int copyf(char *, char *);		/* Copy a file silently */
int copy(char *, char *);		/* Copy a file and display messages */
int mkdirp(const char *path, mode_t mode); /* Same as mkdir -p */

int exists(char *name);			/* Does this pathname exist? (TRUE/FALSE) */
int exist_file(char *); 		/* Does this file exist? (TRUE/FALSE) */
int file_empty(char *); 		/* Is this file empty? (TRUE/FALSE). */
					/* File must exist (else return FALSE)  */
int is_directory(char *);		/* Is name a directory? -> TRUE/FALSE */
int older(char *, char *);		/* Is file 1 older than file 2? */
time_t getmodified(char *);		/* Get time of file modification */
int copydate(char *to, char *from);	/* Copy the file date & time */

char *strgfn(const char *);		/* Get file name position */
void stcgfn(char *, const char *);	/* Get file name */
void stcgfp(char *, const char *);	/* Get file path */
void strmfp(char *, const char *, const char *);    /* Make file pathname */
void strsfp(const char *, char *, char *);          /* Split file pathname */

/* Exit front end, with support for the optional final pause */
void do_exit(int n) {
  if (iPause) {
    fflush(stdin);
    printf("Press Enter to continue... ");
    fflush(stdout);
    getchar();
  }
  exit(n);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    EXE program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|									      |
|    1986-04-01 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
    {
    int argn;			/* Argument index */
    char *arg;
#if defined(_MSDOS) || defined(_WIN32)
    size_t len;
#endif
    char *target;
    int nErrors = 0;
    int iExit = 0;

    for (argn = 1; ((argn<argc) && IsSwitch(argv[argn])); argn += 1)
        {
        arg = argv[argn];
	DEBUG_PRINTF(("Arg = %s\n", arg));
#ifndef __unix__
        arg[0] = '-';
#endif
	if (   streq(arg, "-h")	    /* Display usage */
	    || streq(arg, "-help")	/* The historical name of that switch */
	    || streq(arg, "--help")
	    || streq(arg, "-?"))
	    {
	    usage();
            }
#ifdef _WIN32
	if (   streq(arg, "-A")
	    || streq(arg, "--ansi")) {	/* Force encoding output with the ANSI code page */
	    cp = CP_ACP;
	    continue;
	}
#endif
	DEBUG_CODE(
	if (   streq(arg, "-d")	    /* Debug mode on */
	    || streq(arg, "-debug")	/* The historical name of that switch */
	    || streq(arg, "--debug"))
            {
	    DEBUG_ON();
	    iVerbose = TRUE;
	    if (iVerbose) printf("Debug mode on.\n");
	    continue;
            }
        )
	if (   streq(arg, "-E")     /* NoEmpty mode on */
	    || streq(arg, "-noempty")  /* The historical name of that switch */
	    || streq(arg, "--noempty"))
            {
	    copyempty = FALSE;
	    if (iVerbose) printf("NoEmpty mode on.\n");
	    continue;
            }
	if (   streq(arg, "-f")	    /* Freshen mode on */
	    || streq(arg, "--freshen"))
            {
            fresh = 1;
	    if (iVerbose) printf("Freshen mode on.\n");
	    continue;
            }
	if (   streq(arg, "-F")	    /* Force mode on */
	    || streq(arg, "--force"))
            {
            force = 1;
	    if (iVerbose) printf("Force mode on.\n");
	    continue;
            }
	if (   streq(arg, "-i")	    /* Case-insensitive pattern matching */
	    || streq(arg, "--ignorecase"))
            {
            iFnmFlag |= FNM_CASEFOLD;
	    if (iVerbose) printf("Case-insensitive pattern matching.\n");
	    continue;
            }
	if (   streq(arg, "-k")	    /* Case-sensitive pattern matching */
	    || streq(arg, "--casesensitive"))
            {
            iFnmFlag &= ~FNM_CASEFOLD;
	    if (iVerbose) printf("Case-sensitive pattern matching.\n");
	    continue;
            }
#ifdef _WIN32
	if (   streq(arg, "-O")
	    || streq(arg, "--oem")) {	/* Force encoding output with the OEM code page */
	    cp = CP_OEMCP;
	    continue;
	}
#endif
	if (   streq(arg, "-p")	    /* Final Pause on */
	    || streq(arg, "--pause"))
            {
            iPause = 1;
	    if (iVerbose) printf("Final Pause on.\n");
	    continue;
            }
	if (   streq(arg, "-q")	    /* Quiet/Nologo mode on */
	    || streq(arg, "--quiet")
	    || streq(arg, "-nologo"))	/* The historical name of that switch */
            {
	    iVerbose = FALSE;
	    continue;
            }
	if (   streq(arg, "-r")	    /* Recursive update */
	    || streq(arg, "--recurse"))
            {
            iRecur = 1;
	    if (iVerbose) printf("Recursive update.\n");
	    continue;
            }
	if (   streq(arg, "-S")     /* Show dest instead of source */
	    || streq(arg, "--showdest"))
            {
            show = 1;
	    if (iVerbose) printf("Show destination files names.\n");
	    continue;
            }
#ifdef _WIN32
	if (   streq(arg, "-U")
	    || streq(arg, "--utf8")) {	/* Force encoding output with the UTF-8 code page */
	    cp = CP_UTF8;
	    continue;
	}
#endif
	if (   streq(arg, "-v")	    /* Verbose mode on */
	    || streq(arg, "--verbose"))
            {
	    iVerbose = TRUE;
	    continue;
            }
	if (   streq(arg, "-V")     /* -V: Display the version */
	    || streq(arg, "--version"))
	    {
	    printf("%s\n", version());
	    exit(0);
	    }
	if (   streq(arg, "-X")	    /* NoExec/Test mode on */
	    || streq(arg, "--noexec")
	    || streq(arg, "-t"))	/* The historical name of that switch */
            {
	    test = 1;
	    if (iVerbose) printf("NoExec mode on.\n");
	    continue;
            }
	printf("Unrecognized switch %s. Ignored.\n", arg);
	}

    if ( (argc - argn) < 1 ) usage();

    buffer = malloc(BUFFERSIZE);	/* Allocate memory for copying */
    if (!buffer)
        {
	printf("\nNot enough memory.\n");
        do_exit(1);
        }

    DEBUG_PRINTF(("Size of size_t = %d bits\n", (int)(8*sizeof(size_t))));
    DEBUG_PRINTF(("Size of off_t = %d bits\n", (int)(8*sizeof(off_t))));

    target = argv[--argc];	/* The last argument is the target */
    DEBUG_PRINTF(("Target = %s\n", target));
#if defined(_MSDOS) || defined(_WIN32)
    /* Workaround for a command.com or cmd.exe bug */
    len = strlen(target);
    if (len && (target[len-1] == '"'))
	{
	target[len-1] = DIRSEPARATOR_CHAR;
	DEBUG_PRINTF(("Changing the trailing quote to a backslash: %s\n", target));
	}
    /* Avoid multiple errors when writing to an inexistant or disconnected drive */
    if (target[0] && (target[1] == ':')) {
      struct stat s;
      char szDrive[4];
      int iErr;
      sprintf(szDrive, "%c:\\", target[0]);
      iErr = stat(szDrive, &s);
      if (iErr) {
	fprintf(stderr, "Error: Cannot access drive %c: %s\n", target[0], strerror(errno));
	do_exit(1);
      }
    }
#endif

    for ( ; argn < argc; argn++) 	/* For every source file before that */
        {
        arg = argv[argn];
	nErrors += updateall(arg, target);
        }

    if (nErrors) { /* Display a final summary, as the errors may have scrolled up beyond view */
      fprintf(stderr, "Error: %d file(s) failed to be updated\n", nErrors);
      iExit = 1;
    }

    do_exit(iExit);
    return iExit;
    }

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
	  );
}

void usage(void)
    {
    printf("Update version %s\n\
\n\
Usage: update [SWITCHES] FILES DIRECTORY\n\
       update [SWITCHES] FILES DIRECTORY" DIRSEPARATOR_STRING "NEWDIR" DIRSEPARATOR_STRING "\n\
       update [SWITCHES] FILE  DIRECTORY[" DIRSEPARATOR_STRING "NEWNAME]\n\
\n\
Files:          FILE1 [FILE2 ...]\n\
                Wildcards are allowed in source files pathnames.\n\
\n\
Switches:\n"
#ifdef _WIN32
"\
  -A|--ansi     Force encoding the output using the ANSI character set.\n"
#endif
#ifdef _DEBUG
"\
  -d|--debug    Output debug information.\n"
#endif
"\
  -f|--freshen  Freshen mode. Update only files that exist in both directories.\n\
  -F|--force    Force mode. Overwrite read-only files.\n\
", version());

    printf("\
  -E|--noempty  Noempty mode. Don't copy empty file.\n\
  -h|--help|-?  Display this help screen.\n\
  -i|--ignorecase    Case-insensitive pattern matching. Default for DOS/Windows.\n\
  -k|--casesensitive Case-sensitive pattern matching. Default for Unix.\n"
#ifdef _WIN32
"\
  -O|--oem      Force encoding the output using the OEM character set.\n"
#endif
"\
  -p|--pause    Pause before exit.\n\
  -q|--nologo   Quiet mode. Don't display anything.\n\
  -r|--recurse  Recursively update all subdirectories.\n\
  -S|--showdest Show the destination files names. Default: The sources names.\n"
#ifdef _WIN32
"\
  -U|--utf8     Force encoding the output using the UTF-8 character encoding.\n"
#endif
"\
  -v|--verbose  Verbose node. Display extra status information.\n\
  -V|--version  Display this program version and exit.\n\
  -X|-t         Noexec mode. Display the files that need to be copied.\n\
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

    do_exit(0);
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

int IsSwitch(char *pszArg)
    {
    return (   (*pszArg == '-')
#ifndef __unix__
            || (*pszArg == '/')
#endif
           ); /* It's a switch */
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    updateall						      |
|                                                                             |
|   Description:    Update all files from source dir to dest dir              |
|                                                                             |
|   Parameters:     char *p1	    Source path. Wildcards allowed for files. |
|                   char *p2	    Destination directory		      |
|                                                                             |
|   Return value:   The number of errors encountered. 0=Success               |
|                                                                             |
|   Notes:	    Copy files, except if a newer version is already there.   |
|                                                                             |
|   History:								      |
|    2011-09-06 JFL Added the ability to update to a file with a differ. name.|
*                                                                             *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif
int updateall(char *p1,             /* Wildcard * and ? are interpreted */
	      char *p2)
    {
#if _MSDOS /* In DOS, the pathname size is very small, it can be auto-allocated on the stack */
    char path0[PATHNAME_SIZE], path1[PATHNAME_SIZE], path2[PATHNAME_SIZE];
    char path[PATHNAME_SIZE], name[PATHNAME_SIZE];
    char fullpathname[PATHNAME_SIZE], path3[PATHNAME_SIZE];
#else /* In all other OSs, pathname size is rather large. It'll be dynamically allocated from the heap */
    char *path0, *path1, *path2;
    char *path, *name;
    char *fullpathname, *path3;
#endif
    char *ppath, *pname;
    DIR *pDir;
    struct dirent *pDE;
    char *pattern;
    int err;
    int nErrors = 0;
    int iTargetDirExisted;

    DEBUG_ENTER(("updateall(\"%s\", \"%s\");\n", p1, p2));

#ifndef _MSDOS
    path0 = malloc(PATHNAME_SIZE);
    path1 = malloc(PATHNAME_SIZE);
    path2 = malloc(PATHNAME_SIZE);
    path3 = malloc(PATHNAME_SIZE);
    path = malloc(PATHNAME_SIZE);
    name = malloc(PATHNAME_SIZE);
    fullpathname = malloc(PATHNAME_SIZE);
    if ((!path0) || (!path1) || (!path2) || (!path) || (!name)) {
      fprintf(stderr, "Error: Not enough memory\n");
      nErrors += 1;
      RETURN_INT(nErrors);
    }
#endif

    strcpy(path0, p1);
    if (is_directory(p1)) {          /* If yes, assume "\*.*"  */
      pattern = PATTERN_ALL;
    } else {
      char *pSlash;
      pSlash = strrchr(path0, DIRSEPARATOR_CHAR);
      if (pSlash) {
      	pattern = pSlash + 1;
      	if (!*pattern) {
      	  pattern = PATTERN_ALL; /* There was a trailing / */
      	} else {
      	  pattern = p1 + (pattern - path0); /* Use the original in p1, as the copy in path0 may be overwritten below */
      	}
      	while ((pSlash > path0) && (*(pSlash-1) == DIRSEPARATOR_CHAR)) pSlash -= 1; /* Remove extra consecutive / */
      	*pSlash++ = '\0';
      	if (   (!path0[0])
#if defined(_MSDOS) || defined(_WIN32)
      	    || ((path0[1] == ':') && (!path0[2]) && (p1[2])) /* p1 was like "D:\", and path0 like "D:" */
#endif /* defined(_MSDOS) || defined(_WIN32) */
      	   ) { /* If this was the root directory (possibly on another drive in DOS/Windows) */
      	  /* Then restore that trailing / indicating the root dir */
      	  *(pSlash-1) = DIRSEPARATOR_CHAR;
	  *pSlash++ = '\0';
	}
      } else {
      	pattern = p1;
      	strcpy(path0, ".");
      }
    }

    if (iVerbose) {
      DEBUG_PRINTF(("// ")); /* If debug is on, print the debug indent */
      printf("Update %s from %s to %s\n", pattern, path0, p2);
    }

    /* Check if the target is a file or directory name */
    ppath = p2;
    pname = NULL; /* Implies using pDE->d_name */
    strsfp(p2, path, name);
    if (name[0] && is_directory(path) && (!is_directory(p2)) && (!strpbrk(p1, "*?"))) {
      ppath = path;
      pname = name;
      DEBUG_PRINTF(("// The target is file %s in directory %s\n", pname, ppath));
    } else {
      DEBUG_PRINTF(("// The target is directory %s\n", ppath));
    }
    iTargetDirExisted = is_directory(ppath);

    /* Note: Scan the source directory even in the absence of wildcards.
       In Windows, this makes sure that the copy has the same case as
       the source, even if the command-line argument has a different case. */

    /* Scan all files that match the wild cards */
    pDir = opendir(path0);
    if (!pDir) {
      fprintf(stderr, "Error: can't open directory \"%s\": %s\n", path0, strerror(errno));
      nErrors += 1;
#ifndef _MSDOS
      free(path0); free(path1); free(path2); free(path3); free(path); free(name); free(fullpathname);
#endif
      RETURN_INT(nErrors);
    }
    while ((pDE = readdir(pDir))) {
      DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
      if (   (pDE->d_type != DT_REG)
#if defined(S_ISLNK)
      	  && (pDE->d_type != DT_LNK)
#endif
      	 ) continue;	/* We want only files or links */
      if (fnmatch(pattern, pDE->d_name, iFnmFlag) == FNM_NOMATCH) continue;
      strmfp(path1, path0, pDE->d_name);  /* Compute source path */
      DEBUG_PRINTF(("// Found %s\n", path1));
      strmfp(path2, ppath, pname?pname:pDE->d_name); /* Append it to directory p2 too */
#if defined(S_ISLNK)
      if (pDE->d_type == DT_LNK) {
	err = update_link(path1, path2);
      }
      else
#endif
      {
      	err = update(path1, path2);
      }
      if (err) {
      	fprintf(stderr, "Error: Failed to create \"%s\". %s.\n", path2, strerror(errno));
      	nErrors += 1;
      	/* Continue the directory scan, looking for other files to update */
      }
    }
    closedir(pDir);

    if (iRecur) { /* Parse the directory again, looking for actual directories (not junctions nor symlinkds) */
      pDir = opendir(path0);
      if (!pDir) {
	fprintf(stderr, "Error: Can't open directory \"%s\": %s\n", path0, strerror(errno));
      	nErrors += 1;
#ifndef _MSDOS
	free(path0); free(path1); free(path2); free(path3); free(path); free(name); free(fullpathname);
#endif
	RETURN_INT(nErrors);
      }
      while ((pDE = readdir(pDir))) {
      	int p2_exists, p2_is_dir;

	DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
	if (pDE->d_type != DT_DIR) continue;	/* We want only directories */
	if (streq(pDE->d_name, ".") || streq(pDE->d_name, "..")) continue; /* These are not real subdirs */

	strmfp(path3, path0, pDE->d_name); /* Source subdirectory path: path3 = path0/d_name */
	fullpath(fullpathname, path3, PATHNAME_SIZE); /* Build absolute pathname of source dir */
	strmfp(path1, path3, pattern);	   /* Search pattern: path1 = path3/pattern */
	strmfp(path2, ppath, pDE->d_name); /* Destination subdirectory path: path2 = ppath/dname */
	strcat(path2, DIRSEPARATOR_STRING);/* Make sure the target path gets created if needed */

	p2_exists = exists(path2);
	p2_is_dir = is_directory(path2);
	if ((!p2_exists) || (!p2_is_dir)) {
	  if (test == 1) {
	    if (iVerbose) {
	      DEBUG_PRINTF(("// "));
	      printf("Would copy directory %s\\\n", fullpathname);
	    }
	    /* printf("%s\\\n", fullpathname); // 2015-01-12 JFL Don't display the directory name,
		    as we're interested only in its inner files, and there may be none to copy */
	  } else {
	    if (p2_exists && !p2_is_dir) {
	      err = unlink(path2);	/* Delete the conflicting file/link */
	      if (err) {
		fprintf(stderr, "Error: Failed to remove \"%s\"\n", path2);
		nErrors += 1;
		continue;	/* Try updating something else */
	      }
	      p2_exists = FALSE;
	    }
#if 0	    /* 2015-01-12 JFL Don't create the directory now, as it may never be needed,
				if there are no files that match the input pattern */
	    if (!p2_exists) {	/* Create the missing target directory */
	      printf("%s\\\n", fullpathname);
	      err = mkdirp(path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	      if (err) {
		fprintf(stderr, "Error: Failed to create directory \"%s\"\n", p2);
		nErrors += 1;
		continue;	/* Try updating something else */
	      }
	    }
#endif
	  }
	}

	err = updateall(path1, path2);
	if (err) nErrors += err;

	if (!p2_exists) { /* If we did create the target subdir */
	  copydate(path2, path3); /* Make sure the directory date matches too */
	}
      }
      closedir(pDir);
    }

    if ((!iTargetDirExisted) && is_directory(ppath)) { /* If we did create the target dir */
      copydate(ppath, path0); /* Make sure the directory date matches too */
    }

#ifndef _MSDOS
    free(path0); free(path1); free(path2); free(path3); free(path); free(name); free(fullpathname);
#endif
    RETURN_INT(nErrors);
    }
#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    update						      |
|                                                                             |
|   Description:    Update one file					      |
|                                                                             |
|   Parameters:     char *p1	    Source file pathname                      |
|                   char *p2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success, else Error				      |
|                                                                             |
|   Notes:	    Copy the file, except if a newer version is already there.|
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Updated the test mode support, and fixed a bug when       |
|                   using both the test mode and the showdest mode.           |
*                                                                             *
\*---------------------------------------------------------------------------*/

int update(char *p1,	/* Both names must be complete, without wildcards */
           char *p2)
    {
    int e;
    char name[PATHNAME_SIZE];
    struct stat sP2stat;
    char *p;

    DEBUG_ENTER(("update(\"%s\", \"%s\");\n", p1, p2));

    /* Get the pathname to display, before p2 is possibly modified by the test mode */
    p = p1;		/* By default, show the source file name */
    if (show) p = p2;	/* But in showdest mode, show the destination file name */

    /* In freshen mode, don't copy if the destination does not exist. */
    if (fresh && !exist_file(p2)) RETURN_CONST(0);

    /* In Noempty mode, don't copy empty file */
    if ( (copyempty == FALSE) && (file_empty(p1)) ) RETURN_CONST(0);

    /* If the target exists, make sure it's a file */
    e = lstat(p2, &sP2stat); /* Use lstat to avoid following links */
    if (e == 0) {
      if (S_ISDIR(sP2stat.st_mode)) {	/* If the target is a directory */
	if (!test) {
	  e = rmdir(p2);		/* Then remove it */
	} else {
	  if (iVerbose) {
	    DEBUG_PRINTF(("// "));
	    printf("Would delete directory %s\n", p2);
	  }
	  p2 = ""; /* Trick older() to think the target is deleted */
	}
#if defined(S_ISLNK)
      } else if (S_ISLNK(sP2stat.st_mode)) { /* Else if it's a link */
	if (!test) {
	  e = unlink(p2); /* Then deletes the link, not its target. */
	} else {
	  if (iVerbose) {
	    DEBUG_PRINTF(("// "));
	    printf("Would delete link %s\n", p2);
	  }
	  p2 = ""; /* Trick older() to think the target is deleted */
	}
#endif /* defined(S_ISLNK) */
      } /* Else the target is a plain file */
      if (e) {
      	fprintf(stderr, "Failed to remove \"%s\"\n", p2);
      	RETURN_INT(e);
      }
    }

    /* In any mode, don't copy if the destination is newer than the source. */
    if (older(p1, p2)) RETURN_CONST(0);

    fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of source */
    if (test == 1)
        {
	if (iVerbose) {
	  DEBUG_PRINTF(("// "));
	  printf("Would copy file ");
	}
	printf("%s\n", name);
        RETURN_CONST(0);
        }

    if (!iVerbose) printf("%s\n", name);

    e = copy(p1, p2);

    RETURN_INT_COMMENT(e, (e?"Error\n":"Success\n"));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    update_link						      |
|                                                                             |
|   Description:    Update one symbolic link				      |
|                                                                             |
|   Parameters:     char *p1	    Source link pathname                      |
|                   char *p2	    Destination link pathname		      |
|                                                                             |
|   Return value:   0 = Success, else Error				      |
|                                                                             |
|   Notes:	    Copy the link, except if a newer version is already there.|
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Added support for the --force option.                     |
*                                                                             *
\*---------------------------------------------------------------------------*/

int exists(char *name) {
    int result;
    struct stat sstat;

    DEBUG_ENTER(("exists(\"%s\");\n", name));

    result = !lstat(name, &sstat); // Use lstat, as stat does not detect SYMLINKDs.

    RETURN_BOOL(result);
}

#if defined(S_ISLNK)	/* All recent versions of Windows and Unix, but not MS-DOS or Windows 95/98 */

int is_link(char *name) {
    int result;
    int err;
    struct stat sstat;

    DEBUG_ENTER(("is_link(\"%s\");\n", name));

    err = lstat(name, &sstat); // Use lstat, as stat does not set S_IFLNK.
    result = ((err == 0) && (S_ISLNK(sstat.st_mode)));

    RETURN_BOOL(result);
}

/* Copy link p1 onto link p2, if and only if p1 is newer. */
int update_link(char *p1,	/* Both names must be complete, without wildcards */
                char *p2)
    {
    int err;
    char name[PATHNAME_SIZE];
    char target1[PATHNAME_SIZE];
    DEBUG_CODE(int iSize;)
#if _MSVCLIBX_STAT_DEFINED
    struct stat sP1stat;
#endif
    struct stat sP2stat;
    int bP2Exists;
    int bP2IsLink;
    char path[PATHNAME_SIZE];
    char *p;

    DEBUG_ENTER(("update_link(\"%s\", \"%s\");\n", p1, p2));

    err = lstat(p2, &sP2stat); // Use lstat to avoid following links
    bP2Exists = (err == 0);
    bP2IsLink = (bP2Exists && (S_ISLNK(sP2stat.st_mode)));

    /* In freshen mode, don't copy if the destination does not exist. */
    if (fresh && !bP2IsLink) RETURN_CONST(0);

    /* In any mode, don't copy if the destination is newer than the source. */
    if (bP2IsLink && older(p1, p2)) RETURN_CONST(0);

    p = p1;		/* By default, show the source file name */
    if (show) p = p2;	/* But in showdest mode, show the destination file name */
    fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of source */
    if (test == 1)
        {
	if (iVerbose) {
	  DEBUG_PRINTF(("// "));
	  printf("Would copy link ");
	}
	printf("%s\n", name);
        RETURN_CONST(0);
        }

    if (!iVerbose) printf("%s\n", name);

    if (bP2Exists) { // Then the target has to be removed, even if it's a link
      // First, in force mode, prevent failures if the target is read-only
      if (force && !(sP2stat.st_mode & S_IWRITE)) {
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", p2, _S_IWRITE));
      	err = chmod(p2, _S_IWRITE); /* Try making the target file writable */
      	DEBUG_PRINTF(("  return %d; // errno = %d\n", err, errno));
      }
      if (S_ISDIR(sP2stat.st_mode)) {
	if (!test) err = rmdir(p2);		/* Then remove it */
	else {
	  if (iVerbose) {
	    DEBUG_PRINTF(("// "));
	    printf("Would delete directory %s\n", p2);
	  }
	}
      } else { // It's a file or a link
	if (!test) err = unlink(p2); // If it's a link, deletes the link, not its target.
	else if (!S_ISLNK(sP2stat.st_mode)) {
	  if (iVerbose) {
	    DEBUG_PRINTF(("// "));
	    printf("Would delete file %s\n", p2);
	  }
	} // Else it's a link. Deleting it is normal, not worth mentioning in verbose mode.
      }
      if (err) {
      	fprintf(stderr, "Failed to remove \"%s\"\n", p2);
      	RETURN_INT(err);
      }
    }

    strsfp(p2, path, NULL);
    if (!exists(path)) {
      err = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (err) {
      	fprintf(stderr, "\nError: Failed to create directory \"%s\". %s\n", path, strerror(errno));
	RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
      }
    }

    DEBUG_CODE(iSize = (int)) 
    readlink(p1, target1, sizeof(target1));

    DEBUG_PRINTF(("// Target1=\"%s\", iSize=%d\n", target1, iSize));
    // e = copy(p1, p2);
#if _MSVCLIBX_STAT_DEFINED
    err = lstat(p1, &sP1stat); // Use lstat to avoid following links
    if (sP1stat.st_ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
      err = junction(target1, p2);
    } else if (sP1stat.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) {
      err = symlinkd(target1, p2);
    } else
#endif
    err = symlink(target1, p2);
    if (!err) copydate(p2, p1);

    RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
    }

#endif // !defined(S_ISLNK)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copyf						      |
|                                                                             |
|   Description:    Copy one file					      |
|                                                                             |
|   Parameters:     char *name1	    Source file pathname                      |
|                   char *name2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success						      |
|                   1 = Read error					      |
|                   2 = Write error					      |
|                                                                             |
|   Notes:	    Both names must be correct, and paths must exist.	      |
|                                                                             |
|   History:								      |
|    2013-03-15 JFL Added resiliency:					      |
|                   When reading fails to start, avoid deleting the target.   |
|                   In case of error later on, delete incomplete copies.      |
|    2016-05-10 JFL Added support for the --force option.                     |
*                                                                             *
\*---------------------------------------------------------------------------*/

int copyf(char *name1,		    /* Source file to copy from */
          char *name2)		    /* Destination file to copy to */
    {
    FILE *pfs, *pfd;	    /* Source & destination file pointers */
    int hsource;	    /* Source handle */
    off_t lon;		    /* File length */
    size_t tocopy;	    /* Number of bytes to copy in one pass */
    int iShowCopying = FALSE;
    int nAttempt = 1;	    /* Force mode allows retrying a second time */

    DEBUG_ENTER(("copyf(\"%s\", \"%s\");\n", name1, name2));
    if (iVerbose
#ifdef _DEBUG
        && !iDebug
#endif
	) {
	  iShowCopying = TRUE;
	  printf("\tCopying %s", name1);
	}

    pfs = fopen(name1, "rb");
    if (!pfs) {
      if (iShowCopying) printf("\n");
      RETURN_INT_COMMENT(1, ("Can't open input file\n"));
    }
    hsource = fileno(pfs);

    lon = _filelength(hsource);
    /* Read 1 byte to test access rights. This avoids destroying the target
       if we don't have the right to read the source. */
    if (lon && !fread(buffer, 1, 1, pfs)) {
      if (iShowCopying) printf("\n");
      RETURN_INT_COMMENT(1, ("Can't read the input file\n"));
    }
    fseek(pfs, 0, SEEK_SET);
retry_open_targetfile:
    pfd = fopen(name2, "wb");
    if (!pfd) {
      if ((errno == EACCES) && (nAttempt == 1) && force) {
      	int iErr = chmod(name2, _S_IWRITE); /* Try making the target file writable */
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", name2, _S_IWRITE));
      	DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
      	if (!iErr) {
	  nAttempt += 1;
	  goto retry_open_targetfile;
	}
      }
      if (iShowCopying) printf("\n");
      fclose(pfs);
      RETURN_INT_COMMENT(2, ("Can't open the output file\n"));
    }
    /* hdest = fileno(pfd); */

    if (iShowCopying) printf(" : %"PRIdPTR" bytes\n", lon);

    while (lon) {
      tocopy = (size_t)min(BUFFERSIZE, lon);
      
      XDEBUG_PRINTF(("fread(%p, %"PRIuPTR", 1, %p);\n", buffer, tocopy, pfs));
      if (!fread(buffer, tocopy, 1, pfs)) {
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(1, ("Can't read the input file. Deleted the partial copy.\n"));
      }
      if (!fwrite(buffer, tocopy, 1, pfd)) {
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(2, ("Can't write the output file. Deleted the partial copy.\n"));
      }
      lon -= tocopy;
    }

    fclose(pfs);
    fclose(pfd);

    copydate(name2, name1);	/* & give the same date than the source file */

    DEBUG_PRINTF(("// File %s mode is read%s\n", name2,
			access(name2, 6) ? "-only" : "/write"));

    RETURN_INT_COMMENT(0, ("File copy complete.\n"));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copy						      |
|                                                                             |
|   Description:    Copy one file, creating the target directory if needed    |
|                                                                             |
|   Parameters:     char *name1	    Source file pathname                      |
|                   char *name2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success, else error and errno set		      |
|                                                                             |
|   Notes:	    Both names must be correct, and paths must exist.	      |
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Compiled-out the error messages output. Another error     |
|                   message is displayed by the caller, and having both is    |
|                   confusing. To do: Build an error message string, and      |
|                   pass it back to the caller.                               |
*                                                                             *
\*---------------------------------------------------------------------------*/

int copy(char *name1, char *name2)
    {
    int e;
    char path[PATHNAME_SIZE];

    strsfp(name2, path, NULL);
    if (!exists(path)) {
      e = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (e) {
      	fprintf(stderr, "\nError: Failed to create directory \"%s\". %s\n", path, strerror(errno));
      	return e;
      }
    }

    e = copyf(name1, name2);
#if NEEDED
    switch (e)
        {
        case 0:
            break;
        case 1:
	    fprintf(stderr, "\nError reading from file \"%s\"\n", name1);
            break;
        case 2:
	    fprintf(stderr, "\nError writing to file \"%s\"\n", name2);
            break;
        default:
            break;
        }
#endif
    return(e);
    }

/******************************************************************************
*									      *
*	File information						      *
*									      *
******************************************************************************/

int exist_file(char *name)	/* Does this file exist? (TRUE/FALSE) */
    {
    FILE *pf;

    DEBUG_ENTER(("exist_file(\"%s\");\n", name));

    pf = fopen(name, "r");
    if (pf)
	{
	fclose(pf);
	RETURN_CONST(TRUE);
	}

    RETURN_CONST(FALSE);
    }

int file_empty(char *name)	/* Is this file empty? (TRUE/FALSE) */
    {
    FILE *pf;
    int	 hfile;
    off_t lon;

    DEBUG_ENTER(("file_empty(\"%s\");\n", name));

    pf = fopen(name, "r");
    if (pf)
	{
	DEBUG_PRINTF(("// File %s exists\n", name));
	hfile = fileno(pf);
	lon = _filelength(hfile);
	fclose(pf);
	RETURN_BOOL(lon == 0);
	}

    RETURN_BOOL_COMMENT(FALSE, ("File %s does not exist\n", name));
    }

int is_directory(char *name)	/* Is name a directory? -> TRUE/FALSE */
    {				/* Les carateres * et ? ne sont pas acceptes */
    int result;
    int err;
    struct stat sstat;

    DEBUG_ENTER(("is_directory(\"%s\");\n", name));

    if (strchr(name, '?') || strchr(name, '*'))
	{ /* Wild cards not allowed */
	RETURN_CONST_COMMENT(FALSE, ("Directory %s does not exist\n", name));
	}

    err = lstat(name, &sstat); // Use lstat, as stat does not detect SYMLINKDs.

    result = ((err == 0) && (sstat.st_mode & S_IFDIR));

    RETURN_BOOL_COMMENT(result, ("Directory %s %s\n", name, result ? "exists"
								   : "does not exist"));
    }

int older(char *p1, char *p2)	/* Is file p1 older than file p2? */
    {
    time_t l1, l2;

    DEBUG_ENTER(("older(\"%s\", \"%s\");\n", p1, p2));

    l2 = getmodified(p2);
    if (l2 == 0L)
	{	      /* p2 does not exist */
	RETURN_BOOL_COMMENT(FALSE, ("File %s is newer than missing %s\n", p1, p2));
	}

    l1 = getmodified(p1);

    RETURN_BOOL_COMMENT(l1 <= l2, ("File %s is %s than file %s\n",
				   p1,
				   (l1 <= l2) ? "older" : "newer",
				   p2));
    }

#define CAST_WORD(u) (*(WORD *)&(u))

time_t getmodified(char *name) {
  int err;
  struct stat sstat;
  time_t result = 0L; /* Return 0 = invalid time for missing file */

  if (name && *name) {
    err = lstat(name, &sstat);
    if (!err) result = sstat.st_mtime;
  }

  DEBUG_PRINTF(("// File \"%s\" date/time = %lX\n", name, (long)result));

  return result;
}

/* Create a directory, and all parent directories as needed. Same as mkdir -p */
#ifdef _MSDOS
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif
int mkdirp(const char *path, mode_t mode) {
  char *pPath = strdup(path);
  char *pc;
  int iErr;
  DEBUG_ENTER(("mkdirp(\"%s\", 0x%X);\n", path, mode));
  if (pPath) {
    for (pc = pPath; *pc; pc++) {
      if ((*pc == DIRSEPARATOR_CHAR) && pc[1] && (pc[1] != DIRSEPARATOR_CHAR)) {
      	*pc = '\0';
      	if (access(pPath, F_OK)) { /* If the intermediate path does not exist */
	  iErr = mkdir(pPath, mode); /* Then create it. */
	  DEBUG_CODE(
	    if (!iErr) {
	      DEBUG_PRINTF(("mkdir(\"%s\"); // Success\n", pPath));
	    } else {
	      XDEBUG_PRINTF(("mkdir(\"%s\"); // Failed. errno=%d\n", pPath, errno));
	    }
	  )
	}
      	*pc = DIRSEPARATOR_CHAR;
      }
    }
  }
  iErr = mkdir(path, mode);
  free(pPath);
  RETURN_INT_COMMENT(iErr, ("%s\n", iErr?"Failed":"Success"));
}
#ifdef _MSDOS
#pragma warning(default:4100)
#endif

/******************************************************************************
*									      *
*	Lattice C emulation						      *
*									      *
******************************************************************************/

char *strgfn(const char *pathname)		    /* Get file name position */
    {
    char *pc;

    /* Search for the end of the path */

    pc = strrchr(pathname, DIRSEPARATOR_CHAR);
    if (pc)
	{
	pc += 1;    /* Skip the \ */
	}
    else
	{
	pc = strrchr(pathname, ':');
	if (pc)
	    {
	    pc += 1;	/* Skip the : */
	    }
	else
	    {
	    pc = (char *)pathname;  /* There is just no path */
	    }
	}

    return pc;
    }

void stcgfn(char *name, const char *pathname)	    /* Get file name */
    {
    strcpy(name, strgfn(pathname)); /* Copy the name part of the pathname */

    return;
    }

void stcgfp(char *path, const char *pathname)	    /* Get file path */
    {
    char *pc;
    size_t n;

    pc = strgfn(pathname);
    n = pc - pathname;		    /* Size of the pathname */
    if (n && (pathname[n-1]==DIRSEPARATOR_CHAR))
	n -= 1; 		    /* Skip back the trailing \ if any */

    strncpy(path, pathname, n);     /* Copy the path part of the pathname */
    path[n] = '\0';

    return;
    }

void strmfp(char *pathname, const char *path, const char *name)   /* Make file pathname */
    {
    size_t l;
    DEBUG_ENTER(("strmfp(%p, \"%s\", \"%s\");\n", pathname, path, name));

    strcpy(pathname, path);
    l = strlen(path);
    if (   (l > 0)
	&& (path[l-1] != DIRSEPARATOR_CHAR)
	&& (path[l-1] != ':')
       )
	{
	  strcat(pathname, DIRSEPARATOR_STRING);
	}
    strcat(pathname, name);
    RETURN_COMMENT(("\"%s\"\n", pathname));
    }

/* 2014-02-21 JFL Made name optional */
void strsfp(const char *pathname, char *path, char *name)   /* Split file pathname */
    {
    const char *p;
    DEBUG_ENTER(("strsfp(\"%s\", %p, %p);\n", pathname, path, name));
    p = strrchr(pathname, DIRSEPARATOR_CHAR);
    if (!p) p = strrchr(pathname, ':');
    if (p) {
      size_t n = p-pathname;
      strncpy(path, pathname, n);
      path[n] = '\0';
      p += 1; /* The name following the last \ or : */
      /* Correct the path if it is a root directory or a pure drive */
      if (!path[0]) {
      	strcpy(path, DIRSEPARATOR_STRING);
      } else if ((*p == ':') && !path[1]) {
      	strcpy(path+1, ":");
      } else if ((path[1] == ':') && !path[2]) {
      	strcpy(path+2, DIRSEPARATOR_STRING);
      }
    } else {
      *path = '\0';
      p = pathname;
    }
    if (name) strcpy(name, p);
    RETURN_COMMENT(("path=\"%s\", name=\"%s\"\n", path, p));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copydate						      |
|									      |
|   Description:    Copy the Date/time stamp from one file to another	      |
|									      |
|   Parameters:     char *pszToFile	Destination file		      |
|		    char *pszFromFile	Source file			      |
|									      |
|   Returns:	    0 = Success, else error and errno set		      |
|									      |
|   Notes:	    This operation is useless if the destination file is      |
|		    written to again. So it's necessary to flush it before    |
|		    calling this function.				      |
|									      |
|   History:								      |
|									      |
|    1996-10-14 JFL Made a clean, application-independant, version.	      |
|    2011-05-12 JFL Rewrote in an OS-independant way.			      |
|    2015-01-08 JFL Fallback to using chmod and utimes if lchmod and lutimes  |
|                   are not implemented. This will cause minor problems if the|
|                   target is a link, but will work well in all other cases.  |
*									      *
\*---------------------------------------------------------------------------*/

/* Old Linux versions define lchmod, but only implement a stub that always fails */
#ifdef __stub_lchmod
// #pragma message "The C Library does not implement lchmod. Using our own replacement."
#define lchmod lchmod1 /* Then use our own replacement for lchmod */
int lchmod1(const char *path, mode_t mode) {
  struct stat st = {0};
  int err;
  DEBUG_PRINTF(("lchmod1(\"%s\", %X);\n", path, mode));
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lchmod all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = chmod(path, mode);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif

#ifndef _STRUCT_TIMEVAL
/* No support for micro-second file time resolution. Use utime(). */
int copydate(char *pszToFile, char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct utimbuf utbTo = {0};
  int err;
  err = lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  utbTo.actime = stFrom.st_atime;
  utbTo.modtime = stFrom.st_mtime;
  err = lutime(pszToFile, &utbTo);
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(utbTo.modtime)); // Time of last data modification
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
    DEBUG_PRINTF(("utime(\"%s\", %s) = %d %s\n", pszToFile, buf, err,
      		  err?strerror(errno):""));
  });
  return err;                       /* Success */
}
#else /* defined(_STRUCT_TIMEVAL) */

#ifdef __stub_lutimes
// #pragma message "The C Library does not implement lutimes. Using our own replacement."
#define lutimes lutimes1 /* Then use our own replacement for lutimes */
int lutimes1(const char *path, const struct timeval times[2]) {
  struct stat st = {0};
  int err;
  // DEBUG_PRINTF(("lutimes1(\"%s\", %p);\n", path, &times)); // No need for this as VALUEIZE(lutimes) duplicates this below.
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lutimes all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = utimes(path, times);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif

/* Micro-second file time resolution supported. Use utimes(). */
int copydate(char *pszToFile, char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct timeval tvTo[2] = {{0}, {0}};
  int err;
  lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  TIMESPEC_TO_TIMEVAL(&tvTo[0], &stFrom.st_atim);
  TIMESPEC_TO_TIMEVAL(&tvTo[1], &stFrom.st_mtim);
  err = lutimes(pszToFile, tvTo);
#ifndef _MSVCLIBX_H_ /* Trace lutimes() call and return in Linux too */
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(stFrom.st_mtime)); // Time of last data modification
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d.%06ld",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec, (long)stFrom.st_mtim.tv_nsec / 1000);
    DEBUG_PRINTF((VALUEIZE(lutimes) "(\"%s\", %s) = %d\n", pszToFile, buf, err));
  });
#endif
  return err;                       /* Success */
}
#endif /* !defined(_STRUCT_TIMEVAL) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _filelength						      |
|									      |
|   Description:    Get the length of an open file			      |
|									      |
|   Parameters:     int hFile		File handle			      |
|									      |
|   Returns:	    The file length					      |
|									      |
|   Notes:	    Unix port of a Microsoft function			      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef __unix__

#define _tell(hFile) lseek(hFile, 0, SEEK_CUR);

off_t _filelength(int hFile) {
  off_t curpos = _tell(hFile);			/* Save the current position */
  off_t length = lseek(hFile, 0, SEEK_END);	/* Move to the end of the file */
  lseek(hFile, curpos, SEEK_SET);		/* Return to the initial position */
  return length;
}

#endif /* defined(__unix__) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function	    fullpath						      |
|									      |
|  Description      Front-end to _fullpath, fixing the trail spaces bug	      |
|									      |
|  Parameters       Same as _fullpath					      |
|									      |
|  Returns	    Same as _fullpath					      |
|									      |
|  Notes	    _fullpath uses WIN32's GetFullPathName, which trims the   |
|		    trailing dots and spaces from path names.		      |
|		    							      |
|  History	    							      |
|    2017-01-30 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

char *fullpath(char *absPath, const char *relPath, size_t maxLength) {
  int i;
  int l = (int)strlen(relPath);
  char *pszRet;
  DEBUG_ENTER(("fullpath(%p, \"%s\", %lu);\n", absPath, relPath, (unsigned long)maxLength));
  pszRet = _fullpath(absPath, relPath, maxLength);
  if (pszRet) {
    size_t  m = strlen(pszRet);  /* Current length of the output string */
    maxLength -= 1;		 /* Maximum length of the output string */
    if (!absPath) maxLength = m; /* _fullpath allocated a buffer */
    for (i=l; i && strchr(". \t", relPath[i-1]); i--) ; /* Search the first trailing dot or space */
    for ( ; i < l; i++) { /* Append the trailing dots or spaces to the output string */
      if (!absPath) {		/* Extend the buffer allocated by _fullpath */
      	char *pszRet2 = realloc(pszRet, ++maxLength + 1);
      	if (!pszRet2) {
      	  free(pszRet);
      	  RETURN_STRING(NULL);
      	}
      	pszRet = pszRet2;
      }
      if (m < maxLength) {
      	pszRet[m++] = relPath[i];
      	pszRet[m] = '\0';
      } else { /* The user-provided buffer is too small */
      	errno = ENOMEM;
      	RETURN_STRING(NULL);
      }
    }
  }
  RETURN_STRING(pszRet);
}

#endif /* _WIN32 */

