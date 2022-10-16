/*****************************************************************************\
*                                                                             *
*   Filename:	    crc32.h						      *
*									      *
*   Description:    CRC-32 computation routines				      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2001-02-20 JFL Created this file.					      *
*    2001-02-26 JFL Changed crc32 param 2 from size_t to int.		      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "SysLib.h"		/* SysLib Library core definitions */

#include "qword.h"		/* Defines BYTE, WORD, DWORD, QWORD, etc */

/* CRC-32 computation */
extern DWORD crc32(void FAR *pBuf, int iLen);
