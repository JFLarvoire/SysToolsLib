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
*    2025-11-04 JFL Added a note in the help message for Windows, about       *
*		    Windows limitations with long paths > 260 characters.     *
*    2025-11-11 JFL Added option -@ to get the target dir. name from a file.  *
*                   Necessary for DOS, to enter directories with a name too   *
*		    long to fit on the 128-bytes command line.		      *
*    2025-11-27 JFL In the Unix version, update the PWD environment variable  *
*                   with the new logical directory.                           *
*    2025-12-02 JFL Moved that fix to SysLib's new ChDir() routine.	      *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Execute a command in a given directory, then come back"
#define PROGRAM_NAME    "in"
#define PROGRAM_VERSION "1.2.1"
#define PROGRAM_DATE    "2025-12-02"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
/* SysLib include files */
#include "pathnames.h"
/* SysToolsLib include files */
#include "mainutil.h"	/* SysLib goodies for the main routine */
#include "CmdLine.h"	/* SysLib routines managing command-line arguments */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

char *GetLineFromFile(const char *pszPathName);

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
  -@ FILE  Get the target directory name from that file\n\
"
#ifdef _DEBUG
"\
  -d       Output debug information\n"
#endif
"\
  -V       Display the program version and exit\n\
  -X       Display the equivalent commands, but don't run them\n\
"
#ifdef _WIN32
"\n\
Known limitation with long pathnames > 260 characters: Windows versions up to 8\n\
cannot change the current directory to such long pathnames. Windows ≥ 10 can,\n\
but only if long file name support has been enabled in the registry. And even\n\
in this case, it cannot run a command below that 260 characters threshold.\n\
in.exe will appear to succeed when requested to run a command in such a deep\n\
directory; But that command will actually be run in a parent directory of the\n\
requested one, with the largest path that fits in 260 characters.\n\
"
#endif
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
      if (streq(opt, "@")) {	    /* -@: Get target dir from an input file */
      	if (((i+1)<argc) && !IsSwitch(argv[i+1])) {
	  pszDir = GetLineFromFile(arg = argv[++i]);
	  if (!pszDir) {
	    pferror("Cannot access \"%s\". %s", arg, strerror(errno));
	    return 1;
	  }
	} else {
	  pferror("Missing input file name after -@");
	  return 1;
	}
	continue;
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
      fprintf(stderr, "Warning: Unrecognized switch %s ignored\n", arg);
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
    /* In Unix, chdir()=SysLib's ChDir(), which also updates the PWD env var */
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
    /* Using the system() command ensures that this works in all OSs, searches
       the PATH automatically, and invokes script interpreters as needed. */
    /* 2025-11-01 In Windows, this is known _not_ to work in directories with
       long path names > 260 characters. After trying many alternatives, and
       searching for explanations on the Internet, it seems that this is
       a limitation of Windows' CreateProcess() routine. Contrary to most
       file management routines, this one has _not_ been updated to support
       current directories longer than PATH_MAX. Even in Windows 11 2025H2
       with the registry entry set, the application manifest updated, the
       \\?\ prefix used, the %=C:% environment variable set, etc. 
       In some cases, the current directory is truncated. In other cases, the
       execution of the command fails, with "Invalid directory name" errors.
       See the WIN32 API long pathname support doc:
       https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
     */       
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetLineFromFile					      |
|									      |
|   Description     Get one line from an input file			      |
|									      |
|   Parameters	    const char *pszPathName	The file pathname	      |
|									      |
|   Returns	    A pointer to the allocated string with the input line.    |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    2025-11-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

/* We want to be able to test paths up to and beyond the OS' PATH_MAX */
#define PATH_LINE_MAX (PATH_MAX + 256)

char *GetLineFromFile(const char *pszPathName) {
  FILE *f = NULL;
  char *buf = NULL;
  char *pszLine = NULL;
  int iLen;

  buf = malloc(PATH_LINE_MAX);
  if (!buf) goto cleanup_and_return; /* Out of memory for buffer */

  f = fopen(pszPathName, "r");
  if (!f) goto cleanup_and_return; /* Failed to open the input file */

  pszLine = fgets(buf, PATH_LINE_MAX, f);
  if (!pszLine) goto cleanup_and_return; /* Failed to read input file */

  iLen = (int)strlen(pszLine);
  /* trim the trailing \n if any */
  if (iLen && (pszLine[iLen-1] == '\n')) pszLine[--iLen] = '\0';

  pszLine = realloc(buf, iLen+1);
  if (!pszLine) pszLine = buf;

cleanup_and_return:
  if (f) fclose(f);
  if (!pszLine) free(buf); /* Free the buffer if we don't return it */
  return pszLine;
}

