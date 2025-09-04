/*****************************************************************************\
*                                                                             *
*   Filename	    dict.c						      *
*									      *
*   Description     Dictionary management procedures			      *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2024-06-16 JFL jf.larvoire@free.fr Created this module.                  *
*    2025-08-10 JFL Adapted to dict.h changes.				      *
*                                                                             *
*                   © Copyright 2024 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _GNU_SOURCE 	  /* Else string.h does not define strdup() in Unix */
#include <string.h>

/* SysToolsLib include files */
#include "debugm.h"	  /* SysToolsLib debugging macros */

#define DICT_DEFINE_PROCS /* Generate the dictionary management functions */
#include "dict.h"	  /* Dictionary management definitions */

