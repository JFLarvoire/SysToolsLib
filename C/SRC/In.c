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
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Execute a command in a given directory, then come back"
#define PROGRAM_NAME    "in"
#define PROGRAM_VERSION "1.0"
#define PROGRAM_DATE    "2023-01-09"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
/* SysToolsLib include files */
#include "mainutil.h"	/* SysLib goodies for the main routine */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX
#endif

/* Prototypes */
char *CondQuoteShellArg(char *pszString);

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
  int iLen;
  char szCD[PATH_MAX];
  char *pszDir = NULL;
  char *pszCmd = NULL;
  char *pszCmdLine = NULL;
  char *pszQuoted;
  int iGotDO = FALSE;
  int n;

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
    if (!pszQuoted) goto out_of_memory;
    printf("pushd %s\n", pszQuoted);
    free(pszQuoted);
  }

  /* Allocate a buffer large enough for rebuilding the command line */
  iLen = 1; /* 1 byte for the final NUL */
  for (i=iArg0; i<argc; i++) iLen += (2 * (int)strlen(argv[i])) + 2; /* Worst case for the cmd & every arg */
  pszCmdLine = malloc(iLen);
  if (!pszCmdLine) {
out_of_memory:
    pferror("Not enough memory");
    return 1;
  }
  /* Rebuild the command line */
#if defined(_MSDOS) || defined(_WIN32)
  {
  char c0 = pszCmd[0];		  /* In cmd.exe, a leading @ means "echo off" */
  if (c0 == '@') pszCmd[0] = ' '; /* In that case, force quoting the command */
#endif
  pszQuoted = CondQuoteShellArg(pszCmd);
  if (!pszQuoted) goto out_of_memory;
#if defined(_MSDOS) || defined(_WIN32)
  if (c0 == '@') pszQuoted[1] = c0;
  }
#endif
  n = sprintf(pszCmdLine, "%s", pszQuoted);
  free(pszQuoted);
  for (i=iArg0+1; i<argc; i++) {
    pszQuoted = CondQuoteShellArg(argv[i]);
    if (!pszQuoted) goto out_of_memory;
    n += sprintf(pszCmdLine+n, " %s", pszQuoted);
    free(pszQuoted);
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    CondQuoteShellArg					      |
|									      |
|   Description     Conditionally quote a string, for use as a shell argument |
|									      |
|   Returns	    A copy of the string, quoted and escaped as needed	      |
|									      |
|   Notes	    For Windows, assume it'll be passed to another C program, |
|		    which in turn will remove escape characters ahead of " :  |
|		    https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
|		    							      |
|   History								      |
|    2023-01-07 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_MSDOS) || defined(_WIN32)

char *CondQuoteShellArg(char *pszArg) {
  char szNeedQuote[] = " \t&|()<>^";
  int i;
  int iNeedQuote;
  int iNeedEscape;
  char c;
  int iLen = (int)strlen(pszArg);

  iNeedQuote = (iLen == 0);
  if (!iNeedQuote) for (i=0; (c = szNeedQuote[i]) != '\0'; i++) {
    if (strchr(pszArg, c)) {
      iNeedQuote = TRUE;
      break;
    }
  }
  iNeedEscape = (strchr(pszArg, '"') != NULL);
  if (iNeedQuote || iNeedEscape) {
    int iIn = 0;
    int iOut = 0;
    char *pszQuotedArg = malloc(2*iLen + 3);
    if (!pszQuotedArg) return NULL;
    if (iNeedQuote) pszQuotedArg[iOut++] = '"';
    while ((c = pszArg[iIn++]) != '\0') {
      if (c == '"') { /* Double every \ that immediately precedes the " */
      	for (i=iIn-2; (i >= 0) && (pszArg[i] == '\\'); i--) pszQuotedArg[iOut++] = '\\';
      	pszQuotedArg[iOut++] = '\\'; /* Then add a \ to escape the " itself */
      }
      pszQuotedArg[iOut++] = c;
    }
    if (iNeedQuote) pszQuotedArg[iOut++] = '"';
    pszQuotedArg[iOut++] = '\0';
    return ShrinkBuf(pszQuotedArg, iOut);
  } else {
    return strdup(pszArg);
  }
}

#endif

#if defined(_UNIX)

char *CondQuoteShellArg(char *pszArg) {
  /* Lists based on https://stackoverflow.com/a/27817504/2215591 */
  char szNeedQuote[] = " #&'()*,;<>?[]^{|}~";
  char szNeedEscape[] = "\\\"$`!";
  int iLen = (int)strlen(pszArg);
  int iNeedQuote = (iLen == 0);
  int iNeedEscape = FALSE;
  int iHasControl = FALSE;
  int i;
  char c;

  if (!iNeedQuote) for (i=0; (c = szNeedQuote[i]) != '\0'; i++) {
    if (strchr(pszArg, c)) {
      iNeedQuote = TRUE;
      break;
    }
  }
  for (i=0; (c = szNeedEscape[i]) != '\0'; i++) {
    if (strchr(pszArg, c) || iscntrl(c)) {
      iNeedEscape = TRUE;
      break;
    }
  }
  if (strchr(pszArg, '\x7F')) iHasControl = TRUE;
  if (!iHasControl) for (i=0; i<iLen; i++) {
    if (pszArg[i] < ' ') {
      iHasControl = TRUE;
      break;
    }
  }
  if (iHasControl) { /* If there's any control character, use the $'string' format */
    int iIn = 0;
    int iOut = 0;
    char *pszQuotedArg = malloc(4*iLen + 4);
    if (!pszQuotedArg) return NULL;
    pszQuotedArg[iOut++] = '$';
    pszQuotedArg[iOut++] = '\'';
    while ((c = pszArg[iIn++]) != '\0') {
      if (strchr("\a\b\t\n\v\f\r\e", c)) {
      	pszQuotedArg[iOut++] = '\\';
	if (c == '\a') c = 'a';
	if (c == '\b') c = 'b';
	if (c == '\t') c = 't';
	if (c == '\n') c = 'n';
	if (c == '\v') c = 'v';
	if (c == '\f') c = 'f';
	if (c == '\r') c = 'r';
	if (c == '\e') c = 'e';
        pszQuotedArg[iOut++] = c;
      } else if (iscntrl(c)) { /* Other control character that do not have a C-style escape sequence */
      	iOut += sprintf(pszQuotedArg+iOut, "\\%03o", c);
      } else {
	if (strchr(szNeedEscape, c)) pszQuotedArg[iOut++] = '\\';
        pszQuotedArg[iOut++] = c;
      }
    }
    pszQuotedArg[iOut++] = '\'';
    pszQuotedArg[iOut++] = '\0';
    return ShrinkBuf(pszQuotedArg, iOut);
  } else if ((iNeedQuote || iNeedEscape) && !strchr(pszArg, '\'')) { /* The simplest is to use the 'string' format */
    char *pszQuotedArg = malloc(iLen + 3);
    if (!pszQuotedArg) return NULL;
    sprintf(pszQuotedArg, "'%s'", pszArg);
    return pszQuotedArg;
  } else if (iNeedQuote || iNeedEscape) { /* Else use the "string" format */
    int iIn = 0;
    int iOut = 0;
    char *pszQuotedArg = malloc(2*iLen + 3);
    if (!pszQuotedArg) return NULL;
    if (iNeedQuote) pszQuotedArg[iOut++] = '"';
    while ((c = pszArg[iIn++]) != '\0') {
      if (strchr(szNeedEscape, c)) pszQuotedArg[iOut++] = '\\';
      pszQuotedArg[iOut++] = c;
    }
    if (iNeedQuote) pszQuotedArg[iOut++] = '"';
    pszQuotedArg[iOut++] = '\0';
    return ShrinkBuf(pszQuotedArg, iOut);
  } else {	/* Use the string as it is */
    return strdup(pszArg);
  }
}

#endif
