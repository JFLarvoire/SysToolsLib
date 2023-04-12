/*****************************************************************************\
*                                                                             *
*   Filename	    In.c						      *
*									      *
*   Description     Execute a command in a given directory, then come back    *
*									      *
*   Notes	    Named "in.exe" in Windows, and "In" in Unix.	      *
*		    The latter named is capitalized because 'in' is a	      *
*		    reserved keyword in Unix shells.			      *
*		    							      *
*   History								      *
*    2023-01-03 JFL jf.larvoire@free.fr created this program.                 *
*    2023-03-27 JFL For DOS & Windows, copy the actual command line instead   *
*                   of rebuilding it. This fixes issues with the complex      *
*                   escaping and quoting rules in DOS & Windows cmd shells.   *
*    2023-04-12 JFL Moved CondQuoteShellArg() to SysLib, and some code in     *
*                   main to new SysLib routine DupArgLineTail().              *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Execute a command in a given directory, then come back"
#define PROGRAM_NAME    "in"
#define PROGRAM_VERSION "1.1"
#define PROGRAM_DATE    "2023-04-12"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
/* SysToolsLib include files */
#include "mainutil.h"	/* SysLib goodies for the main routine */
#include "CmdLine.h"	/* SysLib routines managing command-line arguments */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     Main program routine				      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS.			      |
|									      |
|   Notes								      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

char *pszUsage =
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: in [SWITCHES] DIRECTORY [do] COMMAND [ARGUMENTS]\n\
\n\
Switches:\n\
  -?|-h    Display this help message and exit\n\
"
#ifdef _DEBUG
"\
  -d       Output debug information\n"
#endif
"\
  -V       Display the program version and exit\n\
  -X       Display the equivalent commands, but don't run them\n\
"
#include "footnote.h"
;

int main(int argc, char *argv[]) {
  int i;
  int iArg0;
  int iErr;
  int iRet = 0;
  int iExec = TRUE;
  char szCD[PATH_MAX];
  char *pszDir = NULL;
  char *pszCmd = NULL;
  char *pszCmdLine = NULL;
  char *pszQuoted;
  int iGotDO = FALSE;

  /* Parse the command line */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if ((!iGotDO) && IsSwitch(arg)) { /* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "?") || streq(opt, "h")) {
	fputs(pszUsage, stdout);
	return 0;
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_ON();
	  continue;
	}
      )
      if (streq(opt, "V")) {	    /* -V: Display the version */
	puts(DETAILED_VERSION);
	return 0;
      }
      if (streq(opt, "X")) {	    /* -X: No-exec */
	iExec = FALSE;
	continue;
      }
      pferror("Warning: Unrecognized switch %s ignored", arg);
      continue;
    }
    if (!pszDir) {
      pszDir = arg;
      continue;
    }
    if ((!iGotDO) && strieq(arg, "do")) {
      iGotDO = TRUE;
      continue;
    }
    pszCmd = arg;
    break;
  }
  if (!pszCmd) {   /* If there is no command line, exit immediately */
    pferror("Arguments missing. Run `in -?` to get help");
    return 1;
  }
  iArg0 = i;

  /* Save the initial directory */
  if (!getcwd(szCD, PATH_MAX)) {
    pferror("Cannot get the current directory: %s", strerror(errno));
    return 1;
  }

  /* Enter the requested directory */
  if (iExec) {
    iErr = chdir(pszDir);
    if (iErr) {
      pferror("Cannot enter %s: %s", pszDir, strerror(errno));
      return 1;
    }
  } else {
    pszQuoted = CondQuoteShellArg(pszDir);
    if (!pszQuoted) {
      pferror("Not enough memory");
      return 1;
    }
    printf("pushd %s\n", pszQuoted);
    free(pszQuoted);
  }

  /* Build the child command line */
  pszCmdLine = DupArgLineTail(argc, argv, iArg0);
  if (!pszCmdLine) {
    pferror("Cannot rebuild the argument line: %s", strerror(errno));
    return 1;
  }

  /* Run the command */
  if (iExec) {
    iRet = system(pszCmdLine);
  } else {
    printf("%s\n", pszCmdLine);
  }

  /* Return to the initial directory */
  if (iExec) {
    iErr = chdir(szCD);
    if (iErr) {
      pferror("Cannot enter %s: %s", pszDir, strerror(errno));
      return 1;
    }
  } else {
    printf("popd\n");
  }

  return iRet;
}

