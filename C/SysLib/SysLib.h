/*****************************************************************************\
*                                                                             *
*   Filename 	    SysLib.h						      *
*									      *
*   Description     SysLib Library core definitions			      *
*                                                                             *
*   Notes	    Included indirectly by the include files that need it.    *
*		    Do not include directly.				      *
*									      *
*   History								      *
*    2016-04-12 JFL Created this file.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __SYSLIB_H__   /* Prevent multiple inclusions */
#define __SYSLIB_H__

/* Force linking with the SysLib.lib library */
#if defined(_MSC_VER)
#define _SYSLIB_LIB "SysLib.lib"
#pragma message("Adding pragma comment(lib, \"" _SYSLIB_LIB "\")")
#pragma comment(lib, _SYSLIB_LIB)
#endif /* defined(_MSC_VER) */

#if defined(__unix__)
#define _SYSLIB_LIB "libSysLib.a"
#endif

#endif /* defined(__SYSLIB_H__) */
