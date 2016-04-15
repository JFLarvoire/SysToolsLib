/*****************************************************************************\
*                                                                             *
*   File name:	    lessive.c				                      *
*                                                                             *
*   Description:    Remove blanks at the end lines in a text file.	      *
*                                                                             *
*   History:								      *
*    1989-11-28 JFL jf.larvoire@hp.com created this program.                  *
*    1991-07-16 JFL Fixed not to remove blank lines. Version 1.01.	      *
*    1992-05-20 JFL Support for output file equal to the input file.          *
*		    Support for OS/2. Version 1.1.			      *
*    2006-09-11 JFL Support for Win32. Version 1.2.                           *
*    2010-02-12 JFL Fixed bug causing a crash with lines > 256 characters.    *
*                   Version 1.2.1.                                            *
*    2011-02-24 JFL Only display the program banner with the help screen.     *
*                   The -v option now displays the number of lines changed.   *
*                   Try renaming the temp file, and copy it only if needed.   *
*                   Version 1.3.                                              *
*    2012-10-18 JFL Added my name in the help. Version 1.3.1.                 *
*    2014-12-04 JFL Corrected several warnings. Version 1.3.2.		      *
*    2016-01-08 JFL Fixed all warnings in Linux. Version 1.3.3. 	      *
*                                                                             *
\*****************************************************************************/

#define PROGRAM_VERSION "1.3.3"
#define PROGRAM_DATE    "2016-01-08"

#define _CRT_SECURE_NO_WARNINGS /* Avoid MSVC security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define LINESIZE 16384	/* Can't allocate more than 64K for stack in some OS. */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Defined for Win32 applications */

#define TARGET_WIN32

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#include <dos.h>
#include <direct.h>

/* Avoid warnings for names that MSVC thinks deprecated */
#define strlwr _strlwr
#define getcwd _getcwd
#define unlink _unlink

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Defined for MS-DOS applications */

#define TARGET_MSDOS
#define OS_NAME "DOS"

#include <dos.h>
#include <direct.h>

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Defined for Unix applications */

#define TARGET_UNIX
#define OS_NAME "UNIX"

#include <unistd.h>
#include <ctype.h>
#include <dirent.h>

char *strlwr(char *psz)    /* Convenient Microsoft library routine not available on Unix. */
   {
   char c;
   char *psz0 = psz;
   while ((c = *psz)) *(psz++) = (char)tolower(c);
   return psz0;
   }

#endif

/********************** End of OS-specific definitions ***********************/

/* Global variables */

int iVerbose = 0;
#define verbose(args) if (iVerbose) printf args

/* Function prototypes */

int main(int argc, char *argv[]);
void usage(void);                   /* Display a brief help and exit */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    C program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int main(int argc, char *argv[])
    {
    int i;
    char *source = NULL;        /* Source file name */
    FILE *sf;                   /* Source file pointer */
    char *dest = NULL;          /* Destination file name */
    FILE *df;                   /* Destination file pointer */
    FILE *mf = stdout;          /* Messsage file pointer */
    int nChanged = 0;		/* Number of lines changed */
    char line[LINESIZE];
    char c;
    char *pc;
    char tempname[FILENAME_MAX+1];
    char *temp = NULL;
    int iErr;

    for (i=1; i<argc; i++)
        {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) /* It's a switch */
            {
            strlwr(argv[i]);
	    if (streq(argv[i]+1, "help") || streq(argv[i]+1, "h")
					 || streq(argv[i]+1, "?"))
                {
                usage();
                }
	    if (streq(argv[i]+1, "v") || streq(argv[i]+1, "verbose"))
                {
                iVerbose += 1;
		continue;
                }
            printf("Unrecognized switch %s. Ignored.\n", argv[i]);
            continue;
            }
        if (!source)
            {
            source = argv[i];
            continue;
            }
        if (!dest)
            {
            dest = argv[i];
            continue;
            }
        printf("Unexpected argument: %s\nIgnored.\n", argv[i]);
        break;  /* Ignore other arguments */
        }

    if (source)
        {
        sf = fopen(source, "r");
        if (!sf)
            {
            printf("Can't open file %s.\n", source);
            exit(1);
            }
	if (dest && streq(source, dest)) dest = NULL;	/* Security */
        }
    else
        {
        sf = stdin;
        }

    if (dest)
        {
	df = fopen(dest, "w");
        if (!df)
            {
            printf("Can't open file %s.\n", dest);
            exit(1);
            }
	}
    else
	{
	if (source)
	    {
	    pc = getenv("TMP");
	    if (pc)
		strcpy(tempname, pc);
	    else
		strcpy(tempname, getcwd(NULL, FILENAME_MAX+1));
	    if (pc[strlen(tempname)-1] != '\\') strcat(tempname, "\\");
	    strcat(tempname, "LESSIVE.TMP");
	    dest = temp = tempname;
	    verbose(("Temp file: %s\n", temp));
	    df = fopen(temp, "w");
	    if (!df)
		{
		printf("Can't open file %s.\n", temp);
		exit(1);
		}
	    }
	else
	    {
	    df = stdout;
	    }
        }

    if (df == stdout)          /* Messsage file pointer */
	{
	mf = stderr;
	}

    while(fgets(line, LINESIZE, sf))
        {
        int l;
        l = (int)strlen(line);
        i = l;
        /* Is there an \n at the end of the line? */
	if (i > 0)
            {
            c = line[i-1];
            if (c != '\n') c = '\0';
            }
        else
            {
	    c = '\0';
            }
        /* Remove all trailing spaces, tabs, and \n */
        while (i)
            {
            i -= 1;             /* Offset of last character in line */
            if (line[i] == '\n') continue;          /* Newline */
            if (line[i] == ' ') continue;           /* Space */
            if (line[i] == '\x09') continue;        /* Tab */
            i += 1;             /* Length of string to preserve */
            break;
            }
        /* Restore the final \n, if initially present */
        line[i] = c;
        if (c) line[++i] = '\0';
        /* Count lines changed */
        if (i != l) nChanged += 1;

        fputs(line, df);
        }

    fclose(sf);
    fclose(df);

    if (temp)
	{
	char buffer[512];

	strcpy(line, source);
	pc = strrchr(line, '.');
	if (pc && !strchr(pc, '\\'))	/* If extension in name & not in path */
	    strcpy(pc, ".bak"); 	    /* Change extension to .bak */
	else
	    strcat(line, ".bak");	    /* Set extension to .bak */
	unlink(line);			/* Remove the .bak if already there */
	verbose(("Renaming %s as %s.\n", source, line));
	rename(source, line);		/* Rename the source as .bak */

	verbose(("Renaming %s as %s.\n", tempname, source));
	iErr = rename(tempname, source);/* Rename the temp as source */
	if (iErr) /* This may fail if the temp file is not on the same drive */
	    {	  /* In this case, copy the temp file, then delete it. */
	    verbose(("Renaming failed. Copying instead."));
	    sf = fopen(tempname, "r");
	    df = fopen(source, "w");
	    while ((i = (int)fread(buffer, 1, 512, sf)))
		{
		fwrite(buffer, i, 1, df);
		}
	    fclose(sf);
	    fclose(df);
	    unlink(tempname);
	    }
	}

    if (iVerbose) fprintf(mf, "%d lines trimmed\n", nChanged);

    return 0;
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

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
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void)
    {
    printf("\n\
Lessive Version " PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME " - Wipe out trailing blanks\n\
\n\
Usage: lessive [input_file [output_file]]\n\
\n\
Default output_file = input_file\n\
Default input_file = stdin\n\
\n"
#ifndef __unix__
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);
    exit(0);
    }
