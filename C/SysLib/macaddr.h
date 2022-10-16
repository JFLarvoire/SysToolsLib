/*****************************************************************************\
*                                                                             *
*   Filename:	    macaddr.h						      *
*									      *
*   Description:    OS-independant MAC address query routine		      *
*                                                                             *
*   Notes:	    							      *
*		    							      *
*   History:								      *
*    2016-04-24 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _MACADDR_H

#include "syslib.h"

#include "qword.h"	/* Defines BYTE, WORD, DWORD, QWORD, etc */

int GetMacAddress(BYTE *pbMacAddr);

#define MACADDRESS_SIZE 6   /* Network cards MAC addresses are 6-bytes long */

#endif _MACADDR_H
