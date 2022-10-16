/*****************************************************************************\
*                                                                             *
*   Filename:	    NetBIOS.h						      *
*									      *
*   Description:    OS-independant NetBIOS access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    The Win32 implementation is trivial.		      *
*									      *
*   History:								      *
*    1995-05-30 JFL Adapted from "Windows Network Programming" book.	      *
*    2000-09-21 JFL Restructured.					      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _NETBIOS_H_
#define _NETBIOS_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _WIN32

#include <windows.h>
#include <nb30.h>

/* Trivial as everything is defined in Windows.h */

#endif /* _WIN32 */

/*****************************************************************************\
*                                                                             *
*                               MSDOS Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _MSDOS

#define TRUE 1
#define FALSE 0

#include "qword.h"		/* Defines BYTE, WORD, DWORD, QWORD, etc */

typedef int BOOL;
typedef void (far *FARPROC)();

/* Include files from the Lan Manager development kit. */
/* Note: Including the WIN32 "nb30.h" would work almost as well. */
#include <netcons.h>
#pragma warning(disable:4115) /* Avoid warnings "named type definition in parentheses" */
#include <netbios.h>
#pragma warning(default:4115) /* restore warnings "named type definition in parentheses" */
#include <ncb.h>
#include <neterr.h>

#define HIWORD(dw) ((WORD)(((DWORD)(dw)) >> 16))
#define LOWORD(dw) ((WORD)(dw))

typedef NCB far *LPNCB;

#define NB_INTERRUPT	0x5C		/* NetBIOS software interrupt */

#define MAXNAMES	20		/* Maximum names to can display */
#define NETBIOSBUFSIZE (sizeof(struct _astatbuf) + MAXNAMES*sizeof (struct _namebuf))

#define UNINITIALIZED   0		/* States of the remote name buffer */
#define AADDRESS        1
#define NAME            2

#ifdef __cplusplus
extern "C" {
#endif

API_FUNCTION NetBios(LPNCB lpNCB);
BOOL IsNetBiosInstalled(void);

#ifdef __cplusplus
}
#endif

#endif /* _MSDOS */

/*****************************************************************************\
*									      *
*				Common to both				      *
*									      *
\*****************************************************************************/

#endif /* _NETBIOS_H_ */