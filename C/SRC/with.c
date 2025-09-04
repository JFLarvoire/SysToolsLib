/*****************************************************************************\
*                                                                             *
*   Filename	    with.c						      *
*									      *
*   Description     Run a command with specific environment variables         *
*									      *
*   Notes	    Intended for DOS and Windows cmd shells, which do not     *
*		    have that ability buit-in, contrary to Unix shells.       *
*		    							      *
*   History								      *
*    2023-01-09 JFL jf.larvoire@free.fr created this program.                 *
*    2023-04-12 JFL For DOS & Windows, copy the actual command line instead   *
*                   of rebuilding it. This fixes issues with the complex      *
*                   escaping and quoting rules in DOS & Windows cmd shells.   *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Run a command with specific environment variables"
#define PROGRAM_NAME    "with"
#define PROGRAM_VERSION "1.1"
#define PROGRAM_DATE    "2023-04-12"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/*
#include <string.h>
*/
#include <errno.h>
#include <ctype.h>
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
Usage: with [SWITCHES] [VAR=VALUE ...] [do] COMMAND [ARGUMENTS]\n\
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
  char *pszCmd = NULL;
  char *pszCmdLine = NULL;
  int iGotDO = FALSE;
  int nVars = 0;

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
    if ((!iGotDO) && strchr(arg, '=')) { /* It's a variable definition */
      if (iExec) {
	iErr = putenv(arg);
	if (iErr) {
	  pferror("Cannot set %s: %s", arg, strerror(errno));
	  return 1;
	}
      } else {
      	if (!nVars) printf("setlocal\n");
	printf("set \"%s\"\n", arg);
      }
      nVars += 1;
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
    pferror("Command missing. Run `with -?` to get help");
    return 1;
  }
  iArg0 = i;

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
    if (nVars) printf("endlocal\n");
  }

  return iRet;
}

