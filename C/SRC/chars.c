/*****************************************************************************\
*									      *
*  File name:	    chars.c						      *
*									      *
*  Description:     Display all 8-bit characters in hexadecimal		      *
*									      *
*  Notes:	    Useful to see what characters look like in your current   *
*		    code page.						      *
*									      *
*  History:								      *
*    1995-11-03 JFL jf.larvoire@hp.com created this program.		      *
*    2015-01-08 JFL Fix output for Linux, including when encoding is UTF-8.   *
*		    Version 1.1.					      *
*    2017-03-06 JFL Added an optional code page argument for Windows.         *
*		    Version 1.2.					      *
*		    							      *
*       © Copyright 2016-2017 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "1.2"
#define PROGRAM_DATE    "2017-03-06"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#include <windows.h>

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define OS_NAME "DOS"

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define OS_NAME "OS/2"

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Automatically defined when targeting a Unix app. */

#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)

void usage(void);

int main(int argc, char *argv[])
    {
    int i, j;
#ifdef _WIN32
    int iCP = 0;
    DWORD dwCP0 = GetConsoleOutputCP();
#endif
#ifdef __unix__
    int isUTF8 = FALSE;
    char *pszLang = getenv("LANG");

    if (pszLang && strstr(pszLang, "UTF-8")) isUTF8 = TRUE;
#endif

    for (i=1; i<argc; i++) {
      char *arg = argv[i];
      if (   (arg[0]=='-')
#ifndef __unix__
          || (arg[0]=='/')
#endif
        ) {
        arg[0] = '-';
	if (   streq(arg, "-h")	    /* Display usage */
	    || streq(arg, "-help")	/* The historical name of that switch */
	    || streq(arg, "--help")
	    || streq(arg, "-?"))
	    {
	    usage();
            }
	if (   streq(arg, "-V")     /* -V: Display the version */
	    || streq(arg, "--version"))
	    {
	    printf(PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME "\n");
	    exit(0);
	    }
	fprintf(stderr, "Unrecognized switch %s. Ignored.\n", arg);
	continue;
	}
#ifdef _WIN32
    if (!iCP) {
      iCP = atoi(arg);
      continue;
    }
#endif
    fprintf(stderr, "Unrecognized argument %s. Ignored.\n", arg);
    }

#ifdef _WIN32
    if (iCP) SetConsoleOutputCP(iCP);
#endif

    for (j=0; j<16; j++)
	{
	for (i=0; i<8; i++)
	    {
	    int k, l;

	    if (!(i&3)) printf("  ");

	    k = (16*i)+j;
	    switch (k)
		{
#ifndef __unix__
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0D:
		case 0x1A:
		    l = ' ';
		    break;
#endif
		default:
#ifdef __unix__
		    if (k < 0x20) {
		      l = ' ';
		      break;
		    }
#endif
		    l = k;
		    break;
		}

	    printf("  %02X %c", k, l);
	    }
	printf("\n");
	}

    printf("\n");

    for (j=0; j<16; j++)
	{
	for (i=8; i<16; i++)
	    {
	    int k;

	    if (!(i&3)) printf("  ");

	    k = (16*i)+j;

#ifdef __unix__
            if (isUTF8) {
              int c1,c2;
              c1 = 0xC0 + ((k >> 6) & 0x03);
              c2 = 0x80 + (k & 0x3F);
	      printf("  %02X %c%c", k, c1, c2);
            } else
#endif
	    printf("  %02X %c", k, k);
	    }
	printf("\n");
	}

#ifdef _WIN32
    if (iCP) SetConsoleOutputCP(dwCP0);
#endif

    return 0;
    }

void usage(void)
    {
    printf("chars version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME "\n\
\n"
#ifdef _WIN32
"Usage: chars [SWITCHES] [CODEPAGE]\n"
#else
"Usage: chars [SWITCHES]\n"
#endif
"\n\
Switches:\n\
  -h|--help|-?  Display this help screen.\n\
  -V|--version  Display this program version and exit.\n\
\n\
Author: Jean-Francois Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
);

    exit(0);
    }
