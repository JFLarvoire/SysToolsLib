/*****************************************************************************\
*                                                                             *
*   Filename:	    redo.c						      *
*									      *
*   Description:    Run a command recursively in the current directory	      *
*		     and all subdirectories				      *
*									      *
*   Notes:	    The DOS version must be compiled using the large memory   *
*		    model, and linked with at least a 16 KB stack.            *
*									      *
*   History:								      *
*    1986-09-03 JFL jf.larvoire@hp.com created this program.		      *
*    1987-05-12 JFL Corrected bug with directories which have a name with     *
*		     have a name with an extension.			      *
*    1989-10-12 JFL Simplified the recursion algorithm. 		      *
*    1994-05-26 JFL Entirely rewritten based on code from DIRC.C to work      *
*		     in dual mode under both DOS and OS/2.		      *
*		    Version A.03.00.					      *
*    1995-09-25 JFL Added support for Win32.				      *
*		    Changed %. to be a relative path.			      *
*    1996-07-18 JFL finalized support for Win32.			      *
*		    Version 2.0.					      *
*    2003-06-16 JFL Added -from option. Renamed the -b option as -q.	      *
*		    Version 2.1.					      *
*    2003-06-23 JFL Fixed a minor bug with -b to -q renaming.		      *
*		    Version 2.1a.					      *
*    2012-10-01 JFL Added support for a Win64 version.			      *
*		    Added new internal commands up to Windows 7.	      *
*		    Version 2.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.2.1.                 *
*    2014-03-26 JFL Rewriten using the standard directory access functions.   *
*                   Added a Linux version.		                      *
*                   Changed the path replacement sequence from "%." to "{}".  *
*		    Version 3.0.					      *
*    2014-12-03 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 3.0.1.					      *
*    2016-01-08 JFL Fixed all warnings in Linux. Version 3.0.2.		      *
*    2016-09-20 JFL Bug fix: The Win32 version did not process empty args.    *
*		    Version 3.0.3.  					      *
*    2018-04-29 JFL Make sure WIN32 pathname buffers are larger than 260 B.   *
*		    Document WIN32 redo limitations in help.		      *
*    2018-04-30 JFL Check errors for all calls to chdir() and getcwd().	      *
*                   Make sure all aborts display consistent error messages.   *
*		    Rewrote finis() so that it displays errors internally.    *
*		    Version 3.1.  					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.3.1.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 3.1.2.      *
*    2020-03-17 JFL Fixed issue with Unix readdir() not always setting d_type.*
*                   Version 3.1.3.					      *
*    2020-04-20 JFL Added support for MacOS. Version 3.2.                     *
*    2023-11-16 JFL Bugfix in the debug version: Buffer used after free().    *
*                   Version 3.2.1.                                            *
*    2025-08-10 JFL Fixed a build error. No functional change. Version 3.2.2. *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Execute a command recursively"
#define PROGRAM_NAME    "redo"
#define PROGRAM_VERSION "3.2.2"
#define PROGRAM_DATE    "2025-08-10"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <limits.h>
/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
#include "syslib.h"		/* Force linking with SysLib dictionary routines */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/* These are non standard routines, but the leading _ is annoying */ 
#define strlwr _strlwr
#define stricmp _stricmp
#define spawnvp _spawnvp

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#define CDECL _cdecl

/* These are non standard routines, but the leading _ is annoying */ 
#define strdup _strdup

#include <process.h>

#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2		/* To be defined on the command line for the OS/2 version */

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h" 


#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*.*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#define CDECL

/* These are non standard routines, but the leading _ is annoying */ 
#define strdup _strdup

#include <process.h>

#endif /* _OS2 */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32			/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR '\\'
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES TRUE

#define CDECL

/* These are non standard routines, but the leading _ is annoying */ 
#define strdup _strdup

#include <process.h>

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define DIRSEPARATOR '/'
#define PATTERN_ALL "*"     		/* Pattern matching all files */
#define HAS_DRIVES FALSE

#include <ctype.h>

static char *strlwr(char *pString) {
  char c;
  char *pc = pString;
  for ( ; (c=*pc); pc++) *pc = tolower(c);
  return pString;
}

#undef stricmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp

#ifndef FNM_MATCH
#define FNM_MATCH	0	/* Simpler than testing != FNM_NO_MATCH */ 
#endif

/* DOS File attribute constants */

#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */

#define _getdrive() ':'
#define _chdrive(d) 0

#define CDECL

#define max(x,y) (((x)>(y))?(x):(y))

#define _flushall() do {fflush(stdout); fflush(stderr);} while (0)

/* Unix port of Microsoft's spawn* functions */
intptr_t spawnvp(int iMode, const char *pszCommand, char *const *argv);
#define P_WAIT         0	/* Spawn mode: Wait for program termination */
#define P_NOWAIT       1	/* Spawn mode: Do not wait for program termination */

#endif /* __unix__ */

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

#undef FALSE
#undef TRUE

typedef enum {
  FALSE = 0,
  TRUE = 1
} bool;

#define RETCODE_SUCCESS 0	    /* Return codes processed by finis() */
#define RETCODE_NO_FILE 1	    /* Note: Value 1 required by HP Preload -env option */
#define RETCODE_TOO_MANY_FILES 2    /* Note: Value 2 required by HP Preload -env option */
#define RETCODE_NO_MEMORY 3
#define RETCODE_INACCESSIBLE 4
#define RETCODE_EXEC_ERROR 5

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

#define OFFSET_OF(pointer) ((ushort)(ulong)(void far *)pointer)
#define SEGMENT_OF(pointer) ((ushort)(((ulong)(void far *)pointer) >> 16))

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

typedef struct fif {	    /* OS-independant FInd File structure */
    char *name; 		/* File node name, ending with a NUL */
    struct stat st;		/* File size, time, etc */
    struct fif *next;
} fif;

/* Global variables */

#if HAS_DRIVES
int iInitDrive;                     /* Initial drive. 1=A, 2=B, 3=C, etc. */
#endif
char szInitDir[PATHNAME_SIZE];      /* Initial directory on the work drive */
char szStartDir[PATHNAME_SIZE];	    /* Directory where recursion starts */
int iVerbose = FALSE;		    /* If TRUE, echo commands executed */
int iRelat;			    /* Index of a pathname relative to szInitDir */

fif *firstfif = NULL;		    /* Pointer to the first allocated fif structure */

#ifdef _MSDOS
#define MAXARGS 25
#else
#define MAXARGS 1024
#endif
char *command[MAXARGS]; 	/* Command and argument list main copy */

#if defined(_MSDOS) || defined(_WIN32) || defined(_OS2)

char *internes[] = {            /* Internal commands */
    "assoc",			// NT 3.51
    "break",
    "call",
    "cd",
    "chcp",
    "chdir",
    "cls",
    "color",			// 2000?
    "copy",
    "ctty",
    "date",
    "del",
    "dir",
    "dpath",			// NT 3.51
    "echo",
    "endlocal", 		// NT 3.51
    "erase",
    "exit",
    "for",
    "goto",
    "if",
    "keys",
    "lfnfor",			// Win95
    "lh",			// DOS 5
    "loadhigh", 		// DOS 5
    "lock",			// Win95
    "md",
    "mkdir",
    "mklink",			// Windows 7
    "move",
    "path",
    "pathext",			// 2000?
    "pause",
    "popd",			// NT 3.51
    "prompt",
    "pushd",			// NT 3.51
    "rd",
    "rem",
    "ren",
    "rename",
    "rmdir",
    "set",
    "setlocal", 		// NT 3.51
    "shift",
    "start",			// NT 3.51
    "time",
    "title",			// NT 3.51
    "truename",
    "type",
    "unlock",			// Win95
    "ver",
    "verify",
    "vol",
};

#elif defined(_UNIX)

char *internes[] = {            /* Internal commands. (Only those not duplicated in /bin) */
    "alias",
    "bg",
    "bind",
    "builtin",
    "case",
    "cd",
    "chdir",
    "command",
    "declare",
    "dirs",
    "disown",
    "enable",
    "eval",
    "exec",
    "exit",
    "export",
    "fc",
    "for",
    "getopts",
    "hash",
    "help",
    "history",
    "if",
    "jobs",
    "let",
    "local",
    "popd",
    "pushd",
    "read",
    "readonly",
    "set",
    "shift",
    "shopt",
    "source",
    "suspend",
    "test",
    "time",
    "times",
    "trap",
    "type",
    "typeset",
    "ulimit",
    "unalias",
    "unset",
    "until",
    "wait",
    "while",
};

#endif

/* Forward references */

bool interne(char *);		    /* Is a command internal? */
void finis(int retcode, ...);	    /* Return to the initial drive & exit */
void usage(int iErr);               /* Display a brief help and exit */

int descend(char *from, int fif0);  /* Recurse the directory tree */
int lis(char *, char *, int, ushort); /* Scan a directory */
int CDECL cmpfif(const fif **ppfif1, const fif **ppfif2); /* Compare 2 names */
void trie(fif **ppfif, int nfif);   /* Sort file names */
fif **AllocFifArray(size_t nfif);   /* Allocate an array of fif pointers */
void FreeFifArray(fif **fiflist);

int makepathname(char *, char *, char *);

char *getdir(char *, int);          /* Get the current drive directory */

/******************************************************************************
*                                                                             *
*	Function:	main						      *
*                                                                             *
*	Description:	Main procedure					      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	  int argc	Number of command line arguments, including program.  *
*	  char *argv[]	Array of pointers to the arguments. argv[0]=program.  *
*                                                                             *
*       Return value:   0=Success; !0=Failure                                 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	 1986-09-03 JFL	Initial implementation				      *
*	 1994-05-26 JFL	Merged with main routine of DIRC.		      *
*                                                                             *
******************************************************************************/

int main(int argc, char *argv[]) {
  int arg1;   /* First meaningfull argument */
  int cmd1;	/* First offset in command array */
  int i;
  char *pszConclusion = "Redo done";
  char *pszFrom = NULL;
  int iErr;
  char *pcd;

  _flushall(); /* Make sure the above line went to a file if the output is
		  redirected */

  /* Parse the command line */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if ((arg[0] == '-') || (arg[0] == '/')) { /* It's a switch */
      char *option = arg+1;
				  /* Don't forget to add switches to...
				      ... the Recurse list below,
				      ... the Usage display further down.
				  */
      if (streq(option, "?")) {
	usage(0);
      }
      DEBUG_CODE(
	if (streq(option, "d")) {
	  DEBUG_ON();
	  continue;
	}
      )
      if (streq(option, "from")) {
	if ((i+1)<argc) {
	  pszFrom = argv[++i];
	} else {
	  usage(1);
	}
	continue;
      }
      if (streq(option, "q")) {
	iVerbose = FALSE;
	pszConclusion = NULL;
	continue;
      }
      if (streq(option, "v")) {
	iVerbose = TRUE;
	continue;
      }
      if (streq(option, "V")) {	    /* -V: Display the version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    break;  /* Ignore other arguments */
  }
  arg1 = i;

  if (argc <= arg1) {   /* If there is no command line, exit immediately */
    usage(1);
  }

  if (iVerbose) printf(PROGRAM_NAME_AND_VERSION "\n");

  /* Build the sub command line to execute recursively */

  cmd1 = 0;
  if (interne(argv[arg1])) {      /* Is it an internal command? */
#if defined(_MSDOS) || defined(_WIN32) || defined(_OS2)
    command[0] = getenv("COMSPEC"); /* If yes, run a secondary command  */
    command[1] = "/C";		/* processor.			    */
#elif defined(_UNIX)
    command[0] = getenv("SHELL");  /* If yes, run a secondary command  */
    command[1] = "-c";		/* processor.			    */
#endif
    cmd1 = 2;
  }
  for (i=0; (arg1+i)<argc; i++) {  /* Copy the arguments */
    if ((cmd1+i)==(MAXARGS-1)) {	/* If destination array full */
      printf("Warning: Arguments beyond the %dth ignored.\n", i);
      break;
    }
    command[cmd1+i] = argv[arg1+i];
  }
  command[cmd1+i] = NULL;

  /* Save the initial state, and change to the start directory. */

#if HAS_DRIVES
  iInitDrive = _getdrive();
  DEBUG_PRINTF(("Init drive = %c:\n", iInitDrive + '@'));
  if (pszFrom && pszFrom[0] && (pszFrom[1]==':')) {
    _chdrive(toupper(pszFrom[0])-'@');  // Change to the requested drive
    pszFrom += 2;			    // Skip the drive
  }
  DEBUG_PRINTF(("Work drive = %c:\n", _getdrive() + '@'));
#endif
  
  pcd = getdir(szInitDir, PATHNAME_SIZE);
  if (!pcd) {
    finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");
  }
  DEBUG_PRINTF(("Init directory = %s\n", szInitDir));
  if (!pszFrom) 
      pszFrom = szInitDir;
  iErr = chdir(pszFrom);
  if (iErr) {
    finis(RETCODE_INACCESSIBLE, "Cannot access directory \"%s\"", pszFrom);
  }
  pcd = getdir(szStartDir, PATHNAME_SIZE);	// Get the canonic name for the start directory.
  if (!pcd) {
    finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");
  }
  DEBUG_PRINTF(("Work directory = %s\n", szStartDir));
  iRelat = (int)strlen(szStartDir);
  if (iRelat > 1) iRelat += 1;	// If not the root, account for the
				      //  trailing backslash.
  /* Recurse */
  descend(szStartDir, 0);

  if (iVerbose) printf("%s\n", pszConclusion);
  finis(0);
  return 0;
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

void usage(int iErr) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: redo [SWITCHES] {COMMAND LINE}\n\
\n\
Switches:\n\
    -from {path}  Start recursion in the given directory.\n\
    -v	          Echo each path accessed, and the command executed.\n\
\n\
Command line:     Any valid command and arguments.\n\
                  The special sequence \"{}\" is replaced by the current\n\
                  directory name, relative to the initial directory.\n\
"
#ifdef _WIN32
"\n\
Known issue with long pathnames > 260 bytes: Windows versions up to 8 cannot\n\
change the current directory to such long pathnames. Windows 10 can, but only\n\
if it's been enabled in the registry.\n\
On all versions of Windows, and whether or not the Windows 10 registry fix has\n\
been enabled, redo can enumerate paths of any length, and sets the {} sequence\n\
correctly.\n\
If there's a chance that you might have paths longer than 260 bytes in the tree\n\
below your current directory, do not rely on the current directory set by redo,\n\
but use the {} sequence to generate commands with absolute paths arguments.\n\
And of course, use a command that is compatible with paths > 260 bytes.\n\
"
#endif
#include "footnote.h"
);
  exit(iErr);
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
      fprintf(stderr, "redo: Error: ");
      vfprintf(stderr, pszFormat, vl);
      fputs(".\n", stderr);
    }
    va_end(vl);
  }

  chdir(szInitDir);	/* Don't test errors, as we're likely to be here due to another error */
#if HAS_DRIVES
  _chdrive(iInitDrive);
#endif
  exit(retcode);
}

bool interne(char *com) {  /* Is com an internal command? */
  char nom[15];
  int i;

  strncpyz(nom, com, 15);
  strlwr(nom);

  for (i=0; i<(sizeof(internes)/sizeof(char *)); i++)
    if (strcmp(nom, internes[i]) == 0) return(TRUE);

  return(FALSE);
}

/******************************************************************************
*                                                                             *
*       Function:       descend                                               *
*                                                                             *
*       Description:    Go down the directory trees recursively               *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *from    First directory to list.                              *
*         int fif0      Index of the first free file structure                *
*                                                                             *
*       Return value:   0=Success; !0=Failure                                 *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*        1993-10-15 JFL  Initial implementation                               *
*        1994-03-17 JFL  Rewritten to recurse within the same appli. instance.*
*	 1994-05-27 JFL	Updated for REDO.				      *
*                                                                             *
******************************************************************************/

int descend(char *from, int fif0) {
  int fif1;
  int nfif;
  int i;
  fif **ppfif;

  /* Get all subdirectories */
  DEBUG_ENTER(("descend(\"%s\", %d);\n", from, fif0));
  fif1 = lis(from, PATTERN_ALL, fif0, 0x8016);
  nfif = fif1 - fif0;
  ppfif = AllocFifArray(nfif);
  trie(ppfif, nfif);

  for (i=0; i<nfif; i++) {
    NEW_PATHNAME_BUF(name1);

#if PATHNAME_BUFS_IN_HEAP
    if (!name1) break;
#endif

    makepathname(name1, from, ppfif[i]->name);
    TRIM_PATHNAME_BUF(name1);

    descend(name1, fif1);

    FREE_PATHNAME_BUF(name1);
  }

  FreeFifArray(ppfif);

  RETURN_INT(0);
}

/******************************************************************************
*                                                                             *
*	Function:	DoPerPath					      *
*                                                                             *
*	Description:	Routine to be executed once for each subdirectory     *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	Return value:	None						      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*	History:							      *
*	 1994-05-27 JFL	Updated for REDO.				      *
*                                                                             *
******************************************************************************/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

void DoPerPath(void) {
  int err;
  int i;
  NEW_PATHNAME_BUF(path);
  char *pc;
  char *command2[MAXARGS+1];	/* Command and argument list secondary copy */
  int iDem;
  char *ppath;

  DEBUG_ENTER(("DoPerPath();\n"));

#if PATHNAME_BUFS_IN_HEAP
  if (!path) finis(RETCODE_NO_MEMORY, "Not enough memory for execution");
#endif

  pc = getcwd(path, PATHNAME_SIZE);
  if (!pc) finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");
  ppath = path;
#if defined(_MSDOS) || defined(_WIN32) || defined(_OS2)
  ppath += 2;	/* Skip the drive letter */
#endif
  iDem = streq(szStartDir, ppath);
  ppath += iRelat - iDem;

  for (i=0; i<MAXARGS; i++) {
    char *pc2;
    char c;

    pc = command[i];
    if (!pc) break;
    // ~~jfl 1995-09-25 Removed: if (streq(pc, "%.")) command2[i] = path;
    //		    Instead expand string with RELATIVE path
    pc2 = malloc(max(PATHNAME_SIZE, 128));
    if (!pc2) finis(RETCODE_NO_MEMORY, "Not enough memory for command");
    command2[i] = pc2;
    do {
      c = *(pc++);
      *(pc2++) = c;
      if (   ((c == '%') && (*pc == '.'))    /* Initial redo-specific tag */
	  || ((c == '{') && (*pc == '}'))) { /* New find-specific tag */
	pc2 -= 1;   /* Back up over the % sign */
	/* Copy the part of the path relative to the initial path */
	if (!iDem)
	    strcpy(pc2, ppath);
	else	    /* ~~jfl 1996-07-19 */
	    strcpy(pc2, ".");
	pc2 += strlen(pc2);	// Move to the end of string
	pc += 1;		// Skip the period
      }
    } while (c);
    // Free the end of string
    command2[i] = realloc(command2[i], ++pc2 - command2[i]);
    DEBUG_PRINTF(("arg[%d] = \"%s\";\n", i, command2[i]));
  }
  command2[i] = NULL;

  if (iVerbose) {
    printf("[%s]", path);
    for (i=0; (pc=command2[i]); i++) printf("%s ", pc);
    printf("\n");
  }
  err = (int)spawnvp(P_WAIT, command2[0], command2);
  if (err == -1) finis(RETCODE_EXEC_ERROR, "Cannot execute the command");
  if (err) printf("\nredo: %s returns error # %d.\n", command2[0], err);

  for (i=0; command2[i]; i++) free(command2[i]); // Free the copy of the command.

  FREE_PATHNAME_BUF(path);
  RETURN();
}

#ifdef _MSC_VER
#pragma warning(default:4706) /* Restore the "assignment within conditional expression" warning */
#endif

/******************************************************************************
*                                                                             *
*       Function:       lis                                                   *
*                                                                             *
*       Description:    Scan the directory, and fill the fif table            *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         char *startdir Directory to scan. If "NUL", don't scan.             *
*         char *pattern Wildcard pattern.                                     *
*         int nfif      Number of files/directories found so far.             *
*         int attrib    Bit 15: List directories exclusively.                 *
*                       Bits 7-0: File/directory attribute.                   *
*                                                                             *
*       Return value:   Total number of files/directories in fif array.       *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	 1994-05-27 JFL	Updated for REDO.				      *
*                                                                             *
******************************************************************************/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int lis(char *startdir, char *pattern, int nfif, ushort attrib) {
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

  DEBUG_ENTER(("lis(\"%s\", \"%s\", %d, 0x%X);\n", startdir, pattern, nfif, attrib));

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
    if (err) finis(RETCODE_INACCESSIBLE, "Cannot access drive %c\n:", startdir[0]);
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
      fprintf(stderr, "redo: Error: Cannot access directory %s.\n", path);
      FREE_PATHNAME_BUF(initdir);
      DEBUG_PRINTF(("// Cannot access directory %s\n", path));
      FREE_PATHNAME_BUF(path);
      FREE_PATHNAME_BUF(pathname);
      RETURN_INT(nfif);
    }
  }

  pcd = getcwd(path, PATHNAME_SIZE);
  if (!pcd) finis(RETCODE_INACCESSIBLE, "Cannot get the current directory");

  /* Execute the routine once for this path */
  DoPerPath();

  /* start looking for all files */
  pDir = opendirx(path);
  if (pDir) {
    while (pDir && (pDirent = readdirx(pDir))) { /* readdirx() ensures d_type is set */
      struct stat st;
      DEBUG_CODE(
	char *reason;
      )

      makepathname(pathname, path, pDirent->d_name);
#if !_DIRENT2STAT_DEFINED
      lstat(pathname, &st);
#else
      dirent2stat(pDirent, &st);
#endif
      DEBUG_PRINTF(("// Found %10s %12s\n",
	    (pDirent->d_type == DT_DIR) ? "Directory" :
	    (pDirent->d_type == DT_LNK) ? "Link" :
	    (pDirent->d_type == DT_REG) ? "File" :
	    "Other",
	    pDirent->d_name));
      DEBUG_CODE(reason = "it's .";)
      if (    !streq(pDirent->d_name, ".")  /* skip . and .. */
	      DEBUG_CODE(&& (reason = "it's .."))
	   && !streq(pDirent->d_name, "..")
	      DEBUG_CODE(&& (reason = "it's not a directory"))
	   && (   !(attrib & 0x8000)	  /* Skip files if dirs only */
		|| (pDirent->d_type == DT_DIR))
	      DEBUG_CODE(&& (reason = "it's a directory"))
	   && (   (attrib & _A_SUBDIR)	  /* Skip dirs if files only */
		|| (pDirent->d_type != DT_DIR))
	      DEBUG_CODE(&& (reason = "the pattern does not match"))
	   && (fnmatch(pattern2, pDirent->d_name, FNM_CASEFOLD) == FNM_MATCH)
	 ) {
	fif *pfif;

	DEBUG_PRINTF(("// OK\n"));
	pfif = (fif *)malloc(sizeof(fif));
	pname = strdup(pDirent->d_name);
	if (!pfif || !pname) {
	  closedirx(pDir);
	  finis(RETCODE_NO_MEMORY, "Out of memory for directory access");
	}
	pfif->name = pname;
	pfif->st = st;
#if _MSVCLIBX_STAT_DEFINED
	DEBUG_PRINTF(("st.st_Win32Attrs = 0x%08X\n", pfif->st.st_Win32Attrs));
	DEBUG_PRINTF(("st.st_ReparseTag = 0x%08X\n", pfif->st.st_ReparseTag));
#endif /* _MSVCLIBX_STAT_DEFINED */
	// Note: The pointer to the name is copied as well.
	pfif->next = firstfif;
	firstfif = pfif;
	nfif += 1;
      } else {
	DEBUG_PRINTF(("// Ignored because %s\n", reason));
      }
    }

    closedirx(pDir);
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

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/******************************************************************************
*                                                                             *
*       Function:       cmpfif                                                *
*                                                                             *
*       Description:    Compare two files, for sorting the file list          *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*	  fif **ppfif1	First file structure				      *
*	  fif **ppfif2	Second file structure				      *
*                                                                             *
*       Return value:    0 : Equal                                            *
*                       <0 : file1<file2                                      *
*                       >0 : file1>file2                                      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	 1994-05-27 JFL	Updated for REDO.				      *
*                                                                             *
******************************************************************************/

int CDECL cmpfif(const fif **ppfif1, const fif **ppfif2) {
  int ret;

  /* List directories before files */
  ret = S_ISDIR((*ppfif2)->st.st_mode) - S_ISDIR((*ppfif1)->st.st_mode);
  if (ret) return ret;

  /* If both files, or both directories, list names alphabetically */
  ret = _strnicmp((*ppfif1)->name, (*ppfif2)->name, NODENAME_SIZE);
  if (ret) return ret;

  /* Finally do a case dependant comparison */
  return strncmp((*ppfif1)->name, (*ppfif2)->name, NODENAME_SIZE);
}

/******************************************************************************
*                                                                             *
*	Function:	trie						      *
*                                                                             *
*	Description:	Sort re two files, for sorting the file list	      *
*                                                                             *
*       Arguments:                                                            *
*                                                                             *
*         fif *fif1     First file structure                                  *
*         fif *fif2     Second file structure                                 *
*                                                                             *
*       Return value:    0 : Equal                                            *
*                       <0 : file1<file2                                      *
*                       >0 : file1>file2                                      *
*                                                                             *
*       Notes:                                                                *
*                                                                             *
*       Updates:                                                              *
*	 1994-05-27 JFL Updated for REDO.				      *
*                                                                             *
******************************************************************************/

typedef int (CDECL *pCompareProc)(const void *item1, const void *item2);

void trie(fif **ppfif, int nfif) {
  qsort(ppfif, nfif, sizeof(fif *), (pCompareProc)cmpfif);
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    spawnvp						      |
|									      |
|   Description:    Start an outside application			      |
|									      |
|   Parameters:     int iMode		Spawning mode. P_WAIT or P_NOWAIT     |
|		    char *pszCommand	Program to start		      |
|		    char **argv		List of arguments, terminated by NULL |
|									      |
|   Returns:	    The exit code (if P_WAIT) or the process ID (if P_NOWAIT) |
|									      |
|   Notes:	    Unix port of a Microsoft function			      |
|									      |
|   History:								      |
|    2014-03-27 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _UNIX

#include <sys/wait.h>

intptr_t spawnvp(int iMode, const char *pszCommand, char *const *argv) {
  pid_t pid = fork();
  int iRet = 0;
  DEBUG_CODE({
    int i;
    DEBUG_PRINTF(("%s", pszCommand));
    if (DEBUG_IS_ON()) {
      for (i=0; argv[i]; i++) printf(" %s", argv[i]);
      printf("\n");
    }
  })
  if (pid < 0) {		// Failed to fork
    return -1;
  } else if (pid == 0) {	// We're the child instance
    execvp(pszCommand, argv);
    // We only get here if the exec fails
    fprintf(stderr, "Error: Failed to run %s\n", pszCommand);
    exit(255);
  } else {			// We're the parent instance
    switch(iMode) {
      case P_WAIT:
      	wait(&iRet);
	break;
      case P_NOWAIT:
      default:
	iRet = (intptr_t)pid;
	break;
    }
  }
  return iRet;
}

#endif /* defined(_UNIX) */

