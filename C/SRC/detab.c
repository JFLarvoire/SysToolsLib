/*****************************************************************************\
*                                                                             *
*   Filename	    detab.c                                                   *
*                                                                             *
*   Description	    Convert tabs to spaces                                    *
*                                                                             *
*   Notes	    Adapted from Software Tools, by Kernighan and Plauger     *
*                                                                             *
*   History                                                                   *
*    1984-01-14 MB  Written by Michael Burton. Last Update: 14 Jan 1984       *
*    1984-07    JW  Converted to Microsoft C, debugged and improved by        *
*		    Jack Wright  7/84	                                      *
*    1986-12=15 JFL Default spacing of 4 for .C files                         *
*    1987-01-26 JFL Possibility to append to the dest. file                   *
*    2010-11-22 JFL Changed back to 8 spaces per tab for all files.           *
*		    Updated for modern C compilers.                           *
*		    Default input: stdin; Default output: stdout.	      *
*    2012-10-18 JFL Added the option to use - to specify stdin or stdout.     *
*		    Added authors names in the help.                          *
*                   Display the OS name.                                      *
*                   Version 2.1.                                              *
*    2012-10-18 JFL Added build for Unix.                                     *
*                   Version 2.1.1.                                            *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.1.1"
#define PROGRAM_DATE    "2014-12-05"

#define _CRT_SECURE_NO_WARNINGS /* Prevent security warnings for old routines */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */
#define OS_NAME "DOS"
#endif /* defined(_MSDOS) */

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */
#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif
#endif /* defined(_WIN32) */

#ifdef __unix__     // Unix
#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif
#endif /* defined(__unix__) */

/* Global variables */
char *mode = "w";		/* Destination file access mode */
char usage[] = "\
detab version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME " - Convert tabs to spaces\n\
\n\
Usage: detab [OPTIONS] [INFILE [OUTFILE [N]]]\n\
\n\
Options:\n\
  -a        Append a form feed and the output to the destination file.\n\
  -t N      Number of columns between tab stops. Default: 8\n\
\n\
Arguments:\n\
  INFILE    Input file. Default or -: stdin\n\
  OUTFILE   Output file. Default or -: stdout\n\
  N         Number of columns between tab stops. Default: 8\n\
\n\
Authors: Michael Burton, Jack Wright, Jean-Francois Larvoire\n\
";

int main(int argc, char *argv[])
  {
  int col=1, n=8;
  int c;
  FILE *hFrom = stdin;
  FILE *hTo = stdout;
  int narg;			/* Argument scanned number (1 to x) */
  int nargl;			/* Logical argument number (1 to 3) */
  char *arg;			/* Argument scanned */

  for (narg = 1, nargl = 1; narg < argc; narg++)
    {
    arg = argv[narg];
    if ((arg[0] == '-') || (arg[0] == '/'))
      {					/* It's a switch */
      switch (arg[1])
	{
	case '\0':			/* Use - for stdin or stdout */
	  switch (nargl) {
	  case 1: hFrom = stdin; nargl += 1; break;
	  case 2: hTo = stdout; nargl += 1; break;
	  default: break;
	  }
	  break;
	case '?':
	  fputs(usage, stdout);
	  return 0;
	case 'a':			/* Use append mode */
	case 'A':
	  mode = "a";
	  break;
	case 't':			/* Set tab width */
	case 'T':
	  n = atoi(arg);
	  break;
	default:
	  fprintf(stdout, "Invalid switch %s\x07\n", arg);
	  break;
	}
      }
    else				/* It's not a switch */
      {
      switch (nargl)
	{
	case 1:				/* Open first file in read mode */
	  if ((hFrom = fopen(arg, "r")) == 0)
	    {
	    fprintf(stdout, "%s not found\x07\n", arg);
	    return 1;
	    }
#if 0	  /* JFL 2010-11-22 Removed as modern editors don't require changing tab width to adjust indent width. */
	  l = strlen(arg);		/* If it's a .C file ... */
	  if ( (arg[l-2] == '.') && (tolower(arg[l-1]) == 'c') )
	    n = 4;				/* ... default tab interval = 4 */
#endif
	  nargl += 1;			/* The first argument is valid */
	  break;

	case 2:				/* Open the second file in "mode" mode */
	  if ((hTo = fopen(arg, mode)) == 0)
	    {
	    fprintf(stdout, "Unable to open %s\x07\n", arg);
	    return 1;
	    }
	  nargl += 1;			/* The second argument is valid */
	  break;

	case 3:				/* Change the default tab interval */
	  n = atoi(arg);
	  nargl += 1;			/* The third argument is valid */
	  break;

	default:
	  break;			/* Ignore extra arguments */
	} /* End switch */
      } /* End else */
    } /* End for */

  if (n < 1 || n > 32)
    {
    fputs("Tabs < 1 or > 32\x07\n", stdout);
    return 1;
    }

  if (mode[0] == 'a') fputs("\x0C", hTo); /* In append mode, add a form feed */

  while ((c = fgetc(hFrom)) != EOF)   	/*  c MUST be int!  */
    {
    switch (c)
      {
      case '\t':
	do
	  {
	  fputc(' ',hTo);
	  col++;
	  }
	  while (((col-1) % n) != 0);
	  /* Must subtr. 1 since col++ is done BEFORE this check.  */
	break;
      case '\n':
	fputc('\n',hTo);
	col = 1;
	break;
      default:
	fputc(c,hTo);
	col++;
      }
    }

  return 0;
  }

#if 0	/* JFL 2010-11-22 Modern C libraries all have this function built-in. */
int atoi(char s[])   /* convert s to integer */
  {
  int i,n;
  n=0;
  for (i=0;s[i] >= '0' && s[i] <= '9'; ++i) n=10*n+s[i]-'0';
  return(n);
  }
#endif

