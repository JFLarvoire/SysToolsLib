/*****************************************************************************\
*                                                                             *
*   Filename:	    stringx.h						      *
*									      *
*   Description:    Case-insensitive string management routines		      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2012-02-02 JFL Created this file.					      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _STRINGX_H_
#define _STRINGX_H_

#include "SysLib.h"		/* SysLib Library core definitions */

char *stristr(const char *pszString, const char *pszSearch);
size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace);

#endif // ifndef _STRINGX_H_
