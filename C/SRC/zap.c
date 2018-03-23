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
*		    Added options -f and -rf, to delete complete directories. *
*    2018-03-23 JFL Fixed several problems with error messages.		      *
*		    Added routine GetProgramNames(); Use global variables     *
*		    program and progcmd for help, and all tagged messages.    *
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.1.1"
#define PROGRAM_DATE    "2018-03-23"

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

/* Global variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */

/* Forward declarations */
char *version(int iVerbose);
void usage(void);
int IsSwitch(char *pszArg);
int isdir(const char *pszPath);
char *NewPathName(const char *path, const char *name);
typedef struct zapOpts {
  int iFlags;
  char *pszPrefix;
} zapOpts;
int zap(const char *pathname, zapOpts *pzo); /* Remove files in a directory */
int zapbaks(const char *path, zapOpts *pzo); /* Remove backup files in a dir */
int rmdirRF(const char *path, int iFlags);   /* Remove a whole directory */

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

/* Same flags as in rd.c, as we share routine rmdirRF() */
#define FLAG_VERBOSE	0x0001
#define FLAG_NOEXEC	0x0002
#define FLAG_RECURSE	0x0004
#define FLAG_NOCASE	0x0008
#define FLAG_FORCE	0x0010

int main(int argc, char *argv[]) {
  int i;
  int nErr = 0;
  int iRet = 0;
  zapOpts zo = {FLAG_VERBOSE | IGNORECASE, ""};
  int iZapBackup = FALSE;
  int nZaps = 0;

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
      if (streq(opt, "f")) {	/* Force deleting directories */
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
      if (streq(opt, "rf")) {	/* Deleting files recursively in all subdirectories */
	zo.iFlags |= FLAG_RECURSE | FLAG_FORCE;
	continue;
      }
      if (streq(opt, "X")) {	/* NoExec mode: Display what files will be deleted */
	zo.iFlags |= FLAG_NOEXEC;
	continue;
      }
      if (streq(opt, "v")) {	/* Verbose mode */
	zo.iFlags |= FLAG_VERBOSE;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	printf("%s\n", version(zo.iFlags & FLAG_VERBOSE));
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
      if (   (!(zo.iFlags & FLAG_NOEXEC)) /* Skip warning if nothing will be deleted anyway */
      	  && (!((zo.iFlags & FLAG_RECURSE) && (zo.iFlags & FLAG_FORCE)))) {
      	fprintf(stderr, "zap: Error: \"%s\" is a directory! Use -r -f if your really want to delete it.\n", arg);
	continue;
      }
      nErr += rmdirRF(arg, zo.iFlags);   /* Remove a whole directory */
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
    if (nErr > 1) fprintf(stderr, "zap: %d files or directories could not be deleted!\n", nErr);
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
%s version %s\n\
\n\
Delete files visibly, possibly recursively\n\
\n\
Usage:\n\
  %s [SWITCHES] PATHNAME [PATHNAME [...]]\n\
  %s [SWITCHES] -b PATH [PATH [...]]\n\
\n\
Switches:\n\
  -?          Display this help message and exit\n"
#ifdef _DEBUG
"\
  -d          Output debug information\n"
#endif
"\
  -b          Delete backup files: *.bak, *~, #*#\n\
  -f          Use with -r to allow deleting whole directories\n\
  -i          Ignore case. Default in Windows\n\
  -I          Do not ignore case. Default in Unix\n\
  -p PREFIX   Prefix string to insert ahead of output file names\n\
  -q          Quiet mode. Do not output the deleted files names\n\
  -r          Delete files recursively in all subdirectories\n\
  -V          Display this program version and exit\n\
  -X          NoExec mode: Display what would be deleted, but don't do it\n\
\n\
Pathname: [PATH" DIRSEPARATOR_STRING "]NAME (Wildcards allowed in name)\n\
When using wildcards in recursive mode, a search is made in each subdirectory.\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
, program, version(FALSE), progcmd, progcmd);
#ifdef __unix__
  printf("\n");
#endif
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
  int iFNM = (pzo->iFlags & FLAG_NOCASE) ? FNM_CASEFOLD : 0;

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
      	if (pzo->iFlags & FLAG_RECURSE) {
	  char *pPathname2 = NewPathName(pPathname, pName);
	  if (!pPathname2) goto out_of_memory;
      	  nErr += zap(pPathname2, pzo);
      	  free(pPathname2);
      	}
      	break;
      default:
      	if (fnmatch(pName, pDE->d_name, iFNM) == FNM_NOMATCH) break;
      	if (pzo->iFlags & FLAG_VERBOSE) printf("%s%s\n", pzo->pszPrefix, pPathname);
      	if (!(pzo->iFlags & FLAG_NOEXEC)) iErr = unlink(pPathname);
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    rmdirRF						      |
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
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif

int rmdirRF(const char *path, int iFlags) {
  char *pPath;
  int iErr;
  struct stat sstat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix;

  DEBUG_ENTER(("rmdirRF(\"%s\");\n", path));

  iErr = lstat(path, &sstat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT) && !iVerbose) return 0; /* Already deleted. Not an error. */
  if (iErr) return 1;
  if (!S_ISDIR(sstat.st_mode)) {
    errno = ENOTDIR;
    return 1;
  }

  pDir = opendir(path);
  if (!pDir) return 1;
  while ((pDE = readdir(pDir))) {
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    iErr = 0;
    pPath = NewPathName(path, pDE->d_name);
    pszSuffix = "";
    switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	iErr = rmdirRF(pPath, iFlags);
      	pszSuffix = DIRSEPARATOR_STRING;
      	break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      case DT_LNK:
      	pszSuffix = ">";
      	/* Fall through into the DT_REG case */
#endif
      case DT_REG:
      	if (iVerbose) printf("%s%s\n", pPath, pszSuffix);
      	if (!iNoExec) iErr = (unlink(pPath) != 0);
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
      if (pDE->d_type != DT_DIR) fprintf(stderr, "%s: Error deleting \"%s%s\": %s\n", program, pPath, pszSuffix, strerror(errno));
      nErr += iErr;
      /* Continue the directory scan, looking for other files to delete */
    }
    free(pPath);
  }
  closedir(pDir);

  iErr = 0;
  pszSuffix = DIRSEPARATOR_STRING;
  if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
  if (iVerbose) printf("%s%s\n", path, pszSuffix);
  if (!iNoExec) iErr = rmdir(path);
  if (iErr) {
    fprintf(stderr, "%s: Error deleting \"%s%s\": %s\n", program, path, pszSuffix, strerror(errno));
    nErr += 1;
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

