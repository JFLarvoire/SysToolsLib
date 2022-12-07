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
*    2022-11-29 JFL Tweaks and fixes for BIOS/LODOS/DOS builds compatibility. *
*		    							      *
*       (C) Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __SYSLIB_H__   /* Prevent multiple inclusions */
#define __SYSLIB_H__

#define SYSLIB_STRINGIZE(s) #s
#define SYSLIB_VALUEIZE(s) SYSLIB_STRINGIZE(s)

/* Force linking with the SysLib.lib library */
#if defined(_MSC_VER)

#define _SYSLIB_LIB "SysLib.lib"
#pragma message("SysLib.h: #pragma comment(lib, \"" _SYSLIB_LIB "\")")
#pragma comment(lib, _SYSLIB_LIB)

#if defined(_BIOS) || defined(_LODOS)
#define SYSLIBCCC _fastcall /* The default C Calling Convention used for BIOS & LODOS versions of SysLib */
#else
#define SYSLIBCCC _cdecl /* The default C Calling Convention used for other versions of SysLib, including DOS */
#endif
#ifndef CDECL
#define CDECL _cdecl
#endif
#ifndef FASTCALL
#define FASTCALL _fastcall
/* Note that using _fastcall in SysLib causes a warning
   C4124: __fastcall with stack checking is inefficient */
#endif

#else /* GCC, clang, etc. */

#define _SYSLIB_LIB "libSysLib.a"

#define SYSLIBCCC
#ifndef CDECL
#define CDECL
#endif
#ifndef FASTCALL
#define FASTCALL
#endif

#endif

#endif /* defined(__SYSLIB_H__) */
