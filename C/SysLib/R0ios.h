/*****************************************************************************\
*                                                                             *
*   Filename:	    R0IOS.h						      *
*									      *
*   Description:    Definitions for R0IOS.c (IOS.VxD access from WIN32).      *
*                                                                             *
*   Notes:	    Compilable both in C and C++ without any warning.	      *
*		    							      *
*   History:								      *
*    2001-10-01 JFL Created this file.					      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _R0IOS_H_   // Prevent multiple inclusions
#define _R0IOS_H_

#include "SysLib.h"		/* SysLib Library core definitions */

#pragma pack(1)

#include <windef.h>	// Windows types
#include <vmm.h>	// General Ring0 access mechanism
#include <ios.h>	// IOS functions
#include <blockdev.h>	// Block device structures
#include <isp.h>	// IOS services
#include "dcb.h"	// ~~jfl Modified 98DDK's version to avoid conflict with winbase.h own DCB.

// Routines prototypes

#ifdef __cplusplus
extern "C" {
#endif

DWORD R0IosSendCommand(_BlockDev_Command_Block *pCmdData, _BlockDev_Device_Descriptor *pDevData);
_BlockDev_Device_Descriptor *R0IosFindInt13Drive(DWORD dwUnitNumber);
_BlockDev_Device_Descriptor *R0IosGetDeviceList(void);
WORD R0IosService(PISP pIsp);
IOSDCB *R0IosFindDcb(BYTE bDeviceType, BYTE bUnitNumber);

#ifdef __cplusplus
}
#endif

#pragma pack()	/* Restore the default packing size */

#endif // defined(_R0IOS_H_)
