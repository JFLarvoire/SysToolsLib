/*****************************************************************************\
*                                                                             *
*   File name	    mainutil.h						      *
*                                                                             *
*   Description	    Main C program utilility routines			      *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-12-15 JFL Created this file.					      *
*                                                                             *
\*****************************************************************************/

#ifndef _SYSLIB_MAINUTIL_H_
#define _SYSLIB_MAINUTIL_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#define streq(s1, s2) (!strcmp(s1, s2))

int pferror(char *pszFormat, ...);	/* Print error messages on stderr, in a standardized format */

#endif /* _SYSLIB_MAINUTIL_H_ */
