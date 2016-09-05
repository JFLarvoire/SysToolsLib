/*****************************************************************************\
*                                                                             *
*  Filename:	    whichinc.c						      *
*									      *
*  Description:     Find which include files are used by a given C source     *
*									      *
*  History:								      *
*    1995-05-02 JFL Created.						      *
*    2000-03-07 JFL Added the /v option.				      *
*    2001-03-22 JFL Added support for absolute pathnames.		      *
*		    Fixed bug causing a protection fault in WIN32.	      *
*		    Only display the files containing the searched name,      *
*		     if any. Document that second argument in help.	      *
*		    Version 1.2.					      *
*    2001-05-04 JFL Fixed cosmetic problem when an include file is not found. *
*		    Version 1.21.					      *
*    2009-02-24 JFL Added support for #[spaces]include and #[spaces]define.   *
*                   Allow specifying a .h in the INCLUDE path directly.       *
*		    Added option -i for adding INCLUDE paths		      *
*		    Version 1.3.					      *
*    2014-12-04 JFL Added my name and email in the help.                      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define streq(s1, s2) (!strcmp(s1, s2))
#define strieq(s1, s2) (!stricmp(s1, s2))

#define LINESIZE 256

typedef struct tagStringList
    {
    char *string;
    struct tagStringList *next;
    } StringList;

int iVerbose = FALSE;
int iDeltaShift = 2; /* # of characters to indent output per depth level */

char szNewIncludeList[4*1024] = "INCLUDE="; /* All include paths */

/* Redefine obsolete names */ 
#define strlwr _strlwr

/* Forward references */
void usage(int iRetCode);
StringList *WhichInc(char *pszName, int iShift, StringList *psl, void *pRef);

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function:   main                                                           |
|                                                                             |
|  Purpose:    main C routine                                                 |
|                                                                             |
|  Parameters: int argc          Number of arguments                          |
|              char *argv[]      Pointer to an array of pointers to arguments |
|                                                                             |
|  Return:     The program exit code                                          |
|                                                                             |
|  Creation:   1995/05/02 by JFL                                              |
|                                                                             |
|  Modification history:                                                      |
|  Date     Author   Description                                              |
|  -------- -------- ------------------------------------------------------   |
|                                                                             |
*                                                                             *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
    {
    int i;
    char *pszArgName = NULL;
    char szName[FILENAME_MAX];
    int iExtension = FALSE;
    char *pc;
    char *pszSearch = NULL;
    char *pszName = szName;
    char *pszInclude = getenv("INCLUDE");
    int iNewIncludes = FALSE;

    if (pszInclude) strcpy(szNewIncludeList+8, pszInclude);

    /* Get the command line arguments */

    for (i=1; i<argc; i++)
	{
	if ((argv[i][0] == '-') || (argv[i][0] == '/')) /* It's a switch */
	    {
	    strlwr(argv[i]);
	    if (   streq(argv[i]+1, "help")
	        || streq(argv[i]+1, "h")
	        || streq(argv[i]+1, "?"))
		{
		usage(0);
		}
	    if (   streq(argv[i]+1, "i")
	        || streq(argv[i]+1, "I"))
		{
		pszInclude = szNewIncludeList + strlen(szNewIncludeList);
		if (pszInclude > (szNewIncludeList+8)) *(pszInclude++) = ';';
		strcpy(pszInclude, argv[++i]);
		iNewIncludes = TRUE;
		continue;
		}
	    if (streq(argv[i]+1, "s"))
		{
		pszSearch = argv[++i];
		continue;
		}
	    if (streq(argv[i]+1, "v"))
		{
		iVerbose = TRUE;
		continue;
		}
	    printf("Unrecognized switch %s. Ignored.\n", argv[i]);
	    continue;
	    }
	if (!pszArgName)
	    {
	    pszArgName = argv[i];
	    continue;
	    }
	printf("Unexpected argument: %s\nIgnored.\n", argv[i]);
	break;	/* Ignore other arguments */
	}

    if (!pszArgName)
	{
	usage(1);
	}

    /* If we added new includes, update our copy of the environment */

    if (iNewIncludes) _putenv(szNewIncludeList);
    pszInclude = szNewIncludeList + 8;

    /* Add the default extension to the argument if needed */

    strcpy(szName, pszArgName);
    if ( (pc = strrchr(szName, '.')) && !(strchr(pc, '\\')) )
	/* If dot found and no backslash afterwards */
	{
	iExtension = TRUE;
	}
    if (!iExtension)	 /* Use the default extension */
	{
	strcat(szName, ".c");
	}

    /* If it's an include file, search it in the INCLUDE path */
    if ((_access(szName, 0) == -1) && !strchr(szName, '\\'))
	{
	char cBuf[FILENAME_MAX];
	if (iVerbose) printf("\tSearching %s in %s.\n", szName, pszInclude);
	_searchenv(szName, "INCLUDE", cBuf);
	if (!cBuf[0])
	    {
	    printf("File not found: %s\n", szName);
	    exit(1);
	    }
	pszName = cBuf;
	}

    /* Search include files */

    WhichInc(pszName, 0, NULL, pszSearch);

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function:   usage                                                          |
|                                                                             |
|  Purpose:    Display a brief online help and exit                           |
|                                                                             |
|  Parameters: int iRetCode      The program exit code                        |
|                                                                             |
|  Return:     Does not return                                                |
|                                                                             |
|  Creation:   1995/05/02 by JFL                                              |
|                                                                             |
|  Modification history:                                                      |
|  Date     Author   Description                                              |
|  -------- -------- ------------------------------------------------------   |
|                                                                             |
*                                                                             *
\*---------------------------------------------------------------------------*/

void usage(int iRetCode)
    {
    printf("\nWhichInc version 1.3\n\
\n\
Usage:\n\
\n\
whichinc [options] {filename}\n\
\n\
List the include files referenced by the C source {filename}.\n\
Uses the INCLUDE environment variable to find them.\n\
Default {filename} extension: .c\n\
\n\
Options:\n\
  -?            Display this help and exit.\n\
  -i {path}     Add path to the include list. (May be repeated)\n\
  -s {name}     Search where name is #defined, and display the definition.\n\
  -v            Display verbose information during search.\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);

    exit(iRetCode);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function:   WhichInc                                                       |
|                                                                             |
|  Purpose:    Scan a file for #include definitions                           |
|                                                                             |
|  Parameters: char *pszName     File name                                    |
|              int iShift        How much to shift the name displayed         |
|              StringList *psl   Pointer to the list of previous include files|
|              char *pszSearch   Definition to search                         |
|                                                                             |
|  Return:     StringList *      Pointer to the new head of the list          |
|                                                                             |
|  Creation:   1995/05/02 by JFL                                              |
|                                                                             |
|  Modification history:                                                      |
|  Date     Author   Description                                              |
|  -------- -------- ------------------------------------------------------   |
|                                                                             |
*                                                                             *
\*---------------------------------------------------------------------------*/

StringList *WhichInc(char *pszName, int iShift, StringList *psl, char *pszSearch)
    {
    FILE *sf;
    char cLine[LINESIZE];
    char *pc;
    StringList *pNewSL;
    StringList *ps;
    int iNamePrinted = FALSE;

    if (!pszSearch) printf("%*s%s", iShift, "", pszName);

    /* Prevent reopening (sometimes recursively) of a file already included */
    for (ps = psl; ps; ps = ps->next)
	{
	if (streq(pszName, ps->string))
	    {
	    if (!pszSearch) printf(" (Already included.)\n");
	    return psl;
	    }
	}

    /* Open the file */
    sf = fopen(pszName, "rb");
    if (!sf)
	{
        if (pszSearch) printf("%*s%s", iShift, "", pszName);
	printf(" (Not found. Aborting.)\n");
	exit(1);
	}

    if (!pszSearch) printf("\n");

    /* Add a new node ahead of the linked list of file names */
    pc = malloc(sizeof(StringList) + strlen(pszName) + 1);
    if (!pc)
	{
	printf("Not enough memory.\n");
	exit(2);
	}
    pNewSL = (StringList *)pc;
    pc += sizeof(StringList);
    strcpy(pc, pszName);
    pNewSL->string = pc;
    pNewSL->next = psl;
    psl = pNewSL;

    /* Scan the file for inclusions */
    while (fgets(cLine, LINESIZE, sf))
	{
	char cLine2[LINESIZE];
	char *pszToken;
	static char *pszBlanks = " \t\n\r";
	size_t l;

	memcpy(cLine2, cLine, LINESIZE);    /* Save the line read */
	l = strlen(cLine2);
	if ((l > 0) && (cLine2[l-1] == '\n'))
	    cLine2[--l] = '\0'; /* Remove the trailing new line */
	if (   (pszToken = strtok(cLine, pszBlanks))
	    && (   streq(pszToken, "#include")
	        || (   streq(pszToken, "#")
		    && (pszToken = strtok(NULL, pszBlanks))
		    && streq(pszToken, "include")
		   )
	       )
	   )
	    {
	    char *pszIncName;
	    char cBuf[FILENAME_MAX];

	    pszToken = strtok(NULL, pszBlanks);
	    pszIncName = strtok(pszToken, "\"<>");
	    if (iVerbose) printf("\tSearching %s in %s.\n", pszIncName, getenv("INCLUDE"));
	    if (pszIncName[0] != '\\') /* ~~JFL 2001/03/22 Added this test. */
	        _searchenv(pszIncName, "INCLUDE", cBuf);
	    else
	        _fullpath(cBuf, pszIncName, sizeof(cBuf));
	    if (!cBuf[0]) /* if Pathname not found, search also in same directory as parent. */
	        {
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath(pszName, drive, dir, fname, ext );
		sprintf(cBuf, "TEMP=%s%s", drive, dir);
                _putenv(cBuf);
	        _searchenv(pszIncName, "TEMP", cBuf);
	        }
	    if (cBuf[0]) /* if Pathname found */
		{
		/* open it now */
		psl = WhichInc(cBuf, iShift+iDeltaShift, psl, pszSearch);
		}
	    else /* strlen(cBuf) == 0. Pathname not found. */
		{
		if (!pszSearch) printf("%*s%s (Not found, Ignored.)\n", iShift+iDeltaShift, "", pszIncName);
		}
	    }
	memcpy(cLine, cLine2, LINESIZE);    /* Restore the line read */
	if (   pszSearch
	    && (pszToken = strtok(cLine, pszBlanks))
	    && (    streq(pszToken, "#define")
	        || (   streq(pszToken, "#")
		    && (pszToken = strtok(NULL, pszBlanks))
		    && streq(pszToken, "define")
		   )
	       )
	   )
	    {
	    pszToken = strtok(NULL, "( \t\n\r");
	    if (streq(pszToken, pszSearch))
		{
        	if (!iNamePrinted)
        	    {
        	    printf("%*s%s\n", iShift, "", pszName);
        	    iNamePrinted = TRUE;
        	    }
		printf("%*s%s\n", iShift+iDeltaShift, "", cLine2);
		}
	    }
	}

    fclose(sf);

    return psl;
    }

