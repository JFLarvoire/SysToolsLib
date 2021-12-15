/*****************************************************************************\
*                                                                             *
*   File name	    pferror.c						      *
*                                                                             *
*   Description	    Print a formatted error message			      *
*                                                                             *
*   Notes	    A generalized version of perror().                        *
*                                                                             *
*   History                                                                   *
*    2021-12-15 JFL Created this file.					      *
*                                                                             *
\*****************************************************************************/

#include <stdio.h>
#include <stdarg.h>

#include "mainutil.h"	/* This module definitions */

/* Print error messages on stderr, in a standardized format */
int pferror(char *pszFormat, ...) {
  va_list vl;
  int n = fprintf(stderr, "Error: ");
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, "\n");
  va_end(vl);
  return n;
}

