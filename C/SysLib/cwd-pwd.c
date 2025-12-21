/*****************************************************************************\
*                                                                             *
*   File name	    cwd-pwd.c						      *
*                                                                             *
*   Description	    Managed the logical directory in Unix                     *
*                                                                             *
*   Notes	    In Unix, the logical current directory is provided        *
*		    by shells in the PWD environment variable.                *
*		    The physical current directory may be different	      *
*		    if the logical CD contains links in its pathname.         *
*		    Ex: Using the standard C library routines, the sequence...*
*		      chdir(link_to_dir);                                     *
*		      chdir("..");                                            *
*		    ... does not return to the initial directory!             *
*		    It actually returns to the parent of the target dir.      *
*                                                                             *
*                   The routines GetCWD() and ChDir() here manage both        *
*                   the kernel's CWD, and the environment's PWD, to make      *
*                   sure that ChDir("..") always returns to the initial dir.  *
*                                                                             *
*   History                                                                   *
*    2025-11-27 JFL Created this file.					      *
*    2025-12-19 JFL Fixed ChDir() which may have failed without setting iErr. *
*                                                                             *
\*****************************************************************************/

#define _UTF8_LIB_SOURCE
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#define _CRT_SECURE_NO_WARNINGS /* Prevent MSVC warnings about unsecure C library functions */

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

/* SysToolsLib include files */
#include "debugm.h"		/* SysToolsLib debugging macros */

/* SysLib include files */
#include "dirx.h"		/* Directory access functions eXtensions */
#include "pathnames.h"		/* Pathname management definitions and functions */

#define streq(s1, s2) (!strcmp(s1, s2))
#define TRUE 1
#define FALSE 0

#ifdef _UNIX

#undef chdir			/* Here, chdir means the real chdir() */
#undef getcwd			/* Here, getcwd means the real getcwd() */

/*---------------------------------------------------------------------------*\
*									      *
|   Function	    GetCWD						      |
|									      |
|   Description	    Get the Unix logical current working directory	      |
|		    							      |
|   Arguments	    Same as the standard C library's getcwd()                 |
|		    							      |
|   Return value    Same as the standard C library's getcwd()                 |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History                                                                   |
|    2025-11-27 JFL Created this routine.				      |
|    2025-11-28 JFL Make sure PWD is valid, and if not, correct it.	      |
*									      *
\*---------------------------------------------------------------------------*/

char *GetCWD(char *buf, size_t len) {
  char *pszPWD;		/* The logical dir in the PWD variable */
  struct stat stLog;
  char *pszCWD;		/* The physical dir reported by getcwd() */
  struct stat stPhys;
  int iErr;

  if (!buf) return NewCWDString();

  DEBUG_ENTER(("GetCWD(%p, %d);\n", buf, len));

  pszCWD = getcwd(buf, len);	/* Get the physical dir */
  if (!pszCWD) goto fail;	/* There's no physical dir, so no logical one either */
  /* Known limitation: If the buffer is too small for the physical dir, but
     large enough for the logical dir, we'll fail, whereas we could have succeeded.
     This is mitigated by the increased use of NewCWDString(), which allocates huge bufs. */

  pszPWD = getenv("PWD");	/* Get the logical dir */
  if (!pszPWD) goto done;	/* Rare. Return the physical dir in buf */
  /* Most common case: If they're the same, no need to search any further */
  if (streq(pszCWD, pszPWD)) goto done; /* Return the physical=logical dir in buf */

  /* Both defined, but different. Check if they refer to the same physical dir */
  iErr = stat(pszCWD, &stPhys);
  if (iErr) goto fail;	/* Unlikely, but if so, there's no way to verify the PWD validity */
  iErr = stat(pszPWD, &stLog);
  if (iErr) goto done;	/* The PWD variable isn't valid. Return the physical dir in buf */
  /* Both stats valid. Compare at least the device ID and inode number */
  if ((stPhys.st_dev == stLog.st_dev) && (stPhys.st_ino == stLog.st_ino)) {
    /* The two refer to the same physical dir, so the logical one is valid */
    size_t l = strlen(pszPWD);
    if (l >= len) {	/* The buffer is too small */
      errno = ERANGE;
fail:
      RETURN_STRING_COMMENT(NULL, ("%s\n", strerror(errno)));
    }
    DEBUG_PRINTF(("getcwd() = \"%s\";\n", pszCWD));
    strcpy(buf, pszPWD);	/* Overwrite the physical dir */
  } /* Else the two do not match, so the PWD variable is invalid. Return the phys. dir in buf */

done:
  RETURN_STRING(buf);
}

/* Return a new string with the logical CWD */
char *NewCWDString(void) {
  char *pszBuf = NEW_PATHNAME_BUF();
  char *pszRet;
  if (!pszBuf) return NULL;
  pszRet = GetCWD(pszBuf, PATHNAME_BUF_SIZE);
  if (pszRet) {
    TRIM_PATHNAME_BUF(pszBuf);
    return pszBuf;
  } else {
    FREE_PATHNAME_BUF(pszBuf);
    return pszRet;
  }
}

/*---------------------------------------------------------------------------*\
*									      *
|   Function	    ChDir						      |
|									      |
|   Description	    Set the Unix logical current working directory	      |
|		    							      |
|   Arguments	    Same as the standard C library's chdir()                  |
|		    							      |
|   Return value    Same as the standard C library's chdir()                  |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History                                                                   |
|    2025-11-27 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int ChDir(const char *pszPath) {
  char *pszPWD = NULL;		/* The initial logical dir */
  char *pszPWD2 = NULL;		/* The new logical dir to change to */
  int iErr = -1;

  DEBUG_ENTER(("ChDir(\"%s\");\n", pszPath));

  if (!pszPath || !pszPath[0]) {
    errno = ENOENT;
fail:
    RETURN_INT_COMMENT(iErr, ("%s\n", strerror(errno)));
  }

  if (pszPath[0] != '/') { /* If the path is relative, build the new absolute logical path */
    pszPWD = NewCWDString();			/* Get the logical dir */
    if (!pszPWD) goto fail;
    pszPWD2 = NewCompactJoinedPath(pszPWD, pszPath);
    free(pszPWD);
    if (!pszPWD2) goto fail;
    pszPath = pszPWD2;
    DEBUG_PRINTF(("chdir(\"%s\");\n", pszPath)); /* Trace what dir is actually set */
  } /* Else the path is absolute. This will be the new logical path as well. */

  iErr = chdir(pszPath);
  if (!iErr) setenv("PWD", pszPath, TRUE);	/* Record the new logical dir */
  free(pszPWD2);
  DEBUG_CODE(
  if (iErr) goto fail; /* Only jump in DEBUG mode, to display a detailed error message */
  )
  RETURN_INT_COMMENT(iErr, ("Success\n"));
}

#endif /* defined(_UNIX) */

