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
*                                                                             *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage NTFS junctions as if they were relative symbolic links"
#define PROGRAM_NAME    "junction"
#define PROGRAM_VERSION "2022-10-19"

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
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#ifndef _WIN32
#error "Only Windows has junctions"
#endif

/* Local definitions and forward references */
#define JCB_VERBOSE 0x0001
#define JCB_ONCE    0x0002
typedef struct { /* Reference data to pass to the WalkDirTree callback */
  int iFlags;		/* Input: A combination of JCB_xxx flags */
  long nJunction;	/* Output: The number of junctions found */
  void *pTree;		/* Internal: The binary tree of known junctions */
} JCB_REF;	 /* Initialize as {0}, except for the inut flags */
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
  -1      Make sure to list junctions only once. (Useful with -f) \n\
  -a      Display the raw absolute target. Default: Display the relative target\n"
#ifdef _DEBUG
"\
  -D      Enable debug output. Use twice to get extra debugging information\n"
#endif
"\
  -d      Delete the junction. Same as setting TARGET_DIR = \"\"\n\
  -f      Follow junctions and symlinkds when searching recursively\n\
  -l DIR  List junctions in a directory\n\
  -o      Make sure to search linked folders only once. (slower, useful w. -f)\n\
  -q      Quiet mode. Do not report access errors when searching recursively\n\
  -r|-s DIR  List junctions recursively in a directory tree\n\
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
      if (streq(opt, "1")) {	/* Make sure to list junctions only once */
	jcbRef.iFlags |= JCB_ONCE; /* Ensures duplicate paths to the same junction aren't listed twice */
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
      if (streq(opt, "l")) {
	action = ACT_SCAN;
	opts.iFlags |= WDT_NORECURSE;
	continue;
      }
      if (streq(opt, "nobanner")) {
	continue;		/* For MS compatibility. Ignore that */
      }
      if (streq(opt, "o")) {	/* Make sure to scan directories only once */
	opts.iFlags |= WDT_ONCE; /* Slower, but ensures duplicate paths aren't explored twice */
	continue;
      }
      if (streq(opt, "q")) {	/* Quiet mode: Ignore access errors, warnings & infos */
	opts.iFlags |= WDT_QUIET;
	continue;
      }
      if (streq(opt, "r") || streq(opt, "s")) {
	action = ACT_SCAN;
	opts.iFlags &= ~WDT_NORECURSE;
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
int ShowJunctionsCB(char *pszRelPath, struct dirent *pDE, void *pRef) {
  DWORD dwTag;
  JCB_REF *pJcbRef = (JCB_REF *)pRef;
  int iVerbose = pJcbRef->iFlags & JCB_VERBOSE;
  int iOnce = pJcbRef->iFlags & JCB_ONCE;
  tree *pTree = pJcbRef->pTree;

  if (pDE->d_type != DT_LNK) return 0;

  dwTag = MlxGetReparseTag(pszRelPath);
  if (dwTag != IO_REPARSE_TAG_MOUNT_POINT) return 0;

  if (iOnce) { /* Check if that same junction has been seen before */
    struct stat sStat;
    int iErr;

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

  if (iVerbose) {
    char buf[PATH_MAX];
    int iErr = (int)readlink(pszRelPath, buf, sizeof(buf));
    if (iErr == -1) buf[0] = '\0';
    printf("%s -> %s\n", pszRelPath, buf);
  } else {
    printf("%s\n", pszRelPath);
  }
  return 0;

out_of_memory:
  pferror("Out of memory");
  return -1;
}

#pragma warning(default:4100) /* Restore the unreferenced formal parameter warning */

