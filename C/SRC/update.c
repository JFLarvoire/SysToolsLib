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
*    2017-05-11 JFL Display MsvcLibX library version in DOS & Windows.        *
*		    Version 3.5.2.    					      *
*    2017-05-31 JFL But don't display it in help, to limit the 1st line size. *
*		    Version 3.5.3.    					      *
*    2017-10-06 JFL Fixed a conditional compilation bug in MSDOS.	      *
*		    Fixed support for pathnames >= 260 characters. 	      *
*                   Improved mkdirp() speed and error management.             *
*		    Version 3.5.4.    					      *
*    2018-02-27 JFL All updateall() returns done via :cleanup_and_return.     *
*		    Version 3.5.5.    					      *
*    2018-04-24 JFL Use PATH_MAX and NAME_MAX from limits.h. Version 3.5.6.   *
*    2018-05-31 JFL Use the new zapFile() and zapDir() from zap.c.            *
*		    Added option -e to erase target files not in the source.  *
*                   Bug fix: The force option did corrupt the mode flag.      *
*                   Copy empty directories if the iCopyEmptyFiles flag is set.      *
*                   Bug fix: Avoid a crash in update_link() on invalid links. *
*                   Prefix all error messages with the program name.          *
*                   Bug fix: mkdirp() worked, but returned an error, if the   *
*		     path contained a trailing [back]slash.		      *
*		    Version 3.6.    					      *
*    2018-12-18 JFL Added option -P to show the file copy progress.           *
*		    Added option -- to force ending switches.                 *
*		    Version 3.7.    					      *
*    2019-01-10 JFL Added option -T to reset the time of identical files.     *
*		    Fixed 2018-12-18 bug causing Error: Not enough arguments  *
*		    Corrected assignments in conditional expressions.	      *
*		    Version 3.8.    					      *
*    2019-01-16 JFL Fixed the processing of option --. Really. Version 3.8.1. *
*    2019-04-15 JFL Implemented a fullpath() routine for Linux.		      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.3.8.2.*
*    2019-05-21 JFL Lowered the WIN32 buffer sizes to 256KB, to get smoother  *
*                   progress counts on slow networks. V3.8.3.                 *
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 3.8.4.      *
*    2020-01-28 JFL Fixed issue with D:myFile input files. Version 3.8.5.     *
*    2020-03-11 JFL Fixed issue with Unix readdir() not always setting d_type.*
*                   Fixed a memory leak in updateall().                       *
*                   Fixed serious issue when Linux target is a link to a dir. *
*    2020-03-17 JFL Removed isdir(), and use is_effective_directory() instead.*
*    2020-03-19 JFL Fixed warnings and issues on 32-bit OSs. Version 3.8.6.   *
*    2020-03-23 JFL Renamed options -e|--erase as -c|--clean.                 *
*                   Added options -D|--makedirs, independent of -E|--noempty. *
*    2020-03-24 JFL Renamed options -T|-resettime as -R|-resettime, -D|	      *
*		    --makedirs as -T|--tree, and -S|--showdest as -D|--dest.  *
*                   Added options -C|--command, and -S|--source.              *
*    2020-03-25 JFL Fixed issues with copying a link on a file, or vice-versa.*
*                   Added an updOpts argument to update() & update_link().    *
*    2020-03-26 JFL Added option -B|--nobak to skip backup and temp. files.   *
*                   Version 3.9.                                              *
*    2020-04-19 JFL Added support for MacOS. Version 3.10.                    *
*                                                                             *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Update files based on their time stamps"
#define PROGRAM_NAME    "update"
#define PROGRAM_VERSION "3.10"
#define PROGRAM_DATE    "2020-04-19"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <sys/time.h>		/* For lutimes() */
#include <utime.h>		/* For struct utimbuf */
#include <dirent.h>		/* We use the DIR type and the dirent structure */
#include <unistd.h>		/* For the access function */
#include <fnmatch.h>
#include <iconv.h>
#include <inttypes.h>
/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debugging macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define FALSE 0
#define TRUE 1

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#ifdef _MSC_VER
#pragma warning(disable:4001)	/* Ignore the // C++ comment warning */
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

/* Shell command names, for verbose output */
#define COMMENT   ":# "
#define COPY_FILE "copy"
#define COPY_LINK "xcopy /b"
#define MAKE_DIR  "md"
#define DEL_FILE  "del"
#define DEL_DIR   "rd"

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#define _filelength(hFile) _filelengthi64(hFile)

/* Front-end to _fullpath, with work around for trail spaces bug */
char *fullpath(char *absPath, const char *relPath, size_t maxLength);  

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

/* Shell command names, for verbose output */
#define COMMENT   ":# "
#define COPY_FILE "copy"
#define COPY_LINK "rem"
#define MAKE_DIR  "md"
#define DEL_FILE  "del"
#define DEL_DIR   "rd"

#define fullpath _fullpath

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

/* Shell command names, for verbose output */
#define COMMENT   ":# "
#define COPY_FILE "copy"
#define COPY_LINK "rem"
#define MAKE_DIR  "md"
#define DEL_FILE  "del"
#define DEL_DIR   "rd"

#define fullpath _fullpath

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define PATTERN_ALL "*"     		/* Pattern matching all files */

/* Shell command names, for verbose output */
#define COMMENT   "# "
#define COPY_FILE "cp -p"
#define COPY_LINK "cp -p -P"
#define MAKE_DIR  "mkdir"
#define DEL_FILE  "rm"
#define DEL_DIR   "rmdir"

#define _stricmp strcasecmp

/* Redefine Microsoft-specific routines */
off_t _filelength(int hFile);
// Don't use realpath(), as it resolves links, which we do not want.
// #define fullpath(absPath, relPath, maxLength) realpath(relPath, absPath)
char *fullpath(char *absPath, const char *relPath, size_t maxLength);
#define LocalFileTime localtime

/* In MacOS, these struct stat fields have different names */
#if defined(__MACH__)
#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#endif

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(EXE_OS_NAME))
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

#define PATHNAME_SIZE PATH_MAX
#define NODENAME_SIZE (NAME_MAX+1)

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
#define BUFFERSIZE (256L * 1024L)
#endif
char *buffer;       /* Pointer on the intermediate copy buffer */

#define isConsole(iFile) isatty(iFile)

static int test = 0;			/* Flag indicating Test mode */
#define SHOW_NONE    0
#define SHOW_SOURCE  1
#define SHOW_DEST    2
#define SHOW_COMMAND 3
static int show = SHOW_SOURCE;		/* Display 0=Nothing; 1=Source; 2=Dest; 3=Command */
static int fresh = 0;			/* Flag indicating freshen mode */
static int force = 0;			/* Flag indicating force mode */
static int iVerbose = FALSE;		/* Flag for displaying verbose information */
static int iCopyEmptyFiles = TRUE;	/* Flag for copying empty files */
static int iCopyEmptyDirs = FALSE;	/* Flag for copying empty directories */
static int iPause = 0;			/* Flag for stop before exit */
static int iProgress = 0;		/* Flag for showing a progress bar */
#ifdef _UNIX
static int iFnmFlag = 0;		/* Case-sensitive pattern matching */
#else
static int iFnmFlag = FNM_CASEFOLD;	/* Case-insensitive pattern matching */
#endif
static int iRecur = 0;			/* Recursive update */
#ifdef _WIN32
// UINT cp = 0;				/* Initial console code page */
#define cp codePage			/* Initial console code page in iconv.c */
#endif
static int iClean = 0;			/* Flag indicating Clean mode */
static int iResetTime = 0;		/* Reset time of identical files */
static int nobak = FALSE;		/* Flag for skipping backup files */

/* update() and update_link() functions options */
typedef struct updOpts {
  int iFlags;				/* Same FLAG_xxx as zapOpts below */
  int *pmdDone;				/* Optional pointer to a flag that records if the target directory has already be created */
} updOpts;

/* Forward references */

void usage(void);			/* Display usage */
int IsSwitch(char *pszArg);		/* Is this a command-line switch? */
int updateall(char *, char *);		/* Copy a set of files if newer */
int update(char *, char *, updOpts *);	/* Copy a file if newer */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK)/* In DOS it's defined, but always returns 0 */
int update_link(char *, char *, updOpts *);	/* Copy a link if newer */
#endif
int copyf(char *, char *);		/* Copy a file silently */
int copy(char *, char *);		/* Copy a file and display messages */
int mkdirp(const char *path, mode_t mode); /* Same as mkdir -p */

int exists(char *name);			/* Does this pathname exist? (TRUE/FALSE) */
int exist_file(char *); 		/* Does this file exist? (TRUE/FALSE) */
int file_empty(char *); 		/* Is this file empty? (TRUE/FALSE). */
					/* File must exist (else return FALSE)  */
int is_directory(char *);		/* Is name a directory? -> TRUE/FALSE */
int is_effective_directory(char *name); /* Is name a directory, or a link to a directory? */
int older(char *, char *);		/* Is file 1 older than file 2? */
time_t getmodified(char *);		/* Get time of file modification */
int copydate(char *to, char *from);	/* Copy the file date & time */
int filecompare(char *, char *);	/* Compare two files */

char *strgfn(const char *);		/* Get file name position */
void stcgfn(char *, const char *);	/* Get file name */
void stcgfp(char *, const char *);	/* Get file path */
void strmfp(char *, const char *, const char *);    /* Make file pathname */
void strsfp(const char *, char *, char *);          /* Split file pathname */
char *NewPathName(const char *path, const char *name); /* Create a new pathname */
/* zap functions options */
typedef struct zapOpts {
  int iFlags;
  char *pszPrefix;
} zapOpts;
/* zapOpts iFlags */
#define FLAG_VERBOSE	0x0001		/* Display the pathname operated on */
#define FLAG_NOEXEC	0x0002		/* Do not actually execute */
#define FLAG_RECURSE	0x0004		/* Recursive operation */
#define FLAG_NOCASE	0x0008		/* Ignore case */
#define FLAG_FORCE	0x0010		/* Force operation on read-only files */
#define FLAG_COMMAND	0x0020		/* Force operation on read-only files */
int zapFile(const char *path, zapOpts *pzo); /* Delete a file */
int zapFileM(const char *path, int iMode, zapOpts *pzo); /* Faster */
int zapDir(const char *path, zapOpts *pzo);  /* Delete a directory */
int zapDirM(const char *path, int iMode, zapOpts *pzo); /* Faster */

/* Global program name variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */
int printError(char *pszFormat, ...);	/* Print errors in a consistent format */

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

int main(int argc, char *argv[]) {
  int iArg;			/* Argument index */
  char *arg;
#if defined(_MSDOS) || defined(_WIN32)
  size_t len;
#endif
  char *target;
  int nErrors = 0;
  int iExit = 0;
  int iProcessSwitches = TRUE;

  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  for (iArg = 1; iArg<argc; iArg += 1) {
    arg = argv[iArg];
    if (iProcessSwitches && IsSwitch(arg)) {
      char *opt = arg + 1;
      DEBUG_PRINTF(("Switch = %s\n", arg));
      if (   streq(opt, "-")) {	    /* Force end of switches */
	iProcessSwitches = FALSE;
	break;
      }
      if (   streq(opt, "h")	    /* Display usage */
	  || streq(opt, "help")	    /* The historical name of that switch */
	  || streq(opt, "-help")
	  || streq(opt, "?")) {
	usage();
      }
#ifdef _WIN32
      if (   streq(opt, "A")
	  || streq(opt, "-ansi")) {   /* Force encoding output with the ANSI code page */
	cp = CP_ACP;
	continue;
      }
#endif
      if (   streq(opt, "B")	    /* Skip backup files */
	  || streq(opt, "-nobak")) {
	nobak = TRUE;
	if (iVerbose) printf(COMMENT "Skip backup files mode = on\n");
	continue;
      }
      if (   streq(opt, "c")	    /* Clean mode on */
	  || streq(opt, "-clean")) {
	iClean = TRUE;
	if (iVerbose) printf(COMMENT "Clean mode = on\n");
	continue;
      }
      if (   streq(opt, "C")     /* Show the equivalent shell command */
	  || streq(opt, "-command")) {
	show = SHOW_COMMAND;
	if (iVerbose) printf(COMMENT "Show mode = Equivalent shell command\n");
	continue;
      }
      DEBUG_CODE(
      if (   streq(opt, "d")	    /* Debug mode on */
	  || streq(opt, "debug")	    /* The historical name of that switch */
	  || streq(opt, "-debug")) {
	DEBUG_MORE();
	if (iVerbose) printf(COMMENT "Debug mode = on\n");
	continue;
      }
      )
      if (   streq(opt, "D")     /* Show dest instead of source */
	  || streq(opt, "-dest")) {
	show = SHOW_DEST;
	if (iVerbose) printf(COMMENT "Show mode = Destination files names\n");
	continue;
      }
      if (   streq(opt, "E")	    /* NoEmpty files mode on */
	  || streq(opt, "noempty")    /* The historical name of that switch */
	  || streq(opt, "-noempty")) {
	iCopyEmptyFiles = FALSE;
	if (iVerbose) printf(COMMENT "No empty files mode = on\n");
	continue;
      }
      if (   streq(opt, "f")	    /* Freshen mode on */
	  || streq(opt, "-freshen")) {
	fresh = 1;
	if (iVerbose) printf(COMMENT "Freshen mode = on\n");
	continue;
      }
      if (   streq(opt, "F")	    /* Force mode on */
	  || streq(opt, "-force")) {
	force = 1;
	if (iVerbose) printf(COMMENT "Force mode = on\n");
	continue;
      }
      if (   streq(opt, "i")	    /* Case-insensitive pattern matching */
	  || streq(opt, "-ignorecase")) {
	iFnmFlag |= FNM_CASEFOLD;
	if (iVerbose) printf(COMMENT "Pattern matching = Case-insensitive \n");
	continue;
      }
      if (   streq(opt, "k")	    /* Case-sensitive pattern matching */
	  || streq(opt, "-casesensitive")) {
	iFnmFlag &= ~FNM_CASEFOLD;
	if (iVerbose) printf(COMMENT "Pattern matching = Case-sensitive\n");
	continue;
      }
#ifdef _WIN32
      if (   streq(opt, "O")
	  || streq(opt, "-oem")) {    /* Force encoding output with the OEM code page */
	cp = CP_OEMCP;
	continue;
      }
#endif
      if (   streq(opt, "p")	    /* Final Pause on */
	  || streq(opt, "-pause")) {
	iPause = 1;
	if (iVerbose) printf(COMMENT "Final Pause = on\n");
	continue;
      }
      if (   streq(opt, "P")	    /* Show file copy progress */
	  || streq(opt, "-progress")) {
	if (isConsole(fileno(stdout))) { /* Only do it when outputing to the console */
	  iProgress = 1;
	  if (iVerbose) printf(COMMENT "Show file copy progress\n");
	}
	continue;
      }
      if (   streq(opt, "q")	    /* Quiet/Nologo mode on */
	  || streq(opt, "-quiet")) {
	iVerbose = FALSE;
	show = SHOW_NONE;
	continue;
      }
      if (   streq(opt, "r")	    /* Recursive update */
	  || streq(opt, "-recurse")) {
	iRecur = 1;
	if (iVerbose) printf(COMMENT "Recursive update\n");
	continue;
      }
      if (   streq(opt, "R")	    /* Reset time of identical files */
	  || streq(opt, "-resettime")) {
	iResetTime = 1;
	if (iVerbose) printf(COMMENT "Reset time of equal files\n");
	continue;
      }
      if (   streq(opt, "S")     /* Show source files */
	  || streq(opt, "-source")) {
	show = SHOW_SOURCE;
	if (iVerbose) printf(COMMENT "Show mode = Source files names\n");
	continue;
      }
      /* Note:     opt  "t"  is already used, as a synonym for -X */
      if (   streq(opt, "T")	    /* Make empty directories mode on */
	  || streq(opt, "-tree")) {
	iCopyEmptyDirs = TRUE;
	if (iVerbose) printf(COMMENT "Make empty dirs mode = on\n");
	continue;
      }
      if (   streq(opt, "tf")) {    /* Test the fullpath() routine */
	if (iVerbose) printf(COMMENT "Test the fullpath() routine\n");
	printf("%s\n", fullpath(NULL, argv[++iArg], 0));
	exit(0);
      }
#ifdef _WIN32
      if (   streq(opt, "U")
	  || streq(opt, "-utf8")) {   /* Force encoding output with the UTF-8 code page */
	cp = CP_UTF8;
	continue;
      }
#endif
      if (   streq(opt, "v")	    /* Verbose mode on */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	printf(COMMENT "Verbose mode = on\n");
	continue;
      }
      if (   streq(opt, "V")	    /* -V: Display the version */
	  || streq(opt, "-version")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
      if (   streq(opt, "X")	    /* NoExec/Test mode on */
	  || streq(opt, "-noexec")
	  || streq(opt, "t")) {	    /* The historical name of that switch */
	test = 1;
	if (iVerbose) printf(COMMENT "NoExec/Test mode = on\n");
	continue;
      }
      fprintf(stderr, "Warning: Unrecognized switch %s ignored.\n", arg);
      continue;
    }
    /* This is an argument, not a switch */
    break;	/* All arguments are processed below, outside of this loop */
  }

  if ( (argc - iArg) < 1 ) {
    fprintf(stderr, "Error: Not enough arguments.\n");
    do_exit(1);
  }

  buffer = malloc(BUFFERSIZE);	/* Allocate memory for copying */
  if (!buffer) {
    fprintf(stderr, "Error: Not enough memory.\n");
    do_exit(1);
  }

  DEBUG_PRINTF(("Size of size_t = %d bits\n", (int)(8*sizeof(size_t))));
  DEBUG_PRINTF(("Size of off_t = %d bits\n", (int)(8*sizeof(off_t))));
  DEBUG_PRINTF(("Size of dirent = %d bytes\n", (int)(sizeof(struct dirent))));
#if  defined(__unix__) && defined(_LARGEFILE64_SOURCE) && defined(__GNUC__) && (__GNUC__ > 2)
  DEBUG_PRINTF(("Size of dirent64 = %d bytes\n", (int)(sizeof(struct dirent64))));
#endif
#if defined(_MSC_VER)
#pragma warning(disable:4003) /* Ignore the "not enough actual parameters for macro" warning */
#endif
  DEBUG_CODE_IF_ON(
    DEBUG_PRINT_MACRO(__USE_FILE_OFFSET64);		/* Set by GNU LIBC */
    DEBUG_PRINT_MACRO(__USE_LARGEFILE64);		/* Set by GNU LIBC */
    DEBUG_PRINT_MACRO(_DIRENT_MATCHES_DIRENT64);	/* Set by GNU LIBC */
    DEBUG_PRINT_MACRO(_DIRENT_HAVE_D_TYPE);		/* Set by GNU LIBC */
    DEBUG_PRINT_MACRO(_DIRENT_HAVE_D_OFF);		/* Set by GNU LIBC */
    DEBUG_PRINT_MACRO(readdir);				/* May be redefined as readdir64, etc */
  )
#if defined(_MSC_VER)
#pragma warning(default:4003) /* Restore the "not enough actual parameters for macro" warning */
#endif

  target = argv[--argc];	/* The last argument is the target */
  DEBUG_PRINTF(("Target = %s\n", target));
#if defined(_MSDOS) || defined(_WIN32)
  /* Workaround for a command.com or cmd.exe bug */
  len = strlen(target);
  if (len && (target[len-1] == '"')) {
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
      printError("Error: Cannot access drive %c: %s", target[0], strerror(errno));
      do_exit(1);
    }
  }
#endif

  for ( ; iArg < argc; iArg++) { /* For every source file before that */
    arg = argv[iArg];
    nErrors += updateall(arg, target);
  }

  if (nErrors) { /* Display a final summary, as the errors may have scrolled up beyond view */
    printError("Error: %d file(s) failed to be updated", nErrors);
    iExit = 1;
  }

  do_exit(iExit);
  return iExit;
}

void usage(void)
    {
    printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: update [SWITCHES] FILES DIRECTORY\n\
       update [SWITCHES] FILES DIRECTORY" DIRSEPARATOR_STRING "NEWDIR" DIRSEPARATOR_STRING "\n\
       update [SWITCHES] FILE  DIRECTORY[" DIRSEPARATOR_STRING "NEWNAME]\n\
\n\
Files:          FILE1 [FILE2 ...]\n\
                Wildcards are allowed in source files pathnames\n\
\n\
Switches:\n\
  --            End of switches\n"
#ifdef _WIN32
"\
  -A|--ansi     Force encoding the output using the ANSI character set\n\
"
#endif
"\
  -B|--nobak    Skip backup and temporary files *.bak|*~|#*#\n\
  -c|--clean    Clean mode: Delete destination files not in the source set\n\
  -C|--command  Display the equivalent shell commands\n\
"
#ifdef _DEBUG
"\
  -d|--debug    Output debug information\n"
#endif
"\
  -D|--dest     Display destination files copied\n\
  -E|--noempty  Don't copy empty files\n\
");

    printf("\
  -f|--freshen  Update only files that exist in both directories\n\
  -F|--force    Overwrite read-only files\n\
  -h|--help|-?  Display this help screen and exit\n\
  -i|--ignorecase    Case-insensitive pattern matching. Default for DOS/Windows\n\
  -k|--casesensitive Case-sensitive pattern matching. Default for Unix\n"
#ifdef _WIN32
"\
  -O|--oem      Force encoding the output using the OEM character set\n"
#endif
"\
  -p|--pause    Pause before exit\n\
  -P|--progress Display the file copy progress. Useful with very large files\n\
  -q|--quiet    Don't display anything\n\
  -r|--recurse  Recursively update all subdirectories\n\
  -R|--resettime Reset time of identical files\n\
  -S|--source   Display source files copied (Default)\n\
"
#ifdef _WIN32
"\
  -U|--utf8     Force encoding the output using the UTF-8 character encoding\n"
#endif
"\
  -v|--verbose  Display extra status information\n\
  -V|--version  Display this program version and exit\n\
  -X|-t         Noexec/test mode: Display what would be done, but don't do it\n\
\n\
Note: Options -C -D -q -S override each other. The last one provided wins.\n\
\n\
"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef _UNIX
"\n"
#endif
);

    do_exit(0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description     Test if a command line argument is a switch.	      |
|									      |
|   Parameters      char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
#if defined(_WIN32) || defined(_MSDOS)
    case '/':
#endif
      return (*(short*)pszArg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
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
    zapOpts zo = {FLAG_VERBOSE, "- "};
    int iFlags = 0;
    int mdDone = FALSE;
    updOpts uo = {0};

    if (iRecur) iFlags |= FLAG_RECURSE;
    if (test) iFlags |= FLAG_NOEXEC;
    if (force) iFlags |= FLAG_FORCE;
    if (show == SHOW_COMMAND) iFlags |= FLAG_COMMAND;
    zo.iFlags |= iFlags;
    uo.iFlags |= iFlags;
    uo.pmdDone = &mdDone;

    DEBUG_ENTER(("updateall(\"%s\", \"%s\");\n", p1, p2));

#ifndef _MSDOS
    path0 = malloc(PATHNAME_SIZE);
    path1 = malloc(PATHNAME_SIZE);
    path2 = malloc(PATHNAME_SIZE);
    path3 = malloc(PATHNAME_SIZE);
    path = malloc(PATHNAME_SIZE);
    name = malloc(PATHNAME_SIZE);
    fullpathname = malloc(PATHNAME_SIZE);
    if ((!path0) || (!path1) || (!path2) || (!path3) || (!path) || (!name) || (!fullpathname)) {
      printError("Error: Not enough memory");
      nErrors += 1;
      goto cleanup_and_return;
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
      } else { /* There's no / in the pathname */
	pattern = p1;
#if defined(_MSDOS) || defined(_WIN32)
      	if (path0[0] && (path0[1] == ':')) {
      	  if (path0[2]) {	/* path0 was like "D:something" */
      	    pattern = p1 + 2;
      	  } else {		/* path0 was like "D:" */
	    pattern = PATTERN_ALL;
      	  }
	  strcpy(path0+2, ".");
      	} else
#endif /* defined(_MSDOS) || defined(_WIN32) */
	strcpy(path0, ".");
      }
    }

    if (iVerbose) {
      printf(COMMENT "Update %s from %s to %s\n", pattern, path0, p2);
    }

    /* Check if the target is a file or directory name */
    /* Important: We must accept both real directories, and links to directories.
		  Else, there's a risk to delete links in Linux. (This happened!)
		  Ex: `./update update /bin` must not overwrite /bin if it's a link to /usr/bin */
    ppath = p2;
    pname = NULL; /* Implies using pDE->d_name */
    strsfp(p2, path, name);
    if (name[0] && is_effective_directory(path) && (!is_effective_directory(p2)) && (!strpbrk(p1, "*?"))) {
      ppath = path;
      pname = name;
      DEBUG_PRINTF(("// The target is file %s in directory %s\n", pname, ppath));
    } else {
      DEBUG_PRINTF(("// The target is directory %s\n", ppath));
    }
    iTargetDirExisted = is_effective_directory(ppath);

    /* Note: Scan the source directory even in the absence of wildcards.
       In Windows, this makes sure that the copy has the same case as
       the source, even if the command-line argument has a different case. */

    /* Scan all files that match the wild cards */
    pDir = opendirx(path0);
    if (!pDir) {
      printError("Error: can't open directory \"%s\": %s", path0, strerror(errno));
      nErrors += 1;
      goto cleanup_and_return;
    }
    while ((pDE = readdirx(pDir)) != NULL) {
      DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
      if (   (pDE->d_type != DT_REG)
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      	  && (pDE->d_type != DT_LNK)
#endif
      	 ) continue;	/* We want only files or links */
      if (fnmatch(pattern, pDE->d_name, iFnmFlag) == FNM_NOMATCH) continue;
      if (nobak) {
	static char *patterns[] = {"*.bak", "*~", "#*#", NULL};
	char **ppPattern;
	for (ppPattern = patterns; *ppPattern; ppPattern++) {
	  if (fnmatch(*ppPattern, pDE->d_name, iFnmFlag) == 0) break; /* Match */
	}
	if (*ppPattern) continue; /* There was a match, so skip this backup file */
      }
      strmfp(path1, path0, pDE->d_name);  /* Compute source path */
      DEBUG_PRINTF(("// Found %s\n", path1));
      strmfp(path2, ppath, pname?pname:pDE->d_name); /* Append it to directory p2 too */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      if (pDE->d_type == DT_LNK) {
	err = update_link(path1, path2, &uo); /* Displays error messages on stderr */
      }
      else
#endif
      {
      	err = update(path1, path2, &uo); /* Does not display error messages on stderr */
	if (err) {
	  printError("Error: Failed to create \"%s\". %s", path2, strerror(errno));
	}
      }
      if (err) {
      	nErrors += 1;
      	/* Continue the directory scan, looking for other files to update */
      }
    }
    closedirx(pDir);

    /* Scan target files that might be erased */
    if (iClean) {
      fullpath(path2, p2, PATHNAME_SIZE); /* Build absolute pathname of source */
      pDir = opendirx(p2);
      if (pDir) {
	while ((pDE = readdirx(pDir)) != NULL) {
	  struct stat sStat;
	  if (streq(pDE->d_name, ".")) continue;    /* Skip the . directory */
	  if (streq(pDE->d_name, "..")) continue;   /* Skip the .. directory */
	  DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
	  if (fnmatch(pattern, pDE->d_name, iFnmFlag) == FNM_NOMATCH) continue;
	  strmfp(path3, path2, pDE->d_name);  /* Compute the target file pathname */
	  DEBUG_PRINTF(("// Found %s\n", path3));
	  strmfp(path1, path0, pDE->d_name); /* Compute the corresponding source file pathname */
	  if (access(path1, F_OK) == -1) { /* If that source file does not exist */
	    char *pszType = "file";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
	    err = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
	    err = -lstat(path3, &sStat); /* If error, iErr = 1 = # of errors */
#endif
	    if (err) {
	      printError("Error: Can't stat \"%s\"", path1);
	      nErrors += 1;
	      continue;
	    }
	    switch (pDE->d_type) {
	      case DT_DIR:
		err = zapDirM(path3, sStat.st_mode, &zo);
		nErrors += err;
		break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
	      case (DT_LNK):
	      	pszType = "link";
		// Fall through
#endif
	      case DT_REG:
	      	err = zapFileM(path3, sStat.st_mode, &zo);
		if (err) {
		  printError("Error: Can't delete %s \"%s\"", pszType, path3);
		  nErrors += 1;
		}
		break;
	      default:
		printError("Error: Can't delete \"%s\"", path3);
		nErrors += 1;
		break;
	    }
	  }
	}
      }
      closedirx(pDir);
    }

    if (iRecur) { /* Parse the directory again, looking for actual directories (not junctions nor symlinkds) */
      pDir = opendirx(path0);
      if (!pDir) {
	printError("Error: Can't open directory \"%s\": %s", path0, strerror(errno));
      	nErrors += 1;
        goto cleanup_and_return;
      }
      while ((pDE = readdirx(pDir)) != NULL) {
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
	  if (p2_exists && !p2_is_dir) {
	    err = zapFile(path2, &zo); /* Delete the conflicting file/link */
	    if (err) {
	      printError("Error: Failed to remove \"%s\"", path2);
	      nErrors += 1;
	      continue;	/* Try updating something else */
	    }
	    p2_exists = FALSE;
	  }
	  /* 2015-01-12 JFL Don't create the directory now, as it may never be needed,
			      if there are no files that match the input pattern */
	  /* 2018-05-31 JFL Actually do it, but only if the iCopyEmptyFiles flag is set */
	  /* 2020-03-23 JFL Create a new iCopyEmptyDirs flag, to control this independently of the iCopyEmptyFiles flag */
	  if (iCopyEmptyDirs && !p2_exists) { /* Create the missing target directory */
	    if (show == SHOW_COMMAND) {
	      printf(MAKE_DIR " \"%s\"\n", path2);
	    } else if (show) {
	      printf("%s\\\n", fullpathname);
	    }
	    if (!test) {
	      err = mkdirp(path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	      if (err) {
		printError("Error: Failed to create directory \"%s\". %s", path2, strerror(errno));
		nErrors += 1;
		continue;	/* Try updating something else */
	      }
	    }
	  }
	}

	err = updateall(path1, path2);
	if (err) nErrors += err;

	if (!p2_exists) { /* If we did create the target subdir */
	  copydate(path2, path3); /* Make sure the directory date matches too */
	}
      }
      closedirx(pDir);
    }

    if ((!iTargetDirExisted) && is_directory(ppath)) { /* If we did create the target dir */
      copydate(ppath, path0); /* Make sure the directory date matches too */
    }

cleanup_and_return:
#ifndef _MSDOS
    free(path0); free(path1); free(path2); free(path3); free(path); free(name); free(fullpathname);
#endif
    RETURN_INT(nErrors);
    }

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
           char *p2,
           updOpts *puo)
    {
    int err;
    struct stat sP2stat = {0};
    char *p;
    int iCheckOlder = TRUE;
    char path[PATHNAME_SIZE];

    DEBUG_ENTER(("update(\"%s\", \"%s\");\n", p1, p2));

    /* Get the pathname to display, before p2 is possibly modified by the test mode */
    p = p1;				/* By default, show the source file name */
    if (show == SHOW_DEST) p = p2;	/* But in showdest mode, show the destination file name */

    /* In freshen mode, don't copy if the destination does not exist. */
    if (fresh && !exist_file(p2)) RETURN_CONST(0);

    /* In Noempty mode, don't copy empty file */
    if ( (iCopyEmptyFiles == FALSE) && (file_empty(p1)) ) RETURN_CONST(0);

    /* If the target exists, make sure it's a file */
    err = lstat(p2, &sP2stat); /* Use lstat to avoid following links */
    if (err == 0) {
      zapOpts zo = {FLAG_VERBOSE | FLAG_RECURSE, "- "};
      if (test) zo.iFlags |= FLAG_NOEXEC;
      if (force) zo.iFlags |= FLAG_FORCE;
      if (show == SHOW_COMMAND) zo.iFlags |= FLAG_COMMAND;
      if (S_ISDIR(sP2stat.st_mode)) {	/* If the target is a directory */
      	zo.iFlags |= FLAG_VERBOSE; /* Show what's deleted, beyond the obvious target itself */
      	err = zapDirM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
	if (test) p2 = ""; /* Trick older() to think the target is deleted */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      } else if (S_ISLNK(sP2stat.st_mode)) { /* Else if it's a link */
      	zo.iFlags &= ~FLAG_VERBOSE; /* No need to show that that target is deleted */
	err = zapFileM(p2, sP2stat.st_mode, &zo);	/* Deletes the link, not its target. */
	if (test) iCheckOlder = FALSE; /* Skip older() to do as if the target had been deleted */
#endif /* defined(S_ISLNK) */
      } else if (!S_ISREG(sP2stat.st_mode)) { /* If it's anything else but a file */
      	printError("Can't replace \"%s\" with a file", p2);
      	RETURN_INT(EBADF);
      } /* Else the target is a plain file */
      if (err) {
      	printError("Failed to remove \"%s\"", p2);
      	RETURN_INT(err);
      }
    }

    /* In ResetTime mode, check if the files are identical, but dates have changed */
    if (iResetTime) {
      struct stat sP1stat = {0};
      err = lstat(p1, &sP1stat); /* Use lstat to avoid following links */
      if ((err == 0) && (sP1stat.st_size == sP2stat.st_size) && (sP1stat.st_mtime < sP2stat.st_mtime)) {
      	if (!filecompare(p1, p2)) {
	  int seconde, minute, heure, jour, mois, an;
	  struct tm *pTime = LocalFileTime(&(sP1stat.st_mtime)); // Time of last data modification
	  seconde = pTime->tm_sec;
	  minute = pTime->tm_min;
	  heure = pTime->tm_hour;
	  jour = pTime->tm_mday;
	  mois = pTime->tm_mon + 1;
	  an = pTime->tm_year + 1900;
	  if (iVerbose) {
	    if (show == SHOW_COMMAND) printf("cfdt ");
	    printf("%04d-%02d-%02d %02d:%02d:%02d", an, mois, jour, heure, minute, seconde);
	    printf(show == SHOW_COMMAND ? " \"" : " -> ");
	    printf("%s", p2);
	    printf(show == SHOW_COMMAND ? "\"\n" : "\n");
	  }
	  if (test) RETURN_CONST(0);
	  err = copydate(p2, p1);
	  if (err) {
	    printError("Failed to set time for \"%s\"", p2);
	    RETURN_INT(err);
	  }
      	}
      }
      RETURN_CONST(0);
    }

    /* In any mode, don't copy if the destination is newer than the source. */
    if (iCheckOlder && older(p1, p2)) RETURN_CONST(0);

    /* Create the destination directory if needed */
    strsfp(p2, path, NULL);
    if (!exists(path)) {
      if (!(puo && puo->pmdDone && *(puo->pmdDone))) { /* Avoid displaying this multiple times in test mode */
	if (show == SHOW_COMMAND) {
	  char *fullname = malloc(PATHNAME_SIZE);
	  if (fullname) {
	    fullpath(fullname, path, PATHNAME_SIZE); /* Build the absolute pathname of directory */
	    printf(MAKE_DIR " \"%s\"\n", fullname);
	    free(fullname);
	  }
	}
      }
      err = 0;
      if (!test) err = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (err) {
      	printError("Error: Failed to create directory \"%s\". %s", path, strerror(errno));
	RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
      }
      if (puo && puo->pmdDone) *(puo->pmdDone) = TRUE; /* Avoid displaying this multiple times in test mode */
    }

    /* Display what is being copied */
    if (show == SHOW_COMMAND) {
      char *name1 = malloc(PATHNAME_SIZE);
      char *name2 = malloc(PATHNAME_SIZE);
      if (name1 && name2) {
	fullpath(name1, p1, PATHNAME_SIZE); /* Build absolute pathname of source */
	fullpath(name2, p2, PATHNAME_SIZE); /* Build absolute pathname of destination */
	printf(COPY_FILE " \"%s\" \"%s\"\n", name1, name2);
      }
      free(name1);
      free(name2);
    } else if (show) {
      char *name = malloc(PATHNAME_SIZE);
      fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of file */
      printf("%s\n", name);
    }

    if (test == 1) RETURN_CONST(0);

    err = copy(p1, p2);

    RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
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

    result = !lstat(name, &sstat); // Use lstat, to detect even dangling links

    RETURN_BOOL(result);
}

#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */

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
                char *p2,
	        updOpts *puo)
    {
    int err;
    char name[PATHNAME_SIZE];
    char target1[PATHNAME_SIZE];
    int iSize;
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

    if (bP2Exists) { // Then the target has to be removed, even if it's a link
      zapOpts zo = {FLAG_VERBOSE | FLAG_RECURSE, "- "};
      if (test) zo.iFlags |= FLAG_NOEXEC;
      if (force) zo.iFlags |= FLAG_FORCE;
      if (show == SHOW_COMMAND) zo.iFlags |= FLAG_COMMAND;
      // First, in force mode, prevent failures if the target is read-only
      if (force && !(sP2stat.st_mode & S_IWRITE)) {
      	int iMode = sP2stat.st_mode | S_IWRITE;
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", p2, iMode));
      	err = chmod(p2, iMode); /* Try making the target file writable */
      	DEBUG_PRINTF(("  return %d; // errno = %d\n", err, errno));
      }
      if (S_ISDIR(sP2stat.st_mode)) {
      	zo.iFlags |= FLAG_VERBOSE; /* Show what's deleted, beyond the obvious target itself */
	err = zapDirM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
      } else { // It's a file or a link
      	zo.iFlags &= ~FLAG_VERBOSE; /* No need to show that that target is deleted */
	err = zapFileM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
      	if (err) printError("Error: Failed to remove \"%s\"", p2);
      }
      if (err) RETURN_INT(err);
    }

    /* Create the destination directory if needed */
    strsfp(p2, path, NULL);
    if (!exists(path)) {
      if (!(puo && puo->pmdDone && *(puo->pmdDone))) { /* Avoid displaying this multiple times in test mode */
	if (show == SHOW_COMMAND) {
	  char *fullname = malloc(PATHNAME_SIZE);
	  if (fullname) {
	    fullpath(fullname, path, PATHNAME_SIZE); /* Build the absolute pathname of directory */
	    printf(MAKE_DIR " \"%s\"\n", fullname);
	    free(fullname);
	  }
	}
      }
      err = 0;
      if (!test) err = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (err) {
      	printError("Error: Failed to create directory \"%s\". %s", path, strerror(errno));
	RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
      }
      if (puo && puo->pmdDone) *(puo->pmdDone) = TRUE; /* Avoid displaying this multiple times in test mode */
    }

    /* Display what is being copied */
    p = p1;				/* By default, show the source file name */
    if (show == SHOW_DEST) p = p2;	/* But in showdest mode, show the destination file name */
    fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of source */
    if (show == SHOW_COMMAND) {
      printf(COPY_LINK " \"%s\" \"%s\"\n", p1, p2);
    } else if (show) {
      printf("%s\n", name);
    }
    if (test == 1) RETURN_CONST(0);

    /* Get the link target, and give up if we can't read it */
    iSize = (int)readlink(p1, target1, sizeof(target1));
    if (iSize < 0) { /* This may fail for Linux Sub-System Symbolic Links on Windows */
      err = errno;
      printError("Error: Failed to read link \"%s\"", p1);
      RETURN_INT(err);
    }
    DEBUG_PRINTF(("// Target1=\"%s\", iSize=%d\n", target1, iSize));

    /* Create the link copy */
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
    if (err) {
      printError("Error: Failed to create link \"%s\". %s", p2, strerror(errno));
    }

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
    off_t filelen;	    /* File length */
    size_t tocopy;	    /* Number of bytes to copy in one pass */
    int iShowCopying = FALSE;
    int nAttempt = 1;	    /* Force mode allows retrying a second time */
    off_t offset;
    int iWidth = 0;	    /* Number of characters in the iProgress output */
    char *pszUnit = "B";    /* Unit used for iProgress output */
    long lUnit = 1;	    /* Number of bytes for 1 iProgress unit */

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

    filelen = _filelength(hsource);
    /* Read 1 byte to test access rights. This avoids destroying the target
       if we don't have the right to read the source. */
    if (filelen && !fread(buffer, 1, 1, pfs)) {
      if (iShowCopying) printf("\n");
      RETURN_INT_COMMENT(1, ("Can't read the input file\n"));
    }
    fseek(pfs, 0, SEEK_SET);
retry_open_targetfile:
    pfd = fopen(name2, "wb");
    if (!pfd) {
      if ((errno == EACCES) && (nAttempt == 1) && force) {
      	struct stat sStat = {0};
      	int iErr = stat(name2, &sStat);
      	int iMode = sStat.st_mode | S_IWUSR;
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", name2, iMode));
      	iErr = chmod(name2, iMode); /* Try making the target file writable */
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

    if (iShowCopying) printf(" : %"PRIuMAX" bytes\n", (uintmax_t)filelen);

    if (iProgress) {
      if (filelen > (100*1024L*1024L)) {
      	lUnit = 1024L*1024L;
      	pszUnit = "MB";
      } else if (filelen > (100*1024L)) {
      	lUnit = 1024L;
      	pszUnit = "KB";
      }
    }

    for (offset = 0; offset < filelen; offset += tocopy) {
      off_t remainder = filelen - offset;
      tocopy = (size_t)min(BUFFERSIZE, remainder);
      
      if (iProgress) {
      	int pc = (int)((offset * 100) / filelen);
      	iWidth = printf("%3d%% (%"PRIuMAX"%s/%"PRIuMAX"%s)\r", pc, (uintmax_t)(offset/lUnit), pszUnit, (uintmax_t)(filelen/lUnit), pszUnit);
      }
      
      XDEBUG_PRINTF(("fread(%p, %"PRIuPTR", 1, %p);\n", buffer, tocopy, pfs));
      if (!fread(buffer, tocopy, 1, pfs)) {
	if (iProgress && iWidth) printf("\n");
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(1, ("Can't read the input file. Deleted the partial copy.\n"));
      }
      if (!fwrite(buffer, tocopy, 1, pfd)) {
	if (iProgress && iWidth) printf("\n");
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(2, ("Can't write the output file. Deleted the partial copy.\n"));
      }
    }
    if (iProgress && iWidth) printf("%*s\r", iWidth, "");

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

int copy(char *name1, char *name2) {
  int e;
  char path[PATHNAME_SIZE];

  strsfp(name2, path, NULL);
  if (!exists(path)) {
    e = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (e) {
      printError("Error: Failed to create directory \"%s\". %s", path, strerror(errno));
      return e;
    }
  }

  e = copyf(name1, name2);
#if NEEDED
  switch (e) {
    case 0:
      break;
    case 1:
      printError("Error reading from file \"%s\"", name1);
      break;
    case 2:
      printError("Error writing to file \"%s\"", name2);
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

int is_directory(char *name){ /* Is name a directory? -> TRUE/FALSE */
  int result;
  int err;
  struct stat sstat;

  DEBUG_ENTER(("is_directory(\"%s\");\n", name));

  if (strchr(name, '?') || strchr(name, '*')) { /* Wild cards not allowed */
    RETURN_CONST_COMMENT(FALSE, ("%s is not a directory\n", name));
  }

  err = lstat(name, &sstat); // Use lstat, as stat does not detect SYMLINKDs.
  result = ((err == 0) && (S_ISDIR(sstat.st_mode)));
  RETURN_BOOL_COMMENT(result, ("%s %s a directory\n", name, result ? "is" : "is not"));
}

int is_effective_directory(char *name){ /* Is name a directory, or a link to a directory */
  int result;
  int err;
  struct stat sstat;

  DEBUG_ENTER(("is_effective_directory(\"%s\");\n", name));

  err = stat(name, &sstat); // Use stat, as lstat sees SYMLINKDs as links.
  result = ((err == 0) && (S_ISDIR(sstat.st_mode)));
  RETURN_BOOL_COMMENT(result, ("%s %s a directory\n", name, result ? "is" : "is not"));
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    mkdirp						      |
|									      |
|   Description     Create a directory, and all parent directories as needed. |
|									      |
|   Parameters      Same as mkdir					      |
|									      |
|   Returns	    Same as mkdir					      |
|									      |
|   Notes	    Same as mkdir -p					      |
|		    							      |
|   History								      |
|    1990s      JFL Created this routine in update.c			      |
|    2017-10-04 JFL Improved the error handling, stopping at the first error. |
|		    Avoid testing access repeatedly if we know it'll fail.    |
|    2017-10-06 JFL Added the iVerbose arguement.			      |
|    2018-05-31 JFL Bug fix: This worked, but returned an error, if the path  |
|		     contained a trailing [back]slash.			      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif

/* Create one directory */
int mkdir1(const char *pszPath, mode_t pszMode) {
#ifndef HAS_MSVCLIBX
  DEBUG_PRINTF(("mkdir(\"%s\", 0x%X);\n", pszPath, pszMode));
#endif
  return mkdir(pszPath, pszMode);
}

/* Create all parent directories */
int mkdirp(const char *pszPath0, mode_t pszMode) {
  char *pszPath = strdup(pszPath0);
  int iErr = 0; /* Assume success */
  int iSkipTest = FALSE;
  DEBUG_ENTER(("mkdirp(\"%s\", 0x%X);\n", pszPath, pszMode));
  if (pszPath) {
    char c;
    char *pc = pszPath;
    if (pc[0] && (pc[1] == ':') && (pc[2] == DIRSEPARATOR_CHAR)) pc += 2; /* Skip the drive if absolute path */
    for (c = pc[0]; c; ) { /* Repeat for all components in the path */
      while (*pc == DIRSEPARATOR_CHAR) pc++; ; /* Skip leading slashes if absolute path */
      while (*pc && (*pc != DIRSEPARATOR_CHAR)) pc++; /* Skip the file or dir name */
      c = *pc; /* Either NUL or / or \ */
      *pc = '\0'; /* Trim pszPath */
      if (iSkipTest || !is_effective_directory(pszPath)) { /* If the intermediate path does not exist */
	iErr = mkdir1(pszPath, pszMode); /* Then create it. */
	if (iErr) break; /* No need to go further if this failed */
	iSkipTest = TRUE; /* We know future existence tests will fail */
      }
      *pc = c; /* Restore pszPath */
      if (c && !pc[1]) break; /* This was the trailing [back]slash */
    }
  }
  free(pszPath);
  RETURN_INT_COMMENT(iErr, (iErr ? "Failed. errno=%d - %s\n" : "Success\n", errno, strerror(errno)));
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

/* 2020-03-11 JFL Fixed ':' issue in Unix, and improved performance */
void strmfp(char *pathname, const char *path, const char *name)   /* Make file pathname */
    {
    size_t l;
    DEBUG_ENTER(("strmfp(%p, \"%s\", \"%s\");\n", pathname, path, name));

    strcpy(pathname, path);
    l = strlen(path);
    if (   (l > 0)
	&& (path[l-1] != DIRSEPARATOR_CHAR)
#if DIRSEPARATOR_CHAR == '\\'
	&& (path[l-1] != ':')
#endif
       ) pathname[l++] = DIRSEPARATOR_CHAR;
    strcpy(pathname+l, name);
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
|    1996-10-14 JFL Made a clean, application-independent, version.	      |
|    2011-05-12 JFL Rewrote in an OS-independent way.			      |
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
  DEBUG_PRINTF(("copydate(\"%s\", \"%s\")\n", pszToFile, pszFromFile));
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

#ifdef _UNIX

#define _tell(hFile) lseek(hFile, 0, SEEK_CUR);

off_t _filelength(int hFile) {
  off_t curpos = _tell(hFile);			/* Save the current position */
  off_t length = lseek(hFile, 0, SEEK_END);	/* Move to the end of the file */
  lseek(hFile, curpos, SEEK_SET);		/* Return to the initial position */
  return length;
}

#endif /* defined(_UNIX) */

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
    /* If absPath is NULL, then _fullpath() allocated a new buffer with malloc() */
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

#ifdef _UNIX

/* Reimplementation of Microsoft's _fullpath() for Unix, as realpath() resolves links, which we don't want */
/* References:
   https://stackoverflow.com/a/31889126/2215591
   http://svn.python.org/projects/python/trunk/Lib/posixpath.py
*/
char *fullpath(char *absPath, const char *relPath, size_t lBuf) {
  char *pszBuf = absPath;
  size_t l = strlen(relPath);
  size_t lCD = 0;
  int nSlash = 0;
  char *pszIn;
  char *pszOut;
  char *pszOutMin;

  DEBUG_PRINTF(("fullpath(\"%s\", \"%s\", %lu);\n", absPath, relPath, lBuf));

  // If no buffer was provided, allocate one
  if (!pszBuf) {
    lBuf = PATH_MAX;
    pszBuf = malloc(lBuf);
    if (!pszBuf) goto fail;
  }
  // Make sure the output path is absolute
  if (relPath[0] != '/') {	// It's a relative path
    while (!getcwd(pszBuf, lBuf)) {
      if ((errno == ERANGE) && !absPath) {	// The buffer we allocated was too small
	pszBuf = realloc(pszBuf, lBuf *= 2);	// Double its size
	if (pszBuf) continue;
      }
      goto fail;	// Buffer too small, or not enough memory
    }
    lCD = strlen(pszBuf);
    if (!absPath) pszBuf = realloc(pszBuf, lCD+1+l); // May be smaller or bigger
    if (!pszBuf) goto fail;
    if (pszBuf[lCD-1] != '/') {
      if ((lCD+1) >= lBuf) goto fail;
      pszBuf[lCD++] = '/';
    }
  }
  if ((lCD+l) >= lBuf) goto fail;
  strcpy(pszBuf+lCD, relPath);
  // Skip initial slashes. 1=Local path; 2=UNC network path; >2=Local path
  nSlash = 1;	  // At this stage, we're sure that pszBuf[0] == '/'
  if (pszBuf[1] == '/') {
    if (pszBuf[2] != '/') {
      nSlash = 2;
    }
  }
  pszIn = pszOut = pszBuf + nSlash;
  while (*pszIn == '/') pszIn++;
  // Record the end of the root path, beyond which we cannot backtrack
  if (nSlash == 2) {
    while (*pszIn && *pszIn != '/') *(pszOut++) = *(pszIn++);	// Copy the server name
    if (*pszIn == '/') *(pszOut++) = *(pszIn++);		// Copy the following slash, if any
    while (*pszIn == '/') pszIn++;				// Skip any other slashes
    while (*pszIn && *pszIn != '/') *(pszOut++) = *(pszIn++);	// Copy the share name
    if (*pszIn == '/') *(pszOut++) = *(pszIn++);		// Copy the following slash, if any
    while (*pszIn == '/') pszIn++;				// Skip any other slashes
  }
  pszOutMin = pszOut;
  // Scan the remaining path
  while (*pszIn) {
    // char *pc = pszOut;
    // printf("pszBuf = '%.*s'; pszIn = '%s'\n", (int)(pszOut-pszBuf), pszBuf, pszIn); 
    // Remove single and double dots
    if (pszIn[0] == '.') {
      if ((pszIn[1] == '/') || !pszIn[1])  {			// It's a '.'
      	// printf("Handling './'\n");
	pszIn++;						// Skip the '.'
	while (*pszIn == '/') pszIn++;				// Skip the slashes
      } else if ((pszIn[1] == '.') && ((pszIn[2] == '/') || !pszIn[2])) {	// It's a '..'
      	// printf("Handling '../'\n");
	pszIn += 2;						// Skip the '..'
	while (*pszIn == '/') pszIn++;				// Skip the slashes
	if (pszOut > pszOutMin) pszOut--;				// Remove the last slash
	while ((pszOut > pszOutMin) && (pszOut[-1] != '/')) pszOut--;	// Remove the last directory name
      } else {	// Anything else beginning with a . is a normal file or directory name
      	goto copy_node_name;
      }
    } else {
copy_node_name:	// Normal file or directory name
      while (*pszIn && *pszIn != '/') *(pszOut++) = *(pszIn++);	// Copy the name
      if (*pszIn == '/') *(pszOut++) = *(pszIn++);		// Copy the following slash, if any
      while (*pszIn == '/') pszIn++;				// Skip any other slashes
      // printf("Handling '%.*s'\n", (int)(pszOut-pc), pc);
    }
  }
  // Remove the trailing / if there was not one in the input path
  if ((pszOut[-1] == '/') && (pszIn[-1] != '/')) {
    if ((pszOut > pszOutMin) || (nSlash == 2)) pszOut--;
  }
  *pszOut = '\0';	
  return pszBuf;
fail:
  if (pszBuf && !absPath) free(pszBuf);
  return NULL;
}

#endif /* _UNIX */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProgramNames					      |
|									      |
|   Description     Extract the program names from argv[0]		      |
|									      |
|   Parameters      char *argv[0]					      |
|									      |
|   Returns	    0							      |
|									      |
|   Notes	    Sets global variables program and progcmd.		      |
|		    Designed to work independently of MsvcLibX.		      |
|		    							      |
|   History								      |
|    2018-03-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    printError						      |
|									      |
|   Description     Print error messages with a consistent format	      |
|									      |
|   Parameters      char *pszFormat					      |
|		    ...							      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    Uses global variables program and progcmd,		      |
|		    set by GetProgramNames().				      |
|		    							      |
|   History								      |
|    2018-05-31 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int printError(char *pszFormat, ...) {
  va_list vl;
  int n;

  n = fprintf(stderr, "%s: ", program);
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, ".\n");
  va_end(vl);

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    NewPathName						      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
*									      *
\*---------------------------------------------------------------------------*/

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  return buf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zapDir						      |
|									      |
|   Description     Remove a directory, and all its files and subdirectories. |
|									      |
|   Parameters      const char *path		The directory pathname	      |
|		    int iFlags			Verbose & NoExec flags	      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2018-05-31 JFL Changed the iFlags argument to zapOpts *pzo.	      |
|		    The FLAG_FORCE flag now deletes read-only files.	      |
|		    Split zapFile() off of zapDir().			      |
|		    Added zapXxxM routines, with an additional iMode argument,|
|		     to avoid unnecessary slow calls to lstat() in Windows.   |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4459) /* Ignore the "declaration of 'VARIABLE' hides global declaration" warning */
#endif

int zapFileM(const char *path, int iMode, zapOpts *pzo) {
  int iFlags = pzo->iFlags;
  char *pszSuffix = "";
  int iErr = 0;

  DEBUG_ENTER(("zapFileM(\"%s\", 0x%04X);\n", path, iMode));

  if (S_ISDIR(iMode)) {
    errno = EISDIR;
    RETURN_INT(1);
  }
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(iMode)) {
    pszSuffix = ">";
  }
#endif

  if (iFlags & FLAG_COMMAND) {
    printf(DEL_FILE " \"%s\"\n", path);
  } else if (iFlags & FLAG_VERBOSE) {
    printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  }
  if (iFlags & FLAG_NOEXEC) RETURN_INT(0);
  if (iFlags & FLAG_FORCE) {
    if (!(iMode & S_IWRITE)) {
      iMode |= S_IWRITE;
      DEBUG_PRINTF(("chmod(%p, 0x%X);\n", path, iMode));
      iErr = -chmod(path, iMode); /* Try making the target file writable */
      DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
    }
    if (iErr) RETURN_INT(iErr);
  }
  iErr = -unlink(path); /* If error, iErr = 1 = # of errors */

  RETURN_INT(iErr);
}

int zapFile(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapFile(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) RETURN_INT(1);
  
  iErr = zapFileM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

int zapDirM(const char *path, int iMode, zapOpts *pzo) {
  char *pPath;
  int iErr;
  struct stat sStat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFlags = pzo->iFlags;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix;

  DEBUG_ENTER(("zapDirM(\"%s\", 0x%04X);\n", path, iMode));

  if (!S_ISDIR(iMode)) {
    errno = ENOTDIR;
    RETURN_INT(1);
  }

  pDir = opendirx(path);
  if (!pDir) RETURN_INT(1);
  while ((pDE = readdirx(pDir)) != NULL) {
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    pPath = NewPathName(path, pDE->d_name);
    pszSuffix = "";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = -lstat(pPath, &sStat); /* If error, iErr = 1 = # of errors */
#endif
    if (!iErr) switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	iErr = zapDirM(pPath, sStat.st_mode, pzo);
      	pszSuffix = DIRSEPARATOR_STRING;
      	break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      case DT_LNK:
      	pszSuffix = ">";
      	/* Fall through into the DT_REG case */
#endif
      case DT_REG:
	iErr = zapFileM(pPath, sStat.st_mode, pzo);
      	break;
      default:
      	iErr = 1;		/* We don't support deleting there */
#if defined(ENOSYS)
      	errno = ENOSYS;		/* Function not supported */
#else
      	errno = EPERM;		/* Operation not permitted */
#endif
      	pszSuffix = "?";
      	break;
    }
    if (iErr) {
      if (pDE->d_type != DT_DIR) printError("Error deleting \"%s%s\": %s", pPath, pszSuffix, strerror(errno));
      nErr += iErr;
      /* Continue the directory scan, looking for other files to delete */
    }
    free(pPath);
  }
  closedirx(pDir);

  iErr = 0;
  pszSuffix = DIRSEPARATOR_STRING;
  if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
  if (iFlags & FLAG_COMMAND) {
    printf(DEL_FILE " \"%s\"\n", path);
  } else if (iFlags & FLAG_VERBOSE){
    printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  }
  if (!iNoExec) iErr = rmdir(path);
  if (iErr) {
    printError("Error deleting \"%s%s\": %s", path, pszSuffix, strerror(errno));
    nErr += 1;
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

int zapDir(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapDir(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) {
    printError("Error: Can't stat \"%s\": %s", path, strerror(errno));
    RETURN_INT(1);
  }
  
  iErr = zapDirM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

#ifdef _MSC_VER
#pragma warning(default:4459)
#endif

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
*        1995-06-12 JFL Made this routine generic (Independent of DIRC)       *
*        2014-01-21 JFL Use a much larger buffer for 32-bits apps, to improve *
*                       performance.                                          *
*                                                                             *
******************************************************************************/

#ifdef _MSDOS		/* If it's a 16-bits app, use a 4K buffer. */
#define FBUFSIZE 4096
#else			/* Else for 32-bits or 64-bits apps, use a 256K buffer */
#define FBUFSIZE (256 * 1024)
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
    if ((!pbuf1) || (!pbuf2)) {
      fprintf(stderr, "Error: Not enough memory.\n");
      do_exit(1);
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

