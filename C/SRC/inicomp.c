/*****************************************************************************\
*                                                                             *
*  Filename         inicomp.c                                                 *
*                                                                             *
*  Description      Compare two .INI files                                    *
*                                                                             *
*  Notes                                                                      *
*                                                                             *
*  History                                                                    *
*    1993-09-28 JFL Created this program.                                     *
*    1995-09-19 JFL Adapted for multiple OS target.                           *
*                   Changed version to 1.1, although the feature are the same.*
*    2010-06-08 JFL Added options -d and -v.                                  *
*                   Added progress feedback while processing huge files.      *
*    2012-10-18 JFL Added my name in the help. Version 1.2.1.                 *
*    2014       JFL Started rewrite using dict.h.                             *
*    2017-01-02 JFL Finished restructuration using dict.h.		      *
*                   Added option -V. Version 2.0.                             *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "2.0"
#define PROGRAM_DATE    "2017-01-02"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <ctype.h>

#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#include "dict.h"
DICT_DEFINE_PROCS();

/* Constants, structures, types, etc... */

#define FALSE 0
#define TRUE 1

#define streq(s1, s2) (!strcmp(s1, s2))     /* String equal */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2   /* To be defined on the command line for the OS/2 version */

#define OS_NAME "OS/2"

#define LINESIZE 256		/* Maximum line size */

#define PATHNAME_SIZE 260	/* FILENAME_MAX incorrect in stdio.h */

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	    /* Automatically defined when targeting a Win32 applic. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#define LINESIZE 1024		/* Maximum line size */

#define PATHNAME_SIZE FILENAME_MAX

/* These are non standard routines, but the leading _ is annoying */
#define strlwr _strlwr
#define strdup _strdup
#define stricmp _stricmp

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define OS_NAME "DOS"

#define LINESIZE 256        /* Maximum line size */

#define PATHNAME_SIZE FILENAME_MAX

#pragma warning(disable:4505) /* Ignore the "unreferenced local function has been removed" warning */

#endif

/********************** End of OS-specific definitions ***********************/

#define VERSION PROGRAM_VERSION " " PROGRAM_DATE " " OS_NAME

typedef enum {EQUAL, FILE1, FILE2} outstate;

/* Global variables */

int verbose = FALSE;
int compBlanks = FALSE;
int ignoreCase = TRUE;

/* Function prototypes */

char *processFile(char *argname, dict_t *sections);
char *trimLeft(char *s);
char *trimRight(char *s);
int compItem(dictnode *i1, dictnode *i2);
int compStringNB(const char *s1, const char *s2);
int compString(const char *s1, const char *s2);
int compare(char *name1, dict_t *tree1, char *name2, dict_t *tree2);
void newOutState(outstate *pold, outstate new, char *name1, char *name2);
void *printSectNameCB(char *pszName, void *pValue, void *pRef);
void printSectName(char *name);
void *printItemCB(char *pszName, void *pValue, void *pRef);
void *printItem(dictnode *pi);
void usage(void);

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       main                                                      |
|                                                                             |
|   Description:    Process the command line, and have the job done           |
|                                                                             |
|   Input:          int argc        Number of command line arguments          |
|                   char *argv[]    Arguments                                 |
|                                                                             |
|   Output:         None                                                      |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

void main(int argc, char *argv[])
    {
    int i;
    char *f1arg = NULL;         /* File 1 name provided */
    char *f2arg = NULL;         /* File 2 name provided */
    char *name1;                /* File 1 name found */
    char *name2;                /* File 2 name found */
    dict_t *dict1 = NewDict();	/* File 1 sections dictionary */
    dict_t *dict2 = NewDict();	/* File 2 sections dictionary */

    for (i=1; i<argc; i++)
        {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) /* It's a switch */
            {
            if (   streq(argv[i]+1, "help")
                || streq(argv[i]+1, "h")
                || streq(argv[i]+1, "?"))
                {
                usage();
                }
            if (streq(argv[i]+1, "b"))
                {
                compBlanks = TRUE;
                continue;
                }
            if (streq(argv[i]+1, "c"))
                {
                ignoreCase = FALSE;
                continue;
                }
            DEBUG_CODE(
            if (   streq(argv[i]+1, "debug")
                || streq(argv[i]+1, "d"))
                {
                DEBUG_ON();
                verbose = TRUE;
                continue;
                }
                )
            if (   streq(argv[i]+1, "verbose")
                || streq(argv[i]+1, "v"))
                {
                verbose = TRUE;
                continue;
                }
            if (   streq(argv[i]+1, "version")
                || streq(argv[i]+1, "V"))
                {
		printf("inicomp version " VERSION "\n");
                exit(0);
                }
            printf("Unrecognized switch %s. Ignored.\n", argv[i]);
            continue;
            }
        if (!f1arg)
            {
            f1arg = argv[i];
            continue;
            }
        if (!f2arg)
            {
            f2arg = argv[i];
            continue;
            }
        printf("Unexpected argument: %s\nIgnored.\n", argv[i]);
        break;  /* Ignore other arguments */
        }

    if (!f1arg)
        {
        usage();
        }

    /* Sort files */

    name1 = processFile(f1arg, dict1);
    name2 = processFile(f2arg, dict2);

    /* Display differences */

    compare(name1, dict1, name2, dict2);

    exit(0);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       processFile                                               |
|                                                                             |
|   Description:    Digest a .INI file. Build sorted lists of sections & items|
|                                                                             |
|   Input:          char *argname    File name received from the command line |
|                   dict_t *sections Dictionary of all sections               |
|                                                                             |
|   Output:         Pointer to the actual name of the .INI file analysed      |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

char *processFile(char *argname, dict_t *sections) {
  char line[LINESIZE];
  long nl;
  char *pc;
  char *pc2;
  FILE *f;
  char *fname;
  dict_t *items;

  /* Open file */

  strcpy(line, argname);
  if ( ((pc = strrchr(line, '.')) == NULL) || (strchr(pc, '\\') != NULL) ) {
    /* If no dot found or there's a backslash afterwards */
    strcat(line, ".ini");
  }
  fname = strdup(line);

  f = fopen(fname, "r");
  if (!f) {
    printf("Can't open file %s.\n", fname);
    exit(1);
  }

  DEBUG_CODE(
    if (iDebug) fprintf(stderr, "\n\n");
  )
  if (verbose) fprintf(stderr, "Reading %s\n", fname);

  /* Read it & classify lines */

  nl = 0;
  items = NewDict();
  NewDictValue(sections, "", items);	/* The initial unnamed section */
  while (fgets(line, LINESIZE, f)) {
    nl += 1;
    if ((verbose) && ((nl % 1000) == 0)) fprintf(stderr, "Line %ld\n", nl);
    trimRight(line);                    /* Remove trailing blanks and \n */
    pc = trimLeft(line);                /* Skip blanks */
    if (!*pc || (*pc == ';')) continue; /* Ignore blank lines & comments */

    /* TO DO: Process continuation lines with a trailing \ */

    if (*pc == '[') {                   /* If it's a section */
      pc += 1;				/* Skip the '[' */
      pc = trimLeft(pc);                /* Skip blanks */
      pc2 = strchr(pc, ']');
      if (!pc2) {
	printf("Error in file %s line %ld:\n%s\n", fname, nl, line);
	continue;                   /* If no ], try next line */
      }
      *(pc2) = '\0';
      trimRight(pc);
      items = NewDict();
      NewDictValue(sections, pc, items);
      continue;
    }

    /* Else it's an item line */
    pc2 = strchr(pc, '=');
    if (!pc2) {
      if (!strncmp(pc, "Windows Registry Editor", 23)) { /* regedit .reg files header. Version varies. */
	NewDictValue(items, ".reg Header", strdup(pc));
      	continue;
      }
      printf("Unexpected (continuation?) line in file %s line %ld:\n%s\n", fname, nl, line);
      continue;
    }
    *(pc2++) = '\0';
    trimRight(pc);
    pc2 = trimLeft(pc2);
    NewDictValue(items, pc, strdup(pc2));
  }

  return fname;
}

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       trim                                                      |
|                                                                             |
|   Description:    Remove spaces from the ends of a string                   |
|                                                                             |
|   Input:          char *s     String to trim                                |
|                                                                             |
|   Output:         The string address, which may have shifted a bit.         |
|                                                                             |
|   Notes:                                                                    |
|                                                                             |
|   History:                                                                  |
|    2010-07-10 JFL Initial implementation                                    |
|    2017-01-02 JFL Fixed trimRight.                                          |
|                                                                             |
+----------------------------------------------------------------------------*/

char *trimLeft(char *s) {
  while (isspace(*s)) s++;
  return s;
}

char *trimRight(char *s) {
  size_t l = strlen(s);
  for ( ; (l--) && isspace(s[l]); ) {
    s[l] = '\0';
  }
  return s;
}

/*----------------------------------------------------------------------------+
|                                                                             |
|  Function         compItem                                                  |
|                                                                             |
|  Description      Compare two item structures (actually pointers to ...)    |
|                                                                             |
|  Input            dictnode *i1    Structure 1                               |
|                   dictnode *i2    Structure 2                               |
|                                                                             |
|  Output            0 if *i1 = *i2                                           |
|                   <0 if *i1 < *i2                                           |
|                   >0 if *i1 > *i2                                           |
|                                                                             |
|  History                                                                    |
|    1993-09-29 JFL Initial implementation                                    |
|    2017-01-01 JFL Rewritten to handle dictnode items.                       |
|                                                                             |
+----------------------------------------------------------------------------*/

int compItem(dictnode *i1, dictnode *i2)
    {
    int dif;

    dif = compString(i1->pszKey, i2->pszKey);	/* Compare names */
    if (dif) return dif;

    if (!i1->pData && !i2->pData) return 0;
    if (!i1->pData &&  i2->pData) return -1;
    if ( i1->pData && !i2->pData) return 1;

    return compStringNB(i1->pData, i2->pData);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       compStringNB                                              |
|                                                                             |
|   Description:    Compare two strings, possibly ignoring blanks and/or case |
|                                                                             |
|   Input:          char *s1        String 1                                  |
|                   char *s2        String 2                                  |
|                                                                             |
|   Output:          0 if string1 = string2                                   |
|                   <0 if string1 < string2                                   |
|                   >0 if string1 > string2                                   |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

int compStringNB(const char *s1, const char *s2)
    {
    char line1[LINESIZE];
    char line2[LINESIZE];
    const char *p1;
    char *p2;
    char c;

    if (compBlanks)
        {
        return compString(s1, s2);
        }
    else
        {
        for (p1=s1, p2=line1; (c=*p1) != 0; p1++) if (c != ' ') *(p2++) = c;
        *p2 = '\0';
        for (p1=s2, p2=line2; (c=*p1) != 0; p1++) if (c != ' ') *(p2++) = c;
        *p2 = '\0';
        return compString(line1, line2);
        }
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       compString                                                |
|                                                                             |
|   Description:    Compare two strings, possibly ignoring case               |
|                                                                             |
|   Input:          char *s1        String 1                                  |
|                   char *s2        String 2                                  |
|                                                                             |
|   Output:          0 if string1 = string2                                   |
|                   <0 if string1 < string2                                   |
|                   >0 if string1 > string2                                   |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

int compString(const char *s1, const char *s2)
    {
    if (ignoreCase)
        return stricmp(s1, s2);
    else
        return strcmp(s1, s2);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|  Function         compare                                                   |
|                                                                             |
|  Description      Compare and display two sorted .INI files                 |
|                                                                             |
|  Input            char *name1     File 1 name                               |
|                   dict_t *tree1   Tree section structure for file 1         |
|                   char *name2     File 2 name                               |
|                   dict_t *tree2   Tree section structure for file 2         |
|                                                                             |
|  Output           none                                                      |
|                                                                             |
|  History                                                                    |
|    1993-09-29 JFL Initial implementation                                    |
|    2017-01-01 JFL Adapted to dict_t types.                                  |
|                                                                             |
+----------------------------------------------------------------------------*/

int compare(char *name1, dict_t *tree1, char *name2, dict_t *tree2)
    {
    dictnode *n1, *n2;
    dict_t *s1, *s2;		/* Section dictionaries */
    dictnode *i1, *i2;		/* Items */
    dictnode *i01, *i02;
    outstate os;
    int sdone = FALSE;

    printf("Comparing .ini files %s and %s\n", name1, name2); /* Similar message to that of fc.exe */

    os = EQUAL;

    for (n1=FirstDictValue(tree1), n2=FirstDictValue(tree2); n1 || n2; )
        {
        int dif;

	if (sdone) newOutState(&os, EQUAL, name1, name2); /* 2017-01-02 JFL Added to close EQUAL section */

        s1 = s2 = NULL;
        if (n1) s1 = n1->pData;
        if (n2) s2 = n2->pData;
	DEBUG_PRINTF(("// Comparing sections [%s] and [%s]\n", (n1 ? n1->pszKey : "(null)"), (n2 ? n2->pszKey : "(null)")));
        if (!s1 && s2)
            dif = 1;
        else if (s1 && !s2)
            dif = -1;
        else
            dif = compString(n1->pszKey, n2->pszKey);

        if (dif < 0)
            {
            newOutState(&os, FILE1, name1, name2);
            printSectName(n1->pszKey);
            ForeachDictValue(s1, printItemCB, NULL);
            n1 = NextDictValue(tree1, n1);
            continue;
            }

        if (dif > 0)
            {
            newOutState(&os, FILE2, name1, name2);
            printSectName(n2->pszKey);
            ForeachDictValue(s2, printItemCB, NULL);
            n2 = NextDictValue(tree2, n2);
            continue;
            }

        /* Else section names match. Compare Items */

        newOutState(&os, EQUAL, name1, name2);

        sdone = FALSE;
        i01 = i02 = NULL;
	for (i1=FirstDictValue(s1), i2=FirstDictValue(s2); i1 || i2; )
            {
            DEBUG_PRINTF(("// Comparing values \"%s\" and \"%s\"\n", (i1 ? i1->pszKey : "(null)"), (i2 ? i2->pszKey : "(null)")));
            if (!i1 || !i2) /* We're sure at least one of them is not NULL */
                {
                if (!sdone)
                    {
                    printSectName(n1->pszKey);
                    sdone = TRUE;
                    }
                if (!i01 && !i02)   /* Remember the first difference */
                    {
                    i01 = i1;
                    i02 = i2;
                    }
                if (i1)
                    i1 = NextDictValue(s1, i1);
                else
                    i2 = NextDictValue(s2, i2);
                continue;
                }

            dif = compItem(i1, i2);

            if (dif)
                {
                if (!sdone)
                    {
                    printSectName(n1->pszKey);
                    sdone = TRUE;
                    }
                if (!i01 && !i02)   /* Remember the first difference */
                    {
                    i01 = i1;
                    i02 = i2;
                    }
                if (dif < 0)
                    i1 = NextDictValue(s1, i1);
                else
                    i2 = NextDictValue(s2, i2);
                continue;
                }

            /* No difference */

            if (i01 || i02)         /* Display differing lines */
                {
                newOutState(&os, FILE1, name1, name2);
                if (i01) for ( ; i01 != i1; i01 = NextDictValue(s1, i01)) printItem(i01);
                newOutState(&os, FILE2, name1, name2);
                if (i02) for ( ; i02 != i2; i02 = NextDictValue(s2, i02)) printItem(i02);
                i01 = i02 = NULL;
                }

	    i1 = NextDictValue(s1, i1);
	    i2 = NextDictValue(s2, i2);
            }

        if (i01 || i02)         /* Display differing lines */
            {
            newOutState(&os, FILE1, name1, name2);
            if (i01) for ( ; i01; i01 = NextDictValue(s1, i01)) printItem(i01);
            newOutState(&os, FILE2, name1, name2);
            if (i02) for ( ; i02; i02 = NextDictValue(s2, i02)) printItem(i02);
            i01 = i02 = NULL;
            }

	n1 = NextDictValue(tree1, n1);
	n2 = NextDictValue(tree2, n2);
        }

    newOutState(&os, EQUAL, name1, name2);

    return 0;
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       newOutState                                               |
|                                                                             |
|   Description:    Change the output file state                              |
|                                                                             |
|   Input:          outstate *pold  Pointer to the state variable to change   |
|                   outstate new    New state value to reach                  |
|                   char *name1     File 1 name                               |
|                   char *name2     File 2 name                               |
|                                                                             |
|   Output:         None                                                      |
|                                                                             |
|   Notes:          The output state corresponds to the name of the file      |
|                    from which lines are currently being displayed.          |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

void newOutState(outstate *pold, outstate new, char *name1, char *name2)
    {
    while (*pold != new)
        {
        switch (*pold)
            {
            case EQUAL:
                printf("\n***** %s\n", name1);
                *pold = FILE1;
                break;

            case FILE1:
                printf("***** %s\n", name2);
                *pold = FILE2;
                break;

            case FILE2:
                printf("*****\n");
                *pold = EQUAL;
                break;

            default:
                    break;
            }
        }
    return;
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       printSectName                                             |
|                                                                             |
|   Description:    Display a section name                                    |
|                                                                             |
|   Input:          section *ps     Pointer to the section to display         |
|                                                                             |
|   Output:         None                                                      |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */

void *printSectNameCB(char *pszName, void *pValue, void *pRef)
    {
    printf("\n[%s]\n", pszName);
    return NULL;
    }

#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */

void printSectName(char *name)
    {
    printSectNameCB(name, NULL, NULL);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|  Function         printItem                                                 |
|                                                                             |
|  Description      Display a section item                                    |
|                                                                             |
|  History                                                                    |
|    1993-09-29 JFL Initial implementation                                    |
|    2017-01-01 JFL Rewritten to be usable as a dict_t enumeration callback.  |
|                                                                             |
+----------------------------------------------------------------------------*/

#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
 
void *printItemCB(char *pszName, void *pValue, void *pRef)
    {
    printf("    %s", pszName);
    if (pValue) printf(" = %s", pValue);
    printf("\n");
    return NULL;
    }

#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */
 
void *printItem(dictnode *pi)
    {
    return printItemCB(pi->pszKey, pi->pData, NULL);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       usage                                                     |
|                                                                             |
|   Description:    Display a brief online help, describing the command line  |
|                                                                             |
|   Input:          None                                                      |
|                                                                             |
|   Output:         None                                                      |
|                                                                             |
|   Updates:                                                                  |
|     93/09/29 JFL  Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

void usage(void)
    {
    printf("\
inicomp version " VERSION "\n\
\n\
Compare two .ini files, section by section, and item by item.\n\
\n\
Usage: inicomp [switches] {file1[.ini]} {file2[.ini]}\n\
\n\
Switches:\n\
  -b   Include spaces in item values comparisons. Default: Ignore them.\n\
  -c   Account for case in comparisons. Default: Ignore case.\n\
\n\
Author: Jean-Francois Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");
    exit(0);
    }

