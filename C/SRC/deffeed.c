/*****************************************************************************\
*                                                                             *
*   Filename:	    deffeed.c						      *
*									      *
*   Description:    Remove form feeds and tabs from a file		      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*									      *
*    1988-01-21 JFL Created prep3000.c. 				      *
*    1990-03-23 JFL Added detabulation to prep3000.c and renamed it with the  *
*		    more generic name of deffeed.c			      *
*    1990-06-22 JFL Added the -fp argument.				      *
*    1990-07-02 JFL Supress unnecessary carrier-returns and backspaces.       *
*		    Improved error handling. Added the -filter argument.      *
*    1990-10-30 JFL Added multicolumn output capability. Version 1.4.	      *
*    1991-10-31 JFL Changed default tab to 8 for C & H files too. Version 1.5.*
*    1992-05-20 JFL Added the /? swicth. Support for OS/2. Version 1.6.       *
*    1996-03-12 JFL Added the -cleanup switch. Version 1.7.		      *
*    2001-08-24 JFL Added the -self switch. Added WIN32 support. Version 2.0. *
*    2002-03-01 JFL Fixed bug with -setup, broken in 2.0. Version 2.0a.	      *
*    2002-11-28 JFL Avoid unnecessary form-feeds in the case where pages are  *
*		    already exactly lpp lines-long.			      *
*		    Ignore CRLF immediately following a FF.		      *
*		    Added debug code in the debug version. 		      *
*		    Version 2.1.					      *
*    2003-04-15 JFL Added Unix version.					      *
*		    Version 2.2.  					      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*    2016-01-08 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 2.2.2.  					      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.2.2"
#define PROGRAM_DATE    "2016-01-08"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "fcntl.h"
#include "stdlib.h"

#define FALSE 0
#define TRUE 1

#define BUFSIZE 256
#define DEFLPP 60       /* Default number of lines per page */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#ifdef _DEBUG
#define DEBUG_VERSION " Debug"
#define IFDEBUG(args) args
#else
#define DEBUG_VERSION
#define IFDEBUG(args)
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32           /* Automatically defined when targeting a Win32 applic. */

#define TARGET_WIN32

#define OS_NAME "WIN32"

#include "io.h"
#include "direct.h"

#define getcwd _getcwd
#define strlwr _strlwr
#define setmode _setmode
#define unlink _unlink

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS           /* Automatically defined when targeting a 16-bits applic. */

#define TARGET_MSDOS

#define OS_NAME "DOS"

#include "io.h"
#include "direct.h"

#define getcwd _getcwd

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__         /* Automatically defined in all Unix variants */

#define TARGET_UNIX

#define OS_NAME "Unix"

#include <unistd.h>     /* Define getcwd(). */

#define _MAX_PATH 4096  /* There's no limit under Unix. Use 4K as a practical one. */
#define setmode(file, mode) /* There's no disctinction between text and binary modes under Unix. */
#define O_TEXT
#define O_BINARY

#include <ctype.h>      /* Define tolower(). */

char *strlwr(char *psz)    /* Convenient Microsoft library routine not available on Unix. */
   {
   char c;
   char *psz0 = psz;
   while ((c = *psz)) *(psz++) = (char)tolower(c);
   return psz0;
   }

#endif

/********************** End of OS-specific definitions ***********************/

#define VERSION PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME DEBUG_VERSION

/* Global variables */

int ncols = 1;          /* Number of columns */
int wcols = 80;         /* Columns width */
int dcols = 0;          /* Distance between columns */
char *buffer = NULL;

/* Forward references */

void usage(void);
int detab(char *line, int length, int tab);
int output_line(int np, int nl, char *format, char *text, FILE *fdest);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Main program routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The errorlevel code to return to the operating system.    |
|									      |
|   History:								      |
|									      |
|    1988/01/21 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int main(int argc, char *argv[])
    {
    int lpp = -1;       /* Lines per page (default: 60) */
    int tab = -1;       /* Spaces per tab (default: 8) */
    int nsp = 0;        /* Spaces before each line (default: 0) */
    int extra = 0;      /* Extra lines after each page (default: 0) */
    int modnp = 1;      /* Number of logical pages on a physical page */
    int fptp = 0;       /* Number of logical full pages to print (0 = Off) */
    int fptp0 = 0;      /* Same for physical pages in multicolumn operation */
    char line[BUFSIZE]; /* Line buffer */
    int length;         /* Length of above buffer */
    char *pline;        /* Pointer on the beginning of the line */
    int nl = 0;         /* Current line number (0 to lpp-1) */
    int np = 0;         /* Current page number, modulo modnp */
    int npt = 0;        /* Current page number */
    char *format;       /* output format */
    int top_without_ff = TRUE; /* TRUE if we've reached the top of page without a form-feed */
    int i;
    char *source=NULL;  /* Source file name */
    FILE *fsource=stdin;/* Source file pointer */
    FILE *fdest;        /* Destination file pointer */
    char *pszSetup=NULL;/* Setup file name */
    FILE *fsetup;       /* Setup file pointer */
    char *cleanup=NULL; /* Cleanup file name */
    FILE *fcleanup;	/* Cleanup file pointer */
    int filter = FALSE; /* If TRUE, use stdin for the input */
    int nerrors = 0;    /* Number of errors */
    int nwarnings = 0;  /* Number of warnings */
    int buffer_size;    /* Size of the multicolumn buffer */
    int iSelf = FALSE;	/* If TRUE, output file = input file */
    char szTempFileName[_MAX_PATH] = {0};
    char *pc;

    for (i=1; i<argc; i++)          /* Process all command line arguments */
        {
        if (   (*(argv[i]) == '/')          /* Process switches first */
            || (*(argv[i]) == '-') )
            {
            strlwr(argv[i]);
	    if (   streq(argv[i]+1, "help")
		|| streq(argv[i]+1, "h")
		|| streq(argv[i]+1, "?"))
                {
                usage();
                }
	    if (streq(argv[i]+1, "cleanup"))
                {
                if ((i+1) < argc)
                    {
                    i += 1;
		    cleanup = argv[i];
                    }
                else
                    {
		    fprintf(stderr, "Cleanup file not specified.\n");
                    nwarnings += 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "extra"))
                {
                int temp;

                /* Attemp to read the optionnal value following -extra */
                if ( ((i+1) < argc) && (sscanf(argv[i+1], "%d", &temp) == 1) && (temp >= 0))
                    {
                    /* If there was indeed a value, and its conversion
                    succeeded, then store it in the "extra" variable */
                    extra = temp;
                    i += 1;
                    }
                else
                    {
                    /* Else use the default number of extra lines */
                    extra = 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "dcol"))
                {
                int temp1, temp2;

                /* Attemp to read the value following -nsp */
                temp1 = sscanf(argv[i+1], "%d", &temp2);
                /* If there was a valid value, store it in "nsp" */
                if ( ((i+1) < argc) && (temp1 == 1))
                    {
                    dcols = temp2;
                    i += 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "filter"))
                {
                filter = TRUE;
                continue;
                }
	    if (streq(argv[i]+1, "fp"))
                {
                int temp1, temp2;

                /* Attemp to read the value following -page */
                temp1 = sscanf(argv[i+1], "%d", &temp2);
                /* If there was a valid value, store it in "fptp" */
                if ( ((i+1) < argc) && (temp1 == 1))
                    {
                    fptp = temp2;
                    i += 1;
                    }
                else
                    {
                    /* Else use the default number of full pages */
                    fptp = 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "ncol"))
                {
                int temp1, temp2;

                /* Attemp to read the value following -nsp */
                temp1 = sscanf(argv[i+1], "%d", &temp2);
                /* If there was a valid value, store it in "nsp" */
                if ( ((i+1) < argc) && (temp1 == 1))
                    {
                    ncols = temp2;
                    i += 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "nsp"))
                {
                int temp1, temp2;

                /* Attemp to read the value following -nsp */
                temp1 = sscanf(argv[i+1], "%d", &temp2);
                /* If there was a valid value, store it in "nsp" */
                if ( ((i+1) < argc) && (temp1 == 1))
                    {
                    nsp = temp2;
                    i += 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "self"))
                {
                iSelf = TRUE;
                continue;
                }
	    if (streq(argv[i]+1, "setup"))
                {
                if ((i+1) < argc)
                    {
                    i += 1;
                    pszSetup = argv[i];
                    }
                else
                    {
                    fprintf(stderr, "Setup file not specified.\n");
                    nwarnings += 1;
                    }
                continue;
                }
	    if (streq(argv[i]+1, "wcol"))
                {
                int temp1, temp2;

                /* Attemp to read the value following -nsp */
                temp1 = sscanf(argv[i+1], "%d", &temp2);
                /* If there was a valid value, store it in "nsp" */
                if ( ((i+1) < argc) && (temp1 == 1))
                    {
                    wcols = temp2;
                    i += 1;
                    }
                continue;
                }
            fprintf(stderr, "Invalid switch %s\n", argv[i]);
            nwarnings += 1;
            continue;
            }
        /* Assign optionnal arguments in their official order (see usage) */
        if (lpp == -1)
            {
            int temp1, temp2;

            temp1 = sscanf(argv[i], "%d", &temp2);
            if (temp1 == 1)
                {
                /* If there was a valid value, store it in "lpp" */
                lpp = temp2;
                continue;
                }
            else
                {
                /* Else use default, and assume argv[i] is a source name */
                lpp = DEFLPP;
                }
            }
        if (!source && !filter)
            {
            source = argv[i];
            continue;
            }
        if (tab == -1)
            {
            sscanf(argv[i], "%d", &tab);
            continue;
            }
        fprintf(stderr, "Too many arguments.\n");
        nwarnings += 1;
        }

    if (source)
        {
        if (filter)
            {
            fprintf(stderr, "Too many arguments.\n");
            nwarnings += 1;
            }
        else
            {
            /* If the source file name is provided, try to open the file */
            fsource = fopen(source, "r");
            if (!fsource)
                {
                fprintf(stderr, "Can't open input file %s.\n", source);
                exit(1);
                }
            if (tab == -1)
                {
#if NEEDED
                i = strlen(source);
                if (   (strcmpi(source+i-2, ".C")==0)
                    || (strcmpi(source+i-2, ".H")==0) )
                    tab = 4;
		else
#endif
                    tab = 8;
                }
            }
        }
    else
        {
        /* Else use standard input */
        if (filter)
            fsource = stdin;
        else
            nerrors += 1;
        }

    if (iSelf)
	{
	pc = getenv("TEMP");
	if (pc)
	    strcpy(szTempFileName, pc);
	else
	    strcpy(szTempFileName, getcwd(NULL, _MAX_PATH));
	if (pc[strlen(szTempFileName)-1] != '\\') strcat(szTempFileName, "\\");
	strcat(szTempFileName, "DEFFEED.TMP");
	fdest = fopen(szTempFileName, "w");
	if (!fdest)
	    {
	    printf("Can't open file %s.\n", szTempFileName);
	    exit(1);
	    }
	}
     else
	{
	fdest = stdout;
	}

    if (nwarnings || nerrors)
        fprintf(stderr,
                "Type deffeed -help for a description of the arguments\n");

    if (nerrors) exit(1);

    /* Make sure defaults are set */
    if (lpp == -1) lpp = DEFLPP;
    if (tab == -1) tab = 8;

    fprintf(stderr, "%d lines per page, %d lines between pages", lpp, extra);
    if (fptp)
        fprintf(stderr, ", fill a multiple of %d pages", fptp);
    else
        fprintf(stderr, ", do not fill the last page");
    fprintf(stderr, ".\n");

    if (ncols > 1) /* fptp must be set for multicolumn operation */
        {
        fptp0 = fptp;           /* Remember the original setting */
        if (!fptp) fptp = 1;
        fptp *= ncols;          /* One physical page is ncols logical pages */
        }
    if (fptp) modnp = fptp;     /* Else default 1 */

    /* Allocate the buffer */
    buffer_size = (wcols + dcols) * ncols * (lpp + extra);
    buffer = malloc(buffer_size);
    if (!buffer)
        {
        fprintf(stderr, "Not enough memory to run.\n");
        exit(1);
        }

    /* Copy the setup file */
    if (pszSetup)
        {
        fsetup = fopen(pszSetup, "rb");
        if (!fsetup)
            {
            fprintf(stderr, "Can't open setup file %s.\n", pszSetup);
            usage();
            }
        setmode(fileno(fdest), O_BINARY);
        while ((i = (int)fread(line, 1, BUFSIZE, fsetup)))
            fwrite(line, 1, i, fdest);
        setmode(fileno(fdest), O_TEXT);
        fclose(fsetup);
        }

    format = "          %s\n";		/* 10 spaces then the line then CRLF */
    format += (10 - nsp);               /* keep only the required spaces */

    while ((pline = fgets(line, BUFSIZE, fsource)))
        {
        if (nl > 0) top_without_ff = FALSE; /* It's not the top line anymore */
        
        /* Remove the trailing carrier-return(s) and linefeed */
        i = (int)strlen(pline);
        while ((i>0) && ((pline[i-1] == '\n') || (pline[i-1] == '\r')))
            {
            pline[--i]='\0';
            }

        length = BUFSIZE;

        while (*pline == '\f')             /* If line begins with a form feed */
            {
            pline += 1;
            length -= 1;
            if (!top_without_ff)    /* Except in the case where we're at top line without a form-feed... */
                {		    /*  ... fill-up the rest of the page with blank lines.		 */
		IFDEBUG(fprintf(stderr, "Processing form-feed on page %d line %d.\n", npt, nl));
                while (nl < lpp+extra) output_line(np, nl++, format, "", fdest);
                nl = 0;
                np += 1;
                np %= modnp;
                npt += 1;
                }
            else
                {
		IFDEBUG(fprintf(stderr, "Skipping form-feed on page %d line %d.\n", npt, nl));
                }
	    top_without_ff = FALSE; /* Do this exception only once. (We've had a form-feed now.) */
            }
        if ((nl == 0) && (!*pline) && (top_without_ff == FALSE)) continue; /* Ignore CRLF immediately following a FF */
        if (*pline == '\b')
            {
            pline += 1;     /* Remove backspaces at column 0 */
            length -= 1;
            }
        detab(pline, length, tab);
        output_line(np, nl, format, pline, fdest);
        nl += 1;
        if (nl == lpp)
            {
            IFDEBUG(fprintf(stderr, "Reached end of page %d on line %d. Moving to top of next page.\n", npt, nl));
            for (i=0; i<extra; i++) output_line(np, nl+i, format, "", fdest);
            nl = 0;
            np += 1;
            np %= modnp;
            npt += 1;
            top_without_ff = TRUE;	/* Flag the fact we automatically fed a page. */
            }
        }

    if (fptp)
        {
        if (np || nl)
            {
            if (!ncols || fptp0)
                {
                while (np < fptp)
                    {
                    while (nl < lpp+extra) output_line(np, nl++, format, "", fdest);
                    nl = 0;
                    np += 1;
                    }
                }
            else /* Do not output the very last line feed in multicolumn mode */
                {
                while (np < fptp - 1)
                    {
                    while (nl < lpp+extra) output_line(np, nl++, format, "", fdest);
                    nl = 0;
                    np += 1;
                    }
                while (nl < lpp+extra-1) output_line(np, nl++, format, "", fdest);
                i = (int)strlen(format);
                format[i-1] = ' ';      /* Remove the line feed */
                output_line(np, nl++, format, "", fdest);
                nl = 0;
                np += 1;
                }
            np = 0;
            }
        }

    /* Copy the cleanup file */
    if (cleanup)
        {
	fcleanup = fopen(cleanup, "rb");
	if (!fcleanup)
            {
	    fprintf(stderr, "Can't open cleanup file %s.\n", cleanup);
            usage();
            }
        setmode(fileno(fdest), O_BINARY);
	while ((i = (int)fread(line, 1, BUFSIZE, fcleanup)))
            fwrite(line, 1, i, fdest);
        setmode(fileno(fdest), O_TEXT);
	fclose(fcleanup);
        }

    /* Close the input and output files */
    if (fsource != stdin) fclose(fsource);
    if (fdest != stdout) fclose(fdest);

    /* Rename the temporary file */
    if (szTempFileName[0])
	{
	char szBakName[_MAX_PATH];

	strcpy(szBakName, source);
	pc = strrchr(szBakName, '.');
	if (pc && !strchr(pc, '\\'))	    /* If extension in name & not in path */
	    strcpy(pc, ".BAK"); 	    /* Change extension to .BAK */
	else
	    strcat(szBakName, ".BAK");	    /* Set extension to .BAK */
	unlink(szBakName);		    /* Remove the .bak if already there */
	rename(source, szBakName);	    /* Rename the source as .BAK */
	rename(szTempFileName, source);
	}

    return 0;
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help for this program		      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    N/A 						      |
|                                                                             |
|   History:								      |
|									      |
|    1992/05/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void)
    {
  printf("\n\
deffeed version " VERSION "\n\
\n\
Usage:\n\
deffeed [lpp] [source] [tab] [switches] >destination\n\
\n\
  lpp              Lines Per Page. Default: 60.\n\
  source           Input file.\n\
  tab              Spaces per tab. Default: 8 (4 for .C or .H files).\n\
  >destination     Output file. Default: Display.\n\
\n\
Switches:\n\
  -cleanup {file}  Finish by the given cleanup file. Default: None.\n\
  -dcol n          Distance between columns. Default: 0\n\
  -extra [n]       Extra blank lines between pages. Default: 0\n\
  -filter          Use stdin for the source.\n\
  -fp [n]          Fill a multiple of n pages. Default: 1\n\
  -ncol n          Number of columns. Default: 1\n\
  -nsp n           Add n spaces ahead of every line. Default: 0\n\
  -self            Output file = Input file.\n\
  -setup {file}    Output the given setup file first. Default: None.\n\
  -wcol n          Column width. Default: 80\n\
\n"
"Author: Jean-Francois Larvoire"
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);
    exit(1);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    detab						      |
|									      |
|   Description:							      |
|									      |
|   Parameters: 							      |
|									      |
|   Returns:								      |
|                                                                             |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

int detab(char *line, int length, int tab)
    {
    char templine[BUFSIZE];
    int i, j, itab;

    memcpy(templine, line, BUFSIZE);

    for (i=j=itab=0; j<length-tab; i++)
        {
        switch(templine[i])
            {
            case '\t':
                while (itab++<tab) line[j++] = ' ';
                itab = 0;
                break;
            default:
                line[j++] = templine[i];
                itab += 1;
                break;
            }
        if (itab >= tab) itab = 0;
        if (!templine[i]) break;
        }
    line[length-1] = '\0';

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    output_line 					      |
|									      |
|   Description:							      |
|									      |
|   Parameters:	    int np		Current page number modulo fptp	      |
|		    int nl		Current line number modulo lpp+extra  |
|		    char *format	String format			      |
|		    char *text		String to display		      |
|		    FILE *fdest		Output file			      |
|									      |
|   Returns:								      |
|                                                                             |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

int output_line(int np, int nl, char *format, char *text, FILE *fdest)
    {
    char buf[BUFSIZE+20];
    int i;
    char format1[10];

    np %= ncols;

    if (ncols == 1)     /* If only one column, do things simply */
        {
        return fprintf(fdest, format, text);
        }
    else
        {
        if (np < ncols - 1)     /* For all but the last column, accumulate */
            {
            i = sprintf(buf, format, text);         /* Build line to output */
            buf[i-1] = '\0';                        /* Remove the trailing \n */

            i = wcols + dcols;                      /* Column total width */
            sprintf(format1, "%%-%ds", i);          /* Left justify in column */

            i *= (nl * ncols) + np;                 /* Index in buffer */
            return sprintf(buffer+i, format1, buf); /* Store line in buffer */
            }
        else                /* Output accumulated columns and last column */
            {
            i = nl * ncols * (wcols + dcols);   /* Index of accumulated lines */
            fprintf(fdest, "%s", buffer+i);
            return fprintf(fdest, format, text);
            }
        }
    }
