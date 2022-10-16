/*****************************************************************************\
*									      *
*   File name:	    smbios.h						      *
*									      *
*   Description:    SMBIOS tables management definitions		      *
*									      *
*   Notes:	    Definitions for use with smbios.c.			      *
*									      *
*   History:								      *
*    1996-05-20 JFL Initial implementation of dmidump.c.		      *
*    2000-09-21 JFL Split this file out of smbios.c.			      *
*    2001-01-15 JFL Allow inclusion in WIN32 sources.			      *
*    2001-12-20 JFL Changed back constant name from $SMBIOS to $DMI.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*		    Added support for use of 32-bits tables in WIN64.         *
*		    Added preliminary support for SMBIOS 3.0 64-bits tables.  *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2020-04-19 JFL Define and use consistently DWORD_DEFINED & QWORD_DEFINED.*
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SMBIOS_H_
#define _SMBIOS_H_

#if HAS_SYSLIB

#include "SysLib.h"		/* SysLib Library core definitions */
#include "qword.h"		/* Defines BYTE, WORD, DWORD, QWORD, etc */

#else  /* !HAS_SYSLIB */

#ifndef DWORD_DEFINED
#define DWORD_DEFINED

/* Define basic types */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

/* Reference the Nth BYTE in a structure. (That is the BYTE at offset N) */
#define BYTE0(qw) (((BYTE *)(&(qw)))[0])
#define BYTE1(qw) (((BYTE *)(&(qw)))[1])
#define BYTE2(qw) (((BYTE *)(&(qw)))[2])
#define BYTE3(qw) (((BYTE *)(&(qw)))[3])
#define BYTE4(qw) (((BYTE *)(&(qw)))[4])
#define BYTE5(qw) (((BYTE *)(&(qw)))[5])
#define BYTE6(qw) (((BYTE *)(&(qw)))[6])
#define BYTE7(qw) (((BYTE *)(&(qw)))[7])

/* Reference the Nth WORD in a structure. (That is the WORD at offset 2N) */
#define WORD0(qw) (((WORD *)(&(qw)))[0])
#define WORD1(qw) (((WORD *)(&(qw)))[1])
#define WORD2(qw) (((WORD *)(&(qw)))[2])
#define WORD3(qw) (((WORD *)(&(qw)))[3])

/* Reference the Nth DWORD in a structure. (That is the DWORD at offset 4N) */
#define DWORD0(qw) (((DWORD *)(&(qw)))[0])
#define DWORD1(qw) (((DWORD *)(&(qw)))[1])

/* Reference a BYTE, WORD, DWORD, or QWORD, at offset N in a structure */
#define BYTE_AT(p, n) (*(BYTE *)((BYTE *)(p)+(int)(n)))
#define WORD_AT(p, n) (*(WORD *)((BYTE *)(p)+(int)(n)))
#define DWORD_AT(p, n) (*(DWORD *)((BYTE *)(p)+(int)(n)))

#endif /* !defined(DWORD_DEFINED) */

#ifndef QWORD_DEFINED

#ifdef _WIN32
#define QWORD_DEFINED
typedef unsigned __int64 QWORD;
#endif
#ifdef _MSDOS
#define QWORD_DEFINED
typedef struct {DWORD dw0; DWORD dw1;} QWORD;
#endif
#ifndef QWORD_DEFINED /* Any other operating system */
#define QWORD_DEFINED
#include <stdint.h>
typedef uint64_t QWORD;
#endif

#define QWORD_AT(p, n) (*(QWORD *)((BYTE *)(p)+(int)(n)))

#endif /* !defined(QWORD_DEFINED) */

#endif /* HAS_SYSLIB */

#pragma pack(1)

typedef struct
    {
    DWORD dwSignature;
    BYTE bVersion;
    BYTE bHeaderLength;
    LPVOID lpStructures;
    WORD wStructuresLength;
    BYTE bStructuresChecksum;
    BYTE bHeaderChecksum;
    } SMBIOSHEADER, FAR *LPSMBIOSHEADER;

typedef struct
    {
    BYTE bSignature[10];
    BYTE bVersion;
    struct
	{
	WORD wSize;
	WORD wOffset;		/* A null offset marks the end of the table */
	DWORD dwReserved;
	} table[1];
    } SMBIOS20HPHEADER, FAR *LPSMBIOS20HPHEADER;

typedef struct			/* 32-bits DMI 2.x header */
    {
    DWORD dwSignature;		/* 00 Initial _SM_ signature */
    BYTE bChecksum;		/* 04 Header checksum */
    BYTE bHeaderLength;		/* 05 Header length */
    BYTE bMajorVersion; 	/* 06 SMBIOS major version */
    BYTE bMinorVersion;		/* 07 SMBIOS minor version */
    WORD wMaxStructSize;	/* 08 Maximum structure size */
    BYTE bHeaderRevision;	/* 0A Header revision. 0=SMBIOS v2 */
    BYTE bFormatted[5]; 	/* 0B Reserved for SMBIOS 2.1 and 2.2 */
    char bSignature2[5];	/* 10 Intermediate _DMI_ signature */
    BYTE bChecksum2;		/* 15 Intermediate checksum */
    WORD wTotalSize;		/* 16 Total data size of all structures */
    DWORD dwPhysicalAddress;	/* 18 Physical address of the tables area */
    WORD wNumStructures;	/* 1C Number of structures */
    BYTE bcdRevision;		/* 1E Optional packed BCD SMBIOS version number */
    } SMBIOS21HEADER, *PSMBIOS21HEADER, FAR *LPSMBIOS21HEADER;

#ifdef QWORD_DEFINED
typedef struct			/* 64-bits DMI 3.x header */
    {
    char bSignature[5];		/* 00 Initial _SM3_ signature */
    BYTE bChecksum;		/* 05 Header checksum */
    BYTE bHeaderLength;		/* 06 Header length */
    BYTE bMajorVersion; 	/* 07 SMBIOS major version */
    BYTE bMinorVersion;		/* 08 SMBIOS minor version */
    BYTE bRevVersion;		/* 09 SMBIOS Doc revision */
    BYTE bHeaderRevision;	/* 0A Header revision. 1=SMBIOS v3 */
    BYTE bReserved;		/* 0B Reserved */
    DWORD dwMaxTotalSize;	/* 0C Size sufficient for all structures */
    QWORD qwPhysicalAddress;	/* 10 Physical address of the tables area */
    } SMBIOS30HEADER, FAR *LPSMBIOS30HEADER;
#endif

#if HAS_BIOSLIB && defined(_BIOS) /* Use PnP definitions in the BiosLib library */

#include "utildef.h"

#elif defined(_MSDOS)

/* PnP BIOS API */
typedef short (_cdecl far * LPPNPBIOS)(int iFunction, ...);

/* PnP BIOS header, located at a paragraph somewhere in segment F000. */
typedef struct
    {
    DWORD dwSignature;			/* Must be "$PnP". */
    BYTE bVersion;
    BYTE bHeaderLength;
    WORD wControl;
    BYTE bChecksum;
    LPVOID lpNotification;
    LPPNPBIOS lpRmEntry;
    WORD wPmEntryOffset;
    DWORD dwPmEntryBase;
    DWORD dwOemId;
    WORD wRmDS;
    DWORD dwPmDataBase;
    } PNPHEADER, FAR *LPPNPHEADER;

LPVOID FindHeader(DWORD dwExpected);	/* Locate a PnP BIOS-type header */

#endif /* defined(_MSDOS) */

/* Various signatures used to identify the type of header */
#define $PnP 0x506E5024 /* "$PnP" Signature for PnP BIOS and SMBIOS 2.0+ API */
#define $DMI 0x494D4424 /* "$DMI" Signature for SMBIOS 1.0 tables */
#define _DMI 0x494D445F /* "_DMI20_NT_" Signature for HP DMI 2.0 32-bits-RAM tables */
#define _SM_ 0x5F4D535F /* "_SM_" Signature for SMBIOS 2.1+ 32-bits-RAM-style tables */
#define _SM3 0x334D535F /* "_SM3_" Signature for SMBIOS 3.0+ 64-bits-RAM-style tables */

/* SMBIOS data structures */

#if defined(_MSC_VER)
#pragma warning(disable:4200) /* Avoid warnings "nonstandard extension used : zero-sized array in struct/union" */
#endif /* defined(_MSC_VER) */

typedef struct		/* Generic data structure */
    {
    BYTE bType;		/* Component ID Information  Indicator */
    BYTE bLength;	/* Length dependent on structure type and SMBIOS version */
    WORD wHandle;	/* Unique for each structure, starting at 0 */
    BYTE bData[];
    } SMBIOSSTRUCT, *PSMBIOSSTRUCT;

#if defined(_MSC_VER)
#pragma warning(default:4200)
#endif /* defined(_MSC_VER) */

typedef struct		/* System info structure #0x01 */
    {
    BYTE bType;		/* Component ID Information  Indicator */
    BYTE bLength;	/* Length dependent on version supported, 08h for 2.0 or 19h for 2.1 and later. */
    WORD wHandle;	/* Varies */
    BYTE bManufacturer;	/* Number of Null terminated string */
    BYTE bProductName;	/* Number of Null terminated string */
    BYTE bVersion;	/* Number of Null terminated string */
    BYTE bSerialNumber; /* Number of Null terminated string */
    BYTE bUUID[16];	/* Universal Unique ID number.  If the value is all FFh, the ID is not currently present in the system, but is settable.  If the value is all 00h, the ID is not present in the system. */
    BYTE bWakeUpType;	/* Identifies the event that caused the system to power up.  See SMBIOS 2.2 spec §3.2.2.1. */
    } SMBIOSSYSTEMINFO, *PSMBIOSSYSTEMINFO, FAR *LPSMBIOSSYSTEMINFO;

typedef struct		/* HP-proprietary structure #0x85 */
    {
    BYTE bType;
    BYTE bLength;
    WORD wHandle;
    BYTE bCard;		/* 0=None 1=Chanteclerc 2=Tweety 3=Woody 4=Sqeezy 5=Calimero */
    BYTE bConn1Caps;
    BYTE bConn1Use;
    BYTE bConn2Caps;
    BYTE bConn2Use;
    BYTE bConn3Caps;
    BYTE bConn3Use;
    BYTE bConn4Caps;
    BYTE bConn4Use;
    BYTE bMacAddress[6];
    BYTE bNProtocols;
    WORD wProt1Name;
    WORD wProt2Name;
    WORD wProt3Name;
    WORD wProt4Name;
    WORD wProt5Name;
    BYTE bLanStatus;
    BYTE bScsiStatus;
    } SMBIOSLANINFO, *PSMBIOSLANINFO, FAR *LPSMBIOSLANINFO;

#pragma pack()

#ifdef _WIN32 /* Windows provides SMBIOS data with this header */

#pragma warning(disable:4200) /* Avoid warnings "nonstandard extension used : zero-sized array in struct/union" */

typedef struct _RAWSMBIOSDATA {
  BYTE    Used20CallingMethod;
  BYTE    SMBIOSMajorVersion;
  BYTE    SMBIOSMinorVersion;
  BYTE    DmiRevision;
  DWORD   Length;
  BYTE    SMBIOSTableData[];
} RAWSMBIOSDATA, *PRAWSMBIOSDATA;

#pragma warning(default:4200)

#endif /* _WIN32 */

/*****************************************************************************/

/* Prototypes for routines in smbios.c */

/* Private functions. Use only for testing. */
LPVOID FindHeader(DWORD dwExpected);
int SmBiosInit(SMBIOS21HEADER *pSmBiosFound, DWORD dwPreferred);
LPBYTE SmBiosGetDataAddress(SMBIOS21HEADER *p21Hdr);

/* Public functions */
typedef SMBIOS21HEADER *HSMBIOS;
HSMBIOS SmBiosOpen();
void SmBiosClose(HSMBIOS hSMBIOS);
#define SmBiosVersion(hSMBIOS) ((hSMBIOS->bMajorVersion << 8) | hSMBIOS->bMinorVersion)
#define SmBiosMaxStructSize(hSMBIOS) ((int)(hSMBIOS->wMaxStructSize))
void *SmBiosAllocStruct(HSMBIOS hSMBIOS);
int SmBiosGetStructByHandle(HSMBIOS hSMBIOS, int hStruct, void *pBuf);
int SmBiosGetStructByType(HSMBIOS hSMBIOS, int iType, void *pBuf);
char *SmBiosGetString(HSMBIOS hSMBIOS, void *pStruct, unsigned int n);
char *SmBiosGetStringAt(HSMBIOS hSMBIOS, void *pStruct, unsigned int n);
int SmBiosGetFullStructureSize(LPVOID lpStruct);
int SmBiosIsHpPc(HSMBIOS hSMBIOS);

#endif /* _SMBIOS_H_ */
