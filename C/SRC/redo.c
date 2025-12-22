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
*    2025-11-03 JFL Updated the help screen, and its note about limitations   *
*                   for long paths in Windows.                                *
*                   Added option -i as an alias to option -from.              *
*                   Added option -l.                                          *
*    2025-11-04 JFL Added option -m.					      *
*    2025-11-12 JFL Improved a couple of error messages.		      *
*		    Version 3.3.					      *
*    2025-12-03 JFL Rewritten using SysLib's WalkDirTree().		      *
*    2025-12-07 JFL Added options -c, -C, -f, -F, -X.			      *
*                   In verbose mode, display statistics in the end.	      *
*    2025-12-08 JFL Detect when a child process exits with Ctrl-C in Windows. *
*    2025-12-10 JFL Removed all unused remaining code for sorting files.      *
*    2025-12-16 JFL Removed the limitation of the number of arguments.        *
*    2025-12-18 JFL Minor fixes to the above changes.			      *
*    2025-12-20 JFL Use the new error message routines.                       *
*    2025-12-21 JFL Added options -o & -O, and default to mode Once.          *
*		    Version 4.0.					      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Execute a command recursively in all subdirectories"
#define PROGRAM_NAME    "redo"
#define PROGRAM_VERSION "4.0"
#define PROGRAM_DATE    "2025-12-21"

#include <config.h>	/* OS and compiler-specific definitions */

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/wait.h>
/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
#include "pathnames.h"		/* Pathname management functions */
#include "mainutil.h"
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/* These are non standard routines, but the leading _ is annoying */
#define strlwr _strlwr
#define spawnvp _spawnvp

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

/* These are non standard routines, but the leading _ is annoying */
#define strdup _strdup

#include <process.h>
#include <conio.h>	/* For _kbhit() */

#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */

#define NOOP_CMD "break"

#define COMMAND_LINE_MAX 128

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2		/* To be defined on the command line for the OS/2 version */

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h"

/* These are non standard routines, but the leading _ is annoying */
#define strdup _strdup

#include <process.h>

#define NOOP_CMD "break"

#define COMMAND_LINE_MAX 128

#endif /* _OS2 */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

/* These are non standard routines, but the leading _ is annoying */
#define strdup _strdup

#include <process.h>

#define NOOP_CMD "break"

#define COMMAND_LINE_MAX 32768

#endif /* _WIN32 */

/************************* Unix-specific definitions *************************/

#ifdef _UNIX		/* Defined in NMaker versions.h for Unix flavors we support */

#include <ctype.h>

static char *strlwr(char *pString) {
  char c;
  char *pc = pString;
  for ( ; (c=*pc); pc++) *pc = tolower(c);
  return pString;
}

#define _strnicmp strncasecmp

#ifndef FNM_MATCH
#define FNM_MATCH	0	/* Simpler than testing != FNM_NO_MATCH */
#endif

#define NOOP_CMD "true"

#define max(x,y) (((x)>(y))?(x):(y))

/* Unix port of Microsoft's spawn* functions */
intptr_t spawnvp(int iMode, const char *pszCommand, char *const *argv);
#define P_WAIT         0	/* Spawn mode: Wait for program termination */
#define P_NOWAIT       1	/* Spawn mode: Do not wait for program termination */

#define COMMAND_LINE_MAX 32768

#endif /* _UNIX */

/*********************************** Other ***********************************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(EXE_OS_NAME))
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

/********************** End of OS-specific definitions ***********************/

/* Local definitions */

#define RETCODE_SUCCESS 0	    /* Return codes processed by finis() */
#define RETCODE_NO_FILE 1	    /* Note: Value 1 required by HP Preload -env option */
#define RETCODE_TOO_MANY_FILES 2    /* Note: Value 2 required by HP Preload -env option */
#define RETCODE_ABORT 3		    /* MSVC default when raising any signal */
#define RETCODE_INACCESSIBLE 4
#define RETCODE_EXEC_ERROR 5
#define RETCODE_SYNTAX 6
#define RETCODE_NO_MEMORY 7

#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

/* Global variables */

#if HAS_DRIVES
int iInitDrive;                     /* Initial drive. 1=A, 2=B, 3=C, etc. */
#endif
char *pszInitDir;		    /* Initial directory on the work drive */
int iVerbose = FALSE;		    /* If TRUE, echo commands executed */
int iMeasure = 0;		    /* If > 0, measure the paths longer than that length */
int iNoExec = FALSE;		    /* If TRUE, echo the commands, but dont run them */
volatile int iCtrlC = FALSE;	    /* If TRUE, a Ctrl-C has been detected */

char **argvCmd = NULL;		    /* Child command and argument list main copy */
int argcCmd = 0;		    /* Number of items in the command array */

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
#if defined(_MSDOS)
    "truename",
#endif
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

int interne(char *);		    /* Is a command internal? */
void finis(int retcode, ...);	    /* Return to the initial drive & exit */
void OnControlC(int iSignal);
void usage(int iErr);               /* Display a brief help and exit */
void DoPerPath(const char *pszPath, void *pRef);

int redo(char *from, wdt_opts *);   /* Recurse the directory tree */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|                                                                             |
|   Description	    Main procedure					      |
|		    							      |
|   Arguments	                                                              |
|     int argc	    Number of command line arguments, including program.      |
|     char *argv[]  Array of pointers to the arguments. argv[0]=program.      |
|		    							      |
|   Return value    0=Success; !0=Failure                                     |
|		    							      |
|   Notes                                                                     |
|		    							      |
|   History                                                                   |
|    1986-09-03 JFL	Initial implementation				      |
|    1994-05-26 JFL	Merged with main routine of DIRC.		      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int arg1;	/* First meaningfull argument */
  int cmd1;	/* First offset in command array */
  int i;
  char *pszConclusion = "Redo done";
  char *pszFrom = NULL;
  wdt_opts wdtOpts = {0};

  wdtOpts.iFlags |= WDT_CD;	/* Change directories by default */
#if OS_HAS_LINKS
  wdtOpts.iFlags |= WDT_ONCE | WDT_FOLLOW;	/* Follow links by default */
#endif

  /* Parse the command line */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) { /* It's a switch */
      char *option = arg+1;

      if (streq(option, "?")) {
	usage(0);
      }
      if (streq(option, "c")) {
	wdtOpts.iFlags |= WDT_CD;
	continue;
      }
      if (streq(option, "C")) {
	wdtOpts.iFlags &= ~WDT_CD;
	continue;
      }
      DEBUG_CODE(
	if (streq(option, "d")) {
	  DEBUG_ON();
	  continue;
	}
      )
#if OS_HAS_LINKS
      if (streq(option, "f")) {
	wdtOpts.iFlags |= WDT_FOLLOW;
	continue;
      }
      if (streq(option, "F")) {
	wdtOpts.iFlags &= ~WDT_FOLLOW;
	continue;
      }
#endif
      if (streq(option, "i") || streq(option, "from")) {
	if ((i+1)<argc) {
	  pszFrom = argv[++i];
	} else {
	  usage(1);
	}
	continue;
      }
      if (streq(option, "l")) { /* List directory pathnames lengths */
	if (((i+1)<argc) && !IsSwitch(argv[i+1])) {
	  iMeasure = atoi(argv[++i]);
	} else {
	  iMeasure = 1;
	}
	argv[i] = NOOP_CMD; /* Dummy command that does nothing */
	break;
      }
      if (streq(option, "m")) { /* Maximum depth */
	if (((i+1)<argc) && !IsSwitch(argv[i+1])) {
	  wdtOpts.iMaxDepth = atoi(argv[++i]);
	} else {
	  finis(RETCODE_SYNTAX, "Max depth value missing");
	}
	continue;
      }
      if (streq(option, "o")) {
	wdtOpts.iFlags |= WDT_ONCE;
	continue;
      }
      if (streq(option, "O")) {
	wdtOpts.iFlags &= ~WDT_ONCE;
	continue;
      }
      if (streq(option, "q")) {
	iVerbose = FALSE;
	wdtOpts.iFlags |= WDT_QUIET;
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
      if (streq(option, "X")) {
	iNoExec = TRUE;
	continue;
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

  argcCmd = argc - arg1;
  argvCmd = (char **)malloc((argcCmd+3) * sizeof(char *));
  if (!argvCmd) finis(RETCODE_NO_MEMORY, "Not enough memory for command");

  cmd1 = 0;
  if (interne(argv[arg1])) {      /* Is it an internal command? */
#if defined(_MSDOS) || defined(_WIN32) || defined(_OS2)
    argvCmd[0] = getenv("COMSPEC"); /* If yes, run a secondary command  */
    argvCmd[1] = "/C";		/* processor.			    */
#elif defined(_UNIX)
    argvCmd[0] = getenv("SHELL");  /* If yes, run a secondary command  */
    argvCmd[1] = "-c";		/* processor.			    */
#endif
    cmd1 = 2;
    argcCmd += 2;
  }
  for (i=0; (arg1+i)<argc; i++) {  /* Copy the arguments */
    argvCmd[cmd1+i] = argv[arg1+i];
  }
  argvCmd[cmd1+i] = NULL;

  /* Save the initial drive and directory */

#if HAS_DRIVES
  iInitDrive = _getdrive();
  DEBUG_PRINTF(("Init drive = %c:\n", iInitDrive + '@'));
  if (pszFrom && pszFrom[0] && (pszFrom[1]==':')) {
    _chdrive(toupper(pszFrom[0])-'@');  // Change to the requested drive
  }
  DEBUG_PRINTF(("Work drive = %c:\n", _getdrive() + '@'));
#endif

  pszInitDir = getcwd0();
  if (!pszInitDir) {
    finis(RETCODE_INACCESSIBLE, "Can't get CWD. %s", strerror(errno));
  }

  /* Make sure to restore the initial drive/directory in case of a Ctrl-C */
  signal(SIGINT, OnControlC);

  if (!pszFrom) pszFrom = ".";

  /* Recurse */
  redo(pszFrom, &wdtOpts);

  if (iCtrlC) finis(RETCODE_ABORT, "Ctrl-C detected");

  if (iVerbose) printf("# Scanned %lu directories\n", (unsigned long)wdtOpts.nDir);

  /* Report if some errors were ignored */
  if (wdtOpts.nErr) {
    finis(RETCODE_INACCESSIBLE, "Failed to run in %d directories", wdtOpts.nErr);
  }

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
|		    							      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(int iErr) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: redo [SWITCHES] COMMAND_LINE\n\
\n\
Switches:\n\
  -?              Display this help screen and exit\n\
  -c              Change directories while recursing. (Default)\n\
  -C              Do not change directories while recursing.\n\
"
DEBUG_CODE(
"\
  -d              Debug mode. Display how things work internally.\n\
"
)
#if OS_HAS_LINKS
"\
  -f              Follow links to directories while recursing. (Default)\n\
  -F              Do not follow links to directories while recursing.\n\
"
#endif
"\
  -i PATH         Start recursion in the given directory. Default: \".\"\n\
  -l [MIN_LENGTH] List all sub-directories with their paths length. No command\n\
                  executed. Min length: List only longer paths. Default min: 1\n\
  -m MAX_DEPTH    Limit the recursion depth to N levels. Default: 0=no limit\n\
"
#if OS_HAS_LINKS
"\
  -o              Run only once in directories linked multiple times. (Default)\n\
  -O              Run again even it's been in the same directory before.\n\
"
#endif
"\
  -X              Display the commands to be executed, but don't run them.\n\
  -v              Verbose mode. Display the paths, and the commands executed.\n\
  -V              Display the program version and exit\n\
\n\
Command line:     Any valid command and arguments.\n\
                  The special sequence \"{}\" is replaced by the current\n\
                  directory name, relative to the initial directory.\n\
"
#ifdef _MSDOS
"\n\
Known limitation: MS-DOS only supports pathnames shorter than 260 characters.\n\
It will silently ignore directories with longer pathnames when scanning NTFS\n\
volumes on VM hosts or network servers.\n\
"
#endif
#ifdef _WIN32
"\n\
Known limitation with long pathnames ≥ 260 characters: Windows versions up to 8\n\
cannot change the current directory to such long pathnames. Windows ≥ 10 can,\n\
but only if long file name support has been enabled in the registry. And even\n\
in this case, it cannot run a command beyond that 260 characters threshold.\n\
On all versions of Windows, and whether or not the Windows 10 registry fix has\n\
been enabled, redo can enumerate paths of any length, and sets the {} sequence\n\
correctly. But it cannot execute a command beyond the 260 characters threshold.\n\
Actually redo will think it has succeeded, but the command will actually run\n\
in a parent directory of the expected one: The parent with the longest path\n\
that fits in less than 260 characters.\n\
If there's a chance that you might have paths longer than 260 chars in the tree\n\
below the initial directory, do not rely on the current directory set by redo,\n\
but use the -C option, and the {} sequence in arguments, to generate commands\n\
with absolute paths arguments.\n\
And of course, use a command that is compatible with paths ≥ 260 characters.\n\
To verify if this workaround is needed or not, use option -l 260 to list\n\
all paths ≥ 260 characters.\n\
"
#endif
#include "footnote.h"
);
  exit(iErr);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    finis						      |
|									      |
|   Description	    Display opt. msgs; Restore current path and drv, and exit.|
|									      |
|   Parameters	    retcode	The exit code				      |
|		    pszFormat	Optional format string to pass to printf      |
|		    ...		Optional printf arguments		      |
|		    							      |
|   Notes	    If retcode is not 0, but no error message is needed, it   |
|		    is necessary to pass a NULL for pszFormat.		      |
|		    							      |
|   Returns	    Does not return					      |
|									      |
|   History								      |
|    2018-04-30 JFL Redesigned with optional error messages to display.       |
*									      *
\*---------------------------------------------------------------------------*/

void finis(int retcode, ...) {
  DEBUG_ENTER(("finis(%d)\n", retcode));

  if (retcode) { /* There might be an error message to report */
    va_list vl;
    char *pszFormat;

    va_start(vl, retcode);
    pszFormat = va_arg(vl, char *);
    if (pszFormat) { /* There is an error message to report */
      pGenError(((retcode == RETCODE_ABORT) ? "Abort" : "Error"), pszFormat, vl, NULL);
    }
    va_end(vl);
  }

  chdir(pszInitDir);	/* Don't test errors, as we're likely to be here due to another error */
#if HAS_DRIVES
  _chdrive(iInitDrive);
#endif

  DEBUG_LEAVE(("exit(%d);\n", retcode));
  exit(retcode);
}

void OnControlC(int iSignal) {
  UNUSED_ARG(iSignal);
  signal(SIGINT, SIG_DFL); /* Terminate the program with a 2nd Ctrl+C in case proper cleanup hangs */
  iCtrlC = TRUE;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    interne						      |
|									      |
|   Description	    Check if a command is an internal shell command           |
|									      |
|   Parameters	    char *pszCommand	The command name		      |
|		    							      |
|   Returns	    TRUE or FALSE					      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History	    							      |
*									      *
\*---------------------------------------------------------------------------*/

int interne(char *com) {  /* Is com an internal command? */
  char nom[15];
  int i;

  strncpyz(nom, com, 15);
  strlwr(nom);

  for (i=0; i<(sizeof(internes)/sizeof(char *)); i++)
    if (strcmp(nom, internes[i]) == 0) return(TRUE);

  return(FALSE);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function        redo						      |
|                                                                             |
|   Description     Execute a command in every dir & all its subdirectories   |
|                                                                             |
|   Arguments	                                                              |
|     char *from    First directory to list.                                  |
|		    							      |
|   Return value    0=Success; !0=Failure                                     |
|		    							      |
|   Notes                                                                     |
|		    							      |
|   History                                                                   |
|    1993-10-15 JFL Initial implementation                                    |
|    1994-03-17 JFL Rewritten to recurse within the same appli. instance.     |
|    1994-05-27 JFL Updated for REDO.				              |
|    2025-12-02 JFL Rewritten using SysLib's WalkDirTree().		      |
*									      *
\*---------------------------------------------------------------------------*/

int SelectDirsCB(const char *pszPathname, const struct dirent *pDE, void *p) {
#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
  _kbhit();	/* Forces DOS to check for Ctrl-C, even if extended break is off */
#endif
  if (iCtrlC) return TRUE;	/* Abort scan */

  /* Execute the routine once for this path, after entering it */
  if (pDE->d_type == DT_ENTER) DoPerPath(pszPathname, p);

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
  _kbhit();	/* Forces DOS to check for Ctrl-C, even if extended break is off */
#endif
  if (iCtrlC) return TRUE;	/* Abort scan */

  return FALSE;			/* Continue scanning */
}

int redo(char *from, wdt_opts *pwo) {
  void *pScanVars = pwo;
  int iResult;

  DEBUG_ENTER(("descend(\"%s\", {%s});\n", from, ""));

  /* Get all subdirectories */
  pwo->iFlags |= WDT_DIRONLY | WDT_INONLY | WDT_CONTINUE;
  iResult = WalkDirTree(from, pwo, SelectDirsCB, pScanVars);
  if (iResult < 0) {	/* An error occurred */
    DEBUG_LEAVE(("return -1;\n"));
  } else {		/* The walk succeeded */
    DEBUG_LEAVE(("return 0;\n"));
  }
  return iResult;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DoPerPath						      |
|		    							      |
|   Description	    Routine to be executed once for each subdirectory	      |
|		    							      |
|   Arguments	                                                              |
|		    							      |
|   Return value    None						      |
|		    							      |
|   Notes	                                                              |
|		    							      |
|   History								      |
|    1994-05-27 JFL Updated for REDO.					      |
*									      *
\*---------------------------------------------------------------------------*/

void DoPerPath(const char *path, void *pRef) {
  wdt_opts *pwo = (wdt_opts *)pRef;
  int err;
  int i;
  char *pc;
  char **command2 = NULL;	/* Command and argument list secondary copy */
#ifdef _MSDOS
  int iTailSize = 1;		/* The command-line tail size */
#endif				/* DOS limits it to 128 bytes, including the final NUL */

  DEBUG_ENTER(("DoPerPath(\"%s\");\n", path));

  if (iMeasure) {
    int iLen;
    pc = getcwd0();
    if (!pc) finis(RETCODE_NO_MEMORY, "Can't get abs path for %s. %s", path, strerror(errno));
    iLen = (int)strlen(pc);
    if (iLen >= iMeasure) printf("%5d %s\n", iLen, pc);
    goto cleanup_and_return;
  }

  command2 = malloc((argcCmd+1) * sizeof(char *));
  if (!command2) finis(RETCODE_NO_MEMORY, "Not enough memory for expanded command list");

  for (i=0; i<argcCmd; i++) {
    char *pc2;
    char c;

    pc = argvCmd[i];
    if (!pc) break;
    pc2 = malloc(COMMAND_LINE_MAX);
    if (!pc2) finis(RETCODE_NO_MEMORY, "Not enough memory for command");
    command2[i] = pc2;
    do {
      c = *(pc++);
      *(pc2++) = c;
      if (   ((c == '%') && (*pc == '.'))    /* Initial redo-specific tag */
	  || ((c == '{') && (*pc == '}'))) { /* New find-specific tag */
        const char *pc3 = path;
	pc2 -= 1;   /* Back up over the % sign */
	/* Copy the part of the path relative to the initial path */
	if ((pc3[0] == '.') && (pc3[1] == DIRSEPARATOR_CHAR)) pc3 += 2; /* Skip the initial ./ */
	strcpy(pc2, pc3);
	pc2 += strlen(pc2);	// Move to the end of string
	pc += 1;		// Skip the end of the tag ('.' or '}')
      }
    } while (c);
    // Free the end of string
    command2[i] = ShrinkBuf(command2[i], ++pc2 - command2[i]);
    DEBUG_PRINTF(("arg[%d] = \"%s\";\n", i, command2[i]));
#ifdef _MSDOS
    if (i > 0) iTailSize += strlen(command2[i]) + 1;
#endif
  }
  command2[i] = NULL;

  if (iVerbose || iNoExec) {
    if (pwo->iFlags & WDT_CD) {
      const char *pcc = path;
      if ((pcc[0] == '.') && (pcc[1] == DIRSEPARATOR_CHAR)) pcc += 2;
      printf("[%s] ", pcc);
    }
    for (i=0; ((pc=command2[i]) != NULL); i++) printf("%s ", pc);
    printf("\n");
  }
#ifdef _MSDOS
  if (iTailSize > 128) finis(RETCODE_EXEC_ERROR, "Command too long"); /* else spawnv() crashes below! */
#endif
  if (!iNoExec) {
    pc = strrchr(command2[0], DIRSEPARATOR_CHAR);
    if (pc) pc += 1; else pc = command2[0]; /* The node name of the child command */
#if HAS_DRIVES
    if (pc[0] && (pc[1] == ':')) pc += 2;
#endif
    err = (int)spawnvp(P_WAIT, command2[0], command2);
    DEBUG_PRINTF(("Child exit code 0x%02X\n", err));
    if (err == -1) finis(RETCODE_EXEC_ERROR, "Cannot execute %s. %s", command2[0], strerror(errno));
    if (WIFEXITED(err)) {
      err = WEXITSTATUS(err); /* Pass-on the actual child exit code */
    } else if (WIFSIGNALED(err) && (WTERMSIG(err) == SIGINT)) {
      /* This happens if the child does _not_ intercept the SIGINT signal.
	 In this case, the child is terminated by the system */
      fputs("\n", stderr); /* The child output ends with a "^C" string with no \n */
      DEBUG_CODE(
	if (getenv("NOCTRLC2")) {
	  /* Experimental code: See what happens if not calling finis() now */
	  printf("redo: NOCTRLC2, %s interrupted by a Ctrl-C, continuing\n", pc);
	} else
      )
      finis(RETCODE_ABORT, "%s interrupted by a Ctrl-C", pc);
    } else {
      /* The child got killed for various other reasons */
      finis(RETCODE_ABORT, "%s aborted", pc);
    }
    if (err) {
      pfnotice(NULL, "%s exited with error # %d", pc, err);
    }
  }

  for (i=0; command2[i]; i++) free(command2[i]); // Free the copy of the command.
  free(command2); /* Free the array of pointers */

cleanup_and_return:
  RETURN();
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
|    2025-12-16 JFL Do not display an error message here.		      |
|    2025-12-18 JFL Specify the pid number to wait for.			      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _UNIX

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
    return -1;
  } else {			// We're the parent instance
    switch(iMode) {
      case P_WAIT:
      	waitpid(pid, &iRet, 0);
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

