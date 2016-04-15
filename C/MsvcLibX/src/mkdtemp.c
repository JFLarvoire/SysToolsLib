#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

char *mkdtemp(char *pszName) {
  char *pszXXX, *pc;
  int iErr = 0;
  char *base32 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
  int i;

  /* Find the XXX placeholder at the end of the string */
  for (pszXXX=pszName; pszXXX && *pszXXX; pszXXX++) ;
  while ((pszXXX > pszName) && (*(pszXXX-1) == 'X')) pszXXX--;
  if (!pszXXX || (*pszXXX != 'X')) {
    errno = EINVAL;
    return NULL;
  }

  /* Seed the random number generator */
  srand((unsigned)getpid() + ((unsigned)time(NULL) << 10));

  /* Try random file names until one file gets successfully created */
  for (i=0; i<10; i++) {
    for (pc = pszXXX; *pc; pc++) {
      *pc = base32[rand() % 32];
    }
    iErr = _mkdir(pszName);
    if (!iErr) break;
  }

  return iErr ? NULL : pszName;
}

