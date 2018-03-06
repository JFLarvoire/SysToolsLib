/*****************************************************************************\
*		    							      *
*   Filename:	    zap.c						      *
*		    							      *
*   Description:    Delete files visibly, possibly recursively		      *
*		    							      *
*   Notes:	    Uses our custom debugging macros in debugm.h.	      *
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
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.1.0"
#define PROGRAM_DATE    "2018-03-06"

#define _GNU_SOURCE	/* Use GNU extensions. And also MsvcLibX support for UTF-8 I/O */

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>
#include <sys/stat.h>

/* Our house debugging macros */
#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#define TRUE 1
#define FALSE 0

/************************* Unix-specific definitions *************************/

#ifdef __unix__     /* Unix */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"
#define IGNORECASE FALSE

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define IGNORECASE TRUE

#endif /* defined(_WIN32) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"
#define IGNORECASE TRUE

#endif /* defined(_MSDOS) */

/*********************************** Other ***********************************/

#ifndef EXE_OS_NAME
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

#define exists(pathname) (access(pathname, F_OK) != -1) /* Check if a pathname exists */

/********************** End of OS-specific definitions ***********************/

/* Forward declarations */
char *version(int iVerbose);
void usage(void);
int IsSwitch(char *pszArg);
int isdir(const char *pszPath);
char *NewPathName(const char *path, const char *name);
typedef struct zapOpts {
  int iVerbose;
  int iNoExec;
  int iRecurse;
  int iNoCase;
  char *pszPrefix;
} zapOpts;
int zap(const char *pathname, zapOpts *pzo); /* Remove files in a directory */
int zapbaks(const char *path, zapOpts *pzo); /* Remove backup files in a dir */

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

#define FLAG_VERBOSE	0x0001
#define FLAG_NOEXEC	0x0002
#define FLAG_RECUR	0x0004

int main(int argc, char *argv[]) {
  int i;
  int nErr = 0;
  int iRet = 0;
  zapOpts zo = {TRUE, FALSE, FALSE, IGNORECASE, ""};
  int iZapBackup = FALSE;
  int nZaps = 0;

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
      if (   streq(opt, "help")
	  || streq(opt, "-help")
	  || streq(opt, "h")
	  || streq(opt, "?")) {
	usage();
      }
      if (streq(opt, "i")) {	/* Ignore case */
	zo.iNoCase = TRUE;
	continue;
      }
      if (streq(opt, "I")) {	/* Do not ignore case */
	zo.iNoCase = FALSE;
	continue;
      }
      if (streq(opt, "p")) {	/* Prefix string */
	if (((i+1) < argc) && !IsSwitch(argv[i+1])) zo.pszPrefix = argv[++i];
	continue;
      }
      if (streq(opt, "q")) {	/* Quiet mode */
	zo.iVerbose = FALSE;
	continue;
      }
      if (streq(opt, "r")) {	/* Deleting files recursively in all subdirectories */
	zo.iRecurse = TRUE;
	continue;
      }
      if (streq(opt, "X")) {	/* NoExec mode: Display what files will be deleted */
	zo.iNoExec = TRUE;
	continue;
      }
      if (streq(opt, "v")) {	/* Verbose mode */
	zo.iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	printf("%s\n", version(zo.iVerbose));
	exit(0);
      }
      printf("Unrecognized switch %s. Ignored.\n", arg);
      continue;
    } /* End if it's a switch */
    /* If it's an argument */
    nZaps += 1;
    if (iZapBackup) {
      nErr += zapbaks(arg, &zo);
      continue;
    }
    if (isdir(arg)) {
      fprintf(stderr, "zap: Error: \"%s\" is a directory!\n", arg);
      continue;
    }
    nErr += zap(arg, &zo);
    continue;
  }

  if (iZapBackup && !nZaps) {
    nZaps += 1;
    nErr += zapbaks(NULL, &zo);
  }

  if (!nZaps) usage(); /* No deletion was requested */ 

  if (nErr) {
    fprintf(stderr, "zap: %d files could not be deleted!\n", nErr);
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

char *version(int iVerbose) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " EXE_OS_NAME DEBUG_VERSION;
  char *pszLibVer = ""
#if defined(_MSDOS) || defined(_WIN32)
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
    ;
  char *pszVer = NULL;
  if (iVerbose) {
    pszVer = malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void) {
  printf("\n\
zap version %s\n\
\n\
Delete files visibly, possibly recursively\n\
\n\
Usage:\n\
  zap [SWITCHES] PATHNAME [PATHNAME [...]]\n\
  zap [SWITCHES] -b PATH [PATH [...]]\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n"
#ifdef _DEBUG
"\
  -d          Output debug information\n"
#endif
"\
  -b          Delete backup files: *.bak, *~, #*#\n\
  -i          Ignore case. Default in Windows\n\
  -I          Do not ignore case. Default in Unix\n\
  -p PREFIX   Prefix string to insert ahead of output file names\n\
  -q          Quiet mode. Do not output the deleted files names\n\
  -r          Delete files recursively in all subdirectories\n\
  -V          Display this program version and exit\n\
  -X          NoExec mode: Display what would be deleted, but don't do it\n\
\n\
Pathname: [PATH" DIRSEPARATOR_STRING "]NAME (Wildcards allowed in name)\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, version(FALSE));
#ifdef __unix__
  printf("\n");
#endif
  exit(0);
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
|   Function	    isdir						      |
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
*									      *
\*---------------------------------------------------------------------------*/

int isdir(const char *pszPath) {
  struct stat sstat;
  int iErr = lstat(pszPath, &sstat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr) return 0;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(sstat.st_mode)) {
    char *pszReal = realpath(pszPath, NULL);
    int iRet = 0; /* If realpath failed, this is a dangling link, so not a directory */
    if (pszReal) iRet = isdir(pszReal);
    return iRet;
  }
#endif
  return S_ISDIR(sstat.st_mode);
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
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
*									      *
\*---------------------------------------------------------------------------*/

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  return buf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zap							      |
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

int zap(const char *path, zapOpts *pzo) {
  char *pPath;
  char *pName;
  char *pPath2 = NULL;
  char *pPath3 = NULL;
  int iErr;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;

  DEBUG_ENTER(("zap(\"%s\");\n", path));

  pPath2 = strdup(path);
  if (!pPath2) {
out_of_memory:
    fprintf(stderr, "zap: Out of memory\n");
    nErr += 1;
    goto cleanup_and_return;
  }
  pPath = dirname(pPath2);
  pPath3 = strdup(path);
  if (!pPath3) goto out_of_memory;
  pName = basename(pPath3);

  pDir = opendir(pPath);
  if (!pDir) goto cleanup_and_return;
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */
  while ((pDE = readdir(pDir))) {
    char *pPathname = NewPathName(pPath, pDE->d_name);
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (!pPathname) goto out_of_memory;
    iErr = 0;
    switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	if (pzo->iRecurse) {
	  char *pPathname2 = NewPathName(pPathname, pName);
	  if (!pPathname2) goto out_of_memory;
      	  nErr += zap(pPathname2, pzo);
      	  free(pPathname2);
      	}
      	break;
      default:
      	if (fnmatch(pName, pDE->d_name, pzo->iNoCase ? FNM_CASEFOLD : 0) == FNM_NOMATCH) break;
      	if (pzo->iVerbose) printf("%s%s\n", pzo->pszPrefix, pPathname);
      	if (!pzo->iNoExec) iErr = unlink(pPathname);
      	break;
    }
    if (iErr) {
      fprintf(stderr, "zap: Error deleting \"%s\": %s\n", pPathname, strerror(errno));
      nErr += 1; /* Continue the directory scan, looking for other files to delete */
    }
    free(pPathname);
  }
  closedir(pDir);

cleanup_and_return:
  free(pPath2);
  free(pPath3);

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/* Zap backup files in a directory */
int zapbaks(const char *path, zapOpts *pzo) {
  char *patterns[] = {"*.bak", "*~", "#*#"};
  int nErr = 0;
  int i;
  for (i=0; i<(sizeof(patterns)/sizeof(char *)); i++) {
    char *pszPath = NewPathName(path, patterns[i]);
    if (!pszPath) {
      fprintf(stderr, "zap: Out of memory\n");
      return ++nErr;
    }
    nErr += zap(pszPath, pzo);
    free(pszPath);
  }
  return nErr;
}

