/*****************************************************************************\
*                                                                             *
*   Filename:	    tee.c						      *
*									      *
*   Description:    Duplicate the input from stdout to stdout and other files.*
*									      *
*   Notes:	    This is a remake of the Unix tee for DOS and Windows.     *
*									      *
*   History:								      *
*    2012-10-24 JFL Created this program.				      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2016-09-23 JFL Minor tweak to avoid a warning.	                      *
*    2017-03-15 JFL Changed to a UTF-8 app, to support non-ASCII file names.  *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.1.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.1.2.      *
*    2021-01-06 JFL Fixed the exit code for the help screen. Version 1.1.3.   *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Duplicate the input to several outputs"
#define PROGRAM_NAME    "tee"
#define PROGRAM_VERSION "1.1.3"
#define PROGRAM_DATE    "2021-01-06"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _UTF8_SOURCE	/* Enable MsvcLibX support for file names with Unicode characters */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <io.h>
/* SysToolsLib include files */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

/* Avoid warnings for names that MSVC thinks deprecated */
#define read _read

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define PATHNAME_SIZE FILENAME_MAX
#define NODENAME_SIZE 13				/* 8.3 name length = 8+1+3+1 = 13 */

#endif

/********************** End of OS-specific definitions ***********************/

/* Local definitions */

#define TRUE 1
#define FALSE 0

#define BUFSIZE 1024

typedef struct _outStream {
  char *name;
  FILE *f;
  struct _outStream *next;
} outStream;

#define streq(string1, string2) (strcmp(string1, string2) == 0)

/* Forward references */

void usage(void);			/* Display a brief help screen */
outStream *NewOutStream(char *pszName, char *pszMode, outStream *last);
size_t GetDefaultBufSize();

/******************************************************************************
*                                                                             *
*   Function	    main						      *
*                                                                             *
*   Description     Main procedure					      *
*                                                                             *
*   Arguments                                                                 *
*                                                                             *
*	  int argc	Number of command line arguments, including program.  *
*	  char *argv[]	Array of pointers to the arguments. argv[0]=program.  *
*                                                                             *
*   Return value    0=Success; !0=Failure                                     *
*                                                                             *
*   Notes                                                                     *
*                                                                             *
*   History                                                                   *
*    2012-10-24 JFL Created this program.				      *
*                                                                             *
******************************************************************************/

int main(int argc, char *argv[]) {
  int i;
  char *pszMode = "wb";
  outStream *pFirst;
  outStream *pLast;
  outStream *pStream;
  size_t szBuf = BUFSIZE;
  char *pBuf;
  char *pBufSize;
  ssize_t nRead;

  pFirst = pLast = NewOutStream(NULL, NULL, NULL); /* Always output to stdout */

  pBufSize = getenv("TEE_BUFSIZE");
  if (pBufSize) szBuf = atoi(pBufSize);

  /* Parse the command line */
  for (i=1; i<argc; i++) {
    if ((argv[i][0] == '-') || (argv[i][0] == '/')) { /* It's a switch */
      char *option = argv[i]+1;
      if (streq(option, "?") || streq(option, "h") || streq(option, "-help")) {
	usage();
	exit(0);
      }
      if (streq(option, "")) {
	pLast = NewOutStream(NULL, NULL, pLast);
	continue;
      }
      if (streq(option, "a")) {
	pszMode = "ab";
	continue;
      }
      if (streq(option, "A")) {
	pszMode = "wb";
	continue;
      }
      if (streq(option, "b")) {
	if ((i+1)<argc) szBuf = atoi(argv[++i]);
	continue;
      }
      if (streq(option, "V") || streq(option, "-version")) { /* -V: Display the version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      fprintf(stderr, "Unrecognized switch %s. Ignored.\n", argv[i]);
      continue;
      }
    /* This is a file name. Add it to the output streams list */
    pLast = NewOutStream(argv[i], pszMode, pLast);
    pszMode = "wb";	/* Reset to the default write mode */
    continue;
  }

  pBuf = malloc(szBuf);
  if (!pBuf) {
    fprintf(stderr, "Not enough memory\n");
    exit(1);
  }

  /* Make sure no translation is done on stdin or stdout */
  _setmode(_fileno(stdin),  _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);

  /* Make sure no buffering is used on any output file */
  for (pStream = pFirst; pStream; pStream = pStream->next) {
    int iErr = setvbuf(pStream->f, NULL, _IONBF, 0);
    if (iErr) {
      fprintf(stderr, "Cannot unbuffer %s\n", pStream->name);
    }
  }

  /* Copy all incoming data */
  while ((nRead = read(0, pBuf, (int)szBuf)) > 0) { /* Use read to avoid buffering the input */ /* Cast (int) as MS version takes an in */
    /*
    printf("Read %d bytes: ", nRead);
    fwrite(pBuf, 1, nRead, stdout);
    printf("\n");
    */
    for (pStream = pFirst; pStream; pStream = pStream->next) {
      /* printf("Writing to %s\n", pStream->name); */
      fwrite(pBuf, 1, nRead, pStream->f);
    }
  }

  return 0;
}

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: tee [OPTIONS] [[-a] FILENAME] ...\n\
\n\
Options:\n\
  -?	    Display this help screen.\n\
  -a	    Append to the next file. Default: Overwrite it.\n\
  -b	    Set the buffer size. Default: %d\n\
  -V        Display the program version\n\
\n\
Note: The buffer size can also be set by environment variable TEE_BUFSIZE.\n\
\n"
#if defined(_MSDOS)
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, GetDefaultBufSize());
  return;
}

outStream *NewOutStream(char *pszName, char *pszMode, outStream *last) {
  outStream *pStream = (outStream *)malloc(sizeof(outStream));
  if (!pStream) {
    fprintf(stderr, "Not enough memory\n");
    exit(1);
  }
  pStream->name = pszName;
  pStream->next = NULL;
  if (pszName) {
    pStream->f = fopen(pszName, pszMode);
    if (pStream->f) {
      if (last) last->next = pStream;
    } else {
      fprintf(stderr, "Error. Cannot open file %s\n", pszName);
      free(pStream);
      pStream = last;
    }
  } else {
    pStream->name = "stdout";
    pStream->f = stdout;
    if (last) last->next = pStream;
  }
  /* printf("Allocated %p for file %s\n", pStream, pStream->name); */
  return pStream;
}

size_t GetDefaultBufSize() {
  size_t szBuf = BUFSIZE;
  char *pBufSize;

  pBufSize = getenv("TEE_BUFSIZE");
  if (pBufSize) szBuf = atoi(pBufSize);
  return szBuf;
}


