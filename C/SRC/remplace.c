/*****************************************************************************\
*		    							      *
*   Filename	    remplace.c						      *
*		    							      *
*   Description	    A program that replaces a string by another one	      *
*		    							      *
*   Notes	    Adapted from detab.c.				      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug	# Debug ver.  *
*		    							      *
*   History								      *
*    1986-11-05 JFL jf.larvoire@hp.com created this program, based on detab.c.*
*    1992-03-11 JFL Adapted to Microsoft C.				      *
*		    Modified to use binary mode for input and output.	      *
*		    Version 1.1 					      *
*    1992-10-24 JFL Cleaned up warnings.				      *
*		    Changed default to non pipe mode.			      *
*		    Added ability to read and write to the same file.	      *
*		    Improved usage help.				      *
*		    Recompiled in dual mode DOS+OS/2.			      *
*		    Version 1.2 					      *
*    1995-03-06 JFL Clarified the help message. 			      *
*		    Version 1.21					      *
*    1996-05-06 JFL Added support for wild cards.			      *
*		    Version 1.3 					      *
*    1996-06-19 JFL Updated help screen to describe how to manage dble quotes.*
*		    Version 1.31					      *
*    1996-06-24 JFL Added /= option to remove mime =XX encodings.	      *
*		    Version 1.4 					      *
*    1997-04-14 JFL Removed MSVC 1.x specific references to _osfile[].	      *
*		    Version 1.41					      *
*    1999-08-03 JFL Don't output a space when merging MIME lines.             *
*		    Version 1.43					      *
*    2001-11-06 JFL Added options -q, -nb, -same. Count the number of changes.*
*		    Version 1.5 					      *
*    2002-11-28 JFL Fixed a bug in wildcard mode: Incorrect data was output   *
*		    in case of a partial match.				      *
*		    Fixed another bug in case of a partial match: Some actual *
*		    matching strings were then missed.			      *
*		    Version 1.51					      *
*    2003-09-08 JFL Reversed the order of strings and file names.             *
*		    Removed the option -pipe. Now use stdin and stdout by     *
*		    default if no name is specified.			      *
*		    Version 1.6						      *
*    2003-09-23 JFL Fixed bug with -=. Added support for Unix-style files.    *
*		    Version 1.61					      *
*    2004-01-13 JFL Fixed bug with -same. 				      *
*		    Version 1.62					      *
*    2004-02-16 JFL Updated the usage message about " and \ parsing.          *
*    2004-04-01 JFL Rewrote the argument parsing to process \ with standard C *
*		    conventions. Added [] sets and ?+* regular expressions.   *
*    2004-04-02 JFL Added the support of NUL characters. 		      *
*                   Added escaping of special characters '.' and '['.         *
*                   Version 2.0   					      *
*    2004-05-28 JFL Removed debug code forgotten in the -= option. v2.01.     *
*    2004-09-27 JFL Return 0=Success; 1=No change done; 2=Error. v2.02.       *
*    2005-06-10 JFL Fixed a longstanding bug, with backtracking that did not  *
*                    work when inputing from stdin. Version 2.03.             *
*    2005-06-16 JFL Added a Unix version. Updated help. Version 2.04.         *
*    2005-09-22 JFL Fixed a bug when a pattern ends with .*                   *
*                   Added -i option to get the input string on the cmd line.  *
*		    Version 2.1.					      *
*    2005-09-22 JFL Added -% option to decode %NN URL codes.                  *
*		    Updated option -i to default input to the nul device.     *
*		    Use - to specify stdin or stdout instead of a file name.  *
*		    Added options -- and -. .				      *
*		    Version 2.2.					      *
*    2008-12-02 JFL Added -# option to stop parsing arguments.                *
*		    Version 2.21.					      *
*    2008-12-10 JFL Fixed a bug introduced on 12/02.                          *
*		    Updated the debug code to display \x sequences if needed. *
*		    Version 2.22.					      *
*    2010-12-19 JFL Flush every line of output on pipes and consoles.	      *
*		    Useful to see output in real time in long complex cmds.   *
*		    Version 2.23.					      *
*    2011-01-10 JFL Added the -t option to copy the input file time.          *
*		    Fixed the handling of - for the output to stdout.	      *
*		    Removed routine stcd_i, using atoi instead.		      *
*		    Removed the obsolete -w option, and added the -f option.  *
*		    Version 2.3.					      *
*    2011-09-27 JFL Display the number of changes in verbose only.            *
*		    Version 2.3.1.					      *
*    2012-02-29 JFL Added the ability to output the input match where \0 is   *
*		    in the new pattern.					      *
*		    Use my new common debugging macros.			      *
*		    Fixed a bug that caused the DOS version to crash.	      *
*		    Version 2.4.					      *
*    2012-03-12 JFL Added support for negative sets, like [^abc].             *
*		    Version 2.4.1.					      *
*    2012-03-17 JFL Moved utime.h to the standard Unix location.              *
*		    Version 2.4.2.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.4.3.                 *
*    2013-12-16 JFL Added max definition for MinGW.                           *
*    2014-06-24 JFL Corrected the help for the \0 regex replacement.          *
*		    Version 2.4.4.					      *
*    2014-12-03 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 2.4.5.					      *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 2.4.6.  					      *
*    2016-06-21 JFL Added regular expression ranges, like [a-z].              *
*		    Version 2.5.  					      *
*    2016-07-04 JFL Fixed an error in MSDOS help.			      *
*		    Version 2.5.1.  					      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.5.1"
#define PROGRAM_DATE    "2016-07-04"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#define _POSIX_SOURCE /* Force Linux to define fileno in stdio.h */
#define _BSD_SOURCE /* Force Linux to define S_IFREG in sys/stat.h */
#define _LARGEFILE_SOURCE64 1 /* Force using 64-bits file sizes if possible */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>

#define SZ 80               /* Strings size */

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)
#define strieq(string1, string2) (stricmp(string1, string2) == 0)

/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS			/* Define global variables used by our debugging macros */

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#include <direct.h>
#include <io.h>

#define DEVNUL "NUL"

#define stricmp _stricmp

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#define TARGET_MSDOS
#define OS_NAME "DOS"

#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>

#define DEVNUL "NUL"

/* Workaround for a linker bug in DOS, which is case independant.
   So for DOS, FGetC is the same as fgetc. */
#define FGetC FGetC1
#define FPutC FOutC1
#define FSeek FSeek1
#define FWrite FWrite1

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif

#define getch getchar
#define stricmp strcasecmp

#include <unistd.h>

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

#define DEVNUL "/dev/nul"

#endif

/************************* MinGW-specific definitions ************************/

#ifdef __MINGW32__     /* Both MinGW32 and MinGW64 */

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

#endif

/********************** End of OS-specific definitions ***********************/

#ifdef _MSC_VER
#pragma warning(disable:4001) /* Ignore the "nonstandard extension 'single line comment' was used" warning */
#endif

/* Global variables */

int iVerbose = FALSE;
FILE *mf;			    /* Message output file */

/* Forward references */

char *version(void);		    /* Build the version string */
void usage(int);
int is_redirected(FILE *f);	    /* Check if a file handle is the console */
int GetEscChar(char *pszIn, char *pc); /* Get one escaped character */
int GetRxCharSet(char *pszOld, char cSet[256], int *piSetSize, char *pcRepeat);
int GetEscChars(char *pBuf, char *pszFrom, size_t iSize);
void InitBackBuf(char *psz0);
int FGetC(FILE *f);
int FSeek(FILE *f, long lOffset, int iOrigin);
int FPutC(int c, FILE *f);
size_t FWrite(const void *buf, size_t size, size_t count, FILE *f);
char *EscapeChar(char *pBuf, char c);
int PrintEscapeChar(FILE *f, char c);
int PrintEscapeString(FILE *f, char *pc);
void MakeRoom(char **ppOut, int *piSize, int iNeeded);
int MergeMatches(char *new, int iNewSize, char *match, int nMatch, char **ppOut);

/*****************************************************************************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Main program routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the BIOS, if run from ROM.     |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
    {
    int c;                  /* Current character */
    char old[SZ] = "";	    /* old string, to be replaced by the new string */
    char new[SZ] = "";	    /* New string, to replace the old string */
    char *maybe=malloc(SZ); /* Possible matching input */
    int iMaybeSize = SZ;
    int oldDone, newDone;
    int iNewSize = 0;	    /* length of the new string */
    int ixOld;              /* Index in the old string */
    int ixMaybe = 0;	    /* Index in the maybe string */
    FILE *sf, *df;
    int i;
    char *pszInName = NULL;
    char *pszOutName = NULL;
    char *pc;
    char szBakName[FILENAME_MAX+1];
    int sameFile = FALSE;   // Backup the input file, and modify it in place.
    int keepTime = FALSE;   // If true, set the out file time = in file time.
    struct stat sInTime = {0};
    int demime = FALSE;
    long lnChanges = 0;	    // Number of changes done
    int iQuiet = FALSE;
    int iNoBackup = FALSE;
    char cSet[256];	    // Character set to match
    int iSetSize;	    // Number of valid characters in the set.
    char cRepeat = '\0';    // Repeat character. Either '?', '+', '*', or NUL.
    int iOptionI = FALSE;   // TRUE = -i option specified
    int iEOS = FALSE;	    // TRUE = End Of Switches

    mf = stdout; 		/* Assume output on stdout will be visible */
    if (is_redirected(mf))	/* If error and stdout redirected... */
	mf = stderr;			/* ... then use stderr */

    oldDone = newDone = FALSE;
    sf = df = NULL;

    /* Force stdin and stdout to untranslated */
#if defined(_MSDOS) || defined(_WIN32)
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    _setmode( _fileno( stderr ), _O_BINARY );
#endif

    for (i=1; i<argc; i++)          /* Process all command line arguments */
        {
	if (iEOS) goto is_arg;		    // End Of Switches already reached.
        if (   (*(argv[i]) == '/')          /* Process switches first */
            || (*(argv[i]) == '-') )
            {
	    if (strieq(argv[i], "-")) goto is_arg;
	    if (   strieq(argv[i]+1, "?")
	        || strieq(argv[i]+1, "h")
	        || strieq(argv[i]+1, "-help"))
                {
		usage(0);
                }
	    if (strieq(argv[i]+1, "-"))		/* End of switches */
		{
		iEOS = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "."))		/* No operation */
		{
		oldDone = TRUE;
		newDone = TRUE;
		continue;
		}
	    if (strieq(argv[i]+1, "="))		/* Decode Mime =XX codes */
		{
		demime = '=';
		oldDone = TRUE;
		newDone = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "%"))		/* Decode URL %XX codes */
		{
		demime = '%';
		oldDone = TRUE;
		newDone = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "#"))		/* End of command line */
		{ /* Useful for adding comments in a Windows pipe */
		break;
                }
#ifdef _DEBUG
	    if (strieq(argv[i]+1, "d"))
		{
		DEBUG_ON();
		iVerbose = TRUE;
		continue;
                }
#endif
	    if (strieq(argv[i]+1, "f"))		/* Fixed string <==> no regexp */
		{
		cRepeat = '\xFF';
		continue;
                }
	    if (strieq(argv[i]+1, "i"))
		{
		iOptionI = TRUE;
		InitBackBuf(argv[++i]);
		continue;
                }
	    if (strieq(argv[i]+1, "nb"))
		{
		iNoBackup = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "pipe"))	/* Now the default. Left for compatibility with early version. */
                {
		sf = stdin;
		df = stdout;
		continue;
                }
	    if (strieq(argv[i]+1, "q"))
		{
		iQuiet = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "same"))
		{
		sameFile = TRUE;
		continue;
                }
	    if (strieq(argv[i]+1, "t"))
		{
		keepTime = TRUE;
		continue;
                }
	    if (streq(argv[i]+1, "v"))
		{
		iVerbose = TRUE;
		continue;
                }
	    if (streq(argv[i]+1, "V"))
		{
		printf("%s\n", version());
		exit(0);
                }
	    /* Default: Assume it's not a switch, but a string to replace */
	    }
is_arg:
	if (!oldDone)
            {
	    strncpy(old, argv[i], sizeof(old));
	    oldDone = TRUE;
            continue;
            }
	if (!newDone)
            {
	    iNewSize = GetEscChars(new, argv[i], sizeof(new));
	    newDone = TRUE;
            continue;
            }
	if (!sf)
	    {
	    if (streq(argv[i], "-"))
		{
		sf = stdin;
		continue;
		}
	    sf = fopen(argv[i], "rb");
	    if (!sf)
		{
		fprintf(stderr, "Can't open file %s\r\n", argv[i]);
		exit(2);
		}
	    pszInName = argv[i];
	    stat(pszInName, &sInTime);
            continue;
            }
	if (!df)
	    {
	    if (streq(argv[i], "-"))
		{
		df = stdout;
		continue;
		}
	    if (strieq(argv[i], pszInName))
		{
		sameFile = TRUE;
		continue;
		}
	    sameFile = FALSE; // Just in case we got contradictory arguments.
	    df = fopen(argv[i], "wb");
	    if (!df)
		{
		fprintf(stderr, "Can't open file %s\r\n", argv[i]);
		exit(2);
		}
	    pszOutName = argv[i];
            continue;
            }
	usage(2);		    /* Error: Too many arguments */
        }

    if (!oldDone && !demime) usage(2);

    if (sameFile)
	{
	if (!sf) usage(2);
	fclose(sf);

	strcpy(szBakName, pszInName);
	pc = strrchr(szBakName, '.');
	if (pc && !strchr(pc, '\\'))	/* If extension in name & not in path */
	    strcpy(pc, ".bak"); 	/* Change extension to .bak */
	else
	    strcat(szBakName, ".bak");	/* Set extension to .bak */
	unlink(szBakName); 		/* Remove the .bak if already there */
	rename(pszInName, szBakName);	/* Rename the source as .bak */

	sf = fopen(szBakName, "rb");
	if (!sf)
	    {
	    fprintf(stderr, "Can't open file %s\r\n", szBakName);
	    exit(2);
	    }
	df = fopen(pszInName, "wb");
	if (!df)
	    {
	    fclose(sf);
	    fprintf(stderr, "Can't open file %s\r\n", pszInName);
	    exit(2);
	    }
	pszOutName = pszInName;
	}

    if (!sf)
	{
	if (iOptionI)
	    {
	    sf = fopen(DEVNUL, "rb");
	    if (!sf)
		{
		fprintf(stderr, "Can't open file %s\r\n", DEVNUL);
		exit(2);
		}
	    pszInName = DEVNUL;
	    }
	else
	    {
	    sf = stdin;
	    }
	}
    if (!df)
	{
	df = stdout;
	}

    if (iVerbose)
	{
	if (demime && !iQuiet) fprintf(mf, "Replacing Mime %cXX codes.\r\n", demime);
	if (old[0] && !iQuiet) 
	    {
	    fprintf(mf, "Replacing \"%s\" with \"", old);
	    PrintEscapeString(mf, new);
	    fprintf(mf, "\".\r\n");
	    }
	}

    ixOld = 0;
    ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
    ixMaybe = 0;

    while ((c = FGetC(sf)) != EOF)	 /* Read chars until End of file */
	{
	if (demime && (c == demime))
	    {
	    char c0, c1, sz[3];
	    int ic;

	    if ((c0 = (char)FGetC(sf)) == EOF)
		{
		FPutC(c, df);
		break;
		}
	    /* At the end of a line, it signals a broken line. Merge halves. */
	    if (c0 == 0x0A)
		{
		// ~~jfl 2003-09-23 Added support for Unix-style files. Don't output anything.
		lnChanges += 1;
		continue;
		}
	    if ((c1 = (char)FGetC(sf)) == EOF)
		{
		FPutC(c, df);
		FPutC((char)c0, df);
		break;
		}
	    /* At the end of a line, it signals a broken line. Merge halves. */
	    if ((c0 == 0x0D) && (c1 == 0x0A))
		{
		// ~~jfl 1999-08-02 Don't output anything.
		lnChanges += 1;
		continue;
		}
	    /* Else it's an ASCII code */
	    sz[0] = (char)c0;
	    sz[1] = (char)c1;
	    sz[2] = '\0';
            DEBUG_FPRINTF((mf, "Found code %s: ", sz));
	    if (sscanf(sz, "%X", &ic))
		{
                DEBUG_FPRINTF((mf, "Changed to char %c.\n", ic));
		FPutC((char)ic, df);
		lnChanges += 1;
		}
	    else
		{
                DEBUG_FPRINTF((mf, "Not a valid code.\n"));
		FPutC('=', df);
		FWrite(sz, 2, 1, df);
		}
	    continue;
	    }

try_next_set:
        DEBUG_CODE_IF_ON(
	    char cBuf[8];
	    fprintf(mf, "Trying to match '%s' in set \"", EscapeChar(cBuf, (char)c));
	    for (i=0; i<iSetSize; i++) PrintEscapeChar(mf, cSet[i]);
	    fprintf(mf, "\" %c\n", cRepeat);
	    );

	if (memchr(cSet, c, iSetSize))		/* If c belongs to the old string */
            {
            DEBUG_FPRINTF((mf, "Match! Next is old[%d]='%c'\n", ixOld, old[ixOld]));

	    MakeRoom(&maybe, &iMaybeSize, ixMaybe+1); // If needed, extend the buffer
	    maybe[ixMaybe++] = (char)c;		/* Save the matching character */
            if (cRepeat == '?') cRepeat = '\0'; /* We've found it. No more expected. */
            if (cRepeat == '+') cRepeat = '*';  /* We've found it. More possible. */
            if (cRepeat == '*') continue;
            if (!old[ixOld])                    /* and if it is the last char. of the string to replace */
                {
                char *new2;
                int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
		FWrite(new2, iNewSize2, 1, df);       /* then write new string */
		free(new2);
		lnChanges += 1;
                ixOld = 0;                      /* and start over again. */
                ixMaybe = 0;
                }
            }
        else                    /* Else it is an unexpected char. */
            {
            DEBUG_FPRINTF((mf, "No match. Next is old[%d]='%c'\n", ixOld, old[ixOld]));

            if ((cRepeat == '?') || (cRepeat == '*'))
		{
		if (old[ixOld])
		    {
		    ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
		    goto try_next_set;
		    }
		else		    /* The set was complete. Write the new string */
		    {
		    char *new2;
		    int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
		    FWrite(new2, iNewSize2, 1, df);       /* then write new string */
		    free(new2);
		    lnChanges += 1;
		    }
		ixMaybe = 0;
		}
            if (ixMaybe)                    /* If there were pending characters */
                {
		// ~~jfl 2002-11-28 In case of a partial match, output 1 character only, and look for another match starting on the next.
		FSeek(sf, -ixMaybe, SEEK_CUR); // Backtrack to the 2nd input character
		MakeRoom(&maybe, &iMaybeSize, ixMaybe+1); // If needed, extend the buffer
		maybe[ixMaybe++] = (char)c;
		c = maybe[0];
                }
            ixOld = 0;
            ixMaybe = 0;
	    FPutC(c, df);	    /* Then output the given character. */
            }
	ixOld += GetRxCharSet(old+ixOld, cSet, &iSetSize, &cRepeat);
        }
    DEBUG_FPRINTF((mf, "End of file. Flushing remainders.\n"));
    if (((cRepeat == '?') || (cRepeat == '*')) && !old[ixOld])
	{	/* The set was complete. Write the new string */
	char *new2;
	int iNewSize2 = MergeMatches(new, iNewSize, maybe, ixMaybe, &new2);
	FWrite(new2, iNewSize2, 1, df);       /* then write new string */
	free(new2);
	lnChanges += 1;
	}
    else if (ixOld) FWrite(maybe, ixMaybe, 1, df); /* Flush an uncompleted old string */

    fclose(sf);
    fclose(df);

    if (sameFile)
	{
	if (iNoBackup) unlink(szBakName);     /* Optionally don't keep a backup */
	}

    if (pszInName && pszOutName && keepTime)
	{
	struct utimbuf sOutTime = {0};
	sOutTime.actime = sInTime.st_atime;
	sOutTime.modtime = sInTime.st_mtime;
	utime(pszOutName, &sOutTime);
	}

    if (iVerbose) fprintf(mf, "Remplace: %ld changes done.\r\n", lnChanges);

    return ((lnChanges>0) ? 0 : 1);              /* and exit */
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
*									      *
\*---------------------------------------------------------------------------*/

#ifndef S_IFIFO
#define S_IFIFO         0010000         /* pipe */
#endif

int is_redirected(FILE *f)
    {
    int err;
    struct stat buf;			/* Use MSC 6.0 compatible names */
    int h;

    h = fileno(f);			/* Get the file handle */
    err = fstat(h, &buf);		/* Get information on that handle */
    if (err) return FALSE;		/* Cannot tell more if error */
    return (   (buf.st_mode & S_IFREG)	/* Tell if device is a regular file */
            || (buf.st_mode & S_IFIFO)	/* or it's a FiFo */
	   );
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help screen, and exit.		      |
|									      |
|   Parameters:     int err		    The exit code to pass to the OS.  |
|									      |
|   Returns:	    N/A 						      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
	  );
}

void usage(int err)
    {
    FILE *f;

    f = stdout; 		/* Assume output on stdout will be visible */
    if (err && is_redirected(f))	/* If error and stdout redirected... */
	f = stderr;			/* ... then use stderr */

    /* Note: The help is too long, and needs to be split into several sub strings */
    /*       Also be careful of the % character that appears in some options */
    fprintf(f, "\
\r\n\
remplace version %s - Replace substrings in a stream\r\n\
\r\n\
Usage: remplace [switches] {operation} [files_spec]\r\n\
\r\n\
files_spec: [input_file [output_file]]\r\n\
  Default input_file: standard input\r\n\
  Default output_file: standard output\r\n", version());
    fprintf(f, "%s", "\
\r\n\
operation: {old_string new_string}|-=|-%|-.\r\n\
  -=      Decode Mime =XX codes.\r\n\
  -%      Decode URL %XX codes.\r\n\
  -.      No change.\r\n\
\r\n\
switches:\r\n\
  -#      Ignore all further arguments.\r\n\
  -?      Display this brief help screen.\r\n\
  --      End of switches.\r\n"
#ifdef _DEBUG
"  -d      Output debug information\r\n"
#endif
"  -f      Fixed old string = Disable the regular expression subset supported.\r\n\
  -i txt  Input text to use before input file, if any. (Use - for force stdin)\r\n\
  -nb     keep no backup if in_file==out_file. Default: Rename as .bak.\r\n\
  -q      Quiet mode. No status message.\r\n\
  -same   Modify the input file in place.\r\n\
  -t      Set the output file time equal to the initial input file time.\r\n\
  -v      Verbose mode.\r\n\
\r\n\
Examples:\r\n"
);
    fprintf(f,
#if defined _MSDOS || defined _WIN32
"  remplace \\n \\r\\n <unixfile >dosfile\r\n\
  remplace -t \\n \\r\\n unixfile -same\r\n\
  remplace -= unreadable_mime_file\r\n\
  remplace \\CHICAGO \\WIN95 config.sys -same -nb\r\n\
\r\n\
Note that the MSVC command line parser interprets quotes itself this way:\r\n\
Characters surrounded by \"s are parsed as a single argument. \"s are removed.\r\n\
Use \\\" to enter a \". \\ series are used literally, unless followed by a \".\r\n\
In that case, the \\s and \" are treated as by a C compiler.\r\n\
Special characters: Use \\r for CR, \\n for LF, \\x3C for <, \\x3E for >.\r\n\
Use the verbose mode to see how quotes and backslashes went through.\r\n"
#endif
#if defined __unix__
"  remplace \\\\n \\\\r\\\\n <unixfile >dosfile\r\n\
  remplace -t \\\\n \\\\r\\\\n unixfile -same\r\n\
  remplace -= unreadable_mime_file\r\n\
  remplace New-York \"Big apple\" catalog -same -nb\r\n\
\r\n\
Note that the Unix shells interpret quotes and backslashes themselves. Hence\r\n\
the double backslashes in the examples.\r\n\
Special characters: Use \\\\r for CR, \\\\n for LF, \\\\x3C for <, \\\\x3E for >.\r\n\
Use the verbose mode to see how quotes and backslashes went through.\r\n"
#endif
"\r\n\
Regular expressions subset for the old_string:\r\n\
  .     Matches any character.\r\n\
  c?    Matches 0 or 1 occurence of c.\r\n\
  c*    Matches 0 or plus occurences of c.\r\n\
  c+    Matches 1 or plus occurences of c.\r\n\
  [abc] Matches any of the enumerated characters. Use [[] to match one [.\r\n\
  [a-z] Matches any character in the specified range.\r\n\
  [^ab] Matches all but the enumerated characters.\r\n"
#if defined _MSDOS || defined _WIN32
"        Warning: ^ is cmd prompt escape character. Double it if needed.\r\n"
#endif
"\r\n\
The new string may contain the following special sequences\r\n\
  \\\\    Replaced by a single \\\r\n\
  \\\\0   Replaced by the current matching input\r\n\
\r\n\
Return code: 0=Success; 1=No change done; 2=Error.\r\n\
\r\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-Fran√ßois Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\r\n"
#ifdef __unix__
"\r\n"
#endif
);

    exit(err);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetEscChar						      |
|									      |
|   Description:    Get one escaped character from a string.		      |
|									      |
|   Parameters:     char *pszIn		The string to parse	 	      |
|		    char *pc		The characters found                  |
|									      |
|   Returns:	    The number of characters read from pszIn.                 |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    2004-04-02 JFL Created this routine.                                     |
*									      *
\*---------------------------------------------------------------------------*/

int GetEscChar(char *pszIn, char *pc)
    {
    char *psz0 = pszIn;
    char c = *(pszIn++);
    int i;

    if (c == '\\') switch(c = *(pszIn++))
        {
        case '\0':			// End of string
            *pc = '\\';
            pszIn -= 1;
            break;
        case '\\':			// Litteral \ .
            *pc = c;
            break;
        case '0':			// NUL
            *pc = '\0';
            break;
        case 'a':			// Alert (Bell)
            *pc = '\a';
            break;
        case 'b':			// Backspace
            *pc = '\b';
            break;
        case 'e':			// Escape
            *pc = '\x1B';
            break;
        case 'f':			// Form feed
            *pc = '\f';
            break;
        case 'n':			// New Line
            *pc = '\n';
            break;
        case 'r':			// Return
            *pc = '\r';
            break;
        case 't':			// Tabulation
            *pc = '\t';
            break;
        case 'v':			// Vertical tabulation
            *pc = '\v';
            break;
        case 'x':			// Hexadecimal character
            sscanf(pszIn, "%2X", &i);	/* convert next 2 chars */
            pszIn += 2;			/* Count them */
            *pc = (char)i;
            break;
        default:			// Anything else: Preserve the characters.
            *pc = c;
            break;
        }
    else
        {
	*pc = c;
	}

    return (int)(pszIn - psz0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetRxCharSet					      |
|									      |
|   Description:    Get a character set from the old string.		      |
|									      |
|   Parameters:     char *pszOld	Old string		 	      |
|		    char cSet[256]	Where to store the output characters. |
|                   int iSetBufSize     Size of the cSet buffer.              |
|                   int *piSetSize      Size of the output set.               |
|                   char *pcRepeat	The optional ?+* repetition character.|
|					or \xFF to disable regexp mechanism.  |
|									      |
|   Returns:	    The number of characters read from pszOld.                |
|									      |
|   Notes:	    This is a subset of the regular expressions syntax.	      |
|									      |
|   History:								      |
|    2004-04-01 JFL Created this routine.                                     |
|    2011-01-10 JFL Renamed from GetEscCharSet to GetRxCharSet.		      |
|		    Added the option to disable this regexp mechanism.        |
|    2012-03-12 JFL Added support for excluded character sets ([^xyz]).       |
*		    							      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int GetRxCharSet(char *pszOld, char cSet[256], int *piSetSize, char *pcRepeat)
    {
    char c;
    int n = 0;
    char *psz0 = pszOld;
    char negative;
    int i;
    int hasNul;

    if (*pcRepeat != '\xFF')
	{
	switch (c = *pszOld)
	    {
	    case '[':	// '[' is the beginning of a set of characters
		pszOld += 1;
		memset(cSet, 0, 256);
		negative = 0; // 1=This is a negative set, like [^abc]
		if (*pszOld == '^') { negative = 1; pszOld += 1; }
		if (*pszOld == ']') { cSet[(unsigned char)']'] = 1; pszOld += 1; }
		while ((c = *pszOld)) {
		  pszOld += GetEscChar(pszOld, &c);
		  if (c == ']') break;
		  cSet[(unsigned char)c] = 1;
		  if (*pszOld == '-') { // If this is the beginning of a range
		    char cLast = c; // Avoid problem if there's no end.
		    pszOld += 1; // Skip the -
		    pszOld += GetEscChar(pszOld, &cLast);
		    if (cLast < c) cLast = c; // Avoid problem if ends are reversed.
		    for (c++ ; c <= cLast; c++) {
		      cSet[(unsigned char)c] = 1;
		    }
		  }
		}
		if (negative) for (i=0; i<256; i++) cSet[i] = (char)!cSet[i];
		hasNul = cSet[0];
		for (i=1, n=0; i<256; i++) if (cSet[i]) cSet[n++] = (char)i;
		if (hasNul) cSet[n++] = '\0';  // Ensure the NUL is the last character, so that the debug output below does not break.
		break;
	    case '.':	// '.' matches any character
		pszOld += 1;
		for (n=0; n<256; n++)
		    {
		    cSet[n] = (char)(n+1); // Ensure the NUL is the 256th character, so that the debug output below does not break.
		    }
		break;
	    default:	// Normal character
		pszOld += GetEscChar(pszOld, cSet);
		n = 1;
		break;
	    }
	*piSetSize = n;
	if (n<256) cSet[n] = '\0';	// Convenience to make it look like a string in most cases.    
    
	// Now look for the optional ?+* repetition character behind.
	c = *pszOld;
	switch (c)
	    {
	    case '?':
	    case '+':
	    case '*':
		*pcRepeat = c;
		pszOld += 1;
		break;
	    case '\0':
	    default:
		*pcRepeat = '\0';
		break;
	    }
	}
    else /* The regexp mechanism is disabled */
	{
	c = *(pszOld++);
	cSet[0] = c;
	*piSetSize = 1;
	}

#if 0
    DEBUG_CODE_IF_ON(
	fprintf(mf, "Extracted set \"");
	PrintEscapeString(mf, cSet);
	fprintf(mf, "\" %c, return %d\n", *pcRepeat, pszOld - psz0);
    );
#endif

    return (int)(pszOld - psz0);
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetEscChars         				      |
|									      |
|   Description:    Convert an argument string, processing \ escape sequences.|
|									      |
|   Parameters:     char *pBuf		    Where to store the output string  |
|                   char *pszFrom	    argument to process 	      |
|		    int iBufSize            Output buffer size                |
|									      |
|   Returns:	    The number of characters output			      |
|									      |
|   Notes:	    The output is not a string. It's a set of bytes.	      |
|									      |
|   History:								      |
|    2004-04-01 JFL Rewrote the argument parsing to process \ with the        |
|		    standard C conventions.				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetEscChars(char *pBuf, char *pszFrom, size_t iSize)
    {
    char *pBuf0 = pBuf;

    while (*pszFrom)
	{
	if (!iSize) break;
	pszFrom += GetEscChar(pszFrom, pBuf++);
	iSize -= 1;
        }
    if (iSize) *pBuf = '\0';	// Convenience to make it look like a string in most cases.

    return (int)(pBuf - pBuf0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    EscapeChar	         				      |
|									      |
|   Description:    Convert a character into an \ escape sequence if needed.  |
|									      |
|   Parameters:     char *pBuf		    Where to store the output string  |
|                   char c		    character to convert	      |
|									      |
|   Returns:	    pBuf                            			      |
|									      |
|   Notes:	    The output buffer must be at least 5 characters long.     |
|									      |
|   History:								      |
|    2008-12-10 JFL Created this routine.                                     |
*									      *
\*---------------------------------------------------------------------------*/

char *EscapeChar(char *pBuf, char c)
    {
    if ((c < ' ') || ((unsigned char)c > (unsigned char)'\x7F'))
	sprintf(pBuf, "\\x%02X", (unsigned char)c);
    else if (c == '\\') 
	strcpy(pBuf, "\\\\");
    else
	sprintf(pBuf, "%c", c);

    return pBuf;
    }

int PrintEscapeChar(FILE *f, char c)
    {
    char buf[8];

    return fprintf(f, "%s", EscapeChar(buf, c));
    }

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int PrintEscapeString(FILE *f, char *pc)
    {
    int n = 0;
    char c;

    while ((c = *(pc++))) n += PrintEscapeChar(f, c);

    return n;
    }

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FGetC	         				      |
|									      |
|   Description:    Get a character from a file stream.			      |
|									      |
|   Parameters:     FILE *f		    The stream handle		      |
|									      |
|   Returns:	    The character read, or EOF.				      |
|									      |
|   Notes:	    This is a front end to the standard C library fgetc().    |
|		    The goal is to allow limited backtracking even on the     |
|		     standard input stream.				      |
|									      |
|   History:								      |
|    2005-06-10 JFL Created this routine.				      |
|    2008-12-02 JFL Fixed a bug in FSeek, which would not detect seek errors. |
|    2008-12-10 JFL Updated the fix, which was buggy.                         |
*									      *
\*---------------------------------------------------------------------------*/

char *pszBackBuf = NULL;
char *pszBBEnd;
char *pszBBHead;
char *pszBBTail;
size_t lBB;	/* Length of back buffer */
size_t nBB;	/* Number of characters stored in the back buffer */
size_t ixBB;    /* Current position relative to (backwards from) the head */

char *BBMove(char *p)
{
  while (p < pszBackBuf) p += lBB;
  while (p >= pszBBEnd) p -= lBB;
  return p;
}

void InitBackBuf(char *psz0)
{
  ixBB = nBB = strlen(psz0);
  lBB = max(nBB, 1024);
  if (!pszBackBuf)
  {
    pszBackBuf = malloc(lBB);
    if (!pszBackBuf)
    {
      fprintf(stderr, "Error: not enough memory.\n");
      exit(1);
    }
  }
  pszBBEnd = pszBackBuf + lBB;
  strncpy(pszBackBuf, psz0, nBB);
  pszBBTail = pszBackBuf;
  pszBBHead = BBMove(pszBackBuf + nBB);
}

int FGetC(FILE *f)
{
//  if (f != stdin) return fgetc(f);
  if (!pszBackBuf) InitBackBuf("");
  if (!ixBB)
  {
    int iRet = fgetc(f);
    if (iRet == EOF) return iRet;
    if (nBB == lBB) pszBBTail = BBMove(pszBBTail + 1); // Drop one from the tail.
    *pszBBHead = (char)iRet;
    pszBBHead = BBMove(pszBBHead + 1);
    if (nBB < lBB) nBB += 1;
    return iRet;
  }
  else 
  {
    return *BBMove(pszBBHead-(ixBB--));
  }
}

int FSeek(FILE *f, long lOffset, int iOrigin)
{
// if (f != stdin) return fseek(f, lOffset, iOrigin);
  if (iOrigin == SEEK_CUR)
  {
    if (lOffset >= 0) 
    {
      long l;
      for (l=0; l<lOffset; l++) FGetC(f);
      return fseek(f, 0, SEEK_CUR);
    }
    ixBB += (size_t)-lOffset;
    if (ixBB <= nBB) return 0; /* Success */
    /* Else error, not enough data recorded for this move. */
  }
  InitBackBuf(""); /* Clear the whole back buffer */
  return 1; /* Error, unsupported */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FPutC	         				      |
|									      |
|   Description:    Output a character to a file stream.		      |
|									      |
|   Parameters:     char c		    The character to output	      |
|		    FILE *f		    The output stream handle	      |
|									      |
|   Returns:	    The character written, or EOF.			      |
|									      |
|   Notes:	    This is a front end to the standard C library fputc().    |
|		    The difference is that this one flushes the output at     |
|		    the end of every line for the console and pipes.	      |
|		    Useful to see output in real time in long complex cmds.   |
|		    Output to stdout is assumed to be to a consoles or pipe.  |
|									      |
|   History:								      |
|    2010-12-19 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int FPutC(int c, FILE *f) {
  int ret = fputc(c, f);
  if ((f == stdout) && (ret != EOF) && (c == '\n')) {
    fflush(f);
  }
  return ret;
}

size_t FWrite(const void *buf, size_t size, size_t count, FILE *f) {
  size_t ret = fwrite(buf, size, count, f);
  size *= ret; /* The total size actually written */
  if ((f == stdout) && size && memchr(buf, '\n', size)) {
    fflush(f);
  }
  return ret;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MakeRoom	         				      |
|									      |
|   Description:    Make sure a buffer is larger than a minimal size	      |
|									      |
|   Parameters:     							      |
|									      |
|   Returns:	    Nothing						      |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2012-02-29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

void MakeRoom(char **ppOut, int *piSize, int iNeeded) {
  if (iNeeded > *piSize) {
    *piSize = iNeeded+80;
    DEBUG_FPRINTF((mf, "Reallocing buffer 0x%p to size %u\n", *ppOut, *piSize));
    *ppOut = realloc(*ppOut, *piSize);
    if (!*ppOut) {
      fprintf(stderr, "Error: not enough memory.\n");
      exit(1);
    }
  }
  return;
}
  
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MergeMatches         				      |
|									      |
|   Description:    Build the output string based on regex and matches	      |
|									      |
|   Parameters:     							      |
|									      |
|   Returns:	    The size of the output string			      |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2012-02-29 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int MergeMatches(char *new, int iNewSize, char *match, int nMatch, char **ppOut) {
  char *pOut = malloc(SZ);
  int nOutSize = SZ;
  char *pc;
  int ixOut = 0;
  int n;
  
  DEBUG_ENTER(("MergeMatches(\"%s\", %d, \"%.*s\", %d, 0x%p);\n", new, iNewSize, nMatch, match, nMatch, ppOut));

  while (iNewSize) {
    pc = memchr(new, '\\', iNewSize);
    if (pc) {
      n = (int)(pc - new);
    } else {
      n = iNewSize;
    }
    if (n) {
      MakeRoom(&pOut, &nOutSize, ixOut+n);
      memcpy(pOut + ixOut, new, n);
      ixOut += n;
      new += n;
      iNewSize -= n;
    }
    if (pc) {
      MakeRoom(&pOut, &nOutSize, ixOut+2);
      switch (*(pc+1)) {
      case '0': { /* Replace \0 by the full matching string */
	memcpy(pOut + ixOut, match, nMatch);
	ixOut += nMatch;
	new = pc+2;
	iNewSize -= 2;
	break;
      }
      case '\\': /* Replace \\ by a single \ */
      	pOut[ixOut++] = '\\';
      	new += 2;
	iNewSize -= 2;
      	break;
      default: /* Let any other \? sequence fall through unchanged */
      	pOut[ixOut++] = *(new++);
      	iNewSize -= 1;
      	if (iNewSize) {
	  pOut[ixOut++] = *(new++);
	  iNewSize -= 1;
	}
	break;
      }
    }
  }
  *ppOut = pOut;

  DEBUG_LEAVE(("return %d; // \"%.*s\"\n", ixOut, ixOut, pOut));
  return ixOut;
}

