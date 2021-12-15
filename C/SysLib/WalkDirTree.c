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
*                                                                             *
\*****************************************************************************/

#define _UTF8_SOURCE

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debugging macros */

/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
#include "pathnames.h"		/* Pathname management definitions and functions */
#include "mainutil.h"		/* Print errors, streq, etc */

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
|   Function	    WalkDirTree						      |
|									      |
|   Description     Invoke a callback for every file in a directory tree      |
|									      |
|   Parameters      char *path		The directory pathname		      |
|		    							      |
|   Returns	    0=Success, else the number of errors encountered	      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2021-12-14 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int WalkDirTree(char *path, wdt_opts *pOpts, pWalkDirTreeCB_t pWalkDirTreeCB, void *pRef) {
  char *pPath;
  int iRet = 0;
  DIR *pDir;
  struct dirent *pDE;

  DEBUG_ENTER(("WalkDirTree(\"%s\", ...);\n", path));

  if ((!path) || !strlen(path)) RETURN_INT_COMMENT(1, ("path is empty\n"));

  pDir = opendirx(path);
  if (!pDir) {
    if (!(pOpts->iFlags & WDT_IGNOREERR)) {
      pferror("Can't access \"%s\": %s", path, strerror(errno));
      iRet = 1;
    }
    goto fail;
  }

  pPath = path;
  if (streq(pPath, ".")) pPath = NULL;	/* Hide the . path in the output */

  while ((pDE = readdirx(pDir)) != NULL) { /* readdirx() ensures d_type is set */
    char *pPathname;
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    if (streq(pDE->d_name, ".")) continue;	/* Skip the . directory */
    if (streq(pDE->d_name, "..")) continue;	/* Skip the .. directory */

    pPathname = NewCompactJoinedPath(pPath, pDE->d_name);
    if (!pPathname) goto out_of_memory;

    iRet = pWalkDirTreeCB(pPathname, pDE, pRef);
    if (pOpts->pNProcessed && (iRet <= 0)) *(pOpts->pNProcessed) += 1; /* Number of files successfully processed */
    if (iRet) break;	/* 1 = error; -1 = abort the walk */

    switch (pDE->d_type) {
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
      case DT_LNK:
	if (!(   (pOpts->iFlags & WDT_LRECURSE)
	      && (isEffectiveDir(pPathname)))) break;
	/* Else fallthrough into the directory case */
#endif
      case DT_DIR:
      	if (!(pOpts->iFlags & WDT_NORECURSE)) {
      	  iRet = WalkDirTree(pPathname, pOpts, pWalkDirTreeCB, pRef);
      	}
      	break;
      default:
      	break;
    }
    free(pPathname);
    if (iRet) break;	/* 1 = error; -1 = abort the walk */
  }
  closedirx(pDir);
  goto cleanup_and_return;

out_of_memory:
  pferror("Out of memory");
fail:
  pOpts->nErr += 1;
  iRet = 1;
  goto cleanup_and_return;

cleanup_and_return:
  RETURN_INT_COMMENT(iRet, ((iRet == -1) ? "Cancel walk\n" : "Continue walk\n"));
}

