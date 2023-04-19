/*****************************************************************************\
*                                                                             *
*   File name	    dump.c				         	      *
*									      *
*   Description     Dump a file in hexadecimal on standard output	      *
*									      *
*   Notes	    Under Unix, with gcc 3.x, compile with the -x c option,   *
*		    else it'd think this is a C++ source, and generate an     *
*		    error "undefined reference to '__gxx_personality_v0'".    *
*		    Alternative, use the -lstdc++ option to link-in C++ entry.*
*		    							      *
*		    There's a Linux tool called xxd that does almost the same.*
*		    							      *
*   History								      *
*    1995-12-19 JFL jf.larvoire@hp.com created this program.		      *
*    2004-04-05 JFL Dump the standard input if no file is specified.	      *
*                   Display a title with byte offsets, to improve readability.*
*                   Version 1.1.                                              *
*    2005-02-07 JFL Added support for a Unix version. No functional change.   *
*    2005-02-22 JFL Don't consider / as a switch character under Unix.        *
*                   Fixed bug with GetScreenRows, causing /p option to be off *
*                    by 1 line under all OS but MS-DOS. Version 1.1a.         *
*    2005-05-05 JFL Updated GetScreenRows to work under Unix. Version 1.1b.   *
*    2008-01-25 JFL Make termcap library optional under Unix, else use tput.  *
*                   Use fread instead of getch. Version 1.1c.                 *
*    2012-10-01 JFL Added support for a Win64 version. No new features.       *
*		    Version 1.1.4.					      *
*    2012-10-18 JFL Added my name in the help. Version 1.1.5.                 *
*    2014-12-04 JFL Define _GNU_SOURCE to avoid warnings in Linux.            *
*		    Version 1.1.6.					      *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 1.1.7.  					      *
*    2017-04-13 JFL Do not print a final blank line in DOS and Windows.       *
*		    Version 1.1.8.  					      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.1.9.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.1.10.     *
*    2019-10-31 JFL Added option -z to stop input on Ctrl-Z. Version 1.2.     *
*    2020-04-20 JFL Added support for MacOS. Version 1.3.                     *
*    2022-02-24 JFL Fixed the input pipe and redirection detection.           *
*		    Version 1.3.1.					      *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 1.3.2.		      *
*    2023-04-19 JFL Moved GetScreenRows() & GetScreenCols() to SysLib.	      *
*                   Renamed them as GetConRows() & GetConCols(). Ver. 1.3.3.  *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\******************************************************************************/

#define PROGRAM_DESCRIPTION "Dump data as both hexadecimal and text"
#define PROGRAM_NAME    "dump"
#define PROGRAM_VERSION "1.3.3"
#define PROGRAM_DATE    "2023-04-19"

#define _GNU_SOURCE		/* ISO C, POSIX, BSD, and GNU extensions */
#define _CRT_SECURE_NO_WARNINGS /* Avoid MSVC security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define SUPPORTED_OS 1

#define _getch getchar

#include <unistd.h>
#include <ctype.h>

#ifndef USE_TERMCAP
#define USE_TERMCAP 0 /* 1=Use termcap lib; 0=Don't */
#endif

#endif

/************************* OS/2-specific definitions *************************/

#ifdef _OS2    /* To be defined on the command line for the OS/2 version */

#define SUPPORTED_OS 1

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h" 

#undef FILENAME_MAX
#define FILENAME_MAX 260	/* FILENAME_MAX incorrect in stdio.h */

#include <conio.h>
#include <io.h>

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	     /* Automatically defined when targeting a Win32 applic. */

#define SUPPORTED_OS 1

#define NOGDI
#define NOUSER
#define NONLS
#include <windows.h>

#include <conio.h>
#include <io.h>

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	    /* Automatically defined when targeting an MS-DOS applic. */

#define SUPPORTED_OS 1

#include <conio.h>
#include <io.h>

#endif

/******************************* Any other OS ********************************/

#ifndef SUPPORTED_OS
#error "Unsupported OS"
#endif

/********************** End of OS-specific definitions ***********************/

/* SysToolsLib include files */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "console.h"	/* SysLib console management routines */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

/* Global variables */

int paginate = FALSE;

/* Forward references */

void usage(void);
int between(DWORD floor, DWORD u, DWORD ceiling);
void printflf(void);
int is_redirected(FILE *f);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    EXE program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to DOS.			      |
|									      |
|   Updates:								      |
|									      |
|    1995/12/19 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
    {
    int i;
    DWORD dwBase = 0xFFFFFFFFL;     /* First address to dump */
    DWORD dwLength = 0xFFFFFFFFL;   /* Number of bytes to dump */
    BYTE table[16];		    /* 16 bytes table */
    DWORD ul;
    WORD u;
    char *pszName = NULL;	    /* File name */
    FILE *f;
    int iCtrlZ = FALSE;		/* If true, stop input on a Ctrl-Z */

#ifndef _UNIX
    /* Force stdin and stdout to untranslated */
    _setmode( _fileno( stdin ), _O_BINARY );
#endif

    for (i=1; i<argc; i++)
        {
	char *pszArg = argv[i];
        if (IsSwitch(pszArg))
            {
	    char *pszOpt = pszArg+1;
	    if (   streq(pszOpt, "?")
	        || streq(pszOpt, "h")
	        || streq(pszOpt, "-help"))
                {
		usage();
                }
	    if (streq(pszOpt, "p"))
                {
		paginate = GetConRows() - 1;	/* Pause once per screen */
		continue;
                }
	    if (streq(pszOpt, "V")) {		/* -V: Display the version */
		puts(DETAILED_VERSION);
		exit(0);
	    }
	    if (streq(pszOpt, "z")) {		/* -z: Stop input on Ctrl-Z */
		iCtrlZ = TRUE;
		continue;
	    }
	    printf("Unrecognized switch %s. Ignored.\n", argv[i]);
            continue;
	    }
	if (!pszName)
	    {
	    pszName = pszArg;
	    continue;
	    }
	if (dwBase == 0xFFFFFFFFL)
            {
	    if (sscanf(pszArg, "%lX", &ul)) dwBase = ul;
            continue;
            }
	if (dwLength == 0xFFFFFFFFL)
            {
	    if (sscanf(pszArg, "%lX", &ul)) dwLength = ul;
            continue;
            }
        printf("Unexpected argument: %s\nIgnored.\n", pszArg);
        break;  /* Ignore other arguments */
	}

    if (pszName)
	{
        f = fopen(pszName, "rb");
        if (!f)
            {
            printf("Cannot open file %s.\n", pszName);
            exit(1);
            }
        }
    else
        {
	if (!is_redirected(stdin)) usage();
        f = stdin;
        paginate = FALSE;  /* Avoid waiting forever */
        }

    printf("\n\
Offset    00           04           08           0C           0   4    8   C   \n\
--------  -----------  -----------  -----------  -----------  -------- --------\n\
");

    if (dwBase == 0xFFFFFFFFL) dwBase = 0;
    fseek(f, dwBase & 0xFFFFFFF0L, SEEK_SET);

    for (ul = dwBase & 0xFFFFFFF0L;
	 between(dwBase & 0xFFFFFFF0L, ul, dwBase+dwLength);
         ul += 16)
	{
	size_t nRead;

	if (!iCtrlZ) {
	    nRead = fread(table, 1, 16, f);
	} else { /* Read characters 1 by 1, to avoid blocking if the EOF character is not on a 16-bytes boundary */
	    for (nRead = 0; nRead < 16; nRead++) {
	        char c;
	        if (!fread(&c, 1, 1, f)) break;
	        if (c == '\x1A') break; /* We got a SUB <==> EOF character */
	        table[nRead] = c;
	    }
	}
	if (!nRead) break;

	printf("%08lX ", ul);

	/* Display the hex dump */

	for (u=0; u<nRead; u++)
            {
	    if (!(u&3)) printf(" ");
	    if (between(dwBase, ul+u, dwBase+dwLength))
		printf("%02X ", (WORD)table[u]);
            else
		printf("   ");
            }
	for ( ; u<16; u++)
	    {
	    if (!(u&3)) printf(" ");
	    printf("   ");
	    }

	/* Display the character dump */

	for (u=0; u<nRead; u++)
	    {
	    if (!(u&7)) printf(" ");
#ifdef _UNIX
	    if ( (table[u] & 0x7F) < 0x20) table[u] = ' ';
#else
	    switch (table[u])
		{
		case '\x07': table[u] = ' '; break;
		case '\x08': table[u] = ' '; break; /* Backspace	*/
		case '\x09': table[u] = ' '; break; /* Tab		*/
		case '\x0A': table[u] = ' '; break; /* Line feed	*/
		case '\x0D': table[u] = ' '; break; /* Carrier return	*/
		case '\x1A': table[u] = ' '; break; 
		default: break;
		}
#endif
	    if (between(dwBase, ul+u, dwBase+dwLength) && (table[u]>' '))
		printf("%c", table[u]);
            else
		printf(" ");
            }
	for ( ; u<16; u++)
	    {
	    if (!(u&7)) printf(" ");
	    printf(" ");
	    }

	printflf();

	if (iCtrlZ && (nRead < 16)) break;
	}

#ifdef _UNIX
    printflf();
#endif

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
|									      |
|   Updates:								      |
|									      |
|    1995/12/19 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void)
    {
    printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: dump [switches] [filename] [address] [length]\n\
\n\
Switches:\n\
\n\
  -?|-h   Display this help screen\n\
  -p	  Pause for each screen-full of information.\n\
  -z      Stop input on a Ctrl-Z (aka. SUB or EOF) character\n\
"
#include "footnote.h"
);
    exit(1);
    }

int between(DWORD floor, DWORD u, DWORD ceiling)
    {
    if (ceiling >= floor)
        return (u >= floor) && (u < ceiling);       /* TRUE if inside */
    else    /* 32 bits wraparound */
        return !((u < floor) && (u >= ceiling));    /* TRUE if outside */
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    printflf						      |
|                                                                             |
|   Description:    Print a line feed, and possibly pause on full screens     |
|                                                                             |
|   Arguments:								      |
|                                                                             |
|	None								      |
|                                                                             |
|   Return value:   None						      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|    2008/01/25 JFL Added test for Control-C and ESC.                         |
*                                                                             *
\*---------------------------------------------------------------------------*/

void printflf(void)
   {
   static int nlines = 0;
   int c;

   printf("\n");

   if (!paginate) return;

   nlines += 1;
   if (nlines < paginate) return;
   nlines = 0;

   fflush(stdin);		/* Flush any leftover characters */
   printf("Press any key to continue... ");
   c = _getch();		/* Pause until a key is pressed */
   fflush(stdin);		/* Flush any additional characters that may be left */
   printf("\r                                   \r");
   if (c==3 || c==27) exit(0);	/* Ctrl-C or ESC pressed. Exit immediately. */
   return;
   }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    is_redirected					      |
|									      |
|   Description:    Check if a FILE is a device or a disk file. 	      |
|									      |
|   Parameters:     FILE *f		    The file to test		      |
|									      |
|   Returns:	    TRUE if the FILE is a disk file			      |
|									      |
|   Notes:	    Designed for use with the stdin and stdout FILEs. If this |
|		    routine returns TRUE, then they've been redirected.       |
|									      |
|   History:								      |
|    2004-04-05 JFL Added a test of the S_IFIFO flag, for pipes under Windows.|
|    2023-02-24 JFL Use the standard S_ISxxx macros to detect files and fifos.|
*									      *
\*---------------------------------------------------------------------------*/

int is_redirected(FILE *f) {
  int err;
  struct stat st;
  int h;

  h = fileno(f);			/* Get the file handle */
  err = fstat(h, &st);			/* Get information on that handle */
  if (err) return FALSE;		/* Cannot tell more if error */
  return (   (S_ISREG(st.st_mode))	/* Tell if device is a regular file */
	  || (S_ISFIFO(st.st_mode))	/* or it's a FiFo */
	 );
}

