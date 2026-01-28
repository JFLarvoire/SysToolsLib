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
*    2022-01-19 JFL Added option -l (dash-L) to list junctions non-recursively.
*    2022-02-18 JFL Rewrote option -1 (dash-one) to record the junction       *
*                   themselves in a binary tree. Renamed the old -1 as -o.    *
*    2022-10-19 JFL Moved IsSwitch() to SysLib.				      *
*    2022-10-22 JFL Added option -t to list all types of reparse points.      *
*    2023-11-15 JFL Bug fix: When listing junctions in verbose mode, the      *
*                   target was not displayed.                                 *
*    2024-09-26 JFL Fixed a minor warning.                                    *
*    2025-07-26 JFL Fixed option -a to make it compatible with -l and -r.     *
*    2025-07-27 JFL Renamed -a as -R for Raw, and new -a now means Absolute.  *
*                   Also -a and -r can now be used in any order.              *
*                   Document the -i option in the debug version.              *
*    2025-07-29 JFL Added the ability to expose hidden (Ex. Cloud) links.     *
*    2025-07-30 JFL Display WCI links targets & cloud links offline mode.     *
*    2025-08-03 JFL Restructured, and added Linux fifo, char, & block devices.*
*    2026-01-27 JFL Output a Docker "🐋" whale ahead of WCI links targets,    *
*		    to make it clear its root is in a container.	      *
*    2026-01-28 JFL Output type-specific arrows for other reparse points types.
*		    Added option -m to limit the recursive search depth.      *
*                                                                             *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage NTFS junctions as if they were relative symbolic links"
#define PROGRAM_NAME    "junction"
#define PROGRAM_VERSION "2026-01-28"

#define _CRT_SECURE_NO_WARNINGS
#define _UTF8_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros. Include first. */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "pathnames.h"	/* SysLib Pathname management definitions and functions */
#include "tree.h"	/* SysToolsLib Manage a binary tree */
#include "reparsept.h"  /* MsvcLibX Reparse Point management */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#ifndef _WIN32
#error "Only Windows has junctions"
#endif

/* Local definitions and forward references */
#define JCB_VERBOSE   0x0001
#define JCB_ONCE      0x0002
#define JCB_ALLTYPES  0x0004
#define JCB_RAW       0x0008
#define JCB_ABS       0x0010
typedef struct { /* Reference data to pass to the WalkDirTree callback */
  int iFlags;		/* Input: A combination of JCB_xxx flags */
  long nJunction;	/* Output: The number of junctions found */
  void *pTree;		/* Internal: The binary tree of known junctions */
} JCB_REF;	 /* Initialize as {0}, except for the inut flags */
int ShowJunctionsCB(const char *pszRelPath, const struct dirent *pDE, void *pJcbRef);
int ExposeAllReparsePoints(void);
char *GetTagArrow(DWORD dwTag);

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
  fputs(PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: " PROGRAM_NAME " [OPTIONS] JUNCTION [TARGET_DIR]\n\
\n\
  -?      Display this help and exit\n\
  -1      Make sure to list junctions only once. (Useful with -f) \n\
  -a      Display the absolute target. Default: Display the relative target\n"
#ifdef _DEBUG
"\
  -D      Enable debug output. Use twice to get extra debugging information\n"
#endif
"\
  -d      Delete the junction. Same as setting TARGET_DIR = \"\"\n\
  -f      Follow junctions and symlinkds when searching recursively\n"
#ifdef _DEBUG
"\
  -i PATHNAME  Get the unique file ID\n"
#endif
"\
  -l DIR  List junctions in a directory\n\
  -m MAX  Maximum depth when searching recursively. Default: 0 = unlimited\n\
  -o      Make sure to search linked folders only once. (slower, useful w. -f)\n\
  -q      Quiet mode. Do not report access errors when searching recursively\n\
  -R      Display the raw junction target. Default: Display the relative target\n\
  -r|-s DIR  List junctions recursively in a directory tree\n\
  -t      With -l or -r, list all types of reparse points, with their types\n\
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
\n\
This program detects link loops, and silently avoids getting caught.\n\
Use option -v to display warnings about loops detected.\n\
Likewise, the -v option informs about duplicate paths that were skipped.\n\
"
#include "footnote.h"
, stdout);
}

typedef enum {	/* Action to do */
  ACT_CREATE,		/* Create a new junction */
  ACT_GET,		/* Get the target of a junction */
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
  int iRaw = FALSE;
  int iAbs = FALSE;
  int iVerbose = FALSE;
  int iAllTypes = FALSE;
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
      if (streq(opt, "1")) {	/* Make sure to list junctions only once */
	jcbRef.iFlags |= JCB_ONCE; /* Ensures duplicate paths to the same junction aren't listed twice */
	continue;
      }
      if (streq(opt, "a")) {	/* Show the absolute target path */
	jcbRef.iFlags |= JCB_ABS;
      	iAbs = TRUE;
	continue;
      }
      if (streq(opt, "accepteula")) { /* For MS compatibility. Ignore that */
	continue;
      }
      if (streq(opt, "C")) {	/* Test the effect of WDT_CONTINUE  */
	opts.iFlags &= ~WDT_CONTINUE;
	continue;
      }
      DEBUG_CODE(
	if (streq(opt, "D")) {	/* Enable the debug mode */
	  DEBUG_MORE();
	  continue;
	}
      )
      if (streq(opt, "d")) {	/* Delete a junction */
	action = ACT_DELETE;
	continue;
      }
      if (streq(opt, "f")) {	/* Follow symlinkds and junctions */
	opts.iFlags |= WDT_FOLLOW;
	continue;
      }
      if (streq(opt, "i")) {	/* Display the unique file ID */
	action = ACT_GETID;
	continue;
      }
      if (streq(opt, "l")) {	/* List all junctions in a directory */
	action = ACT_SCAN;
	opts.iFlags |= WDT_NORECURSE;
	continue;
      }
      if (streq(opt, "m")) {	/* Maximum depth */
	if (((i+1)<argc) && !IsSwitch(argv[i+1])) {
	  opts.iMaxDepth = atoi(argv[++i]);
	} else {
	  pferror("Max depth value missing");
	  return 2;
	}
	continue;
      }
      if (streq(opt, "nobanner")) { /* For MS compatibility. Ignore that */
	continue;
      }
      if (streq(opt, "o")) {	/* Make sure to scan directories only once */
	opts.iFlags |= WDT_ONCE; /* Slower, but ensures duplicate paths aren't explored twice */
	continue;
      }
      if (streq(opt, "q")) {	/* Quiet mode: Ignore access errors, warnings & infos */
	opts.iFlags |= WDT_QUIET;
	continue;
      }
      if (streq(opt, "R")) {	/* Show the raw junction target */
	jcbRef.iFlags |= JCB_RAW;
      	iRaw = TRUE;
	continue;
      }
      if (streq(opt, "r") || streq(opt, "s")) {	/* List recursively all junctions in a directory tree */
	action = ACT_SCAN;
	opts.iFlags &= ~WDT_NORECURSE;
	continue;
      }
      if (streq(opt, "t")) {	/* With -l or -r, list all types of reparse points, not just junctions */
	jcbRef.iFlags |= JCB_ALLTYPES;
	iAllTypes = TRUE;
	ExposeAllReparsePoints();
	continue;
      }
      if (streq(opt, "V")) {	/* Display version */
	puts(DETAILED_VERSION);
	return 0;
      }
      if (streq(opt, "v")) {
	iVerbose = TRUE;
	jcbRef.iFlags |= JCB_VERBOSE;
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

  if (action == ACT_GETID) { /* Debug: Dump the volume and file ID */
    char *pszPath = pszJunction ? pszJunction : ".";
    FILE_ID fid;
    BOOL bDone = MlxGetFileID(pszPath, &fid);
    if (!bDone) {
      pfcerror("Failed to get the file ID for \"%s\"", pszPath);
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
      printf("# Scanned %lld entries in %lld directories, and found %ld %s\n",
	     (long long)opts.nFile, (long long)opts.nDir, jcbRef.nJunction,
	     (jcbRef.iFlags & JCB_ALLTYPES) ? "reparse points" : "junctions");
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
      pfcerror("Failed to create junction \"%s\"", pszJunction);
      return 1;
    }
    if (iVerbose) {
      printf("%s -> %s\n", pszJunction, pszTarget);
    } else {
      printf("%s\n", pszJunction);
    }
    return 0;
  }

  if (action == ACT_GET) {
    int iOffset = 0;
    DWORD dwTag = MlxGetReparseTag(pszJunction);
    if (!dwTag) {
      pfcerror(NULL);
      return 1;
    }
    dwTag &= IO_REPARSE_TAG_TYPE_BITS;
    if ((dwTag != IO_REPARSE_TAG_MOUNT_POINT) && !iAllTypes) {
      pferror("This is not a junction. Use option -t to read other reparse point types.");
      return 1;
    }
    switch (dwTag) {
      case IO_REPARSE_TAG_CLOUD:
	strcpy(buf, "☁ \\?"); /* Add a space after the cloud, as the cloud is double width, but the cursor moves only 1 column */
	/* TO DO: We don't know how to get the target location in the cloud */
	iErr = 0;
	break;
      case IO_REPARSE_TAG_WCI:
	strcpy(buf, "🐋\\");
	iOffset = lstrlen(buf);
	iErr = MlxReadWci(pszJunction, buf + iOffset, sizeof(buf) - iOffset) ? 0 : -1;
	break;
      default: {
	if (iRaw | iAbs) {
      iErr = MlxReadLink(pszJunction, buf, sizeof(buf)) ? 0 : -1;
	} else {
	  /* Read the link, as if it were a relative symbolic link */
	  iErr = (int)readlink(pszJunction, buf, sizeof(buf));
	}
	break;
      }
    }
    if (iErr == -1) {
      pfcerror("Failed to read junction \"%s\"", pszJunction);
      return 1;
    }
    pszTarget = buf;
    if (iAbs) {
      if (!strncmp(pszTarget, "\\??\\", 4)) pszTarget += 4;
    }
    if (iVerbose) {
      char *pszArrow = iAllTypes ? GetTagArrow(dwTag) : "->";
      printf("%s %s %s\n", pszJunction, pszArrow, pszTarget);
    } else {
      printf("%s\n", pszTarget);
    }
    return 0;
  }

  if (action == ACT_DELETE) {
    DWORD dwTag;
    iErr = (int)readlink(pszJunction, buf, sizeof(buf));
    if (iErr == -1) {
      if (errno == ENOENT) return 0;	/* Deleted an already deleted junction is OK */
not_junction:
      pfcerror("\"%s\" is not a junction", pszJunction);
      return 1;
    }
    dwTag = MlxGetReparseTag(pszJunction);
    if (dwTag != IO_REPARSE_TAG_MOUNT_POINT) goto not_junction;
    iErr = unlink(pszJunction);
    if (iErr) {
      pfcerror("Failed to delete junction \"%s\"", pszJunction);
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

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ShowJunctionsCB					      |
|									      |
|   Description     Process files found in the tree, and display junctions    |
|		    							      |
|   Parameters	    							      |
|		    							      |
|   Returns	    0=Success, continue; 1=Success, stop; -1=Error, stop      |
|		    							      |
|   Notes	    Optionally records the junction file IDs in a binary tree,|
|		    to efficiently recognize the junctions that have already  |
|		    been listed.					      |
|		    The tree is not freed in the end, as this program exits.  |
|		    							      |
|   History	    							      |
*		    							      *
\*---------------------------------------------------------------------------*/

#include "tree.h"
typedef struct _knownJunction {
  NODE_FIELDS(struct _tree, struct _knownJunction); /* The tree-specific fields */
  /* Two key fields */
  dev_t devID;			/* Device ID and file ID uniquely identifying it */
  ino_t fileID;			/* "" */
  /* And one data field */
  const char *pszPathname;	/* Pathname for this visited directory */
} knownJunction;
typedef struct _tree {
  TREE_FIELDS(struct _tree, struct _knownJunction); /* The tree-specific fields */
  /* No global property fields */
} tree;
TREE_DEFINE_TYPES(tree, knownJunction);
TREE_DEFINE_PROCS(tree, knownJunction);
int TREE_CMP(knownJunction)(knownJunction *n1, knownJunction *n2) { /* Node comparison routine */
  if (n1->devID < n2->devID) return -1;
  if (n1->devID > n2->devID) return 1;
  if (n1->fileID < n2->fileID) return -1;
  if (n1->fileID > n2->fileID) return 1;
  return 0;
}
DEBUG_CODE(
int TREE_SPRINT(knownJunction)(char *buf, int len, knownJunction *n) {
  return snprintf(buf, len, "%lX:%llX \"%s\"", (long)n->devID, (long long)n->fileID, n->pszPathname);
}
)
/* Node allocation routine */
knownJunction *new_knownJunction(dev_t devID, ino_t fileID, const char *pszPathname) {
  knownJunction *node = calloc(1, sizeof(knownJunction));
  if (node) {
    node->devID = devID;
    node->fileID = fileID;
    node->pszPathname = pszPathname;
  }
  return node;
}

#pragma warning(disable:4100) /* Ignore the unreferenced formal parameter warning */

/* Callback called by WalkDirTree for every file it finds */
int ShowJunctionsCB(const char *pszRelPath, const struct dirent *pDE, void *pRef) {
  DWORD dwTag;
  JCB_REF *pJcbRef = (JCB_REF *)pRef;
  int iVerbose = pJcbRef->iFlags & JCB_VERBOSE;
  int iOnce = pJcbRef->iFlags & JCB_ONCE;
  int iAllTypes = pJcbRef->iFlags & JCB_ALLTYPES;
  int iRaw = pJcbRef->iFlags & JCB_RAW;
  int iAbs = pJcbRef->iFlags & JCB_ABS;
  tree *pTree = pJcbRef->pTree;
  char buf[PATH_MAX];
  char *pszTarget = buf;
  char *pszType = "?";
  char szTag[16];
  int iErr;

  if (!(pDE->d_attribs & FILE_ATTRIBUTE_REPARSE_POINT)) return 0;

  dwTag = pDE->d_ReparseTag;  /* dwTag = MlxGetReparseTag(pszRelPath); */
  if ((!iAllTypes) && (dwTag != IO_REPARSE_TAG_MOUNT_POINT)) return 0;

  if (iOnce) { /* Check if that same junction has been seen before */
    struct stat sStat;

    if (!pTree) { /* Create the tree, if this has not yet been done */
      pTree = pJcbRef->pTree = new_knownJunction_tree();
      if (!pTree) goto out_of_memory;
    }

    iErr = dirent2stat(pDE, &sStat);
    if ((!iErr) && !sStat.st_ino) {
      FILE_ID fid;
      BOOL bDone = MlxGetFileID(pszRelPath, &fid);
      if (bDone) {
	sStat.st_dev = *(dev_t *)(&fid.dwIDVol0);
	sStat.st_ino = *(ino_t *)(&fid.dwIDFil0);
	if (*(ino_t *)(&fid.dwIDFil2)) sStat.st_ino = 0; /* This is an ReFS ID that does not fit on 64 bits */
      }
    }
    if (sStat.st_ino) {
      char *pszDupName;
      knownJunction *pPrevious;
      knownJunction *pThis = new_knownJunction(sStat.st_dev, sStat.st_ino, NULL);
      if (!pThis) goto out_of_memory;
      pPrevious = get_knownJunction(pTree, pThis);
      if (pPrevious) { /* The same junction has been seen before under another pathname */
	if (iVerbose) {
	  fprintf(stderr, "Notice: Junction \"%s\" is the same as \"%s\"\n", pszRelPath, pPrevious->pszPathname);
	}
	free(pThis);
	return 0;
      }
      /* OK, we've not seen this junction before. Record its name and unique ID in the tree */
      pszDupName = strdup(pszRelPath);
      if (!pszDupName) {
        free(pThis);
      	goto out_of_memory;
      }
      pThis->pszPathname = pszDupName;
      add_knownJunction(pTree, pThis);
    }
  }

  pJcbRef->nJunction += 1;

  iErr = -1;
  switch (dwTag & IO_REPARSE_TAG_TYPE_BITS) {
    int iLen;
    case IO_REPARSE_TAG_MOUNT_POINT: {
      pszType = "Junction";
read_link:
      if (iRaw | iAbs) {
	iErr = MlxReadLink(pszRelPath, buf, sizeof(buf)) ? 0 : -1;
      } else {
	iErr = (int)readlink(pszRelPath, buf, sizeof(buf));
      }
      break;
    }
    case IO_REPARSE_TAG_SYMLINK: {
      pszType = "Symlink";
      /* Using MlxReadLink() in iRaw mode here makes no sense, as SymLinks can
         be relative or absolute, and the latter just have an extra \??\ prefix */
      iErr = (int)readlink(pszRelPath, buf, sizeof(buf));
      if (iErr != -1) {
	struct stat st;
	iErr = dirent2stat(pDE, &st);
	if ((!iErr) && (st.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) pszType = "SymlinkD";
      }
      break;
    }
    case IO_REPARSE_TAG_LX_SYMLINK: {
      pszType = "LinuxLink";
      goto read_link;
    }
    case IO_REPARSE_TAG_APPEXECLINK: {
      pszType = "AppExecLnk";
      iLen = MlxReadAppExecLink(pszRelPath, buf, sizeof(buf)); /* Get the target of an appexeclink */
      iErr = (iLen <= 0) ? -1 : 0;
      break;
    }
    case IO_REPARSE_TAG_CLOUD: {
      DWORD dwAttr = pDE->d_attribs;
      pszType = "CloudLnk";	/* Place Holder for a file in cloud storage */
      /* MsvcLibX does not know how to find the cloud target pathname */
      strcpy(buf, (dwAttr & FILE_ATTRIBUTE_OFFLINE) ? "↑" : "↕"); /* ↑=Remote ↕=Both ↓=Local */ 
      strcat(buf, "☁ \\?"); /* Add a space after the cloud, as the cloud is double width, but the cursor moves only 1 column */
      iErr = 0;
      break;
    }
    case IO_REPARSE_TAG_WCI: {
      DWORD dwAttr = pDE->d_attribs;
      pszType = "WciLink";
      /* MsvcLibX reads WCI links targets as relative to an unknown GUID-defined container root directory */
      strcpy(buf, (dwAttr & FILE_ATTRIBUTE_OFFLINE) ? "↑" : "↕"); /* ↑=Remote ↕=Both ↓=Local */ 
      strcat(buf, "🐋\\");
      iLen = lstrlen(buf);
      iErr = MlxReadWci(pszRelPath, buf + iLen, sizeof(buf) - iLen) ? 0 : -1;
      break;
    }
    case IO_REPARSE_TAG_AF_UNIX: {
      pszType = "LxSocket";
      pszTarget = NULL;
      iErr = 0;
      break;
    }
    case IO_REPARSE_TAG_LX_FIFO: {
      pszType = "LxFifo";
      pszTarget = NULL;
      iErr = 0;
      break;
    }
    case IO_REPARSE_TAG_LX_CHR: {
      pszType = "LxChr";
      pszTarget = NULL;
      iErr = 0;
      break;
    }
    case IO_REPARSE_TAG_LX_BLK: {
      pszType = "LxBlk";
      pszTarget = NULL;
      iErr = 0;
      break;
    }
    default: {
      pszType = szTag;
      sprintf(szTag, "0x%08X", (int)dwTag);
      break;
    }
  }
  if (iErr == -1) pszTarget = "?";

  if ((!iVerbose) || (!pszTarget)) {
    if (!iAllTypes) {
      printf("%s\n", pszRelPath);
    } else {
      printf("%-10s %s\n", pszType, pszRelPath);
    }
  } else {
    if (iAbs) {
      if (!strncmp(pszTarget, "\\??\\", 4)) pszTarget += 4;
    }
    if (!iAllTypes) {
      printf("%s -> %s\n", pszRelPath, pszTarget);
    } else {
      char *pszArrow = GetTagArrow(dwTag);
      printf("%-10s %s %s %s\n", pszType, pszRelPath, pszArrow, pszTarget);
    }
  }
  return 0;

out_of_memory:
  pferror("Out of memory");
  return -1;
}

#pragma warning(default:4100) /* Restore the unreferenced formal parameter warning */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    ExposeAllReparsePoints				      |
|									      |
|   Description     Make sure the application sees all reparse points         |
|		    							      |
|   Parameters	    							      |
|		    							      |
|   Returns	    0=Success						      |
|		    							      |
|   Notes	    https://stackoverflow.com/questions/59152220/cant-get-reparse-point-information-for-the-onedrive-folder
|		    https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetprocessplaceholdercompatibilitymode
|		    							      |
|   History	    							      |
|    2025-07-29 JFL Created this routine.				      |
|    2025-08-03 JFL Moved the real action to MsvcLibX's MlxShowPlaceholders().|
*		    							      *
\*---------------------------------------------------------------------------*/

int ExposeAllReparsePoints() {
  int iLastPHCM;

  DEBUG_ENTER(("ExposeAllReparsePoints();\n"));

  /* Cloud links must be exposed, else they're visible as normal files */
  iLastPHCM = MlxShowPlaceholders();
  if (iLastPHCM < 0) { /* Function return negative values in case of error */
    /* Ignore the error, as this may fail on old versions of Windows that
       do not support the API, and this is not a problem. */
    /* RETURN_INT_COMMENT(1, ("Failed to enable PHCM\n")); */
    DEBUG_PRINTF(("# Ignoring failure to enable PHCM\n"));
  }

  /* Room for possible future tricks to enable more hidden reparse points */

  RETURN_INT_COMMENT(0, ("Success\n"));
}
	
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetTagArrow						      |
|									      |
|   Description     Return an arrow uniquely identifying the tag type         |
|		    							      |
|   Parameters	    							      |
|		    							      |
|   Returns	    							      |
|		    							      |
|   Notes	    Prefer ASCII or Unicode 1 symbols for link types that     |
|		    may occur on Windows XP, as XP does not support Unicode 2.|
|		    For link types introducted in Windows 7 or 8, prefer      |
|		    Unicode 1 symbols.					      |
|		    For link types introducted in Windows 10 or later, any    |
|		    unicode char. is OK, and the more colorfull the better.   |
|		    							      |
|   History	    							      |
|    2026-01-28 JFL Created this routine.				      |
*		    							      *
\*---------------------------------------------------------------------------*/

char *GetTagArrow(DWORD dwTag) {
  switch (dwTag & IO_REPARSE_TAG_TYPE_BITS) {
    case IO_REPARSE_TAG_MOUNT_POINT: /* = Junction */
      return "-J>"; /* J or 💽 or 💾 or 📀 considered */
    case IO_REPARSE_TAG_SYMLINK:
      return "-S>"; /* S or 📁 or 📂 or 📃 or 📄 considered */
    case IO_REPARSE_TAG_LX_SYMLINK:
    case IO_REPARSE_TAG_AF_UNIX:
    case IO_REPARSE_TAG_LX_FIFO: /* ➰ considered */
    case IO_REPARSE_TAG_LX_CHR:
    case IO_REPARSE_TAG_LX_BLK:
      return "-🐧>";
    case IO_REPARSE_TAG_APPEXECLINK:
      return "-🔥>"; /* X or 🔥 considered */
    case IO_REPARSE_TAG_CLOUD:
      return "-☁ >"; /* Add a space after the cloud, as the cloud is double width, but the cursor moves only 1 column */
    case IO_REPARSE_TAG_WCI:
      return "-🐋>"; /* Docker 🐋 whale, ⛟ or ✉ considered as symbols for a container */
    default:
      return "-?>";
  }
}
