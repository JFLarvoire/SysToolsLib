/*****************************************************************************\
*		    							      *
*   Filename:	    zap.c						      *
*		    							      *
*   Description:    Delete files and/or directories visibly		      *
*		    							      *
*   Notes:	    Uses SysToolsLib's custom debugging macros in debugm.h.   *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc zap.c -o zap		    # Release mode version    *
*		    gcc -D_DEBUG zap.c -o zap.debug # Debug version	      *
*		    							      *
*   History:								      *
*    2017-10-09 JFL Created this program, based on md.c, rd.c, and zap.bat.   *
*    2018-03-06 JFL Added options -i and -I. Ignore case in Windows by dflt.  *
*		    Added options -f and -rf, to delete complete directories. *
*    2018-03-23 JFL Fixed several problems with error messages.		      *
*		    Added routine GetProgramNames(); Use global variables     *
*		    program and progcmd for help, and all tagged messages.    *
*    2018-05-31 JFL Restructured rmdirRF() as zapFile() and zapDir().         *
*		    The force option now deletes read-only files. 	      *
*                   Prefix all error messages with the program name.          *
*		    Version 1.2.    					      *
*    2019-04-18 JFL Use the version strings from the new stversion.h. V.1.2.1.*
*    2019-06-13 JFL Added PROGRAM_DESCRIPTION definition. Version 1.2.2.      *
*    2020-01-29 JFL Fixed FLAG_NOCASE default initialization. Version 1.2.3.  *
*    2020-02-04 JFL Added the ability to delete directories using wild cards. *
*    2020-02-05 JFL Fixed and improved the error reporting.		      *
*    2020-02-06 JFL Make sure never to delete a root directory. Version 1.3.  *
*    2020-03-16 JFL Fixed issue with Unix readdir() not always setting d_type.*
*                   Version 1.3.1.					      *
*		    							      *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Delete files and/or directories visibly"
#define PROGRAM_NAME    "zap"
#define PROGRAM_VERSION "1.3.1"
#define PROGRAM_DATE    "2020-03-19"

#include "predefine.h" /* Define optional features we need in the C libraries */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>
#include <sys/stat.h>
/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"
#define IGNORECASE FALSE

#define OS_HAS_DRIVES FALSE

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define IGNORECASE TRUE

#define OS_HAS_DRIVES TRUE

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#endif /* defined(_WIN32) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define IGNORECASE TRUE

#define OS_HAS_DRIVES TRUE

#endif /* defined(_MSDOS) */

/*********************************** Other ***********************************/

#ifndef DIRSEPARATOR_CHAR
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

#define exists(pathname) (access(pathname, F_OK) != -1) /* Check if a pathname exists */

/********************** End of OS-specific definitions ***********************/

/* Global variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */
int printError(char *pszFormat, ...);	/* Print errors in a consistent format */
char *szHalRefusal = "I'm sorry Dave, I'm afraid I can't do that";

/* Forward declarations */
void usage(void);
int IsSwitch(char *pszArg);
int isEffectiveDir(const char *pszPath);
char *NewPathName(const char *path, const char *name);
/* zap functions options */
typedef struct zapOpts {
  int iFlags;
  char *pszPrefix;
  unsigned long *pNDeleted;
} zapOpts;
/* zapOpts iFlags */
#define FLAG_VERBOSE	0x0001		/* Display the pathname operated on */
#define FLAG_NOEXEC	0x0002		/* Do not actually execute */
#define FLAG_RECURSE	0x0004		/* Recursive operation */
#define FLAG_NOCASE	0x0008		/* Ignore case */
#define FLAG_FORCE	0x0010		/* Force operation on read-only files */
int zapFiles(const char *pathname, zapOpts *pzo); /* Remove files in a directory */
int zapBaks(const char *path, zapOpts *pzo); /* Remove backup files in a dir */
int zapFile(const char *path, zapOpts *pzo); /* Delete a file */
int zapFileM(const char *path, int iMode, zapOpts *pzo); /* Faster */
int zapDir(const char *path, zapOpts *pzo);  /* Delete a directory */
int zapDirM(const char *path, int iMode, zapOpts *pzo); /* Faster */
int zapDirs(const char *path, zapOpts *pzo); /* Delete multiple directories */
int isRootDir(const char *dir);		/* Check if dir is a root directory */

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
|    2014-02-05 JFL Created this routine                                      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int i;
  int nErr = 0;
  int iRet = 0;
  unsigned long nDeleted = 0;
  zapOpts zo = {FLAG_VERBOSE | (IGNORECASE ? FLAG_NOCASE : 0), "", NULL};
  int iZapBackup = FALSE;
  int nZaps = 0;
  size_t len;

  zo.pNDeleted = &nDeleted;
  
  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "b")) {	/* Zap Backup Files */
	iZapBackup = TRUE;
	continue;
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_MORE();
	  continue;
	}
      )
      if (streq(opt, "f")) {	/* Force deleting files and non-empty directories */
	zo.iFlags |= FLAG_FORCE;
	continue;
      }
      if (   streq(opt, "help")
	  || streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "i")) {	/* Ignore case */
	zo.iFlags |= FLAG_NOCASE;
	continue;
      }
      if (streq(opt, "I")) {	/* Do not ignore case */
	zo.iFlags &= ~FLAG_NOCASE;
	continue;
      }
      if (streq(opt, "p")) {	/* Prefix string */
	if (((i+1) < argc) && !IsSwitch(argv[i+1])) zo.pszPrefix = argv[++i];
	continue;
      }
      if (streq(opt, "q")) {	/* Quiet mode */
	zo.iFlags &= ~FLAG_VERBOSE;
	continue;
      }
      if (streq(opt, "r")) {	/* Deleting files recursively in all subdirectories */
	zo.iFlags |= FLAG_RECURSE;
	continue;
      }
      if (streq(opt, "rf")) {	/* For people used to Unix' rm -rf */
	zo.iFlags |= FLAG_RECURSE | FLAG_FORCE;
	continue;
      }
      if (streq(opt, "X")) {	/* NoExec mode: Display what would be deleted */
	zo.iFlags |= FLAG_NOEXEC;
	continue;
      }
      if (streq(opt, "v")) {	/* Verbose mode */
	zo.iFlags |= FLAG_VERBOSE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    /* If it's an argument */
    nZaps += 1;
    len = strlen(arg);
#if defined(_MSDOS) || defined(_WIN32) /* Make sure the path uses only native \ separators */
    { int j; for (j=0; (unsigned)j<len; j++) if (arg[j] == '/') arg[j] = '\\'; }
#endif
    if (iZapBackup) {
      nErr += zapBaks(arg, &zo);
      continue;
    }
    if (!len) {
      printError("Error: Empty pathname");
      nErr += 1;
      continue;
    }
#if OS_HAS_DRIVES /* If it's just a drive, append the implicit . path */
    if ((len == 2) && (arg[1] == ':')) {
      char *arg2 = "C:.";
      arg2[0] = arg[0];
      arg = arg2;
    }
#endif
    if (isRootDir(arg)) {
      printError(szHalRefusal);
      nErr += 1;
      continue;
    }
    if (arg[len-1] == DIRSEPARATOR_CHAR) { /* If the name is explicitly flagged as a directory name, but not root */
      nErr += zapDirs(arg, &zo);		/* Remove whole directories */
      continue;
    }
    if (isEffectiveDir(arg)) { /* If the pathname refers to an existing directory */
      nErr += zapDir(arg, &zo);   /* Remove a whole directory */
      continue;
    }
    nErr += zapFiles(arg, &zo);
    continue;
  }

  if (iZapBackup && !nZaps) {
    nZaps += 1;
    nErr += zapBaks(NULL, &zo);
  }

  if (!nZaps) usage(); /* No deletion was requested */ 

  if (nErr) {
    /* Display the error summary if we tried deleting more than one file */
    if ((nErr+nDeleted) > 1UL) printError("%d files or directories could not be deleted", nErr);
    iRet = 1;
  } else {
    iRet = 0;
  }

  return iRet;
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
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage:\n\
  %s [SWITCHES] PATHNAME [PATHNAME [...]]\n\
  %s [SWITCHES] -b [PATH [PATH [...]]]\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n"
#ifdef _DEBUG
"\
  -d          Output debug information\n"
#endif
"\
  -b          Delete backup files: *.bak, *~, #*#    Default path: .\n\
  -f          Force deleting read-only files, and non-empty directories\n\
  -i          Ignore case. Default in Windows\n\
  -I          Do not ignore case. Default in Unix\n\
  -p PREFIX   Prefix string to insert ahead of output file names\n\
  -q          Quiet mode. Do not output the deleted files names\n\
  -r          Delete files recursively in all subdirectories\n\
  -V          Display this program version and exit\n\
  -X          NoExec mode: Display what would be deleted, but don't do it\n\
\n\
Pathname: [PATH" DIRSEPARATOR_STRING "]NAME[" DIRSEPARATOR_STRING "]    (A trailing " DIRSEPARATOR_STRING " flags NAME as a directory name.)\n\
Wildcards are allowed in NAME, but not in PATH.\n\
When using wildcards in recursive mode, a search is made in each subdirectory.\n\
Without a trailing " DIRSEPARATOR_STRING ", wildcards refer to files and links only.\n\
With a trailing " DIRSEPARATOR_STRING ", wildcards refer to directories only.\n\
\n\
Notes:\n\
* Deleting a non-existent file or directory is not an error. Nothing's output.\n\
* If pathname is . then all . contents will be deleted, but not . itself.\n\
* Deleting a non-empty directory (including .) requires using option -r or -f.\n\
* For your own safety, the program will refuse to delete root directories.\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
, progcmd, progcmd);
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProgramNames					      |
|									      |
|   Description     Extract the program names from argv[0]		      |
|									      |
|   Parameters      char *argv[0]					      |
|									      |
|   Returns	    0							      |
|									      |
|   Notes	    Sets global variables program and progcmd.		      |
|		    Designed to work independantly of MsvcLibX.		      |
|		    							      |
|   History								      |
|    2018-03-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    printError						      |
|									      |
|   Description     Print error messages with a consistent format	      |
|									      |
|   Parameters      char *pszFormat					      |
|		    ...							      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    Uses global variables program and progcmd,		      |
|		    set by GetProgramNames().				      |
|		    							      |
|   History								      |
|    2018-05-31 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int printError(char *pszFormat, ...) {
  va_list vl;
  int n;

  n = fprintf(stderr, "%s: ", program);
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, ".\n");
  va_end(vl);

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description     Test if a command line argument is a switch.	      |
|									      |
|   Parameters      char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-':
#if defined(_WIN32) || defined(_MSDOS)
    case '/':
#endif
      return (*(short*)pszArg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    isEffectiveDir					      |
|									      |
|   Description     Check if pathname refers to an existing directory	      |
|									      |
|   Parameters      const char *path		The directory name	      |
|		    							      |
|   Returns	    TRUE or FALSE					      |
|		    							      |
|   Notes	    Resolves links to see what they point to		      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2020-03-16 JFL Use stat instead of lstat, it faster and simpler!         |
*									      *
\*---------------------------------------------------------------------------*/

int isEffectiveDir(const char *pszPath) {
  struct stat sStat;
  int iErr = stat(pszPath, &sStat);
  if (iErr) return 0;
  return S_ISDIR(sStat.st_mode);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    isRootDir						      |
|									      |
|   Description     Check if pathname refers to a root directory	      |
|									      |
|   Parameters      const char *path		The directory name	      |
|		    							      |
|   Returns	    TRUE or FALSE					      |
|		    							      |
|   Notes	    Resolves links to see what they point to		      |
|		    							      |
|   History								      |
|    2020-02-06 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

const char *GetFileName(const char *path) {
  char *pName;
  pName = strrchr(path, DIRSEPARATOR_CHAR);
#if OS_HAS_DRIVES /* DOS / OS2 / Windows */
  if (!pName) pName = strrchr(path, ':');
#endif
  return pName ? pName+1 : path;
}

int isRootDir(const char *dir) {
  int iResult = FALSE;
  DEBUG_ENTER(("isRootDir(\"%s\");\n", dir));
  if (streq(dir, DIRSEPARATOR_STRING)) RETURN_INT(TRUE);
#if OS_HAS_DRIVES /* DOS / OS2 / Windows */
  if (dir[0] && streq(dir+1, ":\\")) RETURN_INT(TRUE);
#endif
  if (streq(GetFileName(dir), ".") || strstr(dir, "..")) {
    char *pszReal = realpath(dir, NULL);
    if (!pszReal) return FALSE; /* Missing dir, or too long to be root */
    iResult = isRootDir(pszReal);
    free(pszReal);
  }
  RETURN_INT(iResult);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    NewPathName						      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    Pointer to the new pathname, or NULL if allocation failed.|
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
|    2019-02-06 JFL Added call to new routine TrimDot, removing all ./ parts. |
*									      *
\*---------------------------------------------------------------------------*/

void TrimDot(char *path) { /* Remote ./ parts in the path */
  char *pIn = path;
  char *pOut = path;
  char c;
  int first = TRUE;
  for ( ; (c = *(pIn++)) != '\0'; ) {
    if (first && (c == '.') && (*pIn == DIRSEPARATOR_CHAR)) {
      pIn += 1; /* Eat up the / and continue */
      continue;
    }
    *(pOut++) = c;
    first = (   (c == DIRSEPARATOR_CHAR)
#if OS_HAS_DRIVES
	     || ((pIn == (path+2)) && (c == ':'))
#endif
            );
  }
  *(pOut++) = c;
}

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  TrimDot(buf);
  return buf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zapFiles						      |
|									      |
|   Description     Delete files visibly, possibly recursively		      |
|									      |
|   Parameters      const char *path		The files pathname	      |
|		    zapOpts *pzo		Zap options		      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-09 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

/* Delete files or links in a given directory, possibly recursively */
int zapFiles(const char *path, zapOpts *pzo) {
  char *pPath;
  char *pName;
  char *pPath2 = NULL;
  char *pPath3 = NULL;
  int iErr;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFNM = (pzo->iFlags & FLAG_NOCASE) ? FNM_CASEFOLD : 0;
  size_t len;

  DEBUG_ENTER(("zapFiles(\"%s\");\n", path));

  if ((!path) || !(len = strlen(path))) RETURN_INT_COMMENT(1, ("path is empty\n"));

  if (!strpbrk(path, "*?")) {	/* If there are no wild cards */
    nErr = zapFile(path, pzo);	    /* Remove that file, and we're done */
    goto cleanup_and_return;
  }

  pPath2 = strdup(path);
  if (!pPath2) {
out_of_memory:
    printError("Out of memory");
fail:
    nErr += 1;
    goto cleanup_and_return;
  }
  pPath = dirname(pPath2);
  pPath3 = strdup(path);
  if (!pPath3) goto out_of_memory;
  pName = basename(pPath3);

  if (strpbrk(pPath, "*?")) { /* If there are wild cards in the path */
    printError("Error: Wild cards aren't allowed in the directory name");
    goto fail;
  }

  if (isRootDir(pPath) && (streq(pName, "*") || streq(pName, "*.*"))) {   /* Refuse to delete a whole root directory */
    printError(szHalRefusal);
    goto fail;
  }

  pDir = opendirx(pPath);
  if (!pDir) {
    printError("Error: Can't access \"%s\": %s", pPath, strerror(errno));
    goto fail;
  }
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */
  while ((pDE = readdirx(pDir))) { /* readdirx() ensures d_type is set */
    char *pPathname = NewPathName(pPath, pDE->d_name);
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (!pPathname) goto out_of_memory;
    switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	if (pzo->iFlags & FLAG_RECURSE) {
	  char *pPathname2 = NewPathName(pPathname, pName);
	  if (!pPathname2) goto out_of_memory;
      	  nErr += zapFiles(pPathname2, pzo);
      	  free(pPathname2);
      	}
      	break;
      default:
      	if (fnmatch(pName, pDE->d_name, iFNM) == FNM_NOMATCH) break;
      	if (pzo->iFlags & FLAG_VERBOSE) printf("%s%s\n", pzo->pszPrefix, pPathname);
	iErr = 0;
      	if (!(pzo->iFlags & FLAG_NOEXEC)) iErr = unlink(pPathname);
	if (iErr) {
	  printError("Error deleting \"%s\": %s", pPathname, strerror(errno));
	  nErr += 1; /* Continue the directory scan, looking for other files to delete */
	} else {
	  if (pzo->pNDeleted) *(pzo->pNDeleted) += 1; /* Number of files successfully deleted */
	}
      	break;
    }
    free(pPathname);
  }
  closedirx(pDir);

cleanup_and_return:
  free(pPath2);
  free(pPath3);

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/* Zap backup files in a directory */
int zapBaks(const char *path, zapOpts *pzo) {
  char *patterns[] = {"*.bak", "*~", "#*#"};
  int nErr = 0;
  int i;
  for (i=0; i<(sizeof(patterns)/sizeof(char *)); i++) {
    char *pszPath = NewPathName(path, patterns[i]);
    if (!pszPath) {
      printError("Out of memory");
      return ++nErr;
    }
    nErr += zapFiles(pszPath, pzo);
    free(pszPath);
  }
  return nErr;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zapDir						      |
|									      |
|   Description     Remove a directory, and all its files and subdirectories. |
|									      |
|   Parameters      const char *path		The directory pathname	      |
|		    int iFlags			Verbose & NoExec flags	      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2018-05-31 JFL Changed the iFlags argument to zapOpts *pzo.	      |
|		    The FLAG_FORCE flag now deletes read-only files.	      |
|		    Split zapFile() off of zapDir().			      |
|		    Added zapXxxM routines, with an additional iMode argument,|
|		     to avoid unnecessary slow calls to lstat() in Windows.   |
*		    							      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

/* Delete one file or link - Internal method */
int zapFileM(const char *path, int iMode, zapOpts *pzo) {
  int iFlags = pzo->iFlags;
  char *pszSuffix = "";
  int iErr = 0;

  DEBUG_ENTER(("zapFileM(\"%s\", 0x%04X);\n", path, iMode));

  if (S_ISDIR(iMode)) {
    errno = EISDIR;
    iErr = 1;
    goto cleanup_and_return;
  }
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(iMode)) {
    pszSuffix = ">";
  }
#endif

  if (iFlags & FLAG_VERBOSE) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  if (iFlags & FLAG_NOEXEC) RETURN_INT(0);
  if (iFlags & FLAG_FORCE) {
    if (!(iMode & S_IWRITE)) {
      iMode |= S_IWRITE;
      DEBUG_PRINTF(("chmod(%p, 0x%X);\n", path, iMode));
      iErr = -chmod(path, iMode); /* Try making the target file writable */
      DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
    }
    if (iErr) goto cleanup_and_return;
  }
  iErr = -unlink(path); /* If error, iErr = 1 = # of errors */

cleanup_and_return:
  if (iErr) {
    printError("Error deleting \"%s\": %s", path, strerror(errno));
  } else {
    if (pzo->pNDeleted) *(pzo->pNDeleted) += 1; /* Number of files successfully deleted */
  }

  RETURN_INT(iErr);
}

/* Delete one file or link */
int zapFile(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;
  size_t len;

  DEBUG_ENTER(("zapFile(\"%s\");\n", path));

  if ((!path) || !(len = strlen(path))) RETURN_INT_COMMENT(1, ("path is empty\n"));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) {
    printError("Error: Can't delete \"%s\": %s", path, strerror(errno));
    RETURN_INT(1);
  }
  
  iErr = zapFileM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

/* Delete one directory - Internal method */
int zapDirM(const char *path, int iMode, zapOpts *pzo) {
  char *pPath;
  int iErr;
  struct stat sStat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFlags = pzo->iFlags;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix = DIRSEPARATOR_STRING;

  DEBUG_ENTER(("zapDirM(\"%s\", 0x%04X);\n", path, iMode));

  if (!S_ISDIR(iMode)) {
    errno = ENOTDIR;
    goto fail;
  }

  if (iFlags & FLAG_RECURSE) { /* If in recursive mode, delete everything inside */
    pDir = opendirx(path);
    if (!pDir) goto fail;
    while ((pDE = readdirx(pDir))) { /* readdirx() ensures d_type is set */
      DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
      pPath = NewPathName(path, pDE->d_name);
      if (!pPath) {
      	closedirx(pDir);
      	goto fail; /* Will report out of memory */
      }
      pszSuffix = "";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
      iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
      iErr = -lstat(pPath, &sStat); /* If error, iErr = 1 = # of errors */
#endif
      if (!iErr) switch (pDE->d_type) {
	case DT_DIR:
	  if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
	  if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
	  /* Do not update iErr, as the error is already reported by the subroutine */
	  nErr += zapDirM(pPath, sStat.st_mode, pzo);
	  pszSuffix = DIRSEPARATOR_STRING;
	  break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
	case DT_LNK:
	  pszSuffix = ">";
	  /* Fall through into the DT_REG case */
#endif
	case DT_REG:
	  /* Do not update iErr, as the error is already reported by the subroutine */
	  nErr += zapFileM(pPath, sStat.st_mode, pzo);
	  break;
	default:
	  iErr = 1;		/* We don't support deleting there */
#if defined(ENOSYS)
	  errno = ENOSYS;		/* Function not supported */
#else
	  errno = EPERM;		/* Operation not permitted */
#endif
	  pszSuffix = "?";
	  break;
      }
      if (iErr) {
	if (pDE->d_type != DT_DIR) printError("Error deleting \"%s%s\": %s", pPath, pszSuffix, strerror(errno));
	nErr += iErr;
	/* Continue the directory scan, looking for other files to delete */
      }
      free(pPath);
    }
    closedirx(pDir);
  }

  /* Skip the directory deletion if the directory is . or PATH\. or  D:. */
  if (!streq(GetFileName(path), ".")) {
    iErr = 0;
    pszSuffix = DIRSEPARATOR_STRING;
    if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
    if (iVerbose) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
    if (!iNoExec) iErr = rmdir(path);
    if (iErr) {
fail:
      printError("Error deleting \"%s%s\": %s", path, pszSuffix, strerror(errno));
      nErr += 1;
    } else {
      if (pzo->pNDeleted) *(pzo->pNDeleted) += 1; /* Number of files successfully deleted */
    }
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

/* Delete one directory */
int zapDir(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;
  size_t len;
  zapOpts zo = *pzo;

  DEBUG_ENTER(("zapDir(\"%s\");\n", path));

  if (zo.iFlags & FLAG_FORCE) zo.iFlags |= FLAG_RECURSE; /* Removing non-empty dirs requires that flag */

  if ((!path) || !(len = strlen(path))) RETURN_INT_COMMENT(1, ("path is empty\n"));

  if (isRootDir(path)) {
    printError(szHalRefusal);
    RETURN_INT_COMMENT(1, ("Can't delete root\n"));
  }

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) {
    char *pszSuffix = DIRSEPARATOR_STRING;
    if (path[len-1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
    printError("Error deleting \"%s%s\": %s", path, pszSuffix, strerror(errno));
    RETURN_INT(1);
  }

  iErr = zapDirM(path, sStat.st_mode, &zo);
  RETURN_INT(iErr);
}

/* Delete subdirectories in a given directory */
int zapDirs(const char *path, zapOpts *pzo) {
  char *pPath;
  char *pName;
  char *pPath2 = NULL;
  char *pPath3 = NULL;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFNM = (pzo->iFlags & FLAG_NOCASE) ? FNM_CASEFOLD : 0;
  size_t len;

  DEBUG_ENTER(("zapDirs(\"%s\");\n", path));

  if ((!path) || !(len = strlen(path))) RETURN_INT_COMMENT(1, ("path is empty\n"));

  if (!strpbrk(path, "*?")) { /* If there are no wild cards */
    nErr = zapDir(path, pzo);	/* Remove that directory, and we're done */
    goto cleanup_and_return;
  }

  pPath2 = strdup(path);
  if (!pPath2) {
out_of_memory:
    printError("Out of memory");
    nErr += 1;
    goto cleanup_and_return;
  }
  if (pPath2[len-1] == DIRSEPARATOR_CHAR) pPath2[--len] = '\0'; /* Remove that directory indicator */
  pPath3 = strdup(pPath2);
  if (!pPath3) goto out_of_memory;
  pPath = dirname(pPath2);
  pName = basename(pPath3);

  if (strpbrk(pPath, "*?")) { /* If there are wild cards in the path */
    printError("Error: Wild cards aren't allowed in the directory name");
    nErr += 1;
    goto cleanup_and_return;
  }

  if (isRootDir(pPath) && (streq(pName, "*") || streq(pName, "*.*"))) {   /* Refuse to delete everything in the root directory */
    printError(szHalRefusal);
    nErr += 1;
    goto cleanup_and_return;
  }

  pDir = opendirx(pPath);
  if (!pDir) {
    printError("Error deleting \"%s\": %s", pPath, strerror(errno));
    goto cleanup_and_return;
  }
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */
  while ((pDE = readdirx(pDir))) { /* readdirx() ensures d_type is set */
    char *pPathname = NewPathName(pPath, pDE->d_name);
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (!pPathname) {
      closedirx(pDir);
      goto out_of_memory;
    }
    switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	if (fnmatch(pName, pDE->d_name, iFNM) == FNM_NOMATCH) break;
	nErr += zapDir(pPathname, pzo);
      	break;
      default:
      	break;
    }
    free(pPathname);
  }
  closedirx(pDir);

cleanup_and_return:
  free(pPath2);
  free(pPath3);

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif
