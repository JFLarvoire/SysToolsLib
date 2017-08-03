/*****************************************************************************\
*                                                                             *
*   Filename:	    Int13.h						      *
*									      *
*   Description:    Data structures used by BIOS int 13H service routines     *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2001-06-06 JFL Created this file.					      *
*    2017-07-28 JFL Added prototypes for BIOS disk I/O routines.	      *
*    2017-08-02 JFL Added common int 13 errors.				      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _INT13_H_
#define _INT13_H_

#include "qword.h"	/* Defines BYTE, WORD, DWORD, QWORD, etc */

#pragma pack(1)

/* Common BIOS int 13 errors. Many others exist. */
#define INT_13_ERR_INVALID_FUNCTION       0x01
#define INT_13_ERR_ADDRESS_MARK_NOT_FOUND 0x02
#define INT_13_ERR_DISK_WRITE_PROTECTED   0x03
#define INT_13_ERR_SECTOR_NOT_FOUND       0x04
#define INT_13_ERR_RESET_FAILED           0x05
#define INT_13_ERR_DISK_CHANGED           0x06
#define INT_13_ERR_CONTROLLER_FAILURE     0x20
#define INT_13_ERR_NO_MEDIA_IN_DRIVE      0x31

/* Hard disk parameters (Pointed by int 41H and int 42H vectors) */
typedef struct {
  WORD wCyls;		/* 00h    WORD    number of cylinders */
  BYTE bHeads;		/* 02h    BYTE    number of heads */
  BYTE bSignature;	/* 03h    BYTE    A0h (signature indicating translated table) */
  BYTE bPSects;		/* 04h    BYTE    number of physical sectors per track */
  WORD wPrecomp;	/* 05h    WORD    starting write precompensation cylinder number */
  BYTE bECCBurst;	/* 07h    BYTE    maximum ECC burst length (XT only) */
  BYTE bFlags;		/* 08h    BYTE    control byte */
  WORD wPCyls;		/* 09h    WORD    number of physical cylinders */
  BYTE bPHeads;		/* 0Bh    BYTE    number of physical heads */
  WORD wLZone;		/* 0Ch    WORD    cylinder number of landing zone */
  BYTE bSects;		/* 0Eh    BYTE    number of logical sectors per track */
  BYTE bChecksum;	/* 0Fh    BYTE    checksum */
} HDD_PARMS;

/* EDD address packet (Used by int 13H functions 42H to 47H) */
typedef struct {
  BYTE bSize;		/* Packet size, in bytes. >=16. */
  BYTE bReserved1;	/* Reserved. Must be 0. */
  BYTE bNumber;		/* Number of sectors to transfer */
  BYTE bReserved2;	/* Reserved. Must be 0. */
  DWORD dwBuffer;	/* Buffer address */
  QWORD qwLBA;		/* Logical Block Address */
/*  QWORD qwBuf64;	// Optional 64-bit buffer address */
} EDDPACKET;

/* Hard disk parameters (Returned by int 13H function 48H) */
typedef struct {
  WORD wIoBase;		/* physical I/O port base address */
  WORD wAddress;	/* disk-drive control port address */
  BYTE bFlags;		/* drive flags */
  BYTE bProp;		/* proprietary information */
			/* bits 7-4 reserved (0) */
			/* bits 3-0: Phoenix proprietary (used by BIOS) */
  BYTE bIrq;		/* IRQ (bits 3-0; bits 7-4 reserved and must be 0) */
  BYTE bSectors;	/* sector count for multi-sector transfers */
  BYTE bDma;		/* DMA control */
			/* bits 7-4: DMA type (0-2) as per ATA-2 specification */
			/* bits 3-0: DMA channel */
  BYTE bPio;		/* programmed I/O control */
			/* bits 7-4: reserved (0) */
			/* bits 3-0: PIO type (1-4) as per ATA-2 specification */
  WORD wOptions;	/* drive options */
  WORD wReserved;	/* reserved (0) */
  BYTE bRevision;	/* extension revision level (high nybble=major, low nybble=minor) */
		 	/* (currently 10h for v1.0 and 11h for v1.1) */
  BYTE bChecksum;	/* 2's complement checksum of bytes 00h-0Eh */
		 	/* 8-bit sum of all bytes 00h-0Fh should equal 00h */
} EDDPARMS;

typedef struct {
  WORD wSize;   	/* (call) size of buffer (001Ah for v1.x, 001Eh for v2.x) */
			/* (ret) size of returned data */
  WORD wInfo;   	/* information flags (see #0184) */
  DWORD dwCyls; 	/* number of physical cylinders on drive */
  DWORD dwHeads;	/* number of physical heads on drive */
  DWORD dwSects;	/* number of physical sectors per track */
  QWORD qwTotal;	/* total number of sectors on drive */
  WORD wBpS;		/* bytes per sector */
  EDDPARMS FAR *lpedd;	/* Pointer to an EDDPARMS table */
} DDPARMS;

#pragma pack()

/* Prototypes for BIOS disk I/O routines */
int _cdecl GetBiosDiskChsParameters(int iDisk, DDPARMS far *lpBuf, int iSize);
int _cdecl GetBiosDiskParameterTable(int iDisk, DDPARMS far *lpBuf, int iSize);
int _cdecl BiosDiskReadChs(WORD wDrive, WORD wCyl, WORD wHead, WORD wSect, WORD n, char far *lpcBuffer);
int _cdecl BiosDiskWriteChs(WORD wDrive, WORD wCyl, WORD wHead, WORD wSect, WORD n, char far *lpcBuffer);
int BiosDiskReadLba(int iDrive, QWORD qwSector, WORD wNum, void far *lpBuf);
int BiosDiskWriteLba(int iDrive, QWORD qwSector, WORD wNum, void far *lpBuf);

#endif /* _INT13_H_ */
