/*****************************************************************************\
*                                                                             *
*   File name	    junction.c						      *
*                                                                             *
*   Description	    Manage NTFS junctions as if they were symbolic links      *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-11-27 JFL Created this program.				      *
*    2021-12-15 JFL Added all missing SysInternals-compatible options.	      *
*    2021-12-21 JFL Added option -f to follow symlinks and junctions. 	      *
*    2021-12-22 JFL Detect link loops, and avoid entering them.       	      *
*    2021-12-26 JFL Changed the verbose flag operation.               	      *
*    2022-01-11 JFL Changed the verbose flag operation.               	      *
*    2022-01-17 JFL Added a dummy -nobanner option.                   	      *
*                                                                             *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage NTFS junctions as if they were relative symbolic links"
#define PROGRAM_NAME    "junction"
#define PROGRAM_VERSION "2022-01-17"

#define _CRT_SECURE_NO_WARNINGS
#define _UTF8_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

/* SysLib include files */
#include "mainutil.h"	/* Main C routine utility routines */
#include "pathnames.h"	/* Pathname management definitions and functions */

/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debugging macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define TRUE 1
#define FALSE 0

#ifndef _WIN32
#error "Only Windows has junctions"
#endif

/* Check if an argument is a switch */
int IsSwitch(char *arg) {
  switch (*arg) {
    case '-':
#if defined(_WIN32) || defined(_MSDOS)
    case '/':
#endif
      return (*(short*)arg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
}

/* WalkDirTree definitions */
typedef struct {
  int iVerbose;
  long nJunction;
} JCB_REF;
int ShowJunctionsCB(char *pszRelPath, struct dirent *pDE, void *pJcbRef);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     Main program routine				      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS.			      |
|									      |
|   Notes								      |
|									      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

void usage() {
  puts(PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: " PROGRAM_NAME " [OPTIONS] JUNCTION [TARGET_DIR]\n\
\n\
  -?      Display this help and exit\n\
  -1      Make sure to search linked folders only once. (slower)\n\
  -a      Display the raw absolute target. Default: Display the relative target\n"
#ifdef _DEBUG
"\
  -D      Enable debug output. Use twice to get extra debugging information\n"
#endif
"\
  -d      Delete the junction. Same as setting TARGET_DIR = \"\"\n\
  -f      Follow links to directories when searching recursively\n\
  -q      Quiet mode. Do not report access errors when searching recursively\n\
  -r|-s   Search junctions recursively in a directory tree\n\
  -V      Display this program version and exit\n\
  -v      Verbose mode. Report both the junction and target. Show search stats.\n\
\n\
Junction: The junction to manage. By default, read and display the target.\n\
 o Targets on the same drive as the junction are shown as a relative path.\n\
 o Junctions on network drives are partially supported. See description below.\n\
\n\
Target dir: If specified, create a junction pointing to that target directory.\n\
 o Relative pathnames are automatically converted to absolute pathnames.\n\
 o Relative pathnames are relative to the junction, not to the curr. directory.\n\
 o Junctions on network drives are partially supported. See description below.\n\
 o Raw target names beginning with \\??\\ are used without any verification.\n\
   Use at your own risk!\n\
\n\
Heuristics for managing junctions on network shares:\n\
  The problem is that junctions record the absolute target pathname as seen on\n\
  the server side, not on the client side. This program attempts to find the\n\
  share's server side base path by trying the following rules in sequence:\n\
   1. Share names with one letter + a $ refer to the drive root. Ex: C$ -> C:\\\n\
   2. Read the base path stored in file \\\\SERVER\\SHARE\\_Base_Path.txt\n\
   3. Share names with one letter also refer to the drive root. Ex: C -> C:\\\n\
   4. Longer names refer to a folder on drive C. Ex: Public -> C:\\Public\n\
  Warning: The first two rules are reliable, the next two are not!\n\
  It may be possible to get the server base path from the client using WMI:\n\
    wmic /node:SERVER share where name=\"SHARE\" get path\n\
  Once found, it is best to store that path in \\\\SERVER\\SHARE\\_Base_Path.txt.\
");
}

typedef enum {	/* Action to do */
  ACT_CREATE,		/* Create a new junction */
  ACT_RAWGET,		/* Get the raw target of a junction */
  ACT_GET,		/* Get the relatie target of a junction */
  ACT_GETID,		/* Get a file ID */
  ACT_DELETE,		/* Delete a junction */
  ACT_SCAN		/* Search recursively for junctions */
} action_t;

int main(int argc, char *argv[]) {
  int i;
  int iErr = 1;		/* Assume failure */
  char *pszJunction = NULL;
  char *pszTarget = NULL;
  char buf[PATH_MAX];
  int iVerbose = FALSE;
  action_t action = ACT_GET;
  wdt_opts opts = {0};		/* Must be cleared before use */
  JCB_REF jcbRef = {0};		/* Must be cleared before use */

  opts.iFlags |= WDT_CONTINUE;	/* Continue searching after recoverable errors */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {	/* This is a switch */
      char *opt = arg + 1;
      if (streq(opt, "?")) {
      	usage();
	return 0;
      }
      if (streq(opt, "1")) {	/* Make sure to scan directories only once */
	opts.iFlags |= WDT_ONCE; /* Slower, but ensures duplicate paths aren't explored twice */
	continue;
      }
      if (streq(opt, "a")) {
	action = ACT_RAWGET;
	continue;
      }
      if (streq(opt, "C")) {	/* Test the effect of WDT_CONTINUE  */
	opts.iFlags &= ~WDT_CONTINUE;
	continue;
      }
      DEBUG_CODE(
	if (streq(opt, "D")) {
	  DEBUG_MORE();
	  continue;
	}
      )
      if (streq(opt, "d")) {
	action = ACT_DELETE;
	continue;
      }
      if (streq(opt, "f")) {	/* Follow symlinkds and junctions */
	opts.iFlags |= WDT_FOLLOW;
	continue;
      }
      if (streq(opt, "i")) {
	action = ACT_GETID;
	continue;
      }
      if (streq(opt, "nobanner")) {
	continue;		/* For MS compatibility. Ignore that */
      }
      if (streq(opt, "q")) {	/* Quiet mode: Ignore access errors, warnings & infos */
	opts.iFlags |= WDT_QUIET;
	continue;
      }
      if (streq(opt, "r") || streq(opt, "s")) {
	action = ACT_SCAN;
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	return 0;
      }
      if (streq(opt, "v")) {
	iVerbose = TRUE;
	jcbRef.iVerbose = TRUE;
	continue;
      }
      pferror("Unknown option: %s", arg);
      return 2;
    }
    /* This is an argument, not a switch */
    if (!pszJunction) {
      pszJunction = arg;
      continue;
    }
    if (!pszTarget) {
      if (arg[0]) {
	pszTarget = arg;
	action = ACT_CREATE;
      } else { /* Empty target => Delete the junction */
	action = ACT_DELETE;
      }
      continue;
    }
    pferror("Unexpected argument: %s", arg);
#if defined(_MSC_VER)
    if (i) /* Stupid hack preventing the next line, at the end of the for {block}, from */
#endif     /* causing an unreachable code warning in ALL versions of the MS C compiler! */
    return 2;
  }

  if (action == ACT_GETID) { /* Scan a directory tree for junctions */
    char *pszPath = pszJunction ? pszJunction : ".";
    FILE_ID fid;
    BOOL bDone = MlxGetFileID(pszPath, &fid);
    if (!bDone) {
      pferror("Failed to get the file ID for \"%s\". %s", pszPath, strerror(errno));
      return 1;
    }
    if (fid.dwIDVol1 || fid.dwIDFil3 || fid.dwIDFil2) {
      printf("Volume SN %08.8lX%08.8lX, File ID %08.8lX%08.8lX%08.8lX%08.8lX\n",
      	     fid.dwIDVol1, fid.dwIDVol0, fid.dwIDFil3, fid.dwIDFil2, fid.dwIDFil1, fid.dwIDFil0);
    } else {
      printf("Volume ID %08.8lX, File ID %08.8lX%08.8lX\n", fid.dwIDVol0, fid.dwIDFil1, fid.dwIDFil0);
    }
    return 0;
  }

  if (action == ACT_SCAN) { /* Scan a directory tree for junctions */
    char *pszDir = pszJunction ? pszJunction : ".";
    iErr = WalkDirTree(pszDir, &opts, ShowJunctionsCB, &jcbRef);
    if (iVerbose) {
      printf("# Scanned %lld entries in %lld directories, and found %ld junctions\n",
	     (long long)opts.nFile, (long long)opts.nDir, jcbRef.nJunction);
    }
    if (opts.nErr) {
      int nSkipped = opts.nErr;
      if (iErr == -1) nSkipped -= 1; /* Remove the final unrecoverable error, which _was_ displayed last */
      if (opts.iFlags & WDT_QUIET) { /* Most errors were hidden and ignored */
	if (nSkipped) fprintf(stderr, "Warning: %d errors were ignored\n", nSkipped);
      } else { /* All errors were displayed. Print a summary */
	if (   (nSkipped > 1)
	    || ((nSkipped == 1) && (jcbRef.nJunction > 10))) {
	  fprintf(stderr, "Notice: %d errors were ignored\n", nSkipped);
	}
      }
    }
    return (iErr == -1) ? 1 : 0;
  }

  if (!pszJunction) {
    pferror("No junction name specified. Use option -? to get help");
    return 1;
  }

  if (action == ACT_CREATE) { /* Create a junction to this target */
    iErr = junction(pszTarget, pszJunction);
    if (iErr) {
      pferror("Failed to create junction \"%s\". %s", pszJunction, strerror(errno));
      return 1;
    }
    if (iVerbose) {
      printf("%s -> %s\n", pszJunction, pszTarget);
    } else {
      printf("%s\n", pszJunction);
    }
    return 0;
  }

  if (action == ACT_RAWGET) {
    /* Read the raw junction target */
    DWORD dwTag = MlxReadLinkU(pszJunction, buf, sizeof(buf));
    if (!dwTag) {
      pferror("Failed to read junction \"%s\"", pszJunction, strerror(errno));
      return 1;
    }
    if (dwTag != IO_REPARSE_TAG_MOUNT_POINT) {
      pferror("\"%s\" is not a junction", pszJunction);
      return 1;
    }
    if (iVerbose) {
      printf("%s -> %s\n", pszJunction, buf);
    } else {
      printf("%s\n", buf);
    }
    return 0;
  }

  if (action == ACT_GET) {
    /* Read the link, as if it were a relative symbolic link */
    iErr = (int)readlink(pszJunction, buf, sizeof(buf));
    if (iErr == -1) {
      pferror("Failed to read junction \"%s\". %s", pszJunction, strerror(errno));
      return 1;
    }
    if (iVerbose) {
      printf("%s -> %s\n", pszJunction, buf);
    } else {
      printf("%s\n", buf);
    }
    return 0;
  }

  if (action == ACT_DELETE) {
    DWORD dwTag;
    iErr = (int)readlink(pszJunction, buf, sizeof(buf));
    if (iErr == -1) {
      if (errno == ENOENT) return 0;	/* Deleted an already deleted junction is OK */
not_junction:
      pferror("\"%s\" is not a junction. %s", pszJunction, strerror(errno));
      return 1;
    }
    dwTag = MlxGetReparseTag(pszJunction);
    if (dwTag != IO_REPARSE_TAG_MOUNT_POINT) goto not_junction;
    iErr = unlink(pszJunction);
    if (iErr) {
      pferror("Failed to delete junction \"%s\". %s", pszJunction, strerror(errno));
      return 1;
    }
    if (iVerbose) {
      printf("%s -> %s\n", pszJunction, buf);
    } else {
      printf("%s\n", buf); /* Display the target of the deleted link */
    }
    return 0;
  }

  return 0;
}

#pragma warning(disable:4100) /* Ignore the unreferenced formal parameter warning */

int ShowJunctionsCB(char *pszRelPath, struct dirent *pDE, void *pRef) {
  DWORD dwTag;
  JCB_REF *pJcbRef = (JCB_REF *)pRef;
  int iVerbose = pJcbRef->iVerbose;

  if (pDE->d_type != DT_LNK) return 0;

  dwTag = MlxGetReparseTag(pszRelPath);
  if (dwTag != IO_REPARSE_TAG_MOUNT_POINT) return 0;

  pJcbRef->nJunction += 1;

  if (iVerbose) {
    char buf[PATH_MAX];
    int iErr = (int)readlink(pszRelPath, buf, sizeof(buf));
    if (iErr == -1) buf[0] = '\0';
    printf("%s -> %s\n", pszRelPath, buf);
  } else {
    printf("%s\n", pszRelPath);
  }
  return 0;
}

#pragma warning(default:4100) /* Restore the unreferenced formal parameter warning */

