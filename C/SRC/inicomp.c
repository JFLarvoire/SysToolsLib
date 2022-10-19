/*****************************************************************************\
*                                                                             *
*  Filename         inicomp.c                                                 *
*                                                                             *
*  Description      Compare two .INI files                                    *
*                                                                             *
*  Notes            In July 2010, I started a redesign of the program.        *
*                   The goal was to gain performance, by replacing the old    *
*                   linked lists with self-balancing binary trees.            *
*                   The performance of linked lists was good when comparing   *
*                   small .ini files, like the old Windows 9x win.ini and     *
*                   system.ini. But it was becoming unacceptably slow when    *
*                   comparing tens of MB-long exported registry trees.        *
*                   The project stalled for several years, until I completed  *
*                   it while traveling back from Christmas 2016 vacations.    *
*                                                                             *
*                   To do:                                                    *
*                   - Convert all file encodings to UTF8 data.		      *
*                   - Use "natural" sort for keys, not ASCII sort.            *
*                     (natural = consider numbers as 1-character values,      *
*                      no matter how many actual characters are present.      *
*                      Ex: "12" < "75" < "128" )                              *
*                   - Add support for multiple homonym sections.              *
*                     (Non-standard, but used in some real cases.)            *
*                                                                             *
*  History                                                                    *
*    1993-09-28 JFL Created this program.                                     *
*    1995-09-19 JFL Adapted for multiple OS target.                           *
*                   Changed version to 1.1, although the feature are the same.*
*    2010-06-08 JFL Added options -d and -v.                                  *
*                   Added progress feedback while processing huge files.      *
*    2010-07-09 JFL Started rewrite using dict.h. (Program does not build)    *
*    2012-10-18 JFL Added my name in the help.				      *
*    2017-01-02 JFL Finished restructuration using dict.h.		      *
*                   Added option -V.		                              *
*    2017-01-04 JFL Use case-independant NewIDict().			      *
*                   Made this a UTF-8 app. supporting non-ASCII file names.   *
*                   Added support for Linux.                                  *
*    2017-01-05 JFL Added support for quoted value names, that may contain '='.
*                   Added support for \ continuation lines.                   *
*    2017-01-06 JFL Quoted value strings may also continue on multiple lines. *
*                   Use multimaps instead of dicts (aka. maps) to avoid       *
*		    issues when encountering duplicate names. (This happens!) *
*    2017-01-07 JFL Count line ending with \r\r\n as two lines.               *
*                   Version 2.0.                                              *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.2.0.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.0.2.      *
*    2020-02-17 JFL Added option -f to allow free lines without an =value.    *
*                   Removed the incorrect code handling homonym sections. The *
*		    standard is to merge multiple parts into 1 single section.*
*		    Version 2.1.                                              *
*    2020-04-20 JFL Added support for MacOS. Version 2.2.                     *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 2.2.1.		      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Compare .ini files, section by section, and item by item"
#define PROGRAM_NAME    "inicomp"
#define PROGRAM_VERSION "2.2.1"
#define PROGRAM_DATE    "2022-10-19"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#define _UTF8_SOURCE		/* Enable MsvcLibX support for file names with Unicode characters */
#define _GNU_SOURCE		/* Else Linux does not define strdup() in string.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <ctype.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#include "dict.h"
DICT_DEFINE_PROCS();

/************************* OS/2-specific definitions *************************/

#ifdef _OS2   /* To be defined on the command line for the OS/2 version */

#define PATHNAME_SIZE 260	/* FILENAME_MAX incorrect in stdio.h */

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	    /* Automatically defined when targeting a Win32 applic. */

#define PATHNAME_SIZE FILENAME_MAX

/* These are standard routines, but Microsoft thinks not */
#define strdup _strdup
/* These are non standard routines, but the leading _ is annoying */
#define strlwr _strlwr
#define stricmp _stricmp

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define PATHNAME_SIZE FILENAME_MAX

#pragma warning(disable:4505) /* Ignore the "unreferenced local function has been removed" warning */

#endif

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define PATHNAME_SIZE FILENAME_MAX

#define stricmp strcasecmp

#endif /* __unix__ */

/******************************* Any other OS ********************************/

#ifndef PATHNAME_SIZE
#error "Unsupported OS"
#endif

/********************** End of OS-specific definitions ***********************/

typedef enum {EQUAL, FILE1, FILE2} outstate;

/* Global variables */

int verbose = FALSE;
int compBlanks = FALSE;
int ignoreCase = TRUE;
int allowNoValue = FALSE;	  /* TRUE = Allow non-standard data lines with a free string, without an =value */

/* Define multimap dictionaries */

int cmpvalue(void *p1, void *p2) {
  return strcmp(p1, p2);
}
int cmpivalue(void *p1, void *p2) {
  return _stricmp(p1, p2);
}

dict_t *NewIniDict(void) { /* Duplicate keys not allowed */
  if (ignoreCase)
    return NewIDict();
  else
    return NewDict();
}

dict_t *NewIniMMap(void) { /* Duplicate keys allowed */
  if (ignoreCase)
    return NewIMMap(cmpivalue);
  else
    return NewMMap(cmpvalue);
}

#define NewIniSectionDict() NewIniDict()  /* Duplicate keys not allowed */
#define NewIniValueDict()   NewIniMMap()  /* Duplicate keys allowed */

/* Function prototypes */

char *processFile(char *argname, dict_t *sections);
char *trimLeft(char *s);
char *trimRight(char *s);
int compItem(dictnode *i1, dictnode *i2);
int compStringNB(const char *s1, const char *s2);
int compString(const char *s1, const char *s2);
int compare(char *name1, dict_t *tree1, char *name2, dict_t *tree2);
void newOutState(outstate *pold, outstate new, char *name1, char *name2);
void *printSectCB(char *pszName, void *pValue, void *pRef);
void *printSectNameCB(char *pszName, void *pValue, void *pRef);
void printSectName(char *name);
void *printItemCB(char *pszName, void *pValue, void *pRef);
void *printItem(dictnode *pi);
void outOfMem(void);
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
|    1993-09-29 JFL Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int i;
  char *f1arg = NULL;         /* File 1 name provided */
  char *f2arg = NULL;         /* File 2 name provided */
  char *name1;                /* File 1 name found */
  char *name2;                /* File 2 name found */
  dict_t *dict1;		/* File 1 sections dictionary */
  dict_t *dict2;		/* File 2 sections dictionary */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) { /* It's a switch */
      char *opt = arg + 1;
      if (   streq(opt, "help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "b")) {
	compBlanks = TRUE;
	continue;
      }
      if (streq(opt, "c")) {
	ignoreCase = FALSE;
	continue;
      }
      if (streq(opt, "C")) {
	ignoreCase = TRUE;
	continue;
      }
      DEBUG_CODE(
      if (   streq(opt, "debug")
	  || streq(opt, "d")) {
	DEBUG_ON();
	verbose = TRUE;
	continue;
      }
      )
      if (streq(opt, "f")) {
	allowNoValue = TRUE;
	continue;
      }
      if (streq(opt, "F")) {
	allowNoValue = FALSE;
	continue;
      }
      if (   streq(opt, "verbose")
	  || streq(opt, "v")) {
	verbose = TRUE;
	continue;
      }
      if (   streq(opt, "version")
	  || streq(opt, "V")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
    if (!f1arg) {
      f1arg = arg;
      continue;
    }
    if (!f2arg) {
      f2arg = arg;
      continue;
    }
    printf("Unexpected argument: %s\nIgnored.\n", arg);
    break;  /* Ignore other arguments */
  }

  if (!f1arg) {
    usage();
  }

  dict1 = NewIniSectionDict();	/* File 1 sections dictionary */
  dict2 = NewIniSectionDict();	/* File 2 sections dictionary */

  /* Sort files */

  name1 = processFile(f1arg, dict1);
  name2 = processFile(f2arg, dict2);

DEBUG_CODE(
  /* Print all data gathered so far */
  printf("***************************************************************\n");
  ForeachDictValue(dict1, printSectCB, NULL);
  printf("***************************************************************\n");
  ForeachDictValue(dict2, printSectCB, NULL);
  printf("***************************************************************\n");
)

  /* Display differences */

  compare(name1, dict1, name2, dict2);

  return 0;
}

/*---------------------------------------------------------------------------*\
|		    							      |
|  Function         getLine                                                   |
|		    							      |
|  Description      Read a line, growing the buffer, if needed to get it all  |
|		    							      |
|  Input            char **pBuf		Address of output buffer pointer      |
|                   size_t *pBufSize	Address of output buffer size	      |
|		    FILE *f		Input file handle		      |
|		    size_t offset	Offset to write in the output buffer  |
|		    long *pNLines	Address of optional line counter      |
|		    							      |
|  Output           Final address of the line read, or NULL if error.	      |
|		    							      |
|  History                                                                    |
|    2017-01-06 JFL Initial implementation                                    |
|		    							      |
\*---------------------------------------------------------------------------*/

/* Read a line, growing the buffer if needed to get the full line */
char *getLine(char **pBuf, size_t *pBufSize, FILE *f, size_t offset, long *pNLines) {
  char *line = *pBuf + offset;
  size_t l;
  long nl = 0;

  if (!fgets(line, (int)(*pBufSize - offset), f)) return NULL;
  l = strlen(line);
  if (pNLines) {
    nl = *pNLines += 1;
    if ((verbose) && ((nl % 10000) == 0)) {
      fprintf(stderr, "Line %ld: %s", nl, line);
      if (l && (line[l-1] != '\n')) fprintf(stderr, "\n");
    }
  }
  /* Check if the buffer is too small, and needs to be extended to get the full line */
  while (((offset+l) == (*pBufSize-1)) && (line[l-1] != '\n')) {
    DEBUG_PRINTF(("Line %ld overflowed. Expanding buffer to %ld bytes: %s\n", nl, (unsigned long)*pBufSize*2, line));
    *pBuf = realloc(*pBuf, *pBufSize *= 2); /* Double its size, to avoid doing this too often */
    if (!*pBuf) outOfMem();
    line = *pBuf + offset;
    if (!fgets(line+l, (int)(*pBufSize-(offset+l)), f)) break;
    l += strlen(line+l);
  }
  /* When lines end with \r\r\n, most editors (but not Notepad) count that as extra lines */
  if (pNLines) {
    size_t i = 1;
    if ((i <= l) && (line[l-i] == '\n')) i += 1; /* Skip the final \n */
    if ((i <= l) && (line[l-i] == '\r')) i += 1; /* Skip the normal \r before that \n */
    for ( ; (i <= l) && (line[l-i] == '\r'); i++) *pNLines += 1; /* If there are other \r, count extra lines */
  }
  return line;
}

/*----------------------------------------------------------------------------+
|                                                                             |
|  Function         processFile                                               |
|                                                                             |
|  Description      Digest a .INI file. Build sorted lists of sections & items|
|                                                                             |
|  Input            char *argname    File name received from the command line |
|                   dict_t *sections Dictionary of all sections               |
|                                                                             |
|  Output           Pointer to the actual name of the .INI file analysed      |
|                                                                             |
|  History                                                                    |
|    1993-09-29 JFL Initial implementation                                    |
|    2016-01-05 JFL Added support for quoted value names, that may contain '='.
|                   Added support for continuation lines.                     |
|                                                                             |
+----------------------------------------------------------------------------*/

#define LINESIZE 256	/* Initial maximum line size. Will grow if needed */
			/* Some registry paths are longer than 256 characters */
			/* Must not be too large, else the small DOS version will fail to work */

char *processFile(char *argname, dict_t *sections) {
  size_t lineSize = LINESIZE;
  char *line;
  long nl;
  char *pc;
  char *pc2;
  FILE *f = NULL;
  char *fname;
  dict_t *items;
  dict_t *items0;
  size_t l;
  int iRegEdit = 0;	/* REGEDIT .reg file version */
  char bom[4];
  char *pszEncoding = "Windows";

  line = malloc(lineSize);
  if (!line) outOfMem();

  /* Open file */

  strcpy(line, argname);
  if ( ((pc = strrchr(line, '.')) == NULL) || (strchr(pc, '\\') != NULL) ) {
    /* If no dot found or there's a backslash afterwards */
    strcat(line, ".ini");
  }
  fname = strdup(line);

  f = fopen(fname, "rb");
  if (!f) {
    printf("Can't open file %s.\n", fname);
    exit(1);
  }

  DEBUG_PRINTF(("\n\n"));
  if (verbose) fprintf(stderr, "Reading %s\n", fname);

  /* Check the encoding */
  /* TO DO: Use that information to convert the input data to UTF8 */
  l = fread(bom, 1, sizeof(bom), f);
  if (!strncmp(bom, "\xEF\xBB\xBF", 3)) {
    pszEncoding = "UTF8";
    fseek(f, 3L, SEEK_SET);
  } else if (   !strncmp(bom, "\xFF\xFE", 2)
	     || ((l >= 4) && !bom[1] && !bom[3])) {
    pszEncoding = "UTF16";
bad_encoding:
    fprintf(stderr, "Error: File %s is encoded as %s. Please convert it to ANSI or UTF8 first.\n", fname, pszEncoding);
    exit(1);
  } else if (   !strncmp(bom, "\xFE\xFF", 2)
	     || ((l >= 4) && !bom[0] && !bom[2])) {
    pszEncoding = "UTF16BE";
    goto bad_encoding;
  } else { /* Assume this is in the default Windows encoding */
    rewind(f);
  }

  /* Read it & classify lines */

  nl = 0;
  items0 = items = NewIniValueDict();
  NewDictValue(sections, "", items);	/* The initial unnamed section */
  while (getLine(&line, &lineSize, f, 0, &nl)) {
    l = strlen(line);
    /* Check if the line ends with an \, showing that there's a continuation line */
check_if_continuation_line:
    if (l && line[l-1] == '\n') line[--l] = '\0'; /* Trim the final \n */
    while (l && line[l-1] == '\r') {line[--l] = '\0';} /* Trim all \r before that \n */
    if (l && line[l-1] == '\\') { /* There is a continuation line */
      size_t i;
      line[--l] = '\0';				  /* Trim the final \ */
      getLine(&line, &lineSize, f, l, &nl);
      i = l;
      while (line[l] == ' ') {
      	for (i = l; (line[i] = line[i+1]) != '\0'; i += 1) ;	/* Move the line ahead by 1 char */
      }
      l = i;				/* Record the new line length */
      goto check_if_continuation_line;
    }

    trimRight(line);                    /* Remove trailing blanks and \n */
    pc = trimLeft(line);                /* Skip blanks */
    if (!*pc || (*pc == ';')) continue; /* Ignore blank lines & comments */

    DEBUG_PRINTF(("Line %lu %s\n", nl, line));

    if (*pc == '[') {                   /* If it's a section */
      pc += 1;				/* Skip the '[' */
      pc = trimLeft(pc);                /* Skip blanks */
      pc2 = strrchr(pc, ']');
      if (!pc2) {
	fprintf(stderr, "Error in file %s line %ld: Missing end of section name:\n%s\n", fname, nl, line);
	continue;                   /* If no ], try next line */
      }
      *(pc2) = '\0';
      trimRight(pc);
      items = NewIniValueDict();
      NewDictValue(sections, pc, items);
      continue;
    }

    /* Else it's an item line */
    if (*pc == '"') {	/* It's a quoted name */
      pc2 = pc += 1;		/* Skip the opening quote */
search_end_of_quoted_name:
      for ( ; *pc2 && (*pc2 != '"'); pc2 += 1) {	/* Search the closing quote */
      	if (*pc2 == '\\') pc2 += 1; /* Skip any escaped character. Ex: \" or \\ */
      }
      if (*pc2 == '"') {
      	*pc2 = '\0';		/* Remove the closing quote */
      	pc2 = strchr(pc2+1, '=');
      } else {	/* The quoted name continues on next line (rare, but happens) */
      	char *line0 = line;
      	*(pc2++) = '\\'; /* Record an escaped new line */
      	*(pc2++) = '\n';
      	*pc2 = '\0';
      	getLine(&line, &lineSize, f, pc2-line, &nl);
      	if (line != line0) {
      	  pc += (line-line0);
      	  pc2 += (line-line0);
      	}
      	goto search_end_of_quoted_name;
      }
    } else {		/* Unquoted name */
      pc2 = strchr(pc, '=');
      if (pc2) *pc2 = '\0';	/* Remove the equal sign */
      trimRight(pc);
    }
    if (!pc2) {
      if (allowNoValue) { /* If we allow non-standard .ini files with value-less lines */
	NewDictValue(items, pc, NULL); /* Enter them without a value */
      	continue;
      }
      if (   (items == items0)
      	  && (   sscanf(pc, "Windows Registry Editor Version %d", &iRegEdit)
	      || sscanf(pc, "REGEDIT%d", &iRegEdit))
      	  ) { /* regedit .reg files header. Version varies. */
	NewDictValue(items, pc, NULL); /* Enter it without a value */
      	continue;
      }
      fprintf(stderr, "Error in file %s line %ld: Unexpected (continuation?) line:\n%s\n", fname, nl, line);
      continue;
    }
    pc2 = trimLeft(pc2+1);
    if (iRegEdit && (*pc2 == '"')) { /* It's a quoted value */
      char *pc3;
      pc3 = pc2 += 1;			/* Skip the opening quote */
search_end_of_quoted_value:
      for ( ; *pc3 && (*pc3 != '"'); pc3 += 1) { /* Search the closing quote */
      	if (*pc3 == '\\') pc3 += 1; /* Skip any escaped character. Ex: \" or \\ */
      }
      if (*pc3 == '"') {	/* We reached the end of string */
      	*pc3 = '\0';			/* Remove the closing quote */
      } else {			/* The string continues on next line */
      	char *line0 = line;
      	if (*(pc3-1) != '\n') { /* The first line had its \n trimmed above */
#if defined(_MSDOS) || defined(_WIN32)
	  *(pc3++) = '\r';
#endif
	  *(pc3++) = '\n';
	  *pc3 = '\0';
	}
      	getLine(&line, &lineSize, f, pc3-line, &nl);
      	if (line != line0) {
      	  pc += (line-line0);
      	  pc2 += (line-line0);
      	  pc3 += (line-line0);
      	}
      	goto search_end_of_quoted_value;
      }
    }
    NewDictValue(items, pc, strdup(pc2));
  }

  /* Cleanup */
  fclose(f);
  free(line);

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
|  Function         compStringNB                                              |
|                                                                             |
|  Description      Compare two strings, possibly ignoring blanks and/or case |
|                                                                             |
|  Input            char *s1        String 1                                  |
|                   char *s2        String 2                                  |
|                                                                             |
|  Output            0 if string1 = string2                                   |
|                   <0 if string1 < string2                                   |
|                   >0 if string1 > string2                                   |
|                                                                             |
|  History                                                                    |
|    1993-09-29 JFL Initial implementation                                    |
|    2017-01-05 JFL Removed the dependancy on the LINESIZE constant.          |
|                                                                             |
+----------------------------------------------------------------------------*/

int compStringNB(const char *s1, const char *s2)
    {
    const char *p1;
    char *p2;
    char c;

    if (compBlanks)
        {
        return compString(s1, s2);
        }
    else
        {
        char *line1, *line2;
        int dif;

        line1 = malloc(strlen(s1) + 1);
        if (!line1) outOfMem();
        line2 = malloc(strlen(s2) + 1);
        if (!line2) outOfMem();

        for (p1=s1, p2=line1; (c=*p1) != 0; p1++) if (c != ' ') *(p2++) = c;
        *p2 = '\0';
        for (p1=s2, p2=line2; (c=*p1) != 0; p1++) if (c != ' ') *(p2++) = c;
        *p2 = '\0';
        dif = compString(line1, line2);
        free(line1);
        free(line2);
        return dif;
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
|    1993-09-29 JFL Initial implementation                                    |
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
    unsigned long nc = 0;	/* Number of comparisons done */

    printf("Comparing .ini files %s and %s\n", name1, name2); /* Similar message to that of fc.exe */
    fflush(stdout);
    fflush(stderr);

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
	    nc += 1;
	    if ((verbose) && ((nc % 10000) == 0)) fprintf(stderr, 
	      "Processing value %lu: %s\\%s\n", nc, n1->pszKey, (i1 ? i1->pszKey : i2->pszKey)
	    );
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
|    1993-09-29 JFL Initial implementation                                    |
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
|    1993-09-29 JFL Initial implementation                                    |
|    2020-02-17 JFL No need to display the initial unnamed section [] name.   |
|                                                                             |
+----------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif

void *printSectNameCB(char *pszName, void *pValue, void *pRef)
    {
    if (pszName[0]) printf("\n[%s]\n", pszName);
    return NULL;
    }

void *printSectCB(char *pszName, void *pValue, void *pRef)
    {
    if (pszName[0]) printf("\n[%s]\n", pszName);
    ForeachDictValue(pValue, printItemCB, pRef);
    return NULL;
    }

#ifdef _MSC_VER
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */
#endif

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
|    2017-01-05 JFL Quote names containing spaces or =.                       |
|    2020-02-17 JFL Don't quote free-style lines without a value.             |
|                                                                             |
+----------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif
 
void *printItemCB(char *pszName, void *pValue, void *pRef)
    {
    char *pszValue = pValue;
    if (   *pszName				    /* If the name is not empty */
        && (   (!pszName[strcspn(pszName, "= \t")]) /* and if there are no spaces or = in the name */
            || (!pszValue))			    /*     or we don't have a value */
    ) {
      printf("    %s", pszName);
    } else {
      printf("    \"%s\"", pszName);
    }
    if (pszValue) {
      size_t l = strlen(pszValue);
      if (   !l					/* If the value is empty */
      	  || pszValue[strcspn(pszValue, "\r\n")]/* or if it spans multiple lines */
      	  || isspace(pszValue[0])		/* or if it begins with a space */
      	  || (l && isspace(pszValue[l-1]))	/* or if it ends with a space */
      ) {
	printf(" = \"%s\"", pszValue);
      } else {
	printf(" = %s", pszValue);
      }
    }
    printf("\n");
    return NULL;
    }

#ifdef _MSC_VER
#pragma warning(default:4100) /* Restore the "unreferenced formal parameter" warning */
#endif
 
void *printItem(dictnode *pi)
    {
    return printItemCB(pi->pszKey, pi->pData, NULL);
    }

/*----------------------------------------------------------------------------+
|                                                                             |
|   Function:       outOfMem                                                  |
|                                                                             |
|   Description:    Display an error message and abort                        |
|                                                                             |
|   Input:          None                                                      |
|                                                                             |
|   Output:         None                                                      |
|                                                                             |
|   Updates:                                                                  |
|    1993-09-29 JFL Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

void outOfMem(void)
    {
    printf("Out of memory.\n");
    exit(1);
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
|    1993-09-29 JFL Initial implementation                                    |
|                                                                             |
+----------------------------------------------------------------------------*/

void usage(void)
    {
    printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: inicomp [switches] FILE1[.ini] FILE2[.ini]\n\
\n\
Switches:\n\
  -b    Include spaces in item values comparisons. Default: Ignore them\n\
  -c    Use case sensitive comparisons for sections and items names\n\
  -C    Use case insensitive comparisons (Default)\n\
  -f    Allow non-standard data lines with a free string, without an =value\n\
  -F    Data lines must have a name=value format (Default)\n\
  -v    Verbose node. Display extra progress information\n\
  -V    Display this program version and exit\n\
\n\
Note: Also usable for .reg files, used by Windows' regedit.exe\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef _UNIX
"\n"
#endif
);
    exit(0);
    }

