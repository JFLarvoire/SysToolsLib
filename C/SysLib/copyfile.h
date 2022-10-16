/*****************************************************************************\
*                                                                             *
*   Filename        copyfile.h                                                *
*                                                                             *
*   Description     Definitions for file copying routines                     *
*                                                                             *
*   Notes           							      *
*                                                                             *
*   History                                                                   *
*    2020-11-05 JFL Created this file.                                        *
*                                                                             *
*         © Copyright 2020 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _COPYFILE_H_
#define _COPYFILE_H_

#include "SysLib.h"		/* SysLib Library core definitions */

int copydate(const char *pszToFile, const char *pszFromFile); /* Copy the file dates */

#endif /* _COPYFILE_H_ */
