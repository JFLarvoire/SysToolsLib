/*****************************************************************************\
*                                                                             *
*   Filename:	    uuid.h						      *
*									      *
*   Description:    OS-independant UUID definitions			      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2000-09-21 JFL Created this file, based on genuuid.c.		      *
*    2001-01-15 JFL Changed data types to Windows-compatible BYTE/WORD/DWORD. *
*                   Made compatible with windows.h.			      *
*    2002-01-03 JFL Added macro function uuidcmp().			      *
*		    Changed IsNullUuid() argument to (void *).		      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2016-04-26 JFL Changed the unsigned64_t type to SysLib's QWORD.	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _UUID_H_
#define _UUID_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/* UUID generation types */

#include "qword.h"		/* Defines BYTE, WORD, DWORD, QWORD, etc */

#define CLOCK_SEQ_LAST              0x3FFF
#define RAND_MASK                   CLOCK_SEQ_LAST

#ifdef uuid_t
#undef uuid_t	/* Defined in Window's rpcdce.h */
#endif

typedef struct _uuid_t {
    DWORD	time_low;
    WORD	time_mid;
    WORD	time_hi_and_version;
    BYTE	clock_seq_hi_and_reserved;
    BYTE	clock_seq_low;
    BYTE	node[6];
} uuid_t, *puuid_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSDOS
static void get_system_time(QWORD *uuid_time);
static void mult32(DWORD u, DWORD v, QWORD *result);

void uuid_init(void);
void uuid_create(uuid_t *uuid);
#endif /* defined(_MSDOS) */

#ifdef _WIN32
#include <rpc.h>	/* Defines WIN32 UUID management routines */
#define uuid_init();
#define uuid_create(puid) UuidCreate((UUID*)(puid))
#endif /* defined(_WIN32) */

int GetPcUuid(uuid_t *pBuf);
int PrintUuid(uuid_t *pUuid);
int IsNullUuid(uuid_t *pUuid);
#define uuidcmp(uuid1, uuid2) memcmp((uuid1), (uuid2), sizeof(uuid_t))

#ifdef __cplusplus
}
#endif

#endif /* _UUID_H_ */
