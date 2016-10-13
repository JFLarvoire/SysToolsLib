/*****************************************************************************\
*                                                                             *
*   Filename:	    uuid.c						      *
*									      *
*   Description:    Test SysLib's UUID management functions		      *
*                                                                             *
*   Notes:	    							      *
*		    							      *
*   History:								      *
*    2016-04-24 JFL Created this file.                     		      *
*    2016-07-12 JFL Added option -V.                     		      *
*		    							      *
\*****************************************************************************/

#define PROGRAM_VERSION "1.0"
#define PROGRAM_DATE    "2016-07-12"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugm.h"
DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#include "uuid.h"

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#if defined(__MINGW64__)
#define OS_NAME "MinGW64"
#elif defined(__MINGW32__)
#define OS_NAME "MinGW32"
#elif defined(_WIN64)
#define OS_NAME "Win64"
#else
#define OS_NAME "Win32"
#endif

#endif

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS applic. */

#ifndef _BIOS
#define OS_NAME "DOS"
#else
#define OS_NAME "BIOS"
#endif

#endif

/********************** End of OS-specific definitions ***********************/

#define streq(s1, s2) (!strcmp(s1, s2))

char *version(void);
void usage(void);

typedef enum {
  AGET,
  ANEW
} action_t;

int main(int argc, char *argv[]) {
  uuid_t uuid;
  int i;
  action_t action = ANEW;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (arg[0] == '-') {
      char *opt = arg+1;
      if (streq(opt, "?")) {
	usage();
	exit(0);
      }
      DEBUG_CODE(
	if (streq(opt, "d")) {
	  DEBUG_ON();
	  printf("Debug mode.\n");
	  continue;
	}
      )
      if (streq(opt, "s")) {
	action = AGET;
	continue;
      }
      if (streq(opt, "n")) {
	action = ANEW;
	continue;
      }
      if (streq(argv[i]+1, "V")) {
	printf("%s\n", version());
	exit(0);
      }
      printf("Unexpected option: %s\nIgnored.\n", arg);
      continue;
    }
    printf("Unexpected argument: %s\nIgnored.\n", arg);
  }

  switch (action) {
  case ANEW:
    uuid_create(&uuid);
    PrintUuid(&uuid);
    break;
  case AGET:
    if (GetPcUuid(&uuid)) {	/* Found it */
      PrintUuid(&uuid);
#ifdef _WIN32 /* Compare the output of our print routine to that of WIN32 */
      DEBUG_CODE({
        char *pszString = NULL;
        UuidToString((GUID *)&uuid, &pszString);
        printf(" (WIN32 result: %s)", pszString);
        RpcStringFree(&pszString);
      })
#endif
    } else {			/* Did not find it */
      printf("Error: Did not find the PC UUID.\n");
    }
    break;
  }

  return 0;
}

char *version(void) {
  return (PROGRAM_VERSION
	  " " PROGRAM_DATE
	  " " OS_NAME
	  DEBUG_VERSION
	  );
}

void usage(void) {
  printf ("\
Usage: uuid [OPTIONS]\n\
\n\
options:\n"
#if _DEBUG
"\
  -d      Debug mode\n"
#endif
"\
  -n      Create a new UUID (Default)\n\
  -s      Get the SMBIOS system UUID\n\
");
}
