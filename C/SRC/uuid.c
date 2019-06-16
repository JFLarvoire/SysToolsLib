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
*    2017-06-29 JFL Fixed a warning. No functional code change.		      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.0.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.0.2.      *
*		    							      *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage UUIDs"
#define PROGRAM_NAME    "uuid"
#define PROGRAM_VERSION "1.0.2"
#define PROGRAM_DATE    "2019-06-12"

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using sprintf and sscanf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

DEBUG_GLOBALS	/* Define global variables used by our debugging macros */

#include "uuid.h"

#define streq(s1, s2) (!strcmp(s1, s2))

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
	puts(DETAILED_VERSION);
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
        RPC_CSTR pszString = NULL;
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

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
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
\n\
Author: Jean-Francois Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");
}
