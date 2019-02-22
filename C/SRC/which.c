/*****************************************************************************\
*		    							      *
*   Filename	    which.c						      *
*		    							      *
*   Description     Find which versions of a program are accessible	      *
*		    							      *
*   Notes	    Inspired by the Unix which.				      *
*		    In DOS and Windows, we need to extend the search to       *
*		    all files with one of the default program extensions.     *
*		    							      *
*		    Gotcha in WoW, that is WIN32 programs running on WIN64:   *
*		    Windows silently redirects accesses to %windir%\System32  *
*		    to %windir%\SysWOW64.				      *
*		    %windir%\System32 contains 64-bits programs.	      *
*		    %windir%\SysWOW64 contains 32-bits programs.	      *
*		    Problem: Some 64-bits programs in the former have no      *
*		    32-bits equivalent in the latter. So which won't see them.*
*		    Since the very goal of using the which command is to find *
*		    which program will be executed at the cmd.exe prompt,     *
*		    which is 64-bits in this case, the result in inaccurate.  *
*		    Workaround: Use the %windir%\Sysnative alias the real     *
*		    		%windir%\System32.			      *
*		    							      *
*		    Uses Unix C library routines not available in Microsoft's *
*		    MSVC C library.					      *
*		    To build in Windows use JFL's MsvcLibX library extensions *
*		    with MSVC.						      *
*		    Does not build in MinGW for now, as it lacks getppid().   *
*		    							      *
*		    Uses JFL's custom debugging macros in debugm.h.	      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc -U_DEBUG which.c -o which	    # Release version *
*		    gcc -D_DEBUG which.c -o which.debug     # Debug version   *
*		    							      *
*   History								      *
*    1987       JFL jf.larvoire@hp.com created this program.		      *
*    1994-09-29 JFL Added OS/2 support.					      *
*    1997-06-06 JFL Added Win32 support.				      *
*    2005-03-14 JFL Output the 1st match only, unless -a option. As Linux.    *
*    2008-06-16 JFL Use the list in env. var. PATHEXT if present.	      *
*		    Version 1.3.					      *
*    2012-02-02 JFL Added the ability to search for a given extension only.   *
*		    Fixed a bug that prevented detection of 64=bit progs      *
*		    in the Windows\System32 directory.			      *
*		    Added support for MinGW, Cygwin, Linux, etc.	      *
*    2012-02-03 JFL Changed version format to: VERSION DATE OS [DEBUG]	      *
*		    Added option -V.					      *
*		    Version 1.4.					      *
*    2012-10-18 JFL Added my name in the help. Version 1.4.1.                 *
*    2013-03-22 JFL Added support for PowerShell, which also starts *.ps1.    *
*		    Do not search in the cur. dir. in Linux and PowerShell.   *
*		    Version 1.5.					      *
*    2013-03-27 JFL Rewrote routine getppid() for WIN64; Moved it to MsvcLibX.*
*		    Correct the output file name case if needed.	      *
*		    Use my debug macros.				      *
*    2013-03-28 JFL Allow searching any number of names, including 0.         *
*		    Version 1.6.					      *
*    2013-03-30 JFL Avoid using fnmatch(). Version 1.6.1.		      *
*    2013-06-04 JFL Detect PowerShell even when invoked in a remote session.  *
*		    Version 1.6.2.					      *
*    2013-03-24 JFL Rebuilt with MsvcLibX.lib with support for UTF-8 names.   *
*		    Version 1.7.					      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 1.7.1.					      *
*    2017-09-04 JFL Test variable NoDefaultCurrentDirectoryInExePath in cmd.  *
*		    Display MsvcLibX library version in DOS & Windows.        *
*		    Version 1.8.					      *
*    2017-10-11 JFL In Windows, also check PS and cmd.exe internal commands.  *
*    2017-10-26 JFL Added options -i and -I to control that int. cmd. search. *
*		    Bugfix: WIN64 will run .com files, which may be exe inside.
*    2017-10-27 JFL Put MSC-specific pragmas within a #ifdef _MSC_VER/#endif. *
*		    Version 1.9.					      *
*    2018-03-21 JFL In verbose mode, display the programs time & links target.*
*    2018-03-22 JFL Moved yesterday's change to new option -l, and changed -v *
*		    to display comments about programs excluded.	      *
*		    Fixed an error message when run in WIN32 LXSS bash.exe.   *
*		    Version 1.10.					      *
*    2019-01-16 JFL Added option -- to stop processing switches.              *
*		    Version 1.11.					      *
*    2019-02-17 JFL Avoid warnings about assignments in cond. expressions.    *
*		    Renamed options -i and -I as -s and -S.		      *
*		    New options -i and -I search WIN32 doskey aliases.	      *
*    2019-02-22 JFL Added -i support for PowerShell.			      *
*		    Version 1.12.					      *
*		    							      *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.12"
#define PROGRAM_DATE    "2019-02-22"

#define _CRT_SECURE_NO_WARNINGS 1

#define _GNU_SOURCE		/* Force MsvcLibX.lib to use UTF-8 strings,
				   and Unix to define lots of extras */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use MinGW, or JFL's MsvcLibX library extensions for MS Visual C++ */
#include <unistd.h>		/* For the access function */
#include <dirent.h>		/* We use the DIR type and the dirent structure */
#include <sys/stat.h>		/* To get the actual file time */
#include <time.h>		/* To get the actual file time */
#include <strings.h>		/* For strcasecmp() */

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

typedef enum {
  SHELL_UNKNOWN = 0,
  SHELL_COMMAND,			/* Windows command.com */
  SHELL_CMD,				/* Windows cmd.exe */
  SHELL_POWERSHELL,			/* Windows PowerShell.exe */
  SHELL_BASH,				/* Unix bash */
} shell_t;

char *pszShells[] = {
  "unknown",
  "command.com",			/* Windows command.com */
  "cmd.exe",				/* Windows cmd.exe */
  "PowerShell",				/* Windows PowerShell.exe */
  "bash",				/* Unix bash */
};

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

char *pszExtDos[] = {"COM", "EXE", "BAT", NULL};
char **ppszExt = pszExtDos;

#define SEARCH_IN_CD TRUE  /* Command.com searches in the current directory */
#define DEFAULT_SHELL SHELL_COMMAND

#endif

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* Automatically defined when targeting an OS/2 application? */

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h"

char *pszExtReal[] = {"COM", "EXE", "BAT", NULL};
char *pszExtProt[] = {"EXE", "CMD", NULL};
char **ppszExt = pszExtProt;

#define SEARCH_IN_CD TRUE  /* The OS/2 shell searches in the current directory */
#define DEFAULT_SHELL SHELL_COMMAND

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

/* Note: In Windows NT, some files may have the .com extension, but actually be
   structured as .exe internally. Look for the 'MZ' header to tell if it's an exe.
   If they are an old-style DOS .com, and your version of Windows does not have
   the 16-bits subsystem installed, then this will trigger an error popup.
   Do not attempt to detect this in which.exe: If a DOS .com is in the PATH,
   it is it that Windows will attempt (and fail) to run. */
char *pszExtWin32[] = {"com", "exe", "cmd", "bat", NULL};
char **ppszExt = pszExtWin32;

#define SEARCH_IN_CD TRUE  /* Cmd.exe searches in the current directory */
#define DEFAULT_SHELL SHELL_CMD

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__	/* Automatically defined when targeting a Unix application */

char *pszExtUnix[] = {NULL};
char **ppszExt = pszExtUnix;

#define SEARCH_IN_CD FALSE  /* Unix shells do not search in the current directory */
#define DEFAULT_SHELL SHELL_BASH

#include <strings.h> /* For strcasecmp() */
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#define _makepath(buf, d, p, n, x) do {strcpy(buf,p); strcat(buf,"/"); strcat(buf,n);} while (0)

#endif

/*********************************** Other ***********************************/

#ifndef EXE_OS_NAME

#define EXE_OS_NAME "Unidentified_OS"

char *pszExtOther[] = {NULL};
char **ppszExt = pszExtOther;

#define SEARCH_IN_CD TRUE

#endif

/********************** End of OS-specific definitions ***********************/

#define streq(s1, s2) (!strcmp(s1, s2))
#define strieq(s1, s2) (!_stricmp(s1, s2))
#define strnieq(s1, s2, n) (!_strnicmp(s1, s2, n))

typedef unsigned char BYTE;

#define FALSE 0
#define TRUE 1

/* Search flags */

#define WHICH_ALL	0x01	    /* Display all commands found. Default: Just the first one */
#define WHICH_VERBOSE	0x02	    /* Display verbose information */
#define WHICH_LONG	0x04	    /* Display the file time */
#define WHICH_XCD	0x08	    /* Force eXcluding files in CD */

/* Global variables */

#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
char szSystem32dir[FILENAME_MAX];
char szSystem64dir[FILENAME_MAX];
int iWoW = FALSE;		    /* TRUE = WIN32 program running in WIN64 */
#endif
shell_t shell = DEFAULT_SHELL;

/* Prototypes */

char *version(int iVerbose);	    /* Build the version string. If verbose, append library versions */
void usage(void);
int IsSwitch(char *pszArg);
int SearchProgramWithAnyExt(char *pszPath, char *pszCommand, int iSearchFlags);
int SearchProgramWithOneExt(char *pszPath, char *pszCommand, char *pszExt, int iSearchFlags);
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace);
#endif
#if defined(_WIN32)
int GetProcessName(pid_t pid, char *name, size_t lname);
int FixNameCase(char *pszPathname);
#endif
#if defined(_MSDOS) || defined(_WIN32)
int SearchCommandInternal(char *pszCommand, int iSearchFlags);
#endif
#if defined(_WIN32)
char **GetInternalCommands(void);    /* Get a NULL-terminated list of internal shell commands */
int SearchCmdInternal(char *pszCommand, int iSearchFlags);
int SearchCmdAliases(char *pszCommand, int iSearchFlags);
int SearchPowerShellAliases(char *pszCommand, int iSearchFlags);
#endif
#if defined(_WIN32) || defined(__unix__)
int SearchPowerShellInternal(char *pszCommand, int iSearchFlags);
int SearchBashInternal(char *pszCommand, int iSearchFlags);
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Main program routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char *pszCommand;
  char *pszPath;
  char *pszTok;
  int iArg;
  char *arg;
  int iFlags = 0;
  int iFound = 0;
#if defined(_WIN32)
  char szShellName[FILENAME_MAX+1];
  int iAlias = 0;
#endif
  char **pathList = malloc(0);
  int nPaths = 0;
  int iPath;
  int iVerbose = FALSE;
  int iInternal = -1;
  int iSearchInCD = SEARCH_IN_CD;   /* TRUE = Search in the current directory first */
  int iProcessSwitches = TRUE;

  for (iArg=1; iArg<argc; iArg++) { /* Process all command line arguments */
    arg = argv[iArg];
    if (iProcessSwitches && IsSwitch(arg)) {
      char *opt = arg+1;
      if (   streq(opt, "-")) {		/* Stop processing switches */
	iProcessSwitches = FALSE;
	continue;
      }
      if (   streq(opt, "?")		/* Display a help screen */
	  || streq(opt, "h")
	  || streq(opt, "-help")) {
	usage();
      }
      if (   streq(opt, "a")		/* Display all matching programs */
	  || streq(opt, "-all")) {
	iFlags |= WHICH_ALL;
	continue;
      }
      DEBUG_CODE(
      if (   streq(opt, "d")		/* Debug mode on */
	  || streq(opt, "-debug")) {
	DEBUG_ON();
	iVerbose = TRUE;
	printf("Debug mode on.\n");
	continue;
      }
      )
#if defined(WIN32)
      if (   streq(opt, "i")		/* Read aliases from stdin */
	  || streq(opt, "-read-alias")) {
	iAlias = 1;
	continue;
      }
      if (   streq(opt, "I")		/* Do NOT read aliases from stdin */
	  || streq(opt, "-skip-alias")) {
	iAlias = 1;
	continue;
      }
#endif // defined(WIN32)
      if (   streq(opt, "l")		/* Display the program time and link target */
	  || streq(opt, "-long")) {
	iFlags |= WHICH_LONG;
	continue;
      }
      if (   streq(opt, "s")		/* Search the shell's internals */
	  || streq(opt, "-internal")) {
	iInternal = 1;
	continue;
      }
      if (   streq(opt, "S")		/* Do not search the shell's internals */
	  || streq(opt, "-no-internal")) {
	iInternal = 0;
	continue;
      }
      if (   streq(opt, "v")		/* Verbose mode on */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	iFlags |= WHICH_VERBOSE | WHICH_LONG;
	continue;
      }
      if (   streq(opt, "V")		/* Get version */
	  || streq(opt, "-version")) {
	printf("%s\n", version(1));
	exit(0);
      }
      printf("Error: Invalid switch ignored: %s\n", arg);
      continue;
    }
    /* This is an argument, not a switch */
    break;	/* All arguments are processed below, outside of this loop */
  }

  /* Find in which shell we're running. PowerShell and cmd search programs differently. */ 
#if defined(_WIN32)
  GetProcessName(getppid(), szShellName, sizeof(szShellName));
  DEBUG_PRINTF(("Executed inside %s.\n", szShellName));
  /* wsmprovhost.exe is PowerShell's remote session host process */
  if (strieq(szShellName, "powershell.exe") || strieq(szShellName, "wsmprovhost.exe")) {
    shell = SHELL_POWERSHELL;
    if (iInternal == -1) iInternal = 0; /* The search is very slow the first time. Don't do it by default. */
    iSearchInCD = FALSE; /* Contrary to cmd, PowerShell does not search in the current directory first */
  } else if (strieq(szShellName, "cmd.exe")) { 
    shell = SHELL_CMD;
    if (iInternal == -1) iInternal = 1; /* The search is fast. Do it by default. */
  } else if (strieq(szShellName, "command.com")) { 
    shell = SHELL_COMMAND;
    if (iInternal == -1) iInternal = 0;	/* The search method is not yet implemented. No need to call the stub. */
  } else if (strieq(szShellName, "bash.exe")) { 
    shell = SHELL_BASH;
    if (iInternal == -1) iInternal = 1; /* The search is fast. Do it by default. */
  }
  if (iInternal == -1) iInternal = 0; /* Unidentified shell. Do not search internals, we don't know how anyway */

#if defined(_MSC_VER)
#pragma warning(disable:4996) /* Ignore the "'GetVersion': was declared deprecated" warning */
#endif
  if (   (shell == SHELL_CMD)		/* If this is cmd.exe */
      && (LOBYTE(GetVersion()) >= 6)	/* And the OS is Vista or later */ 
      && getenv("NoDefaultCurrentDirectoryInExePath")) {
    iSearchInCD = FALSE; /* This disables search in the Current Directory first */
    if (iVerbose) printf("# Environment variable NoDefaultCurrentDirectoryInExePath is set => No search in .\n");
  }
#if defined(_MSC_VER)
#pragma warning(default:4996) /* Restore the "'GetVersion': was declared deprecated" warning */
#endif
#endif

  /* Get the PATH environment variable, and work around known issues in Win32 on WIN64 */
  pszPath = getenv("PATH");
  DEBUG_PRINTF(("set PATH=\"%s\"\n", pszPath));
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
  if (getenv("PROCESSOR_ARCHITEW6432")) { /* This is indeed a 32-bits app on WIN64 */
    size_t l = strlen(pszPath)+1024;
    char *pPath = malloc(l);
    char *pszWindir;
    iWoW = TRUE;
    pszWindir = getenv("windir");
    _makepath(szSystem32dir, "", pszWindir, "System32", NULL);
    _makepath(szSystem64dir, "", pszWindir, "Sysnative", NULL);
    strnirepl(pPath, l, pszPath, szSystem32dir, szSystem64dir);
    pszPath = pPath;
    DEBUG_CODE_IF_ON(
      printf("This is Win32 on Win64.\n");
      printf("szSystem32dir =\"%s\";\n", szSystem32dir);
      printf("szSystem64dir =\"%s\";\n", szSystem64dir);
      printf("pszPath =\"%s\";\n", pszPath);
    )
  } else {
    DEBUG_PRINTF(("This is NOT Win32 on Win64.\n"));
  }
#endif

#if defined(__unix__)
  if (iVerbose) iVerbose = 1; /* Stupid trick to avoid a warning */
#endif

  /* Build the list of directories to search in */
  if (iSearchInCD) {
    pathList = realloc(pathList, (sizeof(char *))*(++nPaths));
    pathList[nPaths-1] = ".";		/* Start with the current directory */
  }
#ifdef __unix__
#define PATH_SEP ":"
#else
#define PATH_SEP ";"
#endif
  if (pszPath) {
    for (pszTok = strtok(pszPath, PATH_SEP); pszTok; pszTok = strtok(NULL, PATH_SEP)) {
      pathList = realloc(pathList, (sizeof(char *))*(++nPaths));
      pathList[nPaths-1] = pszTok;	/* Append each PATH directory */
    }
  }

  /* Finally search for all the requested programs */
  for ( ; iArg<argc; iArg++) { /* Process all remaining command line arguments */
    pszCommand = argv[iArg];

    /* First search in internal commands */
#if defined(_WIN32)
    if (iAlias) {
      switch (shell) {
	case SHELL_CMD:
	  iFound = SearchCmdAliases(pszCommand, iFlags);
	  break;
	case SHELL_POWERSHELL:
	  iFound = SearchPowerShellAliases(pszCommand, iFlags);
	  break;
	default:
	  iFound = FALSE;
	  break;
      }
      if (iFound && !(iFlags & WHICH_ALL)) continue;
    } else
#endif
    if (iInternal) {
      switch (shell) {
#if defined(_MSDOS) || defined(_WIN32)
	case SHELL_COMMAND:
	  iFound = SearchCommandInternal(pszCommand, iFlags);
	  break;
#endif
#if defined(_WIN32)
	case SHELL_CMD:
	  iFound = SearchCmdInternal(pszCommand, iFlags);
	  break;
#endif
#if defined(_WIN32) || defined(__unix__)
	case SHELL_POWERSHELL:
	  iFound = SearchPowerShellInternal(pszCommand, iFlags);
	  break;
	case SHELL_BASH:
	  iFound = SearchBashInternal(pszCommand, iFlags);
	  break;
#endif
	default:
	  iFound = FALSE;
	  break;
      }
      if (iFound && !(iFlags & WHICH_ALL)) continue;
    }

    /* In verbose mode, show eXcluded programs in CD */
#if !defined(_MSDOS)
    if ((iFlags & WHICH_ALL) && (iFlags & WHICH_VERBOSE) && !iSearchInCD
        && strcmp(pathList[0], ".")) {
      SearchProgramWithAnyExt(".", pszCommand, iFlags | WHICH_XCD);
    }
#endif

    /* Then search the PATH */
    for (iPath=0; iPath < nPaths; iPath++) {
      iFound = SearchProgramWithAnyExt(pathList[iPath], pszCommand, iFlags);
      if (iFound && !(iFlags & WHICH_ALL)) break;
    }
  }

  return iFound ? 0 : 1;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    usage						      |
|									      |
|   Description     Display a brief help for this program and exit	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

/* Get the program version string, optionally with libraries versions */
char *version(int iLibsVer) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " EXE_OS_NAME DEBUG_VERSION;
  char *pszVer = NULL;
  if (iLibsVer) {
    char *pszLibVer = ""
#if defined(_MSVCLIBX_H_)	/* If used MsvcLibX */
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
#if defined(__SYSLIB_H__)	/* If used SysLib */
#include "syslib_version.h"
	  " ; SysLib " SYSLIB_VERSION
#endif
    ;
    pszVer = (char *)malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  printf("\n\
Which version %s - Find which program will be executed\n\
\n\
Usage: which [OPTIONS] [COMMAND[.EXT] ...]\n\
\n\
Options:\n\
  --    Stop processing switches.\n\
  -?    Display this help message and exit.\n\
  -a    Display all matches. Default: Display only the first one.\n"
#if defined(_WIN32)
"\
  -i    Read shell internal commands and aliases on stdin (1)\n\
  -I    Do not read shell internal commands and aliases on stdin (Default)\n"
#endif
"\
  -l    Long mode. Also display programs time, and links target.\n\
  -s    Search for the shell internal commands first. (Deprecated, prefer -i)\n\
  -S    Do not search for the shell internal commands. (Faster, default)\n\
  -v    Verbose mode. Like -l, plus comments about non-eligible programs.\n\
  -V    Display this program version and exit.\n\
\n"
#if defined(_WIN32)
"\
(1) In cmd.exe, this requires defining a doskey macro for which itself:\n\
    doskey /macros which=^(help ^& doskey /macros^) ^| which.exe -i $*\n\
    In PowerSehll, this requires defining a function for which itself:\n\
    Function which () {(dir function:)+(dir alias:) | which.exe -i @args}\n\
\n\
Notes:\n\
  Uses the PATHEXT variable to infer other possible names.\n\
  Supports specific rules for cmd and PowerShell.\n\
  When using the -i option (1), searches for internal commands and aliases:\n\
        Ex in cmd.exe:    'which md' outputs: cmd /c MD\n\
        Ex in PowerShell: 'which md' outputs: Alias md -> mkdir\n\
        Ex in both cases: 'which which' outputs the macro/function in (1)\n\
           whereas 'which which.exe' outputs the full pathname to which.exe.\n\
  When not using the -i option, searches only for executables.\n\
        Ex: 'which which' outputs the full pathname to which.exe.\n\
  Option -s uses a child shell to seach for internal commands. It is\n\
  recommended to use option -i instead, as -i runs much faster.\n\
\n"
#endif
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
, version(0));

  exit(0);
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
|   Function	    initExtList						      |
|									      |
|   Description     Initialize the DOS/Windows extension list          	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2008-06-16 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int initExtListDone = FALSE;
void initExtList() {
#if defined(_WIN32)
  /* Build the extension list, based on variable PATHEXT if present. */
  char *pszPathExt = getenv("PATHEXT");
#endif

#if defined(_OS2)
  BYTE b;

  DosGetMachineMode(&b);
  if (b == MODE_REAL)
    ppszExt = pszExtReal;
  else /* MODE_PROTECTED */
    ppszExt = pszExtProt;
#endif

#if defined(_WIN32)
  DEBUG_PRINTF(("  PATHEXT = \"%s\"\n", pszPathExt));
  if (pszPathExt) { /* If defined, use it to rebuild the list dynamically. */
    char **pBuf;
    char *pc;
    char *pcEnd = NULL;
    int nExt = 0;

    pBuf = (char **)malloc(sizeof(char *)); /* Room for the final NUL */
    /* Special case: If the parent is PowerShell, prepend the .ps1 extension */
    if (shell == SHELL_POWERSHELL) {
      pBuf = realloc(pBuf, (nExt + 2) * sizeof(char *));
      pBuf[nExt] = "ps1";
      nExt += 1;
    }
    /* Append each extension to the PATHEXT list */
    for (pc = pszPathExt; pc && *pc; pc = pcEnd) {
      if (!*++pc) break; /* Skip the . */
      pcEnd = strchr(pc, ';');
      if (pcEnd) {
	*pcEnd = '\0';
	pcEnd += 1; /* Skip the ; */
      }
      DEBUG_PRINTF(("  EXT = \"%s\"\n", pc));

      pBuf = realloc(pBuf, (nExt + 2) * sizeof(char *));
      pBuf[nExt] = pc;
      nExt += 1;
    }
    pBuf[nExt] = NULL;
    ppszExt = pBuf;
  }
#endif

  initExtListDone = TRUE;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetInternalCommands					      |
|									      |
|   Description     Build a list of internal shell commands          	      |
|									      |
|   Returns:	    A pointer to a NULL-terminated list of internal commands  |
|									      |
|   Notes	    Caches the list in a temporary file, to improve perf.     |
|		    The temporary file name includes the OS version, so that  |
|		    it's rebuilt if a new OS update includes more commands.   |
|		    							      |
|   History								      |
|    2017-10-11 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)

#define BYTE0(var) (((BYTE *)&(var))[0])
#define BYTE1(var) (((BYTE *)&(var))[1])
#define WORD0(var) (((WORD *)&(var))[0])
#define WORD1(var) (((WORD *)&(var))[1])

char **GetInternalCommands() {
  char **ppszInternalCommands = NULL;
  int nCmd = 0;
#if defined(_WIN32)
  char *pszListFile = NULL;
  size_t l;
  DWORD dwVersion;
  int iMajorVersion;
  int iMinorVersion; 
  int iBuild = 0;
  char *pszTemp = getenv("TEMP");
  if (!pszTemp) pszTemp = getenv("TMP");
  if (!pszTemp) return NULL;
  DEBUG_PRINTF(("pszTemp = \"%s\"\n", pszTemp));
  pszListFile = malloc(strlen(pszTemp) + 40);
  if (!pszListFile) return NULL;
#if defined(_MSC_VER)
#pragma warning(disable:4996) /* Ignore the "'GetVersion': was declared deprecated" warning */
#endif
  dwVersion = GetVersion();
#if defined(_MSC_VER)
#pragma warning(default:4996) /* Restore the "'GetVersion': was declared deprecated" warning */
#endif
  iMajorVersion = (int)BYTE0(dwVersion);
  iMinorVersion = (int)BYTE1(dwVersion);
  if (dwVersion < 0x80000000) iBuild = (int)WORD1(dwVersion);
  strcpy(pszListFile, pszTemp);
  l = strlen(pszTemp);
  if (l && (pszListFile[l-1] != '\\')) strcpy(pszListFile+(l++), "\\");

  if (shell == SHELL_CMD) { /* The shell is cmd.exe */
    FILE *hf;
    char buf[40];
    sprintf(pszListFile+l, "cmd-internal-%d.%d.%d.lst", iMajorVersion, iMinorVersion, iBuild);
    DEBUG_PRINTF(("pszListFile = \"%s\"\n", pszListFile));
    if (access(pszListFile, 0) == -1) { /* If the file does not exist */
      char *pszCmd = malloc(strlen(pszListFile) + 200);
      if (!pszCmd) goto cleanup_and_return;
      /* Generate a batch command that gets help about all standard commands, selects the internal ones, and stores them in pszListFile */
      sprintf(pszCmd, "(for /f \"tokens=1\" %%c in ('help ^| findstr /r /c:\"^[A-Z][A-Z]*  \"') do @if not exist \"%%WINDIR%%\\System32\\%%c.exe\" if not exist \"%%WINDIR%%\\System32\\%%c.com\" echo %%c) >\"%s\" 2>NUL", pszListFile);
      DEBUG_PRINTF(("pszCmd = (%s)\n", pszCmd));
      system(pszCmd);
      free(pszCmd);
    }
#define MAX_INTERNAL_COMMANDS 100
    ppszInternalCommands = malloc((MAX_INTERNAL_COMMANDS+1)*sizeof(char *));
    if (!ppszInternalCommands) goto cleanup_and_return;
    hf = fopen(pszListFile, "rt");
    while (fgets(buf, sizeof(buf), hf)) {
      for (l = strlen(buf); l && ((buf[l-1]=='\r') || (buf[l-1]=='\n')); ) buf[--l] = '\0'; /* Trim trailing CR/LF */
      ppszInternalCommands[nCmd++] = strdup(buf);
      if (nCmd == MAX_INTERNAL_COMMANDS) {
      	fprintf(stderr, "which: Warning: Too many internal commands in \"%s\"\n", pszListFile);
      	break; /* Avoid buffer overflow */
      }
    }
    ppszInternalCommands[nCmd] = NULL;
    fclose(hf);
  }
cleanup_and_return:
  free(pszListFile);
#endif /* defined(_WIN32) */
  DEBUG_CODE_IF_ON(
    int i;
    DEBUG_PRINT_INDENT();
    if (ppszInternalCommands) {
      printf("GetInternalCommands(); // return {");
      for (i=0; i<nCmd; i++) printf("%s%s", i?" ":"", ppszInternalCommands[i]);
      printf("}\n");
    } else {
      printf("GetInternalCommands(); // return NULL\n");
    }
  )
  return ppszInternalCommands;
}

static char **ppszCmdInternals = NULL;

int SearchCmdInternal(char *pszCommand, int iFlags) {
  char **ppszIntCmd;
  int iMargin = (iFlags & WHICH_LONG) ? 20 : 0; /* Margin to align with matches that display a file time */
  if (!ppszCmdInternals) ppszCmdInternals = GetInternalCommands();
  if (ppszCmdInternals) for (ppszIntCmd = ppszCmdInternals; *ppszIntCmd; ppszIntCmd++) {
    if (!_stricmp(pszCommand, *ppszIntCmd)) {
      printf("%*scmd /c %s\n", iMargin, "", *ppszIntCmd);
      return TRUE;
      break;
    }
  }
  return FALSE;
}

#endif /* defined(_WIN32) */

#if defined(_WIN32) || defined(__unix__)

int SearchPowerShellInternal(char *pszCommand, int iFlags) {
  char szCmd[1024];
  int iRet;
  int iMargin = (iFlags & WHICH_LONG) ? 20 : 0; /* Margin to align with matches that display a file time */
  /* Generate a PowerShell command that gets internal commands by the given name, and displays which.exe output directly */
  sprintf(szCmd, "powershell -ExecutionPolicy Bypass -c \"Get-Command %s -CommandType Alias, Cmdlet, Function, Workflow -ErrorAction SilentlyContinue | %% {\\\"%*s$($_.CommandType) \\\"+$(if ($_.DisplayName) {$_.DisplayName} else {$_.Name})}\"", pszCommand, iMargin, "");
  DEBUG_PRINTF(("%s\n", szCmd));
  iRet = system(szCmd); /* Returns 0 if a command was found, else a non-0 error code */
  DEBUG_PRINTF(("  exit %d\n", iRet));
  return (iRet == 0);
}

#endif /* defined(_WIN32) || defined(__unix__) */

#if defined(_WIN32) || defined(__unix__)

#if defined(__unix__)
/* The Unix version of system() runs sh, not bash, even if environment variable SHELL=/bin/bash.
   So we need to start the real bash, and pass it our internal command detection script */
#define BASH "$SHELL"
#else /* WIN32 */
/* The WIN32 version of system() runs cmd.exe, even when run from within bash.exe.
   Also bash.exe is only present in the %windir%\SysNative directory,
   not in the %windir%\SysWOW64 directory, so Win32 apps don't see it. */
#define BASH   "%windir%\\System32\\bash.exe"
#define BASH64 "%windir%\\SysNative\\bash.exe"
#endif

int SearchBashInternal(char *pszCommand, int iFlags) {
  char szCmd[1024];
  int iRet;
  int iMargin = (iFlags & WHICH_LONG) ? 20 : 0; /* Margin to align with matches that display a file time */
  int n;
  /* Generate a bash command that gets help about internal commands, and displays which.exe output directly */
#if defined(__unix__)	/* This is a Unix app running on Unix */
  n = sprintf(szCmd, "%s", BASH);
#elif defined(_WIN64)	/* This is a WIN64 app running on WIN64 */
  n = sprintf(szCmd, "%s", BASH);
#elif defined(_WIN32)
 /* Special case for WIN32 on WIN64 */
    if (iWoW) { 	/* This is a WIN32 app running on WIN64 */
      n = sprintf(szCmd, "%s", BASH64);
    } else {		/* This is a WIN32 app running on WIN32 */
      n = sprintf(szCmd, "%s", BASH);
    }
#else
  #error "Unsuported OS combination"
#endif
  n += sprintf(szCmd+n, " -c \"help %s >/dev/null 2>&1 && echo '%*sbash -c %s'\"", pszCommand, iMargin, "", pszCommand);

  DEBUG_PRINTF(("%s\n", szCmd));
  iRet = system(szCmd); /* Returns 0 if a command was found, else a non-0 error code */
  DEBUG_PRINTF(("  exit %d\n", iRet));
  return (iRet == 0);
}

#endif /* defined(_WIN32) || defined(__unix__) */

#if defined(_MSDOS) || defined(_WIN32)

#if defined(_MSDOS)
/* TODO: DOS does not have _popen. Actually it might not be possible due to the lack of multitasking.
         Using an intermediate file is precisely what the GetInternalCommands() function above does */ 
FILE * __cdecl _popen(const char *pszCommand, const char *pszMode);
void _pclose(FILE *hf);
/* Dummy implementations, so that the code builds and links, but does nothing */
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
FILE * __cdecl _popen(const char *pszCommand, const char *pszMode) {
  return fopen("NUL", pszMode);
}
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */
void _pclose(FILE *hf) {
  fclose(hf);
}
#endif /* defined(_MSDOS) */

char **GetComspecCommands() {
  char **ppszInternalCommands = NULL;
  int nCmd = 0;
  char *pszListFile = NULL;
  char szCmd[128];
  char szLine[128];
  FILE *hCmd;
  int i;
  char *pszShell;

  pszShell = getenv("COMSPEC");
  if (!pszShell) goto cleanup_and_return;
  DEBUG_PRINTF(("pszShell = \"%s\"\n", pszShell));

#define MAX_INTERNAL_COMMANDS 100
  ppszInternalCommands = malloc((MAX_INTERNAL_COMMANDS+1)*sizeof(char *));
  if (!ppszInternalCommands) goto cleanup_and_return;

  sprintf(szCmd, "%s /C HELP", pszShell); /* Do not use quotes around the shell name. This won't work in DOS. */
  hCmd = _popen(szCmd, "rt");
  if (!hCmd) {
    fprintf(stderr, "which: Error: Can't run \"%s\" /C HELP\n", pszShell);
    return NULL;
  }
  while(fgets(szLine, sizeof(szLine), hCmd)) {
    for (i=0; ; i++) if ((szLine[i] < 'A') || (szLine[i] > 'Z')) break;
    if (i < 2) continue; /* If there are less than 2 consecutive capital letters, this is not a command name */
    szLine[i] = '\0';
    /* TODO: Check if it's internal or external */
    ppszInternalCommands[nCmd++] = strdup(szLine);
    if (nCmd == MAX_INTERNAL_COMMANDS) {
      fprintf(stderr, "which: Warning: Too many internal commands in \"%s\"\n", pszListFile);
      break; /* Avoid buffer overflow */
    }
  }
  _pclose(hCmd);

cleanup_and_return:
  DEBUG_CODE_IF_ON(
    DEBUG_PRINT_INDENT();
    if (ppszInternalCommands) {
      printf("GetComspecCommands(); // return {");
      for (i=0; i<nCmd; i++) printf("%s%s", i?" ":"", ppszInternalCommands[i]);
      printf("}\n");
    } else {
      printf("GetComspecCommands(); // return NULL\n");
    }
  )
  return ppszInternalCommands;
}

#if defined(_MSC_VER)
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif

int SearchCommandInternal(char *pszCommand, int iFlags) {
  GetComspecCommands();
  /* TODO: Implement this */
  /* Gotcha: Some versions of DOS have a help.exe or help.com command,
     others don't. This is the case of DOS 7 in Windows 98. */
  /* http://thestarman.pcministry.com/DOS/DOS7INT.htm */
  /* Idea: Run system("ihelp.bat %COMMAND%") with ihelp.bat:
       set PATH=
       %1 /?
     Clearing the PATH ensures that only internal commands can do anything.
     I tried testing the errorlevel, but this does not work.
     Instead, check if anything is written to stderr.
  */
  return FALSE;
}

#if defined(_MSC_VER)
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */
#endif

#endif /* defined(_MSDOS) || defined(_WIN32) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    SearchProgramWithAnyExt				      |
|									      |
|   Description     Search for a given command in a directory          	      |
|									      |
|   Arguments	    pszPath		Directory to search in		      |
|		    pszCommand		Program name to search for	      |
|		    iFlags		Search flags			      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int SearchProgramWithAnyExt(char *pszPath, char *pszCommand, int iFlags) {
  int iFound = FALSE;
  int i;

#ifndef __unix__
  if (strchr(pszCommand, '.')) {
#endif
    iFound |= SearchProgramWithOneExt(pszPath, pszCommand, NULL, iFlags);
#ifndef __unix__
  }
#endif
  if (!initExtListDone) initExtList();
  for (i=0; ppszExt[i]; i++) {
    iFound |= SearchProgramWithOneExt(pszPath, pszCommand, ppszExt[i], iFlags);
    if (iFound && !(iFlags & WHICH_ALL)) return iFound;
  }

  return iFound;
}

int SearchProgramWithOneExt(char *pszPath, char *pszCommand, char *pszExt, int iFlags) {
  char szFname[FILENAME_MAX];
  int nChars = 0;

  _makepath(szFname, "", pszPath, pszCommand, pszExt);
  DEBUG_PRINTF(("  Looking for \"%s\"", szFname));
  if (!access(szFname, F_OK)) {
    char *pszName = szFname;
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
    char szName2[FILENAME_MAX];
#endif
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
    char target[PATH_MAX];
#endif
    int iExecutable;

    DEBUG_PRINTF(("  Matched with errno %d\n", errno));
#if defined(_WIN32)
    FixNameCase(pszName);
#endif
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
    if (iWoW) { /* This is a WIN32 app running on WIN64 */
      strnirepl(szName2, sizeof(szName2), pszName, szSystem64dir, szSystem32dir);
      pszName = szName2;
    }
#endif
    iExecutable = !access(szFname, X_OK); /* access() returns 0=success, -1=error */
    if (iFlags & WHICH_XCD) iExecutable = FALSE; /* CD not in PATH, so actually not executable */
    if (!iExecutable) {
      if (!(iFlags & WHICH_VERBOSE)) goto search_failed;
      nChars += printf("# "); /* Display a comment showing the file, and why it's excluded */
    }

    if (iFlags & WHICH_LONG) {
      struct stat s;
      struct tm *pTime;
      int year, month, day, hour, minute, second;

      /* Get the time stamp of the actual file, not that of the link! */
      if (stat(pszName, &s) == -1) goto search_failed;
      pTime = localtime(&(s.st_mtime)); /* Time of last data modification */
      year = pTime->tm_year + 1900;
      month = pTime->tm_mon + 1;
      day = pTime->tm_mday;
      hour = pTime->tm_hour;
      minute = pTime->tm_min;
      second = pTime->tm_sec;
      nChars += printf("%04d-%02d-%02d %02d:%02d:%02d ", year, month, day, hour, minute, second);
    }
    nChars += printf("%s", pszName);
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
    if (iFlags & WHICH_LONG) {
      struct stat s;
      /* Check if it's a link */
      if (lstat(pszName, &s) == -1) goto search_failed;
      if (S_ISLNK(s.st_mode)) { /* Yes, it's a link */
      	if (readlink(pszName, target, PATH_MAX) == -1) goto search_failed;
	nChars += printf(" -> %s", target); 
      }
    }
#endif
    if (!iExecutable) { /* Display a comment showing why it was excluded */
      if (iFlags & WHICH_XCD) {
      	nChars += printf(" # %s does not search in \".\"", pszShells[shell]);
      } else {
      	nChars += printf(" # Not executable");
      }
      goto search_failed;
    }
    printf("\n");
    return TRUE;	/* Match */
  }
search_failed:
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (nChars) printf("\n"); /* nChars cannot be > 0 for cases this is compiled out (MSDOS) */
#endif
  DEBUG_PRINTF(("  Error %d\n", errno));
  return FALSE;	/* No match */
}

#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strnirepl						      |
|									      |
|   Description:    Case-insensitive string replacement			      |
|									      |
|   Parameters:     char *pszResultBuffer   Output buffer       	      |
|		    size_t lResultBuffer    Output buffer size  	      |
|		    const char *pszString   Source string to copy from	      |
|		    const char *pszSearch   String to search in pszString     |
|		    const char *pszReplace  String to write instead	      |
|									      |
|   Returns:	    The length of the output string.			      |
|									      |
|   History:								      |
|    2012-02-02 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace) {
  size_t lSearch = strlen(pszSearch);
  size_t lReplace = strlen(pszReplace);
  size_t lRemain = lResultBuffer;
  const char *pszIn = pszString;
  char *pszOut = pszResultBuffer;
  if (!lRemain) return 0; /* Prevent catastrophies */
  lRemain -= 1; /* Leave room for the final NUL */
  while (*pszIn && lRemain) {
    if (strnieq(pszIn, pszSearch, lSearch)) {
      strncpy(pszOut, pszReplace, lRemain);
      pszIn += lSearch;
      if (lReplace < lRemain) {
	pszOut += lReplace;
      	lRemain -= lReplace;
      } else {
	pszOut += lRemain;
      	lRemain = 0;
      }
      continue;
    }
    *(pszOut++) = *(pszIn++);
    lRemain -= 1;
  }
  *pszOut = '\0';
  return pszOut - pszResultBuffer;
}

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProcessName					      |
|									      |
|   Description     Get the parent process name                    	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2013-03-22 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)
#include <windows.h>
#pragma pack(push,8) /* Work around a bug in tlhelp32.h in WIN64, which generates the wrong structure if packing has been changed */
#include <tlhelp32.h>
#pragma pack(pop)

int GetProcessName(pid_t pid, char *name, size_t lname) {
  size_t len = 0;
  HANDLE h;
  BOOL bFound;
  PROCESSENTRY32 pe = {0};

  h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (h == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to get list of processes\n");
    return 0;
  }

  pe.dwSize = sizeof(PROCESSENTRY32);
  for (bFound=Process32First(h, &pe); bFound; bFound=Process32Next(h, &pe)) {
    if ((pid_t)(pe.th32ProcessID) == pid) {
      len = strlen(pe.szExeFile);
      if (lname <= len) return -(int)len; /* Not enough room in the output buffer */
      strcpy(name, pe.szExeFile);
      break;
    }
  }

  CloseHandle(h);

  return (int)len;
}

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

#if defined(_WIN32)

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
	pszPath = szRootDir; /* Use the "C:\\" copy on the stack to make sure the routine is reentrant */
	pszPath[0] = pszPathname[0];
      } else { /* Just a root directory name */
	pszPath = "\\";
      }
      pszName += 1;
    }
  } else { /* No path separator */
    pszName = pszPathname;
    if (lDrive) { /* A drive letter, then a file name */
      pszPath = szDriveCurDir; /* Use the "C:." copy on the stack to make sure the routine is reentrant */
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
    RETURN_BOOL_COMMENT(FALSE, ("Can't open directory \"%s\"\n", pszPath));
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

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    SearchCmdAliases					      |
|									      |
|   Description     Search cmd.exe internal commands and aliases from stdin   |
|									      |
|   Returns:	    TRUE if found, false if not				      |
|									      |
|   Notes	    Read the list once, and cache it in global variables.     |
|		    							      |
|   History								      |
|    2019-02-17 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)

/* static char **ppszCmdInternals = NULL;	// cmd.exe internal commands */
static char **ppszCmdAliases = NULL;		// doskey.exe macros
static int nInternalCommands = 0;
static int nAliases = 0;
static char *pszWindir = NULL;

int SearchCmdAliases(char *pszCommand, int iSearchFlags) {
  char szLine[512];
  int i, l;
  int iFound = FALSE;

  if ((!ppszCmdInternals) && (!ppszCmdAliases)) {
    ppszCmdInternals = malloc(sizeof(char *));
    if (!ppszCmdInternals) goto cleanup_and_return;
    ppszCmdInternals[0] = NULL;
    ppszCmdAliases = malloc(sizeof(char *));
    if (!ppszCmdAliases) goto cleanup_and_return;
    ppszCmdAliases[0] = NULL;
    pszWindir = getenv("windir");
    while (fgets(szLine, sizeof(szLine), stdin) != NULL) {
      int isInternal = FALSE;
      int isMacro = FALSE;
      szLine[sizeof(szLine) - 1] = '\0';
      l = (int)strlen(szLine);
      for (; l && ((szLine[l-1]=='\r') || (szLine[l-1]=='\n')); ) szLine[--l] = '\0'; /* Trim trailing CR/LF */
      if (l) isInternal = TRUE;
      /* Check if this is an internal command from HELP, or a dowkey macro, or none */
      for (i=0; i<l; i++) {
      	char c = szLine[i];
      	if ((c == ' ') || (c == '\t')) {
      	  if (i == 0) isInternal = FALSE;
      	  break;
      	}
      	if (c == '=') {
      	  isInternal = FALSE;
      	  if (i > 0) isMacro = TRUE;
      	  break;
      	}
      	if ((c < 'A') || ((c > 'Z') && (c < 'a')) || (c > 'z')) {
      	  isInternal = FALSE;
      	  isMacro = FALSE;
      	  break;
      	}
      	if (c > 'Z') {
      	  isInternal = FALSE;
      	}
      }
      if (isInternal) {
      	char buf[256];
	szLine[i] = '\0';
      	sprintf(buf, "%s\\System32\\%s.exe", pszWindir, szLine);
      	// printf("%s\n", buf);
      	// TODO: Known issue: bcdedit is not detected as external, yet bcdedit.exe is there!
	if (access(buf, X_OK) == 0) continue; /* It's actually an external command */
      	sprintf(buf, "%s\\System32\\%s.com", pszWindir, szLine);
	if (access(buf, X_OK) != -1) continue; /* It's actually an external command */
      	ppszCmdInternals = realloc(ppszCmdInternals, (nInternalCommands + 2) * sizeof(char *));
	if (!ppszCmdInternals) goto cleanup_and_return;
	ppszCmdInternals[nInternalCommands++] = strdup(szLine);
	ppszCmdInternals[nInternalCommands] = NULL;
      } else if (isMacro) {
      	char *pszLine = strdup(szLine);
	pszLine[i] = '\0';
       	ppszCmdAliases = realloc(ppszCmdAliases, (nAliases + 2) * sizeof(char *));
	if (!ppszCmdAliases) goto cleanup_and_return;
	ppszCmdAliases[nAliases++] = pszLine;
	ppszCmdAliases[nAliases] = NULL;
      }
    }
  }
  /* First check aliases */
  for (i=0; i<nAliases; i++) {
    if (!strcasecmp(ppszCmdAliases[i], pszCommand)) {
      char *pszMacro = ppszCmdAliases[i];
      char *pszDefinition = pszMacro + strlen(pszMacro) + 1;
      printf("doskey /macros %s=%s\n", pszMacro, pszDefinition);
      iFound = TRUE;
      if (!(iSearchFlags & WHICH_ALL)) return iFound;
      break;
    }
  }
  /* Then check internal commands */
  for (i=0; i<nInternalCommands; i++) {
    if (!strcasecmp(ppszCmdInternals[i], pszCommand)) {
      printf("cmd /c %s\n", ppszCmdInternals[i]);
      iFound = TRUE;
      if (!(iSearchFlags & WHICH_ALL)) return iFound;
      break;
    }
  }
  /* Keep searching */
  return iFound;

cleanup_and_return:
  return -1;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    SearchPowerShellAliases				      |
|									      |
|   Description     Search PowerShell functions and aliases from stdin	      |
|									      |
|   Returns:	    TRUE if found, false if not				      |
|									      |
|   Notes	    Read the list once, and cache it in global variables.     |
|		    							      |
|   History								      |
|    2019-02-20 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

static char **ppszTypes = NULL;		// PowerShell object types
static char **ppszNames = NULL;		// PowerShell object names
static char **ppszValues = NULL;	// PowerShell object values
static int nObjects = 0;

int SearchPowerShellAliases(char *pszCommand, int iSearchFlags) {
  char szLine[512];
  int i, l;
  int iFound = FALSE;

  if ((!ppszTypes) && (!ppszNames)) {
    ppszTypes = malloc(sizeof(char *));
    if (!ppszTypes) goto cleanup_and_return;
    ppszTypes[0] = NULL;
    ppszNames = malloc(sizeof(char *));
    if (!ppszNames) goto cleanup_and_return;
    ppszNames[0] = NULL;
    ppszValues = malloc(sizeof(char *));
    if (!ppszValues) goto cleanup_and_return;
    ppszValues[0] = NULL;
    
    while (fgets(szLine, sizeof(szLine), stdin) != NULL) {
      char *pc;
      char *pszType;
      char *pszValue;
      char *pszName;

      szLine[sizeof(szLine) - 1] = '\0';
      l = (int)strlen(szLine);
      for (; l && ((szLine[l-1]=='\r') || (szLine[l-1]=='\n')); ) szLine[--l] = '\0'; /* Trim trailing CR/LF */
      /* Check the object type */
      for (pc=szLine; (*pc == ' ') || (*pc == '\t'); pc++) ; // Skip head spaces
      pszType = pc;
      for (; *pc && (*pc != ' ') && (*pc != '\t'); pc++) ; // Skip type chars
      *(pc++) = '\0';
      if (!*pszType) continue;
      if (strcmp(pszType, "Function") && strcmp(pszType, "Alias")) continue;
      pszType = strdup(pszType);
      if (!pszType) goto cleanup_and_return;
      for (; (*pc == ' ') || (*pc == '\t'); pc++) ; // Skip mid spaces
      pszValue = pc;
      if (!pszValue) goto cleanup_and_return;
      for (; pc[0] && (   (         (pc[0] != ' ') && (pc[0] != '\t'))
      	               || (pc[1] && (pc[1] != ' ') && (pc[1] != '\t'))); pc++) ; // Skip value chars
      *(pc++) = '\0';
      if (!*pszValue) continue;
      pszValue = strdup(pszValue);
      if (!pszValue) goto cleanup_and_return;

      pszName = strdup(pszValue);
      if (!pszName) goto cleanup_and_return;
      pszName = strtok(pszName, " \t");
      // printf("pszType=\"%s\"; pszName=\"%s\"; pszValue=\"%s\"\n", pszType, pszName, pszValue);

      ppszTypes = realloc(ppszTypes, (nObjects + 2) * sizeof(char *));
      if (!ppszTypes) goto cleanup_and_return;
      ppszTypes[nObjects] = pszType;
      ppszTypes[nObjects+1] = NULL;

      ppszNames = realloc(ppszNames, (nObjects + 2) * sizeof(char *));
      if (!ppszNames) goto cleanup_and_return;
      ppszNames[nObjects] = pszName;
      ppszNames[nObjects+1] = NULL;

      ppszValues = realloc(ppszValues, (nObjects + 2) * sizeof(char *));
      if (!ppszValues) goto cleanup_and_return;
      ppszValues[nObjects] = pszValue;
      ppszValues[nObjects+1] = NULL;

      nObjects += 1;
    }
  }

  for (i=0; i<nObjects; i++) {
    if (!_stricmp(ppszNames[i], pszCommand)) {
      printf("%s %s\n", ppszTypes[i], ppszValues[i]);
      iFound = TRUE;
      if (!(iSearchFlags & WHICH_ALL)) return iFound;
    }
  }

  /* Keep searching */
  return iFound;

cleanup_and_return:
  return -1;
}

#endif // defined(_WIN32)
