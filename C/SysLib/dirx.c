/*****************************************************************************\
*                                                                             *
*   Filename:	    dirx.c						      *
*									      *
*   Description:    Directory access functions eXtensions		      *
*                                                                             *
*   Notes:	    Work around the problem of readdir() not always setting   *
*		    d_type for some file systems in Unix		      *
*		    							      *
*		    TO DO: Have two distinct readdirx and readdirx64 functions,
*		    and a readdirx macro in dirx.h pointing to the right      *
*		    function depending on the _FILE_OFFSET_BITS value.	      *
*		    							      *
*   History:								      *
*    2020-03-11 JFL Created this file.					      *
*    2020-03-19 JFL Use 64-bits file sizes even in 32-bits OSs, like that in  *
*		    the Raspberry Pi 2.					      *
*									      *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _BSD_SOURCE    		/* Define BSD extensions. Ex: S_IFREG in sys/stat.h */
#define _DEFAULT_SOURCE		/* glibc >= 2.19 will complain about _BSD_SOURCE if it doesn't see this */
#define _LARGEFILE_SOURCE	/* Define LFS extensions. Ex: type off_t, and functions fseeko and ftello */
#define _GNU_SOURCE		/* Implies all the above */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes by default, if possible */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "dirx.h"		/* Directory eXtensions definitions */

/******************************************************************************
*                                                                             *
*       Function        opendirx / readdirx / closedirx                       *
*                                                                             *
*       Description     Front-end to readdir(), always setting d_type.        *
*                                                                             *
*       Notes           In Unix, some file systems set d_type = UNKNOWN.      *
*                       It is necessary to call lstat in this case.           *
*                                                                             *
*                       Known readdirx() limitation: The calling routine      *
*                       must not change the current directory within the      *
*                       readdir() loop.                                       *
*                                                                             *
*       History                                                               *
*        2020-03-11 JFL Created these routines.                               *
*                                                                             *
******************************************************************************/

/* Extended DIR structure, storing the additional information we need */
typedef struct _DIRX {
  DIR *pDir;
  const char *pDirName;
  struct dirent de;
} DIRX;

DIR *opendirx(const char *pDirName) {
  DIR *pDir;
  DIRX *pDirx;

  pDir = opendir(pDirName);
  if (!pDir) return pDir; /* opendir failure is more likely than malloc failure */
  pDirx = malloc(sizeof(DIRX));
  if (!pDirx) goto failed;
  pDirx->pDir = pDir;
  pDirx->pDirName = strdup(pDirName);
  if (!pDirx->pDirName) goto failed;
  return (DIR *)pDirx;    /* Pretend it's a DIR structure */
failed:
  free(pDirx);
  closedir(pDir);
  return NULL;
}

struct dirent *readdirx(DIR *pDir) {
  DIRX *pDirx = (DIRX *)pDir;
  struct stat sStat;
  char *path;
  size_t l;
  int err;
  struct dirent *pDE = readdir(pDirx->pDir); /* Read the actual DIR */
  if (!pDE) return pDE;
  if (pDE->d_type != DT_UNKNOWN) return pDE; /* No need for the workaroud */

  pDirx->de = *pDE;	/* Copy the data, as the original is not writable */
  pDE = &(pDirx->de);	/* Refer to the copy now on */

  /* Generate the pathname of the directory entry */
  path = malloc(strlen(pDirx->pDirName) + strlen(pDE->d_name) + 2);
  if (!path) return pDE; /* Sorry, we can't do any better for lack of memory */
  strcpy(path, pDirx->pDirName);
  l = strlen(path);
  if (l && (path[l-1] != '/')) path[l++] = '/';
  strcpy(path+l, pDE->d_name);
  /* Get the directory entry type */
  err = -lstat(path, &sStat);
  free(path);
  if (err) return pDE; /* Sorry, we can't do any better for lack of information */
  /* Convert the stat mode to a directory entry d_type */
  if      (S_ISREG(sStat.st_mode))  pDE->d_type = DT_REG;
  else if (S_ISDIR(sStat.st_mode))  pDE->d_type = DT_DIR;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* If the OS has links (For some OSs which don't, macros are defined, but always returns 0) */
  else if (S_ISLNK(sStat.st_mode))  pDE->d_type = DT_LNK;
#endif
#if defined(S_ISCHR) && S_ISCHR(S_IFCHR) /* If the OS has character devices (For some OSs which don't, macros are defined, but always returns 0) */
  else if (S_ISCHR(sStat.st_mode))  pDE->d_type = DT_CHR;
#endif
#if defined(S_ISBLK) && S_ISBLK(S_IFBLK) /* If the OS has block devices (For some OSs which don't, macros are defined, but always returns 0) */
  else if (S_ISBLK(sStat.st_mode))  pDE->d_type = DT_BLK;
#endif
#if defined(S_ISFIFO) && S_ISFIFO(S_IFFIFO) /* If the OS has fifos (For some OSs which don't, macros are defined, but always returns 0) */
  else if (S_ISFIFO(sStat.st_mode)) pDE->d_type = DT_FIFO;
#endif
#if defined(S_ISSOCK) && S_ISSOCK(S_IFSOCK) /* If the OS has sockets (For some OSs which don't, macros are defined, but always returns 0) */
  else if (S_ISSOCK(sStat.st_mode)) pDE->d_type = DT_SOCK;
#endif
  return pDE;
}

int closedirx(DIR *pDir) {
  DIRX *pDirx = (DIRX *)pDir;
  pDir = pDirx->pDir;		/* The actual DIR structure pointer */
  free((void *)(pDirx->pDirName));
  free(pDirx);
  return closedir(pDir);
}
