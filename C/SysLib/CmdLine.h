/*****************************************************************************\
*                                                                             *
*   Filename	    CmdLine.h						      *
*									      *
*   Description     Command-line arguments management routines		      *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2023-04-12 JFL jf.larvoire@free.fr created this file.                    *
*                                                                             *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _CMDLINE_H_
#define _CMDLINE_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern char *CondQuoteShellArg(char *pszArg);
extern char *DupArgLineTail(int argc, char **argv, int iArg0);

#ifdef __cplusplus
}
#endif

#endif /* _CMDLINE_H_ */

