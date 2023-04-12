/*****************************************************************************\
*                                                                             *
*   Filename	    CondQuoteShellArg.c					      *
*									      *
*   Description     Conditionally quote a string, for use as a shell argument *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2023-04-12 JFL jf.larvoire@free.fr extracted this routine from in.c.     *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _GNU_SOURCE          /* ISO C, POSIX, BSD, and GNU extensions */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "CmdLine.h"	/* SysLib routines managing command-line arguments */

#include "debugm.h"	/* SysToolsLib debug macros */

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX
#endif

#define TRUE 1
#define FALSE 0

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
