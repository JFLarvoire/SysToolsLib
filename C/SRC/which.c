/*****************************************************************************\
*		    							      *
*   Filename	    which.c						      *
*		    							      *
*   Description     Find which versions of a program are accessible	      *
*		    							      *
*   Notes	    Inspired by the Unix which.				      *
*		    In DOS and Windows, we need to extend the search to       *
*		    all files with one of the default program extensions.     *
*		    							      *
*		    Gotcha in WoW, that is WIN32 programs running on WIN64:   *
*		    Windows silently redirects accesses to %windir%\System32  *
*		    to %windir%\SysWOW64.				      *
*		    %windir%\System32 contains 64-bits programs.	      *
*		    %windir%\SysWOW64 contains 32-bits programs.	      *
*		    Problem: Some 64-bits programs in the former have no      *
*		    32-bits equivalent in the latter. So which won't see them.*
*		    Since the very goal of using the which command is to find *
*		    which program will be executed at the cmd.exe prompt,     *
*		    which is 64-bits in this case, the result in inaccurate.  *
*		    Workaround: Use the %windir%\Sysnative alias the real     *
*		    		%windir%\System32.			      *
*		    							      *
*		    Uses Unix C library routines not available in Microsoft's *
*		    MSVC C library.					      *
*		    To build in Windows use JFL's MsvcLibX library extensions *
*		    with MSVC.						      *
*		    Does not build in MinGW for now, as it lacks getppid().   *
*		    							      *
*		    Uses JFL's custom debugging macros in debugm.h.	      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc -U_DEBUG which.c -o which	    # Release version *
*		    gcc -D_DEBUG which.c -o which.debug     # Debug version   *
*		    							      *
*   History								      *
*    1987       JFL jf.larvoire@hp.com created this program.		      *
*    1994-09-29 JFL Added OS/2 support.					      *
*    1997-06-06 JFL Added Win32 support.				      *
*    2005-03-14 JFL Output the 1st match only, unless -a option. As Linux.    *
*    2008-06-16 JFL Use the list in env. var. PATHEXT if present.	      *
*		    Version 1.3.					      *
*    2012-02-02 JFL Added the ability to search for a given extension only.   *
*		    Fixed a bug that prevented detection of 64=bit progs      *
*		    in the Windows\System32 directory.			      *
*		    Added support for MinGW, Cygwin, Linux, etc.	      *
*    2012-02-03 JFL Changed version format to: VERSION DATE OS [DEBUG]	      *
*		    Added option -V.					      *
*		    Version 1.4.					      *
*    2012-10-18 JFL Added my name in the help. Version 1.4.1.                 *
*    2013-03-22 JFL Added support for PowerShell, which also starts *.ps1.    *
*		    Do not search in the cur. dir. in Linux and PowerShell.   *
*		    Version 1.5.					      *
*    2013-03-27 JFL Rewrote routine getppid() for WIN64; Moved it to MsvcLibX.*
*		    Correct the output file name case if needed.	      *
*		    Use my debug macros.				      *
*    2013-03-28 JFL Allow searching any number of names, including 0.         *
*		    Version 1.6.					      *
*    2013-03-30 JFL Avoid using fnmatch(). Version 1.6.1.		      *
*    2013-06-04 JFL Detect PowerShell even when invoked in a remote session.  *
*		    Version 1.6.2.					      *
*    2013-03-24 JFL Rebuilt with MsvcLibX.lib with support for UTF-8 names.   *
*		    Version 1.7.					      *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*		    Version 1.7.1.					      *
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.7.1"
#define PROGRAM_DATE    "2014-12-04"

#define _CRT_SECURE_NO_WARNINGS 1

#define _UTF8_SOURCE		/* Force MsvcLibX.lib to use UTF-8 strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use MinGW, or JFL's MsvcLibX library extensions if needed */
#include <unistd.h>		/* For the access function */
#include <dirent.h>		/* We use the DIR type and the dirent structure */

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS	/* Automatically defined when targeting an MS-DOS application */

#define OS_NAME "DOS"

char *pszExtDos[] = {"COM", "EXE", "BAT", NULL};
char **pszExt = pszExtDos;

#define SEARCH_IN_CD TRUE  /* Command.com searches in the current directory */

#endif

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* Automatically defined when targeting an OS/2 application? */

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_VIO
#include "os2.h"

#define OS_NAME "OS/2"

char *pszExtReal[] = {"COM", "EXE", "BAT", NULL};
char *pszExtProt[] = {"EXE", "CMD", NULL};
char **pszExt = pszExtProt;

#define SEARCH_IN_CD TRUE  /* The OS/2 shell searches in the current directory */

#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32	/* Automatically defined when targeting a Win32 application */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#ifdef _WIN64
char *pszExtWin64[] = {"exe", "cmd", "bat", NULL};
char **pszExt = pszExtWin64;
#else
char *pszExtWin32[] = {"com", "exe", "cmd", "bat", NULL};
char **pszExt = pszExtWin32;
#endif

#define SEARCH_IN_CD TRUE  /* Cmd.exe searches in the current directory */

#endif

/************************* Unix-specific definitions *************************/

#ifdef __unix__	/* Automatically defined when targeting a Unix application */

#if defined(__CYGWIN64__)
#define OS_NAME "Cygwin64"
#elif defined(__CYGWIN32__)
#define OS_NAME "Cygwin"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unix"
#endif

char *pszExtUnix[] = {NULL};
char **pszExt = pszExtUnix;

#define SEARCH_IN_CD FALSE  /* Unix shells do not search in the current directory */

#include <strings.h> /* For strcasecmp() */
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#define _makepath(buf, d, p, n, x) do {strcpy(buf,p); strcat(buf,"/"); strcat(buf,n);} while (0)

#endif

/*********************************** Other ***********************************/

#ifndef OS_NAME

#define OS_NAME "Unidentified_OS"

char *pszExtOther[] = {NULL};
char **pszExt = pszExtOther;

#define SEARCH_IN_CD TRUE

#endif

/********************** End of OS-specific definitions ***********************/

#define streq(s1, s2) (!strcmp(s1, s2))
#define strieq(s1, s2) (!_stricmp(s1, s2))
#define strnieq(s1, s2, n) (!_strnicmp(s1, s2, n))

typedef unsigned char BYTE;

#define FALSE 0
#define TRUE 1

/* Global variables */

#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
char szSystem32dir[FILENAME_MAX];
char szSystem64dir[FILENAME_MAX];
int iWoW = FALSE;		    /* TRUE = WIN32 program running in WIN64 */
#endif
#if defined(_WIN32)
int isPowerShell = FALSE;	    /* TRUE = Running inside PowerShell */
#endif
int iSearchInCD = SEARCH_IN_CD;	    /* TRUE = Seach in the current directory first */

/* Prototypes */

char *version(void);
void usage(void);
int SearchProgramWithAnyExt(char *pszPath, char *pszCommand, int iAll);
int SearchProgramWithOneExt(char *pszPath, char *pszCommand, char *pszExt);
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace);
#endif
#if defined(_WIN32)
int GetProcessName(pid_t pid, char *name, size_t lname);
int FixNameCase(char *pszPathname);
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Main program routine				      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char *pszCommand;
  char *pszPath;
  char *pszTok;
  int iArg;
  char *arg;
  int iAll = FALSE;
  int iFound = 0;
#if defined(_WIN32)
  char szShellName[FILENAME_MAX+1];
#endif
  char **pathList = malloc(0);
  int nPaths = 0;
  int iPath;

  for (iArg=1; iArg<argc; iArg++) { /* Process all command line arguments */
    arg = argv[iArg];
    if (   (*arg == '-')	    /* Process switches first */
#if defined(_MSDOS) || defined(_WIN32)
	|| (*arg == '/')
#endif
      ) {
      char *opt = arg+1;
      if (   streq(opt, "?")		/* Display a help screen */
	  || streq(opt, "h")
	  || streq(opt, "-help")) {
	usage();
      }
      if (   streq(opt, "a")		/* Display all matching programs */
	  || streq(opt, "-all")) {
	iAll = TRUE;
	continue;
      }
      DEBUG_CODE(
      if (   streq(opt, "d")		/* Debug mode on */
	  || streq(opt, "-debug")) {
	DEBUG_ON();
	printf("Debug mode on.\n");
	continue;
      }
      )
      if (   streq(opt, "V")		/* Get version */
	  || streq(opt, "-version")) {
	printf("%s\n", version());
	exit(0);
      }
      printf("Error: Invalid switch ignored: %s\n", arg);
      usage();
    } else {			    /* This is not a switch */
      break;
    }
  }

  /* Find in which shell we're running. PowerShell and cmd search programs differently. */ 
#if defined(_WIN32)
  GetProcessName(getppid(), szShellName, sizeof(szShellName));
  DEBUG_PRINTF(("Executed inside %s.\n", szShellName));
  /* wsmprovhost.exe is PowerShell's remote session host process */
  if (strieq(szShellName, "powershell.exe") || strieq(szShellName, "wsmprovhost.exe")) { 
    isPowerShell = TRUE;
    iSearchInCD = FALSE; /* Contrary to cmd, PowerShell does not search in the current directory first */
  }
#endif

  /* Get the PATH environment variable, and work around known issues in Win32 on WIN64 */
  pszPath = getenv("PATH");
  DEBUG_PRINTF(("set PATH=\"%s\"\n", pszPath));
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
  if (getenv("PROCESSOR_ARCHITEW6432")) { /* This is indeed a 32-bits app on WIN64 */
    size_t l = strlen(pszPath)+1024;
    char *pPath = malloc(l);
    char *pszWindir;
    iWoW = TRUE;
    pszWindir = getenv("windir");
    _makepath(szSystem32dir, "", pszWindir, "System32", NULL);
    _makepath(szSystem64dir, "", pszWindir, "Sysnative", NULL);
    strnirepl(pPath, l, pszPath, szSystem32dir, szSystem64dir);
    pszPath = pPath;
    DEBUG_CODE_IF_ON(
      printf("This is Win32 on Win64.\n");
      printf("szSystem32dir =\"%s\";\n", szSystem32dir);
      printf("szSystem64dir =\"%s\";\n", szSystem64dir);
      printf("pszPath =\"%s\";\n", pszPath);
    )
  } else {
    DEBUG_PRINTF(("This is NOT Win32 on Win64.\n"));
  }
#endif

  /* Build the list of directories to search in */
  if (iSearchInCD) {
    pathList = realloc(pathList, (sizeof(char *))*(++nPaths));
    pathList[nPaths-1] = "";		/* Start with the current directory */
  }
#ifdef __unix__
#define PATH_SEP ":"
#else
#define PATH_SEP ";"
#endif
  if (pszPath) {
    for (pszTok = strtok(pszPath, PATH_SEP); pszTok; pszTok = strtok(NULL, PATH_SEP)) {
      pathList = realloc(pathList, (sizeof(char *))*(++nPaths));
      pathList[nPaths-1] = pszTok;	/* Append each PATH directory */
    }
  }

  /* Finally search for all the requested programs */
  for ( ; iArg<argc; iArg++) { /* Process all remaining command line arguments */
    pszCommand = argv[iArg];

    for (iPath=0; iPath < nPaths; iPath++) {
      iFound = SearchProgramWithAnyExt(pathList[iPath], pszCommand, iAll);
      if (iFound && !iAll) break;
    }
  }

  return iFound ? 0 : 1;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    usage						      |
|									      |
|   Description     Display a brief help for this program and exit	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

char *version(void) {
  return PROGRAM_VERSION
" " PROGRAM_DATE
" " OS_NAME
#ifdef _DEBUG
" Debug"
#endif
;
}

void usage(void) {
  printf("\n\
Which version %s - Find which program will be executed\n\
\n\
Usage: which [OPTIONS] [COMMAND[.EXT] ...]\n\
\n\
Options:\n\
  -?    Display this help message and exit.\n\
  -a    Display all matches. Default: Display only the first one.\n\
  -V    Display this program version and exit.\n\
\n"
#if defined(_WIN32)
"\n\
Notes:\n\
  Uses the PATHEXT variable to infer other possible names.\n\
  Supports specific rules for cmd and PowerShell.\n\
\n"
#endif
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
, version());

  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    initExtList						      |
|									      |
|   Description     Initialize the DOS/Windows extension list          	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2008-06-16 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int initExtListDone = FALSE;
void initExtList(void) {
#if defined(_WIN32)
  /* Build the extension list, based on variable PATHEXT if present. */
  char *pszPathExt = getenv("PATHEXT");
#endif

#if defined(_OS2)
  BYTE b;

  DosGetMachineMode(&b);
  if (b == MODE_REAL)
    pszExt = pszExtReal;
  else /* MODE_PROTECTED */
    pszExt = pszExtProt;
#endif

#if defined(_WIN32)
  DEBUG_PRINTF(("  PATHEXT = \"%s\"\n", pszPathExt));
  if (pszPathExt) { /* If defined, use it to rebuild the list dynamically. */
    char **pBuf;
    char *pc;
    char *pcEnd = NULL;
    int nExt = 0;

    pBuf = (char **)malloc(sizeof(char *)); /* Room for the final NUL */
    /* Special case: If the parent is PowerShell, prepend the .ps1 extension */
    if (isPowerShell) {
      pBuf = realloc(pBuf, (nExt + 2) * sizeof(char *));
      pBuf[nExt] = "ps1";
      nExt += 1;
    }
    /* Append each extension to the PATHEXT list */
    for (pc = pszPathExt; pc && *pc; pc = pcEnd) {
      if (!*++pc) break; /* Skip the . */
      pcEnd = strchr(pc, ';');
      if (pcEnd) {
	*pcEnd = '\0';
	pcEnd += 1; /* Skip the ; */
      }
      DEBUG_PRINTF(("  EXT = \"%s\"\n", pc));

      pBuf = realloc(pBuf, (nExt + 2) * sizeof(char *));
      pBuf[nExt] = pc;
      nExt += 1;
    }
    pBuf[nExt] = NULL;
    pszExt = pBuf;
  }
#endif

  initExtListDone = TRUE;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    SearchProgramWithAnyExt				      |
|									      |
|   Description     Search for a given command in a directory          	      |
|									      |
|   Arguments	    pszPath		Directory to search in		      |
|		    pszCommand		Program name to search for	      |
|		    iAll		If TRUE, display all matches	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    1987	JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

int SearchProgramWithAnyExt(char *pszPath, char *pszCommand, int iAll) {
  int iFound = FALSE;
  int i;

#ifndef __unix__
  if (strchr(pszCommand, '.')) {
#endif
    iFound |= SearchProgramWithOneExt(pszPath, pszCommand, NULL);
#ifndef __unix__
  }
#endif
  if (!initExtListDone) initExtList();
  for (i=0; pszExt[i]; i++) {
    iFound |= SearchProgramWithOneExt(pszPath, pszCommand, pszExt[i]);
    if (iFound && !iAll) return iFound;
  }

  return iFound;
}

int SearchProgramWithOneExt(char *pszPath, char *pszCommand, char *pszExt) {
  char szFname[FILENAME_MAX];

  _makepath(szFname, "", pszPath, pszCommand, pszExt);
  DEBUG_PRINTF(("  Looking for \"%s\"", szFname));
  if (!access(szFname, 0)) {
    char *pszName = szFname;
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
    char szName2[FILENAME_MAX];
#endif
    DEBUG_PRINTF((" Matched with errno %d\n", errno));
#if defined(_WIN32)
    FixNameCase(pszName);
#endif
#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */
    if (iWoW) { /* This is a WIN32 app running on WIN64 */
      strnirepl(szName2, sizeof(szName2), pszName, szSystem64dir, szSystem32dir);
      pszName = szName2;
    }
#endif
    printf("%s\n", pszName);
    return TRUE;	/* Match */
  }
  DEBUG_PRINTF((" Error %d\n", errno));
  return FALSE;	/* No match */
}

#if defined(_WIN32) && !defined(_WIN64) /* Special case for WIN32 on WIN64 */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strnirepl						      |
|									      |
|   Description:    Case-insensitive string replacement			      |
|									      |
|   Parameters:     char *pszResultBuffer   Output buffer       	      |
|		    size_t lResultBuffer    Output buffer size  	      |
|		    const char *pszString   Source string to copy from	      |
|		    const char *pszSearch   String to search in pszString     |
|		    const char *pszReplace  String to write instead	      |
|									      |
|   Returns:	    The length of the output string.			      |
|									      |
|   History:								      |
|    2012-02-02 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace) {
  size_t lSearch = strlen(pszSearch);
  size_t lReplace = strlen(pszReplace);
  size_t lRemain = lResultBuffer;
  const char *pszIn = pszString;
  char *pszOut = pszResultBuffer;
  if (!lRemain) return 0; /* Prevent catastrophies */
  lRemain -= 1; /* Leave room for the final NUL */
  while (*pszIn && lRemain) {
    if (strnieq(pszIn, pszSearch, lSearch)) {
      strncpy(pszOut, pszReplace, lRemain);
      pszIn += lSearch;
      if (lReplace < lRemain) {
	pszOut += lReplace;
      	lRemain -= lReplace;
      } else {
	pszOut += lRemain;
      	lRemain = 0;
      }
      continue;
    }
    *(pszOut++) = *(pszIn++);
    lRemain -= 1;
  }
  *pszOut = '\0';
  return pszOut - pszResultBuffer;
}

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProcessName					      |
|									      |
|   Description     Get the parent process name                    	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2013-03-22 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)
#include <windows.h>
#pragma pack(push,8) /* Work around a bug in tlhelp32.h in WIN64, which generates the wrong structure if packing has been changed */
#include <tlhelp32.h>
#pragma pack(pop)

int GetProcessName(pid_t pid, char *name, size_t lname) {
  size_t len = 0;
  HANDLE h;
  BOOL bFound;
  PROCESSENTRY32 pe = {0};

  h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (h == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to get list of processes\n");
    return 0;
  }

  pe.dwSize = sizeof(PROCESSENTRY32);
  for (bFound=Process32First(h, &pe); bFound; bFound=Process32Next(h, &pe)) {
    if ((pid_t)(pe.th32ProcessID) == pid) {
      len = strlen(pe.szExeFile);
      if (lname <= len) return -(int)len; /* Not enough room in the output buffer */
      strcpy(name, pe.szExeFile);
      break;
    }
  }

  CloseHandle(h);

  return (int)len;
}

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    FixNameCase						      |
|									      |
|   Description     Correct the file name case if needed		      |
|									      |
|   Notes	    Returns TRUE if something was changed in the name.        |
|									      |
|   History								      |
|    2013-03-27 JFL Initial implementattion.				      |
|    2014-03-20 JFL Bug fix: Make sure the drive letter is upper case.        |
|		    Bug fix: Avoid an unnecessary search if the path is empty.|
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)

#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */

int FixNameCase(char *pszPathname) {
  char *pszPath = pszPathname;
  char *pszName;
  DIR *pDir;
  struct dirent *pDE;
  int iModified = FALSE;
  int lDrive = 0;
  char szRootDir[] = "C:\\";
  char szDriveCurDir[] = "C:.";

  DEBUG_ENTER(("FixNameCase(\"%s\");\n", pszPathname));

  if (pszPathname[0] && (pszPathname[1] == ':')) { /* There's a drive letter */
    char c, C;
    lDrive = 2;
    c = pszPathname[0];
    C = (char)toupper(c);
    if (c != C) {
      pszPathname[0] = C;	/* Make sure the drive is upper case */
      iModified = TRUE;
    }
    if (!pszPathname[2]) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
  }

  pszName = strrchr(pszPathname, '\\');
  if (pszName) { /* There's a path separator */
    if (pszName != (pszPathname + lDrive)) { /* Possibly a drive letter, then a parent path name */
      *(pszName++) = '\0';
      iModified |= FixNameCase(pszPathname); /* Recursively fix the parent pathname */
    } else { /* Possibly a drive letter, then a root directory name */
      if (lDrive) { /* A drive letter, then a root directory name */
	pszPath = szRootDir; // Use the "C:\\" copy on the stack to make sure the routine is reentrant
	pszPath[0] = pszPathname[0];
      } else { /* Just a root directory name */
	pszPath = "\\";
      }
      pszName += 1;
    }
  } else { /* No path separator */
    pszName = pszPathname;
    if (lDrive) { /* A drive letter, then a file name */
      pszPath = szDriveCurDir; // Use the "C:." copy on the stack to make sure the routine is reentrant
      pszPath[0] = pszPathname[0];
      pszName += 2;  /* Skip the drive letter */
    } else {	  /* Just the file name */
      pszPath = ".";
    }
  }
  if (!*pszName) RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));

  /* Scan all directory entries that match the requested name */
  pDir = opendir(pszPath);
  if (!pDir) {
    if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
    RETURN_BOOL_COMMENT(FALSE, ("Can't open directory \"%s\"\n", pszPath));
  }
  while ((pDE = readdir(pDir))) {
    if (_stricmp(pszName, pDE->d_name)) continue; /* Names differ */
    if (strcmp(pszName, pDE->d_name)) { /* If the names match, but the case differs */
      strcpy(pszName, pDE->d_name);	/* Correct the name */
      iModified = TRUE;
    }
    break;
  }
  closedir(pDir);

  if (pszName != pszPathname) *(--pszName) = '\\'; /* Restore the initial \ */
  RETURN_BOOL_COMMENT(iModified, ("\"%s\"\n", pszPathname));
}

#pragma warning(default:4706) /* Restore the "assignment within conditional expression" warning */

#endif

